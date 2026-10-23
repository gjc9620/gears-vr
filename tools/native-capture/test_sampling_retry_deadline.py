"""Owner clock regressions for bounded read probes and ordinary retry deadlines."""
import unittest
from unittest import mock

from world_rebind_monitor import RetryablePreparation
from test_world_rebind_monitor import Harness,snapshot


def transient(scope='timestamp_sample'):
    return RetryablePreparation(ValueError('Synthetic rejected read'),scope=scope)


class SamplingDeadlineTests(unittest.TestCase):
    def test_both_read_scopes_keep_first_ordinary_deadline_and_later_cadence(self):
        for scope in ('timestamp_sample','acquisition_churn'):
            h=Harness();called=[]
            def prepare(*args):
                called.append(h.now);raise transient(scope)
            with self.subTest(scope=scope),h.monitor(prepare=prepare)as monitor:
                for when,due,fast in ((0,.05,True),(.05,.15,True),(.15,.3,True),(.3,2,False),(2,2.3,False),(2.3,4.3,False),(4.3,6.3,False)):
                    self.assertAlmostEqual(h.now,when);monitor.poll(h.status);event=h.finish_worker(monitor)[0]
                    self.assertAlmostEqual(monitor.next_poll,due)
                    self.assertEqual(event['fast_retry'],fast);self.assertEqual(event['retry_scope'],scope)
                    self.assertEqual(monitor.attempts,0);self.assertFalse(h.builds);self.assertFalse(h.signals)
                    h.now=monitor.next_poll
                for actual,expected in zip(called,(0,.05,.15,.3,2,2.3,4.3),strict=True):self.assertAlmostEqual(actual,expected)

    def test_all_attempt_bounds_and_small_backoffs_retain_existing_regular_slots(self):
        for maximum in (1,3,10):
            for backoff in (.01,.2,2):
                h=Harness()
                with self.subTest(maximum=maximum,backoff=backoff),h.monitor(
                        prepare=mock.Mock(side_effect=transient()),max_attempts=maximum,backoff=backoff)as monitor:
                    original=[0.]
                    for number in range(1,maximum+1):original.append(original[-1]+min(backoff,.05*number))
                    first_regular=original[-1]+backoff
                    extra=[backoff]if original[-1]<backoff<first_regular else[]
                    expected=original+extra+[first_regular+backoff*index for index in range(4)]
                    for index,when in enumerate(expected):
                        self.assertAlmostEqual(h.now,when)
                        monitor.poll(h.status);event=h.finish_worker(monitor)[0]
                        self.assertEqual(event['fast_retry'],index<maximum)
                        self.assertEqual(monitor.attempts,0);self.assertGreater(monitor.next_poll,h.now)
                        h.now=monitor.next_poll
                    self.assertFalse(h.signals)

    def test_time_spent_in_failed_workers_is_inside_first_retry_window(self):
        h=Harness();durations=iter((.12,.07,.09,.11));completed=[]
        def prepare(*args):
            h.now+=next(durations);completed.append(h.now);raise transient('acquisition_churn')
        with h.monitor(prepare=prepare)as monitor:
            for _ in range(4):
                monitor.poll(h.status);h.finish_worker(monitor);h.now=monitor.next_poll
            self.assertEqual(len(completed),4)
            self.assertAlmostEqual(monitor.next_poll,2.12)
            self.assertAlmostEqual(monitor.next_poll-completed[-1],1.43)
            self.assertEqual(monitor.attempts,0);self.assertFalse(h.signals)

    def test_slow_failed_worker_keeps_bounded_burst_without_extra_catchup(self):
        h=Harness();calls=[0]
        def prepare(*args):
            calls[0]+=1
            if calls[0]==2:h.now+=2.5
            raise transient()
        with h.monitor(prepare=prepare)as monitor:
            monitor.poll(h.status);h.finish_worker(monitor);h.now=monitor.next_poll
            monitor.poll(h.status);event=h.finish_worker(monitor)[0]
            self.assertTrue(event['fast_retry']);self.assertAlmostEqual(monitor.next_poll-h.now,.1)
            self.assertEqual(monitor.sampling_retry_attempts,2)
            h.now=monitor.next_poll;monitor.poll(h.status);event=h.finish_worker(monitor)[0]
            self.assertTrue(event['fast_retry']);self.assertAlmostEqual(monitor.next_poll-h.now,.15)
            h.now=monitor.next_poll;monitor.poll(h.status);event=h.finish_worker(monitor)[0]
            self.assertFalse(event['fast_retry']);self.assertAlmostEqual(monitor.next_poll-h.now,2)
            self.assertFalse(h.signals)

    def test_recovery_just_after_early_probe_keeps_the_original_followup(self):
        for ready,completion in ((.02,.05),(.31,2),(1.99,2),(2.01,2.3),(3,4.3)):
            h=Harness();calls=[]
            def prepare(*args):
                calls.append(h.now)
                if h.now<ready:raise transient('acquisition_churn')
                return h.prepare(*args)
            with self.subTest(ready=ready),h.monitor(prepare=prepare)as monitor:
                for _ in range(12):
                    monitor.poll(h.status);h.finish_worker(monitor)
                    if monitor.state=='preparing':break
                    h.now=monitor.next_poll
                self.assertEqual(monitor.state,'preparing');self.assertAlmostEqual(h.now,completion)
                self.assertEqual(len(h.builds),1);self.assertEqual(len(h.signals),1)
                h.status.update(world_prepare_requests='1',world_candidate_generation='2');monitor.poll(h.status)
                h.status.update(world_generation='2',world_candidate_generation='0');monitor.poll(h.status)
                self.assertEqual(monitor.baseline,h.current);self.assertEqual(len(h.signals),2)

    def test_ordinary_deadline_tail_controls_real_wait_and_blocks_early_hint(self):
        h=Harness();prepare=mock.Mock(side_effect=transient());hint=mock.Mock(return_value=True)
        with h.monitor(prepare=prepare,changed_hint=hint,hint_interval=.05)as monitor:
            for _ in range(4):
                monitor.poll(h.status);h.finish_worker(monitor);h.now=monitor.next_poll
            h.now=1.98;hint.reset_mock();monitor.poll(h.status)
            self.assertIsNone(monitor.future);hint.assert_not_called();self.assertEqual(prepare.call_count,4)
            with mock.patch('world_rebind_monitor.time.sleep')as sleep:
                monitor.wait(.25)
            self.assertAlmostEqual(sleep.call_args.args[0],.02)
            monitor.poll(None);self.assertIsNone(monitor.future)
            h.now=2;monitor.poll(h.status);h.finish_worker(monitor)
            self.assertEqual(prepare.call_count,5);self.assertFalse(h.signals)

    def test_hard_failures_keep_budget_and_replace_old_transient_deadline(self):
        h=Harness();calls=[0]
        def prepare(*args):
            calls[0]+=1
            if calls[0]in(1,5):raise ValueError('Synthetic hard identity failure')
            raise transient()
        with h.monitor(prepare=prepare)as monitor:
            for call in range(1,7):
                monitor.poll(h.status);event=h.finish_worker(monitor)[0]
                self.assertEqual(monitor.attempts,1 if call<5 else 2)
                if call in(1,5):
                    self.assertNotIn('retry_scope',event)
                    self.assertAlmostEqual(monitor.next_poll-h.now,2 if call==1 else 4)
                if call==6:
                    self.assertFalse(event['fast_retry']);self.assertAlmostEqual(monitor.next_poll-h.now,4)
                h.now=monitor.next_poll
            self.assertFalse(h.signals);self.assertFalse(h.builds)

    def test_complete_fresh_acquisition_and_commit_reset_the_retry_window(self):
        h=Harness();fail=[True]
        def prepare(*args):
            if fail[0]:raise transient()
            return h.prepare(*args)
        with h.monitor(prepare=prepare)as monitor:
            for _ in range(4):
                monitor.poll(h.status);h.finish_worker(monitor);h.now=monitor.next_poll
            fail[0]=False;monitor.poll(h.status);h.finish_worker(monitor)
            self.assertEqual(monitor.sampling_retry_attempts,0)
            self.assertIsNone(monitor.sampling_retry_due);self.assertFalse(monitor.sampling_retry_regular)
            h.status.update(world_prepare_requests='1',world_candidate_generation='2');monitor.poll(h.status)
            h.status.update(world_generation='2',world_candidate_generation='0');monitor.poll(h.status)
            self.assertEqual(monitor.baseline,h.current);self.assertEqual(len(h.signals),2)
            h.current=snapshot(2);h.now=monitor.next_poll;fail[0]=True
            monitor.poll(h.status);event=h.finish_worker(monitor)[0]
            self.assertTrue(event['fast_retry']);self.assertAlmostEqual(monitor.next_poll-h.now,.05)
            self.assertEqual(len(h.builds),1);self.assertEqual(len(h.signals),2)

    def test_deadline_never_authorizes_foreign_candidate_session_or_process(self):
        for fault in ('candidate','owner_creation_time','started_qpc','stop_recorded','publisher_stopped','process','disabled','closed'):
            h=Harness();prepare=mock.Mock(side_effect=transient())
            with self.subTest(fault=fault),h.monitor(prepare=prepare)as monitor:
                for _ in range(4):
                    monitor.poll(h.status);h.finish_worker(monitor);h.now=monitor.next_poll
                if fault=='candidate':h.status['world_candidate_generation']='2'
                elif fault=='process':h.owner=dict(h.owner,creation_time=999)
                elif fault=='disabled':monitor.enabled=False
                elif fault=='closed':monitor.close()
                else:h.status[fault]='999'
                monitor.poll(h.status)
                self.assertIsNone(monitor.future);self.assertEqual(prepare.call_count,4)
                self.assertFalse(h.builds);self.assertFalse(h.signals);self.assertEqual(monitor.baseline,snapshot())


if __name__=='__main__':unittest.main()
