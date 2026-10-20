from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import Mock, patch

from core import Evidence, controllable
from interaction_flow import door_context, wait_effect
from live import InputInterrupted, InteractionReleaseFailed
from test_core import sample
import test_live_input


class DoorFlowTests(unittest.TestCase):
    def clock(self):
        now = [0.0]
        return now, lambda: now[0], lambda seconds: now.__setitem__(0, now[0] + seconds)

    def test_scripted_move_blocks_input_and_drift_classification(self):
        scripted = dict(sample(1, position=(50, 0, 0)), special_move_raw=44)
        self.assertFalse(controllable(scripted))
        with tempfile.TemporaryDirectory() as folder:
            report = Evidence(Path(folder) / 'run', mode='run', provenance={})
            result = report.action('move_forward', sample(0), scripted, dispatched=True,
                                   neutral=dict(scripted, pose=dict(position=(90, 0, 0))))
            self.assertEqual(result['result'], 'unverified')
            self.assertFalse(report.issues)
            result = report.action('move_forward', sample(0), sample(1, position=(40, 0, 0)),
                                   dispatched=True, neutral=scripted)
            self.assertEqual(result['result'], 'observed')
            self.assertFalse(report.issues)

    def test_door_context_requires_typed_fields_world_and_originator(self):
        q = Mock()
        pc, event, door, world, typ = [dict(address=i, cls=i) for i in (1, 2, 3, 4, 5)]
        fields = {'DoorTrigger': dict(type='ObjectProperty', size=8, offset=0, address=10),
                  'bInsideDoorTrigger': dict(type='BoolProperty', size=4, offset=8, address=11),
                  'eSpecialMoveType': dict(type='ByteProperty', size=1, offset=12, address=12)}
        parent = dict(type='StructProperty', size=16, offset=200, address=20)
        q.prop.side_effect = lambda obj, name, kind, **kw: parent if name == 'DoorTriggerDatum' else fields[name]
        q.r.struct_type.return_value = 5
        q.obj.side_effect = lambda address: {3: door, 5: typ}[address]
        q.value.return_value = struct.pack('<QI B3x', 3, 1, 0)
        q.decode.return_value = True
        q.pointer.side_effect = lambda obj, name: door if name == 'Originator' else world
        q.s.derives.return_value = True
        result = door_context(q, pc, event)
        self.assertTrue(result['inside'])
        self.assertEqual(result['door'], door)
        q.pointer.side_effect = lambda obj, name: None if name == 'Originator' else world
        with self.assertRaisesRegex(RuntimeError, 'originator mismatch'):
            door_context(q, pc, event)

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_door_hold_is_long_enough_for_timer_and_releases_on_scripted_move(self, background):
        observer = test_live_input.InputTests().fixture()
        observer.sampler = Mock(controller=1)
        observer.bindings.return_value = {'E': 'Game_Pickup'}
        event = dict(address=2, cls=3)
        q = Mock();q.pointer.return_value = event
        now, clock, sleep = self.clock()
        observer.sample.side_effect = lambda: dict(sample(now[0]), special_move_raw=44 if now[0] >= .36 else 0)
        with patch('world_state.Query', return_value=q), patch('interaction_flow.door_context', return_value=dict(inside=True, door=dict(address=3))), patch('live.time.monotonic', clock), patch('live.time.sleep', sleep):
            receipt = observer.interaction_key('E', event)
        self.assertGreaterEqual(receipt['actual_hold_seconds'], .3)
        self.assertLessEqual(receipt['actual_hold_seconds'], .45)
        self.assertEqual(receipt['special_move_observed'], 44)
        self.assertEqual([c.args[1] for c in observer.user.PostMessageW.call_args_list], [0x100, 0x101])

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_cancel_during_door_hold_still_releases_key(self, background):
        observer = test_live_input.InputTests().fixture()
        observer.sampler = Mock(controller=1);observer.bindings.return_value = {'E': 'Game_Pickup'}
        event = dict(address=2, cls=3);q = Mock();q.pointer.return_value = event
        now, clock, sleep = self.clock()
        observer.user.GetAsyncKeyState.side_effect = lambda key: 0x8000 if now[0] >= .12 else 0
        with patch('world_state.Query', return_value=q), patch('interaction_flow.door_context', return_value=dict(inside=True, door=dict(address=3))), patch('live.time.monotonic', clock), patch('live.time.sleep', sleep):
            with self.assertRaises(InputInterrupted):
                observer.interaction_key('E', event)
        self.assertEqual(observer.user.PostMessageW.call_args_list[-1].args[1], 0x101)

    @patch('live.background_input.change', return_value={'state': 'original'})
    def test_interaction_release_failure_is_fatal_to_the_owned_run(self, background):
        observer = test_live_input.InputTests().fixture()
        observer.sampler = Mock(controller=1);observer.bindings.return_value = {'E': 'Game_Pickup'}
        observer.user.PostMessageW.side_effect = [True, False]
        event = dict(address=2, cls=3);q = Mock();q.pointer.return_value = event
        now, clock, sleep = self.clock()
        with patch('world_state.Query', return_value=q), patch('interaction_flow.door_context', return_value=None), patch('live.time.monotonic', clock), patch('live.time.sleep', sleep):
            with self.assertRaises(InteractionReleaseFailed):
                observer.interaction_key('E', event)

    def test_delayed_door_count_waits_for_scripted_move_end(self):
        observer = Mock(sampler=Mock(controller=1));observer.user.GetAsyncKeyState.return_value = 0
        states = [dict(sample(i), sample_id=i + 1, special_move_raw=move) for i, move in enumerate((44, 44, 0))]
        observer.sample.side_effect = states
        report = Mock(directory=Path(tempfile.gettempdir()) / 'gearsvr-no-door-stop')
        report.observe.side_effect = lambda s: s
        event = dict(address=2, cls=3);q = Mock();q.obj.return_value = event;q.scalar.side_effect = [0, 1, 1]
        now, clock, sleep = self.clock()
        with patch('interaction_flow.Query', return_value=q), patch('interaction_flow.door_context', return_value=dict(inside=False, door=None)), patch('interaction_flow.time.monotonic', clock), patch('interaction_flow.time.sleep', sleep):
            result = wait_effect(observer, report, event, 0, dict(before_sample=sample(0)))
        self.assertEqual(result['after_count'], 1)
        self.assertEqual(len(result['observations']), 3)
        self.assertTrue(result['animation_ended'])
