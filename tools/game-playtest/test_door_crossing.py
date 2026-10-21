import math
import struct
import unittest
from unittest.mock import Mock
from unittest.mock import patch

from interaction_flow import crossing_measure, exit_geometry


class DoorCrossingTests(unittest.TestCase):
    def fixture(self):
        q = Mock()
        pc, door, pawn, align, world = [dict(address=i, cls=i) for i in range(1, 6)]
        door_cylinder = dict(address=6, cls=6, outer=2)
        pawn_cylinder = dict(address=7, cls=7, outer=3)
        typ = dict(address=8, name='Rotator')
        q.obj.side_effect = lambda address: {6: door_cylinder, 7: pawn_cylinder, 8: typ}[address]
        q.pointer.side_effect = lambda obj, name: pawn if name == 'Pawn' else align if name == 'AlignToObj' else world
        q.position.side_effect = lambda obj: [0, 0, 0] if obj == door else [-120, 0, 0]
        q.r.struct_type.return_value = 8
        q.prop.side_effect = lambda obj, name, kind, **kw: (
            dict(size=12) if name == 'Rotation' else dict(size=8) if name == 'CollisionComponent' else
            dict(size=4, offset={'Pitch': 0, 'Yaw': 4, 'Roll': 8}[name]))
        q.value.side_effect = lambda obj, p: (struct.pack('<3i', 0, 16384, 0) if obj == align else
                                            struct.pack('<Q', 6 if obj == door else 7))
        q.scalar.side_effect = lambda obj, *args: 128 if obj == door_cylinder else 34
        q.s.derives.return_value = True
        bounds = patch('interaction_flow.component_bounds')
        mock_bounds = bounds.start();self.addCleanup(bounds.stop)
        mock_bounds.side_effect = lambda query, obj: dict(
            component=door_cylinder if obj == door else pawn_cylinder,
            center=[0, 0, 0] if obj == door else [-120, 0, 0],
            extent=[128, 128, 128] if obj == door else [34, 34, 72])
        return q, pc, door, pawn, align, world

    def test_authored_alignment_and_current_cylinders_define_exit_goal(self):
        q, pc, door, *_ = self.fixture()
        geometry = exit_geometry(q, pc, door)
        self.assertAlmostEqual(geometry['threshold'], 170)
        self.assertAlmostEqual(geometry['exit_goal'][0], 0)
        self.assertAlmostEqual(geometry['exit_goal'][1], 220)
        self.assertEqual(geometry['align']['address'], 4)

    def test_scaled_translated_world_bounds_determine_actual_crossing(self):
        q, pc, door, pawn, align, world = self.fixture()
        with patch('interaction_flow.component_bounds', side_effect=lambda query, obj: dict(
                component=dict(address=6, cls=6, outer=2) if obj == door else dict(address=7, cls=7, outer=3),
                center=[1000, 2000, 0] if obj == door else [-120, 0, 0],
                extent=[256, 256, 128] if obj == door else [34, 34, 72])):
            geometry = exit_geometry(q, pc, door)
        self.assertAlmostEqual(geometry['threshold'], 298)
        self.assertAlmostEqual(geometry['exit_goal'][0], 1000)
        self.assertAlmostEqual(geometry['exit_goal'][1], 2348)

    def test_no_alignment_uses_approach_direction_without_teleporting(self):
        q, pc, door, pawn, _, world = self.fixture()
        q.pointer.side_effect = lambda obj, name: pawn if name == 'Pawn' else None if name == 'AlignToObj' else world
        geometry = exit_geometry(q, pc, door)
        self.assertEqual(geometry['forward'], [1, 0])
        self.assertEqual(geometry['exit_goal'], [220, 0, 0])

    def test_wrong_component_owner_rejects_geometry(self):
        q, pc, door, *_ = self.fixture()
        q.obj.side_effect = lambda address: dict(address=address, name='Rotator', cls=address, outer=999)
        with self.assertRaisesRegex(RuntimeError, 'current cylinder'):
            exit_geometry(q, pc, door)

    def test_passing_requires_far_side_lateral_clearance_and_same_floor(self):
        geometry = dict(center=[0, 0, 0], forward=[1, 0], start=[-120, 0, 0], threshold=170, lateral_limit=128)
        self.assertFalse(crossing_measure(geometry, [-100, 0, 0])['passed'])
        self.assertFalse(crossing_measure(geometry, [220, 200, 0])['passed'])
        self.assertFalse(crossing_measure(geometry, [220, 0, 181])['passed'])
        self.assertTrue(crossing_measure(geometry, [180, 30, 0])['passed'])
        geometry['forward'] = [0, 1]
        self.assertTrue(crossing_measure(geometry, [20, 180, 0])['passed'])
