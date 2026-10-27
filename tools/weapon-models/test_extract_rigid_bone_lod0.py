import pathlib
import math
import struct
import tempfile
import unittest

from extract_rigid_bone_lod0 import extract, mesh_space_reference_pivot, write_obj


def fixture():
    positions = ((0., 0., 0.), (1., 0., 0.), (0., 1., 0.),
                 (2., 0., 0.), (3., 0., 0.), (2., 1., 0.))
    raw = bytearray(6 * 64 + 6 * 2)
    for vertex, position in enumerate(positions):
        at = vertex * 64
        struct.pack_into('<3f', raw, at, *position)
        raw[at + 16:at + 20] = bytes((127, 127, 0, 0))
        struct.pack_into('<2f', raw, at + 20, vertex / 10, vertex / 20)
        struct.pack_into('<f', raw, at + 28, 900 + vertex)  # UV1 must not become UV0.v.
        raw[at + 60] = 255
    # Local index zero is the magazine in chunk zero, chainsaw in chunk one.
    raw[3 * 64 + 60:3 * 64 + 62] = bytes((128, 127))
    raw[3 * 64 + 56:3 * 64 + 58] = bytes((0, 1))
    struct.pack_into('<6H', raw, 6 * 64, 0, 1, 2, 3, 4, 5)
    audited = {'bone_names': ['root', 'magazine', 'chainsaw'],
               'lod0_skin': {'vertex_count': 6, 'vertex_offset_in_export': 0,
                             'index_count': 6, 'index_offset_in_export': 6 * 64,
                             'chunks': [
                                 {'first_vertex': 0, 'last_vertex_exclusive': 3,
                                  'bone_map': [1, 0]},
                                 {'first_vertex': 3, 'last_vertex_exclusive': 6,
                                  'bone_map': [2, 0]}],
                             'sections': [
                                 {'first_index': 0, 'triangles': 1, 'material_index': 0},
                                 {'first_index': 3, 'triangles': 1, 'material_index': 1}]}}
    return raw, audited


class RigidBoneExtractionTests(unittest.TestCase):
    def test_parent_local_reference_pose_composes_into_mesh_space(self):
        quarter_turn = math.sqrt(.5)
        audited = {'bone_names': ['root', 'body', 'hinge'], 'reference_bones': [
            {'bone': 'root', 'parent_index': 0, 'position_parent_local_raw': (10., 20., 0.),
             'orientation_xyzw': (0., 0., quarter_turn, quarter_turn)},
            {'bone': 'body', 'parent_index': 0, 'position_parent_local_raw': (2., 0., 0.),
             'orientation_xyzw': (0., 0., 0., 1.)},
            {'bone': 'hinge', 'parent_index': 1, 'position_parent_local_raw': (0., 3., 0.),
             'orientation_xyzw': (0., 0., 0., 1.)}]}
        result = mesh_space_reference_pivot(audited, 'hinge')
        self.assertAlmostEqual(result[0], 7.)
        self.assertAlmostEqual(result[1], 22.)
        self.assertAlmostEqual(result[2], 0.)

    def test_target_is_extracted_with_primary_uv_despite_other_mixed_chunk(self):
        raw, audited = fixture()
        result = extract(raw, audited, 'magazine')
        self.assertEqual(result['used_vertices'], [0, 1, 2])
        self.assertEqual(len(result['faces']), 1)
        self.assertEqual(result['remainder_used_vertices'], [3, 4, 5])
        self.assertEqual(len(result['remainder_faces']), 1)
        self.assertEqual((len(result['faces']) + len(result['remainder_faces'])) * 3, 6)
        self.assertEqual(result['material_face_counts'], {0: 1})
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'part.obj'
            write_obj(path, result, 'deadbeef', 'magazine')
            obj = path.read_text(encoding='ascii')
            self.assertEqual(obj.count('\nvt '), 3)
            self.assertEqual(obj.count('\nvn '), 3)
            self.assertIn('vt 0.100000001 0.0500000007', obj)
            self.assertIn('vn -0.00392150832 -0.00392150832 -0.999984622', obj)
            self.assertIn('f 3/3/3 2/2/2 1/1/1', obj)
            remainder_path = pathlib.Path(directory) / 'remainder.obj'
            write_obj(remainder_path, result, 'deadbeef', 'magazine', remainder=True)
            remainder_obj = remainder_path.read_text(encoding='ascii')
            self.assertEqual(remainder_obj.count('\nf '), 1)
            self.assertIn('# part remainder', remainder_obj)

    def test_target_mixed_weight_and_crossing_face_fail_closed(self):
        raw, audited = fixture()
        raw[0 * 64 + 56:0 * 64 + 58] = bytes((0, 1))
        raw[0 * 64 + 60:0 * 64 + 62] = bytes((128, 127))
        with self.assertRaisesRegex(ValueError, 'mixed-weight'):
            extract(raw, audited, 'magazine')
        raw, audited = fixture()
        struct.pack_into('<6H', raw, 6 * 64, 0, 1, 3, 2, 4, 5)
        with self.assertRaisesRegex(ValueError, 'crosses a triangle'):
            extract(raw, audited, 'magazine')


if __name__ == '__main__':
    unittest.main()
