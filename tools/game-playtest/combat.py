"""Bounded targeting from fresh hostile/collision evidence and ordinary input."""
import math
import time

from core import controllable, yaw_delta
from world_state import Query, select_hostile, combat_control, squad_hostility, render_hint, capture as capture_world


def axis_decision(calibration, role, error):
    item = (calibration or {}).get(role)
    if not item:
        return None
    axis, rate, value = item.get('axis'), item.get('rate'), item.get('value')
    allowed = ('aLookUp', 'aMouseY') if role == 'pitch' else ('aTurn', 'aMouseX')
    if (axis not in allowed or not all(isinstance(v, (int, float)) and math.isfinite(v) for v in (rate, value, error))
            or not .1 <= abs(rate) <= 4000 or not 0 < value <= 1):
        return None
    if abs(error) < 1e-6:
        return None
    # Metadata/context checks and neutral certification add release latency.
    # Reduce magnitude near the target so the minimum pulse cannot jump over it.
    correction = abs(error) * .65
    value = min(value, correction / (abs(rate) * .12))
    return dict(name='look_' + role, axis=axis, value=math.copysign(value, error / rate),
                seconds=max(.02, min(.20, correction / (abs(rate) * value))),
                target_degrees=min(10 if role == 'pitch' else 45, correction))


def plan(world, *, turn_rate, turn_name, look_calibration=None, aiming=False):
    sample = world['end_sample']
    if sample['errors'] or not controllable(sample):
        return None
    pitch_ready = axis_decision(look_calibration, 'pitch', 1) is not None
    target = select_hostile(world, max_distance=2500 if pitch_ready else 1800,
                           max_height=2500 if pitch_ready else 150, prefer_mounted=True,
                           require_recent_render=world.get('combat_render_hint_policy') == 'recent-body-render-0.5s')
    if not target or not target.get('collision'):
        return None
    pose = sample['pose']
    origin, rotation = pose.get('camera_position'), pose.get('camera_rotation_ru')
    if not origin or not rotation:
        return None
    cylinder = target['collision']
    position = list(target['position'])
    if target.get('driven_vehicle'):
        position[2] += cylinder['height'] * .8
    dx, dy = position[0] - origin[0], position[1] - origin[1]
    distance = math.hypot(dx, dy)
    if distance < 60:
        return None
    desired = math.degrees(math.atan2(dy, dx))
    yaw = rotation[1] * 360 / 65536
    error = (desired - yaw + 180) % 360 - 180
    mounted = bool(target.get('driven_vehicle'))
    precision = mounted and aiming
    tolerance = max(.2 if precision else 1, min(6, math.degrees(math.atan2(cylinder['radius'] * (.25 if precision else .6), distance))))
    common = dict(target=target, yaw_error=error, distance=distance,
                  target_health=target['health'], aim_position=position, line_of_sight_verified=False)
    pitch = ((rotation[0] + 32768) % 65536 - 32768) * 2 * math.pi / 65536
    ray_z = origin[2] + math.tan(pitch) * distance
    if abs(ray_z - position[2]) > cylinder['height'] * (.15 if precision else .65):
        desired_pitch = math.degrees(math.atan2(position[2] - origin[2], distance))
        decision = axis_decision(look_calibration, 'pitch', desired_pitch - math.degrees(pitch))
        return dict(common, **decision) if decision else None
    if abs(error) > tolerance:
        decision = axis_decision(look_calibration, 'yaw', error)
        if decision:
            return dict(common, **decision)
        if turn_rate is None or abs(turn_rate) < 1:
            return None
        angle = error % 360 if turn_rate > 0 else (-error) % 360
        return dict(common, name=turn_name, seconds=max(.02, min(.20, angle / abs(turn_rate))))
    # This checks only other observed live pawns, not walls or unobserved actors.
    for other in world.get('pawns', []):
        if (other['identity'] in (target['identity'], sample['pawn_identity']) or other.get('health', 0) <= 0
                or not other.get('collision')):
            continue
        px, py, pz = other['position']
        t = ((px - origin[0]) * dx + (py - origin[1]) * dy) / (distance * distance)
        if 0 < t < 1:
            separation = math.hypot(px - origin[0] - t * dx, py - origin[1] - t * dy)
            projected_z = origin[2] + (ray_z - origin[2]) * t
            if separation < other['collision']['radius'] and abs(projected_z - pz) < other['collision']['height']:
                return None
    return dict(common, name='fire', seconds=.25)


def certify_target(observer, world, target, name):
    identity = target['identity']
    q = Query(observer)
    pawn = q.obj(identity['address'])
    control = combat_control(q, pawn, world['world_identity'])
    local = q.obj(observer.sampler.pawn)
    if (pawn != identity or control['controller'] != target['controller']
            or control['controlled_pawn'] != target.get('controlled_pawn', pawn)
            or q.pointer(pawn, 'WorldInfo') != world['world_identity'] or not control['ai_controller']):
        return None
    hostility = dict(verified=True, basis='AI currently targets the exact local pawn') if control['enemy'] == local else squad_hostility(q, local, control, world['world_identity'])
    if not hostility:
        return None
    health = q.scalar(pawn, 'Health', 'IntProperty', '<i')
    if name == 'fire' and math.dist(q.position(pawn), target['position']) > target['collision']['radius'] * .25:
        return None
    if (world.get('combat_render_hint_policy') == 'recent-body-render-0.5s' and
            not render_hint(q, pawn, world['world_identity'])['recently_rendered']):
        return None
    q.finish()
    if health <= 0:
        return None
    before = observer.sample()
    if (before['errors'] or not controllable(before) or before.get('pawn_identity') != world['source_sample']['pawn_identity']
            or before.get('native', {}).get('world_generation') != world['source_sample']['native']['world_generation']
            or before['elapsed'] - world['end_sample']['elapsed'] > 1):
        return None
    return dict(before=before, health=health, control=control, hostility=hostility)


def observed_axis_rate(receipt, after, role):
    rotation = (after.get('pose') or {}).get('controller_rotation_ru')
    if (not controllable(after) or after.get('errors') or not rotation or len(rotation) != 3
            or receipt.get('neutral_verified') is not True or receipt.get('writer_closed') is not True
            or not receipt.get('writes') or not receipt.get('value')):
        return None
    elapsed, initial, value = receipt.get('actual_hold_seconds'), receipt.get('initial_angle'), receipt.get('value')
    if not all(isinstance(v, (int, float)) and math.isfinite(v) for v in (elapsed, initial, value)) or elapsed <= 0:
        return None
    change = yaw_delta(initial, rotation[0 if role == 'pitch' else 1]) * 360 / 65536
    rate = change / (value * elapsed)
    return rate if abs(change) >= .1 and .1 <= abs(rate) <= 4000 else None


def aim_calibration_state(attempts, world):
    sample = world['source_sample']
    def identity(obj):
        return tuple((obj or {}).get(key) for key in ('address', 'internal_index', 'number', 'cls', 'outer'))
    key = ('aimed_calibration', sample['native']['world_generation'],
           identity(sample.get('pawn_identity')), identity(sample.get('weapon_identity')))
    return attempts.setdefault(key, {})


def aimed_burst(observer, report, world, decision, bindings, look_calibration, *, aimed_cache=None):
    """Re-observe the firing lane after aim/FOV changes, with both releases scoped."""
    result = {}
    aimed_calibration = {key: dict(value) for key, value in (look_calibration or {}).items() if value}
    for role, value in (aimed_cache or {}).items():
        if role in aimed_calibration and value.get('axis') == aimed_calibration[role].get('axis'):
            aimed_calibration[role] = dict(value)
    base_fov = world['end_sample']['pose'].get('fov')
    def while_aiming(held):
        held = report.observe(held)
        if held['errors'] or not controllable(held):
            return held
        started = time.monotonic()
        for index in range(3):
            if time.monotonic() - started > 1.2:
                break
            fresh = capture_world(observer)
            fov = fresh['end_sample']['pose'].get('fov')
            if base_fov is None or fov is None or fov >= base_fov - 1:
                report.event('aimed_fire_not_ready', reason='Aim FOV change not established', base_fov=base_fov, held_fov=fov)
                return held
            current = plan(fresh, turn_rate=None, turn_name=None, look_calibration=aimed_calibration, aiming=True)
            if not current or current['target']['identity'] != decision['target']['identity']:
                report.event('aimed_fire_not_ready', reason='Fresh aimed target/lane changed')
                return held
            proof = certify_target(observer, fresh, current['target'], current['name'])
            if not proof:
                report.event('aimed_fire_not_ready', reason='Fresh target certificate not admitted')
                return held
            proof['before'] = report.observe(proof['before'])
            elapsed = time.monotonic() - started
            if current['name'] in ('look_pitch', 'look_yaw') and index < 2 and elapsed < 1.0:
                from axis_input import pulse as axis_pulse
                correction = axis_pulse(observer, current['axis'], current['value'], current['seconds'],
                                         target_degrees=current['target_degrees'], emit=report.event,
                                         stop_requested=lambda: (report.directory / 'stop').exists())
                report.assisted = True
                after = report.observe(observer.sample())
                action = report.action(current['name'], proof['before'], after,
                                       dispatched=correction['release_posted'], detail=correction)
                report.event('aimed_correction', target=current['target']['identity'],
                             action_id=action['action_id'], name=current['name'],
                             yaw_error=current['yaw_error'], neutral_verified=correction['neutral_verified'])
                role = 'pitch' if current['name'] == 'look_pitch' else 'yaw'
                rate = observed_axis_rate(correction, after, role)
                if (rate is not None and role in aimed_calibration
                        and rate * aimed_calibration[role]['rate'] > 0
                        and proof['before'].get('pawn_identity') == after.get('pawn_identity')
                        and proof['before']['native'].get('world_generation') == after['native'].get('world_generation')):
                    aimed_calibration[role].update(rate=rate, source_action=action['action_id'], scope='current_aim_hold')
                    if aimed_cache is not None:
                        aimed_cache[role] = dict(aimed_calibration[role])
                    report.event('aimed_axis_response', role=role, rate=rate, action_id=action['action_id'],
                                 scope='Observed current-hold response estimate; neutral verified')
                continue
            if current['name'] != 'fire' or elapsed > 1.4:
                report.event('aimed_fire_not_ready', reason='Bounded aimed correction budget reached',
                             next_action=current['name'], yaw_error=current['yaw_error'])
                return held
            shot = observer.pulse('LeftMouseButton', decision['seconds'], expected_bindings=bindings,
                                  held_keys=('RightMouseButton',), emit=report.event,
                                  stop_requested=lambda: (report.directory / 'stop').exists())
            result.update(proof=proof, shot=shot, decision=current, base_fov=base_fov, aimed_fov=fov)
            break
        return held
    aim = observer.pulse('RightMouseButton', 1.8, expected_bindings=bindings,
                         during=while_aiming, emit=report.event,
                         stop_requested=lambda: (report.directory / 'stop').exists())
    if not result:
        return None
    result['shot'].update(aim_receipt=aim, aimed=True, base_fov=result['base_fov'], aimed_fov=result['aimed_fov'])
    return result


def engagement_window(attempts, world, now):
    """Give navigation eight seconds after twelve seconds of continued combat."""
    sample = world['source_sample']
    pawn = sample['pawn_identity']
    key = ('engagement_window', sample['native']['world_generation'], pawn['address'], pawn['internal_index'])
    state = attempts.setdefault(key, dict(start=None, last=None, until=0))
    if now < state['until']:
        return False, False
    if state['start'] is None or (state['last'] is not None and now - state['last'] > 6):
        state['start'] = now
    if now - state['start'] >= 12:
        state.update(start=None, last=None, until=now + 8)
        return False, True
    state['last'] = now
    return True, False


def step(observer, report, world, explorer, choices, bindings, attempts, look_calibration=None):
    decision = plan(world, turn_rate=explorer.turn_rate if explorer else None,
                    turn_name=explorer.turn if explorer else None, look_calibration=look_calibration)
    if not decision:
        return False
    target, name = decision['target'], decision['name']
    admitted, deferred = engagement_window(attempts, world, time.monotonic())
    if not admitted:
        if deferred:
            report.event('combat_navigation_interval', target=target['identity'], seconds=8,
                         reason='Twelve-second combat interval elapsed; allow route progress')
        return False
    identity = target['identity']
    origin = world['source_sample']['pose']['position']
    attempt_key = (world['source_sample']['native']['world_generation'], identity['address'], identity['internal_index'],
                   identity.get('number', 0), math.floor(origin[0] / 250), math.floor(origin[1] / 250))
    turn_key = (*attempt_key, 'turns')
    if attempts.get(attempt_key, 0) >= 12 or (name != 'fire' and attempts.get(turn_key, 0) >= 24):
        return False
    is_axis = name in ('look_pitch', 'look_yaw')
    key = choices.get(name) if name != 'fire' else 'LeftMouseButton'
    if not is_axis and (not key or (name == 'fire' and bindings.get(key) != 'Game_Shoot')):
        return False
    proof = certify_target(observer, world, target, name)
    if not proof:
        return False
    before = report.observe(proof['before'])
    health, control, hostility = proof['health'], proof['control'], proof['hostility']
    attempts[attempt_key] = attempts.get(attempt_key, 0) + int(name == 'fire')
    attempts[turn_key] = attempts.get(turn_key, 0) + int(name != 'fire')
    if is_axis:
        from axis_input import pulse as axis_pulse
        report.assisted = True
        pulse = axis_pulse(observer, decision['axis'], decision['value'], decision['seconds'],
                           target_degrees=decision['target_degrees'], emit=report.event,
                           stop_requested=lambda: (report.directory / 'stop').exists())
    elif name == 'fire' and bindings.get('RightMouseButton') == 'Game_Target':
        aimed = aimed_burst(observer, report, world, decision, bindings, look_calibration,
                             aimed_cache=aim_calibration_state(attempts, world))
        if not aimed:
            return True
        pulse, proof, decision = aimed['shot'], aimed['proof'], aimed['decision']
        before = proof['before']
        health, control, hostility = proof['health'], proof['control'], proof['hostility']
    else:
        pulse = observer.pulse(key, decision['seconds'], expected_bindings=bindings,
                               stop_requested=lambda: (report.directory / 'stop').exists(), emit=report.event)
    report.event('combat_input', name=name, target=identity, seconds=decision['seconds'],
                 health_before=health, yaw_error=decision['yaw_error'], line_of_sight_verified=False,
                 controlled_pawn=control['controlled_pawn'], hostility=hostility, aim_position=decision['aim_position'],
                 release_posted=pulse['release_posted'])
    time.sleep(.2)
    after = report.observe(observer.sample())
    action = report.action(name, before, after, dispatched=pulse['release_posted'], detail=pulse)
    if name == 'fire' and pulse.get('aimed') and action.get('ammo_used_delta', 0) > 0:
        report.cover('aim_fire', 'passed', action_id=action['action_id'],
                     fov_before=pulse['base_fov'], fov_held=pulse['aimed_fov'],
                     scope='Overlapping owned aim/fire holds, reduced FOV and ammunition response; clearance remains unproven')
    if name == 'fire':
        try:
            q = Query(observer)
            current = q.obj(identity['address'])
            if current != identity:
                raise RuntimeError('Hostile identity changed after shot')
            after_health = q.scalar(current, 'Health', 'IntProperty', '<i')
            q.finish()
            report.event('combat_result', action_id=action['action_id'], target=identity,
                         health_before=health, health_after=after_health, exclusive_damage_attribution=False)
            if after_health < health:
                report.cover('enemy_engagement', 'passed', action_id=action['action_id'], target=identity,
                             health_before=health, health_after=after_health,
                             scope='Hostile health decreased after scoped fire; teammate damage may contribute')
        except (OSError, RuntimeError, ValueError) as error:
            report.event('combat_outcome_unavailable', target=identity, reason=str(error))
    return True
