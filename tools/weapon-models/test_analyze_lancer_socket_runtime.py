import math
import unittest

from analyze_lancer_socket_runtime import summarize


def gun_matrix(angle, translation):
    cosine, sine = math.cos(angle), math.sin(angle)
    return [cosine, sine, 0, 0, -sine, cosine, 0, 0,
            0, 0, 1, 0, *translation, 1]


def world_point(gun, local):
    return [gun[12 + axis] + sum(local[component] * gun[component * 4 + axis]
                                 for component in range(3)) for axis in range(3)]


class LancerSocketRuntimeTest(unittest.TestCase):
    def test_bone_pivot_and_socket_stay_in_gun_frame(self):
        pivot = [0.008, 28.146, 4.396]
        socket = [pivot[0], pivot[1], pivot[2] - 10.8802385]
        rows = []
        for sample in range(16):
            gun = gun_matrix(sample * .03, [sample * .2, -sample * .1, 5])
            bone = gun_matrix(sample * .03, world_point(gun, pivot))
            rows.append({'sample': sample, 'qpc': sample + 1, 'gun': gun,
                         'bone': bone, 'world': world_point(gun, socket),
                         'rotation': [0, 0, 0]})
        begin = ('lancer_socket_begin pc=1 pawn=2 weapon=3 mesh=4 asset=5 '
                 'socket=6 name=Magazine bone=b_AR_Magazine relative=0,0,-10.8802385 '
                 'rotation=0,0,0 count=2 bone_index=8')
        capture = {'passed': True, 'mode': 'socket', 'game_pid': 7,
                   'weapon_mesh_asset': {'name': 'COGAssaultRifle'},
                   'socket_begin': begin, 'socket_samples': rows}
        partition = {'source_path': "SkeletalMesh'COG_AssaultRifle.Mesh.COGAssaultRifle'",
                     'bone': 'b_AR_Magazine',
                     'mesh_space_reference_pivot_candidate_raw': pivot}
        result = summarize(capture, partition)
        self.assertLess(result['runtime_to_static_pivot_distance_raw'], 1e-9)
        self.assertLess(result['maximum_bone_pivot_local_drift_raw'], 1e-9)
        self.assertAlmostEqual(result['pivot_to_socket_axis_in_gun'][2], -1)
        self.assertAlmostEqual(result['pivot_to_socket_distance_raw'], 10.8802385)
        del rows[-1]['bone']
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            summarize(capture, partition)


if __name__ == '__main__':
    unittest.main()
