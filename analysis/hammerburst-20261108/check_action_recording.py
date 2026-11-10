"""Regress the observed startup-only capture and fail before any action."""
import unittest
from types import SimpleNamespace
from hammerburst_action_recording import start_action_recording,action_live
from hammerburst_isolated_operator import menu_source


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
        self.assertLess(source.index("rec.event('native_hand_connection_ready'"),source.index('start_action_recording(rec,live)'))
        self.assertLess(source.index('start_action_recording(rec,live)'),source.index("head_rest=dict(zip"))
        self.assertIn("capture_enabled=False)",source)
        compile(source,'adapted HB operator','exec')


if __name__=='__main__':unittest.main(verbosity=2)
