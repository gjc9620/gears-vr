"""Read-only movie progress evidence for an explicitly identified runtime.

Offsets are from movie-state-35804 runtime evidence, not a generic
IsMoviePlaying declaration. The caller attests the exact process/module identity
and supplies read(address, size) -> bytes and monotonic timestamps. This module
opens no process, scans no memory, calls no engine code and authorizes no pixels.
"""
import json
import struct

SINGLETON_RVAS = (0x1b661f0, 0x1c5f6d8)
VTABLE_RVA = 0x166bd30
UINT_MAX = 0xffffffff
HOLD_NS = 250_000_000


class MovieProgressReader:
    def __init__(self, read, module_base, identity):
        if not isinstance(module_base, int) or not 0 < module_base < 0x7fff00000000 or module_base % 4096:
            raise ValueError('Expected exact aligned module base')
        if not isinstance(identity, dict) or not identity:
            raise ValueError('Expected caller-attested process/module identity')
        self.identity = json.loads(json.dumps(identity, allow_nan=False))
        self.read, self.base = read, module_base
        self._previous = self._last_advance = self._last_sample = None
        self._progress = False

    def _read(self, address, size):
        raw = self.read(address, size)
        if not isinstance(raw, bytes) or len(raw) != size:
            raise ValueError('Short or invalid read')
        return raw

    def _pointer(self, address):
        return struct.unpack('<Q', self._read(address, 8))[0]

    def _reset(self):
        self._previous = self._last_advance = None
        self._progress = False

    def sample(self, now_ns):
        result = dict(state='unknown', evidence='movie_progress', pixels_authorized=False,
                      identity=self.identity.copy(), module_base=hex(self.base),
                      observed_ns=now_ns, consistent_reread=False)
        if not isinstance(now_ns, int) or isinstance(now_ns, bool) or now_ns < 0:
            raise ValueError('Expected nonnegative monotonic nanoseconds')
        if self._last_sample is not None and now_ns <= self._last_sample:
            self._reset()
            result['reason'] = 'nonmonotonic_time'
            return result
        self._last_sample = now_ns
        try:
            globals_ = tuple(self._pointer(self.base + rva) for rva in SINGLETON_RVAS)
            pointer = globals_[0]
            if pointer != globals_[1] or not pointer or pointer % 8 or pointer > 0x7fffffffff80:
                raise ValueError('singleton_mismatch_or_invalid')
            vtable = self._pointer(pointer)
            if vtable != self.base + VTABLE_RVA:
                raise ValueError('vtable_mismatch')
            first = self._read(pointer + 0x60, 24)
            second = self._read(pointer + 0x60, 24)
            frame, total, _, handle = struct.unpack('<IIQQ', first)
            frame2, total2, _, handle2 = struct.unpack('<IIQQ', second)
            if (self._pointer(pointer) != vtable or
                    tuple(self._pointer(self.base + rva) for rva in SINGLETON_RVAS) != globals_ or
                    handle != handle2 or total != total2 or frame2 < frame):
                raise ValueError('snapshot_changed')
            result.update(object=hex(pointer), handle=hex(handle), frame=frame2,
                          total_frames=total, consistent_reread=True)
            if handle == 0 and frame == frame2 == UINT_MAX:
                self._reset()
                result.update(state='inactive', reason='inactive_sentinel')
                return result
            if not handle or handle % 8 or handle > 0x7fffffffffff or not 0 < total < UINT_MAX or not frame <= frame2 <= total:
                raise ValueError('invalid_handle_or_frame_bounds')
        except (OSError, RuntimeError, ValueError, struct.error) as error:
            self._reset()
            result.update(state='unknown', reason=str(error), consistent_reread=False)
            return result

        key = (pointer, handle, total)
        previous = self._previous
        self._previous = (key, frame2)
        if previous is None or previous[0] != key or frame2 < previous[1]:
            self._progress = False
            self._last_advance = None
            result['reason'] = 'acquiring'
        elif frame2 > previous[1]:
            self._progress = True
            self._last_advance = now_ns
            result.update(state='progress', reason='frame_advanced')
        elif self._progress and now_ns - self._last_advance <= HOLD_NS:
            result.update(state='progress', reason='recent_advance')
        else:
            self._progress = False
            result['reason'] = 'awaiting_advance'
        result['last_advance_ns'] = self._last_advance
        return result
