import json
import pathlib
import tempfile
import unittest

import graphics_policy as policy


class GraphicsPolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        self.target = self.root / 'user.ini'
        self.state = self.root / 'backups'

    def apply(self):
        return policy.enforce_off_before_launch(self.target, self.state, stopped=lambda: None)

    def test_roundtrip_encodings_and_preserves_other_settings(self):
        for encoding in ('utf-8', 'utf-8-sig', 'utf-16'):
            with self.subTest(encoding=encoding):
                text = '[Settings]\r\nDLSS=True\r\nAADefaultQualityMode=4\r\nFSR2=False\r\n'
                raw = text.encode(encoding)
                self.target.write_bytes(raw)
                report = self.apply()
                self.assertTrue(report['changed'])
                self.assertEqual(self.target.read_bytes(), text.replace('DLSS=True', 'DLSS=False').encode(encoding))
                self.assertEqual(pathlib.Path(json.loads(pathlib.Path(report['restore_manifest']).read_text())['backup']).read_bytes(), raw)
                self.target.write_bytes(self.target.read_bytes().replace('FSR2=False'.encode('utf-16-le' if encoding == 'utf-16' else 'utf-8'),
                                                                         'FSR2=True'.encode('utf-16-le' if encoding == 'utf-16' else 'utf-8')))
                policy.restore(report['restore_manifest'], self.target, stopped=lambda: None)
                self.assertEqual(self.target.read_bytes(), text.replace('FSR2=False', 'FSR2=True').encode(encoding))

    def test_idempotent_off_creates_no_backup(self):
        raw = b'DLSS=fAlSe\nOther=1\n'
        self.target.write_bytes(raw)
        for _ in range(2):
            report = self.apply()
            self.assertFalse(report['changed'])
            self.assertFalse(report['runtime_lock_verified'])
            self.assertEqual(report['live_dlss'], 'unknown')
        self.assertEqual(self.target.read_bytes(), raw)
        self.assertFalse(self.state.exists())

    def test_reapply_retains_original_backup(self):
        raw = b'DLSS=True\nOther=1\n'
        self.target.write_bytes(raw)
        report = self.apply()
        self.assertFalse(self.apply()['changed'])
        self.target.write_bytes(raw)
        self.assertEqual(self.apply()['restore_manifest'], report['restore_manifest'])

    def test_ambiguous_or_invalid_key_fails_without_write(self):
        for raw in (b'Other=1\n', b'DLSS=True\nDLSS=False\n', b'DLSS=bogus\n'):
            self.target.write_bytes(raw)
            with self.assertRaises(RuntimeError):
                self.apply()
            self.assertEqual(self.target.read_bytes(), raw)
        self.assertFalse(self.state.exists())

    def test_running_guard_prevents_write(self):
        raw = b'DLSS=True\n'
        self.target.write_bytes(raw)
        def running():
            raise RuntimeError('running')
        with self.assertRaisesRegex(RuntimeError, 'running'):
            policy.enforce_off_before_launch(self.target, self.state, stopped=running)
        self.assertEqual(self.target.read_bytes(), raw)
        self.assertFalse(self.state.exists())

    def test_restore_rejects_corrupt_backup_and_wrong_target(self):
        self.target.write_bytes(b'DLSS=True\n')
        report = self.apply()
        manifest = report['restore_manifest']
        with self.assertRaisesRegex(RuntimeError, 'different settings'):
            policy.restore(manifest, self.root / 'other.ini', stopped=lambda: None)
        backup = pathlib.Path(json.loads(pathlib.Path(manifest).read_text())['backup'])
        backup.write_bytes(b'corrupt')
        with self.assertRaisesRegex(RuntimeError, 'backup verification'):
            policy.restore(manifest, self.target, stopped=lambda: None)
        self.assertEqual(self.target.read_bytes(), b'DLSS=False\n')

    def test_existing_temporary_is_not_destroyed(self):
        self.target.write_bytes(b'DLSS=True\n')
        temp = self.target.with_name(self.target.name + '.gearsvr-policy-tmp')
        temp.write_bytes(b'owned by another writer')
        with self.assertRaises(FileExistsError):
            self.apply()
        self.assertEqual(temp.read_bytes(), b'owned by another writer')
        self.assertEqual(self.target.read_bytes(), b'DLSS=True\n')


if __name__ == '__main__':
    unittest.main()
