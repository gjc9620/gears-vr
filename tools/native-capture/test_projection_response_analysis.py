"""Synthetic pixel fixtures exercise registration successes and failures."""
import unittest

import numpy as np
from PIL import Image

from analyze_projection_response import measure


class ProjectionResponseTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rng = np.random.default_rng(81024)
        # Independent multiscale texture with spatial landmarks; not an array
        # of coded expected coordinates. Pillow performs fixture resampling.
        fine = rng.uniform(20, 235, (180, 240)).astype('uint8')
        coarse = Image.fromarray(rng.uniform(20, 235, (45, 60)).astype('uint8')).resize((240, 180), Image.Resampling.BICUBIC)
        cls.image = Image.fromarray((.35*fine+.65*np.array(coarse)).astype('uint8'))
        cls.source = np.asarray(cls.image, dtype=float)
        cls.regions = [('static', (12, 12, 228, 168))]

    def render(self, scale=1, shift=0, vertical=0):
        # Inverse resampling via Pillow, distinct from analyzer's sampler.
        return np.asarray(self.image.transform(self.image.size, Image.Transform.AFFINE,
                          (1/scale, 0, 120-(120+shift)/scale, 0, 1, -vertical),
                          Image.Resampling.BICUBIC), dtype=float)

    def test_zoom_and_wrong_scale(self):
        good = measure(self.source, self.render(2), self.regions, (2., 0.))
        wrong = measure(self.source, self.render(1.5), self.regions, (2., 0.))
        identity = measure(self.source, self.source, self.regions, (2., 0.))
        self.assertTrue(good['expected_response_supported'], good)
        self.assertAlmostEqual(good['observed_fit']['scale'], 2, delta=.075)
        self.assertFalse(wrong['expected_response_supported'])
        self.assertAlmostEqual(wrong['observed_fit']['scale'], 1.5, delta=.075)
        self.assertFalse(identity['expected_response_supported'])
        self.assertGreater(good['expected_minus_identity_ncc'], .2)

    def test_shift_wrong_sign_and_restoration(self):
        good = measure(self.source, self.render(shift=12), self.regions, (1., 12.))
        wrong = measure(self.source, self.render(shift=-12), self.regions, (1., 12.))
        restored = measure(self.source, self.source*.9+8, self.regions, (1., 0.))
        self.assertTrue(good['expected_response_supported'], good)
        self.assertAlmostEqual(good['observed_fit']['shift_pixels'], 12, delta=1)
        self.assertFalse(wrong['expected_response_supported'])
        self.assertLess(wrong['observed_fit']['shift_pixels'], -10)
        self.assertTrue(restored['expected_response_supported'], restored)

    def test_vertical_motion_and_blank_are_not_passes(self):
        moved = measure(self.source, self.render(shift=12, vertical=3), self.regions, (1., 12.))
        blank = measure(np.zeros_like(self.source), self.source, self.regions, (2., 0.))
        self.assertFalse(moved['expected_response_supported'])
        self.assertFalse(blank['reliable'])


if __name__ == '__main__':
    unittest.main()
