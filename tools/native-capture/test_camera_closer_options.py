"""Offline camera setting validation and runner/INI propagation."""
import unittest
from types import SimpleNamespace
from camera_closer_options import camera_closer_metres
import start_capture

class CameraCloserTests(unittest.TestCase):
    def args(self,**values):
        return SimpleNamespace(anchor_observer=True,cinematic_camera=True,continuous_session=True,stereo_xr=True,**values)
    def test_default_and_propagation(self):
        self.assertEqual(camera_closer_metres(SimpleNamespace()),0)
        self.assertEqual(start_capture.hud_size_ini(self.args(camera_closer_metres=.3)),'camera_closer_metres=0.29999999999999999\n')
    def test_reject_before_ini(self):
        for value in (True,None,'0.3',-.01,.51,float('nan'),float('inf')):
            with self.subTest(value=value),self.assertRaises(ValueError):camera_closer_metres(self.args(camera_closer_metres=value))
        for field in ('anchor_observer','cinematic_camera','continuous_session','stereo_xr'):
            args=self.args(camera_closer_metres=.3);setattr(args,field,False)
            with self.subTest(field=field),self.assertRaises(ValueError):start_capture.hud_size_ini(args)
        with self.assertRaises(ValueError):camera_closer_metres(self.args(camera_closer_metres=.3,origin_trial=True))

if __name__=='__main__':unittest.main()
