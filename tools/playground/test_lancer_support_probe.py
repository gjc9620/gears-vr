import unittest

from lancer_support_probe import parse_pose_row


class LancerSupportProbeRows(unittest.TestCase):
    def test_bounded_game_thread_row(self):
        matrix = ','.join(str(1 if i in (0, 5, 10, 15) else 0) for i in range(16))
        row = parse_pose_row('lancer_support_pose sample=4 qpc=12345 ' +
                             ' '.join(f'{name}={matrix}' for name in
                                      ('pawn', 'gun', 'ik_left', 'hand_left', 'hand_right')))
        self.assertEqual((row['sample'], row['qpc']), (4, 12345))
        self.assertEqual(row['hand_left'][15], 1)

    def test_incomplete_or_nonfinite_row_rejected(self):
        for value in ('1,2,3', ','.join(['nan'] + ['0'] * 15)):
            with self.subTest(value=value), self.assertRaises(ValueError):
                parse_pose_row('lancer_support_pose sample=0 qpc=1 ' +
                               ' '.join(f'{name}={value}' for name in
                                        ('pawn', 'gun', 'ik_left', 'hand_left', 'hand_right')))


if __name__ == '__main__':
    unittest.main()
