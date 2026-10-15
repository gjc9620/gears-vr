"""Read-only menu observation for reversible XR display; no game control."""
import os
from pathlib import Path
import struct
import sys
import time

PACKET = struct.Struct('<IIIIQQQQQQ')
UNKNOWN, GAMEPLAY, DEATH_MENU = range(3)


def classify(sample):
    if (sample.get('consistent_reread') is not True or sample.get('read_only') is not True
            or sample.get('player_index') != 0 or sample.get('input_block_count') != 0):
        return UNKNOWN
    top = sample.get('top') or {}
    if (top.get('tag'), top.get('cls'), top.get('input_mode')) == ('DeltaGameover', 'WarUIScene', 1):
        return DEATH_MENU
    # This persistent nonmodal scene was observed atop gameplay; it is not
    # sufficient to render stereo. Native identity and XR ticket gates still apply.
    if (top.get('name'), top.get('cls'), top.get('input_mode')) == (
            'PenderCrossplayInvitePopUp', 'WarUIScene_CrossplayInvitePopUp', 2):
        return GAMEPLAY
    return UNKNOWN


class MenuDisplayMonitor:
    def __init__(self, path, game, owner, identity, reader_factory=None, reader_identity=None, clock=time.perf_counter_ns):
        if reader_factory is None:
            sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'game-io'))
            from menu_state import MenuReader
            from menu_control import reader_process_identity
            reader_factory, reader_identity = lambda: MenuReader(game['pid']), reader_process_identity
        self.path, self.game, self.owner = Path(path), game, owner
        self.identity, self.factory, self.reader_identity, self.clock = identity, reader_factory, reader_identity, clock
        self.reader, self.sequence, self.retry_ns, self.last = None, 0, 0, None
        self.log = self.path.with_suffix('.jsonl').open('w', encoding='utf-8')

    def publish(self, mode, observed, reason):
        import json
        self.sequence += 1
        data = PACKET.pack(0x47564d31, 1, PACKET.size, mode,
                           self.game['pid'], self.game['creation_time'], self.owner['pid'],
                           self.owner['creation_time'], self.sequence, observed)
        temporary = self.path.with_suffix('.tmp')
        temporary.write_bytes(data)
        os.replace(temporary, self.path)
        key = mode, reason
        if key != self.last:
            self.log.write(json.dumps(dict(sequence=self.sequence, observed_ns=observed, mode=mode, reason=reason))+'\n')
            self.log.flush()
            self.last = key

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
