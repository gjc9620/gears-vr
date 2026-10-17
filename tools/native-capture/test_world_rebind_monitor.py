import threading
import time
import unittest
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

    def test_late_generation_with_foreign_request_does_not_update_baseline(self):
        h = Harness()
        with h.monitor() as m:
            h.prepared(m)
            h.status.update(world_generation='2', world_prepare_requests='99')
            self.assertEqual('world_rebind_blocked', m.poll(h.status)[0]['event'])
            self.assertEqual(snapshot(), m.baseline)


if __name__ == '__main__':
    unittest.main()
