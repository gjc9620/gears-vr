import dataclasses
import json
import math
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import armspan_calibration as calibration


VALID = dict(schema=1, armspan_metres=1.7, hands_below_head_metres=-.15)


def sample(sequence=1, received_ns=1_000_000_000, **changes):
    value = calibration.Sample(1, sequence, 1, 1, 1, received_ns,
                               (0., 1.7, 0.), 0., (-.85, 1.55, 0.), (.85, 1.55, 0.), (1., 1.))
    return dataclasses.replace(value, **changes)


def hold(target, start=1_000_000_000, first=1, count=201, **changes):
    result = None
    for index in range(count):
        now = start + index * 10_000_000
        result = target.observe(sample(first + index, now, **changes), now)
    return result


class MeasurementTests(unittest.TestCase):
    def test_two_seconds_unique_tickets_and_sixty_samples_are_both_required(self):
        measure = calibration.Measurement()
        self.assertIsNone(hold(measure, count=200))
        result = measure.observe(sample(201, 3_000_000_000), 3_000_000_000)
        self.assertAlmostEqual(result['armspan_metres'], 1.7)
        self.assertAlmostEqual(result['hands_below_head_metres'], -.15)
        slow = calibration.Measurement()
        for index in range(26):
            now = 1_000_000_000 + index * 80_000_000
            self.assertIsNone(slow.observe(sample(index+1, now), now))

    def test_duplicate_poll_does_not_pad_or_reset_hold(self):
        measure = calibration.Measurement()
        for index in range(201):
            now = 1_000_000_000 + index * 10_000_000
            ticket = sample(index+1, now)
            result = measure.observe(ticket, now)
            if index != 200:
                self.assertIsNone(measure.observe(ticket, now+1_000_000))
        self.assertIsNotNone(result)
        self.assertEqual(len(measure.spans), 201)

    def test_release_stale_nonfinite_bad_tpose_and_ticket_replay_reset(self):
        broken = [dict(squeeze=(1., .79)), dict(left=(0., 1.55, 0.)),
                  dict(right=(.85, 1.9, 0.)), dict(right=(.85, 1.55, .6)),
                  dict(yaw=float('nan')), dict(left=(float('inf'), 1.55, 0.))]
        for change in broken:
            with self.subTest(change=change):
                measure = calibration.Measurement()
                hold(measure, count=100)
                self.assertIsNone(measure.observe(sample(101, 2_000_000_000, **change), 2_000_000_000))
                self.assertEqual(measure.progress, 0)
                self.assertIsNone(hold(measure, start=2_010_000_000, first=102, count=200))
        for sequence, stamp, now in ((50, 2_000_000_000, 2_000_000_000),
                                     (101, 2_000_000_000, 2_100_000_001),
                                     (101, 2_000_000_001, 2_000_000_000)):
            with self.subTest(sequence=sequence, stamp=stamp, now=now):
                measure = calibration.Measurement()
                hold(measure, count=100)
                self.assertIsNone(measure.observe(sample(sequence, stamp), now))
                self.assertEqual(measure.progress, 0)

    def test_identity_gap_and_motion_restart_with_new_sample(self):
        for change in (dict(session=2), dict(space_epoch=2), dict(profile=2), dict(input_epoch=2),
                       dict(left=(-.75, 1.55, 0.))):
            with self.subTest(change=change):
                measure = calibration.Measurement()
                hold(measure, count=100)
                self.assertIsNone(measure.observe(sample(101, 2_000_000_000, **change), 2_000_000_000))
                self.assertEqual(len(measure.spans), 1)
                self.assertEqual(measure.progress, 0)
        measure = calibration.Measurement()
        hold(measure, count=100)
        self.assertIsNone(measure.observe(sample(101, 2_200_000_000), 2_200_000_000))
        self.assertEqual(len(measure.spans), 1)

    def test_measurement_is_heading_relative_and_latched(self):
        measure = calibration.Measurement()
        result = hold(measure, yaw=math.pi/2, head=(4., 1.7, 3.),
                      left=(4., 1.55, 3.85), right=(4., 1.55, 2.15))
        self.assertAlmostEqual(result['armspan_metres'], 1.7)
        self.assertEqual(measure.observe(None, 9_000_000_000), result)

    def test_unknown_or_noninteger_clock_cannot_start_hold(self):
        for now in (None, True, -1, float('nan'), 1e9):
            with self.subTest(now=now):
                measure = calibration.Measurement()
                self.assertIsNone(measure.observe(sample(), now))
                self.assertEqual(measure.progress, 0)


class PersistenceTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name)/'profile'/'weapon-calibration.json'

    def test_missing_and_invalid_records_require_calibration_without_overwriting(self):
        self.assertTrue(calibration.Session(self.path).required)
        self.path.parent.mkdir()
        invalid = [b'{', b'\xff', b'\xef\xbb\xbf'+json.dumps(VALID).encode(), b'null', b'{"schema":true,"armspan_metres":1.7,"hands_below_head_metres":-.15}',
                   b'{"schema":1,"armspan_metres":NaN,"hands_below_head_metres":-0.15}',
                   b'{"schema":1,"schema":1,"armspan_metres":1.7,"hands_below_head_metres":-0.15}',
                   json.dumps(dict(VALID, armspan_metres=.89)).encode(), json.dumps(dict(VALID, extra=1)).encode()]
        for content in invalid:
            with self.subTest(content=content):
                self.path.write_bytes(content)
                session = calibration.Session(self.path)
                self.assertEqual(session.previous.state, 'invalid')
                self.assertTrue(session.required)
                self.assertIsNone(session.value)
                self.assertEqual(self.path.read_bytes(), content)

    def test_save_is_immediate_once_and_valid_record_reused(self):
        session = calibration.Session(self.path)
        original_save = calibration.save
        with mock.patch.object(calibration, 'save', wraps=original_save) as save:
            self.assertIsNotNone(hold(session))
            saved = self.path.read_bytes()
            self.assertTrue(session.saved)
            self.assertEqual(calibration.load(self.path).state, 'valid')
            session.observe(sample(202, 3_010_000_000), 3_010_000_000)
            save.assert_called_once()
            reused = calibration.Session(self.path)
            self.assertFalse(reused.required)
            self.assertEqual(reused.observe(None, 9_000_000_000), reused.previous.value)
            self.assertEqual(self.path.read_bytes(), saved)
            self.assertEqual(save.call_count, 1)

    def test_failed_remeasure_preserves_valid_record(self):
        calibration.save(self.path, VALID, calibration.load(self.path))
        before = self.path.read_bytes()
        session = calibration.Session(self.path, remeasure=True)
        self.assertTrue(session.required)
        self.assertIsNone(hold(session, count=200))
        self.assertEqual(self.path.read_bytes(), before)
        self.assertFalse(session.saved)
        hold(session, start=3_010_000_000, first=202, count=201,
             left=(-.95, 1.55, 0.), right=(.95, 1.55, 0.))
        self.assertAlmostEqual(calibration.load(self.path).value['armspan_metres'], 1.9)

    def test_replace_or_flush_failure_preserves_old_bytes_and_cleans_temp(self):
        calibration.save(self.path, VALID, calibration.load(self.path))
        for function in ('replace', 'fsync'):
            before = self.path.read_bytes()
            with self.subTest(function=function), mock.patch.object(calibration.os, function, side_effect=OSError('fixture')):
                with self.assertRaisesRegex(OSError, 'fixture'):
                    calibration.save(self.path, dict(VALID, armspan_metres=1.9), calibration.load(self.path))
            self.assertEqual(self.path.read_bytes(), before)
            self.assertEqual(list(self.path.parent.glob('*.tmp')), [])

    def test_invalid_values_and_external_replacement_never_overwrite(self):
        previous = calibration.load(self.path)
        for value in (dict(VALID, schema=True), dict(VALID, armspan_metres=True),
                      dict(VALID, armspan_metres=float('nan')), dict(VALID, hands_below_head_metres=-.351)):
            with self.subTest(value=value), self.assertRaises(ValueError):
                calibration.save(self.path, value, previous)
        self.assertFalse(self.path.exists())
        calibration.save(self.path, VALID, previous)
        before = self.path.read_bytes()
        with self.assertRaisesRegex(RuntimeError, 'another session'):
            calibration.save(self.path, dict(VALID, armspan_metres=1.9), previous)
        self.assertEqual(self.path.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
