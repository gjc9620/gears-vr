import pathlib
import tempfile
import unittest

import weapon_calibration as calibration


class CalibrationTests(unittest.TestCase):
    def test_measurement_is_saved_and_failure_preserves_prior_value(self):
        line='controller_armspan_observer measured=1 progress=1 armspan_metres=1.72 hands_below_head_metres=-0.16 saved=0'
        measured=calibration.measured_from_native(line)
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'user'/'weapon-calibration.json'
            calibration.save(path,measured)
            self.assertEqual(calibration.load(path),measured)
            self.assertIsNone(calibration.measured_from_native(line.replace('measured=1','measured=0')))
            self.assertEqual(calibration.load(path),measured)
            with self.assertRaises(ValueError):calibration.save(path,dict(measured,armspan_metres=float('nan')))
            self.assertEqual(calibration.load(path),measured)

    def test_rejects_ambiguous_or_unbounded_native_measurement(self):
        line='controller_armspan_observer measured=1 progress=1 armspan_metres=1.7 hands_below_head_metres=-0.1 saved=0'
        with self.assertRaises(ValueError):calibration.measured_from_native(line+'\n'+line)
        with self.assertRaises(ValueError):calibration.measured_from_native(line.replace('armspan_metres=1.7','armspan_metres=9'))
        with self.assertRaises(ValueError):calibration.validate({'schema':True,'armspan_metres':1.7,'hands_below_head_metres':-.1})


if __name__=='__main__':unittest.main()
