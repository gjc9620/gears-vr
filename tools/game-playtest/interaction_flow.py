"""Typed door context and bounded post-input observation; no native calls."""
import struct
import time
import math

from core import gameplay
from world_state import Query
from mission_flow import component_bounds


def radius(q, actor):
    prop = q.prop(actor, 'CollisionComponent', 'ComponentProperty')
    if prop['size'] != 8:
        raise RuntimeError('Door cylinder reference width rejected')
    component = q.obj(struct.unpack('<Q', q.value(actor, prop))[0])
    if component['outer'] != actor['address'] or not q.s.derives(component['cls'], 'CylinderComponent'):
        raise RuntimeError('Door crossing requires a current cylinder component')
    value = q.scalar(component, 'CollisionRadius', 'FloatProperty', '<f')
    if not 0 < value <= 1000:
        raise RuntimeError('Door crossing cylinder radius rejected')
    return dict(identity=component, radius=value)


def exit_geometry(q, controller, door):
    pawn = q.pointer(controller, 'Pawn')
    authored_center, start = q.position(door), q.position(pawn)
    align = q.pointer(door, 'AlignToObj')
    if align:
        if q.pointer(align, 'WorldInfo') != q.pointer(door, 'WorldInfo'):
            raise RuntimeError('Door alignment belongs to another world')
        prop = q.prop(align, 'Rotation', 'StructProperty')
        typ = q.obj(q.r.struct_type(prop))
        if typ['name'] != 'Rotator' or prop['size'] != 12:
            raise RuntimeError('Door alignment rotation is not Rotator')
        for name, offset in (('Pitch', 0), ('Yaw', 4), ('Roll', 8)):
            p = q.prop(typ, name, 'IntProperty', structure=True)
            if p['offset'] != offset or p['size'] != 4:
                raise RuntimeError('Door rotation component layout rejected')
        rotation = list(struct.unpack('<3i', q.value(align, prop)))
        angle = rotation[1] * math.tau / 65536
        forward = [math.cos(angle), math.sin(angle)]
    else:
        rotation = None
        dx, dy = authored_center[0] - start[0], authored_center[1] - start[1]
        length = math.hypot(dx, dy)
        if length < 1:
            raise RuntimeError('Door forward direction is indeterminate')
        forward = [dx / length, dy / length]
    door_cylinder, pawn_cylinder = radius(q, door), radius(q, pawn)
    door_bounds, pawn_bounds = component_bounds(q, door), component_bounds(q, pawn)
    if (not door_bounds or not pawn_bounds or door_bounds['component'] != door_cylinder['identity'] or
            pawn_bounds['component'] != pawn_cylinder['identity']):
        raise RuntimeError('Door crossing bounds/component identity mismatch')
    center = door_bounds['center']
    x, y = map(abs, forward)
    threshold = (x * (door_bounds['extent'][0] + pawn_bounds['extent'][0]) +
                 y * (door_bounds['extent'][1] + pawn_bounds['extent'][1]) + 8)
    lateral_limit = (y * (door_bounds['extent'][0] - pawn_bounds['extent'][0]) +
                     x * (door_bounds['extent'][1] - pawn_bounds['extent'][1]))
    if lateral_limit < 16:
        raise RuntimeError('Door crossing has no bounded lateral clearance')
    distance = threshold + 50
    if distance > 500:
        raise RuntimeError('Door crossing exceeds bounded local travel')
    return dict(center=center, authored_center=authored_center, forward=forward, start=start, align=align, rotation=rotation,
                door_cylinder=door_cylinder, pawn_cylinder=pawn_cylinder,
                door_bounds=door_bounds, pawn_bounds=pawn_bounds,
                threshold=threshold, lateral_limit=lateral_limit,
                exit_goal=[center[0] + forward[0] * distance,
                           center[1] + forward[1] * distance, start[2]])


def crossing_measure(geometry, position):
    dx, dy = position[0] - geometry['center'][0], position[1] - geometry['center'][1]
    x, y = geometry['forward']
    forward = dx * x + dy * y
    lateral = abs(dx * y - dy * x)
    vertical = abs(position[2] - geometry['start'][2])
    return dict(forward=forward, lateral=lateral, vertical=vertical,
                passed=forward >= geometry['threshold'] and lateral <= geometry['lateral_limit'] and vertical <= 180)


def door_context(q, controller, event, *, include_geometry=False):
    if not q.s.derives(event['cls'], 'SeqEvt_DoorOpen'):
        return None
    prop = q.prop(controller, 'DoorTriggerDatum', 'StructProperty')
    typ = q.obj(q.r.struct_type(prop))
    raw = q.value(controller, prop)
    fields = {}
    for name, kind, size in (('DoorTrigger', 'ObjectProperty', 8),
                             ('bInsideDoorTrigger', 'BoolProperty', 4),
                             ('eSpecialMoveType', 'ByteProperty', 1)):
        p = q.prop(typ, name, kind, structure=True)
        if p['size'] != size or p['offset'] + size > len(raw):
            raise RuntimeError('Door context field layout rejected: ' + name)
        fields[name] = (p, raw[p['offset']:p['offset'] + size])
    pointer = struct.unpack('<Q', fields['DoorTrigger'][1])[0]
    door = q.obj(pointer) if pointer else None
    p, data = fields['bInsideDoorTrigger']
    inside = q.decode(p['type'], data, p['address'])
    if type(inside) is not bool:
        raise RuntimeError('Door inside flag is not decoded')
    if door and (not q.s.derives(door['cls'], 'Trigger_DoorInteraction') or
                 q.pointer(door, 'WorldInfo') != q.pointer(controller, 'WorldInfo') or
                 q.pointer(event, 'Originator') != door):
        raise RuntimeError('Door trigger/event world or originator mismatch')
    result = dict(door=door, inside=inside, move_type=fields['eSpecialMoveType'][1][0],
                struct_identity=typ, property=prop,
                fields={name: p for name, (p, data) in fields.items()})
    if include_geometry and door and inside:
        result['geometry'] = exit_geometry(q, controller, door)
    return result


def wait_effect(observer, report, event, before_count, receipt, *, seconds=5):
    """Allow the ordinary door callback/animation to run before judging input."""
    initial = receipt.get('before_sample') or {}
    deadline = time.monotonic() + seconds
    seen_move = receipt.get('special_move_observed')
    after_count = before_count
    states = []
    while time.monotonic() < deadline:
        if (report.directory / 'stop').exists() or observer.user.GetAsyncKeyState(0x77) & 0x8000:
            raise RuntimeError('Door effect observation cancelled')
        sample = report.observe(observer.sample())
        if (sample.get('errors') or not gameplay(sample.get('menu') or {}) or
                sample.get('pawn_identity') != initial.get('pawn_identity') or
                sample.get('native', {}).get('world_generation') != initial.get('native', {}).get('world_generation')):
            raise RuntimeError('Door effect observation crossed a control/world boundary')
        q = Query(observer)
        current = q.obj(event['address'])
        if current != event:
            raise RuntimeError('Door event identity changed during outcome observation')
        after_count = q.scalar(current, 'TriggerCount', 'IntProperty', '<i')
        context = door_context(q, q.obj(observer.sampler.controller), current)
        q.finish(allow_registry_growth=True)
        move = sample.get('special_move_raw')
        if move:
            seen_move = move
        state = dict(sample=sample['sample_id'], count=after_count, special_move=move,
                     inside=context['inside'], door=context['door'])
        states.append(state)
        report.event('door_effect_observation', **state)
        if after_count is not None and before_count is not None and after_count > before_count and move == 0:
            break
        time.sleep(.1)
    return dict(after_count=after_count, special_move_observed=seen_move,
                observations=states, animation_ended=bool(seen_move and states[-1]['special_move'] == 0))
