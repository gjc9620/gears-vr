"""Owner/worker lease lifecycle tests; no process handles or game input."""
from pathlib import Path
import tempfile
import threading
import time
import unittest
from unittest import mock
from path_choice_center_monitor import PathChoiceCenterMonitor, LivePathChoiceProbe

GAME=dict(pid=3,creation_time=30,image='game')
OWNER=dict(pid=4,creation_time=40,image='owner')

def status(**changes):
    return dict(pid='3',owner_pid='4',owner_creation_time='40',started_qpc='50',phase='running',
        world_generation='1',path_trial_compiled='1',path_trial_failed='0',path_trial_nested='0',
        path_trial_load_rejected='0',**changes)

class Probe:
    def __init__(self,*args):self.value={'scene_owner':{'proof':'fixture'}};self.calls=0;self.closed=False
    def capture(self):
        self.calls+=1
        if isinstance(self.value,Exception):raise self.value
        return self.value
    def close(self):self.closed=True

class MonitorTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.path=Path(self.temp.name)/'trial.ini';self.now=0.
        self.identities={3:dict(GAME),4:dict(OWNER)}
        self.builder=mock.Mock(return_value='[PathChoiceTrial]\nnonce=100\n')
        self.layout=mock.Mock(return_value=(0,374))
        self.m=PathChoiceCenterMonitor(self.path,GAME,OWNER,50,self.identities.get,
            clock=lambda:self.now,nonce_clock=lambda:100,probe_factory=Probe,builder=self.builder,layout=self.layout)
        self.addCleanup(self.m.close)

    def acquire(self,fields=None):
        fields=fields or status();self.m.poll(fields);self.m.future.result(timeout=2)
        return self.m.poll(fields)

    def test_renew_lease_and_revoke_after_scene_closes(self):
        events=self.acquire();self.assertEqual(events[0]['event'],'path_center_lease_published')
        self.assertTrue(self.path.exists());self.assertEqual(self.builder.call_args.kwargs['duration_ms'],8000)
        self.assertEqual(self.builder.call_args.kwargs['world_generation'],1)
        loaded=self.m.poll(dict(status(),path_trial_nonce='100'))
        self.assertEqual(loaded[0]['event'],'path_center_native_loaded')
        self.m.poll(status());self.assertEqual(self.m.probe.calls,1)
        self.now=3.;self.m.probe.value=None
        events=self.acquire();self.assertEqual(events[0]['reason'],'scene_closed');self.assertFalse(self.path.exists())

    def test_world_change_discards_inflight_read_and_revokes_previous_lease(self):
        self.m.poll(status());self.m.future.result(timeout=2)
        events=self.m.poll(dict(status(),world_generation='2'))
        self.assertEqual(events[0]['reason'],'world_changed_during_read')
        self.assertFalse(self.path.exists());self.builder.assert_not_called()
        self.acquire(dict(status(),world_generation='2'))
        self.assertEqual(self.builder.call_args.kwargs['world_generation'],2)
        self.m.poll(dict(status(),world_generation='3'))
        self.assertFalse(self.path.exists())

    def test_process_identity_change_prevents_publication(self):
        self.m.poll(status());self.m.future.result(timeout=2)
        self.identities[3]=dict(GAME,creation_time=31)
        with self.assertRaisesRegex(RuntimeError,'identity changed'):self.m.poll(status())
        self.assertFalse(self.path.exists());self.builder.assert_not_called()

    def test_native_session_mismatch_revokes_without_publishing_pending_result(self):
        self.acquire();self.m.poll(dict(status(),started_qpc='51'))
        self.assertFalse(self.path.exists())
        self.assertEqual(self.m.poll(dict(status(),started_qpc='51')),[])

    def test_layout_and_capture_failure_do_not_publish_or_destroy_foreign_sidecar(self):
        self.path.write_text('another owner')
        self.layout.side_effect=ValueError('inconsistent layout')
        events=self.acquire();self.assertEqual(events[0]['event'],'path_center_acquisition_rejected')
        self.assertEqual(self.path.read_text(),'another owner')
        self.layout.side_effect=None;self.m.probe.value=RuntimeError('changed');self.now=1.
        self.m.poll(status())
        with self.assertRaises(RuntimeError):self.m.future.result(timeout=2)
        self.m.poll(status());self.builder.assert_not_called()

    def test_loaded_failure_stops_feature_and_revokes_lease(self):
        self.acquire();self.m.poll(dict(status(),path_trial_failed='1'))
        self.assertFalse(self.path.exists());self.assertTrue(self.m.disabled)
        self.assertEqual(self.m.poll(status()),[])

    def test_unsupported_native_never_acquires(self):
        events=self.m.poll(dict(status(),path_trial_compiled='0'))
        self.assertEqual(events[0]['reason'],'native_support_missing');self.assertEqual(self.m.probe.calls,0)

    def test_close_preserves_edited_sidecar_and_closes_worker_reader(self):
        self.acquire();self.path.write_text('edited after publication')
        self.m.close();self.assertTrue(self.m.probe.closed)
        self.assertEqual(self.path.read_text(),'edited after publication')
        self.assertEqual(self.m.poll(status()),[]);self.m.close()

    def test_pending_worker_cannot_publish_after_close(self):
        self.m.poll(status());self.m.future.result(timeout=2);self.m.close()
        self.assertFalse(self.path.exists());self.builder.assert_not_called()

    def test_old_read_cannot_create_new_lease_and_previous_lease_is_revoked(self):
        self.acquire();self.now=3.
        self.m.poll(status());self.m.future.result(timeout=2)
        self.now=5.01
        events=self.m.poll(status())
        self.assertEqual(events[-1]['reason'],'StaleLayoutRead')
        self.assertFalse(self.path.exists());self.assertEqual(self.builder.call_count,1)
        self.now=6.02;self.acquire()
        self.assertTrue(self.path.exists());self.assertEqual(self.builder.call_count,2)

    def test_validation_time_is_included_in_read_age(self):
        self.builder.side_effect=lambda *args,**kwargs: setattr(self,'now',2.01) or 'late'
        events=self.acquire()
        self.assertEqual(events[-1]['reason'],'StaleLayoutRead')
        self.assertFalse(self.path.exists())

    def test_blocked_read_does_not_block_native_owner_shutdown_or_interpreter_exit(self):
        entered,release=threading.Event(),threading.Event()
        def blocked():entered.set();release.wait(2);return {'scene_owner':{'late':'result'}}
        self.m.probe.capture=blocked
        self.m.poll(status());self.assertTrue(entered.wait(1))
        worker=self.m.executor
        try:
            before=time.monotonic();self.m.close();elapsed=time.monotonic()-before
            self.assertLess(elapsed,1)
            self.assertIs(self.m.worker_joined,False);self.assertTrue(worker.thread.daemon)
            self.assertFalse(self.path.exists());self.builder.assert_not_called()
        finally:
            release.set();worker.thread.join(1)
        self.assertFalse(worker.thread.is_alive());self.assertTrue(self.m.probe.closed)

if __name__=='__main__':unittest.main()
