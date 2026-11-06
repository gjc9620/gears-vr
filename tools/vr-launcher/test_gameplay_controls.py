import unittest
import launcher

class GameplayControls(unittest.TestCase):
    def test_unified_first_person_ignores_old_disabled_preview_preference(self):
        manifest=dict(first_person_preview=True,sixdof_manual=True,anchor_observer=True,
                      cinematic_camera=True,crosshair_assignment_observer=True)
        self.assertTrue(launcher.gameplay_controls(dict(first_person_preview=False),manifest)['first_person_preview'])
        self.assertFalse(launcher.gameplay_controls(dict(first_person_preview=False),dict(manifest,sixdof_manual=False))['first_person_preview'])
    def setUp(self):
        self.manifest=dict(first_person_preview=True,right_stick_deadzone_control=True,
                           anchor_observer=True,cinematic_camera=True,crosshair_assignment_observer=True)
    def test_defaults_and_explicit_disable(self):
        self.assertEqual(launcher.gameplay_controls({},self.manifest),dict(first_person_preview=True,right_stick_deadzone_percent=2.))
        self.assertFalse(launcher.gameplay_controls(dict(first_person_preview=False),self.manifest)['first_person_preview'])
        self.assertEqual(launcher.gameplay_controls({},{}),dict(first_person_preview=False))
    def test_invalid_settings(self):
        for value in [True,'2',None,-1,50.01,float('inf'),float('nan')]:
            with self.subTest(value=value),self.assertRaises(ValueError):
                launcher.gameplay_controls(dict(right_stick_deadzone_percent=value),self.manifest)
        for value in [1,'true',None]:
            with self.assertRaises(ValueError):launcher.gameplay_controls(dict(first_person_preview=value),self.manifest)
    def test_shared_preferences_remain_usable_with_frozen_older_build(self):
        self.assertEqual(launcher.gameplay_controls(dict(first_person_preview=True,right_stick_deadzone_percent=2),{}),
                         dict(first_person_preview=False))
        broken=dict(self.manifest,cinematic_camera=False)
        with self.assertRaises(ValueError):launcher.gameplay_controls(dict(first_person_preview=True),broken)
    def test_run_camera_requires_explicit_supported_manifest(self):
        enabled=launcher.gameplay_controls({},dict(self.manifest,run_camera_stabilization=True))
        self.assertIs(enabled['run_camera_stabilization'],True)
        self.assertNotIn('run_camera_stabilization',launcher.gameplay_controls({},self.manifest))
        self.assertNotIn('run_camera_stabilization',launcher.gameplay_controls({},dict(self.manifest,run_camera_stabilization=False)))
        for value in (None,1,'true'):
            with self.subTest(value=value),self.assertRaises(ValueError):
                launcher.gameplay_controls({},dict(self.manifest,run_camera_stabilization=value))
    def test_lancer_ranges_only_forward_to_supporting_build(self):
        supported=dict(self.manifest,lancer_interaction_ranges=True)
        self.assertIs(launcher.gameplay_controls({},supported)['lancer_interaction_ranges'],False)
        self.assertIs(launcher.gameplay_controls(dict(lancer_interaction_ranges=True),supported)['lancer_interaction_ranges'],True)
        self.assertNotIn('lancer_interaction_ranges',launcher.gameplay_controls(dict(lancer_interaction_ranges=True),self.manifest))
    def test_lancer_ranges_reject_non_boolean(self):
        for value in (None,1,'true'):
            with self.subTest(value=value),self.assertRaises(ValueError):
                launcher.gameplay_controls(dict(lancer_interaction_ranges=value),self.manifest)
            with self.subTest(capability=value),self.assertRaises(ValueError):
                launcher.gameplay_controls({},dict(self.manifest,lancer_interaction_ranges=value))

if __name__=='__main__':unittest.main()
