import csv
import pathlib
import tempfile
import unittest
from analyze_submit_pair import analyze


class SubmitEvidenceTest(unittest.TestCase):
    def setUp(self):
        self.folder=tempfile.TemporaryDirectory();self.addCleanup(self.folder.cleanup)
        self.csv=pathlib.Path(self.folder.name)/'capture.csv'
        self.status=('capturing pid=123 seconds=12\n'
            'stopped disable_status=0 active=0 dropped=0\n'
            'submit_pair_enabled=1 attempted=2 root=0x4000 owned=1 sources_released=1 original_preserved=1 consumed=1 prepared=1 destroyed=1\n'
            'shadow_state_enabled=1 attempted=1 created=1 release_returned=1 destroyed=1 original_preserved=1 flags=1 thread=42 caller=0x33bc69 temporary=0x6000\n')
        self.rows=[dict(kind='3',object='0x8000',qpc=str(i),family_count='0',renderer_count='0',result='0x0') for i in range(100)]
        for i,kind in enumerate(('9','10','12','11')):
            self.rows.append(dict(kind=kind,object='0x4000',qpc=str(200+i),family_count='2',renderer_count='2',result='0x0'))
        self.rows.append(dict(kind='9',object='0x4000',qpc='210',family_count='1',renderer_count='1',result='0x0'))

    def report(self,status=None,restored=None):
        with self.csv.open('w',newline='') as f:
            writer=csv.DictWriter(f,fieldnames=self.rows[0].keys());writer.writeheader();writer.writerows(self.rows)
        return analyze(status or self.status,restored or {'pid':123,'all_match':True},self.csv)

    def test_complete_and_reused_root(self):
        self.assertTrue(self.report()['passed'])
        self.assertFalse(self.report(restored={'pid':124,'all_match':True})['passed'])

    def test_missing_lifecycle_and_release(self):
        self.rows=[r for r in self.rows if r['kind']!='12']
        self.assertFalse(self.report()['passed'])
        self.assertFalse(self.report(self.status.replace('release_returned=1','release_returned=0'))['passed'])

    def test_present_failure(self):
        self.rows[0]['result']='0x887a0005'
        self.assertFalse(self.report()['passed'])

    def test_rejection_is_never_a_submission_pass(self):
        status=self.status.replace('attempted=2 root=0x4000','attempted=3 root=0x0').replace('consumed=1 prepared=1 destroyed=1','consumed=0 prepared=0 destroyed=0')
        report=self.report(status)
        self.assertTrue(report['safe_rejection']);self.assertFalse(report['passed'])


if __name__=='__main__':unittest.main()
