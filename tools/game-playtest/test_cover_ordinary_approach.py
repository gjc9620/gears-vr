import math
from types import SimpleNamespace
import unittest

from cover_ordinary_approach import navigate


def fixture(positions, angle=0.0):
    positions = iter(positions)
    calls, events = [], []
    def stick(*args):
        calls.append(args)
    ctx = SimpleNamespace(live=lambda: None, location=lambda: next(positions),
                          yaw=lambda: angle, data=dict(motion=[]), stick=stick,
                          g=SimpleNamespace(rec=SimpleNamespace(event=lambda *args, **kw: events.append((args, kw)))))
    return ctx, calls, events


class ApproachProtocolTest(unittest.TestCase):
    def test_recorded_56276_stall_still_rejects(self):
        start = (-3900.0, 0.0, 583.9485473632812)
        stopped = (-3928.939453125, -50.95433044433594, start[2])
        target = (-3979.6602316476506, -140.2601870208192, 563.9999997615814)
        ctx, calls, events = fixture([start, stopped, stopped, stopped, stopped])
        with self.assertRaisesRegex(RuntimeError, 'blocked before cover'):
            navigate(ctx, target)
        self.assertEqual(len(calls), 4)
        self.assertEqual(len(ctx.data['motion']), 5)
        for hand, right, forward, duration in calls:
            self.assertEqual(hand, 0)
            self.assertAlmostEqual(math.hypot(right, forward), .62)
            self.assertEqual(duration, .04)
        self.assertTrue(all(not event[1]['collision_or_deadzone_cause_proven'] for event in events))

    def test_actual_position_alone_ends_walk(self):
        ctx, calls, _ = fixture([(0., 0., 0.), (100., 0., 0.), (170., 0., 0.)])
        self.assertEqual(navigate(ctx, (200., 0., 0.)), (170., 0., 0.))
        self.assertEqual([call[-1] for call in calls], [.22, .04])

    def test_original_yaw_maps_axes_without_hmd_heading(self):
        ctx, calls, _ = fixture([(0., 0., 0.), (0., 190., 0.)], math.pi/2)
        navigate(ctx, (0., 200., 0.))
        self.assertAlmostEqual(calls[0][1], 0.)
        self.assertAlmostEqual(calls[0][2], .62)

    def test_progress_does_not_remove_total_step_bound(self):
        ctx, calls, _ = fixture([(float(i), 0., 0.) for i in range(42)])
        with self.assertRaisesRegex(RuntimeError, 'did not reach cover'):
            navigate(ctx, (500., 0., 0.))
        self.assertEqual(len(calls), 42)

    def test_invalid_measurements_or_widened_arrival_do_not_publish(self):
        for target, positions, angle, arrival in (
            ((math.nan, 0., 0.), [], 0., 35.),
            ((200., 0., 0.), [(math.nan, 0., 0.)], 0., 35.),
            ((200., 0., 0.), [(0., 0., 0.)], math.inf, 35.),
            ((200., 0., 0.), [], 0., 100.),
        ):
            ctx, calls, _ = fixture(positions, angle)
            with self.assertRaises((ValueError, RuntimeError)):
                navigate(ctx, target, arrival_distance=arrival)
            self.assertFalse(calls)


if __name__ == '__main__':
    unittest.main()
