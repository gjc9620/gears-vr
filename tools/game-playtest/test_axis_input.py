import struct
import unittest
from unittest.mock import Mock, patch

from axis_input import pulse, AxisReadInterrupted
from live import InputNotReady, InputInterrupted
from test_core import sample
import test_live_input


class AxisInputTests(unittest.TestCase):
    def fixture(self):
        observer = test_live_input.InputTests().fixture()
        slot = Mock(address=0x1000, input=dict(address=1), controller=dict(address=2),
                    prop=dict(type='FloatProperty', size=4), initial_pitch=0, initial_angle=0,
                    original=struct.pack('<f', 0))
        slot.pitch.return_value = 0
        slot.angle.return_value = 0
        slot.raw.return_value = struct.pack('<f', 0)
        writer = Mock()
        return observer, Mock(return_value=slot), Mock(return_value=writer), slot, writer

    def clock(self):
        now = [0]
        def sleep(seconds):
            now[0] += seconds
        return lambda: now[0], sleep

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_cancel_after_write_neutralizes_same_slot_and_closes_writer(self, background):
        observer, slots, writers, slot, writer = self.fixture()
        clock, sleep = self.clock()
        with patch('axis_input.time.monotonic', clock), patch('axis_input.time.sleep', sleep):
            with self.assertRaises(InputInterrupted):
                pulse(observer, 'aLookUp', .2, .1, slot_factory=slots, writer_factory=writers,
                      stop_requested=Mock(side_effect=[False, False, True]))
        self.assertEqual([struct.unpack('<f', call.args[1])[0] for call in writer.write.call_args_list][-1], 0)
        self.assertEqual(writer.write.call_count, 2)
        writer.close.assert_called_once()
        self.assertEqual(slot.validate.call_args_list[-1].kwargs, dict(neutral=True))

    def test_passive_session_cannot_open_a_writer(self):
        observer, slots, writers, _, _ = self.fixture()
        observer.action_owner = None
        with self.assertRaisesRegex(RuntimeError, 'owned launcher'):
            pulse(observer, 'aLookUp', .2, .1, slot_factory=slots, writer_factory=writers)
        writers.assert_not_called()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_non_neutral_or_unproven_slot_is_rejected_before_writer(self, background):
        observer, slots, writers, _, _ = self.fixture()
        slots.side_effect = RuntimeError('Axis input is not neutral')
        with self.assertRaises(InputNotReady):
            pulse(observer, 'aLookUp', .2, .1, slot_factory=slots, writer_factory=writers)
        writers.assert_not_called()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_partial_write_failure_still_attempts_neutralization(self, background):
        observer, slots, writers, slot, writer = self.fixture()
        writer.write.side_effect = [OSError('partial native write'), None]
        clock, sleep = self.clock()
        with patch('axis_input.time.monotonic', clock), patch('axis_input.time.sleep', sleep):
            with self.assertRaises(OSError):
                pulse(observer, 'aMouseY', .2, .1, slot_factory=slots, writer_factory=writers)
        self.assertEqual(writer.write.call_count, 2)
        self.assertEqual(writer.write.call_args_list[-1].args, (slot.address, slot.original))
        writer.close.assert_called_once()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_recycled_slot_is_never_neutralized_through_an_old_address(self, background):
        observer, slots, writers, slot, writer = self.fixture()
        slot.validate.side_effect = [None, RuntimeError('slot recycled'), RuntimeError('slot recycled')]
        clock, sleep = self.clock()
        with patch('axis_input.time.monotonic', clock), patch('axis_input.time.sleep', sleep):
            with self.assertRaisesRegex(RuntimeError, 'slot recycled'):
                pulse(observer, 'aLookUp', .2, .1, slot_factory=slots, writer_factory=writers)
        self.assertEqual(writer.write.call_count, 1)
        writer.close.assert_called_once()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_read_interruption_is_recoverable_only_after_certified_neutral(self, background):
        observer, slots, writers, slot, writer = self.fixture()
        slot.validate.side_effect = [None, RuntimeError('registry grew'), None]
        clock, sleep = self.clock()
        with patch('axis_input.time.monotonic', clock), patch('axis_input.time.sleep', sleep):
            with self.assertRaises(AxisReadInterrupted) as raised:
                pulse(observer, 'aTurn', .2, .1, slot_factory=slots, writer_factory=writers)
        self.assertTrue(raised.exception.receipt['neutral_verified'])
        self.assertEqual(writer.write.call_args_list[-1].args, (slot.address, slot.original))
        writer.close.assert_called_once()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_translation_stops_at_displacement_limit_and_neutralizes(self, background):
        observer, slots, writers, slot, writer = self.fixture()
        observer.sample.side_effect = [sample(0), sample(.01), sample(.02, position=(65, 0, 0))]
        clock, sleep = self.clock()
        with patch('axis_input.time.monotonic', clock), patch('axis_input.time.sleep', sleep):
            receipt = pulse(observer, 'aBaseY', .2, .2, slot_factory=slots, writer_factory=writers)
        self.assertTrue(receipt['safety_stop'])
        self.assertTrue(receipt['neutral_verified'])
        self.assertGreater(writer.write.call_count, 2)
        self.assertLessEqual(receipt['actual_hold_seconds'], .05)
        self.assertEqual(writer.write.call_args_list[-1].args, (slot.address, slot.original))
        writer.close.assert_called_once()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_near_waypoint_can_reduce_the_limit_without_losing_neutralization(self, background):
        observer, slots, writers, slot, writer = self.fixture()
        observer.sample.side_effect = [sample(0), sample(.01), sample(.02, position=(6, 0, 0))]
        clock, sleep = self.clock()
        with patch('axis_input.time.monotonic', clock), patch('axis_input.time.sleep', sleep):
            receipt = pulse(observer, 'aBaseY', .2, .2, slot_factory=slots, writer_factory=writers,
                            displacement_limit=5)
        self.assertTrue(receipt['safety_stop'])
        self.assertTrue(receipt['neutral_verified'])
        self.assertEqual(receipt['displacement_limit_units'], 5)
        self.assertLessEqual(receipt['actual_hold_seconds'], .05)
        self.assertEqual(writer.write.call_args_list[-1].args, (slot.address, slot.original))
        writer.close.assert_called_once()

    def test_reduced_limits_cannot_expand_the_original_translation_bound(self):
        for limit in (0, 65, float('nan'), True):
            observer, slots, writers, _, _ = self.fixture()
            with self.subTest(limit=limit), self.assertRaises(ValueError):
                pulse(observer, 'aBaseY', .2, .2, displacement_limit=limit,
                      slot_factory=slots, writer_factory=writers)
            slots.assert_not_called()
            writers.assert_not_called()

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_translation_world_change_interrupts_and_neutralizes(self, background):
        observer, slots, writers, slot, writer = self.fixture()
        observer.sample.side_effect = [sample(0), sample(.01), sample(.02, world=2)]
        clock, sleep = self.clock()
        with patch('axis_input.time.monotonic', clock), patch('axis_input.time.sleep', sleep):
            with self.assertRaises(InputInterrupted):
                pulse(observer, 'aBaseY', .2, .2, slot_factory=slots, writer_factory=writers)
        self.assertEqual(writer.write.call_args_list[-1].args, (slot.address, slot.original))
        writer.close.assert_called_once()


if __name__ == '__main__':
    unittest.main()
