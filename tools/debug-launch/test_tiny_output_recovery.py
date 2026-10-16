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

    def test_real_output_requires_two_seconds_of_distinct_fresh_samples(self):
        policy = SwapchainTinyOutputPolicy(IDENTITY)
        self.assertFalse(self.observe(policy, 10)['ready'])
        self.assertFalse(self.observe(policy, 11)['ready'])
        self.assertTrue(self.observe(policy, 12, phase='running')['ready'])
        self.assertEqual(policy.first_ns, 10_000_000_000)

    def test_repeated_sample_does_not_advance_and_stale_resets(self):
        policy = SwapchainTinyOutputPolicy(IDENTITY)
        fields = self.status(10)
        policy.observe(fields, 10_000_000_000)
        self.assertEqual(policy.observe(fields, 12_000_000_000)['reason'], 'duplicate_sample')
        self.assertEqual(policy.observe(fields, 12_000_000_001)['reason'], 'status_stale_or_future')
        self.assertFalse(self.observe(policy, 13)['ready'])
        self.assertFalse(self.observe(policy, 14)['ready'])
        self.assertTrue(self.observe(policy, 15)['ready'])

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
                self.assertFalse(self.observe(policy, 12)['ready'])
                self.assertFalse(self.observe(policy, 13)['ready'])
                self.assertTrue(self.observe(policy, 14)['ready'])

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
        for second in (10, 11):
            with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(second)):
                self.assertEqual(monitor.poll_once(control, second * 1_000_000_000)['event'], 'observation')
        with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(12)):
            event = monitor.poll_once(control, 12_000_000_000)
        self.assertEqual(event['event'], 'recovery_attempt')
        self.assertEqual(event['result']['result'], 'diagnostic_error')
        with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(13, width=1920, height=1080)):
            event = monitor.poll_once(control, 13_000_000_000)
        self.assertEqual(event['dimensions'], [1920, 1080])
        self.assertFalse(event['ready'])
        monitor.attempt.assert_called_once_with(control, IDENTITY, 99, [8, 8])
        control.identity = lambda pid: dict(IDENTITY, creation_time=999)
        self.assertTrue(monitor.poll_once(control, 14_000_000_000)['stop'])

    def test_hidden_window_defers_and_visible_window_reacquires_before_one_attempt(self):
        monitor = TinyOutputMonitor('unused', 'unused', IDENTITY,
                                    attempt=mock.Mock(return_value=dict(attempted=True)))
        control = FakeControl()
        control.state['visible'] = False
        def poll(second):
            with mock.patch('tiny_output_recovery.read_swapchain_status', return_value=self.status(second)):
                return monitor.poll_once(control, second * 1_000_000_000)
        poll(10); poll(11)
        deferred = poll(12)
        self.assertEqual(deferred['event'], 'recovery_deferred')
        self.assertEqual(deferred['reason'], 'window_not_visible_normal_windowed')
        self.assertFalse(monitor.policy.consumed)
        monitor.attempt.assert_not_called()
        control.state['visible'] = True
        self.assertEqual(poll(13)['event'], 'observation')
        self.assertEqual(poll(14)['event'], 'observation')
        self.assertEqual(poll(15)['event'], 'recovery_attempt')
        self.assertTrue(monitor.policy.consumed)
        poll(16); poll(17)
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
