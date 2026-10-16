"""Read-only menu observation for reversible XR display; no game control."""
import os
import json
from pathlib import Path
import struct
import sys
import time

PACKET = struct.Struct('<IIIIQQQQQQ')
UNKNOWN, GAMEPLAY, DEATH_MENU, PAUSE_MENU, MAIN_MENU = range(5)
PAUSE_SETTINGS = frozenset(('DeltaPause-Options', 'DeltaGameConfiguration', 'DeltaVideoSettings',
                            'PenderGraphicsSettings', 'DeltaAudioVideoSettings', 'DeltaInputSettings'))


def scene_context(scene, name, cls='WarUIScene'):
    return (isinstance(scene, dict) and scene.get('name') == name and scene.get('cls') == cls
            and type(scene.get('input_mode')) is int and scene['input_mode'] == 1)


def active_top(sample, top):
    scenes = sample.get('active_scenes')
    if not isinstance(scenes, list) or not scenes or not isinstance(scenes[-1], dict):
        return None
    last = scenes[-1]
    if (not isinstance(top.get('address'), str) or not top['address'] or
            type(last.get('input_mode')) is not int or
            any(top.get(key) != last.get(key) for key in ('address', 'name', 'cls', 'input_mode'))):
        return None
    return scenes


def pause_settings(sample, top):
    name = top.get('tag')
    if not isinstance(name, str) or name not in PAUSE_SETTINGS or not scene_context(top, name):return False
    scenes = active_top(sample, top)
    if scenes is None:return False
    pause_seen = False
    for scene in scenes[:-1]:
        if scene_context(scene, 'DeltaPause-Campaign-Host'):pause_seen = True
        if pause_seen and scene_context(scene, 'DeltaPause-Options') and name != 'DeltaPause-Options':return True
    return pause_seen and name == 'DeltaPause-Options'


def pause_return_confirmation(sample, top):
    if (top.get('tag') != 'WarfareConfirmation' or
            not scene_context(top, 'WarfareConfirmation', 'UIScene_Confirmation') or
            sample.get('dialog_text') not in ({
                'Title':'<Strings:WarfareGame.Generic.ConfirmSurrender_Title>',
                'MessageText':'<Strings:WarfareGame.Generic.ConfirmSurrender>'}, {
                'Title':'<Strings:WarfareGame.Generic.ConfirmCheckPointLoad_Title>',
                'MessageText':'<Strings:WarfareGame.Generic.ConfirmCheckPointLoad>'})):
        return False
    scenes = active_top(sample, top)
    return scenes is not None and any(scene_context(scene, 'DeltaPause-Campaign-Host') for scene in scenes[:-1])


def main_menu(sample, top):
    # Observed campaign chain only: explicit whole-window 2D mirror, never a
    # claim about source image format. Other pages need separate evidence.
    chain = [('DeltaMainMenu', 'WarUIScene_MainMenu'),
             ('DeltaCampaignType', 'WarUIScene_CampaignType'),
             ('DeltaCampaignMenu_ContinueGame', 'WarUIScene_Campaign')]
    name = top.get('tag')
    depth = next((i+1 for i, (scene_name, _) in enumerate(chain) if scene_name == name), 0)
    if not depth:
        return False
    scenes = active_top(sample, top)
    if scenes is None or len(scenes) < depth:return False
    for scene, (scene_name, cls) in zip(scenes[-depth:], chain[:depth]):
        if (not isinstance(scene, dict) or scene.get('name') != scene_name or scene.get('cls') != cls or
                type(scene.get('input_mode')) is not int or scene['input_mode'] != 4):
            return False
    if top.get('name') != name or type(top.get('input_mode')) is not int:return False
    return (type(top.get('number')) is int and top['number'] >= 0 and
            isinstance(top.get('outer'), str) and bool(top['outer']) and
            type(scenes[-1].get('number')) is int and
            all(top.get(key) == scenes[-1].get(key) for key in ('number', 'outer')))


def classify(sample):
    if sample.get('consistent_reread') is not True or sample.get('read_only') is not True:
        return UNKNOWN
    blocked = sample.get('input_block_count')
    # The observed block1 transition permits only authenticated Pause or main-menu display.
    # It is not input permission or gameplay/death-menu recovery permission.
    if type(blocked) is not int or blocked not in (0, 1):
        return UNKNOWN
    # MenuReader reports UIScene.LastPlayerIndex, not a LocalPlayer identity.
    # The persistent nonmodal scene also reports -1 during real gameplay.
    player = sample.get('player_index')
    if type(player) is not int:
        return UNKNOWN
    top = sample.get('top') or {}
    if not isinstance(top, dict):return UNKNOWN
    if player == 0 and main_menu(sample, top):
        return MAIN_MENU
    if blocked == 0 and player == 0 and (top.get('tag'), top.get('cls'), top.get('input_mode')) == ('DeltaGameover', 'WarUIScene', 1):
        return DEATH_MENU
    # Exact observed campaign-host Pause only. Selection can change within the
    # same modal scene, so a particular focused option is not its identity.
    if (player == 0 and type(top.get('input_mode')) is int and
            (top.get('tag'), top.get('cls'), top.get('input_mode')) ==
            ('DeltaPause-Campaign-Host', 'WarUIScene', 1)):
        return PAUSE_MENU
    # Only observed settings scenes under their ordered campaign Pause context.
    if player == 0 and pause_settings(sample, top):
        return PAUSE_MENU
    # Display only: the exact observed surrender/checkpoint prompts under Pause
    # are not authenticated input choices. Title/message pairs cannot mix;
    # no generic confirmation/dialog admission or input permission is granted.
    if player == 0 and pause_return_confirmation(sample, top):
        return PAUSE_MENU
    # This persistent nonmodal scene was observed atop gameplay; it is not
    # sufficient to render stereo. Native identity and XR ticket gates still apply.
    if blocked == 0 and player in (-1, 0) and (top.get('name'), top.get('cls'), top.get('input_mode')) == (
            'PenderCrossplayInvitePopUp', 'WarUIScene_CrossplayInvitePopUp', 2):
        return GAMEPLAY
    return UNKNOWN


class MenuDisplayMonitor:
    def __init__(self, path, game, owner, identity, reader_factory=None, reader_identity=None, clock=time.perf_counter_ns):
        # Keep the real exception identities for injected factories as well.
        # Importing the types does not construct a Resolver or open a process.
        sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'game-io'))
        from menu_state import SnapshotChanged, DialogStorageChanged, InventoryChanged
        self.snapshot_change_types = (SnapshotChanged, DialogStorageChanged, InventoryChanged)
        if reader_factory is None:
            from menu_state import MenuReader
            from menu_control import reader_process_identity
            reader_factory, reader_identity = lambda: MenuReader(game['pid']), reader_process_identity
        self.path, self.game, self.owner = Path(path), game, owner
        self.identity, self.factory, self.reader_identity, self.clock = identity, reader_factory, reader_identity, clock
        self.reader, self.sequence, self.retry_ns, self.last = None, 0, 0, None
        self.publish_failed = False
        self.log = self.path.with_suffix('.jsonl').open('w', encoding='utf-8')

    def publish(self, mode, observed, reason):
        self.sequence += 1
        data = PACKET.pack(0x47564d31, 1, PACKET.size, mode,
                           self.game['pid'], self.game['creation_time'], self.owner['pid'],
                           self.owner['creation_time'], self.sequence, observed)
        temporary = self.path.with_suffix('.tmp')
        try:
            temporary.write_bytes(data)
            os.replace(temporary, self.path)
        except OSError as error:
            # Preserve the previous packet and its acquisition timestamp. The
            # next poll must acquire a new sample before attempting publication.
            try:
                temporary.unlink(missing_ok=True)
            except OSError:
                pass
            if not self.publish_failed:
                self.log.write(json.dumps(dict(event='publish_failed', sequence=self.sequence,
                    observed_ns=observed, error=type(error).__name__, errno=error.errno,
                    winerror=getattr(error, 'winerror', None)))+'\n')
                self.log.flush()
            self.publish_failed = True
            return False
        if self.publish_failed:
            self.log.write(json.dumps(dict(event='publish_recovered', sequence=self.sequence,
                                          observed_ns=observed))+'\n')
            self.log.flush()
            self.publish_failed = False
        key = mode, reason
        if key != self.last:
            self.log.write(json.dumps(dict(sequence=self.sequence, observed_ns=observed, mode=mode, reason=reason))+'\n')
            self.log.flush()
            self.last = key
        return True

    def poll(self):
        observed = self.clock()  # Begin acquisition, never extend a slow read's lease.
        if self.identity(self.game['pid']) != self.game:
            self.publish(UNKNOWN, observed, 'game_identity_changed')
            raise RuntimeError('Menu observer game process changed')
        try:
            if self.reader is None:
                if observed < self.retry_ns:
                    self.publish(UNKNOWN, observed, 'reader_retry');return
                self.reader = self.factory()
                if self.reader_identity(self.reader) != (self.game['pid'], self.game['creation_time']):
                    raise RuntimeError('Menu reader process identity changed')
            sample = self.reader.sample()
            if sample.get('pid') != self.game['pid']:
                raise RuntimeError('Menu sample PID mismatch')
            mode = classify(sample)
            reason = (sample.get('top') or {}).get('tag', 'unknown')
        except (RuntimeError, OSError, ValueError, UnicodeError) as error:
            mode, reason = UNKNOWN, type(error).__name__+': '+str(error)
            # Only these exact acquisition-churn exceptions preserve the reader.
            # Publish no display authority; the next ordinary poll must acquire
            # a whole new sample using its own acquisition-start timestamp.
            if type(error) not in self.snapshot_change_types:
                if self.reader:
                    self.reader.close();self.reader = None
                self.retry_ns = self.clock()+1000000000
        self.publish(mode, observed, reason)

    def close(self):
        try:
            self.publish(UNKNOWN, self.clock(), 'stopped')
        finally:
            if self.reader:self.reader.close();self.reader = None
            self.log.close()
