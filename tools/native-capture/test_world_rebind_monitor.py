import threading
import time
import unittest
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
