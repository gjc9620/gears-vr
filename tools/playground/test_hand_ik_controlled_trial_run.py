"""Reject partial or overstated controlled-IK private retail receipts."""
import unittest

from hand_ik_controlled_trial_run import parse_end


VALID = ('hand_ik_controlled_trial_end status=5 completed_windows=6 '
         'evidence_complete=1 restored=1 setter_calls=12 effector_writes=12 '
         'effect_evidence=0 inconclusive=1 safe_to_unload=1 reason=raw_evidence_only')


class TerminalTest(unittest.TestCase):
    def test_complete_raw_evidence_is_still_inconclusive(self):
        row = parse_end(VALID)
        self.assertEqual(row['completed_windows'], 6)
        self.assertEqual(row['effect_evidence'], 0)
        self.assertEqual(row['inconclusive'], 1)

    def test_partial_or_unsafe_evidence_is_rejected(self):
        for original, replacement in (
            ('completed_windows=6', 'completed_windows=5'),
            ('evidence_complete=1', 'evidence_complete=0'),
            ('restored=1', 'restored=0'),
            ('setter_calls=12', 'setter_calls=11'),
            ('effect_evidence=0', 'effect_evidence=1'),
            ('safe_to_unload=1', 'safe_to_unload=0'),
            ('status=5', 'status=4'),
        ):
            with self.subTest(original=original), self.assertRaises(RuntimeError):
                parse_end(VALID.replace(original, replacement))

    def test_duplicate_terminal_or_field_is_rejected(self):
        for value in (VALID + '\n' + VALID, VALID + ' status=5'):
            with self.subTest(value=value), self.assertRaises(RuntimeError):
                parse_end(value)


if __name__ == '__main__':
    unittest.main()
