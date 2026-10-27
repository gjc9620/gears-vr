"""Runner/service lifecycle seam only; no game, simulator or input is launched."""
import pathlib
import tempfile
import types
import unittest
from unittest import mock

import run_stereo_continuous as runner
import path_choice_position_monitor as service


class Worker:
    error=None
    def __init__(self,probe):pass
    def close(self):return True


class PathChoiceRunnerTests(unittest.TestCase):
    def test_default_off_and_unsupported_build_never_create_monitor(self):
        for enabled in (False,True):
            state={'native_status':{}}
            args=types.SimpleNamespace(path_choice_center=enabled)
            with mock.patch.object(service,'PathChoicePositionMonitor') as monitor:
                self.assertIsNone(runner.start_path_choice_position(args,state,None,0,None))
                monitor.assert_not_called()
            self.assertEqual('path_choice_center_start_error' in state,enabled)

    def test_native99_paths_and_independent_status_reader(self):
        state={'native_status':{'path_position_service_compiled':'1'}}
        args=types.SimpleNamespace(path_choice_center=True,build_dir=pathlib.Path('native'),pid=123)
        control=types.SimpleNamespace(identity=mock.Mock())
        with mock.patch.object(service,'PathChoicePositionMonitor') as monitor, \
                mock.patch.object(runner,'read_session_status',return_value={'fresh':True}) as read:
            runner.start_path_choice_position(args,state,'status',42,control)
            position,close,session,read_status,identity=monitor.call_args.args
            self.assertEqual(position.name,'path-choice-position-trial.ini')
            self.assertEqual(close.name,'path-choice-close-observer.ini')
            self.assertIs(session,state);self.assertIs(identity,control.identity)
            self.assertEqual(read_status(),{'fresh':True})
            read.assert_called_once_with('status',123,42)

    def test_cleanup_proofs_and_unknown_sidecar_retention(self):
        # Exercise the real ownership guard through the runner's lifecycle seam.
        for clean,absent,unknown in ((True,False,False),(False,False,False),
                                      (False,True,False),(True,False,True)):
            with self.subTest(clean=clean,absent=absent,unknown=unknown),tempfile.TemporaryDirectory() as folder:
                position=pathlib.Path(folder)/'position.ini';close=pathlib.Path(folder)/'close.ini'
                state=dict(phase='running',native_started_qpc=50,
                    game_identity={'pid':3},owner_identity={'pid':4},
                    native_cleanup_verified=clean,entry_restore_verified=clean)
                monitor=service.PathChoicePositionMonitor(position,close,state,lambda:None,lambda pid:None,
                    probe_factory=lambda *args:None,worker_factory=Worker)
                monitor.position.publish(b'position');monitor.close_sidecar.publish(b'close')
                if unknown:
                    close.unlink();close.write_bytes(b'unknown owner')
                runner.close_path_choice_position(monitor,state)
                self.assertFalse(position.exists())
                self.assertTrue(close.exists())  # close evidence survives until native cleanup
                runner.finish_path_choice_position(monitor,state,
                    types.SimpleNamespace(game_absent=lambda pid:absent))
                self.assertTrue(state['path_choice_center_worker_joined'])
                self.assertEqual(close.exists(),unknown or not (clean or absent))
                self.assertEqual(state['path_choice_center_sidecars_cleaned'],(clean or absent) and not unknown)
                if unknown:self.assertEqual(close.read_bytes(),b'unknown owner')

    def test_ui_monitor_fault_does_not_abort_vr_cleanup(self):
        import test_run_stereo_continuous as lifecycle
        import menu_display_monitor
        with mock.patch.object(service,'PathChoicePositionMonitor') as factory, \
                mock.patch.object(menu_display_monitor,'MenuDisplayMonitor'):
            monitor=factory.return_value
            monitor.poll.side_effect=RuntimeError('UI observer failed')
            monitor.cleanup_error=None;monitor.worker_joined=True
            monitor.finish_cleanup.return_value=True
            code,state,*_=lifecycle.ContinuousTests().run_mock(path_center=True,startup_menu=True)
            self.assertEqual(code,0)
            self.assertNotIn('error',state)
            self.assertIn('UI observer failed',state['path_choice_center_poll_error'])
            self.assertTrue(state['native_cleanup_verified'])
            self.assertTrue(state['entry_restore_verified'])
            monitor.finish_cleanup.assert_called_once_with(native_cleanup_verified=True,
                entry_restore_verified=True,game_absent=False)


if __name__=='__main__':unittest.main()
