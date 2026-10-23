import json
import pathlib
import tempfile
import unittest
from unittest import mock

import graphics_policy as policy


class GraphicsPolicyTests(unittest.TestCase):
    def test_multiple_profiles_select_only_the_active_steam_account_read_only(self):
        folder = self.root / 'Coalition/WarGame/Config'
        folder.mkdir(parents=True)
        old = folder / '76561197960271872EngineUserSettings.ini'
        active = folder / '76561197960266962EngineUserSettings.ini'
        old.write_bytes(b'DLSS=True\n')
        active.write_bytes(b'DLSS=False\n')
        before = {file.name:file.read_bytes() for file in folder.iterdir()}
        with mock.patch.object(policy.instance_policy, 'local_appdata', return_value=self.root), \
                mock.patch.object(policy, 'active_steam_id', return_value='76561197960266962'):
            self.assertEqual(policy.locate(), active.resolve())
        for account in (None, '76561197960269999'):
            with mock.patch.object(policy.instance_policy, 'local_appdata', return_value=self.root), \
                    mock.patch.object(policy, 'active_steam_id', return_value=account):
                with self.assertRaisesRegex(RuntimeError, 'Multiple user-settings'):
                    policy.locate()
        self.assertEqual({file.name:file.read_bytes() for file in folder.iterdir()}, before)

    def test_single_profile_preserves_existing_offline_selection(self):
        folder = self.root / 'Coalition/WarGame/Config'
        folder.mkdir(parents=True)
        selected = folder / '76561197960271872EngineUserSettings.ini'
        selected.write_bytes(b'DLSS=False\n')
        with mock.patch.object(policy.instance_policy, 'local_appdata', return_value=self.root), \
                mock.patch.object(policy, 'active_steam_id', side_effect=AssertionError('unneeded account lookup')):
            self.assertEqual(policy.locate(), selected.resolve())

    def test_active_steam_account_id_is_individual_public_universe_and_rejects_invalid_values(self):
        import winreg
        for value, kind, expected in ((1234, winreg.REG_DWORD, '76561197960266962'),
                                     (0, winreg.REG_DWORD, None),
                                     ('1234', winreg.REG_SZ, None),
                                     (0x100000000, winreg.REG_DWORD, None)):
            with mock.patch.object(winreg, 'OpenKey'), \
                    mock.patch.object(winreg, 'QueryValueEx', return_value=(value, kind)), \
                    mock.patch.object(policy.steam_account, 'offline_steam_id', return_value=None):
                self.assertEqual(policy.active_steam_id(), expected)

    def test_online_account_wins_and_only_zero_dword_can_use_offline_fallback(self):
        import winreg
        for value, kind, expected, calls in ((1234, winreg.REG_DWORD, '76561197960266962', 0),
                                             (0, winreg.REG_DWORD, '76561197960266963', 1),
                                             (0, winreg.REG_SZ, None, 0)):
            with mock.patch.object(winreg, 'OpenKey'), \
                    mock.patch.object(winreg, 'QueryValueEx', return_value=(value, kind)), \
                    mock.patch.object(policy.steam_account, 'offline_steam_id', return_value='76561197960266963') as offline:
                self.assertEqual(policy.active_steam_id(), expected)
                self.assertEqual(offline.call_count, calls)

    def test_explicit_install_binding_is_scoped_and_does_not_change_files(self):
        folder = self.root / 'Coalition/WarGame/Config'
        folder.mkdir(parents=True)
        old = folder / '76561197960266963EngineUserSettings.ini'
        active = folder / '76561197960266962EngineUserSettings.ini'
        old.write_bytes(b'DLSS=False\n')
        active.write_bytes(b'DLSS=False\n')
        config = self.root / 'tools/vr-launcher/config.json'
        config.parent.mkdir(parents=True)
        legacy = self.root / 'legacy/Binaries_x64/GOWDE-Steam.exe'
        retail = self.root / 'retail/Binaries_x64/GOWDE-Steam.exe'
        config.write_text(json.dumps({'game_settings_paths': {str(legacy): str(old)}}))
        with mock.patch.object(policy, 'ROOT', self.root), \
                mock.patch.object(policy.instance_policy, 'local_appdata', return_value=self.root), \
                mock.patch.object(policy.instance_policy, 'context', return_value=None), \
                mock.patch.object(policy, 'active_steam_id', return_value='76561197960266962'):
            self.assertEqual(policy.locate(legacy), old.resolve())
            self.assertEqual(policy.locate(retail), active.resolve())
            self.assertEqual(policy.locate(), active.resolve())
            with mock.patch.object(policy, 'active_steam_id', return_value=None):
                self.assertEqual(policy.locate(legacy), old.resolve())
                with self.assertRaisesRegex(RuntimeError, 'Multiple user-settings'):
                    policy.locate(retail)
            for target in (self.root / 'outside.ini', folder / 'unrelated.ini'):
                target.write_bytes(b'DLSS=False\n')
                config.write_text(json.dumps({'game_settings_paths': {str(legacy): str(target)}}))
                with self.assertRaisesRegex(RuntimeError, 'Settings binding'):
                    policy.locate(legacy)
        self.assertEqual(old.read_bytes(), b'DLSS=False\n')
        self.assertEqual(active.read_bytes(), b'DLSS=False\n')

    def test_binding_rejects_invalid_types_relative_paths_and_duplicate_install_keys(self):
        config = self.root / 'tools/vr-launcher/config.json'
        config.parent.mkdir(parents=True)
        exe = str(self.root / 'legacy/Binaries_x64/GOWDE-Steam.exe')
        for bindings in ([], {exe: 12}, {'relative.exe': str(self.target)},
                         {exe: 'relative.ini'}, {exe: str(self.target), exe.upper(): str(self.target)}):
            config.write_text(json.dumps({'game_settings_paths': bindings}))
            with mock.patch.object(policy, 'ROOT', self.root), \
                    mock.patch.object(policy.instance_policy, 'context', return_value=None), \
                    mock.patch.object(policy.instance_policy, 'local_appdata', return_value=self.root), \
                    self.assertRaises(RuntimeError):
                policy.locate(exe)

    def test_isolated_instance_ignores_shared_settings_bindings(self):
        folder = self.root / 'Coalition/WarGame/Config'
        folder.mkdir(parents=True)
        settings = folder / 'EngineUserSettings.ini'
        settings.write_bytes(b'DLSS=False\n')
        with mock.patch.object(policy.instance_policy, 'local_appdata', return_value=self.root), \
                mock.patch.object(policy.instance_policy, 'context', return_value=self.root):
            self.assertEqual(policy.locate(self.root / 'game.exe'), settings.resolve())

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
