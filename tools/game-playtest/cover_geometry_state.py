"""Bounded read-only geometry of the controlled pawn's current CoverLink.

Call observer.sample() first. All selected raw values and reflected layouts
share one CoverQuery closing bracket. No native getter, collision query, global
object scan, gameplay input or memory write is performed. Python world-space
composition is an explicit candidate convention, not native getter evidence.
"""
import copy
import math
import struct
import time

from cover_state import CoverQuery, slot_snapshot, struct_type


def _shape(q, field, expected):
    typ = q.obj(struct_type(q, field))
    if (field['size'] != 12 or typ['name'] != expected or
            q.obj(typ['cls'])['name'] != 'ScriptStruct'):
        raise RuntimeError('Cover geometry requires a reflected 12-byte ' + expected + ': ' + field['name'] +
                           ' (current structure=' + typ['name'] + ', size=' + str(field['size']) + ')')
    names = ('X', 'Y', 'Z') if expected == 'Vector' else ('Pitch', 'Yaw', 'Roll')
    kind = 'FloatProperty' if expected == 'Vector' else 'IntProperty'
    components = []
    for index, name in enumerate(names):
        component = q.prop(typ, name, kind, structure=True)
        if component['offset'] != index * 4 or component['size'] != 4:
            raise RuntimeError('Cover geometry component layout rejected: ' + expected + '.' + name)
        components.append(component)
    return dict(field=field, structure=typ, components=components)


def _decode(raw, expected):
    value = list(struct.unpack('<3f' if expected == 'Vector' else '<3i', raw))
    if not all(math.isfinite(x) for x in value):
        raise RuntimeError('Nonfinite cover geometry ' + expected)
    return value


def _geometry(q, obj, name, expected, *, optional=False):
    try:
        field = q.prop(obj, name)
    except RuntimeError as error:
        if optional and str(error) == 'Property ' + name + ' not found':
            return dict(available=False, reason=name + ' is not reflected on the current object')
        raise
    expected_kind = 'FloatProperty' if expected == 'Float' else 'StructProperty'
    if field['type'] != expected_kind:
        raise RuntimeError('Cover geometry reflected type rejected: ' + name + ' is ' + field['type'])
    if expected == 'Float':
        if field['size'] != 4:
            raise RuntimeError('Cover geometry Float width rejected: ' + name)
        layout = dict(field=field)
    else:
        layout = _shape(q, field, expected)
    raw = q.value(obj, field)
    value = struct.unpack('<f', raw)[0] if expected == 'Float' else _decode(raw, expected)
    if expected == 'Float' and not math.isfinite(value):
        raise RuntimeError('Nonfinite cover geometry ' + name)
    return dict(available=True, value=value, raw=raw.hex(), layout=layout)


def _mask(q, field):
    if field['type'] != 'BoolProperty' or field['size'] != 4:
        raise RuntimeError('Cover geometry Boolean type/width rejected: ' + field['name'])
    raw = q.read(field['address'] + 0xd0, 4)
    mask = struct.unpack('<I', raw)[0]
    if not mask or mask & (mask - 1):
        raise RuntimeError('Cover geometry Boolean mask rejected: ' + field['name'])
    return dict(field=field, mask=mask, mask_bytes=raw.hex())


def _boolean(q, obj, name):
    field = q.prop(obj, name, 'BoolProperty')
    proof = _mask(q, field)
    raw = q.value(obj, field)
    proof.update(raw=raw.hex(), value=bool(struct.unpack('<I', raw)[0] & proof['mask']))
    return proof


def _slots(q, link, selected):
    result = slot_snapshot(q, link, selected)
    header = q.prop(link, 'Slots', 'ArrayProperty')
    data, count, capacity = struct.unpack('<Qii', q.value(link, header))
    if count != result['count'] or not 0 < count <= capacity <= 256:
        raise RuntimeError('Cover geometry selected Slots header changed')
    fields = {field['name']: field for field in result['fields']}
    if q.obj(result['structure']['cls'])['name'] != 'ScriptStruct':
        raise RuntimeError('Cover geometry Slots structure class rejected')
    result['geometry_layouts'] = {name: _shape(q, fields[name], expected) for name, expected in
                                  (('LocationOffset', 'Vector'), ('RotationOffset', 'Rotator'))}
    result['boolean_masks'] = {name: _mask(q, field) for name, field in fields.items()
                               if field['type'] == 'BoolProperty'}
    # Keep only the selected typed fields, never infer other native CoverSlot
    # payload fields or reinterpret the whole element as an undocumented type.
    for row in result['rows']:
        row['raw_fields'] = {name: q.read(data + row['index'] * result['stride'] + field['offset'],
                                         field['size']).hex() for name, field in fields.items()}
    return result


def _axes(rotation):
    """Candidate basis Rz(yaw) Ry(-pitch) Rx(-roll), 65536 units/revolution."""
    pitch, yaw, roll = [x * math.tau / 65536 for x in rotation]
    cp, sp, cy, sy, cr, sr = math.cos(pitch), math.sin(pitch), math.cos(yaw), math.sin(yaw), math.cos(roll), math.sin(roll)
    return ([cp * cy, cp * sy, sp],
            [sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp],
            [-cr * sp * cy - sr * sy, sr * cy - cr * sp * sy, cr * cp])


def _apply(axes, value):
    return [sum(axes[column][row] * value[column] for column in range(3)) for row in range(3)]


def slot_world_candidates(location, rotation, slots):
    """Pure calculation; the convention and native equivalence remain unverified."""
    axes = _axes(rotation)
    rows = []
    for slot in slots:
        offset = _apply(axes, slot['LocationOffset'])
        basis = [_apply(axes, axis) for axis in _axes(slot['RotationOffset'])]
        rows.append(dict(index=slot['index'], position=[a + b for a, b in zip(location, offset)],
                         forward=basis[0], right=basis[1], up=basis[2]))
    return dict(candidate_only=True, native_getter_verified=False, collision_verified=False,
                convention='Link.Location + Rz(Yaw) Ry(-Pitch) Rx(-Roll) * LocationOffset; '
                           'slot basis composes Link and slot rotations; 65536 rotator units/revolution; '
                           'no scale, native getter or collision equivalence verified',
                rows=rows)


def capture(observer, *, query_factory=CoverQuery):
    """Return serializable raw geometry after one strict closing bracket."""
    out = dict(schema=1, accepted=False, read_only=True, engine_calls=False, activation_allowed=False,
               actor_or_mesh_fields_written=False, atomic_snapshot_verified=False,
               native_cover_state_certified=False, native_getter_verified=False,
               collision_verified=False, renderer_alignment_verified=False,
               started_perf_counter_ns=time.perf_counter_ns())
    try:
        observer.verify()
        if observer.resolver is None or observer.sampler is None:
            raise RuntimeError('Fresh live readers unavailable; call observer.sample() first')
        q = query_factory(observer)
        pc = q.obj(q.s.controller)
        pawn = q.pointer(pc, 'Pawn')
        if not pawn or not q.s.derives(pawn['cls'], 'WarPawn') or q.pointer(pawn, 'Controller') != pc:
            raise RuntimeError('Cover geometry controlled WarPawn/possession mismatch')
        world = q.pointer(pawn, 'WorldInfo')
        if not world or q.pointer(pc, 'WorldInfo') != world:
            raise RuntimeError('Cover geometry controller/pawn world mismatch')
        link = q.pointer(pawn, 'CurrentLink')
        if not link or not q.s.derives(link['cls'], 'CoverLink') or q.pointer(link, 'WorldInfo') != world:
            raise RuntimeError('Cover geometry current CoverLink class/world unavailable or mismatched')
        values = {}
        for name, kind, fmt in (('CoverType', 'ByteProperty', '<B'), ('CoverAction', 'ByteProperty', '<B'),
                                 ('CoverDirection', 'ByteProperty', '<B'), ('CurrentSlotIdx', 'IntProperty', '<i'),
                                 ('LeftSlotIdx', 'IntProperty', '<i'), ('RightSlotIdx', 'IntProperty', '<i'),
                                 ('CurrentSlotPct', 'FloatProperty', '<f')):
            values[name] = q.scalar(pawn, name, kind, fmt)
        if not 0 <= values['CurrentSlotPct'] <= 1:
            raise RuntimeError('Cover geometry interpolation fraction outside range')
        booleans = dict(bIsInStationaryCover=_boolean(q, pawn, 'bIsInStationaryCover'),
                        bCircular=_boolean(q, link, 'bCircular'))
        values.update({name: proof['value'] for name, proof in booleans.items()})
        link_transform = dict(Location=_geometry(q, link, 'Location', 'Vector'),
                              Rotation=_geometry(q, link, 'Rotation', 'Rotator'))
        pawn_transform = dict(Location=_geometry(q, pawn, 'Location', 'Vector'),
                              Rotation=_geometry(q, pawn, 'Rotation', 'Rotator'))
        parameters = {name: _geometry(q, link, name, expected, optional=True) for name, expected in
                      (('StandHeight', 'Float'), ('MidHeight', 'Float'),
                       ('StandingLeanOffset', 'Vector'), ('CrouchLeanOffset', 'Vector'))}
        slots = _slots(q, link, [values[name] for name in ('CurrentSlotIdx', 'LeftSlotIdx', 'RightSlotIdx')])
        candidates = slot_world_candidates(link_transform['Location']['value'], link_transform['Rotation']['value'], slots['rows'])
        q.finish()
        out.update(accepted=True, controller=pc, pawn=pawn, world=world, link=link, values=values,
                   booleans=booleans, link_transform=link_transform, pawn_transform=pawn_transform,
                   link_parameters=parameters, slots=slots, slot_world_candidates=candidates,
                   unavailable=[name for name, item in parameters.items() if not item['available']],
                   source=dict(game_identity=copy.deepcopy(getattr(observer, 'identities', {}).get('game_identity')),
                               disk_game_sha256=q.r.profile.get('sha256'), module_base=getattr(q.r, 'base', None)),
                   limits='Reflected raw values within a checked interval; candidate slot composition does not '
                          'authenticate fixed facing, cover edges, top collision height, native getters or shooting.')
    except (RuntimeError, OSError, ValueError, TypeError, KeyError, struct.error) as error:
        out.update(rejection=str(error), rejection_type=type(error).__name__)
    out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out
