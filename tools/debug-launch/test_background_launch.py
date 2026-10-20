import importlib.util
import contextlib
import io
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
    def test_test_behind_policy_requires_explicit_built_and_authenticated_artifacts(self):
        import hashlib
        build,_,bootstrap=self.quiet_fixture();self.quiet_patches();popen=mock.Mock(return_value=bootstrap)
        with mock.patch.dict(background_launch.os.environ,{'GEARSVR_TEST_WINDOWS_BEHIND':'1'}):
            with self.assertRaisesRegex(RuntimeError,'explicitly built'):
                background_launch.launch(self.game,popen=popen,runtime=self.runtime)
            popen.assert_not_called()
            with self.assertRaisesRegex(RuntimeError,'Missing built'):
                background_launch.launch(self.game,quiet_build=build,popen=popen,runtime=self.runtime)
            popen.assert_not_called()
            proof=dict(schema=1,policy='hidden_bottom_showna_v1',sha256={
                name:hashlib.sha256((build/name).read_bytes()).hexdigest()
                for name in ('GearsEarlyLaunch.exe','GearsQuietStartup.dll')})
            (build/'test-background-policy.json').write_text(json.dumps(proof))
            with self.assertRaisesRegex(RuntimeError,'Unsupported'):
                background_launch.launch(self.game,quiet_build=build,popen=popen,runtime=self.runtime)
            popen.assert_not_called()
            proof['policy']='hidden_bottom_showwindow_v2'
            (build/'test-background-policy.json').write_text(json.dumps(proof))
            result=background_launch.launch(self.game,quiet_build=build,popen=popen,runtime=self.runtime)
            self.assertTrue(result['test_windows_behind_requested'])
            self.assertEqual(popen.call_args.kwargs['env']['GEARSVR_TEST_WINDOWS_BEHIND'],'1')
            popen.reset_mock();(build/'GearsQuietStartup.dll').write_bytes(b'changed')
            with self.assertRaisesRegex(RuntimeError,'missing or changed'):
                background_launch.launch(self.game,quiet_build=build,popen=popen,runtime=self.runtime)
            popen.assert_not_called()

    def test_test_env_never_changes_manual_foreground_launch(self):
        popen=mock.Mock(return_value=Process(4200))
        with mock.patch.dict(background_launch.os.environ,{'GEARSVR_TEST_WINDOWS_BEHIND':'1'}):
            result=background_launch.launch(self.game,foreground=True,popen=popen,runtime=self.runtime)
        self.assertEqual(result['window_startup_policy'],'foreground')
        self.assertFalse(result['test_windows_behind_requested'])
        self.assertIsNone(popen.call_args.kwargs['startupinfo'])

    def test_birth_receipt_survives_background_helper_failure(self):
        _,_,bootstrap=self.default_quiet_fixture();self.quiet_patches()
        receipt=self.root/'birth.json'
        def spawn(*args,**kwargs):
            if receipt.exists():raise RuntimeError('helper failed')
            return bootstrap
        with self.assertRaisesRegex(RuntimeError,'helper failed'):
            background_launch.launch(self.game,disable_background_input=False,game_io=self.helper,runtime=self.runtime,popen=spawn,launch_receipt=receipt)
        value=json.loads(receipt.read_text())
        self.assertEqual(value['game_identity'],dict(pid=5100,creation_time=987654,image=str(self.game.resolve()).casefold()))
        self.assertIs(value['injection_attempted'],False);self.assertIs(value['prime_attempted'],False)
        popen=mock.Mock()
        with self.assertRaisesRegex(RuntimeError,'fresh'):
            background_launch.launch(self.game,popen=popen,game_io=self.helper,launch_receipt=receipt)
        popen.assert_not_called()

    def setUp(self):
        patcher = mock.patch.object(background_launch, 'process_creation_time', return_value=123456)
        patcher.start(); self.addCleanup(patcher.stop)
        environment = mock.patch.dict(background_launch.os.environ, {'GEARSVR_QUIET_STARTUP_BUILD':'','GEARSVR_TEST_WINDOWS_BEHIND':''})
        environment.start(); self.addCleanup(environment.stop)
        self.temp = tempfile.TemporaryDirectory()
        self.root = pathlib.Path(self.temp.name)
        self.game = self.root / 'GOWDE-Steam.exe'
        self.game.write_bytes(b'fixture')
        self.helper = self.root / 'background_input.py'
        self.helper.write_text('# fixture\n', encoding='utf-8')
        self.runtime = self.root / 'runtime'

    def tearDown(self):
        self.temp.cleanup()

    def test_explicit_background_input_uses_frozen_quiet_policy_and_hidden_helpers(self):
        build, _, bootstrap = self.default_quiet_fixture()
        self.quiet_patches()
        popen = mock.Mock(side_effect=[bootstrap, Process(5200)])
        result = background_launch.launch(self.game, ['-NoPauseUnfocus', 'SP_Prison_P'],
            disable_background_input=False, game_io=self.helper, runtime=self.runtime, popen=popen)
        self.assertEqual(popen.call_args_list[0].args[0], [str(build/'GearsEarlyLaunch.exe'), str(self.game),
            str(build/'GearsQuietStartup.dll'), '-NoPauseUnfocus', 'SP_Prison_P'])
        for call in popen.call_args_list:
            self.assertEqual(call.kwargs['creationflags'], background_launch.subprocess.CREATE_NO_WINDOW)
            self.assertEqual(call.kwargs['startupinfo'].wShowWindow, 0)
        self.assertEqual(popen.call_args_list[1].args[0][2:],
            ['enable', '--creation-time', '987654', '--pid', '5100', '--wait-seconds', '60'])
        self.assertEqual(result['window_startup_policy'], 'quiet_startup')
        self.assertEqual(popen.call_args_list[0].kwargs['env']['GEARSVR_QUIET_MAIN_NOACTIVATE'],'1')
        self.assertEqual(result['creation_time'], 987654)
        saved=json.loads((self.runtime/'background-launch-5100.json').read_text())
        self.assertEqual(saved['game_args'], ['-NoPauseUnfocus', 'SP_Prison_P'])

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
        result = background_launch.launch(self.game, foreground=True, disable_background_input=False,
                                          game_io=self.helper, runtime=self.runtime, popen=popen)
        self.assertEqual(popen.call_count, 2)
        self.assertTrue(result['background_input_requested'])

    def test_default_python_launch_never_starts_background_helper(self):
        _, _, bootstrap = self.default_quiet_fixture()
        self.quiet_patches()
        for foreground in (False, True):
            with self.subTest(foreground=foreground):
                popen = mock.Mock(return_value=Process(4200) if foreground else bootstrap)
                result = background_launch.launch(self.game, foreground=foreground,
                    game_io=self.root/'missing.py', runtime=self.runtime, popen=popen)
                self.assertEqual(popen.call_count, 1)
                self.assertFalse(result['background_input_requested'])
                self.assertNotIn('background_input_helper_pid', result)
                self.assertNotIn('background_input_stdout', result)
                saved = json.loads(pathlib.Path(result['record_path']).read_text())
                self.assertFalse(saved['background_input_requested'])

    def test_cli_background_input_requires_explicit_opt_in_and_rejects_conflicting_flags(self):
        with mock.patch.object(background_launch, 'launch', return_value={}) as launch, contextlib.redirect_stdout(io.StringIO()):
            base = ['--game-path', str(self.game)]
            for extra, disabled in (([], True), (['--disable-background-input'], True),
                                    (['--enable-background-input'], False),
                                    (['--foreground'], True), (['--foreground', '--enable-background-input'], False)):
                with self.subTest(extra=extra):
                    self.assertEqual(background_launch.main(base + extra), 0)
                    self.assertIs(launch.call_args.kwargs['disable_background_input'], disabled)
            launch.reset_mock()
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                background_launch.main(base + ['--enable-background-input', '--disable-background-input'])
            launch.assert_not_called()

    def test_command_line_preserves_dash_game_argument(self):
        with mock.patch.object(background_launch, 'launch', return_value={}) as launch:
            self.assertEqual(background_launch.main(['--game-path',str(self.game),'--game-arg=-NoPauseUnfocus']),0)
            self.assertEqual(launch.call_args.args[1], ['-NoPauseUnfocus'])

    def test_only_controller_disabled_keeps_quiet_startup(self):
        _, _, bootstrap = self.default_quiet_fixture()
        self.quiet_patches()
        popen = mock.Mock(return_value=bootstrap)
        result = background_launch.launch(self.game, disable_background_input=True,
            game_io=self.root/'missing.py', runtime=self.runtime, popen=popen)
        self.assertEqual(popen.call_count,1)
        self.assertFalse(result['background_input_requested'])
        self.assertEqual(result['window_startup_policy'],'quiet_startup')

    def default_quiet_fixture(self):
        import hashlib
        build, report, bootstrap = self.quiet_fixture()
        for name,value in (('DEFAULT_QUIET_BUILD',build),('DEFAULT_QUIET_SHA256',
                {name:hashlib.sha256(b'fixture').hexdigest() for name in background_launch.DEFAULT_QUIET_SHA256})):
            patcher=mock.patch.object(background_launch,name,value);patcher.start();self.addCleanup(patcher.stop)
        return build,report,bootstrap

    def test_missing_or_changed_default_never_starts_game_or_falls_back(self):
        build, _, _ = self.default_quiet_fixture()
        popen=mock.Mock()
        for name in background_launch.DEFAULT_QUIET_SHA256:
            file=build/name
            file.write_bytes(b'changed')
            with self.assertRaisesRegex(RuntimeError,'artifact missing or changed'):
                background_launch.launch(self.game,disable_background_input=True,popen=popen)
            file.unlink()
            with self.assertRaisesRegex(RuntimeError,'artifact missing or changed'):
                background_launch.launch(self.game,disable_background_input=True,popen=popen)
            file.write_bytes(b'fixture')
        popen.assert_not_called()

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
            quiet_build=build, disable_background_input=False, game_io=self.helper, runtime=self.runtime, popen=popen)
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

    def test_direct_quiet_launch_can_inherit_or_override_without_parent_changes(self):
        build, _, bootstrap = self.quiet_fixture()
        self.quiet_patches()
        key='GEARSVR_QUIET_MAIN_NOACTIVATE'
        for inherited in (None,'0','1'):
            with mock.patch.dict(quiet_launch.os.environ, {'QUIET_TEST_KEEP':'value'}, clear=True):
                if inherited is not None:quiet_launch.os.environ[key]=inherited
                for requested in (None,False,True):
                    with self.subTest(inherited=inherited,requested=requested):
                        parent=dict(quiet_launch.os.environ);popen=mock.Mock(return_value=bootstrap)
                        quiet_launch.launch_quiet(self.game,[],build,popen=popen,main_noactivate=requested)
                        child=popen.call_args.kwargs['env']
                        if requested is None:self.assertIsNone(child)
                        else:
                            self.assertEqual(child[key],'1' if requested else '0')
                            self.assertEqual(child['QUIET_TEST_KEEP'],'value')
                            self.assertIsNot(child,quiet_launch.os.environ)
                        self.assertEqual(dict(quiet_launch.os.environ),parent)

    def test_background_default_enforces_flag_even_if_parent_disabled_it(self):
        _, _, bootstrap = self.default_quiet_fixture()
        self.quiet_patches()
        with mock.patch.dict(background_launch.os.environ, {'GEARSVR_QUIET_MAIN_NOACTIVATE':'0'}):
            popen=mock.Mock(return_value=bootstrap)
            background_launch.launch(self.game,disable_background_input=True,runtime=self.runtime,popen=popen)
            self.assertEqual(popen.call_args.kwargs['env']['GEARSVR_QUIET_MAIN_NOACTIVATE'],'1')
            self.assertEqual(background_launch.os.environ['GEARSVR_QUIET_MAIN_NOACTIVATE'],'0')

    def test_direct_quiet_rejects_nonboolean_override_before_spawn(self):
        popen=mock.Mock()
        with self.assertRaises(ValueError):quiet_launch.launch_quiet(self.game,[],self.root,popen=popen,main_noactivate=1)
        popen.assert_not_called()

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
            self.assertEqual(background_launch.main(['--game-path',str(self.game),'--foreground']),0)
            self.assertIsNone(launch.call_args.kwargs['quiet_build'])

    def test_cli_without_environment_selects_frozen_default(self):
        with mock.patch.object(background_launch,'launch',return_value={}) as launch:
            self.assertEqual(background_launch.main(['--game-path',str(self.game)]),0)
            self.assertEqual(launch.call_args.kwargs['quiet_build'],background_launch.DEFAULT_QUIET_BUILD)


if __name__ == '__main__':
    unittest.main()
