import unittest

from lancer_support_probe import (parse_fixed_feed_row, parse_insertion_candidate_row,
                                  parse_magazine_follow_row,
                                  parse_magazine_pose_row,
                                  parse_pose_row, parse_socket_row)


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

    def test_same_tick_socket_row(self):
        matrix = ','.join(str(1 if i in (0, 5, 10, 15) else 0) for i in range(16))
        row = parse_socket_row('lancer_socket_sample sample=2 qpc=12345 ' +
                               f'gun={matrix} world=1.5,2.5,3.5 rotation=0,100,200')
        self.assertEqual(row['sample'], 2)
        self.assertEqual(row['world'], [1.5, 2.5, 3.5])
        with_bone = parse_socket_row('lancer_socket_sample sample=2 qpc=12345 ' +
                                     f'gun={matrix} bone={matrix} '
                                     'world=1.5,2.5,3.5 rotation=0,100,200')
        self.assertEqual(with_bone['bone'][15], 1)
        with self.assertRaises(ValueError):
            parse_socket_row('lancer_socket_sample sample=2 qpc=12345 ' +
                             f'gun={matrix} world=nan,2.5,3.5 rotation=0,100,200')

    def test_fixed_feed_world_row(self):
        row = parse_fixed_feed_row('lancer_fixed_feed_anchor weapon=0x1 mesh=0x2 '
                                   'point=1,2,3 outward=0,0,-1')
        self.assertEqual(row['point'], [1, 2, 3])
        with self.assertRaises(ValueError):
            parse_fixed_feed_row('lancer_fixed_feed_anchor weapon=0x1 mesh=0x2 '
                                 'point=1,2,3 outward=0,0,0')

    def test_held_magazine_matrix_row(self):
        matrix = ','.join(str(1 if i in (0, 5, 10, 15) else 0) for i in range(16))
        row = parse_magazine_pose_row('lancer_reload_magazine_pose sample=9 qpc=12345 ' +
                                      ' '.join(f'{key}={matrix}' for key in
                                               ('gun', 'pawn', 'left', 'magazine')))
        self.assertEqual(row['sample'], 9)
        self.assertEqual(row['magazine'][15], 1)

    def test_original_component_follow_row(self):
        source = ('lancer_magazine_follow weapon=0x1 component=0x2 '
                  'left=1,2,3 world=4,5,6 matrix=1 attached=1 '
                  'ammo_control=1 target=1.0 reattached=1')
        row = parse_magazine_follow_row(source)
        self.assertEqual(row['world'], [4., 5., 6.])
        self.assertTrue(row['reattached'])
        with self.assertRaises(ValueError):
            parse_magazine_follow_row(source.replace('matrix=1', 'matrix=2'))

    def test_insertion_candidate_row(self):
        row = parse_insertion_candidate_row(
            'lancer_magazine_insertion_candidate result=2 sequence=33 '
            'depth=1.5 lateral=1.8 alignment=0.99 clock=0.98')
        self.assertEqual((row['result'], row['sequence']), (2, 33))
        with self.assertRaises(ValueError):
            parse_insertion_candidate_row(
                'lancer_magazine_insertion_candidate result=2 sequence=33 '
                'depth=nan lateral=1.8 alignment=0.99 clock=0.98')


if __name__ == '__main__':
    unittest.main()
