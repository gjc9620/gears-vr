"""Bounded current skeletal physics source/receiver observations, never a permit.

No virtual method, engine function, getter, write or allocation is dispatched.
Full original update chunks are checked against the historical artifact. Only
the exact original entry prefix may come from the existing sealed LIVE hook
attestation; actual patched bytes are separately retained by the read bracket.
No generic Resolver read substitution or guessed native call ABI is used.
"""
import copy
import hashlib
from pathlib import Path
import struct
import sys
import time
from types import SimpleNamespace

from cover_state import CoverQuery, struct_type
from cover_animation_state import lineage
from cover_bone_query import _chain, _field, _mesh, _module, _name, _reference
import cover_surface_query as surface

MAX_BODIES = 64
PHYSICS_CHUNKS = (
    (0xf26d4, 1422, 'c8b9a07a24f586eab33c7983ca2f19a7f1a3a10e758a7ea0bf84c5bfaf686683'),
    (0x8a018a, 911, 'f09cae23b775ebbf39371b65f0376e1a9ec677c95b571ff69ddacde94607e08e'),
    (0x2151e4, 1609, '9f65c3f76ae41e5b353fd1ea7d00cf14c781cf00b1b2dafd7f8fcb5aaea81e15'),
    (0x8d6068, 295, '3572b92974875d92f55073c9b17e9a08d385b640bb916361eec2274b3f3fad4c'),
)


def _attestation(*args, **kwargs):
    # Match the existing native-tool bootstrap only when a real attestation
    # is requested. Importing/testing pure reflected logic opens no process.
    directory = str(Path(__file__).resolve().parents[1] / 'native-capture')
    if directory not in sys.path:
        sys.path.insert(0, directory)
    from hook_entry_attestation import HookEntryAttestation
    return HookEntryAttestation(*args, **kwargs)


def _historical():
    raw = surface.DEFAULT_BLOB.read_bytes()
    if len(raw) != surface.HISTORICAL_SIZE or hashlib.sha256(raw).hexdigest() != surface.HISTORICAL_SHA256:
        raise RuntimeError('Physics historical code artifact differs')
    chunks = []
    for rva, size, digest in PHYSICS_CHUNKS:
        data = raw[rva - surface.HISTORICAL_RVA:rva - surface.HISTORICAL_RVA + size]
        if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
            raise RuntimeError('Physics original full code chunk differs')
        chunks.append((rva, data, digest))
    return chunks


def _code(q, module, proof, chunks):
    result = []
    for rva, expected, digest in chunks:
        surface._range(module, rva, len(expected), True)
        at = module['base'] + rva
        actual = q.read(at, len(expected))
        if rva == 0xf26d4:
            original = proof.read_entry(at, 32, q.read)
            actual = original + actual[32:]
        if actual != expected or hashlib.sha256(actual).hexdigest() != digest:
            raise RuntimeError('Physics current full code/attested entry differs: ' + hex(rva))
        result.append(dict(rva=hex(rva), size=len(expected), sha256=digest,
                           sealed_entry_used=at in proof.by_target))
    return result


def _array(q, obj, name, target_name, expected_offset):
    field = _field(q, obj, name, 'ArrayProperty')
    if field['offset'] != expected_offset or field['size'] != 16:
        raise RuntimeError('Physics named array does not match original consumer: ' + name)
    raw = q.value(obj, field)
    data, count, capacity = struct.unpack('<Qii', raw)
    if (not 0 < count <= MAX_BODIES or not count <= capacity <= 256 or
            not 0 < data < (1 << 64) - capacity * 8 or data & 7):
        raise RuntimeError('Physics body array bounds rejected; no truncation: ' + name)
    inner = q.obj(struct.unpack('<Q', q.read(field['address'] + q.r.profile['struct_property_struct'], 8))[0])
    dim, width = struct.unpack('<II', q.read(inner['address'] + 0x70, 8))
    target = q.obj(struct.unpack('<Q', q.read(inner['address'] + q.r.profile['struct_property_struct'], 8))[0])
    if (inner['outer'] != field['address'] or inner['number'] or
            q.obj(inner['cls'])['name'] != 'ObjectProperty' or dim != 1 or width != 8 or
            struct.unpack('<I', q.read(inner['address'] + 0x94, 4))[0] != 0 or
            target['name'] != target_name or target['number'] or q.obj(target['cls'])['name'] != 'Class'):
        raise RuntimeError('Physics object array element schema rejected: ' + name)
    pointers_raw = q.read(data, count * 8)
    pointers = struct.unpack('<' + 'Q' * count, pointers_raw)
    if any(not p for p in pointers) or len(set(pointers)) != count:
        raise RuntimeError('Physics object array has null/duplicate members: ' + name)
    members = [q.obj(p) for p in pointers]
    if any(m['name'].startswith('Default__') or target not in list(_chain(q, m['cls'])) for m in members):
        raise RuntimeError('Physics object array member class rejected: ' + name)
    return dict(field=field, inner=inner, target_class=target, header=raw.hex(),
                pointers=pointers_raw.hex(), count=count, capacity=capacity), members


def _pointer_field(q, obj, name, offset):
    field = _field(q, obj, name, 'StructProperty')
    typ = q.obj(struct_type(q, field))
    if (field['offset'] != offset or field['size'] != 8 or typ['name'] != 'Pointer' or typ['number'] or
            q.obj(typ['cls'])['name'] != 'ScriptStruct' or
            struct.unpack('<I', q.read(typ['address'] + 0x88, 4))[0] != 8):
        raise RuntimeError('Physics opaque pointer field differs from original consumer: ' + name)
    return dict(field=field, structure=typ)


def _receiver(q, pointer, prefixes):
    if not pointer:
        return dict(present=False, address=0)  # Absence is not a skipped-body success.
    if not 0 < pointer < (1 << 64) - 8 or pointer & 7:
        raise RuntimeError('Physics receiver pointer is invalid')
    vtable = struct.unpack('<Q', q.read(pointer, 8))[0]
    if not 0 < vtable < (1 << 64) - 736 or vtable & 7:
        raise RuntimeError('Physics receiver vtable is invalid')
    slots = {}
    for offset in (40, 104, 728):
        target = struct.unpack('<Q', q.read(vtable + offset, 8))[0]
        if not 0 < target < (1 << 64) - 32:
            raise RuntimeError('Physics observed virtual target is invalid')
        if target not in prefixes:
            raw = q.read(target, 32)
            prefixes[target] = dict(address=target, bytes=raw.hex(), sha256=hashlib.sha256(raw).hexdigest())
        slots[str(offset)] = target
    return dict(present=True, address=pointer, vtable=vtable, slots=slots,
                method_names_resolved=False, methods_called=False)


def capture(observer, native_dll, native_sha256, *, query_factory=CoverQuery,
            attestation_factory=_attestation, historical_loader=_historical):
    """Close one current source observation; never certify active physics/pose.

    Array/index/component associations and readable code prefixes do not prove
    virtual method meaning, physical scene membership, update coverage, finite
    receiver transforms, same-pose hit use, or query-visible synchronization.
    """
    out = dict(schema=1, accepted=False, read_only=True, engine_calls=False, queue_submissions=0,
               body_modified=False, activation_allowed=False, collision_verified=False,
               physical_pose_verified=False, hit_geometry_verified=False, active_physics_verified=False,
               update_coverage_verified=False, matched_bones_verified=False,
               virtual_target_module_verified=False, virtual_target_executable_verified=False,
               world_synchronization_verified=False, atomic_snapshot_verified=False,
               physical_headset_acceptance=False, bodies=[], started_perf_counter_ns=time.perf_counter_ns())
    q = proof = None
    try:
        if (type(native_sha256) is not str or len(native_sha256) != 64 or
                any(c not in '0123456789abcdef' for c in native_sha256)):
            raise ValueError('Exact already loaded Native SHA required')
        lifecycle = surface._lifecycle(observer)
        disk = surface._disk_module(observer)
        chunks = historical_loader()
        game, owner = (lifecycle['identities'][k] for k in ('game_identity', 'owner_identity'))
        dll = Path(native_dll).resolve()
        proof = attestation_factory(dll.with_name('hook-entry-manifest.json'), pid=game['pid'],
                                    game_creation=game['creation_time'], owner_identity=owner,
                                    dll=dll, expected_dll_sha256=native_sha256)
        q = query_factory(observer)
        module = _module(q, SimpleNamespace(module=disk), game)
        if proof.module_base != module['base']:
            raise RuntimeError('Physics sealed Native and reader module bases differ')
        code = _code(q, module, proof, chunks)
        roles = _mesh(q)
        cover_roles = surface._roles(q)
        if any(cover_roles[k] != roles[k] for k in ('controller', 'pawn', 'world')):
            raise RuntimeError('Physics mesh and current cover associations differ')
        cover_state = {name: q.scalar(roles['pawn'], name, 'ByteProperty', '<B')
                       for name in ('CoverType', 'CoverAction')}
        if not cover_state['CoverType']:
            raise RuntimeError('Physics cover source requires current nonzero CoverType')
        mesh = roles['mesh']
        for name, offset in (('PhysicsAsset', 0x248), ('PhysicsAssetInstance', 0x250)):
            field = _field(q, mesh, name, 'ObjectProperty')
            if field['offset'] != offset or field['size'] != 8:
                raise RuntimeError('Physics mesh field differs from original consumer: ' + name)
        asset, instance = (_reference(q, mesh, name) for name in ('PhysicsAsset', 'PhysicsAssetInstance'))
        if (not asset or not instance or 'PhysicsAsset' not in lineage(q, asset) or
                'PhysicsAssetInstance' not in lineage(q, instance) or
                asset['name'].startswith('Default__') or instance['name'].startswith('Default__')):
            raise RuntimeError('Physics current asset/instance unavailable or wrong class')
        owner = _reference(q, instance, 'Owner')
        if owner not in (roles['pawn'], mesh):
            raise RuntimeError('Physics instance Owner is not the current Pawn/mesh')
        arrays, bodies = _array(q, instance, 'Bodies', 'RB_BodyInstance', 0x6c)
        setups_layout, setups = _array(q, asset, 'BodySetup', 'RB_BodySetup', 0x68)
        if len(bodies) != len(setups):
            raise RuntimeError('Physics instance/setup arrays differ in count')
        root = q.scalar(instance, 'RootBodyIndex', 'IntProperty', '<i')
        if not 0 <= root < len(bodies):
            raise RuntimeError('Physics RootBodyIndex is outside current body array')
        layouts, setup_layouts, prefixes = {}, {}, {}
        for index, (body, setup) in enumerate(zip(bodies, setups)):
            if body['cls'] not in layouts:
                index_field = _field(q, body, 'BodyIndex', 'IntProperty')
                owner_field = _field(q, body, 'OwnerComponent', 'ComponentProperty')
                if index_field['size'] != 4 or owner_field['size'] != 8:
                    raise RuntimeError('Physics body association field width differs')
                layouts[body['cls']] = dict(index=index_field, owner=owner_field,
                    main=_pointer_field(q, body, 'BodyData', 0x88),
                    auxiliary=_pointer_field(q, body, 'BoneSpring', 0x90))
            fields = layouts[body['cls']]
            if (struct.unpack('<i', q.value(body, fields['index']))[0] != index or
                    struct.unpack('<Q', q.value(body, fields['owner']))[0] != mesh['address']):
                raise RuntimeError('Physics body index/current OwnerComponent differs')
            if setup['cls'] not in setup_layouts:
                field = _field(q, setup, 'BoneName', 'NameProperty')
                if field['offset'] != 0xb4 or field['size'] != 8:
                    raise RuntimeError('Physics setup BoneName differs from original consumer')
                setup_layouts[setup['cls']] = field
            raw = q.value(setup, setup_layouts[setup['cls']])
            name_index, number = struct.unpack('<II', raw)
            name = q.r.name(name_index)
            if type(name) is not str or not 0 < len(name) <= 128:
                raise RuntimeError('Physics body BoneName text is unbounded')
            named = _name(q, name, name_index)
            named['number'] = number
            row = dict(index=index, instance=body, setup=setup, bone_name=named, bone_name_bytes=raw.hex())
            out['bodies'].append(row)
            for label in ('main', 'auxiliary'):
                raw = q.value(body, fields[label]['field'])
                pointer = struct.unpack('<Q', raw)[0]
                row[label] = _receiver(q, pointer, prefixes)
                row[label]['field_bytes'] = raw.hex()
        q.finish()
        proof.reverify()
        if surface._lifecycle(observer) != lifecycle:
            raise RuntimeError('Physics source session/game/owner changed at closing boundary')
        out.update(accepted=True, source=lifecycle, roles=roles, cover_roles=cover_roles,
                   cover_state=cover_state, module=module, code_chunks=code,
                   physics_asset=asset, physics_instance=instance, instance_owner=owner,
                   body_array=arrays, setup_array=setups_layout, root_body_index=root,
                   body_layouts=layouts, setup_layouts=setup_layouts, virtual_target_prefixes=list(prefixes.values()),
                   unique_main_receivers=len({b['main']['address'] for b in out['bodies'] if b['main']['present']}),
                   native_sha256=native_sha256,
                   limits='Closed source/array/index/owner and receiver/vtable/code-byte observations only; '
                          'no method names, pose coverage, active scene, hit/muzzle or synchronization proof')
    except (RuntimeError, OSError, ValueError, TypeError, KeyError, struct.error) as error:
        out.update(rejection=str(error), rejection_type=type(error).__name__)
        witness = getattr(q, 'closing_rejection', None)
        if witness is not None:
            out['closing_rejection'] = copy.deepcopy(witness)
    finally:
        if proof is not None:
            proof.close()
        if q is not None:
            out.update(reads=q.reads, bytes=q.bytes)
    out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out
