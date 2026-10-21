import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec=importlib.util.spec_from_file_location('dlss_manual',Path(__file__).with_name('dlss_manual.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class ManualLauncherTests(unittest.TestCase):
    def test_environment_preserves_audio_and_clears_background_flags(self):
        original={'PATH':'original','GEARSVR_TEST_MUTE_GAME':'1','GEARSVR_TEST_WINDOWS_BEHIND':'1',
            'gearSVR_test_other':'1','XR_RUNTIME_JSON':'simulator','GEARSVR_QUIET_STARTUP_BUILD':'behind'}
        env=module.launch_environment(original,Path('manual.json'))
        self.assertEqual(env['PATH'],'original')
        self.assertEqual(env['GEARSVR_TEST_NGX_MANUAL_OWNED'],'1')
        self.assertNotIn('GEARSVR_TEST_MUTE_GAME',env)
        self.assertNotIn('GEARSVR_TEST_WINDOWS_BEHIND',env)
        self.assertNotIn('XR_RUNTIME_JSON',env)
        self.assertNotIn('GEARSVR_QUIET_STARTUP_BUILD',env)
        self.assertEqual(original['GEARSVR_TEST_MUTE_GAME'],'1')

    def test_private_runs_preserve_prior_event_log_and_default_manifest(self):
        with tempfile.TemporaryDirectory()as directory:
            root=Path(directory);source=root/'source';source.mkdir()
            dll=source/'test.dll';dll.write_bytes(b'accepted DLL')
            events=source/'ngx-isolated-trial-events.txt';events.write_text('prior evidence')
            expected={str(dll):module.digest(dll)};manifest={'native_build':'source','native_dll':'test.dll'}
            first=module.prepare_run(root,manifest,expected,'first')
            second=module.prepare_run(root,manifest,expected,'second')
            self.assertNotEqual(first,second)
            self.assertEqual(events.read_text(),'prior evidence')
            self.assertEqual(manifest['native_build'],'source')
            self.assertEqual((first.parent/'test.dll').read_bytes(),b'accepted DLL')
            self.assertFalse((first.parent/events.name).exists())
            self.assertEqual(json.loads(first.read_text())['native_build'],'analysis/native-dlss-manual/first')

    def test_changed_candidate_refused_before_new_directory(self):
        with tempfile.TemporaryDirectory()as directory:
            root=Path(directory);dll=root/'test.dll';dll.write_bytes(b'first')
            expected={str(dll):module.digest(dll)};dll.write_bytes(b'changed')
            with self.assertRaisesRegex(RuntimeError,'SHA256 mismatch'):
                module.prepare_run(root,{},expected,'no')
            self.assertFalse((root/'analysis').exists())

    def test_existing_run_directory_is_not_reused(self):
        with tempfile.TemporaryDirectory()as directory:
            root=Path(directory);dll=root/'test.dll';dll.write_bytes(b'DLL')
            expected={str(dll):module.digest(dll)}
            module.prepare_run(root,{},expected,'one')
            with self.assertRaises(FileExistsError):module.prepare_run(root,{},expected,'one')

if __name__=='__main__':unittest.main()
