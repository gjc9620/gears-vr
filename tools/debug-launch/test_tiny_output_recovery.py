import copy
import json
import pathlib
import tempfile
import threading
import unittest
from types import SimpleNamespace
from unittest import mock

from tiny_output_recovery import attempt_recovery, skip_reason, SWP_FLAGS, SwapchainTinyOutputPolicy, TinyOutputMonitor, read_swapchain_status, WindowsControl


IDENTITY = {'pid': 123, 'creation_time': 456, 'image': 'j:/game/gowde-steam.exe'}


class FakeControl:
    def __init__(self):
        self.state = {'identity': dict(IDENTITY), 'window_pid': 123,
                      'visible': True, 'minimized': False, 'maximized': False,
                      'windowed': True, 'rect': [10, 20, 1943, 1136],
                      'client_size': [1920, 1080]}
        self.calls = []
        self.after_first = None
        self.fail = False

    def foreground(self):
        return 789028

    def identity(self, pid):
        return dict(IDENTITY)

    def snapshot(self, pid, hwnd):
        return copy.deepcopy(self.state)

    def resize(self, hwnd, width, height):
        self.calls.append((hwnd, width, height))
        if self.fail:
            return False
        self.state['rect'][2:] = [self.state['rect'][0] + width, self.state['rect'][1] + height]
        if len(self.calls) == 1 and self.after_first:
            self.after_first(self.state)
        return True


class TinyOutputRecoveryTests(unittest.TestCase):
    @staticmethod
    def status(seconds, **changes):
        fields = dict(schema=1, pid=123, creation_time=456, phase='prime', valid=1,
                      candidates=1, hwnd=99, chain=123456, width=8, height=8, windowed=1,
                      qpc=int(seconds * 10_000_000), qpc_frequency=10_000_000, complete=1)
        fields.update(changes)
        return fields

    def observe(self, policy, seconds, **changes):
        return policy.observe(self.status(seconds, **changes), int(seconds * 1_000_000_000))

    def test_real_output_requires_five_seconds_of_distinct_fresh_samples(self):
        policy = SwapchainTinyOutputPolicy(IDENTITY)
        for second in (10, 11, 12, 13, 14, 14.9999999):
            self.assertFalse(self.observe(policy, second)['ready'])
        # Wall time at the boundary with an unchanged QPC is insufficient.
        duplicate = policy.observe(self.status(14.9999999), 15_000_000_000)
        self.assertEqual(duplicate['reason'], 'duplicate_sample')
        self.assertFalse(duplicate['ready'])
        self.assertTrue(self.observe(policy, 15, phase='running')['ready'])
        self.assertEqual(policy.first_ns, 10_000_000_000)

    def test_repeated_sample_does_not_advance_and_stale_resets(self):
        policy = SwapchainTinyOutputPolicy(IDENTITY)
        fields = self.status(10)
        policy.observe(fields, 10_000_000_000)
        self.assertEqual(policy.observe(fields, 12_000_000_000)['reason'], 'duplicate_sample')
        self.assertEqual(policy.observe(fields, 12_000_000_001)['reason'], 'status_stale_or_future')
        for second in (13, 14, 15, 16, 17):
            self.assertFalse(self.observe(policy, second)['ready'])
        self.assertTrue(self.observe(policy, 18)['ready'])

    def test_invalid_identity_size_and_output_replacement_reacquire(self):
        cases = [dict(pid=124), dict(creation_time=457), dict(width=1920, height=1080),
                 dict(width=8, height=9), dict(valid=0), dict(candidates=2),
                 dict(phase='stopped'), dict(hwnd=100), dict(chain=987654), dict(width=0),
                 dict(windowed=0), dict(windowed=True)]
        for changes in cases:
            with self.subTest(changes=changes):
                policy = SwapchainTinyOutputPolicy(IDENTITY)
                self.observe(policy, 10)
                self.assertFalse(self.observe(policy, 11, **changes)['ready'])
                for second in (12, 13, 14, 15, 16):
                    self.assertFalse(self.observe(policy, second)['ready'])
                self.assertTrue(self.observe(policy, 17)['ready'])

    def test_missing_dxgi_windowed_field_never_authorizes_old_schema(self):
        policy = SwapchainTinyOutputPolicy(IDENTITY)
        fields = self.status(10)
        del fields['windowed']
        self.assertEqual(policy.observe(fields, 10_000_000_000)['reason'], 'dxgi_not_attested_windowed')

    def test_decorated_normal_window_can_extend_beyond_monitor_bounds(self):
        control = WindowsControl.__new__(WindowsControl)
        control.identity = lambda pid: dict(IDENTITY)
        def owner(hwnd, pointer):
            pointer._obj.value = 123
        def rectangle(values):
            def read(hwnd, pointer):
                rect = pointer._obj
                rect.left, rect.top, rect.right, rect.bottom = values
                return True
            return read
        control.u = SimpleNamespace(IsWindow=lambda hwnd: True, GetWindowThreadProcessId=owner,
            GetWindowRect=rectangle((0, 0, 1933, 1116)), GetClientRect=rectangle((0, 0, 1920, 1080)),
            GetWindowLongW=lambda hwnd, index: 0x14cb0000, IsWindowVisible=lambda hwnd: True,
            IsIconic=lambda hwnd: False, IsZoomed=lambda hwnd: False,
            GetMonitorInfoW=mock.Mock(side_effect=AssertionError('Monitor coverage cannot identify DXGI fullscreen')))
        snapshot = control.snapshot(123, 99)
        self.assertTrue(snapshot['windowed'])
        self.assertEqual(snapshot['rect'], [0, 0, 1933, 1116])
        self.assertIsNone(skip_reason([8, 8], IDENTITY, snapshot))
        control.u.GetMonitorInfoW.assert_not_called()
        for style in (0x14000000, 0x14c00000, 0x14080000):
            control.u.GetWindowLongW = lambda hwnd, index: style
            self.assertFalse(control.snapshot(123, 99)['windowed'])

    def test_missing_samples_and_backward_qpc_do_not_count_as_continuity(self):
        policy = SwapchainTinyOutputPolicy(IDENTITY)
        self.observe(policy, 10)
        self.assertFalse(self.observe(policy, 13)['ready'])
        self.assertFalse(self.observe(policy, 12)['ready'])
        self.assertFalse(self.observe(policy, 13)['ready'])

    def test_reader_bounds_and_duplicate_fields(self):
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 'status.txt'
            path.write_text(' '.join(f'{key}={value}' for key, value in self.status(10).items()))
            self.assertEqual(read_swapchain_status(path), self.status(10))
            path.write_text('schema=1 schema=1')
            with self.assertRaises(ValueError): read_swapchain_status(path)
            path.write_bytes(b'x' * 4097)
            with self.assertRaises(ValueError): read_swapchain_status(path)

    def test_monitor_attempts_only_once_even_on_error_and_keeps_followup_observations(self):
        monitor = TinyOutputMonitor('unused', 'unused', IDENTITY, attempt=mock.Mock(side_effect=RuntimeError('resize diagnostic')))
        control = FakeControl()
        for second in (10, 11, 12, 13, 14):
            with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(second)):
                self.assertEqual(monitor.poll_once(control, second * 1_000_000_000)['event'], 'observation')
        with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(15)):
            event = monitor.poll_once(control, 15_000_000_000)
        self.assertEqual(event['event'], 'recovery_attempt')
        self.assertEqual(event['result']['result'], 'diagnostic_error')
        with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(16, width=1920, height=1080)):
            event = monitor.poll_once(control, 16_000_000_000)
        self.assertEqual(event['dimensions'], [1920, 1080])
        self.assertFalse(event['ready'])
        monitor.attempt.assert_called_once_with(control, IDENTITY, 99, [8, 8])
        control.identity = lambda pid: dict(IDENTITY, creation_time=999)
        self.assertTrue(monitor.poll_once(control, 17_000_000_000)['stop'])

    def poll_status(self, monitor, control, second, value):
        kwargs = {'side_effect': value} if isinstance(value, Exception) else {'return_value': value}
        with mock.patch('tiny_output_recovery.read_swapchain_status', **kwargs):
            return monitor.poll_once(control, int(second * 1_000_000_000))

    def test_normal_startup_two_and_half_second_tiny_transient_recovers_without_action(self):
        monitor = TinyOutputMonitor('unused', 'unused', IDENTITY, attempt=mock.Mock())
        control = FakeControl()
        control.snapshot = mock.Mock(side_effect=AssertionError('Transient must not inspect window'))
        # Recorded startup shape: normal DXGI output, about 2.5s at 8x8,
        # then natural restoration to 2534x1009. Times are synthetic.
        sequence = [(9.5, 2534, 1009)] + [(10 + index * .5, 8, 8) for index in range(6)] + [
            (12.6, 2534, 1009), (13.5, 2534, 1009)]
        for second, width, height in sequence:
            event = self.poll_status(monitor, control, second,
                self.status(second, width=width, height=height))
            self.assertEqual(event['event'], 'observation')
            self.assertFalse(event['ready'])
        self.assertEqual(event['reason'], 'output_not_tiny')
        self.assertIsNone(monitor.policy.first_ns)
        self.assertFalse(monitor.policy.consumed)
        monitor.attempt.assert_not_called()
        control.snapshot.assert_not_called()
        self.assertEqual(control.calls, [])

    def test_monitor_five_second_boundary_requires_advancing_fresh_output_sample(self):
        attempt = mock.Mock(return_value=dict(attempted=True))
        monitor = TinyOutputMonitor('unused', 'unused', IDENTITY, attempt=attempt)
        control = FakeControl()
        for second in (10, 11, 12, 13, 14, 14.9):
            event = self.poll_status(monitor, control, second, self.status(second))
            self.assertEqual(event['reason'], 'observing_tiny')
            attempt.assert_not_called()
        event = self.poll_status(monitor, control, 15, self.status(14.9))
        self.assertEqual(event['reason'], 'duplicate_sample')
        attempt.assert_not_called()
        event = self.poll_status(monitor, control, 15, self.status(15))
        self.assertEqual(event['event'], 'recovery_attempt')
        self.assertEqual(event['observation']['observed_tiny_seconds'], 5)
        attempt.assert_called_once_with(control, IDENTITY, 99, [8, 8])

    def test_transient_permission_errors_need_next_distinct_fresh_sample_and_attempt_once(self):
        attempt = mock.Mock(return_value=dict(attempted=True))
        monitor = TinyOutputMonitor('unused', 'unused', IDENTITY, attempt=attempt)
        control = FakeControl()
        self.poll_status(monitor, control, 10, self.status(10))
        gap = self.poll_status(monitor, control, 10.5, PermissionError('atomic replacement'))
        self.assertTrue(gap['continuity_retained'])
        attempt.assert_not_called()
        self.poll_status(monitor, control, 11, self.status(11))
        gap = self.poll_status(monitor, control, 12, PermissionError('atomic replacement'))
        self.assertTrue(gap['continuity_retained'])
        attempt.assert_not_called()  # Wall time alone cannot authorize recovery.
        duplicate = self.poll_status(monitor, control, 12.1, self.status(11))
        self.assertEqual(duplicate['reason'], 'duplicate_sample')
        attempt.assert_not_called()
        for second in (12.2, 13.2, 14.2):
            event = self.poll_status(monitor, control, second, self.status(second))
            self.assertEqual(event['event'], 'observation')
            attempt.assert_not_called()
        event = self.poll_status(monitor, control, 15.2, self.status(15.2))
        self.assertEqual(event['event'], 'recovery_attempt')
        self.assertAlmostEqual(event['observation']['observed_tiny_seconds'], 5.2)
        self.poll_status(monitor, control, 15.5, PermissionError('atomic replacement'))
        self.poll_status(monitor, control, 16, self.status(16))
        attempt.assert_called_once_with(control, IDENTITY, 99, [8, 8])

    def test_prolonged_unreadable_or_fresh_sample_gap_resets_continuity(self):
        for gap_time, fresh_time in ((12.000000001, 12.2), (11.5, 13)):
            with self.subTest(gap_time=gap_time):
                monitor = TinyOutputMonitor('unused', 'unused', IDENTITY, attempt=mock.Mock())
                control = FakeControl()
                self.poll_status(monitor, control, 10, self.status(10))
                gap = self.poll_status(monitor, control, gap_time, PermissionError('busy'))
                self.assertEqual(gap['continuity_retained'], gap_time <= 12)
                fresh = self.poll_status(monitor, control, fresh_time, self.status(fresh_time))
                self.assertFalse(fresh['ready'])
                self.assertEqual(fresh['observed_tiny_seconds'], 0)
                monitor.attempt.assert_not_called()

    def test_parse_identity_and_output_errors_reset_even_after_brief_io_gap(self):
        cases = (ValueError('malformed'), UnicodeError('not ascii'),
                 self.status(11, creation_time=999), self.status(11, width=1920),
                 self.status(11, windowed=0), self.status(11, valid=0))
        for invalid in cases:
            with self.subTest(invalid=invalid):
                monitor = TinyOutputMonitor('unused', 'unused', IDENTITY, attempt=mock.Mock())
                control = FakeControl()
                self.poll_status(monitor, control, 10, self.status(10))
                self.poll_status(monitor, control, 10.5, PermissionError('busy'))
                self.poll_status(monitor, control, 11, invalid)
                fresh = self.poll_status(monitor, control, 12, self.status(12))
                self.assertEqual(fresh['observed_tiny_seconds'], 0)
                monitor.attempt.assert_not_called()
        control.identity = lambda pid: dict(IDENTITY, creation_time=999)
        with mock.patch('tiny_output_recovery.read_swapchain_status') as read:
            self.assertTrue(monitor.poll_once(control, 13_000_000_000)['stop'])
            read.assert_not_called()
        self.assertIsNone(monitor.policy.last_ns)

    def test_window_guard_after_io_gap_still_requires_new_full_interval(self):
        monitor = TinyOutputMonitor('unused', 'unused', IDENTITY, attempt=mock.Mock())
        control = FakeControl()
        self.poll_status(monitor, control, 10, self.status(10))
        self.poll_status(monitor, control, 11, self.status(11))
        self.poll_status(monitor, control, 11.5, PermissionError('busy'))
        for second in (12, 13, 14):
            self.assertEqual(self.poll_status(monitor, control, second, self.status(second))['event'], 'observation')
        control.state['window_pid'] = 124
        event = self.poll_status(monitor, control, 15, self.status(15))
        self.assertEqual(event['event'], 'recovery_deferred')
        self.assertEqual(event['reason'], 'identity_or_window_changed')
        self.assertFalse(monitor.policy.consumed)
        monitor.attempt.assert_not_called()
        control.state['window_pid'] = 123
        event = self.poll_status(monitor, control, 16, self.status(16))
        self.assertEqual(event['observed_tiny_seconds'], 0)

    def test_hidden_window_defers_and_visible_window_reacquires_before_one_attempt(self):
        monitor = TinyOutputMonitor('unused', 'unused', IDENTITY,
                                    attempt=mock.Mock(return_value=dict(attempted=True)))
        control = FakeControl()
        control.state['visible'] = False
        def poll(second):
            with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(second)):
                return monitor.poll_once(control, second * 1_000_000_000)
        for second in (10, 11, 12, 13, 14):
            self.assertEqual(poll(second)['event'], 'observation')
        deferred = poll(15)
        self.assertEqual(deferred['event'], 'recovery_deferred')
        self.assertEqual(deferred['reason'], 'window_not_visible_normal_windowed')
        self.assertFalse(monitor.policy.consumed)
        monitor.attempt.assert_not_called()
        control.state['visible'] = True
        for second in (16, 17, 18, 19, 20):
            self.assertEqual(poll(second)['event'], 'observation')
        self.assertEqual(poll(21)['event'], 'recovery_attempt')
        self.assertTrue(monitor.policy.consumed)
        poll(22); poll(23)
        monitor.attempt.assert_called_once_with(control, IDENTITY, 99, [8, 8])

    def test_background_monitor_isolates_errors_and_closes_with_diagnostics(self):
        with tempfile.TemporaryDirectory() as temp:
            diagnostic = pathlib.Path(temp) / 'diagnostic.jsonl'
            observed = threading.Event()
            class BrokenControl:
                def identity(self, pid):
                    observed.set()
                    raise RuntimeError('isolated observer fault')
            monitor = TinyOutputMonitor('unused', diagnostic, IDENTITY,
                                        control_factory=BrokenControl, interval=.01).start()
            self.assertTrue(observed.wait(1))
            self.assertTrue(monitor.close())
            events = [json.loads(line) for line in diagnostic.read_text().splitlines()]
            self.assertIn('observer_error', [event['event'] for event in events])
            self.assertEqual(events[-1]['event'], 'monitor_stopped')

    def ready_monitor(self, attempt=None):
        monitor = TinyOutputMonitor('unused', 'unused', IDENTITY, attempt=attempt)
        for second in (10, 11, 12, 13, 14):
            self.observe(monitor.policy, second)
        return monitor

    def test_observe_only_does_not_inspect_or_resize_window_when_tiny_is_confirmed(self):
        monitor=self.ready_monitor(attempt=mock.Mock())
        monitor.allow_recovery=False
        control=FakeControl();control.snapshot=mock.Mock(side_effect=AssertionError('Window must not be touched'))
        with mock.patch('tiny_output_recovery.read_swapchain_status',return_value=self.status(15)):
            event=monitor.poll_once(control,15_000_000_000)
        self.assertEqual(event['reason'],'recovery_disabled')
        self.assertFalse(monitor.policy.consumed)
        monitor.attempt.assert_not_called();control.snapshot.assert_not_called()
        self.assertEqual(control.calls,[])

    def test_default_attempt_emits_each_operation_boundary_without_window_details(self):
        monitor, control, stages = self.ready_monitor(), FakeControl(), []
        with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(15)):
            event = monitor.poll_once(control, 15_000_000_000, emit=stages.append)
        self.assertEqual(event['result']['result'], 'resize_restore_attempted')
        self.assertEqual([row['phase'] for row in stages], [
            'window_snapshot_begin', 'window_snapshot_end', 'recovery_action_begin',
            'attempt_snapshot_before_begin', 'attempt_snapshot_before_end', 'resize_begin', 'resize_end',
            'attempt_snapshot_after_resize_begin', 'attempt_snapshot_after_resize_end', 'restore_begin', 'restore_end',
            'attempt_snapshot_after_restore_begin', 'attempt_snapshot_after_restore_end', 'recovery_action_complete'])
        for row in stages:
            self.assertEqual(set(row), {'event', 'phase', 'target_identity', 'hwnd'})
            self.assertEqual(row['target_identity'], IDENTITY)
            self.assertEqual(row['hwnd'], 99)
        self.assertEqual(SWP_FLAGS, 0x216)

    def test_custom_attempt_retains_its_four_positional_arguments_with_phase_emitter(self):
        attempt = mock.Mock(return_value={'attempted':True})
        monitor, control, stages = self.ready_monitor(attempt), FakeControl(), []
        with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(15)):
            result = monitor.poll_once(control, 15_000_000_000, emit=stages.append)
        attempt.assert_called_once_with(control, IDENTITY, 99, [8, 8])
        self.assertEqual(result['result'], {'attempted':True})
        self.assertEqual([row['phase'] for row in stages],
            ['window_snapshot_begin', 'window_snapshot_end', 'recovery_action_begin', 'recovery_action_complete'])

    def test_resize_exception_retains_last_begin_without_false_end(self):
        for error in (OSError('resize failed'), RuntimeError('custom resize fault')):
            with self.subTest(error=type(error).__name__):
                monitor, control, stages = self.ready_monitor(), FakeControl(), []
                with mock.patch.object(control, 'resize', side_effect=error) as resize, \
                        mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(15)):
                    result = monitor.poll_once(control, 15_000_000_000, emit=stages.append)
                phases = [row['phase'] for row in stages]
                self.assertEqual([phase for phase in phases if phase.endswith('_begin')][-1], 'resize_begin')
                self.assertNotIn('resize_end', phases)
                self.assertNotIn('restore_begin', phases)
                self.assertEqual(phases[-1], 'recovery_action_complete')
                self.assertIn(result['result']['result'], ('error', 'diagnostic_error'))
                resize.assert_called_once_with(99, 1932, 1116)

    def test_phase_callback_failure_does_not_change_recovery_result(self):
        expected = self.run_attempt(FakeControl())
        failing = mock.Mock(side_effect=OSError('phase record unavailable'))
        actual = attempt_recovery(FakeControl(), IDENTITY, 99, (8, 8), emit=failing)
        self.assertEqual(actual, expected)
        self.assertEqual(failing.call_count, 10)

    def test_run_phase_record_failure_does_not_block_custom_recovery_or_stop_record(self):
        with tempfile.TemporaryDirectory() as temp:
            records, failed_stages = [], []
            class StageFailingStream:
                def __enter__(self):return self
                def __exit__(self, *args):pass
                def write(self, value):
                    row = json.loads(value)
                    if row['event'] == 'recovery_stage':
                        failed_stages.append(row['phase'])
                        raise OSError('stage-only storage fault')
                    records.append(row)
                def flush(self):pass
            monitor = self.ready_monitor()
            monitor.diagnostic_path = pathlib.Path(temp)/'diagnostic.jsonl'
            control = FakeControl()
            def custom(c, identity, hwnd, output):
                monitor.stop_event.set()
                return {'attempted':True}
            monitor.attempt = mock.Mock(side_effect=custom)
            monitor.control_factory = lambda:control
            with mock.patch.object(pathlib.Path, 'open', return_value=StageFailingStream()), \
                    mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(15)), \
                    mock.patch('tiny_output_recovery.time.perf_counter_ns', return_value=15_000_000_000):
                monitor._run()
            monitor.attempt.assert_called_once_with(control, IDENTITY, 99, [8, 8])
            self.assertEqual(len(failed_stages), 4)
            self.assertIn('recovery_attempt', [row['event'] for row in records])
            self.assertEqual(records[-1]['event'], 'monitor_stopped')

    def run_attempt(self, control, output=(8, 8)):
        return attempt_recovery(control, IDENTITY, 99, output)

    def test_nonactivating_resize_and_restore_reports_attempt_only(self):
        control = FakeControl()
        result = self.run_attempt(control)
        self.assertEqual(control.calls, [(99, 1932, 1116), (99, 1933, 1116)])
        self.assertEqual(result['result'], 'resize_restore_attempted')
        self.assertTrue(result['restored'])
        self.assertEqual(result['foreground_before'], result['foreground_after'])
        self.assertEqual(SWP_FLAGS, 0x216)
        self.assertNotIn('fixed', result)

    def test_requires_positive_explicit_tiny_dimensions(self):
        for output in (None, (), (0, 8), (-1, 8), (8, 9), (1920, 1080), (True, 8), ('8', 8)):
            with self.subTest(output=output):
                control = FakeControl()
                result = self.run_attempt(control, output)
                self.assertFalse(result['attempted'])
                self.assertEqual(control.calls, [])

    def test_identity_and_window_guards(self):
        changes = [ {'identity': None}, {'window_pid': 124}, {'visible': False},
                   {'minimized': True}, {'maximized': True}, {'windowed': False},
                   {'client_size': [639, 1080]}, {'client_size': [1920, 359]} ]
        for change in changes:
            with self.subTest(change=change):
                control = FakeControl()
                control.state.update(change)
                self.assertFalse(self.run_attempt(control)['attempted'])
                self.assertEqual(control.calls, [])
        control = FakeControl()
        control.state['identity']['creation_time'] += 1
        self.assertFalse(self.run_attempt(control)['attempted'])

    def test_restore_does_not_overwrite_user_or_identity_changes(self):
        changes = [lambda state: state.update(rect=[10, 20, 1600, 900]),
                   lambda state: state.update(rect=[11, 20, 1943, 1136]),
                   lambda state: state.update(identity=None),
                   lambda state: state.update(window_pid=321),
                   lambda state: state.update(windowed=False),
                   lambda state: state.update(maximized=True)]
        for change in changes:
            with self.subTest(change=change):
                control = FakeControl()
                control.after_first = change
                result = self.run_attempt(control)
                self.assertEqual(len(control.calls), 1)
                self.assertFalse(result['restored'])
                self.assertEqual(result['result'], 'restore_skipped')

    def test_failed_resize_does_not_issue_second_resize(self):
        control = FakeControl()
        control.fail = True
        result = self.run_attempt(control)
        self.assertFalse(result['resized'])
        self.assertFalse(result['restored'])
        self.assertEqual(len(control.calls), 1)

    def test_missing_hwnd_snapshot_is_safe(self):
        control = FakeControl()
        control.state = None
        self.assertFalse(self.run_attempt(control)['attempted'])
        self.assertEqual(control.calls, [])


if __name__ == '__main__':
    unittest.main()
