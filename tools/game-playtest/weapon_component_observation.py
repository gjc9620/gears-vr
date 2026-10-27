"""Pair an existing stable pose read with read-only gun component metadata.

No engine call, input, process creation, writer, native callback or permission.
The original pose result stays usable if the extra observation is rejected.
"""
import hashlib
import functools
import math
import struct
import time


def mul(a, b):
    return [sum(a[i * 4 + k] * b[k * 4 + j] for k in range(4))
            for i in range(4) for j in range(4)]


def inverse_affine(m):
    if len(m) != 16 or not all(math.isfinite(x) for x in m):
        raise RuntimeError('Nonfinite component/bone matrix')
    if any(abs(m[i]) > 1e-6 for i in (3, 7, 11)) or abs(m[15] - 1) > 1e-6:
        raise RuntimeError('Non-affine component/bone matrix')
    a, b, c, d, e, f, g, h, i = (m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10])
    det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g)
    if abs(det) < 1e-10:
        raise RuntimeError('Singular component/bone matrix')
    v = [(e*i-f*h)/det, (c*h-b*i)/det, (b*f-c*e)/det,
         (f*g-d*i)/det, (a*i-c*g)/det, (c*d-a*f)/det,
         (d*h-e*g)/det, (b*g-a*h)/det, (a*e-b*d)/det]
    out = [v[0], v[1], v[2], 0, v[3], v[4], v[5], 0, v[6], v[7], v[8], 0, 0, 0, 0, 1]
    for j in range(3):
        out[12+j] = -sum(m[12+k] * out[k*4+j] for k in range(3))
    if not all(math.isfinite(x) for x in out):
        raise RuntimeError('Nonfinite inverse')
    return out


def _pin(q, address, raw):
    key = address, len(raw)
    if key in q.metadata and q.metadata[key] != raw:
        raise RuntimeError('Component metadata changed during observation')
    q.metadata[key] = raw


def _property(q, obj, name, kind=None, *, structure=False):
    previous = dict(q.metadata)
    value = q.prop(obj, name, kind, structure=structure)
    if any(q.metadata.get(k) != v for k, v in previous.items()):
        raise RuntimeError('Component property metadata changed during observation')
    owner = obj['address'] if structure else obj['cls']
    if not hasattr(q, '_component_selectors'):
        q._component_selectors = {}
    q._component_selectors[(owner, name)] = not structure
    return value


def _value_extent(q, obj, p):
    extent_address = obj['cls'] + 0x88
    extent_raw = q.read(extent_address, 4)
    extent = struct.unpack('<I', extent_raw)[0]
    if not extent or p['offset'] > extent or p['size'] > extent - p['offset']:
        raise RuntimeError('Component property exceeds actual class extent')
    _pin(q, extent_address, extent_raw)


def _value(q, obj, p):
    _value_extent(q, obj, p)
    raw = q.value(obj, p)
    _pin(q, obj['address'] + p['offset'], raw)
    return raw


def _pointer(q, obj, label, kinds=('ObjectProperty', 'ComponentProperty')):
    p = _property(q, obj, label)
    if p['type'] not in kinds or p['size'] != 8:
        raise RuntimeError('Wrong component pointer property: ' + label)
    value = struct.unpack('<Q', _value(q, obj, p))[0]
    if not value:
        raise RuntimeError('Missing component pointer: ' + label)
    return q.obj(value)


def _structure_field(q, obj, label, expected, size):
    p = _property(q, obj, label, 'StructProperty')
    definition = q.obj(q.r.struct_type(p))
    definition_extent = q.read(definition['address'] + 0x88, 4)
    if definition['name'] != expected or p['size'] != size or struct.unpack('<I', definition_extent)[0] != size:
        raise RuntimeError('Wrong component structure: ' + label)
    _pin(q, definition['address'] + 0x88, definition_extent)
    inner_raw = q.read(p['address'] + q.r.profile['struct_property_struct'], 8)
    if struct.unpack('<Q', inner_raw)[0] != definition['address']:
        raise RuntimeError('Component struct selector changed')
    _pin(q, p['address'] + q.r.profile['struct_property_struct'], inner_raw)
    return p


def _structure(q, obj, label, expected, size):
    p = _structure_field(q, obj, label, expected, size)
    return _value(q, obj, p)


def _boolean(q, obj, label):
    p = _property(q, obj, label, 'BoolProperty')
    if p['size'] != 4:
        raise RuntimeError('Wrong component bool width: ' + label)
    mask_raw = q.read(p['address'] + 0xd0, 4)
    mask = struct.unpack('<I', mask_raw)[0]
    if not mask or mask & (mask - 1):
        raise RuntimeError('Wrong component bool mask: ' + label)
    _pin(q, p['address'] + 0xd0, mask_raw)
    return bool(struct.unpack('<I', _value(q, obj, p))[0] & mask)


def _matrix_address(q, obj):
    p = _structure_field(q, obj, 'LocalToWorld', 'Matrix', 64)
    _value_extent(q, obj, p)
    return obj['address'] + p['offset']


def _matrix_bytes(q, address):
    raw = q.read(address, 64)
    value = list(struct.unpack('<16f', raw))
    inverse_affine(value)  # Reject malformed/degenerate transforms; not a pose grant.
    return raw


def _discover(q, body):
    """Typed/static anchors span the later dynamic window; CTW values do not."""
    pawn, mesh = q.obj(body['pawn']['address']), q.obj(body['mesh']['address'])
    if pawn != body['pawn'] or mesh != body['mesh'] or _pointer(q, pawn, 'Mesh') != mesh:
        raise RuntimeError('Component observation body identity changed')
    weapon = _pointer(q, pawn, 'Weapon', ('ObjectProperty',))
    if not q.s.derives(weapon['cls'], 'Weap_AssaultRifle') or _pointer(q, weapon, 'Instigator', ('ObjectProperty',)) != pawn:
        raise RuntimeError('Expected actual owned Lancer')
    if _boolean(q, weapon, 'bDeleteMe'):
        raise RuntimeError('Deleting Lancer')
    component = _pointer(q, weapon, 'Mesh')
    if not q.s.derives(component['cls'], 'SkeletalMeshComponent'):
        raise RuntimeError('Wrong gun component class')
    asset = _pointer(q, component, 'SkeletalMesh', ('ObjectProperty',))
    if asset['name'] != 'COGAssaultRifle' or not q.s.derives(asset['cls'], 'SkeletalMesh'):
        raise RuntimeError('Unsupported gun asset')
    if _boolean(q, component, 'HiddenGame'):
        raise RuntimeError('Hidden gun component')
    absolute = [_boolean(q, component, name) for name in ('AbsoluteTranslation', 'AbsoluteRotation', 'AbsoluteScale')]
    translation = list(struct.unpack('<3f', _structure(q, component, 'Translation', 'Vector', 12)))
    rotation = list(struct.unpack('<3i', _structure(q, component, 'Rotation', 'Rotator', 12)))
    if not all(math.isfinite(x) for x in translation):
        raise RuntimeError('Nonfinite relative gun translation')
    gun_address, avatar_address = _matrix_address(q, component), _matrix_address(q, mesh)
    if avatar_address != mesh['address'] + 0xa0:
        raise RuntimeError('Typed Avatar matrix differs from pose reader layout')
    # Compare the unique attachment's actual FName with Ref115 bone52 bytes.
    avatar_asset = _pointer(q, mesh, 'SkeletalMesh', ('ObjectProperty',))
    ref_header = q.read(avatar_asset['address'] + 0xa4, 16)
    ref, count, capacity = struct.unpack('<Qii', ref_header)
    if count != 115 or not 115 <= capacity <= 512 or not ref:
        raise RuntimeError('Missing actual Ref115')
    reference = q.read(ref, 115 * 68)
    if hashlib.sha256(reference).hexdigest() != body['reference_sha256']:
        raise RuntimeError('Reference skeleton changed')
    _pin(q, avatar_asset['address'] + 0xa4, ref_header)
    _pin(q, ref, reference)
    expected_bone = reference[52*68:52*68+8]
    p = _property(q, mesh, 'Attachments', 'ArrayProperty')
    header = _value(q, mesh, p)
    if len(header) != 16:
        raise RuntimeError('Wrong attachment array header')
    data, n, cap = struct.unpack('<Qii', header)
    if not 0 <= n <= cap <= 128 or (n and not data):
        raise RuntimeError('Invalid attachment array')
    inner_pointer = q.read(p['address'] + 0xd0, 8)
    _pin(q, p['address'] + 0xd0, inner_pointer)
    inner = q.obj(struct.unpack('<Q', inner_pointer)[0])
    if inner['outer'] != p['address'] or not q.s.derives(inner['cls'], 'StructProperty'):
        raise RuntimeError('Wrong attachment inner property')
    inner_meta = q.read(inner['address'] + 0x70, 8)
    if struct.unpack('<II', inner_meta) != (1, 52):
        raise RuntimeError('Wrong attachment stride')
    _pin(q, inner['address'] + 0x70, inner_meta)
    definition_pointer = q.read(inner['address'] + 0xd0, 8)
    _pin(q, inner['address'] + 0xd0, definition_pointer)
    definition = q.obj(struct.unpack('<Q', definition_pointer)[0])
    extent = q.read(definition['address'] + 0x88, 4)
    if not q.s.derives(definition['cls'], 'ScriptStruct') or struct.unpack('<I', extent)[0] != 52:
        raise RuntimeError('Wrong attachment structure')
    _pin(q, definition['address'] + 0x88, extent)
    for name, kind, offset, size in [('Component','ComponentProperty',0,8),('BoneName','NameProperty',8,8),('RelativeLocation','StructProperty',16,12),('RelativeRotation','StructProperty',28,12),('RelativeScale','StructProperty',40,12)]:
        field = _property(q, definition, name, kind, structure=True)
        if field['offset'] != offset or field['size'] != size:
            raise RuntimeError('Attachment member layout mismatch: ' + name)
    raw = q.read(data, n*52) if n else b''
    if raw:
        _pin(q, data, raw)
    entries = [raw[i*52:(i+1)*52] for i in range(n) if struct.unpack_from('<Q', raw, i*52)[0] == component['address']]
    if len(entries) != 1 or entries[0][8:16] != expected_bone:
        raise RuntimeError('Gun not uniquely attached to actual weapon-end bone52')
    entry = entries[0]
    members = struct.unpack_from('<3f3i3f', entry, 16)
    location, angles, scale = members[:3], members[3:6], members[6:9]
    if not all(math.isfinite(x) for x in location + scale):
        raise RuntimeError('Nonfinite attachment transform')
    return dict(weapon=weapon,component=component,asset=asset,
                _matrix_addresses=(gun_address, avatar_address),absolute_flags=absolute,relative_translation=translation,
                relative_rotation=rotation,attachment_bytes=entry.hex(),weapon_end_name_bytes=expected_bone.hex(),
                attachment_location=list(location),attachment_rotation=list(angles),attachment_scale=list(scale))


def _same_pose(a, b):
    return all(a[k] == b[k] for k in ('pawn','mesh','pose_epoch','reference_sha256','space_sha256','local_sha256','ctw'))


def _close_selectors(q):
    if getattr(q.r, '_field_hint_properties', None) is not None:
        raise RuntimeError('Discovery-only property hints are not live component evidence')
    for (owner, name), selected in q.properties.items():
        if q.r.property(owner, name, q._component_selectors[(owner, name)]) != selected:
            raise RuntimeError('Component property selector changed: ' + name)


def _close_pose(q, body):
    mesh = body['mesh']['address']
    epoch = q.read(mesh + 0x338, 4)
    header = q.read(mesh + 0x26c, 56)
    space, n, cap, local, nl, cl = struct.unpack_from('<QiiQii', header)
    _, desired, desired_cap, master = struct.unpack_from('<QiiQ', header, 32)
    if (n != 115 or nl != 115 or not space or not local or master or
            not 115 <= cap <= 512 or not 115 <= cl <= 512 or not 0 <= desired <= desired_cap <= 512):
        raise RuntimeError('Pose layout changed at component close')
    if (struct.unpack('<I', epoch)[0] != body['pose_epoch'] or
            hashlib.sha256(q.read(space, 115*64)).hexdigest() != body['space_sha256'] or
            hashlib.sha256(q.read(local, 115*32)).hexdigest() != body['local_sha256'] or
            list(struct.unpack('<16f', q.read(mesh+0xa0,64))) != body['ctw'] or
            q.read(mesh+0x338,4) != epoch or q.read(mesh+0x26c,56) != header):
        raise RuntimeError('Pose changed at component close')


def pair_probe(observer, pose_probe, query_factory=None):
    before = pose_probe(observer)  # Independently usable fallback, unchanged on rejection.
    start = time.perf_counter_ns()
    try:
        if query_factory is None:
            from world_state import Query
            query_factory = Query
        q = query_factory(observer)
        component = _discover(q, before)
        gun_address, avatar_address = component.pop('_matrix_addresses')
        _close_selectors(q)
        q.finish()
        metadata_verified_ns = time.perf_counter_ns()
        # Original pose_probe and its complete coherent acceptance are unchanged.
        # The discovery pose is not required to freeze during typed discovery.
        window_begin_ns = time.perf_counter_ns()
        after = pose_probe(observer)
        if any(before[k] != after[k] for k in ('pawn', 'mesh', 'reference_sha256')):
            raise RuntimeError('Body identity/reference changed after component discovery')
        gun_raw = _matrix_bytes(q, gun_address)
        avatar_raw = _matrix_bytes(q, avatar_address)
        if list(struct.unpack('<16f', avatar_raw)) != after['ctw']:
            raise RuntimeError('Avatar component matrix changed around pose read')
        if _matrix_bytes(q, gun_address) != gun_raw:
            raise RuntimeError('Gun matrix changed during dynamic pair window')
        _close_pose(q, after)
        window_end_ns = time.perf_counter_ns()
        # Only identities/selectors/static attachment/reference data close here.
        # Dynamic CTW/Local/Space belong to the bounded window above, not to now.
        _close_selectors(q)
        q.finish()
        observer.verify()
        gun_matrix = list(struct.unpack('<16f', gun_raw))
        component.update(accepted=True,begin_ns=start,end_ns=time.perf_counter_ns(),
                         metadata_verified_ns=metadata_verified_ns,
                         pair_window_begin_ns=window_begin_ns,pair_window_end_ns=window_end_ns,
                         dynamic_pose_required_stable_through_metadata_close=False,
                         pose_epoch=after['pose_epoch'],weapon_end_world=after['bones']['weapon_end']['world_matrix'],
                         gun_matrix=gun_matrix,avatar_matrix=list(struct.unpack('<16f', avatar_raw)),
                         gun_relative_to_weapon_end=mul(gun_matrix,inverse_affine(after['bones']['weapon_end']['world_matrix'])),
                         same_stable_read_cohort=True,same_C0_not_proved=True,final_render_phase_not_proved=True,
                         observation_only=True,engine_calls=0,writes=0)
        after['weapon_component_pair'] = component
        return after
    except (RuntimeError,OSError,ValueError,KeyError,struct.error) as error:
        before['weapon_component_pair'] = dict(accepted=False,begin_ns=start,end_ns=time.perf_counter_ns(),
                                               error_type=type(error).__name__,error=str(error),
                                               observation_only=True,same_C0_not_proved=True,engine_calls=0,writes=0)
    return before


def wrap_probe(pose_probe):
    """Keep the existing host's probe(observer) entry and original pose gate."""
    @functools.wraps(pose_probe)
    def probe(observer):
        return pair_probe(observer, pose_probe)
    return probe
