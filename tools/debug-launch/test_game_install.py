"""Installation validation and reservation argument tests; never launches the game."""
import hashlib
import contextlib
import io
import json
import os
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import game_install
sys.path.insert(0,str(HERE.parent/'native-capture'))
import launch_reserve


class InstallTests(unittest.TestCase):
    @unittest.skipUnless(os.name == 'nt', 'Windows downloaded-script handling')
    def test_marked_downloaded_script_can_probe_python_without_starting_game(self):
        import shutil
        import subprocess
        with tempfile.TemporaryDirectory(prefix='GearsVR downloaded script ') as directory:
            root=pathlib.Path(directory)
            script=root/'tools/debug-launch/Start-DebugGame.ps1'
            script.parent.mkdir(parents=True)
            shutil.copy2(HERE/'Start-DebugGame.ps1',script)
            pathlib.Path(str(script)+':Zone.Identifier').write_text('[ZoneTransfer]\r\nZoneId=3\r\n',encoding='ascii')
            command=game_install.debug_command(root/'never-started/Binaries_x64/GOWDE-Steam.exe',root)+['-PythonCheckOnly']
            result=subprocess.run(command,env=dict(os.environ,GEARSVR_PYTHON=sys.executable),
                capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=20,
                creationflags=subprocess.CREATE_NO_WINDOW)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertIs(json.loads(result.stdout)['check_only'],True)

    def test_windowless_startup_retains_error_output_before_birth(self):
        from types import SimpleNamespace
        command=['powershell','-File','startup fixture.ps1']
        for code in (0,1):
            stdout,stderr=io.StringIO(),io.StringIO()
            result=SimpleNamespace(returncode=code,stdout='startup output\n',
                stderr='RuntimeError: Expected exactly one user-settings file\n' if code else '')
            with mock.patch.object(launch_reserve,'run_hidden',return_value=result) as run,\
                    contextlib.redirect_stdout(stdout),contextlib.redirect_stderr(stderr):
                if code:
                    with self.assertRaisesRegex(RuntimeError,'Game startup script failed.*1'):
                        launch_reserve.run_game_startup(command)
                else:launch_reserve.run_game_startup(command)
            run.assert_called_once_with(command,capture_output=True,text=True,encoding='utf-8',errors='replace')
            self.assertEqual(stdout.getvalue(),result.stdout)
            self.assertEqual(stderr.getvalue(),result.stderr)

    def test_steam_launch_context_is_title_specific_and_does_not_modify_parent(self):
        for supplied in ({}, {'SteamAppId':'480', 'SteamGameId':'480', 'KEEP':'yes'},
                         {'STEAMAPPID':'480', 'steamgameid':'480', 'KEEP':'yes'}):
            before = dict(supplied)
            child = game_install.launch_environment(supplied)
            expected = {k:v for k,v in before.items() if k.casefold() not in ('steamappid', 'steamgameid')}
            self.assertEqual(child, dict(expected, SteamAppId='2523720', SteamGameId='2523720'))
            self.assertEqual(supplied, before)
            self.assertIsNot(child, supplied)
        with mock.patch.dict(os.environ, {'SteamAppId':'480', 'SteamGameId':'480'}):
            before = dict(os.environ)
            child = game_install.launch_environment()
            self.assertEqual(child['SteamAppId'], '2523720')
            self.assertEqual(child['SteamGameId'], '2523720')
            self.assertEqual(dict(os.environ), before)

    def test_experimental_upscaler_switch_is_explicit_and_preserves_background_flags(self):
        selected = pathlib.Path('X:/an install with spaces/Binaries_x64/GOWDE-Steam.exe')
        original = game_install.debug_command(selected)
        self.assertNotIn('-ExperimentalPreserveUpscaler', original)
        experimental = game_install.debug_command(selected, experimental_preserve_upscaler=True)
        self.assertEqual(experimental, original + ['-ExperimentalPreserveUpscaler'])
        self.assertIn('-DisableBackgroundInput', experimental)
        self.assertNotIn('-Foreground', experimental)

    def test_downloaded_startup_script_uses_only_child_execution_policy(self):
        command=game_install.debug_command(pathlib.Path('X:/Steam Game/Binaries_x64/GOWDE-Steam.exe'))
        self.assertEqual(command[command.index('-ExecutionPolicy')+1],'Bypass')
        self.assertLess(command.index('-ExecutionPolicy'),command.index('-File'))
        self.assertNotIn('Set-ExecutionPolicy',' '.join(command))

    def test_reserve_rejects_experimental_upscaler_lock_conflicts_before_preflight(self):
        for flag in ('--graphics-startup-lock', '--graphics-menu-lock'):
            with mock.patch.object(sys, 'argv', ['launch_reserve', '--experimental-preserve-upscaler', flag]), \
                    mock.patch.object(launch_reserve, 'validate_game') as validate, \
                    mock.patch.object(launch_reserve.subprocess, 'run') as run:
                with self.assertRaises(SystemExit):launch_reserve.main()
                validate.assert_not_called();run.assert_not_called()

    def fixture(self,root):
        exe=root/'custom folder/Binaries_x64/GOWDE-Steam.exe'
        exe.parent.mkdir(parents=True);exe.write_bytes(b'supported fixture executable')
        config=exe.parent.parent/'WarGame/Config/Steam/Cooked/Coalesced.ini'
        config.parent.mkdir(parents=True);config.write_bytes(b'supported fixture config')
        profile=root/'tools/package-inspector/reloaded-profile.json'
        profile.parent.mkdir(parents=True);profile.write_text(json.dumps({'sha256':hashlib.sha256(exe.read_bytes()).hexdigest()}))
        return exe,config

    def test_selection_and_read_only_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);exe,config=self.fixture(root)
            before={p:p.read_bytes() for p in root.rglob('*') if p.is_file()}
            with mock.patch.object(game_install,'CONFIG_HASHES',{hashlib.sha256(config.read_bytes()).hexdigest()}):
                self.assertEqual(game_install.validate_game(exe,root),exe.resolve())
                self.assertEqual(game_install.validate_game(exe.relative_to(root),root),exe.resolve())
                with self.assertRaisesRegex(RuntimeError,'profile'):
                    game_install.validate_game(exe,root,'unknown hash')
            self.assertEqual(before,{p:p.read_bytes() for p in root.rglob('*') if p.is_file()})
            command=game_install.debug_command(exe,root)
            self.assertIn('-DisableBackgroundInput',command)
            self.assertIn('-DisableBackgroundInput',game_install.debug_command(exe,root,foreground=True))
            self.assertEqual(command[-2:],['-GamePath',str(exe)])
            self.assertIn('custom folder',command[-1]) # one literal argument, no shell interpolation

    def test_bad_build_layout_config_and_directory_rejected_read_only(self):
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory);exe,config=self.fixture(root)
            valid=exe.read_bytes();config_valid=config.read_bytes()
            with mock.patch.object(game_install,'CONFIG_HASHES',{hashlib.sha256(config_valid).hexdigest()}):
                for path in (root,root/'missing.exe',exe.parent/'different.exe'):
                    with self.subTest(path=path),self.assertRaises(RuntimeError):game_install.validate_game(path,root)
                exe.write_bytes(b'unknown build')
                with self.assertRaisesRegex(RuntimeError,'Unvalidated'):game_install.validate_game(exe,root)
                exe.write_bytes(valid);config.write_bytes(b'unknown config')
                with self.assertRaisesRegex(RuntimeError,'directory/config'):game_install.validate_game(exe,root)
                self.assertEqual(config.read_bytes(),b'unknown config')
                config.unlink()
                with self.assertRaisesRegex(RuntimeError,'directory/config'):game_install.validate_game(exe,root)

    def test_reserve_rejects_path_before_subprocess_or_policy(self):
        with mock.patch.object(sys,'argv',['launch_reserve','--game-path','missing/Binaries_x64/GOWDE-Steam.exe']),mock.patch.object(launch_reserve.subprocess,'run') as run:
            with self.assertRaisesRegex(RuntimeError,'supported'):launch_reserve.main()
            run.assert_not_called()

    def test_default_reserve_passes_validated_exe_to_debug_launch(self):
        # Stop at the first self-test. Demonstrates preflight selection/default
        # without running any native process or loading/injecting a DLL.
        for explicit in (False,True):
            argv=['launch_reserve']+(['--game-path','X:/a folder/Binaries_x64/GOWDE-Steam.exe'] if explicit else [])
            accepted=game_install.DEFAULT_GAME.resolve()
            with mock.patch.object(sys,'argv',argv),mock.patch.object(launch_reserve,'validate_game',return_value=accepted) as validate,mock.patch.object(launch_reserve.subprocess,'run',side_effect=RuntimeError('test stop before self-test')):
                with self.assertRaisesRegex(RuntimeError,'test stop'):launch_reserve.main()
                value=validate.call_args.args[0]
                self.assertEqual(value,pathlib.Path(argv[-1]) if explicit else None)
                self.assertEqual(game_install.debug_command(accepted)[-1],str(accepted))

    def test_reserve_launches_exact_selected_exe_argument_without_shell_code(self):
        from types import SimpleNamespace
        selected=pathlib.Path('X:/an install with spaces/Binaries_x64/GOWDE-Steam.exe')
        symbol=SimpleNamespace(name=b'GearsCaptureReserve',forwarder=None,address=123)
        pe=SimpleNamespace(DIRECTORY_ENTRY_EXPORT=SimpleNamespace(symbols=[symbol]),close=mock.Mock())
        def subprocess_result(command,**kwargs):
            return SimpleNamespace(returncode=0,stdout='55' if 'Select-Object -ExpandProperty Id' in command[-1] else '',stderr='')
        with mock.patch.object(sys,'argv',['launch_reserve','--game-path',str(selected)]),mock.patch.object(launch_reserve,'validate_game',return_value=selected),mock.patch.object(launch_reserve.pefile,'PE',return_value=pe),mock.patch.object(launch_reserve.subprocess,'run',side_effect=subprocess_result) as run,mock.patch.object(launch_reserve.k,'OpenProcess',side_effect=RuntimeError('stop before process access')),mock.patch.object(launch_reserve,'WindowsControl',return_value=SimpleNamespace(identity=lambda pid:dict(pid=pid,creation_time=456,image=str(selected)))),mock.patch.object(launch_reserve,'preparation_live_guard',return_value=lambda:None):
            with self.assertRaisesRegex(RuntimeError,'stop before process'):launch_reserve.main()
            commands=[call.args[0] for call in run.call_args_list]
            self.assertIn(game_install.debug_command(selected),commands)
            self.assertEqual(sum('-File' in command for command in commands),1)

    def test_reserve_experimental_flag_reaches_powershell_command(self):
        from types import SimpleNamespace
        selected = pathlib.Path('X:/an install with spaces/Binaries_x64/GOWDE-Steam.exe')
        pe = SimpleNamespace(DIRECTORY_ENTRY_EXPORT=SimpleNamespace(symbols=[
            SimpleNamespace(name=b'GearsCaptureReserve', forwarder=None, address=123)]), close=mock.Mock())
        def result(command, **kwargs):
            return SimpleNamespace(returncode=0, stdout='55' if 'Select-Object -ExpandProperty Id' in command[-1] else '', stderr='')
        with mock.patch.object(sys, 'argv', ['launch_reserve', '--experimental-preserve-upscaler']), \
                mock.patch.object(launch_reserve, 'validate_game', return_value=selected), \
                mock.patch.object(launch_reserve.pefile, 'PE', return_value=pe), \
                mock.patch.object(launch_reserve.subprocess, 'run', side_effect=result) as run, \
                mock.patch.object(launch_reserve.k, 'OpenProcess', side_effect=RuntimeError('stop before process access')), \
                mock.patch.object(launch_reserve, 'WindowsControl', return_value=SimpleNamespace(
                    identity=lambda pid:dict(pid=pid,creation_time=456,image=str(selected)))), \
                mock.patch.object(launch_reserve, 'preparation_live_guard', return_value=lambda:None):
            with self.assertRaisesRegex(RuntimeError, 'stop before process'):launch_reserve.main()
            commands = [call.args[0] for call in run.call_args_list]
            self.assertIn(game_install.debug_command(selected, experimental_preserve_upscaler=True), commands)

    def test_powershell_rejects_unknown_path_without_policy_or_launch(self):
        import subprocess
        # The real PS entrypoint is safe for an invalid path: its first Python
        # validation exits before either policy application or Start-Process.
        result=subprocess.run(['powershell','-NoProfile','-NonInteractive','-File',str(HERE/'Start-DebugGame.ps1'),'-GamePath',str(HERE/'missing/Binaries_x64/GOWDE-Steam.exe')],capture_output=True,text=True)
        self.assertNotEqual(result.returncode,0)
        self.assertIn('no startup changes made',result.stderr)


if __name__=='__main__':unittest.main()
