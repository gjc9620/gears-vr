"""Deterministic service lifecycle tests; no process, input, native or XR actions."""
from concurrent.futures import Future
import json
import threading
import time
import unittest
from unittest import mock
from types import SimpleNamespace

from path_choice_center_monitor import _ReadWorker
from path_choice_position_collect import ActivationCertificate, AcquisitionSourceChanged
from path_choice_position_monitor import LivePathChoicePositionProbe, Observation, PathChoicePositionMonitor, _acquisition_source_conflict


GAME = dict(pid=3, creation_time=30, image='game')
OWNER = dict(pid=4, creation_time=40, image='owner')
SESSION = dict(game_identity=GAME, owner_identity=OWNER, native_started_qpc=50, phase='running')


class ImmediateWorker:
    def __init__(self, probe):self.probe = probe; self.error = None
    def submit(self, function):
        future = Future()
        try:future.set_result(function())
        except Exception as error:future.set_exception(error)
        return future
    def close(self):self.probe.close(); return True


class Probe:
    def __init__(self, *args):
        self.present = True; self.absent = False; self.full_reads = 0; self.reads = 0; self.closed = False
    def capture(self, nonce, acquire, generation):
        self.reads += 1
        if not self.present:return Observation(False, scene_absent=self.absent)
        certificate = None
        if acquire:
            self.full_reads += 1
            roles = {key:dict(address=101 + index, internal_index=index, cls=201)
                     for index, key in enumerate(('scene', 'client', 'controller', 'local_player'))}
            baseline = dict(nonce=nonce, roles=roles, nodes=list(range(13)))
            certificate = ActivationCertificate('position nonce=' + str(nonce) + ' offset=374',
                'close nonce=' + str(nonce), json.dumps(baseline), '{}')
        return Observation(True, certificate, scene_address=101)
    def close(self):self.closed = True


class MonitorTests(unittest.TestCase):
    def setUp(self):
        self.now = 10.; self.actions = []; self.files = {}; self.identities = {3:GAME, 4:OWNER}
        self.status = dict(pid=3, owner_pid=4, owner_creation_time=40, started_qpc=50,
            phase='running', world_generation=1, path_position_service_compiled=1,
            path_position_identity_lost=0, path_position_ownership_lost=0,
            path_position_setter_failed=0, path_position_pending_restore=0, path_position_unresolved=0,
            path_position_active_nonce=0, path_position_consumed_nonce=0,
            path_position_last_restored_nonce=0, path_position_last_restored_tick=0)
        for prefix in ('path_position_', 'path_close_'):
            self.status.update({prefix + key:0 for key in ('nonce', 'renewals', 'accepted_tick', 'expires_tick')})
        case = self
        class Sidecar:
            def __init__(self, path):self.path = path; self.owned = False; self.raw = None; case.files[path] = self
            def publish(self, raw):
                if self.owned:raise RuntimeError('Already owned')
                self.owned = True; self.raw = raw; case.actions.append(('publish', self.path, raw))
                case.ack(self.path, first=True)
            def renew(self):
                case.actions.append(('renew', self.path, self.raw))
                if case.auto_ack:case.ack(self.path)
            def remove(self):
                if self.owned:case.actions.append(('remove', self.path, self.raw))
                self.owned = False
        self.Sidecar = Sidecar; self.auto_ack = True
        self.m = self.create()
        self.addCleanup(self.m.close)

    def create(self, **kwargs):
        return PathChoicePositionMonitor('position', 'close', SESSION, lambda:dict(self.status), self.identities.get,
            clock=lambda:self.now, tick_clock=lambda:int(self.now * 1000), nonce_clock=lambda:100,
            probe_factory=kwargs.pop('probe_factory', Probe), sidecar_factory=self.Sidecar,
            worker_factory=kwargs.pop('worker_factory', ImmediateWorker),
            **kwargs)

    def ack(self, path, first=False):
        prefix = 'path_position_' if path == 'position' else 'path_close_'
        self.status[prefix + 'nonce'] = self.m.nonce
        self.status[prefix + 'renewals'] = 0 if first else self.status[prefix + 'renewals'] + 1
        self.status[prefix + 'accepted_tick'] = int(self.now * 1000)
        self.status[prefix + 'expires_tick'] = int(self.now * 1000) + (6000 if path == 'position' else 8000)
        if path == 'position':
            self.status['path_position_active_nonce'] = self.status['path_position_consumed_nonce'] = self.m.nonce
            self.status['path_position_pending_restore'] = 1

    def step(self, amount=.25):
        self.now += amount
        return self.m.poll()

    def hold(self):
        for _ in range(8):
            self.m.poll()
            if self.m.state == 'holding':return
            self.now += .1
        self.fail('Did not enter holding: ' + str(self.m.state))

    def test_holds_over_30_seconds_with_one_baseline_nonce_and_exact_close_first_renewals(self):
        self.hold(); nonce = self.m.nonce; certificate = self.m.certificate
        for _ in range(160):self.step()
        self.assertEqual(self.m.state, 'holding')
        self.assertEqual(self.m.nonce, nonce); self.assertIs(self.m.certificate, certificate)
        self.assertEqual(self.m.probe.full_reads, 1)
        renewals = [row for row in self.actions if row[0] == 'renew']
        self.assertGreater(len(renewals), 20)
        self.assertEqual([row[1] for row in renewals], ['close', 'position'] * (len(renewals) // 2))
        self.assertEqual(set(row[2] for row in renewals if row[1] == 'position'),
                         {certificate.position_ini.encode('ascii')})

    def test_same_nonce_ack_is_insufficient_to_renew_position(self):
        self.hold(); self.auto_ack = False
        for _ in range(12):self.step()
        self.assertEqual(self.m.renew_phase, 'close')
        self.assertEqual([row[1] for row in self.actions if row[0] == 'renew'], ['close'])
        for _ in range(12):self.step()
        self.assertTrue(self.m.terminal); self.assertFalse(self.m.position.owned)
        self.assertTrue(self.m.close_sidecar.owned)

    def test_renewal_counter_without_advanced_expiry_cannot_touch_position(self):
        self.hold(); self.auto_ack = False
        for _ in range(10):self.step()
        self.assertEqual(self.m.renew_phase, 'close')
        self.status['path_close_renewals'] += 1
        self.m.poll()
        self.assertEqual(self.m.renew_phase, 'close')
        self.assertEqual([row[1] for row in self.actions if row[0] == 'renew'], ['close'])

    def test_false_close_restore_latches_until_actual_scene_leave_then_reopen(self):
        self.hold(); nonce = self.m.nonce
        self.status.update(path_position_last_restored_nonce=nonce, path_position_last_restored_tick=11000,
                           path_position_active_nonce=0, path_position_pending_restore=0)
        for _ in range(20):self.step()
        self.assertEqual(self.m.state, 'latched_closed'); self.assertEqual(self.m.probe.full_reads, 1)
        self.assertEqual(self.m.last_restored_nonce, nonce)
        self.m.probe.present = False; self.m.probe.absent = True
        for _ in range(6):self.step()
        self.m.probe.present = True
        for _ in range(12):self.step()
        self.assertEqual(self.m.state, 'holding'); self.assertGreater(self.m.nonce, nonce)
        self.assertEqual(self.m.probe.full_reads, 2)

    def test_overlay_stops_renewal_but_does_not_count_as_scene_leave(self):
        self.hold(); self.m.probe.present = False
        for _ in range(6):self.step()
        self.assertEqual(self.m.state, 'await_restore'); self.assertFalse(self.m.leave_seen)
        self.assertFalse(self.m.position.owned); self.assertTrue(self.m.close_sidecar.owned)

    def test_world_change_discards_completed_old_world_certificate(self):
        self.m.poll(); self.assertTrue(self.m.future.done())
        self.status['world_generation'] = 2
        events = self.m.poll()
        self.assertTrue(any(row.get('reason') == 'late_or_world_changed_read' for row in events))
        event=next(row for row in events if row.get('reason')=='late_or_world_changed_read')
        self.assertEqual((event['request_generation'],event['current_generation']),(1,2))
        self.assertEqual(event['age_seconds'],0);self.assertFalse(event['cancelled'])
        self.assertFalse(self.m.position.owned); self.assertFalse(self.m.close_sidecar.owned)

    def test_future_finishing_during_done_uses_a_clock_sample_after_result(self):
        self.m.close();case=self
        class FinishOnDone(Future):
            def __init__(self,function):super().__init__();self.function=function
            def done(self):
                if not super().done():
                    case.now+=.125
                    try:self.set_result(self.function())
                    except Exception as error:self.set_exception(error)
                return super().done()
        class Worker(ImmediateWorker):
            def submit(self,function):return FinishOnDone(function)
        self.m=self.create(worker_factory=Worker);self.addCleanup(self.m.close)
        self.m.poll();events=self.m.poll()
        self.assertEqual(self.now,10.125)
        self.assertFalse(any(row.get('reason')=='late_or_world_changed_read' for row in events))
        self.assertEqual([row['event'] for row in events],['path_position_close_published'])
        self.assertEqual(self.m.certificate_finished,10.125)
        self.assertTrue(self.m.close_sidecar.owned);self.assertFalse(self.m.position.owned)

    def test_cancel_discards_late_result_and_bounded_worker_join(self):
        self.m.poll(); self.m.close()
        self.assertEqual(self.m.poll(), [])
        self.assertFalse(self.m.position.owned); self.assertFalse(self.m.close_sidecar.owned)
        self.assertTrue(self.m.worker_joined)

    def test_stale_result_budget_is_wrapper_finished_time(self):
        self.m.poll(); self.now += 2.01
        events = self.m.poll()
        self.assertTrue(any(row.get('reason') == 'late_or_world_changed_read' for row in events))
        event=next(row for row in events if row.get('reason')=='late_or_world_changed_read')
        self.assertAlmostEqual(event['age_seconds'],2.01)
        self.assertEqual((event['request_generation'],event['current_generation']),(1,1))
        self.assertFalse(event['cancelled'])
        self.assertFalse(self.m.close_sidecar.owned)

    def test_completed_result_at_exact_two_second_budget_still_accepts(self):
        self.m.poll();self.now+=2
        events=self.m.poll()
        self.assertTrue(any(row['event']=='path_position_close_published' for row in events))
        self.assertFalse(any(row.get('reason')=='late_or_world_changed_read' for row in events))

    def test_cancelled_completed_result_reports_cancellation_without_publication(self):
        self.m.poll();completed=self.m.future;case=self
        class CancelOnDone:
            def done(self):case.m.cancel.set();return completed.done()
            def result(self):return completed.result()
        self.m.future=CancelOnDone();events=self.m.poll()
        event=next(row for row in events if row.get('reason')=='late_or_world_changed_read')
        self.assertTrue(event['cancelled']);self.assertEqual(event['age_seconds'],0)
        self.assertEqual((event['request_generation'],event['current_generation']),(1,1))
        self.assertEqual(self.actions,[])

    def test_slow_final_identity_status_check_cannot_extend_certificate_budget(self):
        count = [0]
        def delayed_status():
            count[0] += 1
            if count[0] == 3:self.now += 3
            return dict(self.status)
        self.m.read_status = delayed_status
        self.m.poll(); self.m.poll()
        self.assertTrue(self.m.terminal); self.assertFalse(self.m.close_sidecar.owned)

    def test_restore_nonce_without_clean_owner_ack_retains_close_sidecar(self):
        self.hold()
        self.status.update(path_position_last_restored_nonce=self.m.nonce,
                           path_position_last_restored_tick=11000, path_position_pending_restore=1)
        self.m.poll()
        self.assertTrue(self.m.close_sidecar.owned)
        self.assertEqual(self.m.last_restored_nonce, 0)

    def test_wrong_native_identity_and_rejection_are_terminal_without_global_stop(self):
        self.hold(); self.status['path_position_ownership_lost'] = 1
        self.m.poll(); self.assertTrue(self.m.terminal)
        self.assertFalse(self.m.position.owned); self.assertTrue(self.m.close_sidecar.owned)
        self.assertFalse(hasattr(self.m, 'signal'))

    def test_expired_native_receipt_stops_renewal(self):
        self.hold(); self.status['path_position_expires_tick'] = int(self.now * 1000) - 1
        self.m.poll(); self.assertTrue(self.m.terminal)
        self.assertFalse(self.m.position.owned); self.assertTrue(self.m.close_sidecar.owned)

    def test_cleanup_requires_both_native_and_entry_proof_or_game_absence(self):
        self.hold(); self.m.close()
        self.assertFalse(self.m.position.owned); self.assertTrue(self.m.close_sidecar.owned)
        self.assertFalse(self.m.finish_cleanup(native_cleanup_verified=True))
        self.assertFalse(self.m.finish_cleanup(entry_restore_verified=True))
        self.assertTrue(self.m.finish_cleanup(native_cleanup_verified=True, entry_restore_verified=True))
        self.assertFalse(self.m.close_sidecar.owned)

    def test_runner_cached_status_cannot_override_latest_native_world(self):
        self.status['started_qpc'] = 999
        self.m.poll(dict(self.status, started_qpc=50))
        self.assertTrue(self.m.terminal); self.assertEqual(self.m.probe.reads, 0)

    def test_acquisition_failure_preserves_bounded_detail_in_one_terminal_event(self):
        self.m.close()
        class SceneOwnerRejected(RuntimeError):pass
        class RejectedProbe(Probe):
            def capture(self, nonce, acquire, generation):
                try:
                    raise OSError('read failure ' + 'c' * 1000)
                except OSError as cause:
                    raise SceneOwnerRejected('Owner source read failed ' + 'x' * 1000) from cause
        self.m = self.create(probe_factory=RejectedProbe)
        self.addCleanup(self.m.close)
        self.m.poll()
        events = self.m.poll()
        self.assertEqual(len(events), 1)
        event = events[0]
        self.assertEqual(event['event'], 'path_position_revoked')
        self.assertEqual(event['reason'], 'SceneOwnerRejected')
        self.assertEqual(event['error_state'], 'acquiring')
        self.assertTrue(event['error_message'].startswith('Owner source read failed'))
        self.assertEqual(len(event['error_message']), 512)
        self.assertEqual(event['error_causes'][0]['type'], 'OSError')
        self.assertEqual(len(event['error_causes'][0]['message']), 256)
        self.assertLessEqual(len(event['error_frames']), 4)
        self.assertEqual(event['error_frames'][-1]['function'], 'capture')
        self.assertEqual(set(event['error_frames'][-1]), {'file', 'function', 'line'})
        self.assertTrue(self.m.terminal)
        self.assertFalse(self.m.position.owned); self.assertFalse(self.m.close_sidecar.owned)
        self.assertIsNone(self.m.future)
        self.assertEqual(self.m.poll(), [])

    def test_menu_and_owner_conflicts_survive_existing_error_detail(self):
        from scene_owner_snapshot import SceneOwnerRejected, read_conflict_error
        from menu_state import RegistrySnapshotChanged, SnapshotChanged
        for kind in (RegistrySnapshotChanged, SnapshotChanged, SceneOwnerRejected):
            error=read_conflict_error(kind,'changed','test_stage',0x300,b'abc',b'aXc',{})
            try:
                raise SceneOwnerRejected('Owner source read failed') from error
            except SceneOwnerRejected as wrapped:
                detail=self.m._exception_detail(wrapped)
            self.assertEqual(detail['error_read_conflict'],error.read_conflict)
            self.assertNotIn('registry',detail['error_read_conflict'])
        unrelated=RuntimeError('untrusted')
        unrelated.read_conflict={'registry':'objects'}
        self.assertNotIn('error_read_conflict',self.m._exception_detail(unrelated))

    def test_forced_registry_bracket_detail_is_terminal_without_retry(self):
        import struct
        from scene_owner_snapshot import read_conflict_error
        from menu_state import ForcedRegistryRefreshChanged
        error=read_conflict_error(ForcedRegistryRefreshChanged,
            'Registry header changed during forced refresh','forced_registry_bracket',0x100,
            struct.pack('<Qii',0x9000,1001,2000),struct.pack('<Qii',0x9000,1005,2000),{'objects':0x100})
        self.m.close()
        class RejectedProbe(Probe):
            def capture(self,*args):raise error
        self.m=self.create(probe_factory=RejectedProbe);self.addCleanup(self.m.close)
        self.m.poll();events=self.m.poll()
        self.assertEqual(len(events),1)
        self.assertEqual(events[0]['event'],'path_position_revoked')
        self.assertEqual(events[0]['error_read_conflict'],error.read_conflict)
        self.assertEqual(events[0]['reason'],'ForcedRegistryRefreshChanged')
        self.assertEqual(self.m.acquisition_attempts,1)
        self.assertTrue(self.m.terminal)
        self.assertEqual(self.actions,[])
        self.assertEqual(self.m.poll(),[])

    def test_collector_byte_conflict_evidence_survives_wrapper_and_stays_terminal(self):
        from path_choice_position_collect import AcquisitionSourceChanged
        self.m.close()
        class RejectedProbe(Probe):
            def capture(self, nonce, acquire, generation):
                try:
                    raise AcquisitionSourceChanged('repeated_read',0x1230,b'abc',b'aXc',
                        [dict(file='menu_state.py',function='member',line=399,field='Value')])
                except AcquisitionSourceChanged as cause:
                    raise RuntimeError('Owner source read failed') from cause
        self.m = self.create(probe_factory=RejectedProbe)
        self.addCleanup(self.m.close)
        self.m.poll(); events=self.m.poll()
        self.assertEqual(len(events),1)
        detail=events[0]['error_read_conflict']
        self.assertEqual(detail['changed_address'],'0x1231')
        self.assertEqual(detail['expected_window'],'6263')
        self.assertEqual(detail['actual_window'],'5863')
        self.assertEqual(detail['first_read'][0]['field'],'Value')
        self.assertEqual(events[0]['event'],'path_position_revoked')
        self.assertEqual(self.m.acquisition_attempts,1)
        self.assertTrue(self.m.terminal)
        self.assertEqual(self.actions,[])

    def test_live_verifier_rechecks_reader_and_frozen_world_and_pending_restore(self):
        module = SimpleNamespace(reader_process_identity=lambda reader:(3, 30))
        with mock.patch.dict('sys.modules', {'menu_control':module}):
            session, status = self.m.verify_session(object(), 1)
            self.assertEqual(session, SESSION)
            self.status['world_generation'] = 2
            with self.assertRaisesRegex(RuntimeError, 'world changed'):
                self.m.verify_session(object(), 1)
            self.status['world_generation'] = 1
            self.status['path_position_pending_restore'] = 1
            with self.assertRaisesRegex(RuntimeError, 'state changed'):
                self.m.verify_session(object(), 1)
            self.status['path_position_pending_restore'] = 0
            module.reader_process_identity = lambda reader:(3, 31)
            with self.assertRaisesRegex(RuntimeError, 'handle identity'):
                self.m.verify_session(object(), 1)

    def live_probe_modules(self):
        self.fixed_prefix_sample=self.enterContext(mock.patch("path_choice_position_collect.sample_fixed_prefix",
            side_effect=lambda reader, **options: reader.sample(**options)))
        reader = SimpleNamespace(
            sample=lambda **kwargs:dict(top=dict(cls='UIScene_PathChoice', address='0x65'), active_scenes=[]),
            r=SimpleNamespace(resolve=lambda:dict(controller_address='0x67', local_player_address='0x68')),
            close=mock.Mock())
        return {'menu_state':SimpleNamespace(MenuReader=lambda pid:reader),
                'menu_control':SimpleNamespace(reader_process_identity=lambda current:(3, 30))}

    @staticmethod
    def acquisition_error(phase='final_reread'):
        before=bytearray(96);after=bytearray(before);before[0x1d]=0x8f;after[0x1d]=0x9b
        return AcquisitionSourceChanged(phase,0x123000,bytes(before),bytes(after),
            [dict(file='synthetic.py',function='identity',line=1,field='StateStack')])

    def snapshot_retry_fixture(self, failures=1, *, unknown=False, world_change=False, postpublish=False, registry=False,
                               acquisition=None, prior_snapshot=False):
        self.m.close()
        class SnapshotChanged(RuntimeError):pass
        class SceneOwnerRejected(RuntimeError):pass
        class RegistrySnapshotChanged(RuntimeError):pass
        readers = []
        def new_reader(pid):
            index = len(readers)
            def sample(**kwargs):
                failing = (kwargs.get('include_owner') and index < failures and
                           (acquisition is None or (prior_snapshot and index==0))) or (
                    postpublish and self.m.close_sidecar.owned)
                if failing:
                    if world_change:self.status['world_generation'] = 2
                    if acquisition and not (prior_snapshot and index==0):raise self.acquisition_error(acquisition)
                    if registry:raise RegistrySnapshotChanged('Registry changed before snapshot verification')
                    try:
                        fault = RuntimeError if unknown else SnapshotChanged
                        raise fault('UI changed during snapshot')
                    except RuntimeError as cause:
                        raise SceneOwnerRejected('Owner source read failed') from cause
                return dict(top=dict(cls='UIScene_PathChoice', address='0x65'), active_scenes=[])
            reader = SimpleNamespace(sample=sample, close=mock.Mock(),acquisition_index=index,
                r=SimpleNamespace(resolve=lambda:dict(controller_address='0x67', local_player_address='0x68')))
            readers.append(reader)
            return reader
        modules = dict(menu_state=SimpleNamespace(MenuReader=new_reader, SnapshotChanged=SnapshotChanged,
            RegistrySnapshotChanged=RegistrySnapshotChanged),
            scene_owner_snapshot=SimpleNamespace(SceneOwnerRejected=SceneOwnerRejected),
            menu_control=SimpleNamespace(reader_process_identity=lambda reader:(3, 30)))
        self.enterContext(mock.patch.dict('sys.modules', modules))
        self.enterContext(mock.patch("path_choice_position_collect.sample_fixed_prefix",
            side_effect=lambda reader, **options: reader.sample(**options)))
        certificate_probe = Probe()
        def collect(reader, sample, status, session, nonce, verify, *, registry_header_policy, target_vertical_region):
            self.assertEqual(registry_header_policy, 'fixed_prefix_v1')
            self.assertEqual(target_vertical_region, (.25, .75))
            verify(reader)
            if acquisition and reader.acquisition_index<failures:
                if world_change:self.status['world_generation']=2
                raise self.acquisition_error(acquisition)
            return certificate_probe.capture(nonce, True, 1).certificate
        self.enterContext(mock.patch('path_choice_position_collect.collect', side_effect=collect))
        self.m = self.create(probe_factory=LivePathChoicePositionProbe)
        self.addCleanup(self.m.close)
        return readers, certificate_probe

    def test_direct_final_read_conflict_reacquires_everything_with_new_reader(self):
        readers,collector=self.snapshot_retry_fixture(acquisition='final_reread')
        self.m.poll();nonce=self.m.nonce;events=self.m.poll()
        self.assertEqual(events[0]['event'],'path_position_acquisition_discarded')
        self.assertEqual(events[0]['reason'],'AcquisitionSourceChanged')
        self.assertEqual(events[0]['error_read_conflict']['changed_offset'],0x1d)
        self.assertEqual(self.actions,[]);self.assertEqual(len(readers),2)
        readers[0].close.assert_called_once();self.assertIsNot(readers[0],readers[1])
        self.hold();self.assertEqual(self.m.nonce,nonce);self.assertEqual(self.m.acquisition_attempts,2)
        self.assertEqual(collector.full_reads,1)
        self.assertEqual([x[1] for x in self.actions if x[0]=='publish'],['close','position'])

    def test_direct_repeated_read_conflict_uses_same_bounded_retry(self):
        readers,collector=self.snapshot_retry_fixture(acquisition='repeated_read')
        self.m.poll();events=self.m.poll()
        self.assertEqual(events[0]['event'],'path_position_acquisition_discarded')
        self.assertEqual(self.actions,[]);self.hold()
        self.assertEqual(len(readers),2);self.assertEqual(collector.full_reads,1)

    def test_mixed_snapshot_and_acquisition_conflicts_share_three_attempt_budget(self):
        readers,collector=self.snapshot_retry_fixture(failures=2,acquisition='final_reread',prior_snapshot=True)
        events=[]
        for _ in range(4):events.extend(self.m.poll())
        discarded=[e for e in events if e['event']=='path_position_acquisition_discarded']
        self.assertEqual([e['reason'] for e in discarded],['SceneOwnerRejected','AcquisitionSourceChanged'])
        self.assertEqual([e['retry_count'] for e in discarded],[1,2])
        self.assertEqual(len(readers),3);self.assertEqual(self.m.acquisition_attempts,3)
        self.hold();self.assertEqual(collector.full_reads,1)

    def test_mixed_budget_final_failure_is_terminal_without_any_publication(self):
        readers,collector=self.snapshot_retry_fixture(failures=10,acquisition='final_reread',prior_snapshot=True)
        events=[]
        for _ in range(6):events.extend(self.m.poll())
        self.assertEqual(len(readers),3);self.assertEqual(self.m.acquisition_attempts,3)
        self.assertTrue(self.m.terminal);self.assertEqual(self.actions,[]);self.assertEqual(collector.full_reads,0)
        self.assertEqual([e['retry_count'] for e in events if e['event']=='path_position_acquisition_discarded'],[1,2])
        self.assertEqual(events[-1]['event'],'path_position_revoked')
        for reader in readers:reader.close.assert_called_once()

    def test_prefix_and_unknown_acquisition_phases_are_terminal(self):
        for phase in ('fixed_prefix_header','unknown'):
            self.assertFalse(_acquisition_source_conflict(self.acquisition_error(phase)))
        readers,collector=self.snapshot_retry_fixture(acquisition='fixed_prefix_header')
        self.m.poll();self.m.poll()
        self.assertEqual(len(readers),1);self.assertTrue(self.m.terminal)
        self.assertEqual(self.actions,[]);self.assertEqual(collector.full_reads,0)

    def test_wrapped_subclassed_short_or_malformed_conflict_is_not_retried(self):
        cause=self.acquisition_error()
        wrapped=RuntimeError('wrapped');wrapped.__cause__=cause
        self.assertFalse(_acquisition_source_conflict(wrapped))
        cause.__cause__=ValueError('other failure');self.assertFalse(_acquisition_source_conflict(cause))
        cause=self.acquisition_error();cause.__context__=ValueError('other failure')
        self.assertFalse(_acquisition_source_conflict(cause))
        class Derived(AcquisitionSourceChanged):pass
        self.assertFalse(_acquisition_source_conflict(Derived('final_reread',1,b'a',b'b',[])))
        for field,value in (('actual_size',95),('changed_offset',96),('address','bad'),('changed_address','0x99'),
                            ('expected_window','zz'),('actual_window','')):
            error=self.acquisition_error();error.read_conflict[field]=value
            with self.subTest(field=field):self.assertFalse(_acquisition_source_conflict(error))

    def test_direct_source_conflict_cannot_retry_after_publication(self):
        readers,collector=self.snapshot_retry_fixture(failures=0,postpublish=True,acquisition='final_reread')
        self.hold();events=[]
        for _ in range(10):events.extend(self.step())
        self.assertEqual(len(readers),1);self.assertTrue(self.m.terminal)
        self.assertFalse(any(e['event']=='path_position_acquisition_discarded' for e in events))
        self.assertEqual(collector.full_reads,1)

    def test_direct_source_conflict_cannot_retry_across_world_change(self):
        readers,collector=self.snapshot_retry_fixture(acquisition='final_reread',world_change=True)
        self.m.poll();events=self.m.poll()
        self.assertEqual(len(readers),1);self.assertTrue(self.m.terminal)
        self.assertFalse(any(e['event']=='path_position_acquisition_discarded' for e in events))
        self.assertEqual(self.actions,[]);self.assertEqual(collector.full_reads,0)

    def test_direct_source_conflict_requires_clean_native_and_no_cancellation(self):
        readers,_=self.snapshot_retry_fixture(acquisition='final_reread')
        self.m.poll()
        for key in ('path_position_pending_restore','path_position_active_nonce','path_position_unresolved'):
            self.status[key]=1
            with self.subTest(key=key),self.assertRaisesRegex(RuntimeError,'Native position state changed before snapshot retry'):
                self.m._retry_unowned_snapshot(self.acquisition_error(),1)
            self.status[key]=0
        self.m.cancel.set()
        self.assertFalse(self.m._retry_unowned_snapshot(self.acquisition_error(),1))
        self.assertEqual(len(readers),1);self.assertEqual(self.actions,[])

    def test_registry_conflict_restarts_full_unowned_capture(self):
        readers, collector = self.snapshot_retry_fixture(registry=True)
        self.m.poll(); events=self.m.poll()
        self.assertEqual(events[0]['event'],'path_position_acquisition_discarded')
        self.assertEqual(self.actions,[])
        self.assertEqual(len(readers),2)
        readers[0].close.assert_called_once()
        self.hold()
        self.assertEqual(collector.full_reads,1)

    def test_registry_conflict_is_bounded(self):
        readers,_=self.snapshot_retry_fixture(failures=10,registry=True)
        for _ in range(6):self.m.poll()
        self.assertEqual(len(readers),3)
        self.assertTrue(self.m.terminal)
        self.assertEqual(self.actions,[])

    def test_registry_conflict_cannot_retry_across_world_change(self):
        readers,_=self.snapshot_retry_fixture(registry=True,world_change=True)
        self.m.poll();events=self.m.poll()
        self.assertEqual(len(readers),1)
        self.assertTrue(self.m.terminal)
        self.assertFalse(any(e['event']=='path_position_acquisition_discarded' for e in events))
        self.assertEqual(self.actions,[])

    def test_exact_nested_snapshot_conflict_discards_full_unowned_capture_with_fresh_reader(self):
        readers, collector = self.snapshot_retry_fixture()
        self.m.poll(); nonce = self.m.nonce
        events = self.m.poll()
        self.assertEqual([event['event'] for event in events], ['path_position_acquisition_discarded'])
        self.assertEqual(events[0]['retry_count'], 1)
        self.assertEqual(events[0]['error_causes'][0]['type'], 'SnapshotChanged')
        self.assertEqual(collector.full_reads, 1)
        self.assertEqual(self.actions, [])
        self.assertEqual(len(readers), 2)
        readers[0].close.assert_called_once()
        self.assertIsNot(readers[0], readers[1])
        self.hold()
        self.assertEqual(self.m.nonce, nonce)
        self.assertEqual(self.m.acquisition_attempts, 2)
        self.assertEqual([row[1] for row in self.actions if row[0] == 'publish'], ['close', 'position'])
        self.assertEqual(collector.full_reads, 1)

    def test_snapshot_acquisition_retry_bound_exhaustion_is_terminal_without_publication(self):
        readers, collector = self.snapshot_retry_fixture(failures=10)
        events = []
        for _ in range(6):events.extend(self.m.poll())
        self.assertEqual(len(readers), 3)
        self.assertEqual(self.m.acquisition_attempts, 3)
        self.assertTrue(self.m.terminal)
        self.assertEqual(self.actions, [])
        self.assertEqual(collector.full_reads, 0)
        self.assertEqual([e['retry_count'] for e in events if e['event'] == 'path_position_acquisition_discarded'], [1, 2])
        self.assertEqual(events[-1]['event'], 'path_position_revoked')
        self.assertEqual(events[-1]['retry_count'], 2)
        for reader in readers:reader.close.assert_called_once()

    def test_unknown_snapshot_cause_is_terminal_without_retry(self):
        readers, collector = self.snapshot_retry_fixture(unknown=True)
        self.m.poll(); events = self.m.poll()
        self.assertEqual(len(readers), 1)
        self.assertTrue(self.m.terminal)
        self.assertEqual(events[-1]['event'], 'path_position_revoked')
        self.assertEqual(self.actions, [])

    def test_snapshot_conflict_world_change_cannot_retry(self):
        readers, collector = self.snapshot_retry_fixture(world_change=True)
        self.m.poll(); events = self.m.poll()
        self.assertEqual(len(readers), 1)
        self.assertTrue(self.m.terminal)
        self.assertFalse(any(e['event'] == 'path_position_acquisition_discarded' for e in events))
        self.assertEqual(self.actions, [])

    def test_exact_snapshot_conflict_after_publication_is_terminal_without_retry(self):
        readers, collector = self.snapshot_retry_fixture(failures=0, postpublish=True)
        self.hold()
        events = []
        for _ in range(10):events.extend(self.step())
        self.assertEqual(len(readers), 1)
        self.assertTrue(self.m.terminal)
        self.assertFalse(self.m.position.owned)
        self.assertTrue(self.m.close_sidecar.owned)
        self.assertFalse(any(e['event'] == 'path_position_acquisition_discarded' for e in events))
        self.assertEqual(collector.full_reads, 1)

    def test_live_probe_holds_pending_restore_one_over_30s_with_native_poll_cadence(self):
        self.m.close()
        modules = self.live_probe_modules()
        certificate_probe = Probe()
        def collect(reader, sample, status, session, nonce, verify, *, registry_header_policy, target_vertical_region):
            self.assertEqual(registry_header_policy, 'fixed_prefix_v1')
            self.assertEqual(target_vertical_region, (.25, .75))
            verify(reader)
            return certificate_probe.capture(nonce, True, 1).certificate
        original_ack = self.ack; pending = []; published = dict(self.status)
        self.ack = lambda path, first=False:pending.append((path, first))
        with mock.patch.dict('sys.modules', modules), mock.patch('path_choice_position_collect.collect', side_effect=collect):
            self.m = self.create(probe_factory=LivePathChoicePositionProbe)
            self.addCleanup(self.m.close)
            self.m.read_status = lambda:dict(published)
            # Native file polling every 200 ms; published status every 1 s.
            for step in range(420):
                self.now = 10 + step / 10
                if step % 2 == 0:
                    queued, pending[:] = pending[:], []
                    for path, first in queued:original_ack(path, first)
                if step % 10 == 0:published.update(self.status)
                self.m.poll()
            self.assertEqual(self.m.state, 'holding')
            self.assertFalse(self.m.terminal)
            self.assertEqual(certificate_probe.full_reads, 1)
            self.assertEqual(self.fixed_prefix_sample.call_count, 2)
            self.assertTrue(self.fixed_prefix_sample.call_args.kwargs["include_owner"])
            self.assertEqual(published['path_position_pending_restore'], 1)
            renewals = [row[1] for row in self.actions if row[0] == 'renew']
            self.assertGreater(len(renewals), 10)
            self.assertEqual(renewals, ['close', 'position'] * (len(renewals) // 2))

    def test_live_probe_pending_lease_requires_owned_exact_unrestored_nonce(self):
        self.hold()
        with mock.patch.dict('sys.modules', self.live_probe_modules()):
            probe = LivePathChoicePositionProbe(GAME, self.m.verify_session)
            self.addCleanup(probe.close)
            self.assertTrue(probe.capture(self.m.nonce, False, 1).scene_present)
            with self.assertRaisesRegex(RuntimeError, 'state changed'):
                probe.capture(self.m.nonce, True, 1)
            for key, value in (
                    ('path_position_nonce', self.m.nonce + 1),
                    ('path_position_active_nonce', self.m.nonce + 1),
                    ('path_position_consumed_nonce', self.m.nonce + 1),
                    ('path_position_last_restored_nonce', self.m.nonce),
                    ('path_position_ownership_lost', 1),
                    ('world_generation', 2)):
                original = self.status[key]
                with self.subTest(key=key), self.assertRaises(RuntimeError):
                    self.status[key] = value
                    probe.capture(self.m.nonce, False, 1)
                self.status[key] = original
            with self.assertRaisesRegex(RuntimeError, 'state changed'):
                probe.capture(self.m.nonce + 1, False, 1)
            # Revocation removes position but retains the owned close guard while
            # read-only observation waits for native restoration.
            self.m.position.remove()
            self.assertTrue(probe.capture(self.m.nonce, False, 1).scene_present)
            self.m.close_sidecar.remove()
            with self.assertRaisesRegex(RuntimeError, 'state changed'):
                probe.capture(self.m.nonce, False, 1)

    def test_real_monitor_hung_worker_cannot_publish_after_bounded_close(self):
        self.m.close()
        entered = threading.Event(); released = threading.Event()
        class BlockedProbe(Probe):
            def capture(self, *args):
                entered.set(); released.wait(3)
                return super().capture(*args)
        self.m = self.create(worker_factory=_ReadWorker, probe_factory=BlockedProbe)
        self.addCleanup(self.m.close)
        self.m.poll(); self.assertTrue(entered.wait(1))
        before = time.monotonic(); self.m.close()
        self.assertLess(time.monotonic() - before, .6); self.assertFalse(self.m.worker_joined)
        released.set(); self.m.executor.thread.join(1)
        self.assertEqual(self.m.poll(), [])
        self.assertFalse(self.m.position.owned); self.assertFalse(self.m.close_sidecar.owned)

    def test_hung_real_read_worker_is_daemon_and_shutdown_is_bounded(self):
        released = threading.Event(); entered = threading.Event()
        probe = Probe(); worker = _ReadWorker(probe)
        def blocked():entered.set(); released.wait(3); return Observation(False)
        future = worker.submit(blocked)
        self.assertTrue(entered.wait(1))
        before = time.monotonic()
        self.assertFalse(worker.close(timeout=.02)); self.assertLess(time.monotonic() - before, .3)
        self.assertTrue(worker.thread.daemon); self.assertFalse(future.done())
        released.set(); worker.thread.join(1); self.assertTrue(probe.closed)


if __name__ == '__main__':unittest.main()
