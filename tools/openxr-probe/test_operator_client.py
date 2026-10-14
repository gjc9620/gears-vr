"""Offline Operator profile/cleanup checks; no proxy or XR process starts."""
import json
import math
import pathlib
import tempfile
import unittest
from unittest import mock
import operator_client as operator


BASE={'position':[1.,2.,3.],'orientation':[0.,math.sqrt(.5),0.,math.sqrt(.5)]}


class ProfileTests(unittest.TestCase):
    def test_call_clocks_remain_separate_on_success_and_failure(self):
        for fail in (False,True):
            with self.subTest(fail=fail),tempfile.TemporaryDirectory() as folder:
                motion=operator.OperatorMotion(pathlib.Path(folder)/'probe')
                motion.client=mock.Mock()
                if fail:motion.client.rpc.side_effect=RuntimeError('offline failure')
                else:motion.client.rpc.return_value={'content':[]}
                with mock.patch.object(operator.time,'monotonic',side_effect=[10.,11.]) as old_clock, \
                     mock.patch.object(operator.time,'perf_counter_ns',side_effect=[123456789,223456789]) as perf:
                    if fail:
                        with self.assertRaisesRegex(RuntimeError,'offline failure'):motion._call('openxr_get_head_pose',{})
                    else:motion._call('openxr_get_head_pose',{})
                action=motion.report['actions'][0]
                self.assertEqual((action['started_monotonic'],action['finished_monotonic']),(10.,11.))
                self.assertEqual((action['started_perf_counter_ns'],action['finished_perf_counter_ns']),(123456789,223456789))
                self.assertEqual(old_clock.call_count,2);self.assertEqual(perf.call_count,2)
                saved=json.loads(pathlib.Path(folder,'probe.operator.json').read_text())
                self.assertEqual(saved['timing_domain'],'perf_counter_ns')
                self.assertEqual(saved['perf_counter_clock']['implementation'],operator.time.get_clock_info('perf_counter').implementation)
                self.assertEqual('error' in action,fail)

    def test_default_profile_unchanged(self):
        targets=operator.motion_targets(BASE)
        self.assertEqual([t['label'] for t in targets],['yaw_+15','yaw_-15','local_x_+0.06m','local_x_-0.06m','return_initial'])
        self.assertEqual(targets[-1]['orientation'],BASE['orientation'])

    def test_independent_local_axes_use_left_multiplication(self):
        targets=operator.motion_targets(BASE,'sixdof')
        self.assertEqual(len(targets),13)
        # qX(10deg)*qY(90deg), analytically distinct from qY*qX.
        s,c=math.sin(math.radians(5)),math.cos(math.radians(5))
        expected=[s*math.sqrt(.5),c*math.sqrt(.5),s*math.sqrt(.5),c*math.sqrt(.5)]
        for a,b in zip(targets[2]['orientation'],expected):self.assertAlmostEqual(a,b)
        self.assertTrue(all(t['position']==BASE['position'] for t in targets[:6]))
        for i,t in enumerate(targets[6:12]):
            axis=i//2;offset=(.06,.04,.05)[axis]*(1 if i%2==0 else -1)
            self.assertEqual(t['orientation'],BASE['orientation'])
            self.assertAlmostEqual(t['position'][axis],BASE['position'][axis]+offset)
            self.assertTrue(all(t['position'][j]==BASE['position'][j] for j in range(3) if j!=axis))
        self.assertEqual(targets[-1],dict(label='return_initial',**BASE))

    def run_motion(self,fail_set=False,tracked=True):
        clock=[0.];calls=[]
        class Stop:
            def is_set(self):return False
            def wait(self,seconds):clock[0]+=seconds;return False
            def set(self):pass
        class Client:
            def rpc(self,method,params,timeout):
                name=params['name'];calls.append((name,params['arguments'],timeout))
                if name=='openxr_set_head_pose':
                    if fail_set:raise RuntimeError('set failed')
                    clock[0]+=3.5
                payload={'success':True}
                if name=='openxr_get_head_pose':payload={'pose':BASE,'flags':{k:tracked for k in
                    ('position_valid','orientation_valid','position_tracked','orientation_tracked')}}
                return {'content':[{'type':'text','text':json.dumps(payload)}]}
        with tempfile.TemporaryDirectory() as folder,mock.patch.object(operator.time,'monotonic',side_effect=lambda:clock[0]):
            motion=operator.OperatorMotion(pathlib.Path(folder)/'probe',total_timeout=170,profile='sixdof')
            motion.client=Client();motion.initial=BASE;motion.stop=Stop()
            motion.start_motion(lambda:True)
            if fail_set or not tracked:
                with self.assertRaises(RuntimeError):motion.finish(1)
            else:motion.finish(1)
            report=json.loads(pathlib.Path(folder,'probe.operator.json').read_text())
        return calls,report

    def test_thirteen_sequential_replies_labels_and_restore(self):
        calls,report=self.run_motion()
        self.assertEqual([c[0] for c in calls],['openxr_set_head_pose']*13+['openxr_release_input_devices','openxr_get_head_pose'])
        self.assertAlmostEqual(report['motion_elapsed_seconds'],45.5)
        self.assertTrue(report['success']);self.assertTrue(report['override_released'])
        actions=report['actions'][:13]
        self.assertEqual(len({a['motion_label'] for a in actions}),13)
        for a,b in zip(actions,actions[1:]):self.assertGreaterEqual(b['started_monotonic'],a['finished_monotonic'])
        self.assertTrue(all(a['arguments']['base_space']=='local' and a['arguments']['duration_seconds']==3.5
                            and a['finished_utc'] and a['raw_result'] for a in actions))

    def test_tool_failure_still_releases(self):
        calls,report=self.run_motion(fail_set=True)
        self.assertEqual([c[0] for c in calls],['openxr_set_head_pose','openxr_release_input_devices','openxr_get_head_pose'])
        self.assertFalse(report['success']);self.assertTrue(report['override_released'])

    def test_restored_untracked_flags_fail(self):
        _,report=self.run_motion(tracked=False)
        self.assertFalse(report['success']);self.assertIn('fully valid/tracked',report['cleanup_error'])


if __name__=='__main__':unittest.main()
