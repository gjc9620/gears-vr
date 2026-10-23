"""Real sampler rejection evidence plus mocked owner; no game or XR opened."""
import copy
from types import SimpleNamespace
import tempfile
import unittest
from unittest import mock

from test_anchor_config import Fixture
from test_world_rebind_monitor import Harness,snapshot
import test_world_rebind_runtime as adapter_fixtures
from anchor_config import (LiveRegistryAnchorSampler,AnchorRejected,AnchorSamplingRejected,
                           AnchorRegistryChanged,timestamp_guard_retryable)
from mounted_context import MountedTimestampChanged
from world_rebind_monitor import RetryablePreparation
import world_rebind_runtime as runtime


def timestamp_failure(mounted=False):
    inner=MountedTimestampChanged('Mounted camera timestamp changed at bracket boundary') if mounted else AnchorRejected(
        'Cross-frame camera timestamp during identity checks')
    with mock.patch.object(LiveRegistryAnchorSampler,'_strict_guard',side_effect=inner):
        try:LiveRegistryAnchorSampler(Fixture())
        except AnchorSamplingRejected as error:return error
    raise AssertionError('Expected actual strict sampler rejection')


class TimestampRecoveryTests(unittest.TestCase):
    def test_only_actual_rejected_timestamp_attempts_qualify(self):
        for mounted in (False,True):
            error=timestamp_failure(mounted)
            self.assertTrue(timestamp_guard_retryable(error))
            self.assertEqual(5,len(error.evidence['guard_acquisitions'][-1]['attempts']))
        original=timestamp_failure()
        for case in ('success','late_success','mixed','string_bool','bad_reason','bad_type','missing','too_many','outer_reason','registry_subclass'):
            evidence=copy.deepcopy(original.evidence);guard=evidence['guard_acquisitions'][-1]
            reason=str(original);kind=AnchorSamplingRejected
            if case=='success':guard['accepted']=True
            if case=='late_success':guard['attempts'][-1]['strict_guard_accepted']=True
            if case=='mixed':guard['attempts'][0]['retryable']=False
            if case=='string_bool':guard['attempts'][0]['retryable']='true'
            if case=='bad_reason':guard['attempts'][0]['reason']='Code mismatch'
            if case=='bad_type':guard['attempts'][0]['exception_type']=[]
            if case=='missing':evidence={}
            if case=='too_many':guard['attempts']*=2
            if case=='outer_reason':reason='Unrelated code failure'
            if case=='registry_subclass':kind=AnchorRegistryChanged
            with self.subTest(case=case):self.assertFalse(timestamp_guard_retryable(kind(reason,evidence)))
        fake=RuntimeError(str(original));fake.evidence=original.evidence
        self.assertFalse(timestamp_guard_retryable(fake))

    def test_adapter_marks_only_read_only_errors_and_preserves_original(self):
        error=timestamp_failure()
        with self.assertRaises(RetryablePreparation) as marked:
            runtime._read_only_acquisition(mock.Mock(side_effect=error))
        self.assertIs(marked.exception.error,error)
        hard=AnchorSamplingRejected('Code mismatch',error.evidence)
        with self.assertRaises(AnchorSamplingRejected) as raised:
            runtime._read_only_acquisition(mock.Mock(side_effect=hard))
        self.assertIs(raised.exception,hard)

    def test_observation_closes_its_reader_before_retry(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=adapter_fixtures.Tests().adapter(directory)
            reader=SimpleNamespace(resolve=mock.Mock(return_value={}),p=SimpleNamespace(close=mock.Mock()))
            with mock.patch.object(runtime,'Resolver',return_value=reader), \
                 mock.patch.object(runtime,'LiveRegistryAnchorSampler',side_effect=timestamp_failure()):
                with self.assertRaises(RetryablePreparation):adapter.observe(lambda:None)
            reader.p.close.assert_called_once()

    def test_preparation_failure_closes_attestation_and_never_publishes(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=adapter_fixtures.Tests().adapter(directory);proof=mock.MagicMock()
            with mock.patch.object(runtime,'HookEntryAttestation',return_value=proof), \
                 mock.patch.object(runtime,'build_candidate',side_effect=timestamp_failure()), \
                 mock.patch.object(runtime,'publish_candidate') as publish:
                with self.assertRaises(RetryablePreparation):adapter.prepare(adapter.baseline,1,1,lambda:None)
            proof.__exit__.assert_called_once();publish.assert_not_called()

    def test_publication_exception_is_never_reclassified(self):
        from test_world_rebind_runtime import config
        with tempfile.TemporaryDirectory() as directory:
            adapter=adapter_fixtures.Tests().adapter(directory);error=timestamp_failure()
            candidate=SimpleNamespace(anchor_metadata=dict(config=config()))
            with mock.patch.object(runtime,'HookEntryAttestation'), \
                 mock.patch.object(runtime,'build_candidate',return_value=candidate), \
                 mock.patch.object(runtime,'publish_candidate',side_effect=error):
                with self.assertRaises(AnchorSamplingRejected) as raised:adapter.prepare(adapter.baseline,1,1,lambda:None)
            self.assertIs(raised.exception,error)

    def test_bounded_short_retries_then_regular_wait_and_full_recovery(self):
        for stage in ('observe','prepare'):
            h=Harness();calls=[0];error=timestamp_failure()
            def reject_or_finish(*args):
                calls[0]+=1
                if calls[0]<=5:raise RetryablePreparation(error)
                return h.observe(*args) if stage=='observe' else h.prepare(*args)
            with h.monitor(**{stage:reject_or_finish}) as monitor:
                for expected in (.05,.1,.15,1.7,.3):
                    monitor.poll(h.status);events=h.finish_worker(monitor)
                    self.assertAlmostEqual(expected,monitor.next_poll-h.now)
                    self.assertEqual(events[0]['reason'],'AnchorSamplingRejected')
                    self.assertEqual(events[0]['retry_scope'],'timestamp_sample')
                    self.assertEqual(0,monitor.attempts);self.assertFalse(h.signals)
                    monitor.poll(h.status);self.assertIsNone(monitor.future)
                    self.assertAlmostEqual(min(.25,expected),monitor.poll_interval(.25))
                    h.now=monitor.next_poll
                monitor.poll(h.status);self.assertTrue(monitor.fast_build_poll);h.finish_worker(monitor)
                self.assertEqual('preparing',monitor.state);self.assertEqual(0,monitor.sampling_retry_attempts)
                self.assertEqual(1,len(h.builds));self.assertEqual(1,len(h.signals))
                h.status.update(world_prepare_requests='1',world_candidate_generation='2');monitor.poll(h.status)
                h.status.update(world_generation='2',world_candidate_generation='0');monitor.poll(h.status)
                self.assertEqual(h.current,monitor.baseline)

    def test_diagnostic_retryable_summary_alone_keeps_ordinary_backoff(self):
        h=Harness();error=timestamp_failure()
        with h.monitor(prepare=mock.Mock(side_effect=error)) as monitor:
            monitor.poll(h.status);event=h.finish_worker(monitor)[0]
            self.assertTrue(event['guard_failure']['retryable'])
            self.assertNotIn('fast_retry',event);self.assertEqual(2,monitor.next_poll-h.now)
            self.assertEqual(1,monitor.attempts)

    def test_fresh_unchanged_observation_resets_the_short_retry_budget(self):
        h=Harness();h.current=snapshot();reject=[True]
        def observe(check):
            if reject[0]:raise RetryablePreparation(timestamp_failure())
            return h.observe(check)
        with h.monitor(observe=observe,max_attempts=1) as monitor:
            monitor.poll(h.status);h.finish_worker(monitor);self.assertEqual(1,monitor.sampling_retry_attempts)
            h.now=monitor.next_poll;reject[0]=False;monitor.poll(h.status);h.finish_worker(monitor)
            self.assertEqual(0,monitor.sampling_retry_attempts);self.assertFalse(h.signals)
            h.now=monitor.next_poll;reject[0]=True;monitor.poll(h.status);h.finish_worker(monitor)
            self.assertAlmostEqual(.05,monitor.next_poll-h.now)

    def test_changed_owner_during_cooldown_cannot_start_another_acquisition(self):
        h=Harness();prepare=mock.Mock(side_effect=RetryablePreparation(timestamp_failure()))
        with h.monitor(prepare=prepare) as monitor:
            monitor.poll(h.status);h.finish_worker(monitor)
            h.owner=dict(h.owner,creation_time=999);h.now=monitor.next_poll;monitor.poll(h.status)
            self.assertEqual(1,prepare.call_count);self.assertIsNone(monitor.future);self.assertFalse(h.signals)


if __name__=='__main__':unittest.main()
