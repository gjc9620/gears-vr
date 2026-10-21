import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from run import ProtectedFiles


class ProtectedFileTests(unittest.TestCase):
    def test_test_changes_and_exact_original_restored(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path = root / 'profile'
            path.write_bytes(b'original\x00binary')
            store = ProtectedFiles([path], root / 'backup', lambda: None)
            store.replace(path, b'test fixture')
            receipts = store.restore()
            self.assertEqual(path.read_bytes(), b'original\x00binary')
            self.assertEqual((root / 'backup/0.after').read_bytes(), b'test fixture')
            self.assertEqual(receipts[0]['restored_sha256'], hashlib.sha256(path.read_bytes()).hexdigest())

    def test_live_game_blocks_replace_and_restore(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path = root / 'profile'
            path.write_bytes(b'original')
            alive = False
            def stopped():
                if alive:
                    raise RuntimeError('game alive')
            store = ProtectedFiles([path], root / 'backup', stopped)
            alive = True
            with self.assertRaisesRegex(RuntimeError, 'alive'):
                store.replace(path, b'test')
            with self.assertRaisesRegex(RuntimeError, 'alive'):
                store.restore()
            self.assertEqual(path.read_bytes(), b'original')

    def test_corrupt_backup_never_replaces_profile(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path = root / 'profile'
            path.write_bytes(b'original')
            store = ProtectedFiles([path], root / 'backup', lambda: None)
            store.replace(path, b'current')
            (root / 'backup/0').write_bytes(b'corrupted')
            with self.assertRaisesRegex(RuntimeError, 'hash changed'):
                store.restore()
            self.assertEqual(path.read_bytes(), b'current')

    def test_unknown_external_change_is_not_overwritten_during_preparation(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path = root / 'profile'
            path.write_bytes(b'original')
            store = ProtectedFiles([path], root / 'backup', lambda: None)
            path.write_bytes(b'external change')
            with self.assertRaisesRegex(RuntimeError, 'outside this test'):
                store.replace(path, b'fixture')
            self.assertEqual(path.read_bytes(), b'external change')


if __name__ == '__main__':
    unittest.main()
