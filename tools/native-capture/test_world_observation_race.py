"""Certified preparation vs prior observation; synthetic memory and owner only."""
import dataclasses
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import world_rebind_runtime as runtime
import world_rebind as rebind
import test_world_rebind_runtime as adapter_fixtures
import test_world_rebind_serialization as candidate_fixtures
from test_mounted_integration import MountedUIFixture
from test_world_rebind_monitor import Harness
from world_rebind_monitor import RetryablePreparation


def certified_candidate(number=0):
    fixture=MountedUIFixture();fixture.word(fixture.TURRET+76,number)
    with mock.patch.object(candidate_fixtures,'MountedUIFixture',return_value=fixture):
        return candidate_fixtures.mounted_candidate()


def candidate_snapshot(candidate):
    return runtime.snapshot(rebind.thaw(candidate.anchor_metadata['config']))


def adapter_for(directory,candidate):
    adapter=adapter_fixtures.Tests().adapter(directory)
    adapter.game=rebind.thaw(candidate.game_identity);adapter.owner=rebind.thaw(candidate.owner_identity)
    adapter.local=rebind.thaw(candidate.local_identity);adapter.profile=MountedUIFixture().profile
    return adapter


def owner_for(candidate,baseline):
    h=Harness();h.game=rebind.thaw(candidate.game_identity);h.owner=rebind.thaw(candidate.owner_identity)
    h.current=candidate_snapshot(candidate)
    h.status.update(pid=str(h.game['pid']),owner_pid=str(h.owner['pid']),owner_creation_time=str(h.owner['creation_time']))
    return h,h.monitor(baseline=candidate_snapshot(baseline),identity=lambda pid:h.game if pid==h.game['pid']else h.owner)


class ObservationRaceTests(unittest.TestCase):
    def test_real_certified_mismatch_keeps_both_snapshots_and_never_publishes(self):
        observed_candidate=certified_candidate();current=certified_candidate(1)
        observed=candidate_snapshot(observed_candidate);actual=candidate_snapshot(current)
        self.assertNotEqual(observed,actual);self.assertEqual(observed.local_player,actual.local_player)
        before=rebind.serialize_candidate(current)
        with tempfile.TemporaryDirectory()as directory:
            adapter=adapter_for(directory,current)
            with mock.patch.object(runtime,'HookEntryAttestation')as proof,\
                 mock.patch.object(runtime,'build_candidate',return_value=current),\
                 mock.patch.object(runtime,'publish_candidate')as publish,\
                 mock.patch.object(adapter,'_remember_prepared_camera')as remember,\
                 self.assertRaises(RetryablePreparation)as rejected:
                adapter.prepare(observed,1,1,lambda:None)
            error=rejected.exception
            self.assertEqual(error.scope,'acquisition_churn');self.assertIs(type(error.error),runtime.WorldObservationChanged)
            self.assertEqual(error.error.observed,observed);self.assertEqual(error.error.current,actual)
            self.assertIs(error.__cause__,error.error);proof.return_value.__exit__.assert_called_once()
            publish.assert_not_called();remember.assert_not_called()
        self.assertEqual(rebind.serialize_candidate(current),before)

    def test_matching_full_candidate_keeps_original_publication_and_receipt(self):
        candidate=certified_candidate();observed=candidate_snapshot(candidate)
        with tempfile.TemporaryDirectory()as directory:
            adapter=adapter_for(directory,candidate)
            with mock.patch.object(runtime,'HookEntryAttestation')as proof,\
                 mock.patch.object(runtime,'build_candidate',return_value=candidate)as build,\
                 mock.patch.object(runtime,'publish_candidate')as publish:
                receipt=adapter.prepare(observed,1,3,lambda:None)
            self.assertEqual(receipt.snapshot,observed);self.assertEqual((receipt.previous_generation,receipt.request),(1,3))
            publish.assert_called_once_with(candidate,receipt.path);build.assert_called_once()
            self.assertIsNotNone(adapter._camera_hint);proof.return_value.__exit__.assert_called_once()

    def test_missing_malformed_or_foreign_local_observation_remains_hard(self):
        candidate=certified_candidate();actual=candidate_snapshot(candidate)
        foreign=dataclasses.replace(actual,local_player=dict(zip(
            ('address','cls','outer','number','internal_index','name'),actual.local_player))|{'number':19})
        for observed in (None,{},object(),foreign):
            with self.subTest(observed=type(observed).__name__),tempfile.TemporaryDirectory()as directory:
                adapter=adapter_for(directory,candidate)
                with mock.patch.object(runtime,'HookEntryAttestation'),mock.patch.object(runtime,'build_candidate',return_value=candidate),\
                     mock.patch.object(runtime,'publish_candidate')as publish,self.assertRaises(RuntimeError)as rejected:
                    adapter.prepare(observed,1,1,lambda:None)
                self.assertNotIsInstance(rejected.exception,RetryablePreparation);publish.assert_not_called()

    def test_same_message_or_typed_error_from_other_stages_does_not_grant_retry(self):
        candidate=certified_candidate();observed=candidate_snapshot(candidate)
        message='World changed during automatic certificate preparation'
        for stage in ('proof','build','publish'):
            for typed in (False,True):
                error=runtime.WorldObservationChanged(observed,observed)if typed else RuntimeError(message)
                with self.subTest(stage=stage,typed=typed),tempfile.TemporaryDirectory()as directory:
                    adapter=adapter_for(directory,candidate)
                    with mock.patch.object(runtime,'HookEntryAttestation',**({'side_effect':error}if stage=='proof'else{})),\
                         mock.patch.object(runtime,'build_candidate',**({'side_effect':error}if stage=='build'else{'return_value':candidate})),\
                         mock.patch.object(runtime,'publish_candidate',**({'side_effect':error}if stage=='publish'else{}))as publish,\
                         self.assertRaises(RuntimeError)as rejected:
                        adapter.prepare(observed,1,1,lambda:None)
                    self.assertIs(rejected.exception,error);self.assertNotIsInstance(rejected.exception,RetryablePreparation)
                    if stage!='publish':publish.assert_not_called()

    def check_owner_reacquisition(self,prior_hard=False):
        target=certified_candidate();other=certified_candidate(1)
        h,monitor=owner_for(target,other);calls=[];before_target=candidate_snapshot(other)
        with tempfile.TemporaryDirectory()as directory:
            adapter=adapter_for(directory,target)
            def build(*args,**kwargs):
                calls.append(h.now)
                if prior_hard and len(calls)==1:raise ValueError('Synthetic hard code failure')
                return other if len(calls)<=5+int(prior_hard)else target
            monitor.prepare=adapter.prepare
            with monitor,mock.patch.object(runtime,'HookEntryAttestation')as proof,\
                 mock.patch.object(runtime,'build_candidate',side_effect=build),\
                 mock.patch.object(runtime,'publish_candidate')as publish:
                if prior_hard:
                    monitor.poll(h.status);event=h.finish_worker(monitor)[0]
                    self.assertNotIn('retry_scope',event);self.assertEqual(monitor.attempts,1)
                    h.now=monitor.next_poll
                for delay in (.05,.1,.15,1.7,.3):
                    monitor.poll(h.status);event=h.finish_worker(monitor)[0]
                    self.assertEqual(event['reason'],'WorldObservationChanged');self.assertEqual(event['retry_scope'],'acquisition_churn')
                    self.assertEqual(monitor.attempts,int(prior_hard));self.assertAlmostEqual(monitor.next_poll-h.now,delay)
                    self.assertEqual(monitor.baseline,before_target);self.assertFalse(h.signals);publish.assert_not_called()
                    h.now=monitor.next_poll
                monitor.poll(h.status);h.finish_worker(monitor)
                self.assertEqual(monitor.state,'preparing');self.assertEqual(len(calls),6+int(prior_hard))
                publish.assert_called_once_with(target,monitor.prepared.path);self.assertEqual(len(h.signals),1)
                h.status.update(world_prepare_requests='1',world_candidate_generation='2');monitor.poll(h.status)
                h.status.update(world_generation='2',world_candidate_generation='0');monitor.poll(h.status)
                self.assertEqual(monitor.baseline,h.current);self.assertEqual(monitor.attempts,0);self.assertEqual(len(h.signals),2)
                self.assertEqual(proof.return_value.__exit__.call_count,len(calls))

    def test_owner_reacquires_after_five_races_and_commits_only_matching_certificate(self):
        self.check_owner_reacquisition()

    def test_observation_races_do_not_clear_existing_hard_failure_budget(self):
        self.check_owner_reacquisition(prior_hard=True)

    def test_single_acquisition_still_requires_its_snapshot_predicate(self):
        candidate=certified_candidate()
        with tempfile.TemporaryDirectory()as directory:
            adapter=adapter_for(directory,candidate);accept=mock.Mock(return_value=False)
            with mock.patch.object(runtime,'HookEntryAttestation'),mock.patch.object(runtime,'build_candidate',return_value=candidate),\
                 mock.patch.object(runtime,'publish_candidate')as publish:
                actual,receipt=adapter.prepare_changed(1,1,accept,lambda:None)
            accept.assert_called_once_with(actual);self.assertEqual(actual,candidate_snapshot(candidate))
            self.assertIsNone(receipt);publish.assert_not_called()


if __name__=='__main__':unittest.main()
