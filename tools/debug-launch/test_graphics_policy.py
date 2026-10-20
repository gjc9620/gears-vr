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

    def test_experimental_sr_preflight_is_read_only_in_supported_encodings(self):
        for encoding in ('utf-8', 'utf-8-sig', 'utf-16'):
            raw = 'DLSS=True\r\nFSR2=False\r\nFSR3=False\r\nMFSR=False\r\nAADefaultQualityMode=3\r\nOther=keep\r\n'.encode(encoding)
            self.target.write_bytes(raw)
            report = policy.check_experimental_sr(self.target)
            self.assertEqual(report['startup_policy'], 'experimental_preserve_upscaler')
            self.assertFalse(report['changed'])
            self.assertEqual(report['live_dlss'], 'unknown')
            self.assertEqual(self.target.read_bytes(), raw)
            self.assertFalse(self.state.exists())

    def test_experimental_sr_rejects_dlaa_other_upscalers_and_ambiguous_keys_without_write(self):
        text = 'DLSS=True\nFSR2=False\nFSR3=False\nMFSR=False\nAADefaultQualityMode=3\n'
        invalid = [text.replace('DLSS=True', 'DLSS=False'), text + 'DLSS=True\n',
                   text.replace('AADefaultQualityMode=3', 'AADefaultQualityMode=4'),
                   text.replace('AADefaultQualityMode=3', 'AADefaultQualityMode=2'),
                   text.replace('AADefaultQualityMode=3\n', '')]
        for key in ('FSR2', 'FSR3', 'MFSR'):
            invalid.extend((text.replace(key+'=False', key+'=True'),
                            text.replace(key+'=False\n', ''), text+key+'=False\n'))
        for value in invalid:
            raw = value.encode()
            self.target.write_bytes(raw)
            with self.assertRaisesRegex(RuntimeError, 'Experimental DLSS SR'):
                policy.check_experimental_sr(self.target)
            self.assertEqual(self.target.read_bytes(), raw)
            self.assertFalse(self.state.exists())


if __name__ == '__main__':
    unittest.main()
