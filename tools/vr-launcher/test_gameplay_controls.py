import unittest
import launcher

class GameplayControls(unittest.TestCase):
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

if __name__=='__main__':unittest.main()
