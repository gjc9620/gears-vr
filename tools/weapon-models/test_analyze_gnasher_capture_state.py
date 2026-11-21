"""Numeric capture-state checks only; no process, Native or engine calls."""
from copy import deepcopy
import unittest
from analyze_gnasher_runtime import summarize

def fixture():
    identity=[1.,0,0,0,0,1.,0,0,0,0,1.,0,0,0,0,1.]
    return dict(passed=False,sampling_passed=True,shutdown_mode='launcher_owned',
        shutdown_deferred=True,launcher_cleanup_verified=False,animation='SG_Chamber',game_pid=123,
        samples=[dict(frame=i,main=identity[:],lever=identity[:]) for i in range(35)])

class CaptureState(unittest.TestCase):
    def test_sampled_parent_cleanup_remains_pending(self):
        result=summarize(fixture());self.assertTrue(result['launcher_cleanup_pending']);self.assertEqual(result['capture_pid'],123)
    def test_historical_completed_capture(self):
        capture=fixture();capture['passed']=True
        self.assertFalse(summarize(capture)['launcher_cleanup_pending'])
    def test_incomplete_or_unwitnessed_sampling_rejected(self):
        for key,value in [('sampling_passed',False),('sampling_passed',1),('shutdown_mode','probe_owned'),
                          ('shutdown_deferred',False),('launcher_cleanup_verified',None),('passed',None)]:
            with self.subTest(key=key,value=value):
                capture=deepcopy(fixture());capture[key]=value
                with self.assertRaises(ValueError):summarize(capture)
    def test_pending_cleanup_does_not_bypass_sample_integrity(self):
        capture=fixture();capture['samples'].pop()
        with self.assertRaises(ValueError):summarize(capture)
        capture=fixture();capture['samples'][3]['main'][0]=float('nan')
        with self.assertRaises(ValueError):summarize(capture)
    def bracket_fixture(self):
        capture=fixture();capture['capture_protocol']='bracketed_original_sg35_v1'
        for row in capture['samples']:
            row.update(main_after=row['main'][:],observed_node_time_seconds=.5,observed_node_time_after_seconds=.5)
        return capture
    def test_stable_separate_reads_are_explicitly_not_atomic(self):
        summary=summarize(self.bracket_fixture());self.assertTrue(summary['bracketed_original_reads'])
        self.assertFalse(summary['bone_reads_atomic']);self.assertEqual(summary['maximum_main_bracket_translation_raw'],0)
    def test_moving_main_or_node_rejects_reference(self):
        capture=self.bracket_fixture();capture['samples'][5]['main_after'][12]=.051
        with self.assertRaisesRegex(ValueError,'changed across'):summarize(capture)
        capture=self.bracket_fixture();capture['samples'][5]['observed_node_time_after_seconds']=.501
        with self.assertRaisesRegex(ValueError,'changed across'):summarize(capture)

if __name__=='__main__':unittest.main()
