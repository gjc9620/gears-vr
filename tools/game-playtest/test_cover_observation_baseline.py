from pathlib import Path
import tempfile
import unittest

import cover_observation_baseline as baseline


class RestorationTest(unittest.TestCase):
    def test_exact_bytes_absences_and_file_sets_restore(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            private = root / 'private'
            private.mkdir()
            original = private / 'save.bin'
            original.write_bytes(b'original\x00\xff')
            public = root / 'public.bin'
            public.write_bytes(b'public')
            absent = private / 'absent.bin'
            proof = baseline.capture([(public, True), (absent, False)], [private], root/'backup', root)
            original.write_bytes(b'game changed save')
            absent.write_bytes(b'new explicitly tracked file')
            extra = private / 'new-game-save.bin'
            extra.write_bytes(b'new unlisted game save')
            result = baseline.restore(proof)
            self.assertTrue(result['all_bytes_and_absences_match'])
            self.assertEqual(original.read_bytes(), b'original\x00\xff')
            self.assertEqual(public.read_bytes(), b'public')
            self.assertFalse(absent.exists())
            self.assertFalse(extra.exists())

    def test_changed_read_only_prevents_all_writes(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            writable, protected = root/'owned', root/'protected'
            writable.write_bytes(b'owned before')
            protected.write_bytes(b'user before')
            proof = baseline.capture([(writable, False), (protected, True)], [], root/'backup', root)
            writable.write_bytes(b'owned after')
            protected.write_bytes(b'user after')
            with self.assertRaises(RuntimeError):
                baseline.restore(proof)
            self.assertEqual(writable.read_bytes(), b'owned after')
            self.assertEqual(protected.read_bytes(), b'user after')

    def test_corrupt_later_backup_prevents_earlier_restore(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            first, last = root/'first', root/'last'
            first.write_bytes(b'first before')
            last.write_bytes(b'last before')
            proof = baseline.capture([(first, False), (last, False)], [], root/'backup', root)
            first.write_bytes(b'first after')
            Path(proof['files'][1]['saved']).write_bytes(b'bad backup')
            with self.assertRaises(ValueError):
                baseline.restore(proof)
            self.assertEqual(first.read_bytes(), b'first after')

    def test_unexpected_subdirectory_is_not_recursively_deleted(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            private = root/'private'
            private.mkdir()
            proof = baseline.capture([], [private], root/'backup', root)
            new = private/'unexpected'
            new.mkdir()
            (new/'keep').write_bytes(b'keep')
            with self.assertRaises(ValueError):
                baseline.restore(proof)
            self.assertEqual((new/'keep').read_bytes(), b'keep')


if __name__ == '__main__':
    unittest.main()
