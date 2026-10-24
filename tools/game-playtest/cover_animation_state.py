"""Read-only cover animation diagnostics; no engine calls or pose writes.

Call observer.sample() first. Accepted means the selected reflected graph and
values survived closing checks, not an atomic animation frame or collision proof.
Only cover/per-bone blend nodes contribute changing weights to the witness.
"""
import math
import struct
import time

from cover_state import CoverQuery, struct_type

MAX_NODES, MAX_DEPTH, MAX_CHILDREN = 256, 32, 32


def unavailable(reason):
    return dict(available=False, reason=reason)


def prop(q, obj, name, kind=None, *, structure=False, optional=False):
    try:
        return q.prop(obj, name, kind, structure=structure)
    except RuntimeError as error:
        # Do not turn races, unreadable memory or malformed metadata into gaps.
        if optional and str(error) == 'Property ' + name + ' not found':
            return None
        raise


def reference(q, obj, name, *, optional=False):
    field = prop(q, obj, name, optional=optional)
    if field is None:
        return unavailable(name + ' is not reflected')
    if field['type'] not in ('ObjectProperty', 'ComponentProperty') or field['size'] != 8:
        raise RuntimeError('Animation reference type/width rejected: ' + name)
    address = struct.unpack('<Q', q.value(obj, field))[0]
    return q.obj(address) if address else None


def lineage(q, obj):
    names, seen, address = [], set(), obj['cls']
    while address:
        if address in seen or len(seen) >= 64:
            raise RuntimeError('Animation class ancestry cycle/bound')
        seen.add(address)
        cls = q.obj(address)
        names.append(cls['name'])
        address = struct.unpack('<Q', q.read(address + q.r.profile['field_super'], 8))[0]
    return names


def scalar(q, obj, name, kind, *, structure=False, base=None, stride=None):
    field = prop(q, obj, name, kind, structure=structure, optional=True)
    if field is None:
        return unavailable(name + ' is not reflected')
    if field['size'] != (1 if kind == 'ByteProperty' else 4):
        raise RuntimeError('Animation scalar width rejected: ' + name)
    if base is None:
        raw = q.value(obj, field)
    else:
        if field['offset'] < 0 or field['offset'] + field['size'] > stride:
            raise RuntimeError('Animation field escapes array element: ' + name)
        address = base + field['offset']
        raw = q.read(address, field['size'])
        if q.read(address, len(raw)) != raw:
            raise RuntimeError('Animation field changed: ' + name)
    value = q.decode(kind, raw, field['address'])
    if isinstance(value, dict) or (isinstance(value, float) and not math.isfinite(value)):
        raise RuntimeError('Animation scalar semantics rejected: ' + name)
    return value


def array_layout(q, obj, name, limit, *, expected=None, optional=False):
    """Freeze header/inner/layout only; read selected fields, not opaque payloads."""
    field = prop(q, obj, name, 'ArrayProperty', optional=optional)
    if field is None:
        return None
    if field['size'] != 16:
        raise RuntimeError('Animation array header width rejected: ' + name)
    data, count, capacity = struct.unpack('<Qii', q.value(obj, field))
    if not 0 <= count <= capacity <= limit or (count and not data):
        raise RuntimeError('Animation array bounds rejected: ' + name)
    inner = q.obj(struct.unpack('<Q', q.read(
        field['address'] + q.r.profile['struct_property_struct'], 8))[0])
    if inner['outer'] != field['address']:
        raise RuntimeError('Animation array inner owner rejected: ' + name)
    dim, stride = struct.unpack('<II', q.read(inner['address'] + 0x70, 8))
    if dim != 1 or not 1 <= stride <= 1024 or count * stride > 32768:
        raise RuntimeError('Animation array stride rejected: ' + name)
    kind = q.r.class_name(inner)
    typ = q.obj(struct_type(q, dict(address=inner['address'], type=kind))) if kind == 'StructProperty' else None
    if expected and (typ is None or typ['name'] != expected):
        raise RuntimeError('Animation array structure rejected: ' + name)
    return dict(data=data, count=count, stride=stride, inner_kind=kind, structure=typ)


def children(q, node, selected):
    layout = array_layout(q, node, 'Children', MAX_CHILDREN, expected='AnimBlendChild')
    typ, stride = layout['structure'], layout['stride']
    field = prop(q, typ, 'Anim', 'ObjectProperty', structure=True)
    if field['size'] != 8 or field['offset'] < 0 or field['offset'] + 8 > stride:
        raise RuntimeError('Animation child pointer escapes element')
    rows = []
    for index in range(layout['count']):
        base = layout['data'] + index * stride
        address = base + field['offset']
        raw = q.read(address, 8)
        if q.read(address, 8) != raw:
            raise RuntimeError('Animation child edge changed')
        pointer = struct.unpack('<Q', raw)[0]
        row = dict(index=index, Anim=q.obj(pointer) if pointer else None)
        if selected:
            for name in ('Weight', 'DesiredWeight'):
                row[name] = scalar(q, typ, name, 'FloatProperty', structure=True,
                                   base=base, stride=stride)
            mirror = prop(q, typ, 'bMirrorSkeleton', 'BoolProperty', structure=True, optional=True)
            if mirror is None:
                row['bMirrorSkeleton'] = unavailable('bMirrorSkeleton is not reflected')
            else:
                if mirror['size'] != 4 or mirror['offset'] < 0 or mirror['offset'] + 4 > stride:
                    raise RuntimeError('Animation child mirror escapes element')
                mask_raw = q.read(mirror['address'] + 0xd0, 4)
                mask = struct.unpack('<I', mask_raw)[0]
                if not mask or mask & (mask - 1):
                    raise RuntimeError('Animation child mirror Bool mask rejected')
                raw = q.read(base + mirror['offset'], 4)
                row['bMirrorSkeleton'] = dict(available=True, value=bool(struct.unpack('<I', raw)[0] & mask),
                    word=raw.hex(), mask=mask, mask_bytes=mask_raw.hex(), property=mirror,
                    field_offset=mirror['offset'], child_stride=stride, native_consumer_verified=False)
        rows.append(row)
    return rows


def masks(q, node):
    layout = array_layout(q, node, 'MaskList', MAX_CHILDREN,
                          expected='PerBoneMaskInfo', optional=True)
    if layout is None:
        return unavailable('MaskList is not reflected')
    rows = []
    for index in range(layout['count']):
        base = layout['data'] + index * layout['stride']
        row = dict(index=index)
        for name, kind in (('DesiredWeight', 'FloatProperty'),
                           ('BlendTimeToGo', 'FloatProperty'), ('bPendingBlend', 'BoolProperty')):
            row[name] = scalar(q, layout['structure'], name, kind, structure=True,
                               base=base, stride=layout['stride'])
        rows.append(row)
    return dict(available=True, count=layout['count'], stride=layout['stride'], rows=rows)


def bone_gap(q, mesh):
    # Saved Engine metadata exposes RefSkeleton as IntProperty elements, not
    # authenticated named bone records. Never reinterpret that native storage.
    out = unavailable('No authenticated bone-name-to-SpaceBases-index mapping; no bone matrices decoded')
    out.update(requested_regions=['root', 'spine', 'head'], matrices=[])
    space = array_layout(q, mesh, 'SpaceBases', 512, optional=True)
    out['SpaceBases'] = ({k: v for k, v in space.items() if k != 'data'} if space else
                         unavailable('SpaceBases is not reflected'))
    asset = reference(q, mesh, 'SkeletalMesh', optional=True)
    if not asset or asset.get('available') is False:
        out['RefSkeleton'] = unavailable('SkeletalMesh asset unavailable')
    else:
        if 'SkeletalMesh' not in lineage(q, asset):
            raise RuntimeError('Animation skeletal asset class rejected')
        layout = array_layout(q, asset, 'RefSkeleton', 512, optional=True)
        out['RefSkeleton'] = ({k: v for k, v in layout.items() if k != 'data'} if layout else
                             unavailable('RefSkeleton is not reflected'))
    return out


def capture(observer, *, query_factory=CoverQuery):
    out = dict(schema=1, read_only=True, accepted=False, complete=False,
               atomic_snapshot_verified=False, renderer_alignment_verified=False,
               native_cover_state_certified=False, collision_verified=False,
               activation_allowed=False, started_perf_counter_ns=time.perf_counter_ns(), nodes=[])
    try:
        observer.verify()
        q = query_factory(observer)
        pc = q.obj(observer.sampler.controller)
        pawn = q.pointer(pc, 'Pawn')
        if not pawn or 'WarPawn' not in lineage(q, pawn) or q.pointer(pawn, 'Controller') != pc:
            raise RuntimeError('Animation controlled WarPawn/possession rejected')
        world = q.pointer(pawn, 'WorldInfo')
        if not world or q.pointer(pc, 'WorldInfo') != world:
            raise RuntimeError('Animation controller/pawn world mismatch')
        mesh = reference(q, pawn, 'Mesh')
        if (not mesh or mesh['name'].startswith('Default__') or
                'SkeletalMeshComponent' not in lineage(q, mesh) or reference(q, mesh, 'Owner') != pawn):
            raise RuntimeError('Animation pawn mesh owner/class rejected')
        root = reference(q, mesh, 'Animations')
        if not root or 'AnimTree' not in lineage(q, root):
            raise RuntimeError('Animation live AnimTree unavailable')
        template = reference(q, mesh, 'AnimTreeTemplate', optional=True)
        if root == template:
            raise RuntimeError('Animation root aliases template')
        out.update(controller=pc, pawn=pawn, world=world, mesh=mesh,
                   animation_root=root, animation_template=template)
        visited, active = set(), set()

        def visit(node, depth):
            address = node['address']
            if address in active:
                raise RuntimeError('Animation child graph cycle')
            if depth > MAX_DEPTH:
                raise RuntimeError('Animation graph depth bound')
            classes = lineage(q, node)
            if (node['name'].startswith('Default__') or 'AnimNode' not in classes or
                    reference(q, node, 'SkelComponent') != mesh):
                raise RuntimeError('Animation node instance/mesh backlink rejected')
            if address in visited:
                return
            if len(visited) >= MAX_NODES:
                raise RuntimeError('Animation graph node bound')
            visited.add(address)
            active.add(address)
            roles = [c for c in ('WarAnim_CoverBlend', 'WarAnim_BlendPerBone') if c in classes]
            selected = bool(roles)
            row = dict(identity=node, class_name=classes[0], roles=roles,
                       depth=depth, selected=selected, children=[])
            out['nodes'].append(row)
            if selected:
                owner = reference(q, node, 'WarPawnOwner', optional=True)
                if owner != pawn and not (owner and owner.get('available') is False):
                    raise RuntimeError('Animation blend pawn backlink rejected')
                row['WarPawnOwner'] = owner
                row['fields'] = {name: scalar(q, node, name, kind) for name, kind in
                                 (('Weight', 'FloatProperty'), ('DesiredWeight', 'FloatProperty'),
                                  ('NodeTotalWeight', 'FloatProperty'))}
                if 'WarAnim_CoverBlend' in classes:
                    row['fields']['AnimCoverAction'] = scalar(q, node, 'AnimCoverAction', 'ByteProperty')
                if 'WarAnim_BlendPerBone' in classes:
                    row['MaskList'] = masks(q, node)
            if 'AnimNodeBlendBase' in classes:
                row['children'] = children(q, node, selected)
                for child in row['children']:
                    if child['Anim']:
                        visit(child['Anim'], depth + 1)
            active.remove(address)

        visit(root, 0)
        out['bones'] = bone_gap(q, mesh)
        q.finish()
        out.update(accepted=True, complete=True, visited_nodes=len(visited),
                   cover_nodes=[r['identity'] for r in out['nodes'] if 'WarAnim_CoverBlend' in r['roles']],
                   per_bone_nodes=[r['identity'] for r in out['nodes'] if 'WarAnim_BlendPerBone' in r['roles']],
                   limits='Selected reflected animation fields only; no engine calls, bone-index guesses, pose writes, atomic frame or collision proof.')
    except (RuntimeError, OSError, ValueError, struct.error) as error:
        out.update(rejection=str(error), rejection_type=type(error).__name__)
    out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out
