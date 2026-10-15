import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock


MODULE_PATH = pathlib.Path(__file__).with_name('background_launch.py')
sys.path.insert(0, str(MODULE_PATH.parent))
import quiet_launch
SPEC = importlib.util.spec_from_file_location('background_launch', MODULE_PATH)
background_launch = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(background_launch)


class Process:
    def __init__(self, pid):
        self.pid = pid


class BackgroundLaunchTests(unittest.TestCase):
    def setUp(self):
        patcher = mock.patch.object(background_launch, 'process_creation_time', return_value=123456)
        patcher.start(); self.addCleanup(patcher.stop)
        self.temp = tempfile.TemporaryDirectory()
        self.root = pathlib.Path(self.temp.name)
        self.game = self.root / 'GOWDE-Steam.exe'
        self.game.write_bytes(b'fixture')
        self.helper = self.root / 'background_input.py'
        self.helper.write_text('# fixture\n', encoding='utf-8')
        self.runtime = self.root / 'runtime'

    def tearDown(self):
        self.temp.cleanup()

    def test_default_launch_uses_nonactivating_visible_window_and_starts_helper_hidden(self):
        calls = []

        def popen(command, **kwargs):
            calls.append((command, kwargs))
            return Process(4100 if len(calls) == 1 else 4101)

        result = background_launch.launch(self.game, ['-NoPauseUnfocus', 'SP_Prison_P'],
                                          game_io=self.helper, runtime=self.runtime, popen=popen)
        self.assertEqual(calls[0][0], [str(self.game.resolve()), '-NoPauseUnfocus', 'SP_Prison_P'])
        self.assertEqual(calls[0][1]['cwd'], str(self.game.parent))
        self.assertEqual(calls[0][1]['creationflags'], background_launch.subprocess.CREATE_NO_WINDOW)
        self.assertEqual(calls[0][1]['startupinfo'].wShowWindow,
                         background_launch.SW_SHOWNOACTIVATE)
        self.assertEqual(calls[0][1]['startupinfo'].dwFlags & background_launch.subprocess.STARTF_USESHOWWINDOW,
                         background_launch.subprocess.STARTF_USESHOWWINDOW)
        self.assertEqual(calls[1][0][2:], ['enable', '--creation-time', '123456', '--pid', '4100', '--wait-seconds', '60'])
        self.assertEqual(calls[1][1]['creationflags'], background_launch.subprocess.CREATE_NO_WINDOW)
        self.assertEqual(calls[1][1]['startupinfo'].wShowWindow, 0)
        self.assertIn('--creation-time', calls[1][0])
        self.assertEqual(result['creation_time'],123456)
        self.assertEqual(result['background_input_helper_pid'], 4101)
        self.assertEqual(result['window_startup_policy'], 'shown_no_activate')
        saved = json.loads((self.runtime / 'background-launch-4100.json').read_text(encoding='utf-8'))
        self.assertEqual(saved['game_args'], ['-NoPauseUnfocus', 'SP_Prison_P'])
        self.assertEqual(saved['pid'], 4100)

    def test_foreground_and_disabled_modes_do_not_start_helper(self):
        calls = []

        def popen(command, **kwargs):
            calls.append((command, kwargs))
            return Process(4200)

        result = background_launch.launch(self.game, ['-NoPauseUnfocus'], foreground=True, disable_background_input=True,
                                          game_io=self.root / 'missing.py', runtime=self.runtime, popen=popen)
        self.assertEqual(len(calls), 1)
        self.assertIsNone(calls[0][1]['startupinfo'])
        self.assertEqual(calls[0][1]['creationflags'], 0)
        self.assertEqual(result['window_startup_policy'], 'foreground')
        self.assertFalse(result['background_input_requested'])

    def test_foreground_window_does_not_disable_controller_option(self):
        popen = mock.Mock(side_effect=[Process(4300), Process(4301)])
        result = background_launch.launch(self.game, foreground=True,
                                          game_io=self.helper, runtime=self.runtime, popen=popen)
        self.assertEqual(popen.call_count, 2)
        self.assertTrue(result['background_input_requested'])

    def test_command_line_preserves_dash_game_argument(self):
        with mock.patch.object(background_launch, 'launch', return_value={}) as launch:
            self.assertEqual(background_launch.main(['--game-path',str(self.game),'--game-arg=-NoPauseUnfocus']),0)
            self.assertEqual(launch.call_args.args[1], ['-NoPauseUnfocus'])

    def test_only_controller_disabled_keeps_background_window(self):
        calls = []
        def popen(command, **kwargs):
            calls.append((command, kwargs)); return Process(4400)
        result = background_launch.launch(self.game, disable_background_input=True,
                                          game_io=self.root / 'missing.py', runtime=self.runtime, popen=popen)
        self.assertEqual(len(calls), 1)
        self.assertFalse(result['background_input_requested'])

    def quiet_fixture(self):
        build = self.root / 'quiet build'
        build.mkdir()
        for name in ('GearsEarlyLaunch.exe', 'GearsQuietStartup.dll'):
            (build / name).write_bytes(b'fixture')
        report = dict(pid=5100, creation_time=987654, entry_restored=True,
                      debugger_detached=True, quiet_policy_installed=True, pre_entry_visible_windows=0)
        bootstrap = mock.Mock(pid=5000, returncode=0)
        bootstrap.communicate.return_value = json.dumps(report), ''
        return build, report, bootstrap

    def quiet_patches(self):
        validation = mock.patch.object(quiet_launch, 'validate_game', return_value=self.game)
        control = mock.patch.object(quiet_launch, 'windows_control')
        validate, factory = validation.start(), control.start()
        self.addCleanup(validation.stop); self.addCleanup(control.stop)
        factory.return_value.identity.return_value = dict(pid=5100, creation_time=987654, image=str(self.game))
        return validate, factory.return_value

    def test_quiet_launch_verifies_child_and_arms_real_pid_without_bootstrap_timeout(self):
        build, report, bootstrap = self.quiet_fixture()
        validate, control = self.quiet_patches()
        popen = mock.Mock(side_effect=[bootstrap, Process(5200)])
        result = background_launch.launch(self.game, ['-NoPauseUnfocus', 'argument with spaces'],
            quiet_build=build, game_io=self.helper, runtime=self.runtime, popen=popen)
        validate.assert_called_once_with(self.game.resolve())
        control.identity.assert_called_once_with(5100)
        bootstrap.communicate.assert_called_once_with()
        command, = popen.call_args_list[0].args
        self.assertEqual(command, [str(build/'GearsEarlyLaunch.exe'), str(self.game),
                                  str(build/'GearsQuietStartup.dll'), '-NoPauseUnfocus', 'argument with spaces'])
        kwargs = popen.call_args_list[0].kwargs
        self.assertEqual(kwargs['startupinfo'].wShowWindow, 0)
        self.assertEqual(kwargs['creationflags'], background_launch.subprocess.CREATE_NO_WINDOW)
        self.assertNotIn('timeout', kwargs)
        self.assertEqual(popen.call_args_list[1].args[0][2:],
                         ['enable', '--creation-time', '987654', '--pid', '5100', '--wait-seconds', '60'])
        self.assertEqual(result['pid'], report['pid'])
        self.assertEqual(result['quiet_startup']['bootstrap_pid'], 5000)
        self.assertEqual(result['window_startup_policy'], 'quiet_startup')
        self.assertTrue((self.runtime/'background-launch-5100.json').is_file())
        self.assertFalse((self.runtime/'background-launch-5000.json').exists())

    def test_quiet_success_can_disable_helper(self):
        build, _, bootstrap = self.quiet_fixture()
        self.quiet_patches()
        popen = mock.Mock(return_value=bootstrap)
        result = background_launch.launch(self.game, quiet_build=build, disable_background_input=True,
            game_io=self.root/'missing.py', runtime=self.runtime, popen=popen)
        self.assertEqual(popen.call_count, 1)
        self.assertEqual(result['creation_time'], 987654)

    def test_quiet_foreground_conflict_is_rejected_before_side_effects(self):
        popen = mock.Mock()
        with self.assertRaisesRegex(ValueError, 'cannot be combined'):
            background_launch.launch(self.game, quiet_build=self.root, foreground=True, popen=popen)
        popen.assert_not_called()

    def test_quiet_missing_build_and_game_validation_do_not_spawn(self):
        build, _, _ = self.quiet_fixture()
        validate, _ = self.quiet_patches()
        popen = mock.Mock()
        for name in ('GearsEarlyLaunch.exe', 'GearsQuietStartup.dll'):
            path = build/name
            path.unlink()
            with self.assertRaises(FileNotFoundError):
                background_launch.launch(self.game, quiet_build=build, disable_background_input=True, popen=popen)
            path.write_bytes(b'fixture')
        validate.side_effect = RuntimeError('unsupported game')
        with self.assertRaisesRegex(RuntimeError, 'unsupported game'):
            background_launch.launch(self.game, quiet_build=build, disable_background_input=True, popen=popen)
        popen.assert_not_called()

    def test_quiet_bootstrap_failure_never_falls_back_or_starts_helper(self):
        build, _, bootstrap = self.quiet_fixture()
        _, control = self.quiet_patches()
        bootstrap.returncode = 1
        bootstrap.communicate.return_value = '', 'owned_child_cleanup pid=5100 exited=1'
        popen = mock.Mock(return_value=bootstrap)
        with self.assertRaisesRegex(RuntimeError, 'Quiet startup failed'):
            background_launch.launch(self.game, quiet_build=build, game_io=self.helper, popen=popen)
        self.assertEqual(popen.call_count, 1)
        control.identity.assert_not_called()

    def test_quiet_spawn_or_communication_errors_do_not_fall_back_or_kill_bootstrap(self):
        build, _, bootstrap = self.quiet_fixture()
        _, control = self.quiet_patches()
        for failure in ('spawn', 'communication'):
            with self.subTest(failure=failure):
                bootstrap.communicate.side_effect = OSError('pipe failed') if failure == 'communication' else None
                popen = mock.Mock(side_effect=OSError('spawn failed')) if failure == 'spawn' else mock.Mock(return_value=bootstrap)
                with self.assertRaises(OSError):
                    background_launch.launch(self.game, quiet_build=build, game_io=self.helper, popen=popen)
                self.assertEqual(popen.call_count, 1)
                bootstrap.kill.assert_not_called()
                bootstrap.terminate.assert_not_called()
        control.identity.assert_not_called()

    def test_quiet_rejects_malformed_multiple_duplicate_or_incomplete_reports(self):
        build, report, bootstrap = self.quiet_fixture()
        self.quiet_patches()
        bad_reports = ['', '[]', '{}', json.dumps(report)+'\n'+json.dumps(report),
                       json.dumps(report)[:-1]+',"pid":5100}']
        for key, value in (('pid', True), ('pid', 0), ('pid', 5000), ('creation_time', 0),
                           ('entry_restored', 1), ('debugger_detached', False), ('quiet_policy_installed', False),
                           ('pre_entry_visible_windows', False), ('pre_entry_visible_windows', 1)):
            bad_reports.append(json.dumps(dict(report, **{key:value})))
        for output in bad_reports:
            with self.subTest(output=output):
                bootstrap.communicate.return_value = output, ''
                popen = mock.Mock(return_value=bootstrap)
                with self.assertRaisesRegex(RuntimeError, 'Invalid quiet startup report'):
                    background_launch.launch(self.game, quiet_build=build, game_io=self.helper, popen=popen)
                self.assertEqual(popen.call_count, 1)

    def test_quiet_rejects_dead_wrong_path_creation_or_pid(self):
        build, _, bootstrap = self.quiet_fixture()
        _, control = self.quiet_patches()
        correct = dict(pid=5100, creation_time=987654, image=str(self.game))
        for identity in (None, dict(correct, pid=5200), dict(correct, creation_time=987655),
                         dict(correct, image=str(self.root/'other.exe'))):
            with self.subTest(identity=identity):
                control.identity.return_value = identity
                popen = mock.Mock(return_value=bootstrap)
                with self.assertRaisesRegex(RuntimeError, 'identity mismatch or exited'):
                    background_launch.launch(self.game, quiet_build=build, game_io=self.helper, popen=popen)
                self.assertEqual(popen.call_count, 1)

    def test_quiet_cli_environment_default_and_explicit_override(self):
        with mock.patch.dict(background_launch.os.environ, {'GEARSVR_QUIET_STARTUP_BUILD': str(self.root/'env')}),\
             mock.patch.object(background_launch, 'launch', return_value={}) as launch:
            self.assertEqual(background_launch.main(['--game-path', str(self.game)]), 0)
            self.assertEqual(launch.call_args.kwargs['quiet_build'], self.root/'env')
            self.assertEqual(background_launch.main(['--game-path', str(self.game), '--quiet-build', str(self.root/'explicit')]), 0)
            self.assertEqual(launch.call_args.kwargs['quiet_build'], self.root/'explicit')


if __name__ == '__main__':
    unittest.main()
