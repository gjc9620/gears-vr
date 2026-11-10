"""Start the private action recording after Native's startup handoff.

The shared recorder still owns the five-second fresh-frame timeout. This
boundary does not permit weapon actions before the first captured frame.
"""
import time


def start_action_recording(recorder, live, *, clock=time.monotonic, sleep=time.sleep):
    assert live(), 'Owned game changed before action recording'
    recorder.capture_enabled = True
    recorder.event('HB_action_recording_after_native_handoff', startup_pixels_recorded=False)
    recorder.start()
    deadline = clock() + 5
    while True:
        assert live(), 'Owned game changed while waiting for action recording'
        if recorder.errors:
            raise RuntimeError('Action recording failed before input: ' + str(recorder.errors))
        if recorder.frames:
            recorder.event('HB_first_action_capture_ready', file=recorder.frames[0]['file'])
            return
        if clock() >= deadline:
            raise TimeoutError('No first action projection frame; no weapon action permitted')
        sleep(.01)
