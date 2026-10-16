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
                stack.enter_context(mock.patch.object(launch_reserve.time,'monotonic',side_effect=[0,1,13]))
                stack.enter_context(mock.patch.object(launch_reserve.time,'sleep'))
                stack.enter_context(mock.patch.object(launch_reserve,'WindowsControl',return_value=SimpleNamespace(identity=lambda pid:identity)))
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
