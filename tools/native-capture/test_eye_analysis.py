"""Keep eye arithmetic success separate from resource and submission claims."""
import pathlib
import tempfile
import unittest
from analyze_constructor_eye import analyze
from analyze_constructor_shadow import analyze as unchanged


class EyeEvidenceTest(unittest.TestCase):
    status=('capturing pid=123 seconds=20\n'
            'stopped disable_status=0 active=0\n'
            'shadow_view_enabled=1 attempted=2 stage=6 checks=255 nonempty_arrays=2 source=0x1000 temporary=0x2000\n'
            'constructor_shadow_enabled=1 derived_finite=1 derived_max_error=1.2\n'
            'constructor_state_enabled=1\n'
            'constructor_eye_enabled=1 shifted=1 combined_valid=1 combined_relative_error=1e-8\n'
            'shadow_state_enabled=1 attempted=1 created=1 release_returned=1 destroyed=1 original_preserved=1 flags=1 thread=10 caller=0x33bc69 temporary=0x3000\n')

    def setUp(self):
        self.folder=tempfile.TemporaryDirectory()
        self.csv=pathlib.Path(self.folder.name)/'capture.csv'
        self.csv.write_text('kind,thread,result\n7,10,0x0\n'+'3,20,0x0\n'*100)
        self.addCleanup(self.folder.cleanup)

    def test_complete(self):
        self.assertTrue(analyze(self.status,{'pid':123,'all_match':True},self.csv)['passed'])

    def test_bad_geometry(self):
        for value in ('nan','inf','-1','.001'):
            status=self.status.replace('combined_relative_error=1e-8','combined_relative_error='+value)
            self.assertFalse(analyze(status,{'pid':123,'all_match':True},self.csv)['passed'])

    def test_missing_shift_cleanup_or_present(self):
        for before,after in (('shifted=1','shifted=0'),('destroyed=1','destroyed=0'),
                             ('checks=255','checks=223'),('derived_finite=1','derived_finite=0'),
                             ('derived_max_error=1.2','derived_max_error=0')):
            self.assertFalse(analyze(self.status.replace(before,after),{'pid':123,'all_match':True},self.csv)['passed'])
        self.csv.write_text('kind,thread,result\n3,20,0x80004005\n'*1)
        self.assertFalse(analyze(self.status,{'pid':123,'all_match':True},self.csv)['passed'])

    def test_not_original_equivalence(self):
        status=self.status.replace('derived_max_error=1.2','derived_max_error=0')
        self.assertFalse(unchanged(status,{'pid':123,'all_match':True},independent=True)['passed'])


if __name__=='__main__':unittest.main()
