"""Receipt transitions only: never starts or inspects a game."""
import json
import pathlib
import tempfile
import unittest
import contextlib
import io
import sys
from types import SimpleNamespace
from unittest import mock
import launch_reserve


class ReserveLifecycleTests(unittest.TestCase):
    def test_loader_timeout_reports_last_existing_snapshot_without_an_extra_poll(self):
        exe=pathlib.Path('J:/fixture/GOWDE-Steam.exe');ticks=[0.]
        identity=dict(pid=55,creation_time=456,image=str(exe).casefold())
        born=dict(schema=1,kind='reservation_birth',game_identity=identity,injection_attempted=False,prime_attempted=False)
        loaded={name:(1,exe) for name in ['gowde-steam.exe','kernelbase.dll']+[f'module{i:03d}.dll' for i in range(260)]}
        output=io.StringIO()
        with mock.patch.object(launch_reserve,'WindowsControl',return_value=SimpleNamespace(identity=lambda pid:identity)),\
             mock.patch.object(launch_reserve,'preparation_live_guard',return_value=mock.Mock()),\
             mock.patch.object(launch_reserve,'k') as native,\
             mock.patch.object(launch_reserve,'modules',return_value=loaded) as read,\
             mock.patch.object(launch_reserve,'run_hidden') as shell,contextlib.redirect_stdout(output):
            native.OpenProcess.return_value=123
            with self.assertRaisesRegex(RuntimeError,'dependencies not ready'):
                launch_reserve.wait_loader_dependencies(exe,born,1,clock=lambda:ticks[0],sleep=lambda delay:ticks.__setitem__(0,1.))
            read.assert_called_once_with(123);shell.assert_not_called();native.CloseHandle.assert_called_once_with(123)
        report=json.loads(output.getvalue())
        self.assertEqual(report['last_loaded_modules'],sorted(loaded)[:256])
        self.assertEqual(report['loaded_module_count'],262);self.assertTrue(report['loaded_modules_truncated'])
        self.assertEqual(report['missing_modules'],['d3d12.dll','dxgi.dll'])

    def test_command_budget_survives_slow_validation_and_prevents_injection(self):
        exe=pathlib.Path('J:/fixture/GOWDE-Steam.exe');ticks=[0.]
        identity=dict(pid=55,creation_time=456,image=str(exe))
        pe=SimpleNamespace(close=mock.Mock(),DIRECTORY_ENTRY_EXPORT=SimpleNamespace(symbols=[
            SimpleNamespace(name=b'GearsCaptureReserve',forwarder=None,address=123)]))
        def hidden(*args,**kwargs):ticks[0]=30;return SimpleNamespace(returncode=0)
        def validate(*args):
            if ticks[0]==30:ticks[0]=51
            return exe
        with contextlib.ExitStack() as stack:
            stack.enter_context(mock.patch.object(sys,'argv',['launch_reserve']))
            stack.enter_context(mock.patch.object(launch_reserve,'validate_game',side_effect=validate))
            stack.enter_context(mock.patch.object(launch_reserve,'run_hidden',side_effect=hidden))
            stack.enter_context(mock.patch.object(launch_reserve.pefile,'PE',return_value=pe))
            stack.enter_context(mock.patch.object(launch_reserve.time,'monotonic',side_effect=lambda:ticks[0]))
            guard=mock.Mock()
            wait=stack.enter_context(mock.patch.object(launch_reserve,'wait_loader_dependencies',return_value=(
                55,123,{'gowde-steam.exe':(1,exe)},identity,guard)))
            stack.enter_context(mock.patch.object(launch_reserve,'WindowsControl',return_value=SimpleNamespace(identity=lambda pid:identity)))
            native=stack.enter_context(mock.patch.object(launch_reserve,'k'))
            marked=stack.enter_context(mock.patch.object(launch_reserve,'mark_launch_stage'))
            remote=stack.enter_context(mock.patch.object(launch_reserve,'remote_call'))
            with self.assertRaisesRegex(RuntimeError,'deadline before injection'):launch_reserve.main()
            self.assertEqual(wait.call_args.args[2],50)
            marked.assert_not_called();native.VirtualAllocEx.assert_not_called();remote.assert_not_called()
            native.CloseHandle.assert_called_once_with(123)

    def test_owner_exit_or_cancel_prevents_even_legacy_discovery(self):
        exe=pathlib.Path('J:/fixture/GOWDE-Steam.exe')
        for cancelled in (False,True):
            with self.subTest(cancelled=cancelled),contextlib.ExitStack() as stack:
                stack.enter_context(mock.patch.object(launch_reserve,'WindowsControl',return_value=SimpleNamespace(
                    identity=lambda pid:dict(pid=99,creation_time=456) if cancelled else None)))
                native=stack.enter_context(mock.patch.object(launch_reserve,'k'))
                native.OpenEventW.return_value=10;native.WaitForSingleObject.return_value=0
                shell=stack.enter_context(mock.patch.object(launch_reserve,'run_hidden'))
                read=stack.enter_context(mock.patch.object(launch_reserve,'modules'))
                with self.assertRaisesRegex(RuntimeError,'Owner'):
                    launch_reserve.wait_loader_dependencies(exe,None,25,clock=lambda:0,sleep=mock.Mock(),
                        owner_pid=99,owner_creation_time=456)
                native.OpenProcess.assert_not_called();shell.assert_not_called();read.assert_not_called()
                if cancelled:native.CloseHandle.assert_called_once_with(10)

    def test_exact_birth_waits_for_delayed_graphics_without_name_enumeration(self):
        exe=pathlib.Path('J:/fixture/GOWDE-Steam.exe');ticks=[0.]
        identity=dict(pid=55,creation_time=456,image=str(exe).casefold())
        born=dict(schema=1,kind='reservation_birth',game_identity=identity,injection_attempted=False,prime_attempted=False)
        loaded={name:(1,exe) for name in ('kernelbase.dll','gowde-steam.exe','d3d12.dll','dxgi.dll')}
        def read(handle):return loaded if ticks[0]>=13 else {'gowde-steam.exe':(1,exe)}
        with mock.patch.object(launch_reserve,'WindowsControl',return_value=SimpleNamespace(identity=lambda pid:identity)),\
             mock.patch.object(launch_reserve,'preparation_live_guard',return_value=mock.Mock()) as guard,\
             mock.patch.object(launch_reserve,'k') as native,mock.patch.object(launch_reserve,'modules',side_effect=read),\
             mock.patch.object(launch_reserve,'run_hidden') as shell:
            native.OpenProcess.return_value=123
            value=launch_reserve.wait_loader_dependencies(exe,born,25,clock=lambda:ticks[0],
                sleep=lambda delay:ticks.__setitem__(0,ticks[0]+delay))
            self.assertGreaterEqual(ticks[0],13);self.assertLess(ticks[0],25)
            self.assertEqual(value[:4],(55,123,loaded,identity))
            native.OpenProcess.assert_called_once_with(0x43a,False,55);native.CloseHandle.assert_not_called()
            shell.assert_not_called();guard.assert_called_once_with(55,None,None)

    def test_wait_refuses_reused_identity_cancellation_and_late_success(self):
        exe=pathlib.Path('J:/fixture/GOWDE-Steam.exe')
        identity=dict(pid=55,creation_time=456,image=str(exe).casefold())
        born=dict(schema=1,kind='reservation_birth',game_identity=identity,injection_attempted=False,prime_attempted=False)
        for scenario in ('reused_before','reused_after','cancel_before','cancel_after','late','expired','slow_open'):
            with self.subTest(scenario=scenario),contextlib.ExitStack() as stack:
                ticks=[25. if scenario=='expired' else 0.]
                changed=dict(identity,creation_time=789)
                identities=[changed] if scenario=='reused_before' else [identity,changed] if scenario=='reused_after' else None
                control=SimpleNamespace(identity=mock.Mock(**({'side_effect':identities} if identities else {'return_value':identity})))
                stack.enter_context(mock.patch.object(launch_reserve,'WindowsControl',return_value=control))
                checks=mock.Mock(side_effect=RuntimeError('Owner cancelled') if scenario=='cancel_before' else
                                 [None,None,RuntimeError('Owner cancelled')] if scenario=='cancel_after' else None)
                stack.enter_context(mock.patch.object(launch_reserve,'preparation_live_guard',return_value=checks))
                native=stack.enter_context(mock.patch.object(launch_reserve,'k'));native.OpenProcess.return_value=123
                if scenario=='slow_open':
                    def slow_open(*args):ticks[0]=25;return 123
                    native.OpenProcess.side_effect=slow_open
                def read(handle):
                    if scenario=='late':ticks[0]=25
                    return {name:(1,exe) for name in ('kernelbase.dll','gowde-steam.exe','d3d12.dll','dxgi.dll')}
                read=stack.enter_context(mock.patch.object(launch_reserve,'modules',side_effect=read))
                shell=stack.enter_context(mock.patch.object(launch_reserve,'run_hidden'))
                stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
                with self.assertRaises(RuntimeError):
                    launch_reserve.wait_loader_dependencies(exe,born,25,clock=lambda:ticks[0],sleep=lambda delay:None)
                if scenario in ('reused_before','cancel_before','expired'):
                    native.OpenProcess.assert_not_called();read.assert_not_called();native.CloseHandle.assert_not_called()
                elif scenario=='slow_open':read.assert_not_called();native.CloseHandle.assert_called_once_with(123)
                else:read.assert_called_once();native.CloseHandle.assert_called_once_with(123)
                shell.assert_not_called()

    def test_legacy_enumeration_is_rate_limited_and_locks_selected_installation(self):
        exe=pathlib.Path('J:/fixture/GOWDE-Steam.exe');identity=dict(pid=55,creation_time=456,image=str(exe))
        for wrong_path in (False,True):
            with self.subTest(wrong_path=wrong_path),contextlib.ExitStack() as stack:
                ticks=[0.];observed=[]
                def scan(command,**kwargs):
                    observed.append(ticks[0]);return SimpleNamespace(stdout='' if len(observed)==1 else '55')
                stack.enter_context(mock.patch.object(launch_reserve,'run_hidden',side_effect=scan))
                stack.enter_context(mock.patch.object(launch_reserve,'WindowsControl',return_value=SimpleNamespace(
                    identity=lambda pid:dict(identity,image='J:/other/GOWDE-Steam.exe') if wrong_path else identity)))
                stack.enter_context(mock.patch.object(launch_reserve,'preparation_live_guard',return_value=lambda:None))
                native=stack.enter_context(mock.patch.object(launch_reserve,'k'));native.OpenProcess.return_value=123
                stack.enter_context(mock.patch.object(launch_reserve,'modules',return_value={
                    name:(1,exe) for name in ('kernelbase.dll','gowde-steam.exe','d3d12.dll','dxgi.dll')}))
                def wait():return launch_reserve.wait_loader_dependencies(exe,None,25,clock=lambda:ticks[0],
                    sleep=lambda delay:ticks.__setitem__(0,ticks[0]+delay))
                if wrong_path:
                    with self.assertRaisesRegex(RuntimeError,'path differs'):wait()
                    native.OpenProcess.assert_not_called()
                else:self.assertEqual(wait()[0],55)
                self.assertEqual(len(observed),2);self.assertGreaterEqual(observed[1]-observed[0],1)

    def test_loader_timeout_keeps_exact_birth_and_never_attempts_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            exe=pathlib.Path(directory)/'GOWDE-Steam.exe';receipt=pathlib.Path(directory)/'birth.json'
            identity=dict(pid=55,creation_time=456,image=str(exe.resolve()).casefold())
            value=dict(schema=1,kind='reservation_birth',game_identity=identity,
                       injection_attempted=False,prime_attempted=False)
            def hidden(command,**kwargs):
                if '-LaunchReceipt' in command:receipt.write_text(json.dumps(value))
                return SimpleNamespace(returncode=0,stdout='55' if 'Select-Object -ExpandProperty Id' in command[-1] else '')
            pe=SimpleNamespace(close=mock.Mock(),DIRECTORY_ENTRY_EXPORT=SimpleNamespace(symbols=[
                SimpleNamespace(name=b'GearsCaptureReserve',forwarder=None,address=123)]))
            with contextlib.ExitStack() as stack:
                stack.enter_context(mock.patch.object(sys,'argv',['launch_reserve','--launch-receipt',str(receipt)]))
                stack.enter_context(mock.patch.object(launch_reserve,'validate_game',return_value=exe))
                stack.enter_context(mock.patch.object(launch_reserve,'run_hidden',side_effect=hidden))
                stack.enter_context(mock.patch.object(launch_reserve.pefile,'PE',return_value=pe))
                stack.enter_context(mock.patch.object(launch_reserve.time,'monotonic',side_effect=[0,0,1,26,26]))
                stack.enter_context(mock.patch.object(launch_reserve.time,'sleep'))
                stack.enter_context(mock.patch.object(launch_reserve,'WindowsControl',return_value=SimpleNamespace(identity=lambda pid:identity)))
                stack.enter_context(mock.patch.object(launch_reserve,'preparation_live_guard',return_value=lambda:None))
                native=stack.enter_context(mock.patch.object(launch_reserve,'k'));native.OpenProcess.return_value=1
                stack.enter_context(mock.patch.object(launch_reserve,'modules',return_value={}))
                remote=stack.enter_context(mock.patch.object(launch_reserve,'remote_call'))
                output=stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
                with self.assertRaisesRegex(RuntimeError,'dependencies not ready'):launch_reserve.main()
                self.assertIn('reservation_birth',output.getvalue())
                self.assertEqual(json.loads(receipt.read_text()),value)
                native.VirtualAllocEx.assert_not_called();native.WriteProcessMemory.assert_not_called();remote.assert_not_called()

    def test_injection_receipt_is_monotonic_and_identity_checked(self):
        identity=dict(pid=55,creation_time=456,image='fixture.exe')
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'birth.json'
            path.write_text(json.dumps(dict(schema=1,kind='reservation_birth',game_identity=identity,
                                           injection_attempted=False,prime_attempted=False)))
            with self.assertRaisesRegex(RuntimeError,'identity'):
                launch_reserve.mark_launch_stage(path,dict(identity,creation_time=789),'injection_attempted')
            self.assertFalse(json.loads(path.read_text())['injection_attempted'])
            launch_reserve.mark_launch_stage(path,identity,'injection_attempted')
            launch_reserve.mark_launch_stage(path,identity,'prime_attempted')
            value=json.loads(path.read_text())
            self.assertIs(value['injection_attempted'],True);self.assertIs(value['prime_attempted'],True)


if __name__=='__main__':unittest.main()
