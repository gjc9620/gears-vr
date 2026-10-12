"""Reject incomplete, cross-process and nonfinite reconstruction evidence."""
import unittest
from analyze_constructor_shadow import analyze


class ConstructorEvidenceTest(unittest.TestCase):
    status = ('capturing pid=123 seconds=20\n'
              'stopped disable_status=0 active=0\n'
              'shadow_view_enabled=1 attempted=2 stage=6 checks=255 nonempty_arrays=2 source=0x1000 temporary=0x2000\n'
              'constructor_shadow_enabled=1 derived_finite=1 derived_max_error=0\n')

    def test_complete(self):
        self.assertTrue(analyze(self.status, {'pid': 123, 'all_match': True})['passed'])

    def test_wrong_process(self):
        self.assertFalse(analyze(self.status, {'pid': 124, 'all_match': True})['passed'])

    def test_state_mode(self):
        status=self.status+'constructor_state_enabled=1\n'
        self.assertFalse(analyze(status, {'pid': 123, 'all_match': True})['passed'])
        self.assertTrue(analyze(status, {'pid': 123, 'all_match': True}, independent=True)['passed'])

    def test_matrix_errors(self):
        for error in ('nan', 'inf', '-1', '.01'):
            status = self.status.replace('derived_max_error=0', 'derived_max_error=' + error)
            self.assertFalse(analyze(status, {'pid': 123, 'all_match': True})['passed'])

    def test_unfinished_and_unrestored(self):
        self.assertFalse(analyze(self.status.replace('attempted=2', 'attempted=1'), {'pid': 123, 'all_match': True})['passed'])
        self.assertFalse(analyze(self.status, {'pid': 123, 'all_match': False})['passed'])

    def test_missing_or_duplicate(self):
        with self.assertRaises(ValueError):
            analyze(self.status.split('constructor_shadow_enabled=')[0], {'pid': 123, 'all_match': True})
        with self.assertRaises(ValueError):
            analyze(self.status + self.status, {'pid': 123, 'all_match': True})


if __name__ == '__main__':
    unittest.main()
