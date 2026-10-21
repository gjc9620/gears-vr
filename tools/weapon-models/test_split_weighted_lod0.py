import unittest

from split_weighted_lod0 import area, clip, partition


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


if __name__ == '__main__':
    unittest.main()
