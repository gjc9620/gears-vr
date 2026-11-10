"""Start the private action recording after Native's startup handoff.

The shared recorder still owns the five-second fresh-frame timeout. The
private caller also supplies its identity-bound first real XR projection
receipt; Native hand connection and a waiting-card capture are insufficient.
"""
import time


def action_live(binding, game, recorder):
    if not binding.live(game):
        return False
    if recorder is not None and recorder.errors:
        raise RuntimeError('Private HB recording failed; further actions stopped: ' + str(recorder.errors))
    return True


def start_action_recording(recorder, live, *, projection_ready=None,clock=time.monotonic, sleep=time.sleep):
    assert live(), 'Owned game changed before action recording'
    if projection_ready is not None:
        # Startup observation only: no input, screenshot request, source time
        # or firing permission is produced while waiting for the real handoff.
        deadline=clock()+15
        while not projection_ready():
            assert live(), 'Owned game changed before first real XR projection'
            if clock()>=deadline:
                raise TimeoutError('No identity-bound first real XR projection; no weapon action permitted')
            sleep(.05)
        assert live(), 'Owned game changed at first real XR projection'
        recorder.event('HB_real_projection_ready',identity_bound=True)
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
