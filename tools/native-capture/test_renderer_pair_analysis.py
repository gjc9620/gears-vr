import unittest
from test_eye_analysis import EyeEvidenceTest
from analyze_renderer_pair import analyze


class PairEvidenceTest(unittest.TestCase):
    status=EyeEvidenceTest.status
    setUp=EyeEvidenceTest.setUp
    def test_pair_evidence(self):
        complete=self.status+'renderer_pair_enabled=1 checks=255 renderer=0x4000 retained=0\n'
        self.assertTrue(analyze(complete,{'pid':123,'all_match':True},self.csv)['passed'])
        for before,after in (('checks=255 renderer','checks=191 renderer'),('retained=0','retained=1'),
                             ('renderer=0x4000','renderer=0x0'),('pair_enabled=1','pair_enabled=0')):
            self.assertFalse(analyze(complete.replace(before,after),{'pid':123,'all_match':True},self.csv)['passed'])
        self.assertFalse(analyze(complete,{'pid':124,'all_match':True},self.csv)['passed'])


if __name__=='__main__':unittest.main()
