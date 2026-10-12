import unittest
import test_submit_pair_analysis as fixture
from analyze_submit_duration import analyze


class DurationEvidenceTest(unittest.TestCase):
    setUp=fixture.SubmitEvidenceTest.setUp

    def report(self,status=None,mono=False,marker=77):
        # Populate a reused root across 20 independent construction lifetimes.
        self.rows=self.rows[:100]
        for i in range(20):
            for j,kind in enumerate(('9','10','12','11')):
                self.rows.append(dict(kind=kind,object='0x4000',qpc=str(200+i*250+j),family_count='2',renderer_count='2',result='0x0'))
        self.rows.append(dict(kind='9',object='0x4000',qpc='6000',family_count='1',renderer_count='1',result='0x0'))
        base='qpc_frequency=1000\n'+self.status.replace('consumed=1 prepared=1 destroyed=1','consumed=20 prepared=20 destroyed=20')
        base+='submit_lifetime seconds=5 total=20 closed=1 pending=0 retained=0 skipped=0\n'
        if mono:
            base+='submit_view_count=1 submit_event_marker=77\n'
            for row in self.rows:
                row['test_state']=str(marker) if row['family_count']=='2' else '0'
                if row['family_count']=='2':
                    row['family_count']=row['renderer_count']='1'
        status=status(base) if status else base
        # Reuse the fixture's CSV writer; its single-frame verdict is irrelevant.
        fixture.SubmitEvidenceTest.report(self,status)
        return analyze(status,{'pid':123,'all_match':True},self.csv)

    def test_complete_reused_roots(self):
        self.assertTrue(self.report()['passed'])

    def test_marked_mono_excludes_ordinary_single_view(self):
        report=self.report(mono=True)
        self.assertTrue(report['passed'])
        self.assertEqual(report['csv_pair_events']['9'],20)

    def test_missing_mono_marker_rejects_ordinary_records(self):
        self.assertFalse(self.report(mono=True,marker=0)['passed'])

    def test_unfinished_or_retained(self):
        for before,after in (('pending=0','pending=1'),('retained=0','retained=1'),
            ('closed=1','closed=0'),('destroyed=20','destroyed=19'),('seconds=5 total','seconds=1 total')):
            self.assertFalse(self.report(lambda status:status.replace(before,after))['passed'])


if __name__=='__main__':unittest.main()
