import unittest

from analyze_hand_ik_controlled_trial import (
    interpolate_x, matched_span_controls, position, unique_ticks)


def row(index, tick, x, *, known='1'):
    matrix = [0.0] * 16
    matrix[12] = x
    return {'phase': 'pre_natural', 'mesh_tick_known': known,
            'mesh_tick': str(tick), 'qpc': str(100 + index * 100),
            'right': ','.join(map(str, matrix)),
            'left': ','.join(map(str, matrix))}


class ControlledTrialAnalysisTest(unittest.TestCase):
    def test_prediction_uses_bracketing_qpc_span(self):
        self.assertAlmostEqual(interpolate_x(row(0, 1, 0), row(2, 3, .02), 200), .01)
        with self.assertRaises(ValueError):
            interpolate_x(row(0, 1, 0), row(2, 3, .02), 300)

    def test_same_span_natural_control_leaves_out_center(self):
        linear = [row(i, i + 1, i / 100) for i in range(9)]
        self.assertTrue(all(abs(value) < 1e-9
                            for value in matched_span_controls(linear, 100, 100)))
        linear[4] = row(4, 5, 1.04)
        self.assertIn(1.0, matched_span_controls(linear, 100, 100))

    def test_natural_control_requires_distinct_known_ticks(self):
        samples = {index: row(index, index, index / 100)
                   for index in range(8)}
        summary = {'first_sample': '0', 'last_sample': '7',
                   'distinct_mesh_ticks': '8', 'complete': '1'}
        self.assertEqual(len(unique_ticks(samples, summary)), 8)
        samples[7] = row(7, 6, .07)
        with self.assertRaises(ValueError):
            unique_ticks(samples, summary)
        samples[7] = row(7, 7, .07, known='0')
        with self.assertRaises(ValueError):
            unique_ticks(samples, summary)

    def test_nonfinite_bone_matrix_rejected(self):
        value = row(0, 1, 0)
        value['right'] = value['right'].replace('0.0', 'nan', 1)
        with self.assertRaises(ValueError):
            position(value, 'right')


if __name__ == '__main__':
    unittest.main()
