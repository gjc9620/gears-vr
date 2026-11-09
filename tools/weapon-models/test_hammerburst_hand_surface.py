"""Regression for the actual HB fitting omission of mixed forearm/twist skin."""
import unittest
import numpy as np
from hammerburst_hand_surface import evaluate


class MixedHandSkinTest(unittest.TestCase):
    def setUp(self):
        self.vertices = np.asarray([[0., 0, 0], [1, 0, 0], [0, 1, 0], [0, 0, 1], [2, 0, 0]])
        self.indices = np.zeros((5, 4), dtype=int)
        self.indices[:, 0] = [11, 11, 11, 29, 10]
        self.indices[3, 1] = 11
        self.weights = np.zeros((5, 4))
        self.weights[:, 0] = 1
        self.weights[3] = [.75, .25, 0, 0]
        self.triangles = np.asarray([[0, 1, 2], [1, 2, 3], [3, 2, 4]])
        self.bind_inverse = np.repeat(np.eye(4)[None], 115, axis=0)
        self.palette = self.bind_inverse.copy()

    def skin(self):
        return evaluate(self.vertices, self.indices, self.weights, self.triangles,
                        self.bind_inverse, self.palette)

    def test_mixed_palm_faces_are_retained(self):
        surface = self.skin()
        self.assertEqual(surface.strict_hand_triangle_count, 1)
        self.assertEqual(surface.mixed_triangle_count, 2)
        np.testing.assert_array_equal(surface.original_triangle_indices, [0, 1, 2])

    def test_original_forearm_twist_weight_changes_skin(self):
        self.palette[29, 0, 3] = 4
        surface = self.skin()
        np.testing.assert_allclose(surface.vertices[3], [3, 0, 1], atol=1e-12)
        np.testing.assert_array_equal(surface.vertices[:3], self.vertices[:3])

    def test_identity_palette_preserves_original_geometry(self):
        np.testing.assert_array_equal(self.skin().vertices, self.vertices)

    def test_no_implicit_reference_for_missing_parent(self):
        self.palette = self.palette[:114]
        with self.assertRaisesRegex(ValueError, "Complete115"):
            self.skin()

    def test_nonfinite_parent_rejected(self):
        self.palette[29, 0, 0] = np.nan
        with self.assertRaisesRegex(ValueError, "Nonfinite"):
            self.skin()

    def test_original_weights_are_never_renormalized(self):
        self.weights[3, 0] = .5
        with self.assertRaisesRegex(ValueError, "no renormalization"):
            self.skin()


if __name__ == "__main__":
    unittest.main()
