"""Bounded ordinary controller walking for the cover observation fixture.

56276 moved at 0.62, then stopped after the old driver changed to 0.22.
Keep the observed responding amplitude and brake by pulse duration instead.
This is a targeted driver change, not a deadzone or collision diagnosis.
"""
import math
import time


def navigate(ctx, target, *, arrival_distance=35.0):
    if (len(target) != 3 or not all(math.isfinite(v) for v in target)
            or not math.isfinite(arrival_distance) or arrival_distance != 35.0):
        raise ValueError('The cover observation retains its exact 35-unit arrival bound')
    previous = previous_distance = None
    stalls = 0
    for step in range(42):
        ctx.live()
        pos = ctx.location()
        if len(pos) != 3 or not all(math.isfinite(v) for v in pos):
            raise RuntimeError('Current player location is invalid')
        dx, dy = target[0]-pos[0], target[1]-pos[1]
        distance = math.hypot(dx, dy)
        ctx.data['motion'].append(dict(step=step, ns=time.perf_counter_ns(),
                                       position=pos, target=target, distance=distance,
                                       scope='cover ordinary approach; no setter'))
        if distance < arrival_distance:
            return pos
        if (previous is not None and math.hypot(pos[0]-previous[0], pos[1]-previous[1]) < 8
                and previous_distance-distance < .5):
            stalls += 1
        else:
            stalls = 0
        if stalls >= 3:
            raise RuntimeError('Controlled walking blocked before cover; no teleport fallback')
        angle = ctx.yaw()
        if not math.isfinite(angle):
            raise RuntimeError('Current player yaw is invalid')
        scale = .62/distance
        forward = (dx*math.cos(angle)+dy*math.sin(angle))*scale
        right = (-dx*math.sin(angle)+dy*math.cos(angle))*scale
        duration = .22 if distance > 170 else .04
        ctx.g.rec.event('cover_ordinary_approach_step', distance=distance,
                        stick=[right, forward], duration=duration,
                        amplitude=.62, collision_or_deadzone_cause_proven=False)
        # Same fresh original gamepad publisher and smooth start/stop path.
        ctx.stick(0, right, forward, duration)
        previous, previous_distance = pos, distance
    raise RuntimeError('Bounded VR walking did not reach cover')
