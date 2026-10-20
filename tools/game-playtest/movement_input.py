"""Accept forward input only after observed forward travel and neutral settling."""
import math
import time

from core import controllable, yaw_delta


def response(before, after, neutral):
    samples = (before, after, neutral)
    if any(s.get('errors') or not controllable(s) for s in samples):
        return None
    if any(s.get('pawn_identity') != before.get('pawn_identity') or
           s.get('native', {}).get('world_generation') != before.get('native', {}).get('world_generation')
           for s in samples):
        return None
    a, b, c = [s['pose']['position'] for s in samples]
    yaw = before['pose']['yaw_ru'] * math.tau / 65536
    dx, dy = b[0] - a[0], b[1] - a[1]
    planar = math.hypot(dx, dy)
    forward = dx * math.cos(yaw) + dy * math.sin(yaw)
    return dict(distance=planar, forward_distance=forward,
                forward_fraction=forward / planar if planar else 0,
                vertical_change=abs(b[2] - a[2]), neutral_drift=math.dist(b, c),
                yaw_change=abs(yaw_delta(before['pose']['yaw_ru'], after['pose']['yaw_ru']) * 360 / 65536))


def calibrate(observer, report):
    from axis_input import pulse, AxisReadInterrupted
    from live import InputNotReady
    from scenarios import wait_state, playable
    report.assisted = True
    best = None
    for value in (.2, .5, 1.0):
        baseline = wait_state(observer, report, playable)
        time.sleep(.2)
        before = wait_state(observer, report, playable)
        if (baseline.get('pawn_identity') != before.get('pawn_identity') or
                math.dist(baseline['pose']['position'], before['pose']['position']) > 2):
            report.event('forward_calibration_baseline_unstable')
            if best:
                break
            continue
        try:
            receipt = pulse(observer, 'aBaseY', value, .2, emit=report.event,
                            stop_requested=lambda: (report.directory / 'stop').exists())
        except (InputNotReady, AxisReadInterrupted) as error:
            report.event('forward_calibration_rejected', reason=str(error))
            if isinstance(error, AxisReadInterrupted) and error.receipt.get('writes', 0) > 0 and not error.receipt.get('neutral_verified'):
                raise
            if best:
                break
            continue
        time.sleep(.2)
        after = wait_state(observer, report, playable)
        time.sleep(.2)
        neutral = wait_state(observer, report, playable)
        result = report.action('move_forward', before, after, dispatched=receipt['release_posted'],
                               neutral=neutral, detail=receipt)
        measured = response(before, after, neutral)
        report.event('forward_calibration_attempt', value=value, receipt=receipt, response=measured,
                     action_id=result['action_id'])
        if (measured and measured['distance'] >= 6 and measured['forward_fraction'] >= .8
                and measured['neutral_drift'] <= 4 and measured['vertical_change'] <= 12
                and measured['yaw_change'] <= 3 and receipt['neutral_verified']):
            candidate = dict(axis='aBaseY', value=value,
                            speed=measured['forward_distance'] / receipt['actual_hold_seconds'],
                            pawn_identity=before['pawn_identity'],
                            world_generation=before['native']['world_generation'],
                            source_action=result['action_id'], backend='reflected_input_float')
            if best is None or candidate['speed'] > best['speed']:
                best = candidate
            if best['speed'] >= 120 or value == 1:
                break
    if best:
        report.cover('walk', 'passed', **best,
                     scope='Owned forward input, observed travel direction and neutral settling')
    return best
