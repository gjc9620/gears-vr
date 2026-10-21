"""Temporary fixtures only; never package a real stage or launch the game."""
import hashlib
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import zipfile

import package_portable as packaging


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name); self.stage = self.base / 'stage'; self.stage.mkdir()
        self.output = self.base / 'candidate.zip'
        self.rows = []
        self.add('tools/vr-launcher/manifest.json', json.dumps({
            'native_build': 'tools/native-capture/build-fixture', 'native_dll': 'GearsCaptureFixture.dll',
            'xr_build': 'tools/openxr-probe/build-fixture'}).encode())
        self.add('README.txt', b'instructions')
        self.add('tools/native-capture/build-fixture/GearsCaptureFixture.dll', b'our-mod')
        self.add('runtime/python/Lib/os.py', b'python')
        self.add('portable-candidate.json', b'{}')
        present = {row['path'] for row in self.rows}
        required = packaging.allowed_paths(self.stage) | {
            'runtime/python/' + name for name in packaging.staging.PYTHON_FILES}
        for name in sorted(required - present):
            self.add(name, b'required-fixture')
        self.save()

    def put(self, name, content):
        path = self.stage / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(content)

    def add(self, name, content):
        self.put(name, content)
        self.rows.append({'path': name, 'size': len(content), 'sha256': hashlib.sha256(content).hexdigest()})

    def save(self):
        self.put('inventory.json', json.dumps({'schema': 1, 'files': self.rows,
            'inventory_self_excluded': True, 'bytes': sum(row['size'] for row in self.rows)}).encode())

    def build(self):
        return packaging.package(self.stage, self.output)

    def assert_clean(self):
        self.assertFalse(self.output.exists())
        self.assertEqual(list(self.base.glob('.candidate.zip.*.tmp')), [])

    def test_exact_zip_contents_exclude_runtime_added_files(self):
        for name in ('analysis/runtime/gpu.png', 'analysis/vr-launcher-ui/log.json',
                     'tools/native-capture/build-fixture/capture.dds', 'tools/native-capture/build-fixture/run.log'):
            self.put(name, b'private runtime data')
        inventory_bytes = (self.stage / 'inventory.json').read_bytes()
        report = self.build()
        with zipfile.ZipFile(self.output) as archive:
            self.assertEqual(archive.namelist(), [row['path'] for row in self.rows] + ['inventory.json'])
            self.assertEqual(archive.read('inventory.json'), inventory_bytes)
            for row in self.rows:
                self.assertEqual(archive.read(row['path']), (self.stage / row['path']).read_bytes())
        self.assertEqual(report['files'], len(self.rows) + 1)
        self.assertEqual(report['sha256'], hashlib.sha256(self.output.read_bytes()).hexdigest())
        self.assertEqual(report['archive_bytes'], self.output.stat().st_size)

    def test_changed_file_rejected_and_own_temp_removed(self):
        self.put('README.txt', b'changed')
        with self.assertRaisesRegex(ValueError, 'Inventory verification'):
            self.build()
        self.assert_clean()

    def test_release_notes_required_only_for_versioned_public_release(self):
        self.rows = [row for row in self.rows if row['path'] != 'portable-candidate.json']
        self.add('portable-candidate.json', json.dumps({'kind': 'public-test-release', 'version': '0.0.1'}).encode())
        self.save()
        with self.assertRaisesRegex(ValueError, 'Missing required inventory members'):
            self.build()
        self.assert_clean()
        self.add('RELEASE_NOTES.txt', b'public test notes'); self.save()
        self.build()
        with zipfile.ZipFile(self.output) as archive:
            self.assertEqual(archive.read('RELEASE_NOTES.txt'), b'public test notes')

    def candidate_alias(self, alias, contents=None):
        self.rows = [row for row in self.rows if row['path'] != 'portable-candidate.json']
        self.add('portable-candidate.json', json.dumps({'candidate_manifest': alias}).encode())
        if contents is not None: self.add(alias, contents)
        self.save()

    def test_one_exact_active_manifest_alias_is_allowed(self):
        alias = 'tools/vr-launcher/manifest-runtime-resolution1.json'
        self.candidate_alias(alias, (self.stage / 'tools/vr-launcher/manifest.json').read_bytes())
        self.build()
        with zipfile.ZipFile(self.output) as archive:
            self.assertEqual(archive.read(alias), archive.read('tools/vr-launcher/manifest.json'))

    def test_alias_cannot_admit_different_content_or_arbitrary_manifest(self):
        alias = 'tools/vr-launcher/manifest-runtime-resolution1.json'
        self.candidate_alias(alias, b'{}')
        with self.assertRaisesRegex(ValueError, 'differs from active'): self.build()
        self.assert_clean()
        self.candidate_alias('tools/vr-launcher/anything.json')
        with self.assertRaisesRegex(ValueError, 'Unexpected candidate manifest alias'): self.build()
        self.assert_clean()

    def test_named_alias_does_not_allow_second_manifest(self):
        alias = 'tools/vr-launcher/manifest-runtime-resolution1.json'
        self.candidate_alias(alias, (self.stage / 'tools/vr-launcher/manifest.json').read_bytes())
        self.add('tools/vr-launcher/manifest-extra.json', b'{}'); self.save()
        with self.assertRaisesRegex(ValueError, 'allowlist'): self.build()
        self.assert_clean()

    def test_unsafe_and_nonallowlisted_paths_rejected(self):
        original = list(self.rows)
        for name in ('../private', '/absolute', 'C:/private', r'..\private', r'runtime\python\os.py',
                     'README.txt:stream', 'x//y', 'x/./y', 'x/NUL.txt', 'x/trailing.',
                     'analysis/gpu.png', 'game/GOWDE-Steam.exe', 'runtime/python/Lib/analysis/private.py',
                     'runtime/python/Lib/derivatives/pseudocode.py',
                     'tools/native-capture/build-fixture/capture.dds', 'inventory.json'):
            with self.subTest(name=name):
                self.rows = original + [{'path': name, 'size': 0, 'sha256': hashlib.sha256(b'').hexdigest()}]
                self.save()
                with self.assertRaises(ValueError): self.build()
                self.assert_clean()

    def test_duplicate_case_alias_and_invalid_metadata_rejected(self):
        original = list(self.rows)
        for row in (dict(self.rows[1]), dict(self.rows[1], path='readme.txt'),
                    dict(self.rows[1], size=True), dict(self.rows[1], sha256='bad')):
            self.rows = original + [row]; self.save()
            with self.assertRaises(ValueError): self.build()
            self.assert_clean()

    def test_missing_required_members_rejected_before_temp_creation(self):
        original = list(self.rows)
        for name in ('GearsVR Launcher.exe',
                     'tools/native-capture/build-fixture/GearsCaptureFixture.dll',
                     'runtime/python/python.exe', 'runtime/python/python312.dll',
                     'licenses/MinHook.txt'):
            with self.subTest(name=name):
                self.rows = [row for row in original if row['path'] != name]
                self.save()
                with mock.patch.object(packaging.tempfile, 'mkstemp') as temporary:
                    with self.assertRaisesRegex(ValueError, 'Missing required inventory members'):
                        self.build()
                    temporary.assert_not_called()
                self.assert_clean()

    def test_inventory_bounds_and_duplicate_json_keys(self):
        with mock.patch.object(packaging, 'MAX_INVENTORY_BYTES', 8):
            with self.assertRaisesRegex(ValueError, 'size bound'): self.build()
        self.put('inventory.json', b'{"schema": 1, "schema": 1}')
        with self.assertRaisesRegex(ValueError, 'Duplicate JSON key'): self.build()
        self.assert_clean()

    def test_symlink_payload_rejected(self):
        path = self.stage / 'README.txt'; path.unlink()
        outside = self.base / 'private'; outside.write_bytes(b'instructions')
        try: path.symlink_to(outside)
        except OSError as error: self.skipTest(str(error))
        with self.assertRaisesRegex(ValueError, 'Symlink/reparse'): self.build()
        self.assert_clean()

    def test_previous_output_preserved(self):
        self.output.write_bytes(b'previous')
        with self.assertRaises(FileExistsError): self.build()
        self.assertEqual(self.output.read_bytes(), b'previous')
        self.assertEqual(list(self.base.glob('.candidate.zip.*.tmp')), [])

    def test_failed_reopen_verification_does_not_publish(self):
        unrelated = self.base / '.candidate.zip.unrelated.tmp'; unrelated.write_bytes(b'keep')
        with mock.patch.object(packaging, 'verify_zip', side_effect=ValueError('verification failed')):
            with self.assertRaisesRegex(ValueError, 'verification failed'): self.build()
        self.assertFalse(self.output.exists())
        self.assertEqual(list(self.base.glob('.candidate.zip.*.tmp')), [unrelated])
        self.assertEqual(unrelated.read_bytes(), b'keep')

    def test_publish_race_preserves_competing_output(self):
        real_verify = packaging.verify_zip
        def race(*args):
            real_verify(*args)
            self.output.write_bytes(b'competing output')
        with mock.patch.object(packaging, 'verify_zip', side_effect=race):
            with self.assertRaises(FileExistsError): self.build()
        self.assertEqual(self.output.read_bytes(), b'competing output')
        self.assertEqual(list(self.base.glob('.candidate.zip.*.tmp')), [])

    def test_verifier_detects_tampered_member_and_extra_member(self):
        self.build()
        with zipfile.ZipFile(self.output, 'a') as archive:
            archive.writestr('analysis/private.png', b'private')
        with self.assertRaisesRegex(ValueError, 'member list'):
            packaging.verify_zip(self.output, (self.stage / 'inventory.json').read_bytes(), self.rows)


if __name__ == '__main__': unittest.main()
