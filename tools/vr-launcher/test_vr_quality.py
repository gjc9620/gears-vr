"""Pure profile/capability/limit tests; no game, native module, or XR calls."""
from dataclasses import FrozenInstanceError
import unittest
import vr_quality as quality


class VrQualityTests(unittest.TestCase):
    def test_exact_two_tiers_and_integer_sbs_arithmetic(self):
        for tier, eye, source, pixels in (
                ('low', 2048, 4096, 8388608), ('high', 4096, 8192, 33554432)):
            profile = quality.resolve_quality({'vr_quality': tier}, {'fixed_vr_resolution': True})
            self.assertEqual((profile.eye_width, profile.eye_height), (eye, eye))
            self.assertEqual((profile.source_width, profile.source_height, profile.source_pixels), (source, eye, pixels))
            self.assertIs(type(profile.source_pixels), int)
            self.assertLessEqual(profile.source_width, quality.MAX_SOURCE_EXTENT)
            with self.assertRaises(FrozenInstanceError):
                profile.quality = 'high'

    def test_release_default_low_and_legacy_absent_unchanged(self):
        self.assertEqual(quality.resolve_quality({}, {'fixed_vr_resolution': True}).quality, 'low')
        for manifest in ({}, {'fixed_vr_resolution': False}):
            self.assertIsNone(quality.resolve_quality({}, manifest))

    def test_explicit_requests_require_build_capability(self):
        for tier in ('low', 'high'):
            for manifest in ({}, {'fixed_vr_resolution': False}):
                config = {'vr_quality': tier, 'game_path': 'untouched'}
                with self.assertRaisesRegex(ValueError, 'does not support independent VR resolution'):
                    quality.resolve_quality(config, manifest)
                self.assertEqual(config, {'vr_quality': tier, 'game_path': 'untouched'})

    def test_strict_tier_and_boolean_capability(self):
        for value in ('Low', 'HIGH', 'medium', 'auto', ' low', 'low ', '', None, True, 2048, {}, []):
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, 'exactly'):
                quality.resolve_quality({'vr_quality': value}, {'fixed_vr_resolution': True})
        for value in ('true', 1, 0, None, []):
            with self.subTest(capability=value), self.assertRaisesRegex(ValueError, 'boolean'):
                quality.resolve_quality({}, {'fixed_vr_resolution': value})

    def test_high_uses_per_eye_system_limits_not_sbs_width(self):
        limits = quality.VrRuntimeLimits(4096, 4096, (4096, 4096), (4096, 4096))
        quality.require_size_support(quality.VrQualityProfile('high'), limits)
        quality.require_size_support(quality.VrQualityProfile('low'), limits)
        for values in (
                (4095, 4096, (4096, 4096), (4096, 4096)),
                (4096, 4095, (4096, 4096), (4096, 4096)),
                (4096, 4096, (4095, 4096), (4096, 4096)),
                (4096, 4096, (4096, 4095), (4096, 4096)),
                (4096, 4096, (4096, 4096), (4095, 4096)),
                (4096, 4096, (4096, 4096), (4096, 4095))):
            with self.subTest(values=values), self.assertRaisesRegex(ValueError, 'exceeds advertised'):
                quality.require_size_support(quality.VrQualityProfile('high'), quality.VrRuntimeLimits(*values))

    def test_limits_are_exact_positive_integers_and_immutable_pairs(self):
        for value in (False, 0, -1, 4096., '4096', 0x100000000):
            with self.subTest(value=value), self.assertRaises(ValueError):
                quality.VrRuntimeLimits(value, 4096, (4096, 4096), (4096, 4096))
        for widths in ([4096, 4096], (), (4096,), (4096, 4096, 4096), (4096, True)):
            with self.subTest(widths=widths), self.assertRaises(ValueError):
                quality.VrRuntimeLimits(4096, 4096, widths, (4096, 4096))

    def test_launch_once_guard_rejects_hot_switch_and_legacy_upgrade(self):
        low, high = quality.VrQualityProfile('low'), quality.VrQualityProfile('high')
        quality.require_same_launch_profile(low, quality.VrQualityProfile('low'))
        quality.require_same_launch_profile(None, None)
        for active, requested in ((low, high), (high, low), (None, low), (high, None)):
            with self.subTest(active=active, requested=requested), self.assertRaisesRegex(ValueError, 'stop and restart'):
                quality.require_same_launch_profile(active, requested)


if __name__ == '__main__':
    unittest.main()
