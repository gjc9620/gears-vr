import math
import unittest

from analyze_lancer_support_runtime import gun_local_point, summarize


def matrix(angle, translation):
    c, s = math.cos(angle), math.sin(angle)
    return [c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, *translation, 1]


def hand(gun, point, relative_angle=0):
    world = [gun[12 + k] + sum(point[i] * gun[i * 4 + k] for i in range(3))
             for k in range(3)]
    return matrix(math.atan2(gun[1], gun[0]) + relative_angle, world)


class LancerSupportRuntimeTest(unittest.TestCase):
    def test_row_vector_inverse_and_moving_gun(self):
        rows = []
        for index in range(24):
            gun = matrix(index * .025, [index * .1, -index * .03, 5])
            left = hand(gun, [8.5, 43.5, .2])
            right = hand(gun, [-2.3, 0, 0], math.pi / 2)
            rows.append({'sample': index, 'qpc': index + 1, 'gun': gun,
                         'hand_left': left, 'ik_left': left, 'hand_right': right})
        capture = {'passed': True, 'left_hand_ik_on': True, 'game_pid': 12,
                   'weapon_mesh_asset': {'name': 'COGAssaultRifle'}, 'samples': rows}
        result = summarize(capture)
        for actual, expected in zip(result['left_palm_in_gun_raw'], [8.5, 43.5, .2]):
            self.assertAlmostEqual(actual, expected, places=6)
        self.assertLess(result['maximum_left_palm_local_drift_raw'], 1e-9)
        self.assertLess(result['maximum_right_hand_basis_component_drift'], 1e-9)
        expected_basis = [[0, 1, 0], [-1, 0, 0], [0, 0, 1]]
        for row, expected in zip(result['right_hand_basis_in_gun_rows'], expected_basis):
            for actual, target in zip(row, expected):
                self.assertAlmostEqual(actual, target, places=6)
        self.assertGreater(result['first_to_last_gun_world_travel_raw'], 2)

    def test_rejects_shear_and_incomplete_capture(self):
        gun = matrix(0, [0, 0, 0])
        gun[4] = .5
        with self.assertRaisesRegex(ValueError, 'sheared'):
            gun_local_point(gun, matrix(0, [1, 2, 3]))
        with self.assertRaisesRegex(ValueError, 'complete'):
            summarize({'passed': True, 'left_hand_ik_on': True,
                       'weapon_mesh_asset': {'name': 'COGAssaultRifle'},
                       'samples': []})


if __name__ == '__main__':
    unittest.main()
