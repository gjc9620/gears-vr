import copy
import pathlib
import tempfile
import unittest

from tiny_output_recovery import attempt_recovery, SWP_FLAGS, StartupTinyOutputPolicy


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
    def test_startup_policy_requires_persistent_tiny_before_stereo(self):
        policy = StartupTinyOutputPolicy()
        policy.update({'gpu_source_size': [8, 8]}, 10)
        self.assertFalse(policy.ready(11.99))
        self.assertTrue(policy.ready(12))
        self.assertFalse(policy.ready(12, stereo_submitted=True))
        policy.consumed = True
        self.assertFalse(policy.ready(13))

    def test_normal_size_retirement_and_stereo_invalidate_tiny(self):
        for event in ({'gpu_source_size': [1920, 1080]}, {'gpu_source_size': [8, 9]},
                      {'gpu_source_size': None}, {'stereo_source_retired': True},
                      {'reference_epoch_first_projection': True, 'end_frame_succeeded': True}):
            policy = StartupTinyOutputPolicy()
            policy.update({'gpu_source_size': [8, 8]}, 10)
            policy.update(event, 11)
            self.assertFalse(policy.ready(13))
        policy = StartupTinyOutputPolicy()
        policy.update({'gpu_source_size': [8, 8]}, 10)
        policy.update({'stereo_source_retired': True}, 11)
        policy.update({'gpu_source_size': [8, 8]}, 12)
        self.assertFalse(policy.ready(13))
        self.assertTrue(policy.ready(14))

    def test_reader_only_processes_new_complete_lines_and_bounds_history(self):
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 'xr.txt'
            path.write_bytes(b'{"gpu_source_size":[8,8]}')
            policy = StartupTinyOutputPolicy()
            policy.read_new(path, 10)
            self.assertFalse(policy.ready(12))
            with path.open('ab') as stream: stream.write(b'\n')
            policy.read_new(path, 12)
            self.assertTrue(policy.ready(14))
            policy.read_new(path, 15)
            self.assertEqual(policy.tiny_since, 12)
            with path.open('ab') as stream: stream.write(b'x' * 65537)
            policy.read_new(path, 16)
            self.assertFalse(policy.ready(20))

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
