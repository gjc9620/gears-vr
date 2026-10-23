import threading
import time
import unittest
from unittest import mock
import json
import struct
from dataclasses import replace

from world_rebind_monitor import PreparedRebind, WorldRebindMonitor, WorldSnapshot


def obj(address, name='Object', number=0):
    return dict(address=address, cls=900, outer=0, number=number, internal_index=address, name=name)


def snapshot(change=0, pawn=60):
    return WorldSnapshot(obj(10), obj(20, number=change), obj(30), obj(40), obj(50), obj(pawn))


class Harness:
    def __init__(self):
        self.now = 0.0
        self.game = dict(pid=1, creation_time=111, image='game.exe')
        self.owner = dict(pid=2, creation_time=222, image='owner.exe')
        self.current = snapshot(1)
        self.builds, self.signals = [], []
        self.status = dict(pid='1', owner_pid='2', owner_creation_time='222', started_qpc='333',
                           phase='running', complete='1', continuous_session='1',
                           world_handoff_supported='1', world_generation='1',
                           world_prepare_requests='0', world_prepare_rejected='0',
                           world_candidate_generation='0')

    def identity(self, pid):
        return self.game if pid == 1 else self.owner

    def observe(self, check):
        check()
        return self.current

    def prepare(self, target, generation, request, check):
        check()
        self.builds.append((target, generation, request))
        return PreparedRebind(target, generation, request,
                              f'/private/world-rebind-111-{generation + 1}-{request}.ini')

    def monitor(self, **kwargs):
        options = dict(game=self.game, owner=self.owner, started_qpc=333, baseline=snapshot(),
                       observe=self.observe, prepare=self.prepare, identity=self.identity,
                       signal=lambda event: self.signals.append(event) or True,
                       clock=lambda: self.now, enabled=True)
        options.update(kwargs)
        return WorldRebindMonitor(**options)

    def finish_worker(self, monitor):
        monitor.future.result(timeout=2)
        return monitor.poll(self.status)

    def published(self, monitor):
        monitor.poll(self.status)
        self.finish_worker(monitor)

    def prepared(self, monitor):
        self.published(monitor)
        self.status.update(world_prepare_requests='1', world_candidate_generation='2')
        monitor.poll(self.status)


class MonitorTests(unittest.TestCase):
    def test_hinted_transaction_uses_one_acquisition_and_short_bounded_polling(self):
        h=Harness();h.current=snapshot();observe=mock.Mock(side_effect=h.observe)
        calls=[]
        def changed(generation,request,accept,check):
            check();calls.append(h.current)
            return h.current,h.prepare(h.current,generation,request,check) if accept(h.current) else None
        with h.monitor(probe_interval=2,changed_hint=lambda *_:h.current!=snapshot(),observe=observe,
                       prepare_changed=changed) as m:
            self.assertEqual(m.poll_interval(.1),.1)
            m.poll(h.status);h.finish_worker(m)
            h.current=snapshot(9);h.now=.2;m.poll(h.status)
            self.assertEqual(m.poll_interval(.1),.01)
            h.finish_worker(m)
            self.assertEqual(observe.call_count,1)
            self.assertEqual(calls,[h.current]);self.assertEqual(len(h.builds),1)
            self.assertTrue(m.work_timing['single_acquisition'])
            self.assertEqual(m.state,'preparing');self.assertEqual(m.poll_interval(.25),.01)
            h.status.update(world_prepare_requests='1',world_candidate_generation='2')
            m.poll(h.status);self.assertEqual(m.state,'committing')
            h.status.update(world_generation='2',world_candidate_generation='0')
            m.poll(h.status)
            self.assertEqual(m.state,'idle');self.assertEqual(m.baseline,h.current)
            self.assertEqual(m.poll_interval(.1),.1)
        self.assertEqual(m.poll_interval(.25),.25)

    def test_false_hint_cannot_publish_unchanged_foreign_local_or_exhausted_target(self):
        for case in ('unchanged','foreign_local','pawn_opt_out','exhausted'):
            with self.subTest(case=case):
                h=Harness();h.current=snapshot();accepted=[]
                def changed(generation,request,accept,check):
                    check();ok=accept(h.current);accepted.append(ok)
                    return h.current,h.prepare(h.current,generation,request,check) if ok else None
                hint=mock.Mock(return_value=False)
                with h.monitor(probe_interval=2,changed_hint=hint,prepare_changed=changed) as m:
                    m.poll(h.status);h.finish_worker(m)
                    hint.return_value=True
                    if case=='foreign_local':h.current=replace(h.current,local_player=obj(11))
                    if case=='pawn_opt_out':h.current=snapshot(pawn=61)
                    if case=='exhausted':
                        h.current=snapshot(9);m.target=h.current;m.attempts=m.max_attempts
                    h.now=.2;m.poll(h.status);h.finish_worker(m)
                    self.assertEqual(accepted,[False]);self.assertEqual(m.state,'idle')
                self.assertFalse(h.builds);self.assertFalse(h.signals)

    def test_unchanged_periodic_observation_keeps_ordinary_polling(self):
        for changed in (False,True):
            h=Harness();h.current=snapshot(9) if changed else snapshot()
            observing,observed,release=threading.Event(),threading.Event(),threading.Event()
            def observe(check):
                observing.set();release.wait(2);check();return h.current
            def prepare(*args):
                observed.set();release.wait(2);return h.prepare(*args)
            with self.subTest(changed=changed),h.monitor(observe=observe,prepare=prepare) as m:
                m.poll(h.status);self.assertTrue(observing.wait(2))
                self.assertEqual(m.poll_interval(.1),.1)
                release.set()
                if changed:self.assertTrue(observed.wait(2))
                # Before consuming a completed worker, only a full changed
                # observation (or an explicit pointer hint) shortens building.
                m.future.result(timeout=2)
                self.assertEqual(m.poll_interval(.1),.01 if changed else .1)
                h.finish_worker(m)

    def test_short_polling_does_not_busy_wait_during_backoff_or_timeout(self):
        h=Harness()
        with h.monitor() as m:
            for value in (0,-.1,float('nan'),float('inf')):
                with self.assertRaises(ValueError):m.poll_interval(value)
            m.state='send_prepare';m.next_poll=2
            self.assertEqual(m.poll_interval(.25),.25)
            h.now=2;self.assertEqual(m.poll_interval(.25),.01)
            m.state='preparing';m.deadline=3
            h.now=3;self.assertEqual(m.poll_interval(.25),.25)
            m.deadline=10;m.timed_out=True
            self.assertEqual(m.poll_interval(.25),.25)
        with h.monitor(enabled=False) as m:
            m.state='building';m.deadline=10
            self.assertEqual(m.poll_interval(.25),.25);self.assertIsNone(m.executor)

    def test_idle_hint_deadline_avoids_extra_certificate_work(self):
        h=Harness();h.current=snapshot();hint=mock.Mock(return_value=False);observe=mock.Mock(wraps=h.observe)
        with h.monitor(changed_hint=hint,hint_interval=.05,probe_interval=2,observe=observe) as m:
            self.assertEqual(.25,m.poll_interval(.25)) # No accepted native status yet.
            h.published(m);self.assertTrue(m.hint_ready)
            self.assertAlmostEqual(.05,m.poll_interval(.1));self.assertAlmostEqual(.05,m.poll_interval(.25))
            self.assertEqual(.02,m.poll_interval(.02))
            for index in range(10):
                h.now=m.next_hint;m.poll(h.status)
                self.assertEqual(index+2,hint.call_count);self.assertIsNone(m.future)
            observe.assert_called_once();self.assertFalse(h.builds);self.assertFalse(h.signals)

    def test_hint_deadline_suspends_when_native_status_or_candidate_is_unavailable(self):
        h=Harness();h.current=snapshot();hint=mock.Mock(return_value=False)
        with h.monitor(changed_hint=hint,hint_interval=.05,probe_interval=2) as m:
            h.published(m)
            for status in (None,dict(h.status,pid='999'),dict(h.status,stop_recorded='1'),dict(h.status,world_candidate_generation='2')):
                m.poll(status);self.assertFalse(m.hint_ready)
                self.assertEqual(.25,m.poll_interval(.25));self.assertIsNone(m.future);self.assertFalse(h.signals)
                m.poll(h.status);self.assertTrue(m.hint_ready)

    def test_hint_deadline_honors_backoff_full_probe_and_minimum_sleep(self):
        h=Harness();h.current=snapshot();hint=mock.Mock(return_value=False)
        with h.monitor(changed_hint=hint,hint_interval=.05,probe_interval=2) as m:
            h.published(m);m.hint_retry_after=2
            self.assertEqual(.25,m.poll_interval(.25))
            h.now=1.98;m.poll(h.status);self.assertEqual(1,hint.call_count)
            self.assertAlmostEqual(.02,m.poll_interval(.25))
            h.now=1.999;self.assertEqual(.01,m.poll_interval(.25))
            h.now=2;m.poll(h.status);h.finish_worker(m);self.assertEqual(2,hint.call_count)
            m.hint_retry_after=0;m.next_hint=h.now+1;m.next_poll=h.now+.03
            self.assertAlmostEqual(.03,m.poll_interval(.25))
            m.next_hint=h.now-1;self.assertEqual(.01,m.poll_interval(.25))
        self.assertEqual(.25,m.poll_interval(.25))

    def test_hint_deadline_does_not_accelerate_disabled_or_missing_hint_adapters(self):
        h=Harness();h.current=snapshot()
        with h.monitor() as m:
            h.published(m);self.assertEqual(.25,m.poll_interval(.25))
        with h.monitor(enabled=False,changed_hint=lambda *_:True,hint_interval=.05) as m:
            m.poll(h.status);self.assertEqual(.25,m.poll_interval(.25));self.assertIsNone(m.executor)

    def test_nonfinite_coordination_deadlines_are_rejected(self):
        h=Harness()
        for field in ('probe_interval','build_timeout','prepare_timeout','commit_timeout','backoff'):
            for invalid in (float('nan'),float('inf'),float('-inf')):
                with self.assertRaises(ValueError):h.monitor(**{field:invalid})

    def test_wait_wakes_for_running_worker_before_any_native_signal(self):
        h=Harness();entered=threading.Event();release=threading.Event();waiting=threading.Event()
        def observe(check):
            check();entered.set()
            if not release.wait(2):raise RuntimeError('test release timeout')
            check();return h.current
        with h.monitor(observe=observe) as m:
            helper=None
            try:
                m.poll(h.status);self.assertTrue(entered.wait(2));complete=m.work_complete;original=complete.wait
                def wait(timeout):waiting.set();return original(timeout)
                def finish():
                    if waiting.wait(2):release.set()
                helper=threading.Thread(target=finish);helper.start()
                with mock.patch.object(complete,'wait',side_effect=wait):self.assertTrue(m.wait(1))
                self.assertTrue(m.future.done());self.assertFalse(h.signals)
                self.assertFalse(complete.is_set());m.poll(h.status)
                self.assertEqual('preparing',m.state);self.assertEqual(1,len(h.signals))
            finally:
                release.set()
                if helper is not None:helper.join(2)

    def test_completed_notification_is_taken_once_during_status_gap(self):
        h=Harness()
        with h.monitor() as m:
            m.poll(h.status);m.future.result(timeout=2);complete=m.work_complete
            self.assertTrue(complete.wait(2));m.poll(None)
            self.assertTrue(m.wait(.25));self.assertFalse(complete.is_set());self.assertFalse(h.signals)
            self.assertFalse(m.wait(.01));self.assertFalse(h.signals)
            m.poll(h.status);self.assertEqual('preparing',m.state);self.assertIsNone(m.work_complete)
            self.assertEqual(1,len(h.signals))

    def test_late_callback_from_consumed_worker_cannot_wake_the_next_worker(self):
        from concurrent.futures import Future
        h=Harness();h.current=snapshot();first=Future();second=Future();callbacks=[]
        first.add_done_callback=callbacks.append
        with h.monitor() as m:
            m.executor=mock.Mock();m.executor.submit.side_effect=[first,second]
            m.poll(h.status);old_complete=m.work_complete
            first.set_result((snapshot(),None,None));m.poll(h.status)
            self.assertEqual('idle',m.state);self.assertIsNone(m.work_complete)
            h.now=m.next_poll;m.poll(h.status);current=m.work_complete
            self.assertIsNot(current,old_complete);callbacks[0](first)
            self.assertTrue(old_complete.is_set());self.assertFalse(current.is_set());self.assertFalse(m.wait(.01))
            second.set_result((snapshot(),None,None));self.assertTrue(m.wait(.25))
            m.poll(h.status);self.assertFalse(h.signals)

    def test_worker_wait_preserves_backoff_and_disabled_adapter(self):
        h=Harness()
        with h.monitor(prepare=mock.Mock(side_effect=ValueError('invalid certificate'))) as m:
            m.poll(h.status);m.future.result(timeout=2);complete=m.work_complete
            m.poll(h.status);self.assertEqual('idle',m.state);complete.set()
            with mock.patch('world_rebind_monitor.time.sleep') as sleep:
                self.assertFalse(m.wait(.25));sleep.assert_called_once_with(.25)
            self.assertFalse(h.signals)
        with h.monitor(enabled=False) as m:
            with mock.patch('world_rebind_monitor.time.sleep') as sleep:
                self.assertFalse(m.wait(.1));sleep.assert_called_once_with(.1)
            self.assertIsNone(m.executor);self.assertIsNone(m.work_complete)
            for invalid in (0,-.1,float('nan'),float('inf')):
                with self.assertRaises(ValueError):m.wait(invalid)

    def test_close_ignores_late_worker_completion_without_control_signals(self):
        h=Harness();entered=threading.Event()
        def observe(check):
            entered.set()
            if not m.cancel.wait(2):raise RuntimeError('test cancellation timeout')
            check();return h.current
        with h.monitor(observe=observe) as m:
            m.poll(h.status);self.assertTrue(entered.wait(2));complete=m.work_complete
            m.close();self.assertTrue(complete.wait(1));self.assertIsNone(m.work_complete)
            self.assertEqual('closed',m.state);self.assertEqual([],m.poll(h.status))
            with mock.patch('world_rebind_monitor.time.sleep') as sleep:
                self.assertFalse(m.wait(.1));sleep.assert_called_once_with(.1)
            self.assertFalse(h.builds);self.assertFalse(h.signals)

    def test_pointer_hint_skips_two_second_wait_but_requires_full_observation(self):
        h=Harness();h.current=snapshot();hint=mock.Mock(return_value=False)
        with h.monitor(probe_interval=2,include_pawn_changes=True,changed_hint=hint) as m:
            m.poll(h.status);h.finish_worker(m)
            self.assertEqual(m.next_poll,2)
            h.now=.1;m.poll(h.status);self.assertIsNone(m.future)
            hint.return_value=True;h.current=snapshot(9);h.now=.2
            m.poll(h.status);self.assertEqual(m.state,'building')
            h.finish_worker(m);self.assertEqual(len(h.builds),1)
            self.assertEqual(m.state,'preparing')
        # A false positive has no authority to publish or signal.
        h=Harness();h.current=snapshot()
        with h.monitor(probe_interval=2,changed_hint=lambda *_:True) as m:
            m.poll(h.status);h.finish_worker(m);h.now=.1
            m.poll(h.status);h.finish_worker(m)
        self.assertFalse(h.builds);self.assertFalse(h.signals)

    def test_hint_failure_and_backoff_cannot_bypass_rejection_or_session_identity(self):
        h=Harness();h.current=snapshot();hint=mock.Mock(side_effect=OSError('unreadable'))
        with h.monitor(probe_interval=2,changed_hint=hint) as m:
            m.poll(h.status);h.finish_worker(m);h.now=.1
            self.assertEqual(m.poll(h.status),[]);self.assertIsNone(m.future)
            hint.reset_mock();h.status['owner_creation_time']='999';h.now=.3;m.poll(h.status)
            hint.assert_not_called()
        h=Harness();hint=mock.Mock(return_value=True)
        with h.monitor(changed_hint=hint,prepare=lambda *_:(_ for _ in ()).throw(ValueError('reject'))) as m:
            m.poll(h.status);h.finish_worker(m);hint.reset_mock();h.now=.1;m.poll(h.status)
            hint.assert_not_called();self.assertIsNone(m.future)

    def test_due_periodic_probe_prefers_single_acquisition_for_changed_pointer(self):
        h=Harness();h.current=snapshot(pawn=61);observe=mock.Mock(side_effect=AssertionError('Redundant observation'))
        def changed(generation,request,accept,check):
            check();self.assertTrue(accept(h.current))
            return h.current,h.prepare(h.current,generation,request,check)
        with h.monitor(observe=observe,changed_hint=lambda *_:True,prepare_changed=changed,include_pawn_changes=True)as m:
            m.poll(h.status);h.finish_worker(m)
            self.assertTrue(m.work_timing['single_acquisition']);observe.assert_not_called()
            self.assertEqual(m.state,'preparing');self.assertEqual(len(h.builds),1)

    def test_false_or_unreadable_hint_preserves_due_full_identity_probe(self):
        for failure in (None,OSError('Unavailable')):
            h=Harness();h.current=snapshot(9);observe=mock.Mock(side_effect=h.observe)
            hint=mock.Mock(return_value=False,side_effect=failure)
            with self.subTest(failure=failure),h.monitor(observe=observe,changed_hint=hint)as m:
                m.poll(h.status);h.finish_worker(m)
                self.assertFalse(m.work_timing['single_acquisition']);observe.assert_called_once()
                self.assertEqual(m.state,'preparing');self.assertEqual(len(h.builds),1)

    def test_due_hint_after_failed_worker_uses_a_new_cancellation_scope(self):
        h=Harness();calls=[]
        def hint(baseline,check):check();return True
        def changed(generation,request,accept,check):
            check();calls.append(request)
            if len(calls)==1:raise ValueError('Temporary read failure')
            return h.current,h.prepare(h.current,generation,request,check)
        with h.monitor(changed_hint=hint,prepare_changed=changed)as m:
            m.poll(h.status);h.finish_worker(m)
            self.assertEqual(m.state,'idle');old_cancel=m.cancel;self.assertTrue(old_cancel.is_set())
            h.now=m.next_poll;m.poll(h.status)
            self.assertEqual(m.state,'building');h.finish_worker(m)
            self.assertEqual(m.state,'preparing');self.assertEqual(calls,[1,1])
            self.assertTrue(old_cancel.is_set());self.assertIsNot(m.cancel,old_cancel)

    def test_idle_hint_does_not_uncancel_an_old_inflight_worker(self):
        from concurrent.futures import Future
        h=Harness();calls=[]
        def hint(baseline,check):check();calls.append(True);return True
        with h.monitor(changed_hint=hint)as m:
            old=Future();m.future=old;m.cancel.set();old_cancel=m.cancel
            m.poll(h.status)
            self.assertEqual(calls,[True]);self.assertIs(m.future,old)
            self.assertIs(m.cancel,old_cancel);self.assertTrue(old_cancel.is_set())
            self.assertFalse(h.builds);self.assertFalse(h.signals)
            old.set_result((None,None,None))

    def test_opt_in_and_capability_do_not_even_observe(self):
        h = Harness()
        for enabled, capability in ((False, '1'), (True, '0'), (True, None)):
            with h.monitor(enabled=enabled) as m:
                h.status['world_handoff_supported'] = capability
                self.assertEqual([], m.poll(h.status))
                self.assertIsNone(m.executor)
        self.assertFalse(h.builds)

    def test_complete_two_phase_flow_updates_baseline_only_after_commit(self):
        h = Harness()
        with h.monitor() as m:
            h.prepared(m)
            self.assertEqual('committing', m.state)
            self.assertEqual(snapshot(), m.baseline)
            self.assertEqual('committing', m.state)
            self.assertEqual(['Local\\GearsVR.Capture.WorldRebindPrepare.1',
                              'Local\\GearsVR.Capture.WorldRebindCommit.1'], h.signals)
            h.status.update(world_generation='2', world_candidate_generation='0')
            self.assertEqual('world_rebind_committed', m.poll(h.status)[0]['event'])
            self.assertEqual(h.current, m.baseline)
            h.now += 2
            m.poll(h.status)
            h.finish_worker(m)
            self.assertEqual(1, len(h.builds))

    def test_rejection_counts_and_null_pawn_do_not_trigger(self):
        h = Harness()
        h.current = snapshot()
        h.status['anchor_submission_rejected'] = '9999999'
        with h.monitor() as m:
            m.poll(h.status)
            h.finish_worker(m)
            self.assertFalse(h.builds)
            h.current = None  # No valid full world while Pawn is null.
            h.now += 2
            m.poll(h.status)
            h.finish_worker(m)
            self.assertFalse(h.builds)
        with self.assertRaises(ValueError):
            snapshot(1, pawn=0)

    def test_address_reuse_is_detected_but_pawn_only_change_is_not(self):
        self.assertTrue(snapshot(1).changed_from(snapshot()))
        self.assertFalse(snapshot(pawn=61).changed_from(snapshot()))
        h = Harness()
        data = dict(zip(('local_player', 'controller', 'camera', 'level', 'world_info', 'pawn'),
                        (obj(11), obj(20), obj(30), obj(40), obj(50), obj(60))))
        h.current = WorldSnapshot(**data)
        with h.monitor() as m:
            m.poll(h.status)
            h.finish_worker(m)
            self.assertFalse(h.builds)

    def test_pawn_only_opt_in_commits_then_updates_baseline_without_repeating(self):
        for pawn in (obj(61),obj(60,number=1),dict(obj(60),cls=901),dict(obj(60),outer=88),
                     dict(obj(60),internal_index=62),dict(obj(60),name='NewPawn')):
            with self.subTest(pawn=pawn):
                h=Harness();h.current=replace(snapshot(),pawn=pawn)
                with h.monitor(include_pawn_changes=True) as m:
                    h.prepared(m)
                    self.assertEqual('committing',m.state);self.assertEqual(snapshot(),m.baseline)
                    self.assertEqual(1,m.attempts) # Worker completion uses the same Pawn-aware predicate.
                    self.assertEqual(1,len(h.builds));self.assertEqual(2,len(h.signals))
                    h.status.update(world_generation='2',world_candidate_generation='0')
                    self.assertEqual('world_rebind_committed',m.poll(h.status)[0]['event'])
                    self.assertEqual(h.current,m.baseline)
                    h.now+=2;m.poll(h.status);h.finish_worker(m)
                    self.assertEqual(1,len(h.builds));self.assertEqual(2,len(h.signals))

    def test_pawn_change_default_is_ignored_and_null_transition_never_prepares(self):
        h=Harness();h.current=snapshot(pawn=61)
        with h.monitor() as m:
            m.poll(h.status);h.finish_worker(m)
            self.assertFalse(h.builds);self.assertFalse(h.signals)
        h=Harness();h.current=None
        with h.monitor(include_pawn_changes=True) as m:
            for current in (None,snapshot(),None):
                h.current=current;h.now+=2;m.poll(h.status);h.finish_worker(m)
                self.assertFalse(h.builds);self.assertFalse(h.signals)
            h.current=snapshot(pawn=61);h.now+=2;h.prepared(m)
            self.assertEqual(1,len(h.builds));self.assertEqual('committing',m.state)
        with self.assertRaises(ValueError):snapshot(pawn=0)

    def test_pawn_opt_in_keeps_full_local_player_identity_constraint(self):
        h=Harness();h.current=replace(snapshot(pawn=61),local_player=obj(10,number=1))
        with h.monitor(include_pawn_changes=True) as m:
            m.poll(h.status);h.finish_worker(m)
            self.assertFalse(h.builds);self.assertFalse(h.signals)
        for invalid in (1,None,'true'):
            with self.assertRaises(ValueError):h.monitor(include_pawn_changes=invalid)

    def test_quiesced_recovery_uses_one_acquisition_even_without_a_pointer_change(self):
        h=Harness();h.current=snapshot();h.status['world_handoff_phase']='4';calls=[]
        observe=mock.Mock(side_effect=AssertionError('Redundant full observation'))
        prepare=mock.Mock(side_effect=AssertionError('Redundant separate preparation'))
        def changed(generation,request,accept,check):
            check();calls.append((generation,request));self.assertTrue(accept(h.current))
            return h.current,h.prepare(h.current,generation,request,check)
        with h.monitor(observe=observe,prepare=prepare,changed_hint=lambda *_:False,prepare_changed=changed) as m:
            h.published(m)
            self.assertEqual([(1,1)],calls);self.assertTrue(m.work_timing['single_acquisition'])
            self.assertEqual('preparing',m.state);observe.assert_not_called();prepare.assert_not_called()
            self.assertEqual(1,len(h.signals));self.assertIn('Prepare',h.signals[0])
            h.status.update(world_prepare_requests='1',world_candidate_generation='2')
            m.poll(h.status);self.assertEqual('committing',m.state)
            h.status.update(world_generation='2',world_candidate_generation='0',world_handoff_phase='3')
            self.assertEqual('world_rebind_committed',m.poll(h.status)[0]['event'])
            self.assertEqual(snapshot(),m.baseline)

    def test_single_quiesced_acquisition_keeps_local_identity_and_pending_guards(self):
        h=Harness();h.status['world_handoff_phase']='4'
        foreign=replace(h.current,local_player=obj(11));calls=[]
        def changed(generation,request,accept,check):
            check();calls.append(foreign);self.assertFalse(accept(foreign));return foreign,None
        with h.monitor(prepare_changed=changed,observe=mock.Mock(side_effect=AssertionError('extra read'))) as m:
            h.published(m);self.assertEqual([foreign],calls)
            self.assertFalse(h.builds);self.assertFalse(h.signals);self.assertEqual(snapshot(),m.baseline)
        for field,value in (('world_candidate_generation','2'),('owner_creation_time','999'),('stop_recorded','1')):
            h=Harness();h.status.update(world_handoff_phase='4',**{field:value});callback=mock.Mock()
            with h.monitor(prepare_changed=callback) as m:
                m.poll(h.status);self.assertIsNone(m.future);callback.assert_not_called();self.assertFalse(h.signals)

    def test_quiesced_legacy_adapter_keeps_full_observe_then_prepare(self):
        h=Harness();h.current=snapshot();h.status['world_handoff_phase']='4'
        observe=mock.Mock(wraps=h.observe);prepare=mock.Mock(wraps=h.prepare)
        with h.monitor(observe=observe,prepare=prepare) as m:
            h.published(m);observe.assert_called_once();prepare.assert_called_once()
            self.assertFalse(m.work_timing['single_acquisition']);self.assertEqual('preparing',m.state)

    def test_quiesced_timestamp_retry_retains_one_acquisition_and_backoff(self):
        from world_rebind_monitor import RetryablePreparation
        h=Harness();h.current=snapshot();h.status['world_handoff_phase']='4';calls=[]
        def changed(generation,request,accept,check):
            check();calls.append(request)
            if len(calls)<=3:raise RetryablePreparation(ValueError('read-only fixture timestamp race'))
            self.assertTrue(accept(h.current));return h.current,h.prepare(h.current,generation,request,check)
        with h.monitor(prepare_changed=changed,observe=mock.Mock(side_effect=AssertionError('extra read'))) as m:
            for delay in (.05,.1,.15):
                m.poll(h.status);event=h.finish_worker(m)[0]
                self.assertEqual('timestamp_sample',event['retry_scope']);self.assertEqual(0,m.attempts)
                self.assertTrue(m.work_timing['single_acquisition']);self.assertAlmostEqual(delay,m.next_poll-h.now)
                self.assertFalse(h.signals);h.now=m.next_poll
            h.published(m);self.assertEqual([1,1,1,1],calls)
            self.assertEqual(1,len(h.builds));self.assertEqual(1,len(h.signals))

    def test_native_stale_rejection_allows_same_baseline_recovery(self):
        h = Harness()
        with h.monitor() as m:
            h.prepared(m)
            # Native strict post-quiescence check rejects the candidate, leaves
            # old generation quiescing and clears pending. The player can return
            # to the startup identities and still need an explicit new commit.
            h.status.update(world_candidate_generation='0', world_handoff_phase='4')
            events = m.poll(h.status)
            self.assertEqual('commit_rejected_or_request_race', events[0]['reason'])
            h.current = snapshot()
            h.now += 10
            m.poll(h.status)
            h.finish_worker(m)
            self.assertEqual(2, len(h.builds))
            self.assertEqual(2, h.builds[-1][2])

    def test_exact_native_session_and_owner_guards(self):
        for key, value in (('pid', '7'), ('owner_creation_time', '223'),
                           ('started_qpc', '334'), ('complete', '0'),
                           ('phase', 'stopping'), ('stop_recorded', '1'),
                           ('publisher_stopped', '1')):
            h = Harness()
            with h.monitor() as m:
                h.published(m)
                h.status.update(world_prepare_requests='1', world_candidate_generation='2')
                h.status[key] = value
                m.poll(h.status)
                self.assertEqual(1, len(h.signals), key)

    def test_quiesced_recovery_survives_repeated_same_identity_rejections(self):
        h=Harness()
        with h.monitor(max_attempts=2) as m:
            h.prepared(m)
            for request in range(1,7):
                h.status.update(world_candidate_generation='0',world_handoff_phase='4')
                self.assertEqual('world_rebind_failed',m.poll(h.status)[0]['event'])
                self.assertLessEqual(m.next_poll-h.now,4+1e-9)
                for _ in range(3):
                    self.assertEqual([],m.poll(h.status))
                    self.assertIsNone(m.future)
                h.now=m.next_poll
                m.poll(h.status);h.finish_worker(m)
                self.assertEqual('preparing',m.state)
                self.assertEqual(request+1,h.builds[-1][2])
                h.status.update(world_prepare_requests=str(request+1),world_candidate_generation='2')
                m.poll(h.status)
                self.assertEqual('committing',m.state)
            h.status.update(world_generation='2',world_candidate_generation='0',world_handoff_phase='3')
            self.assertEqual('world_rebind_committed',m.poll(h.status)[0]['event'])
            self.assertEqual(h.current,m.baseline)
            self.assertEqual(len(h.builds),7)

    def test_quiesced_recovery_keeps_retrying_transient_worker_failures(self):
        h=Harness();calls=[];h.status['world_handoff_phase']='4'
        def prepare(*args):
            calls.append(args[0])
            if len(calls)<=4:raise RuntimeError('camera is changing')
            return h.prepare(*args)
        with h.monitor(prepare=prepare,max_attempts=2) as m:
            for _ in range(5):
                m.poll(h.status);h.finish_worker(m)
                if m.state=='idle':h.now=m.next_poll
            self.assertEqual(5,len(calls));self.assertEqual('preparing',m.state)
            self.assertEqual(1,len(h.builds));self.assertEqual(1,len(h.signals))

    def test_process_reuse_prevents_publication_signal(self):
        h = Harness()
        with h.monitor() as m:
            m.poll(h.status)
            m.future.result(timeout=2)
            h.game = dict(h.game, creation_time=999)
            m.poll(h.status)
            self.assertFalse(h.signals)

    def test_slow_worker_does_not_block_poll_and_cancellation_is_cooperative(self):
        h = Harness()
        entered, release = threading.Event(), threading.Event()
        def slow(target, generation, request, check):
            entered.set()
            release.wait(2)
            check()
            return h.prepare(target, generation, request, check)
        with h.monitor(prepare=slow) as m:
            m.poll(h.status)
            self.assertTrue(entered.wait(2))
            started = time.monotonic()
            for _ in range(100):
                m.poll(h.status)
            self.assertLess(time.monotonic() - started, .25)
            h.now = 16
            m.poll(h.status)
            # Still one outstanding job even after retry backoff.
            h.now = 100
            m.poll(h.status)
            self.assertTrue(m.cancel.is_set())
            release.set()
        self.assertFalse(h.signals)
        self.assertEqual('closed', m.state)

    def test_bounded_build_failure_retries_then_new_identity_can_retry(self):
        h = Harness()
        calls = []
        def fail(*args):
            calls.append(args[0])
            raise ValueError('certificate rejected')
        with h.monitor(prepare=fail, max_attempts=2) as m:
            for _ in range(5):
                h.now += 10
                m.poll(h.status)
                h.finish_worker(m)
            self.assertEqual(2, len(calls))
            h.current = snapshot(2)
            h.now += 10
            m.poll(h.status)
            h.finish_worker(m)
            self.assertEqual(3, len(calls))
        self.assertFalse(h.signals)

    def test_anchor_failure_preserves_class_and_specific_reason(self):
        # The coordinator handles callback exceptions without importing the
        # game-memory sampling stack.
        class AnchorSamplingRejected(RuntimeError):
            pass
        h = Harness()
        def fail(*args):
            raise AnchorSamplingRejected('Camera view target is not controlled Pawn')
        with h.monitor(prepare=fail) as m:
            m.poll(h.status)
            events = h.finish_worker(m)
            self.assertEqual([dict(event='world_rebind_failed', reason='AnchorSamplingRejected',
                                   detail='Camera view target is not controlled Pawn', request=1)], events)
            self.assertEqual('idle', m.state)
            self.assertEqual(1, m.attempts)
            self.assertEqual(2, m.next_poll)
            self.assertTrue(m.cancel.is_set())
            self.assertFalse(h.signals)

    def test_worker_failure_detail_is_bounded_and_single_line(self):
        h = Harness()
        def fail(*args):
            raise ValueError('camera\r\nchanged\0\t' + 'x' * 1000)
        with h.monitor(observe=fail) as m:
            m.poll(h.status)
            event = h.finish_worker(m)[0]
            self.assertEqual('ValueError', event['reason'])
            self.assertTrue(event['detail'].startswith('camera changed '))
            self.assertLessEqual(len(event['detail']), 512)
            self.assertTrue(event['detail'].isprintable())
            self.assertFalse(h.builds)
            self.assertFalse(h.signals)

    def test_sampling_deadline_keeps_last_inner_rejection_without_raw_graph(self):
        h = Harness()
        error = RuntimeError('Anchor guard acquisition deadline exceeded')
        error.evidence = dict(guard_acquisitions=[dict(stage='initial', attempts=[
            dict(reason='Mounted camera timestamp changed at bracket boundary',
                 strict_guard_accepted=False, raw_guard={'secret': 'DO_NOT_LOG_RAW_GRAPH'})])])
        def fail(*args):raise error
        with h.monitor(observe=fail) as m:
            m.poll(h.status)
            event = h.finish_worker(m)[0]
            self.assertIn('last=Mounted camera timestamp changed at bracket boundary', event['detail'])
            self.assertIn('guard=initial', event['detail'])
            self.assertNotIn('DO_NOT_LOG_RAW_GRAPH', event['detail'])
            self.assertFalse(h.builds)
            self.assertFalse(h.signals)

    def test_late_success_is_not_reported_as_timestamp_rejection(self):
        from world_rebind_monitor import _error_detail
        error = RuntimeError('Anchor guard acquisition deadline exceeded')
        guard = dict(stage='initial', accepted=False,
                     failure_reason='Anchor guard acquisition deadline exceeded',
                     attempts=[dict(reason=None, strict_guard_accepted=True)])
        error.evidence = dict(guard_acquisitions=[guard])
        self.assertIn('last=strict guard accepted after deadline', _error_detail(error))
        guard['accepted'] = True
        self.assertEqual(str(error), _error_detail(error))

    def test_mounted_failure_event_has_public_stable_and_changing_receipts(self):
        for changing in (False, True):
            h = Harness()
            observation = dict(target_field=dict(address=0x12345678, value=80),
                target_is_driver=True, roles={'controlled':obj(60, 'PRIVATE_OBJECT_NAME')},
                consistent_read=not changing,
                timestamp=dict(consistent=not changing, expected_raw='PRIVATE_BYTES', observed_raw='PRIVATE_BYTES'),
                headers={'objects':dict(consistent=not changing, observed_raw='PRIVATE_BYTES'),
                         'names':dict(consistent=True, observed_raw='PRIVATE_BYTES')},
                mismatches=[dict(stage='timestamp', address=0x12345678)] if changing else [])
            if changing:
                observation['read_error'] = dict(stage='target_identity', address=0x12345678,
                    error='PRIVATE_READ_ERROR', exception_type='PrivateReader')
            error = RuntimeError('Anchor guard acquisition deadline exceeded')
            error.evidence = dict(guard_acquisitions=[dict(stage='sample_before', attempts=[
                dict(reason='Mounted camera target is not controlled turret', retryable=False,
                     strict_guard_accepted=False, elapsed_ms=12.5, camera_observation=observation)])])
            def fail(*args):raise error
            with h.monitor(prepare=fail) as m:
                m.poll(h.status)
                event = h.finish_worker(m)[0]
                summary = event['guard_failure']
                self.assertEqual('mounted_target_mismatch', summary['cause'])
                self.assertEqual('driver', summary['target_relation'])
                self.assertEqual(not changing, summary['consistent_read'])
                self.assertEqual('changed' if changing else 'unchanged', summary['timestamp_state'])
                self.assertEqual('changed' if changing else 'unchanged', summary['objects_registry_state'])
                self.assertEqual('target_identity' if changing else None, summary['read_error_stage'])
                self.assertEqual(['timestamp'] if changing else [], summary['mismatch_stages'])
                self.assertFalse(summary['retryable'])
                self.assertEqual(1, summary['attempts'])
                self.assertEqual('idle', m.state)
                self.assertFalse(h.signals)
                encoded = json.dumps(summary)
                for private in ('PRIVATE', 'address', 'expected_raw', 'observed_raw', '12345678', 'PrivateReader'):
                    self.assertNotIn(private, encoded)

    def test_guard_failure_cause_enums_missing_and_malformed_evidence(self):
        from world_rebind_monitor import _guard_failure
        cases = [({'exception_type':'MountedTimestampChanged'}, {}, 'timestamp_changed'),
                 ({'exception_type':'RegistryHeaderChanged'}, {}, 'registry_changed'),
                 ({'camera_observation':{'read_error':{'stage':'memory'}}}, {}, 'read_failed'),
                 ({'strict_guard_accepted':True}, {'failure_reason':'Anchor guard acquisition deadline exceeded'}, 'deadline'),
                 ({}, {}, 'unknown')]
        for attempt, fields, cause in cases:
            error = RuntimeError('PRIVATE_ERROR')
            error.evidence = dict(guard_acquisitions=[dict(stage='PRIVATE_STAGE', attempts=[attempt], **fields)])
            summary = _guard_failure(error)
            self.assertEqual(cause, summary['cause'])
            self.assertEqual('unknown', summary['stage'])
            self.assertEqual('unavailable', summary['timestamp_state'])
            self.assertIsNone(summary['consistent_read'])
            self.assertNotIn('PRIVATE', json.dumps(summary))
        self.assertIsNone(_guard_failure(RuntimeError('missing')))

    def test_actual_sampler_exceptions_reach_monitor_failure_summary(self):
        from test_mounted_context import MountedFixture
        from test_anchor_config import Fixture
        from anchor_config import LiveRegistryAnchorSampler, AnchorSamplingRejected, AnchorRegistryChanged

        def mounted_failure():
            fixture = MountedFixture()
            fixture.ptr(fixture.sampler.view_target, 0x20000)
            LiveRegistryAnchorSampler(fixture, clock_ns=lambda: 0)

        def registry_failure():
            fixture = Fixture()
            sampler = LiveRegistryAnchorSampler(fixture)
            sampler.guard_stage = 'assembly_after'
            original = fixture.read
            hits = 0
            def changing(address, size):
                nonlocal hits
                if (address, size) == (fixture.objects_header, 16):
                    hits += 1
                    if hits == 2:
                        data, count, capacity = struct.unpack('<Qii', original(address, size))
                        fixture.put(address, struct.pack('<Qii', data, count + 1, capacity + 1))
                return original(address, size)
            fixture.read = changing
            sampler.guard()

        for producer, expected_type, cause in (
                (mounted_failure, AnchorSamplingRejected, 'mounted_target_mismatch'),
                (registry_failure, AnchorRegistryChanged, 'registry_changed')):
            def fail(*args):
                try:producer()
                except expected_type:raise
                self.fail('synthetic sampler must reject')
            h = Harness()
            with h.monitor(prepare=fail) as m:
                m.poll(h.status)
                event = h.finish_worker(m)[0]
                self.assertEqual(expected_type.__name__, event['reason'])
                summary = event['guard_failure']
                self.assertEqual(cause, summary['cause'])
                self.assertEqual(1, summary['attempts'])
                if cause == 'mounted_target_mismatch':
                    self.assertEqual('driver', summary['target_relation'])
                    self.assertTrue(summary['consistent_read'])
                else:
                    self.assertEqual('unavailable', summary['objects_registry_state'])
                    self.assertIsNone(summary['consistent_read'])
                self.assertFalse(h.signals)
                for private in ('address', 'Turret_0', 'WarPawn', 'expected_header', 'observed_header'):
                    self.assertNotIn(private, json.dumps(summary))

    def test_top_level_typed_discovery_failure_uses_no_invented_camera_checks(self):
        # Import fixture first to expose the package-inspector dependency path.
        from test_anchor_config import Fixture
        from anchor_config import AnchorSamplingRejected, AnchorRegistryChanged
        errors = [AnchorSamplingRejected('PRIVATE', dict(stage='initial', exception_type='MountedTimestampChanged',
                    guard_acquisitions=[], attempts=[{'PRIVATE':'not a guard attempt'}])),
                  AnchorRegistryChanged('PRIVATE', dict(stage='initial', exception_type='RegistryHeaderChanged',
                    guard_acquisitions=[], registry_change={'registry':'objects', 'expected_header':[12345678]})),
                  AnchorSamplingRejected('PRIVATE', dict(stage=None, exception_type=None, guard_acquisitions=[]))]
        for error, cause in zip(errors, ('timestamp_changed', 'registry_changed', 'unknown')):
            h = Harness()
            def fail(*args):raise error
            with h.monitor(observe=fail) as m:
                m.poll(h.status)
                summary = h.finish_worker(m)[0]['guard_failure']
                self.assertEqual(cause, summary['cause'])
                self.assertEqual(0, summary['attempts'])
                self.assertIsNone(summary['strict_guard_accepted'])
                self.assertIsNone(summary['retryable'])
                self.assertIsNone(summary['consistent_read'])
                self.assertEqual('unknown', summary['target_relation'])
                self.assertEqual('unavailable', summary['timestamp_state'])
                self.assertEqual('unavailable', summary['objects_registry_state'])
                self.assertNotIn('PRIVATE', json.dumps(summary))
                self.assertNotIn('12345678', json.dumps(summary))

    def test_sampling_detail_handles_malformed_and_large_nested_evidence(self):
        from world_rebind_monitor import _error_detail
        error = RuntimeError('deadline')
        for evidence in (None, [], {'guard_acquisitions': [None]},
                         {'guard_acquisitions': [{'attempts': 'not a list'}]}):
            error.evidence = evidence
            self.assertEqual('deadline', _error_detail(error))
        error.evidence = dict(guard_acquisitions=[dict(stage='x\n' * 100,
            attempts=[dict(reason='changed\r\n\0' * 100, raw_guard={'secret': 'do not include'})])])
        detail = _error_detail(error)
        self.assertLessEqual(len(detail), 512)
        self.assertTrue(detail.isprintable())
        self.assertNotIn('do not include', detail)

    def test_unprintable_exception_message_does_not_hide_failure(self):
        class BrokenMessageError(Exception):
            def __str__(self):
                raise RuntimeError('cannot format')
        h = Harness()
        for error in (BrokenMessageError(), ValueError('')):
            with self.subTest(error_type=type(error).__name__):
                def fail(*args):
                    raise error
                with h.monitor(prepare=fail) as m:
                    m.poll(h.status)
                    self.assertEqual([dict(event='world_rebind_failed', reason=type(error).__name__,
                                           request=1)], h.finish_worker(m))
                    self.assertEqual('idle', m.state)
                    self.assertFalse(h.signals)

    def test_mismatched_path_and_request_receipt_never_commit(self):
        h = Harness()
        def wrong(target, generation, request, check):
            return PreparedRebind(target, generation, request, '/private/world-rebind-2.ini')
        with h.monitor(prepare=wrong) as m:
            m.poll(h.status)
            events = h.finish_worker(m)
            self.assertEqual('ValueError', events[0]['reason'])
            self.assertFalse(h.signals)
        with h.monitor() as m:
            h.published(m)
            h.status['world_candidate_generation'] = '2'
            h.status['world_prepare_requests'] = '2'
            m.poll(h.status)
            self.assertEqual(1, len(h.signals))

    def test_close_never_sends_late_commit(self):
        h = Harness()
        m = h.monitor()
        h.published(m)
        h.status.update(world_prepare_requests='1', world_candidate_generation='2')
        m.close()
        self.assertEqual([], m.poll(h.status))
        self.assertEqual(1, len(h.signals))

    def test_prepare_timeout_keeps_receipt_and_accepts_late_ack_without_resignal(self):
        h = Harness()
        with h.monitor() as m:
            h.published(m)
            h.now = 20
            events = m.poll(h.status)
            self.assertEqual('world_rebind_waiting', events[0]['event'])
            for _ in range(5):
                self.assertEqual([], m.poll(h.status))
            self.assertEqual(1, len(h.signals))
            self.assertIsNotNone(m.prepared)
            h.status.update(world_prepare_requests='1', world_candidate_generation='2')
            m.poll(h.status)
            self.assertEqual('committing', m.state)
            self.assertEqual(2, len(h.signals))

    def test_commit_timeout_and_status_gap_reconcile_late_success(self):
        h = Harness()
        with h.monitor() as m:
            h.prepared(m)
            h.now = 40
            self.assertEqual('world_rebind_waiting', m.poll(h.status)[0]['event'])
            self.assertEqual([], m.poll(None))
            h.status.update(world_generation='2', world_candidate_generation='0')
            self.assertEqual('world_rebind_committed', m.poll(h.status)[0]['event'])
            self.assertEqual(h.current, m.baseline)

    def test_failed_signal_reuses_one_owned_package_with_bounded_backoff(self):
        h = Harness()
        signals = []
        def signal(event):
            signals.append(event)
            return len(signals) == 2
        with h.monitor(signal=signal) as m:
            h.published(m)
            self.assertEqual('send_prepare', m.state)
            m.poll(h.status)
            self.assertEqual(1, len(signals))
            h.now = 3
            m.poll(h.status)
            self.assertEqual('preparing', m.state)
            self.assertEqual(1, len(h.builds))
            self.assertEqual(signals[0], signals[1])

    def test_status_gap_after_publication_keeps_worker_receipt(self):
        h = Harness()
        published, finish = threading.Event(), threading.Event()
        def publish_late(target, generation, request, check):
            result = h.prepare(target, generation, request, check)
            published.set()
            finish.wait(2)
            # Successful publication must return its receipt even after cancel.
            return result
        with h.monitor(prepare=publish_late) as m:
            m.poll(h.status)
            self.assertTrue(published.wait(2))
            m.poll(None)
            self.assertEqual('building', m.state)
            self.assertTrue(m.cancel.is_set())
            finish.set()
            h.finish_worker(m)
            self.assertEqual('preparing', m.state)
            self.assertEqual(1, len(h.builds))
            self.assertEqual(1, len(h.signals))

    def test_signal_outage_beyond_budget_retries_exact_receipt_then_waits_for_ack(self):
        for kind in ('Prepare','Commit'):
            with self.subTest(kind=kind):
                h=Harness();attempts=[]
                def signal(event):
                    if f'WorldRebind{kind}.' not in event:return True
                    attempts.append(event)
                    return len(attempts)>5
                with h.monitor(signal=signal,max_attempts=2) as m:
                    (h.published if kind=='Prepare' else h.prepared)(m)
                    receipt=m.prepared;request=m.request
                    for _ in range(5):
                        self.assertEqual('send_'+kind.lower(),m.state)
                        self.assertIs(receipt,m.prepared);self.assertEqual(request,m.request)
                        self.assertLessEqual(m.next_poll-h.now,4)
                        self.assertAlmostEqual(min(.25,m.next_poll-h.now),m.poll_interval(.25))
                        self.assertEqual([],m.poll(None))
                        for _ in range(3):m.poll(h.status)
                        h.now=m.next_poll;m.poll(h.status)
                    self.assertEqual('preparing' if kind=='Prepare' else 'committing',m.state)
                    self.assertEqual(6,len(attempts));self.assertEqual(1,len(set(attempts)))
                    self.assertEqual(1,len(h.builds));self.assertIs(receipt,m.prepared)
                    # A successful signal is never resent merely because its
                    # exact native receipt is late or briefly unavailable.
                    h.now+=100;m.poll(h.status);m.poll(None);m.poll(h.status)
                    self.assertEqual(6,len(attempts))
                    h.status.update(world_prepare_requests='1',world_candidate_generation='2')
                    m.poll(h.status)
                    h.status.update(world_generation='2',world_candidate_generation='0')
                    self.assertEqual('world_rebind_committed',m.poll(h.status)[0]['event'])
                    self.assertEqual(h.current,m.baseline)

    def test_signal_retry_never_overrides_foreign_request_or_changed_owner(self):
        for change in ('request','owner','stopped'):
            with self.subTest(change=change):
                h=Harness();signal=mock.Mock(return_value=False)
                with h.monitor(signal=signal,max_attempts=1) as m:
                    h.published(m);receipt=m.prepared
                    h.now=m.next_poll
                    if change=='request':h.status.update(world_prepare_requests='9',world_candidate_generation='2')
                    if change=='owner':h.owner=dict(h.owner,creation_time=999)
                    if change=='stopped':h.status['publisher_stopped']='1'
                    m.poll(h.status)
                    self.assertEqual(1,signal.call_count);self.assertIs(receipt,m.prepared)
                    self.assertEqual('send_prepare' if change=='stopped' else 'blocked',m.state)

    def test_set_event_failure_retries_same_receipt(self):
        h=Harness();signal=mock.Mock(side_effect=[OSError('SetEvent failed'),True])
        with h.monitor(signal=signal,max_attempts=1) as m:
            h.published(m);receipt=m.prepared
            self.assertEqual('send_prepare',m.state)
            h.now=m.next_poll;m.poll(h.status)
            self.assertEqual('preparing',m.state);self.assertIs(receipt,m.prepared)
            self.assertEqual(2,signal.call_count);self.assertEqual(1,len(h.builds))

    def test_failed_control_short_retries_honor_deadline_and_keep_one_receipt(self):
        for kind in ('Prepare','Commit'):
            for failure in ('false','set_event_error'):
                with self.subTest(kind=kind,failure=failure):
                    h=Harness();attempts=[]
                    def signal(event):
                        if f'WorldRebind{kind}.' not in event:return True
                        attempts.append(event)
                        if len(attempts)>3:return True
                        if failure=='set_event_error':raise OSError('SetEvent failed')
                        return False
                    with h.monitor(signal=signal)as m:
                        (h.published if kind=='Prepare'else h.prepared)(m)
                        receipt=m.prepared;request=m.request
                        for number,delay in enumerate((.05,.1,.15),1):
                            self.assertEqual('send_'+kind.lower(),m.state)
                            self.assertAlmostEqual(delay,m.next_poll-h.now)
                            self.assertAlmostEqual(delay,m.poll_interval(.25))
                            with mock.patch('world_rebind_monitor.time.sleep')as sleep:
                                self.assertFalse(m.wait(.25));sleep.assert_called_once()
                                self.assertAlmostEqual(delay,sleep.call_args.args[0])
                            h.now=m.next_poll-.001;m.poll(h.status)
                            self.assertEqual(len(attempts),number)
                            h.now=m.next_poll;m.poll(h.status)
                            self.assertIs(m.prepared,receipt);self.assertEqual(m.request,request)
                            self.assertEqual(len(h.builds),1)
                        self.assertEqual('preparing'if kind=='Prepare'else'committing',m.state)
                        self.assertEqual(len(attempts),4);self.assertEqual(len(set(attempts)),1)
                        h.now+=100;m.poll(h.status);m.poll(None);m.poll(h.status)
                        self.assertEqual(len(attempts),4)

    def test_late_prepare_ack_clears_timeout_before_unaccepted_commit_retry(self):
        h=Harness();commits=[]
        def signal(event):
            if 'WorldRebindCommit.'not in event:return True
            commits.append(event);return len(commits)>4
        with h.monitor(signal=signal)as m:
            h.published(m);receipt=m.prepared;h.now=20;m.poll(h.status)
            self.assertTrue(m.timed_out)
            h.status.update(world_prepare_requests='1',world_candidate_generation='2');m.poll(h.status)
            self.assertEqual(m.state,'send_commit');self.assertFalse(m.timed_out)
            self.assertAlmostEqual(m.next_poll-h.now,.05);self.assertAlmostEqual(m.poll_interval(.25),.05)
            for delay in (.1,.15,1.7):
                h.now=m.next_poll;m.poll(h.status)
                self.assertAlmostEqual(m.next_poll-h.now,delay)
            h.now=m.next_poll;m.poll(h.status)
            self.assertEqual(m.state,'committing');self.assertIs(m.prepared,receipt)
            self.assertEqual(len(commits),5);self.assertEqual(len(h.builds),1)
            self.assertAlmostEqual(m.deadline-h.now,m.commit_timeout)

    def test_regular_control_backoff_starts_after_the_bounded_short_burst(self):
        for kind in ('Prepare','Commit'):
            h=Harness();attempts=[]
            def signal(event):
                if f'WorldRebind{kind}.'not in event:return True
                attempts.append(event);return False
            with self.subTest(kind=kind),h.monitor(signal=signal)as m:
                (h.published if kind=='Prepare'else h.prepared)(m);receipt=m.prepared
                for index,delay in enumerate((.05,.1,.15,1.7,4,6,6)):
                    self.assertAlmostEqual(m.next_poll-h.now,delay)
                    self.assertAlmostEqual(m.poll_interval(.25),min(.25,delay))
                    self.assertIs(m.prepared,receipt);self.assertEqual(len(h.builds),1)
                    deadline=m.next_poll;h.now=deadline-.02
                    self.assertAlmostEqual(m.poll_interval(.25),.02);m.poll(h.status)
                    self.assertEqual(len(attempts),index+1)
                    if index<6:h.now=deadline;m.poll(h.status)
                self.assertEqual(len(attempts),7);self.assertEqual(len(set(attempts)),1)

    def test_short_control_retry_retains_session_process_and_transaction_guards(self):
        for kind in ('Prepare','Commit'):
            for fault in ('owner','game','session','stopped','request','candidate','generation','disabled','closed'):
                with self.subTest(kind=kind,fault=fault):
                    h=Harness();attempts=[]
                    def signal(event):
                        if f'WorldRebind{kind}.' not in event:return True
                        attempts.append(event);return False
                    with h.monitor(signal=signal)as m:
                        (h.published if kind=='Prepare'else h.prepared)(m);receipt=m.prepared
                        h.now=m.next_poll
                        if fault=='owner':h.owner=dict(h.owner,creation_time=999)
                        if fault=='game':h.game=dict(h.game,creation_time=999)
                        if fault=='session':h.status['started_qpc']='999'
                        if fault=='stopped':h.status['publisher_stopped']='1'
                        if fault=='request':h.status['world_prepare_requests']='9'
                        if fault=='candidate':h.status['world_candidate_generation']='99'
                        if fault=='generation':h.status['world_generation']='99'
                        if fault=='disabled':m.enabled=False
                        if fault=='closed':m.close()
                        m.poll(h.status)
                        self.assertEqual(len(attempts),1);self.assertIs(m.prepared,receipt)
                        self.assertEqual(m.baseline,snapshot());self.assertEqual(len(h.builds),1)

    def test_control_probes_preserve_ordinary_boundaries_for_other_attempt_limits(self):
        for bound in (1,3,10):
            for backoff in (.01,2.0):
                for ready in (backoff,backoff+backoff*min(2,bound)):
                    h=Harness();attempts=[]
                    def signal(event):attempts.append(h.now);return h.now>=ready-1e-9
                    with self.subTest(bound=bound,backoff=backoff,ready=ready),\
                         h.monitor(signal=signal,max_attempts=bound,backoff=backoff)as m:
                        h.published(m)
                        for _ in range(20):
                            if m.state=='preparing':break
                            h.now=m.next_poll;m.poll(h.status)
                        self.assertEqual(m.state,'preparing');self.assertAlmostEqual(h.now,ready)
                        self.assertEqual(len(h.builds),1);self.assertEqual(attempts[-1],h.now)

    def test_unknown_control_exception_does_not_join_short_retries(self):
        for kind in ('Prepare','Commit'):
            h=Harness();attempts=[]
            def signal(event):
                if f'WorldRebind{kind}.' not in event:return True
                attempts.append(event);raise RuntimeError('unknown control outcome')
            with self.subTest(kind=kind),h.monitor(signal=signal)as m:
                (h.published if kind=='Prepare'else h.prepared)(m)
                self.assertEqual(m.state,'blocked');h.now+=100;m.poll(h.status)
                self.assertEqual(len(attempts),1);self.assertEqual(m.poll_interval(.25),.25)

    def test_new_recovery_resets_previous_receipt_timeout_polling(self):
        h=Harness()
        with h.monitor() as m:
            h.prepared(m);h.now=40;m.poll(h.status)
            self.assertTrue(m.timed_out)
            h.status.update(world_candidate_generation='0',world_handoff_phase='4')
            m.poll(h.status)
            self.assertFalse(m.timed_out);self.assertAlmostEqual(.1,m.poll_interval(.25))
            h.now=m.next_poll;m.poll(h.status)
            self.assertEqual('building',m.state);self.assertFalse(m.timed_out)
            self.assertEqual(.01,m.poll_interval(.25));h.finish_worker(m)

    def test_quiesced_retry_still_requires_exact_local_player(self):
        h=Harness();h.status['world_handoff_phase']='4'
        h.current=replace(h.current,local_player=obj(10,number=1))
        with h.monitor(max_attempts=1) as m:
            for _ in range(4):
                m.poll(h.status);h.finish_worker(m);h.now=m.next_poll
            self.assertFalse(h.builds);self.assertFalse(h.signals)

    def test_exact_native_rejection_gets_bounded_fast_retries_then_regular_backoff(self):
        h=Harness()
        with h.monitor(max_attempts=3) as m:
            h.prepared(m)
            for request,delay in enumerate((.1,.2,.3,6),start=1):
                h.status.update(world_candidate_generation='0',world_handoff_phase='4')
                event=m.poll(h.status)[0]
                self.assertAlmostEqual(delay,m.next_poll-h.now)
                self.assertEqual(event['fast_retry'],request<=3)
                self.assertAlmostEqual(event['retry_after_ms'],delay*1000)
                for _ in range(3):
                    m.poll(h.status);self.assertIsNone(m.future)
                self.assertAlmostEqual(min(.25,delay),m.poll_interval(.25))
                h.now=m.next_poll;m.poll(h.status);h.finish_worker(m)
                self.assertEqual(request+1,m.request)
                h.status.update(world_prepare_requests=str(request+1),world_candidate_generation='2')
                m.poll(h.status);self.assertEqual('committing',m.state)
            h.status.update(world_generation='2',world_candidate_generation='0',world_handoff_phase='3')
            m.poll(h.status);self.assertEqual(0,m.native_retry_attempts)

    def test_fast_retry_requires_exact_request_rejection_and_quiesced_phase(self):
        for mismatch in ('request','rejected','phase'):
            h=Harness()
            with h.monitor() as m:
                h.prepared(m)
                h.status.update(world_candidate_generation='0',world_handoff_phase='4')
                if mismatch=='request':h.status['world_prepare_requests']='2'
                if mismatch=='rejected':h.status['world_prepare_rejected']='1'
                if mismatch=='phase':h.status['world_handoff_phase']='0'
                event=m.poll(h.status)[0]
                self.assertNotIn('fast_retry',event);self.assertEqual(0,m.native_retry_attempts)
                self.assertEqual(2,m.next_poll-h.now)

    def test_worker_timing_preserves_qpc_boundaries_in_success_and_read_retry(self):
        from world_rebind_monitor import RetryablePreparation
        for retry in (False,True):
            h=Harness()
            def prepare(*args):
                if retry:raise RetryablePreparation(ValueError('test read-only timestamp rejection'))
                return h.prepare(*args)
            with h.monitor(prepare=prepare) as m:
                with mock.patch('world_rebind_monitor.time.perf_counter_ns',side_effect=[100,120,150,200]):
                    m.poll(h.status);event=h.finish_worker(m)[0]
                timing=event['worker_timing']
                self.assertEqual('qpc_monotonic_ns',timing['clock'])
                self.assertEqual((100,120,200),tuple(timing[key] for key in
                    ('requested_ns','worker_started_ns','worker_finished_ns')))
                self.assertEqual((.00003,.00005,.00008),tuple(timing[key] for key in
                    ('observe_ms','prepare_ms','worker_ms')))
                self.assertEqual('world_rebind_failed' if retry else 'world_rebind_prepare',event['event'])
                self.assertEqual(0 if retry else 1,len(h.signals))

    def test_native_timestamp_rejections_do_not_strand_a_stable_target(self):
        h=Harness()
        with h.monitor() as m:
            h.published(m)
            for request,delay in enumerate((.05,.1,.15,2,2,2),start=1):
                h.status.update(world_prepare_requests=str(request),world_prepare_rejected=str(request),
                    world_candidate_generation='0',world_prepare_timestamp_request=str(request))
                event=m.poll(h.status)[0]
                self.assertEqual('native_timestamp_sample',event['retry_scope'])
                self.assertEqual(request<=3,event['fast_retry'])
                self.assertEqual(delay*1000,event['retry_after_ms'])
                self.assertEqual(0,m.attempts)
                self.assertEqual(request,m.native_prepare_retry_attempts)
                self.assertAlmostEqual(min(.25,delay),m.poll_interval(.25))
                self.assertEqual(snapshot(),m.baseline)
                self.assertIsNone(m.prepared)
                m.poll(h.status);self.assertIsNone(m.future)
                h.now=m.next_poll;m.poll(h.status)
                self.assertEqual(.01,m.poll_interval(.25))
                h.finish_worker(m)
                self.assertEqual('preparing',m.state)
                self.assertEqual(request+1,m.request)
                self.assertEqual(request+1,len(h.builds))
                self.assertTrue(all('Prepare' in signal for signal in h.signals))
            self.assertEqual(list(range(1,8)),[build[2] for build in h.builds])
            h.status.update(world_prepare_requests='7',world_candidate_generation='2',world_prepare_timestamp_request='0')
            m.poll(h.status)
            self.assertEqual('committing',m.state)
            self.assertEqual(0,m.native_prepare_retry_attempts)
            self.assertIn('Commit',h.signals[-1])
            h.status.update(world_generation='2',world_candidate_generation='0',world_handoff_phase='3')
            self.assertEqual('world_rebind_committed',m.poll(h.status)[0]['event'])
            self.assertEqual(h.current,m.baseline)

    def test_native_timestamp_retry_requires_exact_owned_rejection_receipt(self):
        for mismatch in ('missing','stale','counter','request','numeric','padded','candidate'):
            h=Harness()
            with h.monitor() as m:
                h.published(m)
                h.status.update(world_prepare_requests='1',world_prepare_rejected='1',world_prepare_timestamp_request='1')
                if mismatch=='missing':del h.status['world_prepare_timestamp_request']
                if mismatch=='stale':h.status['world_prepare_timestamp_request']='2'
                if mismatch=='counter':h.status['world_prepare_rejected']='2'
                if mismatch=='request':h.status['world_prepare_requests']='2'
                if mismatch=='numeric':h.status['world_prepare_timestamp_request']=1
                if mismatch=='padded':h.status['world_prepare_timestamp_request']='01'
                if mismatch=='candidate':h.status['world_candidate_generation']='2'
                event=m.poll(h.status)[0]
                self.assertNotIn('retry_scope',event,mismatch)
                self.assertEqual(0,m.native_prepare_retry_attempts,mismatch)
                self.assertEqual(1,m.attempts,mismatch)
                self.assertEqual(1,len(h.signals),mismatch)

    def test_native_timestamp_retry_preserves_prior_structural_failure_budget(self):
        h=Harness()
        with h.monitor(max_attempts=2) as m:
            h.published(m)
            # An ordinary native rejection consumes one attempt.
            h.status.update(world_prepare_requests='1',world_prepare_rejected='1')
            m.poll(h.status);self.assertEqual(1,m.attempts)
            h.now=m.next_poll;m.poll(h.status);h.finish_worker(m)
            # Timestamp rejection restores just this attempt, not prior failures.
            h.status.update(world_prepare_requests='2',world_prepare_rejected='2',world_prepare_timestamp_request='2')
            m.poll(h.status);self.assertEqual(1,m.attempts)
            h.now=m.next_poll;m.poll(h.status);h.finish_worker(m)
            h.status.update(world_prepare_requests='3',world_prepare_rejected='3',world_prepare_timestamp_request='0')
            m.poll(h.status);self.assertEqual(2,m.attempts)
            for _ in range(3):
                h.now+=10;m.poll(h.status);h.finish_worker(m)
            self.assertEqual(3,len(h.builds))
            self.assertEqual(3,len(h.signals))

    def test_native_timestamp_receipt_does_not_bypass_session_or_process_identity(self):
        for mismatch in ('process','started_qpc','stop_recorded','owner_creation_time'):
            h=Harness()
            with h.monitor() as m:
                h.published(m)
                h.status.update(world_prepare_requests='1',world_prepare_rejected='1',world_prepare_timestamp_request='1')
                if mismatch=='process':h.game=dict(h.game,creation_time=999)
                else:h.status[mismatch]='999'
                m.poll(h.status)
                self.assertEqual(0,m.native_prepare_retry_attempts,mismatch)
                self.assertEqual(1,len(h.signals),mismatch)
                self.assertEqual(snapshot(),m.baseline,mismatch)

    def test_late_generation_with_foreign_request_does_not_update_baseline(self):
        h = Harness()
        with h.monitor() as m:
            h.prepared(m)
            h.status.update(world_generation='2', world_prepare_requests='99')
            self.assertEqual('world_rebind_blocked', m.poll(h.status)[0]['event'])
            self.assertEqual(snapshot(), m.baseline)


if __name__ == '__main__':
    unittest.main()
