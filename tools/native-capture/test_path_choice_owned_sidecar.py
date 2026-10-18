"""Real Windows temporary-file ownership and exclusive-handle tests; no game."""
import os
from pathlib import Path
import tempfile
import unittest

from path_choice_owned_sidecar import OwnedSidecar, OwnershipError


@unittest.skipUnless(os.name == 'nt', 'Windows handle semantics')
class OwnedSidecarTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / 'position.ini'
        self.file = OwnedSidecar(self.path)

    def test_complete_publication_renewal_preserves_bytes_id_and_deletes_exact_file(self):
        raw = b'[PathChoicePositionTrial]\nnonce=123\n'
        self.file.publish(raw)
        identity = self.file.file_id; before = self.path.stat().st_mtime_ns
        self.file.renew()
        self.assertEqual(self.path.read_bytes(), raw)
        self.assertEqual(self.file.file_id, identity)
        self.assertGreater(self.path.stat().st_mtime_ns, before)
        self.file.remove(); self.assertFalse(self.path.exists()); self.assertFalse(self.file.owned)

    def test_unknown_final_is_preserved_and_no_partial_temp_remains(self):
        self.path.write_bytes(b'foreign')
        with self.assertRaises(OSError):
            self.file.publish(b'owned')
        self.assertEqual(self.path.read_bytes(), b'foreign')
        self.assertFalse(self.file.owned)
        self.assertEqual(list(self.path.parent.glob('*.tmp')), [])

    def test_same_bytes_replacement_is_preserved_by_file_identity(self):
        self.file.publish(b'owned')
        original = self.path.with_suffix('.original')
        self.path.rename(original); self.path.write_bytes(b'owned')
        with self.assertRaises(OwnershipError):self.file.renew()
        with self.assertRaises(OwnershipError):self.file.remove()
        self.assertEqual(self.path.read_bytes(), b'owned')
        self.assertTrue(self.file.owned)

    def test_same_file_edited_bytes_are_neither_renewed_nor_deleted(self):
        self.file.publish(b'owned'); self.path.write_bytes(b'other')
        before = self.path.stat().st_mtime_ns
        with self.assertRaises(OwnershipError):self.file.renew()
        with self.assertRaises(OwnershipError):self.file.remove()
        self.assertEqual(self.path.read_bytes(), b'other')
        self.assertEqual(self.path.stat().st_mtime_ns, before)

    def test_real_exclusive_handle_blocks_renew_delete_and_replacement(self):
        self.file.publish(b'owned')
        handle = self.file._open(self.path)
        try:
            with self.assertRaises(OSError):self.file.renew()
            with self.assertRaises(OSError):self.file.remove()
            with self.assertRaises(OSError):self.path.rename(self.path.with_suffix('.moved'))
            self.assertTrue(self.file.owned)
        finally:
            self.file._k.CloseHandle(handle)
        self.file.renew(); self.file.remove()

    def test_final_directory_is_never_replaced(self):
        self.path.mkdir()
        with self.assertRaises(OSError):self.file.publish(b'owned')
        self.assertTrue(self.path.is_dir())

    def test_missing_owned_file_cleanup_does_not_adopt_anything(self):
        self.file.publish(b'owned'); self.path.unlink()
        self.file.remove(); self.assertFalse(self.file.owned)


if __name__ == '__main__':unittest.main()
