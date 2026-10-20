"""Offline movie progress tests; no process handles, input or engine calls."""
import json
import struct
import unittest

from movie_state import MovieProgressReader, SINGLETON_RVAS, VTABLE_RVA, UINT_MAX


class FakeReader:
    base, object = 0x140000000, 0x20000

    def __init__(self):
        self.memory, self.calls, self.change = {}, [], None
        for rva in SINGLETON_RVAS:
            self.put(self.base + rva, struct.pack('<Q', self.object))
        self.put(self.object, struct.pack('<Q', self.base + VTABLE_RVA))
        self.frame(10)

    def put(self, address, data):
        self.memory[address] = data

    def frame(self, frame, total=1370, handle=0x80000):
        self.put(self.object + 0x60, struct.pack('<IIQQ', frame, total, 0, handle))

    def read(self, address, size):
        self.calls.append((address, size))
        raw = self.memory[address]
        return self.change(address, size, raw) if self.change else raw


class MovieStateTests(unittest.TestCase):
    def setUp(self):
        self.fake = FakeReader()
        self.reader = MovieProgressReader(self.fake.read, self.fake.base,
                                          dict(pid=123, creation_time=456, runtime_sha256='test'))

    def sample(self, ms):
        result = self.reader.sample(ms * 1_000_000)
        json.dumps(result, allow_nan=False)
        self.assertFalse(result['pixels_authorized'])
        return result

    def test_requires_cross_sample_advance_and_bounded_reads(self):
        self.assertEqual(self.sample(0)['state'], 'unknown')
        self.assertEqual(len(self.fake.calls), 8)
        self.fake.frame(11)
        self.assertEqual(self.sample(33)['state'], 'progress')
        self.assertLessEqual(max(size for _, size in self.fake.calls), 24)

    def test_repeat_expiry_and_recovery(self):
        self.sample(0)
        self.fake.frame(11)
        self.sample(1)
        self.assertEqual(self.sample(251)['state'], 'progress')
        self.assertEqual(self.sample(252)['state'], 'unknown')
        self.fake.frame(12)
        self.assertEqual(self.sample(253)['state'], 'progress')

    def test_static_candidate_never_becomes_progress(self):
        for ms in (0, 10, 250, 1000):
            result = self.sample(ms)
            self.assertEqual(result['state'], 'unknown')
            self.assertTrue(result['consistent_reread'])

    def test_backward_handle_total_and_object_changes_reacquire(self):
        for change in ('backward', 'handle', 'total', 'object'):
            with self.subTest(change=change):
                self.setUp(); self.sample(0)
                self.fake.frame(11); self.sample(1)
                if change == 'backward': self.fake.frame(1)
                elif change == 'handle': self.fake.frame(12, handle=0x90000)
                elif change == 'total': self.fake.frame(12, total=89)
                else:
                    old = self.fake.object
                    self.fake.object = 0x30000
                    self.fake.put(self.fake.object, self.fake.memory[old])
                    for rva in SINGLETON_RVAS:
                        self.fake.put(self.fake.base + rva, struct.pack('<Q', self.fake.object))
                    self.fake.frame(12)
                self.assertEqual(self.sample(2)['state'], 'unknown')
                self.fake.frame(13, total=89 if change == 'total' else 1370,
                                handle=0x90000 if change == 'handle' else 0x80000)
                self.assertEqual(self.sample(3)['state'], 'progress')

    def test_inactive_sentinel_clears_history(self):
        self.sample(0); self.fake.frame(11); self.sample(1)
        self.fake.frame(UINT_MAX, UINT_MAX, 0)
        self.assertEqual(self.sample(2)['state'], 'inactive')
        self.fake.frame(12)
        self.assertEqual(self.sample(3)['state'], 'unknown')

    def test_disagreement_vtable_bounds_and_read_failure_fail_closed(self):
        for fault in ('global', 'vtable', 'bounds', 'zero_handle', 'short', 'unreadable'):
            with self.subTest(fault=fault):
                self.setUp(); self.sample(0)
                self.fake.frame(11); self.sample(1)
                if fault == 'global': self.fake.put(self.fake.base + SINGLETON_RVAS[1], bytes(8))
                elif fault == 'vtable': self.fake.put(self.fake.object, bytes(8))
                elif fault == 'bounds': self.fake.frame(1371)
                elif fault == 'zero_handle': self.fake.frame(12, handle=0)
                elif fault == 'short': self.fake.change = lambda address, size, raw: raw[:-1]
                else:
                    def fail(*args): raise OSError('unreadable')
                    self.fake.change = fail
                result = self.sample(2)
                self.assertEqual(result['state'], 'unknown')
                self.assertFalse(result['consistent_reread'])

    def test_bracket_changes_rejected(self):
        for field in ('pointer', 'vtable', 'handle', 'total', 'frame'):
            with self.subTest(field=field):
                self.setUp()
                calls = {}
                def changed(address, size, raw):
                    calls[address] = calls.get(address, 0) + 1
                    if calls[address] == 2:
                        if field == 'pointer' and address == self.fake.base + SINGLETON_RVAS[0]: return bytes(8)
                        if field == 'vtable' and address == self.fake.object: return bytes(8)
                        if address == self.fake.object + 0x60:
                            values = list(struct.unpack('<IIQQ', raw))
                            index = {'frame': 0, 'total': 1, 'handle': 3}.get(field)
                            if index is not None:
                                values[index] += -1 if field == 'frame' else 8
                                return struct.pack('<IIQQ', *values)
                    return raw
                self.fake.change = changed
                result = self.sample(0)
                self.assertEqual(result['state'], 'unknown')
                self.assertFalse(result['consistent_reread'])
                self.assertEqual(result['reason'], 'snapshot_changed')
                self.assertFalse(self.sample(1)['state'] == 'progress')

    def test_forward_frame_within_bracket_still_requires_second_sample(self):
        calls = 0
        def advancing(address, size, raw):
            nonlocal calls
            if address == self.fake.object + 0x60:
                calls += 1
                return struct.pack('<IIQQ', 10 + calls, 1370, 0, 0x80000)
            return raw
        self.fake.change = advancing
        self.assertEqual(self.sample(0)['state'], 'unknown')
        self.assertEqual(self.sample(33)['state'], 'progress')

    def test_nonmonotonic_time_invalidates_progress(self):
        self.sample(10); self.fake.frame(11); self.sample(20)
        self.assertEqual(self.sample(19)['reason'], 'nonmonotonic_time')
        self.fake.frame(12)
        self.assertEqual(self.sample(21)['state'], 'unknown')


if __name__ == '__main__':
    unittest.main()
