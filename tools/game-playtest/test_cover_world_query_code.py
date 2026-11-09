"""Actual CoverQuery bracket with retained code and synthetic process memory."""
import copy
import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import cover_world_query_code as code
from cover_state import CoverQuery
from test_cover_surface_query import SurfaceMemory


class WorldQueryCodeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # This private artifact is required for the original-byte regression;
        # absence is explicit rather than replacing it with invented bytes.
        if not Path(code.surface.DEFAULT_BLOB).is_file():
            raise unittest.SkipTest('Retained original runtime code artifact unavailable')
        cls.original = code._historical()

    def fixture(self):
        f = SurfaceMemory()
        f.module['sha256'] = code.surface.GAME_SHA256
        for rva, raw, _ in self.original:
            f.put(f.BASE + rva, raw)
        q = CoverQuery(f.observer)
        q.r.profile = dict(q.r.profile, sha256=code.surface.GAME_SHA256)
        return f, q

    def observe(self, f, q):
        with patch.object(code, '_historical', return_value=self.original):
            return code.observe(q, f.module)

    def test_full_original_ranges_include_missing_continuation(self):
        self.assertEqual(sum(len(raw) for _, raw, _ in self.original), 3550)
        rva, raw, _ = self.original[-1]
        self.assertEqual(rva, 0xcfa40)
        self.assertEqual(len(raw), 1129)
        self.assertEqual(hashlib.sha256(raw[0xcfd58 - rva:]).hexdigest(),
                         'b1466e9577b878a43a57c8fa90634ae5f61239879e485471224a92d99347007d')

    def test_same_query_records_and_closes_every_complete_range(self):
        f, q = self.fixture()
        result = self.observe(f, q)
        for rva, raw, _ in self.original:
            self.assertEqual(q.values[f.BASE + rva, len(raw)], raw)
        self.assertTrue(result['closing_required'])
        self.assertTrue(result['code_observed'])
        for key in ('engine_calls', 'virtual_calls', 'dynamic_dispatch_verified',
                    'initial_overlap_verified', 'sweep_verified',
                    'world_synchronization_verified', 'activation_allowed'):
            self.assertFalse(result[key], key)
        q.finish()

    def test_each_original_range_mismatch_is_rejected(self):
        for rva, raw, expected in self.original:
            with self.subTest(rva=hex(rva)):
                f, q = self.fixture(); changed = bytearray(raw); changed[-1] ^= 1
                f.put(f.BASE + rva, changed)
                with self.assertRaises(code.CodeRejected) as caught:
                    self.observe(f, q)
                witness = caught.exception.source_witness
                self.assertEqual(witness['rva'], hex(rva))
                self.assertEqual(witness['expected_sha256'], expected)
                self.assertFalse(witness['closing_verified'])
                self.assertFalse(witness['activation_allowed'])

    def test_change_inside_previously_missing_tail_rejects(self):
        f, q = self.fixture(); rva, raw, _ = self.original[-1]
        changed = bytearray(raw); changed[0xcfd77 - rva] ^= 1
        f.put(f.BASE + rva, changed)
        with self.assertRaises(code.CodeRejected) as caught:
            self.observe(f, q)
        self.assertEqual(caught.exception.source_witness['rva'], '0xcfa40')

    def test_continuation_change_after_observation_rejects_closing(self):
        f, q = self.fixture(); self.observe(f, q)
        rva, raw, _ = self.original[-1]; changed = bytearray(raw); changed[-1] ^= 1
        f.put(f.BASE + rva, changed)
        with self.assertRaisesRegex(RuntimeError, 'changed'):
            q.finish()

    def test_wrong_module_reader_and_disk_identity_reject(self):
        for defect in range(3):
            with self.subTest(defect=defect):
                f, q = self.fixture()
                if defect == 0:
                    f.module['base'] += 0x1000
                elif defect == 1:
                    f.module['sha256'] = '0' * 64
                else:
                    q.r.profile = dict(q.r.profile, sha256='0' * 64)
                with self.assertRaisesRegex(RuntimeError, 'identity'):
                    self.observe(f, q)

    def test_nonexecutable_or_short_module_rejects(self):
        for defect in range(2):
            f, q = self.fixture(); f.module = copy.deepcopy(f.module)
            if defect:
                f.module['size'] = 0xcfd58
            else:
                f.module['sections'][0]['characteristics'] &= ~0x20000000
            with self.assertRaisesRegex(RuntimeError, 'bounds|range'):
                self.observe(f, q)

    def test_bad_historical_file_rejects(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'wrong.bin'; path.write_bytes(b'not original code')
            with self.assertRaisesRegex(RuntimeError, 'historical'):
                code._historical(path)


if __name__ == '__main__':
    unittest.main()
