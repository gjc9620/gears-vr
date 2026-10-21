"""Startup failure receipts, including real query-only observation of a Python child."""
import ctypes
from ctypes import wintypes
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

import startup_diagnostics as diagnostics


class StartupDiagnosticsTests(unittest.TestCase):
    def child_identity(self, child):
        kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
        kernel.GetProcessTimes.restype = wintypes.BOOL
        created, exited, system, user = (wintypes.FILETIME() for _ in range(4))
        self.assertTrue(kernel.GetProcessTimes(int(child._handle), ctypes.byref(created), ctypes.byref(exited),
                                              ctypes.byref(system), ctypes.byref(user)))
        return dict(pid=child.pid, creation_time=(created.dwHighDateTime << 32) | created.dwLowDateTime)

    def python_child(self):
        child = subprocess.Popen([sys.executable, '-I', '-B', '-c', 'import sys; sys.stdin.read(1); sys.exit(1)'],
            stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        def finish():
            if child.poll() is None:
                child.stdin.write(b'x'); child.stdin.flush()
                child.wait(timeout=10)
            child.stdin.close()
        self.addCleanup(finish)
        return child

    def test_same_handle_retains_real_exit_code_and_keeps_thread_result_separate(self):
        child = self.python_child()
        identity = self.child_identity(child)
        observer = diagnostics.ProcessObservation(identity)
        self.addCleanup(observer.close)
        self.assertEqual(observer.snapshot()['state'], 'running')
        child.stdin.write(b'x'); child.stdin.flush(); child.wait(timeout=10)
        value = observer.snapshot()
        self.assertEqual(value['state'], 'exited')
        self.assertTrue(value['same_identity'])
        self.assertEqual(value['exit_code'], 1)
        self.assertEqual(value['exit_code_hex'], '0x00000001')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'attempt.jsonl'
            trace = diagnostics.StartupTrace(path, metadata={'version':'0.0.2'}, attempt_id='a'*32)
            trace.observation = observer
            trace.begin('target_priming'); trace.end(remote_thread_exit_code=1)
            trace.failure(RuntimeError('Target priming failed (1); do not activate'))
            rows = diagnostics.read_trace(path)
            self.assertEqual(rows[-2]['remote_thread_exit_code'], 1)
            self.assertEqual(rows[-1]['process']['exit_code'], 1)
            self.assertEqual(rows[-1]['phase'], 'target_priming')

    def test_reused_identity_is_not_reported_as_a_running_game(self):
        child = self.python_child()
        identity = self.child_identity(child)
        observer = diagnostics.ProcessObservation(dict(identity, creation_time=identity['creation_time']+1))
        self.addCleanup(observer.close)
        self.assertIsNone(observer.handle)
        self.assertEqual(observer.snapshot()['state'], 'identity_changed')
        self.assertFalse(observer.snapshot()['same_identity'])

    def test_trace_io_failure_does_not_prevent_or_mask_launch_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'trace.jsonl'
            with mock.patch.object(Path, 'open', side_effect=PermissionError('logging denied')):
                trace = diagnostics.StartupTrace(path, metadata={}, attempt_id='b'*32)
            trace.begin('launch_script')
            original = RuntimeError('original startup failure')
            try:
                raise original
            except RuntimeError as error:
                trace.failure(error)
                self.assertIs(error, original)
            trace.close()
            self.assertIsNone(trace.path)
            trace = diagnostics.StartupTrace(path, metadata={}, attempt_id='b'*32)
            trace.observation = mock.Mock()
            trace.observation.snapshot.side_effect = PermissionError('query denied')
            trace.failure(original)
            last = diagnostics.read_trace(path)[-1]
            self.assertIn('original startup failure', last['error'])
            self.assertEqual(last['process']['state'], 'unavailable')
            trace.close()

    def test_resume_preserves_attempt_and_timeline_while_partial_tail_is_ignored(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'trace.jsonl'
            first = diagnostics.StartupTrace(path, metadata={}, attempt_id='c'*32)
            second = diagnostics.StartupTrace(path, append=True)
            self.assertEqual(second.started_ns, first.started_ns)
            self.assertEqual(second.attempt_id, first.attempt_id)
            second.begin('selftests'); second.end(scheduled_tests=22)
            with path.open('ab') as stream:stream.write(b'{"partial":')
            rows = diagnostics.read_trace(path)
            self.assertEqual(len(rows), 3)
            self.assertEqual(rows[-1]['scheduled_tests'], 22)

    def test_trace_size_is_bounded_and_failures_are_best_effort(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'trace.jsonl'
            trace = diagnostics.StartupTrace(path, metadata={}, attempt_id='d'*32)
            for _ in range(80):trace.emit('owner_failure', error='x'*4096)
            self.assertLessEqual(path.stat().st_size, diagnostics.MAX_TRACE_BYTES)

    def test_prime_status_rejects_old_and_wrong_pid_and_never_copies_bytes(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'target-prime-status.txt'
            start = time.time_ns()
            self.assertEqual(diagnostics.prime_status(path, start, 55)['state'], 'missing')
            path.write_text('hook_target=0x123456 create=MH_OK queue=MH_ERROR_MEMORY_ALLOC\n'
                'prime_entry before=488bc45356574154415541564157\n'
                'graphics_prime_admission rejected=1 reason=7 policy_reason=2 window=0x987654\n'
                'prime_setup_failed\ncapturing pid=55 lifetime=owner\n')
            value = diagnostics.prime_status(path, start, 55)
            self.assertEqual(value['state'], 'captured')
            self.assertTrue(value['pid_verified']); self.assertTrue(value['setup_failed'])
            self.assertEqual(value['hooks'][0]['queue'], 'MH_ERROR_MEMORY_ALLOC')
            self.assertEqual(value['graphics_rejection'], {'reason':7, 'policy_reason':2})
            self.assertNotIn('488bc4', json.dumps(value)); self.assertNotIn('0x123456', json.dumps(value))
            self.assertEqual(diagnostics.prime_status(path, start, 56)['state'], 'pid_mismatch')
            self.assertEqual(diagnostics.prime_status(path, time.time_ns()+10**9, 55)['state'], 'stale')

    def test_context_only_records_owned_fields_and_fingerprints(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root/'tools/vr-launcher').mkdir(parents=True)
            (root/'tools/vr-launcher/launcher.py').write_text('fixture')
            with mock.patch.dict(os.environ, {'PASSWORD':'PRIVATE_SECRET', 'XR_RUNTIME_JSON':'C:/PrivateUser/runtime.json'}):
                value = diagnostics.context(root, {'native_sha256':'a'*64, 'game_path':'PRIVATE_GAME_PATH'})
            self.assertEqual(value['version'], '0.0.2')
            self.assertEqual(value['runtime_selection'], 'override')
            self.assertEqual(value['steam_app_id'], '2523720')
            self.assertIn('tools/vr-launcher/launcher.py', value['script_hashes'])
            self.assertNotIn('PRIVATE', json.dumps(value))

    def test_real_reservation_entry_records_validation_failure_before_game_creation(self):
        root = Path(__file__).resolve().parents[2]
        sys.path.insert(0, str(root/'tools/debug-launch'))
        sys.path.insert(0, str(root/'tools/native-capture'))
        import launch_reserve
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'trace.jsonl'
            diagnostics.StartupTrace(path, metadata={'version':'0.0.2'}, attempt_id='e'*32)
            argv = ['launch_reserve.py', '--startup-diagnostics', str(path), '--game-path',
                    str(Path(directory)/'missing/Binaries_x64/GOWDE-Steam.exe')]
            with mock.patch.object(sys, 'argv', argv), mock.patch.object(launch_reserve, 'run_hidden') as launch:
                with self.assertRaisesRegex(RuntimeError, 'supported'):
                    launch_reserve.main()
            launch.assert_not_called()
            rows = diagnostics.read_trace(path)
            self.assertEqual(rows[-1]['event'], 'failure')
            self.assertEqual(rows[-1]['phase'], 'game_validation')
            self.assertEqual(rows[-1]['exception_type'], 'RuntimeError')

    def test_status_from_selftests_is_not_attached_before_actual_priming(self):
        with tempfile.TemporaryDirectory() as directory:
            trace = diagnostics.StartupTrace(Path(directory)/'trace.jsonl', metadata={}, attempt_id='f'*32)
            trace.observation = mock.Mock()
            trace.observation.snapshot.return_value = {'pid':55, 'state':'running'}
            trace.status_path = Path(directory)/'target-prime-status.txt'
            trace.begin('entry_verification')
            with mock.patch.object(diagnostics, 'prime_status') as read:
                trace.failure(RuntimeError('entry mismatch'))
            read.assert_not_called()
            trace.close()


if __name__ == '__main__':
    unittest.main()
