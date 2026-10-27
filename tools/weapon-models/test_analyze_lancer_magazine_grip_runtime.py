import math
import unittest

from analyze_lancer_magazine_grip_runtime import summarize


def matrix(angle, translation):
    cosine, sine = math.cos(angle), math.sin(angle)
    return [cosine, sine, 0, 0, -sine, cosine, 0, 0,
            0, 0, 1, 0, *translation, 1]


def world_point(parent, point):
    return [parent[12 + axis] + sum(point[component] * parent[component * 4 + axis]
                                      for component in range(3)) for axis in range(3)]


class OriginalLancerMagazineGripTest(unittest.TestCase):
    def test_stable_hold_is_separate_from_release_motion(self):
        timeline, poses = [], []
        for sample in range(30):
            left = matrix(sample * .025, [sample * .1, -sample * .2, 10])
            offset = [19, -5.5, .14] if sample < 21 else [19 + sample - 20, -5.5, .14]
            magazine = matrix(sample * .025, world_point(left, offset))
            timeline.append({'sample': sample, 'qpc': sample + 1, 'world': sample * .02,
                             'reload_start': 0, 'mag_pawn': True})
            poses.append({'sample': sample, 'qpc': sample + 1,
                          'gun': matrix(0, [0, 0, 0]), 'pawn': matrix(0, [0, 0, 0]),
                          'left': left, 'magazine': magazine})
        capture = {'passed': True, 'mode': 'reload_visual', 'game_pid': 12,
                   'weapon_mesh_asset': {'name': 'COGAssaultRifle'},
                   'reload_visual_timeline': timeline, 'held_magazine_poses': poses}
        result = summarize(capture)
        self.assertEqual(result['stable_hand_hold_samples'], 21)
        self.assertEqual(result['stable_hand_hold_last_sample'], 20)
        for actual, expected in zip(result['magazine_origin_in_left_hand_during_hold_raw'],
                                    [19, -5.5, .14]):
            self.assertAlmostEqual(actual, expected, places=6)
        self.assertLess(result['maximum_origin_drift_during_hold_raw'], 1e-9)
        self.assertGreater(result['maximum_origin_drift_all_attached_raw'], 8)
        poses[0]['qpc'] = 999
        with self.assertRaisesRegex(ValueError, 'complete'):
            summarize(capture)


if __name__ == '__main__':
    unittest.main()
