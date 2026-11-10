"""Regress the observed startup-only capture and fail before any action."""
import unittest
import json,tempfile
from pathlib import Path
from types import SimpleNamespace
from hammerburst_action_recording import start_action_recording,action_live
from hammerburst_isolated_operator import menu_source,Binding


class Tests(unittest.TestCase):
    def run_boundary(self, outcome, live=lambda: True):
        now = [0.]
        recorder = SimpleNamespace(capture_enabled=False, frames=[], errors=[], events=[])
        recorder.event = lambda *a, **k: recorder.events.append((a,k))
        def start():
            self.assertTrue(recorder.capture_enabled)
            if outcome == 'frame': recorder.frames.append({'file':'frame-00000.jpg'})
            if outcome == 'error': recorder.errors.append('No complete fresh projection frame')
        recorder.start = start
        def sleep(seconds): now[0] += seconds
        start_action_recording(recorder, live, clock=lambda: now[0], sleep=sleep)
        return recorder, now[0]

    def test_first_frame_allows_actions(self):
        rec, elapsed = self.run_boundary('frame')
        self.assertEqual(elapsed,0)
        self.assertEqual(rec.events[-1][0][0],'HB_first_action_capture_ready')

    def test_missing_capture_stops_at_existing_five_second_boundary(self):
        with self.assertRaisesRegex(TimeoutError,'no weapon action permitted'):
            self.run_boundary('missing')

    def test_original_capture_failure_cannot_be_ignored(self):
        with self.assertRaisesRegex(RuntimeError,'No complete fresh projection frame'):
            self.run_boundary('error')

    def test_lost_identity_stops_before_start(self):
        with self.assertRaisesRegex(AssertionError,'Owned game changed'):
            self.run_boundary('frame',live=lambda: False)

    def test_capture_failure_after_first_frame_stops_further_actions(self):
        binding=SimpleNamespace(live=lambda game: True)
        recorder=SimpleNamespace(frames=[{'file':'frame-00000.jpg'}],errors=[])
        self.assertTrue(action_live(binding,object(),recorder))
        recorder.errors.append('No complete fresh projection frame')
        with self.assertRaisesRegex(RuntimeError,'further actions stopped'):
            action_live(binding,object(),recorder)

    def test_capture_guard_keeps_original_identity_failure(self):
        binding=SimpleNamespace(live=lambda game: False)
        self.assertFalse(action_live(binding,object(),None))

    def test_real_adapter_starts_capture_after_handoff_and_before_actions(self):
        source=menu_source()
        call='start_action_recording(rec,live,projection_ready=lambda:BOUND.projection_ready(game))'
        self.assertLess(source.index("rec.event('native_hand_connection_ready'"),source.index(call))
        self.assertLess(source.index(call),source.index("head_rest=dict(zip"))
        self.assertIn("capture_enabled=False)",source)
        compile(source,'adapted HB operator','exec')

    def projection_boundary(self,ready_after=0,missing_capture=False,identity_lost=False):
        now=[0.];starts=[]
        rec=SimpleNamespace(capture_enabled=False,frames=[],errors=[],events=[])
        rec.event=lambda *a,**kw:rec.events.append((a,kw))
        def start():
            starts.append(now[0])
            if not missing_capture:rec.frames.append({'file':'actual.jpg'})
        rec.start=start
        try:
            start_action_recording(rec,lambda:not identity_lost or now[0]<1,
                projection_ready=lambda:now[0]>=ready_after,clock=lambda:now[0],sleep=lambda d:now.__setitem__(0,now[0]+d))
        except BaseException:
            self.last_boundary=(rec,now[0],starts);raise
        return rec,now[0],starts

    def test_wait_for_real_projection_before_starting_capture(self):
        rec,elapsed,starts=self.projection_boundary(ready_after=7)
        self.assertGreaterEqual(starts[0],7)
        self.assertLess(elapsed,7.1)
        self.assertEqual(rec.events[0][0][0],'HB_real_projection_ready')

    def test_missing_real_projection_never_starts_capture(self):
        with self.assertRaisesRegex(TimeoutError,'first real XR projection'):
            self.projection_boundary(ready_after=100)
        self.assertEqual(self.last_boundary[2],[])
        self.assertLess(self.last_boundary[1],15.1)

    def test_identity_loss_during_projection_wait_stops_before_capture(self):
        with self.assertRaisesRegex(AssertionError,'Owned game changed'):
            self.projection_boundary(ready_after=7,identity_lost=True)
        self.assertEqual(self.last_boundary[2],[])

    def test_projection_handoff_does_not_extend_five_second_capture_timeout(self):
        with self.assertRaisesRegex(TimeoutError,'No first action projection frame'):
            self.projection_boundary(ready_after=7,missing_capture=True)
        self.assertLess(self.last_boundary[1]-self.last_boundary[2][0],5.1)

    def test_exact_bound_existing_projection_receipt(self):
        with tempfile.TemporaryDirectory() as directory:
            binding=object.__new__(Binding);binding.report=Path(directory)/'report.json'
            game={'pid':10,'creation_time':11};xr={'pid':20,'creation_time':21}
            binding.pinned=({},game,xr);binding.continuous_session_id='owned';binding.live=lambda g:g==game
            good=dict(game_identity=game,xr_identity=xr,session_id='owned',source=1,
                startup_timings=dict(first_stereo_submission=dict(xr_monotonic_ns=900,timing_is_runner_observation=False,evidence='XR epoch1 actual projection successful xrEndFrame marker')))
            binding.report.write_text(json.dumps(good));self.assertTrue(binding.projection_ready(game,now=1000))
            for mutation in range(11):
                value=json.loads(json.dumps(good));timing=value['startup_timings']['first_stereo_submission']
                if mutation==0:value['game_identity']['creation_time']+=1
                if mutation==1:value['xr_identity']['creation_time']+=1
                if mutation==2:value['session_id']='other'
                if mutation==3:value['source']=0
                if mutation==4:value['startup_timings']={}
                if mutation==5:timing['xr_monotonic_ns']=0
                if mutation==6:timing['xr_monotonic_ns']=1001
                if mutation==7:timing['xr_monotonic_ns']=True
                if mutation==8:timing['timing_is_runner_observation']=True
                if mutation==9:timing['evidence']='native running'
                if mutation==10:timing['xr_monotonic_ns']='900'
                binding.report.write_text(json.dumps(value))
                with self.subTest(mutation=mutation):self.assertFalse(binding.projection_ready(game,now=1000))

    def test_missing_or_malformed_receipt_and_dead_owner_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            b=object.__new__(Binding);b.report=Path(directory)/'missing.json';b.pinned=({}, {}, {});b.live=lambda g:True
            self.assertFalse(b.projection_ready({},now=1000));b.report.write_text('{')
            self.assertFalse(b.projection_ready({},now=1000));b.live=lambda g:False
            self.assertFalse(b.projection_ready({},now=1000))


if __name__=='__main__':unittest.main(verbosity=2)
