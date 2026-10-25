"""No game, native writes or XR; guardian ownership uses short-lived test threads."""
import copy
import gc
import json
from pathlib import Path
import tempfile
import threading
import unittest
from unittest import mock
import weakref

from run_camera_session import RunCameraSession


def identity(pid):
    return dict(pid=pid, creation_time=pid * 10, image='synthetic-' + str(pid))


class FakePolicy:
    def __init__(self, allow):
        self.identity, self.allow = identity(123), allow
        self.ticks = self.closes = 0
        self.report = dict(applied=False, closed=False, released=False, pending_fields=['owned'], errors=[])
        self.close_results = []

    def tick(self):
        self.ticks += 1
        self.report['applied'] = True

    def close(self):
        self.closes += 1
        if self.close_results:
            result = self.close_results.pop(0)
            if isinstance(result, Exception):
                raise result
            return result
        if self.allow.is_set():
            self.report.update(applied=False, closed=True, released=True, pending_fields=[])
        return copy.deepcopy(self.report)


class SessionTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.path = Path(folder.name) / 'specific-session.json'
        self.original = dict(session_id='specific-token', owner_identity=identity(777), game_identity=identity(123),
                             launcher_metadata='must survive cleanup')
        self.path.write_text(json.dumps(self.original), encoding='utf-8')
        self.allowed = threading.Event()
        self.policy = FakePolicy(self.allowed)
        self.now, self.published = 0., []

    def publish(self, path, report):
        self.published.append((path, copy.deepcopy(report)))
        path.write_text(json.dumps(report), encoding='utf-8')

    def session(self, **kwargs):
        session = RunCameraSession(identity(123), identity, session_path=self.path, session_id='specific-token',
            owner=identity(777), publish=kwargs.pop('publish', self.publish),
            policy_factory=kwargs.pop('policy_factory', lambda *_: self.policy),
            clock=lambda: self.now, retry_wait=lambda _: threading.Event().wait(.01), **kwargs)

        reference = weakref.ref(session)
        def finish():
            self.allowed.set()
            pending = reference()
            if pending is not None:
                pending.close(attempts=3)
                pending.finish(lambda: None)
                pending.thread.join(timeout=2.)

        self.addCleanup(finish)  # An assertion failure must not leave an idle non-daemon test worker.
        return session

    def test_tick_retries_unready_construction_at_most_once_per_second(self):
        calls = []

        def factory(*_):
            calls.append(1)
            if len(calls) == 1:
                raise OSError('mesh not ready')
            return self.policy

        session = self.session(policy_factory=factory)
        session.tick()
        for _ in range(20):
            session.tick()
        self.assertEqual(len(calls), 1)
        self.assertIn('mesh not ready', session.snapshot()['session_errors'][0]['reason'])
        self.now = 1.
        session.tick()
        self.assertEqual(len(calls), 2)
        self.assertEqual(self.policy.ticks, 1)

    def test_close_is_bounded_fail_closed_and_guardian_never_ticks(self):
        session = self.session()
        session.tick()
        self.policy.close_results = [OSError('temporarily unreadable'), dict(closed=True, released=False, pending_fields=[]),
                                     dict(closed=True, released=True)]
        self.assertFalse(session.close(attempts=1000))
        self.assertEqual(self.policy.closes, 3)
        self.assertTrue(session.pending)
        session.tick()
        self.assertEqual(self.policy.ticks, 1)
        self.assertEqual(len(session.snapshot()['session_errors']), 3)

    def test_pending_guardian_retains_object_and_lock_then_publishes_only_its_sidecar(self):
        session = self.session()
        session.tick()
        self.assertFalse(session.thread.daemon)
        self.assertFalse(session.close())
        releases, done, thread = [], session.done, session.thread
        weak_session, weak_policy = weakref.ref(session), weakref.ref(session.policy)
        self.assertTrue(session.finish(lambda: releases.append('original continuous lock')))
        del self.policy
        del session
        gc.collect()
        self.assertIsNotNone(weak_session())
        self.assertIsNotNone(weak_policy())
        self.assertEqual(releases, [])
        self.assertFalse(done.is_set())
        self.assertEqual(json.loads(self.path.read_text()), self.original)
        self.allowed.set()
        self.assertTrue(done.wait(2.))
        thread.join(2.)
        self.assertEqual(releases, ['original continuous lock'])
        sidecar = self.path.with_suffix('.camera-cleanup.json')
        report = json.loads(sidecar.read_text())
        self.assertEqual(report['session_id'], 'specific-token')
        self.assertTrue(report['policy_closed'])
        self.assertFalse(report['pending'])
        self.assertFalse(report['owner_lock_retained'])
        self.assertTrue(all(path == sidecar for path, _ in self.published))
        self.assertEqual(json.loads(self.path.read_text()), self.original)

    def test_session_replacement_refuses_sidecar_write_without_discarding_policy(self):
        session = self.session()
        session.tick()
        session._publish()
        before = session.sidecar.read_bytes()
        self.path.write_text(json.dumps(dict(self.original, session_id='another-session')), encoding='utf-8')
        session._publish()
        self.assertEqual(session.sidecar.read_bytes(), before)
        self.assertIn('session identity changed', session.snapshot()['session_last_error']['reason'])
        self.assertTrue(session.pending)

    def test_publish_failure_does_not_release_pending_lock_or_hide_responsibility(self):
        def unavailable(*_):
            raise OSError('report sharing error')

        session = self.session(publish=unavailable)
        session.tick()
        session._publish()
        self.assertIn('report sharing error', session.snapshot()['session_last_error']['reason'])
        releases = []
        self.assertTrue(session.finish(lambda: releases.append(True)))
        self.assertEqual(releases, [])
        self.allowed.set()
        self.assertTrue(session.done.wait(2.))
        self.assertEqual(releases, [True])

    def test_closehandle_false_retains_guardian_lock_until_verified_success(self):
        from test_run_stereo_continuous import runner
        permitted, published_pending = threading.Event(), threading.Event()
        control = runner.WindowsControl.__new__(runner.WindowsControl)
        control.k = mock.Mock()
        control.k.CloseHandle.side_effect = lambda _handle: permitted.is_set()

        def publish(path, report):
            self.publish(path, report)
            if report['owner_lock_retained']:
                published_pending.set()

        session = self.session(publish=publish)
        session.tick()
        self.allowed.set()  # Policy can release; the native owner handle cannot yet.
        with mock.patch.object(runner.ctypes, 'get_last_error', return_value=6):
            with self.assertRaises(OSError) as failed:
                control.close_lock(0xbeef)
            self.assertEqual(failed.exception.winerror, 6)
            self.assertTrue(session.finish(lambda: control.close_lock(0xbeef)))
            try:
                self.assertTrue(published_pending.wait(2.))
                self.assertTrue(session.closed)
                self.assertFalse(session.lock_released)
                self.assertFalse(session.done.is_set())
                report = json.loads(session.sidecar.read_text())
                self.assertTrue(report['policy_closed'])
                self.assertTrue(report['pending'])
                self.assertTrue(report['owner_lock_retained'])
                self.assertTrue(any(item['stage'] == 'owner_lock_close' for item in session.snapshot()['session_errors']))
            finally:
                permitted.set()
                self.assertTrue(session.done.wait(2.))
                session.thread.join(2.)
        self.assertTrue(session.lock_released)
        self.assertFalse(json.loads(session.sidecar.read_text())['owner_lock_retained'])
        self.assertTrue(all(call.args == (0xbeef,) for call in control.k.CloseHandle.call_args_list))

    def test_reused_process_cannot_receive_first_tick_and_idle_worker_closes_normally(self):
        self.policy.identity = identity(124)
        session = self.session()
        session.tick()
        self.assertEqual(self.policy.ticks, 0)
        self.assertIn('another process identity', session.snapshot()['session_last_error']['reason'])
        self.allowed.set()
        self.assertTrue(session.close())
        releases = []
        self.assertFalse(session.finish(lambda: releases.append(True)))
        self.assertTrue(session.done.wait(1.))
        self.assertEqual(releases, [])  # Session thread still owns its lock.

    def test_guardian_start_failure_happens_before_policy_construction(self):
        called = []

        class FailedThread:
            def __init__(self, **kwargs):
                self.daemon = kwargs['daemon']

            def start(self):
                raise RuntimeError('worker unavailable')

        with self.assertRaisesRegex(RuntimeError, 'worker unavailable'):
            self.session(thread_factory=FailedThread, policy_factory=lambda *_: called.append(1))
        self.assertEqual(called, [])


if __name__ == '__main__':
    unittest.main()
