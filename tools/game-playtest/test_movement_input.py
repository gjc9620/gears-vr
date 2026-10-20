import unittest
from pathlib import Path
import tempfile
from unittest.mock import Mock, patch

from movement_input import response, calibrate
from axis_input import AxisReadInterrupted
from test_core import sample


class ForwardResponseTests(unittest.TestCase):
    def test_verified_calibration_survives_later_zero_write_read_interruption(self):
        observer = Mock()
        report = Mock(directory=Path(tempfile.gettempdir()) / 'gearsvr-forward-no-stop')
        report.action.return_value = dict(action_id=3)
        start = sample(0)
        moved = sample(1, position=(25, 0, 0))
        receipt = dict(release_posted=True, neutral_verified=True, actual_hold_seconds=.25)
        interrupted = AxisReadInterrupted('Reader retired before dispatch', dict(writes=0, neutral_verified=False, writer_closed=True))
        with patch('scenarios.wait_state', side_effect=[start, start, moved, moved, moved, moved]), patch('axis_input.pulse', side_effect=[receipt, interrupted]), patch('movement_input.time.sleep'):
            accepted = calibrate(observer, report)
        self.assertEqual(accepted['value'], .2)
        self.assertEqual(accepted['speed'], 100)
        self.assertEqual(accepted['source_action'], 3)
        report.cover.assert_called_once()

    def test_forward_direction_rotates_with_view(self):
        measured = response(sample(0, yaw=16384), sample(1, yaw=16384, position=(0, 40, 0)),
                            sample(2, yaw=16384, position=(0, 41, 0)))
        self.assertAlmostEqual(measured['forward_fraction'], 1)
        self.assertEqual(measured['neutral_drift'], 1)

    def test_backward_and_lateral_travel_are_not_forward(self):
        backward = response(sample(0), sample(1, position=(-40, 0, 0)), sample(2, position=(-40, 0, 0)))
        lateral = response(sample(0), sample(1, position=(0, 40, 0)), sample(2, position=(0, 40, 0)))
        self.assertEqual(backward['forward_fraction'], -1)
        self.assertEqual(lateral['forward_fraction'], 0)

    def test_scene_or_possession_change_invalidates_response(self):
        self.assertIsNone(response(sample(0), sample(1, world=2), sample(2, world=2)))
        changed = sample(2)
        changed['pawn_identity'] = dict(address=2000, internal_index=5)
        self.assertIsNone(response(sample(0), sample(1), changed))


if __name__ == '__main__':
    unittest.main()
