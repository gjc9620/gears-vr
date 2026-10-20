"""Read-only menu observation for reversible XR display; no game control."""
import os
import json
import re
from pathlib import Path
import struct
import sys
import time
import hashlib

PACKET = struct.Struct('<IIIIQQQQQQ')
UNKNOWN, GAMEPLAY, DEATH_MENU, PAUSE_MENU, MAIN_MENU = range(5)
LOADING_DISPLAY = 5
ACQUISITION_WAIT = 6
MOVIE_DISPLAY = 7
# Runtime35804 matched historical artifact1. No game code is executed.
MOVIE_CODE = (
    (0x434e24,0xf8,'87debeb3f8cf80bb64605af2531f7785b68aa3d46e0b9d7da00ac78f0e2c2387'),
    (0x318e64,2182,'7523c77773a6d2e89d7753744f1503f7763fa33d1a8f87a8900d3c3bfc153423'),
    (0x319854,1024,'09758a99b7c7a04e2cdb0f4012e917caeba5a2b5849c84fb6b11b524abf911b3'),
)


def attested_movie_reader(reader, game):
    from movie_state import MovieProgressReader
    resolver=reader.r
    if resolver.base != 0x140000000:raise RuntimeError('Movie module base mismatch')
    for rva,size,digest in MOVIE_CODE:
        raw=resolver.read(resolver.base+rva,size)
        if len(raw)!=size or hashlib.sha256(raw).hexdigest()!=digest:
            raise RuntimeError('Movie runtime code identity mismatch')
    return MovieProgressReader(resolver.read,resolver.base,dict(pid=game['pid'],creation_time=game['creation_time'],
        module_base=hex(resolver.base),code_sha256=[entry[2] for entry in MOVIE_CODE]))
PAUSE_SETTINGS = frozenset(('DeltaPause-Options', 'DeltaGameConfiguration', 'DeltaVideoSettings',
                            'PenderGraphicsSettings', 'DeltaAudioVideoSettings', 'DeltaInputSettings'))


def state_evidence(sample):
    """Own only already-read scalar identities/gates and bounded scene chains."""
    if not isinstance(sample, dict):return None
    def scalar(value):
        return value if value is None or type(value) in (bool, int, float, str) else None
    def scene(value):
        if not isinstance(value, dict):return None
        result = {key: scalar(value[key]) for key in (
            'address', 'name', 'number', 'cls', 'outer', 'input_mode', 'tag', 'tag_number') if key in value}
        if isinstance(value.get('states'), list):
            result['states'] = [scalar(item) for item in value['states']]
        return result
    result = {key: scalar(sample.get(key)) for key in (
        'read_only', 'consistent_reread', 'input_block_count', 'player_index')}
    result['top'] = scene(sample.get('top'))
    result['focused_leaf'] = scene(sample.get('focused_leaf'))
    for key, limit, tail in (('active_scenes', 32, True), ('focus_chain', 16, False)):
        values = sample.get(key)
        result[key] = [scene(value) for value in (values[-limit:] if tail else values[:limit])] if isinstance(values, list) else None
        result[key+'_total'] = len(values) if isinstance(values, list) else None
        result[key+'_truncated'] = isinstance(values, list) and len(values) > limit
        # Active-scene tail preserves the observed top; focus-chain prefix
        # preserves its root. The focused leaf is separately retained above.
        result[key+'_selection'] = 'tail' if tail else 'prefix'
    return result


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
    # VDXR session 30768 (2026-10-20) retains these parents while selecting a
    # different chapter. The child pages use modal input_mode=1, unlike their
    # mode=4 parents. UNKNOWN would revoke WGC until gameplay resumes.
    chain = [('DeltaMainMenu', 'WarUIScene_MainMenu', 4),
             ('DeltaCampaignType', 'WarUIScene_CampaignType', 4),
             ('DeltaCampaignMenu_ContinueGame', 'WarUIScene_Campaign', 4),
             ('DeltaSelectAct', 'WarUIScene', 1),
             ('DeltaSelectMap', 'WarUIScene', 1),
             ('DeltaSelectDifficulty', 'WarUIScene', 1),
             ('WarfareConfirmation', 'UIScene_Confirmation', 1)]
    name = top.get('tag')
    depth = next((i+1 for i, (scene_name, _, _) in enumerate(chain) if scene_name == name), 0)
    if not depth:
        return False
    scenes = active_top(sample, top)
    if scenes is None or len(scenes) < depth:return False
    selected = scenes[-depth:]
    for scene, (scene_name, cls, mode) in zip(selected, chain[:depth]):
        if (not isinstance(scene, dict) or scene.get('name') != scene_name or scene.get('cls') != cls or
                type(scene.get('input_mode')) is not int or scene['input_mode'] != mode):
            return False
    if depth > 3:
        # Authenticate the full observed parent chain. A similarly named page
        # elsewhere, a partial/truncated stack or mixed scene clients is not
        # display authority. No focus, dialog choice or game input is admitted.
        if (sample.get('active_scenes_truncated', False) is not False or
                sample.get('focus_chain_truncated', False) is not False):return False
        addresses = []
        for scene in selected:
            address, outer = scene.get('address'), scene.get('outer')
            if (not isinstance(address, str) or re.fullmatch(r'0x[0-9a-fA-F]{1,16}', address) is None or
                    int(address, 16) == 0 or not isinstance(outer, str) or
                    re.fullmatch(r'0x[0-9a-fA-F]{1,16}', outer) is None or int(outer, 16) == 0 or
                    outer != top.get('outer') or type(scene.get('number')) is not int or scene['number'] < 0):
                return False
            addresses.append(int(address, 16))
        if len(set(addresses)) != depth:return False
    if top.get('name') != name or type(top.get('input_mode')) is not int:return False
    return (type(top.get('number')) is int and top['number'] >= 0 and
            isinstance(top.get('outer'), str) and bool(top['outer']) and
            type(scenes[-1].get('number')) is int and
            all(top.get(key) == scenes[-1].get(key) for key in ('number', 'outer')))


def loading_display(sample):
    """Exact observed blocked nonmodal stack; generic display only, never input."""
    if (sample.get('read_only') is not True or sample.get('consistent_reread') is not True
            or type(sample.get('input_block_count')) is not int or sample['input_block_count'] not in (1, 2)
            or type(sample.get('player_index')) is not int or sample['player_index'] != 0
            or sample.get('active_scenes_truncated', False) is not False
            or sample.get('focus_chain_truncated', False) is not False):
        return False
    core = ('address', 'name', 'number', 'cls', 'outer')
    def pointer(value):
        return (type(value) is str and re.fullmatch(r'0x[0-9a-fA-F]{1,16}', value) is not None
                and int(value, 16) > 0)
    def identity(value):
        return (isinstance(value, dict) and pointer(value.get('address')) and pointer(value.get('outer'))
                and type(value.get('number')) is int and value['number'] >= 0
                and type(value.get('name')) is str and bool(value['name'])
                and type(value.get('cls')) is str and bool(value['cls']))
    top = sample.get('top')
    scenes = sample.get('active_scenes')
    focus = sample.get('focus_chain')
    leaf = sample.get('focused_leaf')
    if (not identity(top) or top.get('tag') != 'PenderCrossplayInvitePopUp'
            or type(top.get('tag')) is not str or type(top.get('tag_number')) is not int
            or top['tag_number'] != 0 or not isinstance(scenes, list) or len(scenes) != 3
            or not isinstance(focus, list) or len(focus) != 1):
        return False
    expected = (('HUD_Toast', 'WarUIScene_Toast', 0), ('MessageToast', 'WarUIScene_Toast', 0),
                ('PenderCrossplayInvitePopUp', 'WarUIScene_CrossplayInvitePopUp', 2))
    for scene, (name, cls, mode) in zip(scenes, expected):
        if (not identity(scene) or (scene['name'], scene['cls']) != (name, cls)
                or type(scene.get('input_mode')) is not int or scene['input_mode'] != mode
                or scene['outer'] != top['outer']):
            return False
    if (len({scene['address'] for scene in scenes}) != 3
            or type(top.get('input_mode')) is not int or top['input_mode'] != 2
            or any(top.get(key) != scenes[-1].get(key) for key in (*core, 'input_mode'))):
        return False
    for focused in (focus[0], leaf):
        if (not identity(focused) or any(focused.get(key) != top.get(key) for key in core)
                or type(focused.get('states')) is not list or focused['states'] != ['UIState_Enabled']):
            return False
    return True


def classify(sample):
    if sample.get('consistent_reread') is not True or sample.get('read_only') is not True:
        return UNKNOWN
    if loading_display(sample):return LOADING_DISPLAY
    blocked = sample.get('input_block_count')
    # Outside the exact loading topology above, block1 permits only authenticated
    # Pause/main-menu display. Neither loading block1/2 nor other block1 states
    # grant input permission or gameplay/death-menu recovery permission.
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
    def __init__(self, path, game, owner, identity, reader_factory=None, reader_identity=None, clock=time.perf_counter_ns,*,movie_display=False):
        # Keep the real exception identities for injected factories as well.
        # Importing the types does not construct a Resolver or open a process.
        sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'game-io'))
        from menu_state import SnapshotChanged, DialogStorageChanged, InventoryChanged, RegistrySnapshotChanged
        self.snapshot_change_types = (SnapshotChanged, DialogStorageChanged, InventoryChanged, RegistrySnapshotChanged)
        self.snapshot_changed_type = SnapshotChanged
        if reader_factory is None:
            from menu_state import MenuReader
            from menu_control import reader_process_identity
            reader_factory, reader_identity = lambda: MenuReader(game['pid']), reader_process_identity
        self.path, self.game, self.owner = Path(path), game, owner
        self.identity, self.factory, self.reader_identity, self.clock = identity, reader_factory, reader_identity, clock
        self.reader, self.sequence, self.retry_ns, self.last = None, 0, 0, None
        if type(movie_display) is not bool:raise ValueError('movie_display must be boolean')
        self.movie_enabled=movie_display;self.movie_reader=None
        self.last_menu_mode=UNKNOWN
        self.publish_failed = False
        self.log = self.path.with_suffix('.jsonl').open('w', encoding='utf-8')

    def publish(self, mode, observed, reason, evidence=None):
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
        # Retain the observed block2 -> block1 loading transition without
        # changing packet publication or other modes' log deduplication.
        if mode == LOADING_DISPLAY and isinstance(evidence, dict) and loading_display(evidence):
            key = mode, reason, evidence['input_block_count']
        if key != self.last:
            self.log.write(json.dumps(dict(sequence=self.sequence, observed_ns=observed, mode=mode,
                reason=reason, state_evidence=state_evidence(evidence),
                **({'movie_progress':evidence['movie_progress']} if isinstance(evidence,dict) and 'movie_progress' in evidence else {})))+'\n')
            self.log.flush()
            self.last = key
        return True

    def poll(self):
        observed = self.clock()  # Begin acquisition, never extend a slow read's lease.
        sample = None
        sampling = False
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
                if self.movie_enabled:
                    self.movie_reader=attested_movie_reader(self.reader,self.game)
                    if self.reader_identity(self.reader) != (self.game['pid'],self.game['creation_time']):
                        raise RuntimeError('Movie reader process identity changed after attestation')
            sampling = True
            sample = self.reader.sample()
            sampling = False
            if sample.get('pid') != self.game['pid']:
                raise RuntimeError('Menu sample PID mismatch')
            mode = classify(sample)
            reason = (sample.get('top') or {}).get('tag', 'unknown')
        except (RuntimeError, OSError, ValueError, UnicodeError) as error:
            sample = None  # A failed acquisition never carries old/partial state evidence.
            mode = ACQUISITION_WAIT if sampling and type(error) is self.snapshot_changed_type else UNKNOWN
            reason = type(error).__name__+': '+str(error)
            # Only these exact acquisition-churn exceptions preserve the reader.
            # Exact sample-stage SnapshotChanged permits only a generic waiting
            # display choice. It conveys no acquired game-image/input authority.
            # The next poll acquires a whole sample with its own start timestamp.
            if type(error) not in self.snapshot_change_types:
                if self.reader:
                    self.reader.close();self.reader = None
                self.movie_reader=None
                self.retry_ns = self.clock()+1000000000
        self.last_menu_mode=mode
        # Movie progress is independent of transient UI ledger churn. It uses
        # only the still-owned, identity-checked process reader; no cached UI
        # sample can authorize it, and explicit acquired menus take precedence.
        if self.movie_enabled and self.reader and self.movie_reader:
            if self.reader_identity(self.reader)!=(self.game['pid'],self.game['creation_time']) or self.identity(self.game['pid'])!=self.game:
                self.publish(UNKNOWN,observed,'movie_process_identity_changed')
                raise RuntimeError('Movie reader process identity changed')
            movie=self.movie_reader.sample(observed)
            if movie['state']=='progress' and mode not in (MAIN_MENU,DEATH_MENU,PAUSE_MENU):
                mode=MOVIE_DISPLAY;reason='verified_movie_progress'
                # Do not extend a progress receipt when only its hold applies.
                observed=min(observed,movie['last_advance_ns'])
                sample=dict(sample or {},movie_progress=movie)
        self.publish(mode, observed, reason, evidence=sample)

    def close(self):
        try:
            self.publish(UNKNOWN, self.clock(), 'stopped')
        finally:
            if self.reader:self.reader.close();self.reader = None
            self.log.close()
