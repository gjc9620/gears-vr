"""Read-only current CoverLink getter / Actor.Trace metadata diagnostics.

Call observer.sample() first. This module never attaches a bridge, allocates a
request, invokes a function, submits a queue item or writes process memory.
Private receipts contain reflected headers and belong under ignored analysis/.
"""
import copy
import hashlib
from pathlib import Path
import struct
import time
from types import SimpleNamespace

from cover_state import CoverQuery
from cover_animation_state import array_layout, lineage
from cover_bone_query import _chain, _children, _field, _reference, _module, _pe_layout


GAME_SHA256 = 'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4'
HISTORICAL_SHA256 = '911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6'
HISTORICAL_SIZE, HISTORICAL_RVA = 20027727, 0x1000
DEFAULT_BLOB = Path('J:/gearsVr/analysis/ghidra/render-16812-verified/section_1000.bin')
TRACE_RVA, TRACE_INDEX, GNATIVES_RVA = 0xcdacc, 277, 0x1b86280
TRACE_CHUNKS = ((0xcdacc, 1873, '44b9510b16dd79df98cd823a89f854b200cf6bad49f6b7c3cd8721c26cff1b22'),
                (0x89b1be, 186, '3d526e6b29f039bbbca1e7df396ec87919bbd4334d499b32cd10a3fe973455f2'))
HEADER_SIZE, PARAM_CAP, CAPTURE_SECONDS = 0x11c, 4096, 15
SLOT_SPECS = (('SlotIdx', 'IntProperty', 4, None), ('ReturnValue', 'StructProperty', 12, None))
TRACE_SPECS = (('HitLocation', 'StructProperty', 12, 'Vector'), ('HitNormal', 'StructProperty', 12, 'Vector'),
               ('TraceEnd', 'StructProperty', 12, 'Vector'), ('TraceStart', 'StructProperty', 12, 'Vector'),
               ('bTraceActors', 'BoolProperty', 4, None), ('Extent', 'StructProperty', 12, 'Vector'),
               ('HitInfo', 'StructProperty', 40, 'TraceHitInfo'), ('ExtraTraceFlags', 'IntProperty', 4, None),
               ('ReturnValue', 'ObjectProperty', 8, None))
HIT_SPECS = (('Material', 'ObjectProperty', 8, 0), ('PhysMaterial', 'ObjectProperty', 8, 8),
             ('Item', 'IntProperty', 4, 16), ('LevelIndex', 'IntProperty', 4, 20),
             ('BoneName', 'NameProperty', 8, 24), ('HitComponent', 'ComponentProperty', 8, 32))


def _historical(path):
    path = Path(path).resolve()
    if path.stat().st_size != HISTORICAL_SIZE:
        raise RuntimeError('Surface historical code blob size rejected')
    with path.open('rb') as stream:
        raw = stream.read(HISTORICAL_SIZE + 1)
    if len(raw) != HISTORICAL_SIZE or hashlib.sha256(raw).hexdigest() != HISTORICAL_SHA256:
        raise RuntimeError('Surface historical code blob SHA256 rejected')
    chunks = []
    for rva, size, digest in TRACE_CHUNKS:
        offset = rva - HISTORICAL_RVA
        data = raw[offset:offset + size] if offset >= 0 else b''
        if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
            raise RuntimeError('Surface historical Trace full chunk rejected')
        chunks.append((rva, data, digest))
    return dict(path=str(path), sha256=HISTORICAL_SHA256, size=len(raw), section_rva=hex(HISTORICAL_RVA),
                provenance='historical PID16812 runtime section; not disk EXE or current process'), chunks


def _disk_module(observer):
    r, game = observer.resolver, observer.identities['game_identity']
    path = Path(game['image']).resolve()
    if (r.profile['sha256'] != GAME_SHA256 or Path(r.path).resolve() != path or
            path.name.lower() != 'gowde-steam.exe' or r.pid != game['pid']):
        raise RuntimeError('Surface selected disk/reader identity rejected')
    before = path.stat()
    if not 512 <= before.st_size < 1 << 32:
        raise RuntimeError('Surface executable disk bounds rejected')
    digest, total, started = hashlib.sha256(), 0, time.monotonic()
    with path.open('rb') as stream:
        while block := stream.read(1024 * 1024):
            total += len(block)
            if total > before.st_size or time.monotonic() - started > CAPTURE_SECONDS:
                raise RuntimeError('Surface executable grew/hash time budget exceeded')
            digest.update(block)
        stream.seek(0)
        prefix = stream.read(min(65536, before.st_size))
    if total != before.st_size or digest.hexdigest() != GAME_SHA256:
        raise RuntimeError('Surface current disk executable SHA256 rejected')
    nt = struct.unpack_from('<I', prefix, 0x3c)[0]
    if not 64 <= nt <= len(prefix) - 88:
        raise RuntimeError('Surface disk PE offset rejected')
    header_size = struct.unpack_from('<I', prefix, nt + 24 + 60)[0]
    if not 512 <= header_size <= len(prefix):
        raise RuntimeError('Surface disk PE header size rejected')
    headers = prefix[:header_size]
    layout = _pe_layout(headers, before.st_size)
    after = path.stat()
    if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
        raise RuntimeError('Surface disk executable changed during hash/header read')
    return dict(base=r.base, path=str(path), sha256=GAME_SHA256, headers=headers,
                disk_size=before.st_size, size=layout['fields']['size_of_image'], sections=layout['sections'])


def _lifecycle(observer):
    observer.verify()
    if observer.resolver is None or observer.sampler is None:
        raise RuntimeError('Surface query requires observer.sample() first')
    identities = copy.deepcopy(observer.identities)
    if (not observer.session.get('session_id') or
            any(observer.session.get(key) != identities.get(key) for key in ('game_identity', 'owner_identity')) or
            not identities.get('game_identity') or not identities.get('owner_identity')):
        raise RuntimeError('Surface current session/owner identity rejected')
    return dict(session_id=observer.session['session_id'], identities=identities)


def _kind(q, obj):
    return q.obj(obj['cls'])['name']


def _range(module, rva, size, executable=False):
    if type(rva) is not int or not 0 <= rva < module['size'] or not 0 < size <= module['size'] - rva:
        raise RuntimeError('Surface module RVA/range rejected')
    section = next((s for s in module['sections'] if s['characteristics'] & 0x40000000 and
                    (not executable or (s['characteristics'] & 0x20000000 and s['name'] not in ('.boot', '.themida'))) and
                    s['rva'] <= rva and rva + size <= s['rva'] + s['size']), None)
    if section is None:
        raise RuntimeError('Surface module section permissions/bounds rejected')
    return section


def _roles(q):
    pc = q.obj(q.s.controller)
    pawn = _reference(q, pc, 'Pawn')
    if (not pawn or pawn['name'].startswith('Default__') or 'WarPawn' not in lineage(q, pawn) or
            _reference(q, pawn, 'Controller') != pc):
        raise RuntimeError('Surface controlled WarPawn/possession rejected')
    world = _reference(q, pawn, 'WorldInfo')
    link = _reference(q, pawn, 'CurrentLink')
    if not world or _reference(q, pc, 'WorldInfo') != world:
        raise RuntimeError('Surface controller/pawn WorldInfo mismatch')
    if (not link or link['name'].startswith('Default__') or 'CoverLink' not in lineage(q, link) or
            _reference(q, link, 'WorldInfo') != world):
        raise RuntimeError('Surface current CoverLink class/WorldInfo rejected')
    slot = q.scalar(pawn, 'CurrentSlotIdx', 'IntProperty', '<i')
    slots = array_layout(q, link, 'Slots', 256, expected='CoverSlot')
    slots_field = _field(q, link, 'Slots', 'ArrayProperty')
    slots_raw = q.value(link, slots_field)
    data, count, capacity = struct.unpack('<Qii', slots_raw)
    if (slots['inner_kind'] != 'StructProperty' or slots['stride'] != 212 or
            _kind(q, slots['structure']) != 'ScriptStruct' or
            struct.unpack('<I', q.read(slots['structure']['address'] + 0x88, 4))[0] != 212 or
            not 0 <= slot < slots['count'] or data != slots['data'] or count != slots['count'] or
            not 0 < data < (1 << 64) - capacity * 212 or data & 3):
        raise RuntimeError('Surface current selected CoverSlot layout/bounds rejected')
    return dict(controller=pc, pawn=pawn, world=world, link=link, current_slot=slot,
                slots=dict(slots, capacity=capacity, header_bytes=slots_raw.hex()))


def _property(q, child, owner, bound):
    kind = _kind(q, child)
    if (child['outer'] != owner['address'] or child['number'] or 'Property' not in lineage(q, child)):
        raise RuntimeError('Surface selected property identity/owner rejected')
    raw, d0 = q.read(child['address'] + 0x70, 0x28), q.read(child['address'] + 0xd0, 8)
    dimension, width = struct.unpack_from('<II', raw)
    flags, offset = struct.unpack_from('<Q', raw, 8)[0], struct.unpack_from('<I', raw, 0x24)[0]
    if dimension != 1 or not 0 < width <= 1024 or offset > bound - width:
        raise RuntimeError('Surface parameter/component dimension/width/bounds rejected: ' + child['name'])
    result = dict(identity=child, name=child['name'], type=kind, dimension=dimension, size=width, offset=offset,
                  flags=flags, flags_hex=hex(flags), metadata_bytes=raw.hex(), d0_bytes=d0.hex())
    if kind == 'BoolProperty':
        mask = struct.unpack_from('<I', d0)[0]
        if width != 4 or not mask or mask & (mask - 1):
            raise RuntimeError('Surface BoolProperty D0 mask rejected')
        result['bool_mask'] = mask
    elif kind in ('StructProperty', 'ObjectProperty', 'ComponentProperty', 'ClassProperty'):
        target = q.obj(struct.unpack('<Q', d0)[0])
        if target['number'] or _kind(q, target) != ('ScriptStruct' if kind == 'StructProperty' else 'Class'):
            raise RuntimeError('Surface property D0 reflected target class rejected')
        result['target'] = target
    return result


def _shape(q, field, expected, structures):
    typ = field['target']
    if typ['name'] != expected or _kind(q, typ) != 'ScriptStruct':
        raise RuntimeError('Surface selected structure type rejected: ' + expected)
    size = struct.unpack('<I', q.read(typ['address'] + 0x88, 4))[0]
    if size != field['size']:
        raise RuntimeError('Surface selected structure size rejected')
    if expected in structures:
        if structures[expected]['identity'] != typ:
            raise RuntimeError('Surface duplicate named structure identity rejected')
        return
    if expected == 'TraceHitInfo':
        specs = HIT_SPECS
    else:
        names = ('X', 'Y', 'Z') if expected == 'Vector' else ('Pitch', 'Yaw', 'Roll')
        kind = 'FloatProperty' if expected == 'Vector' else 'IntProperty'
        specs = tuple((name, kind, 4, i * 4) for i, name in enumerate(names))
    children = _children(q, typ)
    if len(children) != len(specs) or {c['name'] for c in children} != {s[0] for s in specs}:
        raise RuntimeError('Surface selected structure children missing/extra/duplicate')
    components = []
    for name, kind, width, offset in specs:
        p = _property(q, next(c for c in children if c['name'] == name), typ, size)
        if (p['type'], p['size'], p['offset']) != (kind, width, offset):
            raise RuntimeError('Surface selected component layout/type rejected: ' + expected + '.' + name)
        components.append(p)
    structures[expected] = dict(identity=typ, size=size, components=components)


def _function(q, receiver, owner_name, name, module, structures, row):
    owners = [owner for owner in _chain(q, receiver['cls']) if owner['name'] == owner_name]
    if len(owners) != 1 or owners[0]['number'] or _kind(q, owners[0]) != 'Class':
        raise RuntimeError('Surface exact declaring class unavailable: ' + owner_name)
    owner = owners[0]
    matches = [c for c in _children(q, owner) if c['name'] == name]
    if len(matches) != 1 or matches[0]['number'] or _kind(q, matches[0]) != 'Function':
        raise RuntimeError('Surface Function absent/ambiguous/type rejected: ' + name)
    function = matches[0]
    header = q.read(function['address'], HEADER_SIZE)
    native = struct.unpack_from('<H', header, 0xf8)[0]
    size, return_offset = struct.unpack_from('<HH', header, 0x106)
    entry = struct.unpack_from('<Q', header, 0x114)[0]
    row.update(identity=function, declaring_class=owner, receiver=receiver, header_bytes=header.hex(),
               header_sha256=hashlib.sha256(header).hexdigest(), native_index=native, parms_size=size,
               return_offset=return_offset, function_flags=struct.unpack_from('<I', header, 0xf4)[0],
               function_flags_offset=0xf4,
               entry=entry, entry_rva=hex(entry - module['base']), fields=[])
    if not 0 < size <= PARAM_CAP or not 0 <= return_offset < size:
        raise RuntimeError('Surface current UFunction parameter header bounds rejected')
    _range(module, entry - module['base'], 1, True)
    specs = TRACE_SPECS if name == 'Trace' else tuple((n, k, w, ('Vector' if name == 'GetSlotLocation' else 'Rotator') if n == 'ReturnValue' else t) for n, k, w, t in SLOT_SPECS)
    children = _children(q, function)
    if len(children) != len(specs) or {c['name'] for c in children} != {s[0] for s in specs}:
        raise RuntimeError('Surface function children missing/extra/duplicate: ' + name)
    for label, kind, width, shape in specs:
        field = _property(q, next(c for c in children if c['name'] == label), function, size)
        row['fields'].append(field)
        if (field['type'], field['size']) != (kind, width) or not field['flags'] & 0x80:
            raise RuntimeError('Surface current typed function parameter rejected: ' + name + '.' + label)
        if (bool(field['flags'] & 0x400) != (label == 'ReturnValue') or
                (label == 'ReturnValue' and field['offset'] != return_offset)):
            raise RuntimeError('Surface current return flag/offset rejected')
        if shape:
            _shape(q, field, shape, structures)
    spans = sorted((f['offset'], f['offset'] + f['size']) for f in row['fields'])
    if any(end > start for (_, end), (start, _) in zip(spans, spans[1:])):
        raise RuntimeError('Surface overlapping current parameter fields rejected')
    row['metadata_accepted'] = True


def capture(observer, *, historical_blob=DEFAULT_BLOB, query_factory=CoverQuery):
    """Capture exactly three current reflected signatures and Trace code proof.

    Accepted certifies these read intervals only. Slot getter entries are new
    leads for parent-owned IDA; no wrapper/impl certificate is invented for them.
    """
    out = dict(schema=1, accepted=False, read_only=True, metadata_only=True, engine_calls=False, queue_submissions=0,
               activation_allowed=False, actor_or_mesh_fields_written=False, collision_verified=False,
               native_getter_verified=False, native_impl_verified=False, atomic_snapshot_verified=False,
               functions={}, structures={}, started_perf_counter_ns=time.perf_counter_ns())
    q = None
    started = time.monotonic()
    try:
        context = _lifecycle(observer)
        out['context'] = context
        source, expected = _historical(historical_blob)
        disk = _disk_module(observer)
        out['historical_source'] = source
        if disk['sha256'] != GAME_SHA256 or _lifecycle(observer) != context:
            raise RuntimeError('Surface lifecycle/source identity changed before query')
        if time.monotonic() - started > CAPTURE_SECONDS:
            raise RuntimeError('Surface pre-query time budget exceeded')
        q = query_factory(observer)
        module = _module(q, SimpleNamespace(module=disk), context['identities']['game_identity'])
        out['module'] = module
        roles = _roles(q)
        out['roles'] = roles
        for receiver, owner, name in ((roles['link'], 'CoverLink', 'GetSlotLocation'),
                                       (roles['link'], 'CoverLink', 'GetSlotRotation'),
                                       (roles['controller'], 'Actor', 'Trace')):
            row = dict(metadata_accepted=False, native_impl_verified=False, wrapper_code_verified=False)
            out['functions'][name] = row
            _function(q, receiver, owner, name, module, out['structures'], row)
        trace = out['functions']['Trace']
        if trace['native_index'] != TRACE_INDEX or trace['entry'] != module['base'] + TRACE_RVA:
            raise RuntimeError('Surface current Trace NativeIndex/entry differs from reviewed mapping')
        slot_rva = GNATIVES_RVA + TRACE_INDEX * 8
        _range(module, slot_rva, 8)
        slot_raw = q.read(module['base'] + slot_rva, 8)
        slot = struct.unpack('<Q', slot_raw)[0]
        out['trace_table'] = dict(index=TRACE_INDEX, table_rva=hex(GNATIVES_RVA), slot_rva=hex(slot_rva),
                                  entry=slot, entry_rva=hex(slot - module['base']), slot_bytes=slot_raw.hex(),
                                  equals_reflected_entry=slot == trace['entry'])
        if slot != trace['entry']:
            raise RuntimeError('Surface current Trace277 table/reflection mismatch')
        trace['chunks'] = []
        for rva, data, digest in expected:
            section = _range(module, rva, len(data), True)
            current = q.read(module['base'] + rva, len(data))
            equal = current == data
            trace['chunks'].append(dict(rva=hex(rva), size=len(data), expected_sha256=digest,
                                        current_sha256=hashlib.sha256(current).hexdigest(), equal=equal,
                                        differing_bytes=sum(a != b for a, b in zip(current, data)), section=section))
            if not equal:
                raise RuntimeError('Surface current Trace full chunk differs from historical source: ' + hex(rva))
        q.finish()
        if _roles(q) != roles:
            raise RuntimeError('Surface controlled link/slot binding changed at closing boundary')
        q.finish()
        if _lifecycle(observer) != context or time.monotonic() - started > CAPTURE_SECONDS:
            raise RuntimeError('Surface closing lifecycle/time boundary rejected')
        trace['wrapper_code_verified'] = True
        out.update(accepted=True, limits='Metadata and Trace wrapper bytes only. Slot wrappers/implementations, shared VM '
                   'route/patch ownership, collision semantics, legal grabbing and native getter execution remain unverified.')
    except (RuntimeError, OSError, ValueError, TypeError, KeyError, struct.error) as error:
        out.update(rejection=str(error), rejection_type=type(error).__name__)
    if q is not None:
        out['query_budget'] = dict(reads=q.reads, bytes=q.bytes, limits=dict(reads=16384, bytes=2 * 1024 * 1024, seconds=3))
    out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out
