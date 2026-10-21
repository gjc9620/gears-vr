"""Pure temporary-file fixtures. No native binaries, Python copies or game launches."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import stage_portable as stage
import portable_preflight as preflight


class StageTests(unittest.TestCase):
    def test_packaging_contains_every_generated_continuous_selftest_executable(self):
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'native-capture'))
        from startup_selftests import continuous_suite,capture_suite
        suite=continuous_suite()
        required={row[0] for row in suite}
        self.assertTrue(required<=set(stage.NATIVE_TESTS))
        self.assertIn('startup_selftests',stage.PROJECT_MODULES['native-capture'].split())
        self.assertNotIn('TwoViewRendererShadowTest.exe',required)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name); self.root = self.base / 'source'; self.python = self.base / 'python'
        self.destination = self.base / 'stage'
        self.manifest = dict(schema=1, game_path='private/game/GOWDE-Steam.exe',
            native_build='tools/native-capture/build-fixture', native_dll='Mod.dll',
            xr_build='tools/openxr-probe/build-fixture', native_sha256='', xr_sha256='')
        for relative in [*stage.payload_paths(self.manifest), *stage.LICENSES]:
            self.put(self.root / relative, b'fixture')
        self.put(self.root/stage.AUDIO_PROVENANCE,json.dumps(dict(schema=1,policy='manual_game_session_v1',
            helper=Path(stage.AUDIO_HELPER).name,helper_sha256=stage.digest(self.root/stage.AUDIO_HELPER))).encode())
        for relative in (*stage.PYTHON_FILES, 'Lib/os.py', 'Lib/site-packages/private.py',
                         'Lib/test/private.py', 'Lib/__pycache__/private.pyc', 'DLLs/_ctypes.pyd'):
            self.put(self.python / relative, b'python-fixture')
        self.manifest['native_sha256'] = stage.digest(self.root / self.manifest['native_build'] / 'Mod.dll')
        self.manifest['xr_sha256'] = stage.digest(self.root / self.manifest['xr_build'] / 'OpenXRPoseProbe.exe')
        self.put(self.root / 'tools/vr-launcher/manifest.json', json.dumps(self.manifest).encode())
        self.put(self.root / 'tools/vr-launcher/config.json', b'{"game_path":"private","launcher_language":"zh-CN"}')
        self.put(self.root / 'analysis/private.sqlite', b'private')
        hashes = {name: stage.digest(self.root / stage.QUIET_BUILD / name) for name in stage.QUIET_HASHES}
        patcher = mock.patch.object(stage, 'QUIET_HASHES', hashes); patcher.start(); self.addCleanup(patcher.stop)

    @staticmethod
    def put(path, content):
        path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(content)

    def build(self): return stage.stage(self.root, self.python, self.destination)

    def test_allowlist_inventory_and_user_placeholder_preserve_sources(self):
        original = (self.root / 'tools/vr-launcher/config.json').read_bytes()
        result = self.build(); checked = preflight.check_inventory(self.destination)
        self.assertEqual(result['files'], checked['files'])
        names = {item['path'] for item in result['files']}
        self.assertIn('README.txt', names)
        self.assertIn('runtime/python/Lib/os.py', names)
        self.assertIn(stage.QUIET_BUILD + '/GearsQuietStartup.dll', names)
        self.assertFalse(any('private' in name or 'site-packages' in name or '__pycache__' in name for name in names))
        self.assertFalse((self.destination / 'analysis/private.sqlite').exists())
        config = json.loads((self.destination / 'tools/vr-launcher/config.json').read_text())
        manifest = json.loads((self.destination / 'tools/vr-launcher/manifest.json').read_text())
        self.assertEqual(config['game_path'], ''); self.assertEqual(manifest['game_path'], 'SELECT_GAME_EXECUTABLE')
        self.assertEqual(config['launcher_language'], 'en')
        self.assertEqual((self.root / 'tools/vr-launcher/config.json').read_bytes(), original)
        self.assertTrue(all({'path', 'source', 'license_category', 'size', 'sha256'} <= item.keys() for item in result['files']))

    def test_packaged_audio_import_and_hash_check_never_falls_back(self):
        repository=Path(__file__).resolve().parents[2]
        self.put(self.root/'tools/vr-launcher/game_audio_guard.py',
                 (repository/'tools/vr-launcher/game_audio_guard.py').read_bytes())
        self.build()
        code=('import sys; from pathlib import Path; root=Path(sys.argv[1]).resolve(); '
              'sys.path.insert(0,str(root/"tools/vr-launcher")); '
              'import game_audio_guard as audio; '
              'assert audio.MANUAL_HELPER.resolve()==root/"tools/game-audio/GearsTestAudioGuard.exe"; '
              'audio.verify_manual_helper()')
        command=[sys.executable,'-I','-B','-c',code,str(self.destination)]
        def run():return subprocess.run(command,cwd=self.destination,capture_output=True,text=True,timeout=20,
                            creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        result=run();self.assertEqual(result.returncode,0,result.stderr)
        helper=self.destination/stage.AUDIO_HELPER;helper.write_bytes(b'wrong-helper')
        self.assertNotEqual(run().returncode,0)
        helper.unlink();self.assertNotEqual(run().returncode,0)

    def test_audio_helper_and_receipt_required_before_stage(self):
        helper=self.root/stage.AUDIO_HELPER
        helper.write_bytes(b'wrong-or-old-helper')
        with self.assertRaisesRegex(ValueError,'provenance/hash'):self.build()
        self.assertFalse(self.destination.exists())
        helper.unlink()
        with self.assertRaises(FileNotFoundError):self.build()
        self.assertFalse(self.destination.exists())

    def test_existing_target_never_overwritten(self):
        self.destination.mkdir(); self.put(self.destination / 'keep', b'keep')
        with self.assertRaises(FileExistsError): self.build()
        self.assertEqual((self.destination / 'keep').read_bytes(), b'keep')

    def test_public_release_includes_inventoried_documents_and_acceptance_scope(self):
        for name in ('README.txt', 'RELEASE_NOTES.txt'):
            self.put(self.root / 'docs/releases/0.0.1' / name, ('release ' + name).encode())
        result = stage.stage(self.root, self.python, self.destination, release_version='0.0.1')
        preflight.check_inventory(self.destination)
        self.assertIn('RELEASE_NOTES.txt', {row['path'] for row in result['files']})
        self.assertEqual((self.destination / 'README.txt').read_text(), 'release README.txt')
        metadata = json.loads((self.destination / 'portable-candidate.json').read_text())
        self.assertEqual(metadata['version'], '0.0.1')
        self.assertEqual(metadata['kind'], 'public-test-release')
        self.assertNotIn('release_ready', metadata)
        self.assertIn('Reticle issue resolved', metadata['user_reported_acceptance'])

    def test_public_release_requires_documents_before_destination_creation(self):
        with self.assertRaises(FileNotFoundError):
            stage.stage(self.root, self.python, self.destination, release_version='0.0.1')
        self.assertFalse(self.destination.exists())

    def test_startup_hotfix_uses_002_documents_and_records_baseline_acceptance(self):
        for name in ('README.txt', 'RELEASE_NOTES.txt'):
            self.put(self.root/'docs/releases/0.0.2'/name,('0.0.2 '+name).encode())
        stage.stage(self.root,self.python,self.destination,release_version='0.0.2')
        preflight.check_inventory(self.destination)
        metadata=json.loads((self.destination/'portable-candidate.json').read_text())
        self.assertEqual(metadata['version'],'0.0.2')
        self.assertEqual(metadata['baseline_version'],'0.0.1')
        self.assertEqual((self.destination/'README.txt').read_text(),'0.0.2 README.txt')

    def test_new_runtime_modules_import_only_from_stage_without_repository_fallback(self):
        repository = Path(__file__).resolve().parents[2]
        modules = {'vr-launcher':'game_audio_guard', 'debug-launch': 'tiny_output_recovery', 'game-io': 'movie_state',
                   'native-capture': 'camera_closer_options startup_selftests path_choice_group_fit'}
        paths = ['tools/vr-launcher/' + name + '.py' for name in ('stage_portable', 'portable_preflight')]
        paths += ['tools/' + folder + '/' + name + '.py' for folder, names in modules.items() for name in names.split()]
        for relative in paths:
            self.put(self.root / relative, (repository / relative).read_bytes())
        inventory = self.build()
        names = {item['path'] for item in inventory['files']}
        self.assertTrue(set(paths) <= names)
        code = ('import json,sys; from pathlib import Path; '
                'root=Path(sys.argv[1]); '
                'sys.path.insert(0,str(root/"tools/vr-launcher")); '
                'from portable_preflight import import_project_modules; '
                'print(json.dumps(import_project_modules(root,json.loads(sys.argv[2]))))')
        command = [sys.executable, '-I', '-B', '-c', code, str(self.destination), json.dumps(modules)]
        result = subprocess.run(command, cwd=self.destination, capture_output=True, text=True, timeout=30,
                                creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(set(json.loads(result.stdout)), {'game_audio_guard','tiny_output_recovery', 'movie_state', 'camera_closer_options','startup_selftests','path_choice_group_fit'})
        # Removing a required staged helper must fail instead of finding this
        # developer checkout, an environment path, or a cached imported module.
        (self.destination / 'tools/debug-launch/tiny_output_recovery.py').unlink()
        missing = subprocess.run(command, cwd=self.destination, capture_output=True, text=True, timeout=30,
                                 creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        self.assertNotEqual(missing.returncode, 0)
        self.assertIn('tiny_output_recovery', missing.stderr)

    def test_missing_camera_helper_fails_staged_import_without_repository_fallback(self):
        repository = Path(__file__).resolve().parents[2]
        for relative in ('tools/vr-launcher/stage_portable.py', 'tools/vr-launcher/portable_preflight.py',
                         'tools/native-capture/camera_closer_options.py'):
            self.put(self.root / relative, (repository / relative).read_bytes())
        self.build()
        (self.destination / 'tools/native-capture/camera_closer_options.py').unlink()
        code = ('import sys; from pathlib import Path; root=Path(sys.argv[1]); '
                'sys.path.insert(0,str(root/"tools/vr-launcher")); '
                'from portable_preflight import import_project_modules; '
                'import_project_modules(root,{"native-capture":"camera_closer_options"})')
        result = subprocess.run([sys.executable, '-I', '-B', '-c', code, str(self.destination)],
                                cwd=self.destination, capture_output=True, text=True, timeout=30,
                                creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('camera_closer_options', result.stderr)

    def test_missing_or_wrong_artifact_rejects_before_destination_creation(self):
        artifact = self.root / stage.QUIET_BUILD / 'GearsQuietStartup.dll'
        artifact.write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'Quiet11 hash'): self.build()
        self.assertFalse(self.destination.exists())
        artifact.unlink()
        with self.assertRaises(FileNotFoundError): self.build()
        self.assertFalse(self.destination.exists())

    def test_explicit_candidate_stages_its_artifact_without_changing_default(self):
        default = self.root / 'tools/vr-launcher/manifest.json'
        original = default.read_bytes()
        candidate = dict(self.manifest, native_dll='Candidate.dll')
        artifact = self.root / candidate['native_build'] / candidate['native_dll']
        self.put(artifact, b'candidate-only')
        candidate['native_sha256'] = stage.digest(artifact)
        selected = self.base / 'candidate.json'
        selected.write_text(json.dumps(candidate))
        result = stage.stage(self.root, self.python, self.destination, manifest_path=selected)
        self.assertEqual(default.read_bytes(), original)
        self.assertEqual((self.destination / candidate['native_build'] / 'Candidate.dll').read_bytes(), b'candidate-only')
        self.assertFalse((self.destination / candidate['native_build'] / 'Mod.dll').exists())
        names = {item['path'] for item in result['files']}
        self.assertNotIn('candidate.json', names)
        self.assertFalse((self.destination / 'analysis/private.sqlite').exists())
        preflight.check_inventory(self.destination)

    def test_manifest_paths_cannot_escape(self):
        for value in ('../outside', str(self.base / 'outside'), 'analysis/re-kb'):
            with self.assertRaises(ValueError): stage.payload_paths(dict(self.manifest, native_build=value))
        with self.assertRaises(ValueError): stage.contained(self.root, '../outside')

    def test_named_resolution_candidate_and_gui_are_isolated_from_player_defaults(self):
        player = self.root / 'GearsVR Launcher.exe'
        player_bytes = player.read_bytes()
        default = self.root / 'tools/vr-launcher/manifest.json'
        default_bytes = default.read_bytes()
        config = self.root / 'tools/vr-launcher/config.json'
        config_bytes = config.read_bytes()
        selected = self.base / 'manifest-runtime-resolution1.json'
        selected.write_text(json.dumps(dict(self.manifest, runtime_render_size=True)))
        gui = self.base / 'candidate/GearsVrLauncher.exe'; self.put(gui, b'candidate-gui')
        result = stage.stage(self.root, self.python, self.destination,
                             manifest_path=selected, launcher_path=gui)
        self.assertEqual(player.read_bytes(), player_bytes)
        self.assertEqual(default.read_bytes(), default_bytes)
        self.assertEqual(config.read_bytes(), config_bytes)
        self.assertEqual((self.destination / 'GearsVR Launcher.exe').read_bytes(), b'candidate-gui')
        active = self.destination / 'tools/vr-launcher/manifest.json'
        alias = 'tools/vr-launcher/manifest-runtime-resolution1.json'
        self.assertEqual((self.destination / alias).read_bytes(), active.read_bytes())
        self.assertNotIn(b'private/game', active.read_bytes())
        portable_config = json.loads((self.destination / 'tools/vr-launcher/config.json').read_text())
        self.assertEqual(portable_config['render_scale'], 1.0)
        self.assertEqual(portable_config['launcher_language'], 'en')
        metadata = json.loads((self.destination / 'portable-candidate.json').read_text())
        self.assertEqual(metadata['candidate_manifest'], alias)
        self.assertEqual(metadata['source_manifest_sha256'], stage.digest(selected))
        readme = (self.destination / 'README.txt').read_text(encoding='utf-8')
        self.assertIn('runtime recommendation', readme)
        self.assertIn('默认100%', readme)
        self.assertIn('Export logs', readme)
        row = next(item for item in result['files'] if item['path'] == 'GearsVR Launcher.exe')
        self.assertEqual(row['source'], 'candidate-gui:GearsVrLauncher.exe')
        preflight.check_inventory(self.destination)

    def test_source_or_ancestor_destination_refused(self):
        for destination in (self.root, self.root.parent, self.python):
            with self.assertRaises((FileExistsError, ValueError)): stage.stage(self.root, self.python, destination)

    def test_symlink_source_is_not_collected(self):
        source = self.root / 'GearsVR Launcher.exe'; source.unlink()
        outside = self.base / 'outside'; outside.write_bytes(b'private')
        try: source.symlink_to(outside)
        except OSError as error: self.skipTest(str(error))
        with self.assertRaisesRegex(ValueError, 'Symlink/reparse'): self.build()
        self.assertFalse(self.destination.exists())

    def test_preflight_detects_modified_and_uninventoried_files(self):
        self.build(); file = self.destination / 'runtime/python/Lib/os.py'; before = file.read_bytes()
        file.write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'Inventory verification'): preflight.check_inventory(self.destination)
        file.write_bytes(before); self.put(self.destination / 'private.bin', b'private')
        with self.assertRaisesRegex(ValueError, 'Uninventoried'): preflight.check_inventory(self.destination)

    def test_preflight_uses_only_bundled_isolated_interpreter_and_clean_environment(self):
        receipt = {'passed': True}
        with mock.patch.dict(stage.os.environ, {'PYTHONHOME': 'private', 'PYTHONPATH': 'private',
                'GEARSVR_PYTHON': 'private', 'GEARSVR_QUIET_STARTUP_BUILD': 'private'}), \
             mock.patch.object(stage.subprocess, 'run', return_value=mock.Mock(returncode=0, stdout=json.dumps(receipt))) as run:
            self.assertEqual(stage.run_preflight(self.destination, self.base / 'receipt.json'), receipt)
            command = run.call_args.args[0]
            self.assertEqual(command[:3], [str(self.destination / 'runtime/python/python.exe'), '-I', '-B'])
            for key in ('PYTHONHOME', 'PYTHONPATH', 'GEARSVR_PYTHON', 'GEARSVR_QUIET_STARTUP_BUILD'):
                self.assertNotIn(key, run.call_args.kwargs['env'])
                self.assertIn(key, stage.os.environ)

    def test_misplaced_dll_does_not_satisfy_exe_loader_directory(self):
        binary = self.destination / 'tools/xr/Probe.exe'; self.put(binary, b'exe')
        self.put(self.destination / 'unrelated/vendor.dll', b'dll')
        system = self.base / 'windows/System32'; system.mkdir(parents=True)
        with mock.patch.dict(os.environ, {'PATH': str(self.destination / 'unrelated')}):
            with self.assertRaisesRegex(ValueError, 'Unresolved PE import vendor.dll'):
                preflight.resolve_pe_dependency(self.destination, binary, 'vendor.dll', self.manifest, system)
        self.put(binary.parent / 'vendor.dll', b'dll')
        result = preflight.resolve_pe_dependency(self.destination, binary, 'vendor.dll', self.manifest, system)
        self.assertEqual(result['path'], 'tools/xr/vendor.dll')

    def test_python_extension_and_injected_game_loader_contexts(self):
        root = self.destination; system = self.base / 'windows/System32'; system.mkdir(parents=True)
        pyd = root / 'runtime/python/DLLs/_ctypes.pyd'; self.put(pyd, b'pyd')
        self.put(pyd.parent / 'libffi-8.dll', b'ffi'); self.put(root / 'runtime/python/python312.dll', b'python')
        self.assertEqual(preflight.resolve_pe_dependency(root, pyd, 'libffi-8.dll', self.manifest, system)['scope'], 'bundled-loader-directory')
        self.assertEqual(preflight.resolve_pe_dependency(root, pyd, 'python312.dll', self.manifest, system)['path'], 'runtime/python/python312.dll')
        capture = root / self.manifest['native_build'] / self.manifest['native_dll']; self.put(capture, b'capture')
        self.put(capture.parent / 'vendor.dll', b'vendor')
        with self.assertRaisesRegex(ValueError, 'sibling alone is insufficient'):
            preflight.resolve_pe_dependency(root, capture, 'vendor.dll', self.manifest, system)
        self.put(system / 'kernel32.dll', b'windows')
        self.assertEqual(preflight.resolve_pe_dependency(root, capture, 'kernel32.dll', self.manifest, system)['scope'], 'windows-system32')
        with self.assertRaisesRegex(ValueError, 'Unresolved PE import'):
            preflight.resolve_pe_dependency(root, capture, 'user32.dll', self.manifest, system)

    def test_delay_imports_are_checked_and_misplaced_dependency_rejected(self):
        binary = self.destination / 'tools/xr/Probe.exe'; self.put(binary, b'exe')
        self.put(self.destination / 'unrelated/delayed.dll', b'dll')
        system = self.base / 'windows/System32'; system.mkdir(parents=True)
        pe = mock.Mock(DIRECTORY_ENTRY_IMPORT=[], DIRECTORY_ENTRY_DELAY_IMPORT=[SimpleNamespace(dll=b'delayed.dll')])
        library = SimpleNamespace(PE=mock.Mock(return_value=pe), DIRECTORY_ENTRY={
            'IMAGE_DIRECTORY_ENTRY_IMPORT': 1, 'IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT': 13})
        with self.assertRaisesRegex(ValueError, 'Unresolved PE import delayed.dll'):
            preflight.inspect_pe_imports(self.destination, binary, self.manifest, library, system)
        pe.parse_data_directories.assert_called_once_with(directories=[1, 13]); pe.close.assert_called_once()
        self.put(binary.parent / 'delayed.dll', b'dll')
        result = preflight.inspect_pe_imports(self.destination, binary, self.manifest, library, system)
        self.assertEqual(result[0]['kind'], 'delay-import')


if __name__ == '__main__': unittest.main()
