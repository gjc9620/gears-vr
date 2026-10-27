import unittest
import struct

from split_weighted_lod0 import (area, clip, component_bounds,
                                 connected_components, partition, read_lod0)


class WeightedPartitionTests(unittest.TestCase):
    def test_triangle_crossing_magazine_seam_keeps_full_surface(self):
        face = (((0., 0., 0.), 0.),
                ((1., 0., 0.), 1.),
                ((0., 1., 0.), 1.))
        detachable = clip(face, True)
        frame = clip(face, False)
        self.assertEqual((len(detachable), len(frame)), (2, 1))
        self.assertAlmostEqual(sum(map(area, detachable + frame)), .5)
        shared = {vertex for triangle in detachable for vertex in triangle} & {
            vertex for triangle in frame for vertex in triangle}
        self.assertEqual(shared, {(.5, 0., 0.), (0., .5, 0.)})

    def test_exact_threshold_and_rigid_triangles(self):
        positions = [(0., 0., 0.), (1., 0., 0.), (0., 1., 0.),
                     (2., 0., 0.), (3., 0., 0.), (2., 1., 0.)]
        parts, counts = partition(positions, [0., .5, 1., 1., 1., 1.],
                                  (0, 1, 2, 3, 4, 5))
        self.assertEqual(counts['source_triangles'], 2)
        self.assertAlmostEqual(counts['source_surface_area_raw_squared_units'], 1.)
        self.assertAlmostEqual(counts['split_surface_area_raw_squared_units'], 1.)
        self.assertEqual(len(parts['frame']), 1)
        self.assertEqual(len(parts['detachable']), 2)

    def test_fail_closed_on_bad_index_or_missing_part(self):
        vertices = [(0., 0., 0.), (1., 0., 0.), (0., 1., 0.)]
        with self.assertRaisesRegex(ValueError, 'vertex buffer'):
            partition(vertices, [0., 1., 1.], (0, 1, 3))
        with self.assertRaisesRegex(ValueError, 'both'):
            partition(vertices, [1., 1., 1.], (0, 1, 2))

    def test_separate_disconnected_and_flat_candidate_without_deleting_surface(self):
        broad = ((0., 0., 0.), (1., 0., 0.), (0., 1., 1.))
        flat = ((4., 0., 0.), (4., 1., 0.), (4., 0., 1.))
        groups = connected_components([flat, broad])
        self.assertEqual([len(group) for group in groups], [1, 1])
        self.assertEqual(sum(map(len, groups)), 2)
        self.assertEqual(component_bounds(groups[0])['near_zero_thickness_axes'], [0])
        with self.assertRaisesRegex(ValueError, 'non-finite'):
            connected_components([((float('nan'), 0., 0.), *broad[:2])])

    def test_same_local_bone_index_maps_differently_in_two_chunks(self):
        positions = ((0., 0., 0.), (1., 0., 0.), (0., 1., 0.),
                     (2., 0., 0.), (3., 0., 0.), (2., 1., 0.))
        raw = bytearray(6 * 64 + 6 * 2)
        for vertex, position in enumerate(positions):
            at = vertex * 64
            struct.pack_into('<3f', raw, at, *position)
            raw[at + 56] = 0  # Each chunk uses local BoneMap index zero.
            raw[at + 60] = 255
        struct.pack_into('<6H', raw, 6 * 64, 0, 1, 2, 3, 4, 5)
        audited = {'bone_names': ['root', 'magazine', 'chainsaw_blur'],
                   'lod0_skin': {'vertex_count': 6, 'index_count': 6,
                                 'vertex_offset_in_export': 0,
                                 'index_offset_in_export': 6 * 64,
                                 'chunks': [
                                     {'first_vertex': 0, 'last_vertex_exclusive': 3,
                                      'bone_map': [1, 0]},
                                     {'first_vertex': 3, 'last_vertex_exclusive': 6,
                                      'bone_map': [2, 0]}]}}
        _, weights, _ = read_lod0(raw, audited, 'magazine')
        self.assertEqual(weights, [1., 1., 1., 0., 0., 0.])
        parts, counts = partition(positions, weights, (0, 1, 2, 3, 4, 5))
        self.assertEqual(counts['seam_triangles'], 0)
        self.assertEqual((len(parts['detachable']), len(parts['frame'])), (1, 1))


if __name__ == '__main__':
    unittest.main()
