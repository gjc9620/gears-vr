"""Focused checks for preventing stuck inputs; actual capture/control tested live."""
import unittest
from unittest.mock import patch
import game_io


class FakePad:
    def __init__(self):
        self.neutral = True
        self.sent = []
    def reset(self): self.neutral = True
    def left_joystick_float(self, x, y): self.neutral = self.neutral and x == y == 0
    def right_joystick_float(self, x, y): self.neutral = self.neutral and x == y == 0
    def left_trigger_float(self, x): self.neutral = self.neutral and x == 0
    def right_trigger_float(self, x): self.neutral = self.neutral and x == 0
    def update(self): self.sent.append(self.neutral)


class InputGuards(unittest.TestCase):
    def setUp(self):
        # Unit tests must not read the service's real stop file or physical F8.
        stop = patch.object(game_io, 'interrupted', return_value=False)
        stop.start()
        self.addCleanup(stop.stop)

    def test_focus_loss_releases_an_active_stick(self):
        pad = FakePad()
        with patch.object(game_io, 'game_window', return_value=(1, 2)), \
             patch.object(game_io, 'focused', side_effect=[True, True, False]):
            with self.assertRaisesRegex(RuntimeError, 'Input stopped'):
                game_io.perform(pad, None, {'right': [0.6, 0], 'seconds': 1})
        self.assertEqual(pad.sent, [False, True])

    def test_unfocused_window_receives_no_input(self):
        pad = FakePad()
        with patch.object(game_io, 'game_window', return_value=(1, 2)), \
             patch.object(game_io, 'focused', return_value=False):
            with self.assertRaisesRegex(RuntimeError, 'no input sent'):
                game_io.perform(pad, None, {'right': [0.6, 0]})
        self.assertEqual(pad.sent, [])

    def test_invalid_actions_never_reach_device(self):
        for request in ({'seconds': 10}, {'right': [float('nan'), 0]}, {'buttons': ['unknown']}):
            pad = FakePad()
            with self.assertRaises(ValueError):
                game_io.perform(pad, None, request)
            self.assertEqual(pad.sent, [])


if __name__ == '__main__':
    unittest.main()
