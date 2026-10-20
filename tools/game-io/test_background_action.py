"""Mock-only checks: no virtual controller or real game input is created."""
import contextlib
import json
import pathlib
import tempfile
import unittest
from unittest import mock

import background_input
import game_io
from test_game_io import FakePad


class BackgroundActionTests(unittest.TestCase):
    def setUp(self):
        self.pad = FakePad()
        self.identity = {'pid': 42, 'creation_time': 123, 'image': str(game_io.GAME)}
        self.request = {'background': True, 'pid': 42, 'creation_time': 123,
                        'right': [0.6, 0], 'seconds': 0.02}
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.window = stack.enter_context(mock.patch.object(game_io, 'game_window', return_value=(42, 7)))
        stack.enter_context(mock.patch.object(game_io, 'interrupted', return_value=False))
        self.focus = stack.enter_context(mock.patch.object(game_io, 'focused', side_effect=AssertionError('focus queried')))
        self.activate = stack.enter_context(mock.patch.object(game_io.user, 'SetForegroundWindow'))
        self.show = stack.enter_context(mock.patch.object(game_io.user, 'ShowWindow'))
        self.status = stack.enter_context(mock.patch.object(background_input, 'change',
            return_value={'state': 'enabled', 'identity': self.identity}))
        self.control = stack.enter_context(mock.patch.object(background_input, 'WindowsControl')).return_value
        self.control.identity.return_value = self.identity
        stack.enter_context(mock.patch.object(game_io.time, 'sleep'))
        self.clock = stack.enter_context(mock.patch.object(game_io.time, 'monotonic',
            side_effect=[0, 0.01, 0.03, 0.04]))

    def perform(self, **overrides):
        return game_io.perform(self.pad, None, dict(self.request, **overrides))

    def assert_neutral_only(self):
        self.assertEqual(self.pad.sent, [True])

    def test_enabled_background_action_does_not_query_or_activate_focus(self):
        result = self.perform()
        self.assertTrue(result['background'])
        self.assertEqual(result['creation_time'], 123)
        self.assertEqual(self.pad.sent, [False, True])
        self.status.assert_called_once_with(42, 'status', expected_creation=123)
        self.assertGreaterEqual(self.control.identity.call_count, 2)
        self.focus.assert_not_called()
        self.activate.assert_not_called()
        self.show.assert_not_called()

    def test_exact_authorization_is_required(self):
        for overrides in ({'pid': None}, {'creation_time': None}, {'pid': 9}, {'wait_focus': 1}):
            with self.subTest(overrides=overrides):
                self.pad.sent.clear()
                with self.assertRaises((ValueError, RuntimeError)):
                    self.perform(**overrides)
                self.assert_neutral_only()
        self.status.assert_not_called()

    def test_reused_pid_rejection_is_neutral(self):
        self.status.side_effect = RuntimeError('Process creation identity changed; refusing reused PID')
        with self.assertRaisesRegex(RuntimeError, 'reused PID'):
            self.perform()
        self.assert_neutral_only()

    def test_disabled_unknown_or_mismatched_status_refuses_input(self):
        for report in ({'state': 'original', 'identity': self.identity},
                       {'state': 'unknown', 'identity': self.identity},
                       {'state': 'enabled', 'identity': dict(self.identity, creation_time=999)},
                       {'state': 'enabled', 'identity': dict(self.identity, pid=99)}):
            with self.subTest(report=report):
                self.pad.sent.clear()
                self.status.return_value = report
                with self.assertRaisesRegex(RuntimeError, 'not verified enabled'):
                    self.perform()
                self.assert_neutral_only()

    def test_stop_before_status_refuses_input(self):
        with mock.patch.object(game_io, 'interrupted', return_value=True):
            with self.assertRaisesRegex(RuntimeError, 'stop request'):
                self.perform()
        self.status.assert_not_called()
        self.assert_neutral_only()

    def test_window_change_before_update_refuses_input(self):
        self.window.side_effect = [(42, 7), (42, 8)]
        with self.assertRaisesRegex(RuntimeError, 'identity changed'):
            self.perform()
        self.assert_neutral_only()

    def test_creation_change_during_hold_neutralizes(self):
        self.control.identity.side_effect = [self.identity, dict(self.identity, creation_time=999)]
        with self.assertRaisesRegex(RuntimeError, 'identity changed'):
            self.perform()
        self.assertEqual(self.pad.sent, [False, True])

    def test_stop_during_hold_neutralizes(self):
        with mock.patch.object(game_io, 'interrupted', side_effect=[False, False, True]):
            with self.assertRaisesRegex(RuntimeError, 'stop request'):
                self.perform()
        self.assertEqual(self.pad.sent, [False, True])

    def test_staging_failure_neutralizes(self):
        with mock.patch.object(self.pad, 'right_joystick_float', side_effect=RuntimeError('device failure')):
            with self.assertRaisesRegex(RuntimeError, 'device failure'):
                self.perform()
        self.assert_neutral_only()

    def test_old_service_rejects_background_before_queueing(self):
        with tempfile.TemporaryDirectory() as directory:
            state = pathlib.Path(directory)
            (state / 'server.json').write_text(json.dumps({'pid': 9, 'state': 'ready'}))
            with mock.patch.object(game_io, 'STATE', state), mock.patch.object(game_io.sys, 'argv',
                    ['game_io.py', 'action', '--background', '--pid', '42', '--creation-time', '123']):
                with self.assertRaisesRegex(RuntimeError, 'stop and restart'):
                    game_io.main()
            self.assertFalse((state / 'requests').exists())


if __name__ == '__main__':
    unittest.main()
