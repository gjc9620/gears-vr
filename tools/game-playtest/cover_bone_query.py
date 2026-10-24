"""Bounded diagnostic MatchRefBone/GetBoneMatrix calls on the existing GT queue.

The caller must own the exact session, call observer.sample() first, and supply
the already loaded Playground DLL and its SHA256. No hook, gameplay setter,
input, opaque RefSkeleton read, or pose write is performed. Each getter has its
own checked observation interval; several returned bones are NOT one pose epoch.

certificate_path explicitly selects the private reviewed getter certificate.
Every wrapper/implementation main/cold chunk is compared with CURRENT memory;
an old RVA alone never authorizes a call. Receipts contain private source bytes
and belong under ignored analysis/, never in a release package.
"""
import copy
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys
import time

from cover_state import CoverQuery, struct_type
from cover_animation_state import array_layout, lineage

PARAM_CAP, REQUEST_SIZE, MAX_BONES, CAPTURE_SECONDS = 4096, 24 + 4096, 8, 14.0
GETTERS = {'MatchRefBone': ('BoneName', 'NameProperty', 8, 'IntProperty', 4),
           'GetBoneMatrix': ('BoneIndex', 'IntProperty', 4, 'StructProperty', 64)}
# The purchased disk image and the read-only live t2 PE diagnostic agree on
# every other header byte. The observed runtime Resource directory uses an
# existing section with exactly this RVA/VirtualSize. This is not a general
# mutable-header mask or a claim about which runtime code changed the directory.
# Evidence: ignored analysis/cover-implementation-20261024/
# bone-getters-live-t2-pe-diagnostic.json (live header SHA256
# 33d6f8872cc0b3c5bf8d931579708a4113f767eb824b24fa8a7f758c820ba70d).
RUNTIME_RESOURCE_LAYOUTS = {
    'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4':
        ((0x2013000, 0x226cc), (0x1eae000, 0xb37e8)),
}


class SubmitFailure(RuntimeError):
    def __init__(self, message, *, pending_possible):
        super().__init__(message)
        self.pending_possible = pending_possible


class _AttachedBridge:
    """Compose Bridge; keep its attach/hash/identity gates, avoid Bridge.call.

    Bridge.allocate can lose the address if its initial write fails. Bridge.export
    waits a fixed 15 seconds. The two tiny operations below retain that address
    and bound the SAME PlaygroundSubmit export by this capture's deadline.
    """
    def __init__(self, identity, dll, *, expected_sha256, attach_only):
        if attach_only is not True:
            raise RuntimeError('Bone queries require attach_only')
        root = Path(__file__).resolve().parents[2]
        for relative in ('tools/package-inspector/python-libs', 'tools/playground'):
            directory = str(root / relative)
            if directory not in sys.path:
                sys.path.insert(0, directory)
        spec = importlib.util.spec_from_file_location('_cover_bone_playground_runtime', root / 'tools/playground/runtime.py')
        runtime = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(runtime)
        self.runtime = runtime
        self.bridge = None
        try:
            self.bridge = runtime.Bridge(identity, dll, expected_sha256=expected_sha256, attach_only=True)
            image = Path(identity['image']).resolve()
            pe = runtime.pefile.PE(str(image), fast_load=True)
            headers = bytes(pe.header)
            pe.close()
            disk_size = image.stat().st_size
            layout = _pe_layout(headers, disk_size)
            self.module = dict(base=self.bridge.r.base, path=str(image),
                               sha256=self.bridge.r.profile['sha256'], headers=headers,
                               disk_size=disk_size, size=layout['fields']['size_of_image'],
                               sections=layout['sections'])
            if 'PlaygroundSubmit' not in self.bridge.exports:
                raise RuntimeError('Loaded PlaygroundSubmit export unavailable')
            entry = self.bridge.base + self.bridge.exports['PlaygroundSubmit']
            dll_pe = runtime.pefile.PE(str(self.bridge.dll), fast_load=True)
            expected = dll_pe.get_data(self.bridge.exports['PlaygroundSubmit'], 32)
            dll_pe.close()
            if len(expected) != 32 or self.bridge.r.read(entry, 32) != expected:
                raise RuntimeError('Current PlaygroundSubmit entry differs from pinned DLL')
            self.submit_source = dict(entry=entry, bytes=expected.hex(),
                                      sha256=hashlib.sha256(expected).hexdigest(), dll_sha256=expected_sha256,
                                      dll_path=str(self.bridge.dll), dll_base=self.bridge.base)
        except BaseException as error:
            if self.bridge is not None:
                self.bridge.close()
            if isinstance(error, runtime.pefile.PEFormatError):
                raise RuntimeError('Bone bridge PE format rejected: ' + str(error)) from error
            raise

    def __getattr__(self, name):
        return getattr(self.bridge, name)

    def allocate(self, size):
        self.verify()
        address = self.runtime.k.VirtualAllocEx(self.handle, None, size, 0x3000, 4)
        if not address:
            raise OSError('Bone request VirtualAllocEx failed; no allocation returned')
        return address

    def read(self, address, size):
        self.verify()
        return self.r.read(address, size)

    def submit(self, address, timeout_ms):
        import ctypes as c
        from ctypes import wintypes as w
        self.verify()
        source = self.submit_source
        if self.r.read(source['entry'], 32).hex() != source['bytes']:
            raise SubmitFailure('PlaygroundSubmit entry changed before dispatch', pending_possible=False)
        thread = self.runtime.k.CreateRemoteThread(self.handle, None, 0, source['entry'], address, 0, None)
        if not thread:
            raise SubmitFailure('PlaygroundSubmit thread creation failed', pending_possible=False)
        try:
            status = self.runtime.k.WaitForSingleObject(thread, timeout_ms)
            if status != 0:
                raise SubmitFailure('PlaygroundSubmit remote thread completion unknown', pending_possible=True)
            code = w.DWORD()
            if not self.runtime.k.GetExitCodeThread(thread, c.byref(code)):
                raise SubmitFailure('PlaygroundSubmit exit code unavailable', pending_possible=True)
            return code.value
        finally:
            # Closing this handle does NOT cancel the thread or free its request.
            self.runtime.k.CloseHandle(thread)


def _remaining(deadline):
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise RuntimeError('Bone capture total time budget exhausted')
    return remaining


def _ownership(observer, expected=None):
    observer.verify()
    identities = getattr(observer, 'identities', {})
    owner = identities.get('owner_identity')
    if (not owner or getattr(observer, 'action_owner', None) != owner or
            observer.session.get('owner_identity') != owner or
            observer.session.get('game_identity') != identities.get('game_identity') or
            not observer.session.get('session_id')):
        raise RuntimeError('Bone getter requires this exact session action_owner')
    if observer.resolver is None or observer.sampler is None:
        raise RuntimeError('Call observer.sample() before bone getters')
    if expected is not None and (identities['game_identity'] != expected['game_identity'] or
            owner != expected['owner_identity'] or observer.session['session_id'] != expected['session_id']):
        raise RuntimeError('Bone capture lifecycle changed')
    return identities['game_identity']


def _chain(q, address):
    seen = set()
    while address:
        if address in seen or len(seen) >= 64:
            raise RuntimeError('Bone class/structure ancestry cycle or bound')
        seen.add(address)
        obj = q.obj(address)
        yield obj
        address = struct.unpack('<Q', q.read(address + q.r.profile['field_super'], 8))[0]


def _kind(q, obj):
    return q.obj(obj['cls'])['name']


def _field(q, obj, name, kind=None, *, structure=False):
    owners = list(_chain(q, obj['address'] if structure else obj['cls']))
    for owner in owners:
        try:
            field = q.prop(owner, name, kind, structure=True)
        except RuntimeError as error:
            if str(error) == 'Property ' + name + ' not found':
                continue
            raise
        # Authenticate membership only on the actual declaring owner. Visiting
        # every unrelated child of each non-declaring ancestor made those Next
        # links closing witnesses three more times for every getter, exhausting
        # the unchanged Query read budget on the retail reflected class graph.
        children = _children(q, owner)
        fresh = q.obj(field['address'])
        if (fresh['outer'] != owner['address'] or _kind(q, fresh) != field['type'] or
                'Property' not in lineage(q, fresh) or
                (kind is not None and field['type'] != kind)):
            raise RuntimeError('Bone field identity/class/owner rejected: ' + name)
        matches = [child for child in children if child['name'] == name]
        if len(matches) != 1 or matches[0] != fresh:
            raise RuntimeError('Bone property detached/ambiguous in current child chain: ' + name)
        return field
    raise RuntimeError('Required bone property not found: ' + name)


def _reference(q, obj, name):
    field = _field(q, obj, name)
    if field['type'] not in ('ObjectProperty', 'ComponentProperty') or field['size'] != 8:
        raise RuntimeError('Bone reference type/width rejected: ' + name)
    address = struct.unpack('<Q', q.value(obj, field))[0]
    return q.obj(address) if address else None


def _mesh(q):
    pc = q.obj(q.s.controller)
    pawn = _reference(q, pc, 'Pawn')
    if not pawn or 'WarPawn' not in lineage(q, pawn) or _reference(q, pawn, 'Controller') != pc:
        raise RuntimeError('Bone controlled pawn/possession rejected')
    world = _reference(q, pawn, 'WorldInfo')
    if not world or _reference(q, pc, 'WorldInfo') != world:
        raise RuntimeError('Bone controlled pawn world mismatch')
    mesh = _reference(q, pawn, 'Mesh')
    if (not mesh or mesh['name'].startswith('Default__') or
            'SkeletalMeshComponent' not in lineage(q, mesh) or _reference(q, mesh, 'Owner') != pawn):
        raise RuntimeError('Bone controlled mesh class/owner rejected')
    asset = _reference(q, mesh, 'SkeletalMesh')
    if not asset or 'SkeletalMesh' not in lineage(q, asset):
        raise RuntimeError('Bone skeletal asset class rejected')
    space_field = _field(q, mesh, 'SpaceBases', 'ArrayProperty')
    space = array_layout(q, mesh, 'SpaceBases', 512)
    inner = q.obj(struct.unpack('<Q', q.read(space_field['address'] + q.r.profile['struct_property_struct'], 8))[0])
    if _kind(q, inner) != space['inner_kind']:
        raise RuntimeError('Bone SpaceBases inner class changed')
    if not 0 < space['count'] <= 512:
        raise RuntimeError('Bone live SpaceBases count unavailable')
    # Its reflected inner may be native BoneAtom; only count is used. Never read
    # the payload or use its stride to manufacture a named-bone mapping.
    return dict(controller=pc, pawn=pawn, world=world, mesh=mesh, asset=asset,
                space_bases={k: v for k, v in space.items() if k != 'data'})


def _pe_layout(raw, disk_size):
    """Parse bounded PE32+ header structure without following disk/live RVAs."""
    if (type(raw) is not bytes or not 64 <= len(raw) <= 65536 or raw[:2] != b'MZ' or
            type(disk_size) is not int or not len(raw) <= disk_size < 1 << 32):
        raise RuntimeError('Bone module DOS/header/file bounds rejected')
    nt = struct.unpack_from('<I', raw, 0x3c)[0]
    if not 64 <= nt <= len(raw) - 24 or raw[nt:nt + 4] != b'PE\0\0':
        raise RuntimeError('Bone module PE signature/offset rejected')
    machine, count, stamp, symbols, symbol_count, optional_size, characteristics = struct.unpack_from('<HHIIIHH', raw, nt + 4)
    optional, table = nt + 24, nt + 24 + optional_size
    if (machine != 0x8664 or not 1 <= count <= 96 or optional_size != 240 or
            table + count * 40 > len(raw)):
        raise RuntimeError('Bone module PE64 machine/optional/section table bounds rejected')
    fields = dict(pe_offset=nt, machine=machine, section_count=count, timestamp=stamp,
                  symbol_pointer=symbols, symbol_count=symbol_count, optional_size=optional_size,
                  characteristics=characteristics)
    for name, offset, fmt in (('magic', 0, 'H'), ('entry_rva', 16, 'I'), ('image_base', 24, 'Q'),
                              ('section_alignment', 32, 'I'), ('file_alignment', 36, 'I'),
                              ('size_of_image', 56, 'I'), ('size_of_headers', 60, 'I'),
                              ('checksum', 64, 'I'), ('subsystem', 68, 'H'),
                              ('dll_characteristics', 70, 'H'), ('directory_count', 108, 'I')):
        fields[name] = struct.unpack_from('<' + fmt, raw, optional + offset)[0]
    image_size, alignment, file_alignment = (fields[name] for name in ('size_of_image', 'section_alignment', 'file_alignment'))
    if (fields['magic'] != 0x20b or fields['directory_count'] != 16 or
            fields['size_of_headers'] != len(raw) or not len(raw) <= image_size < 1 << 32 or
            not alignment or alignment & (alignment - 1) or not file_alignment or
            file_alignment & (file_alignment - 1) or alignment < file_alignment or
            image_size % alignment or len(raw) % file_alignment or
            not 0 < fields['image_base'] < (1 << 64) - image_size):
        raise RuntimeError('Bone module PE64 image/header/alignment fields rejected')
    sections = []
    virtual_intervals, raw_intervals = [], []
    for index in range(count):
        offset = table + index * 40
        name, virtual_size, rva, raw_size, raw_pointer, _, _, _, _, flags = struct.unpack_from('<8sIIIIIIHHI', raw, offset)
        size = max(virtual_size, raw_size)
        if (not size or rva < len(raw) or rva % alignment or rva + size > image_size or
                (raw_size and (raw_pointer < len(raw) or raw_pointer % file_alignment or
                               raw_pointer + raw_size > disk_size))):
            raise RuntimeError('Bone module section image/file bounds rejected')
        sections.append(dict(name=name.rstrip(b'\0').decode('ascii', errors='strict'), rva=rva, size=size,
                             virtual_size=virtual_size, raw_size=raw_size, raw_pointer=raw_pointer,
                             characteristics=flags))
        virtual_intervals.append((rva, rva + size))
        if raw_size:
            raw_intervals.append((raw_pointer, raw_pointer + raw_size))
    for intervals in (virtual_intervals, raw_intervals):
        intervals.sort()
        if any(end > start for (_, end), (start, _) in zip(intervals, intervals[1:])):
            raise RuntimeError('Bone module overlapping section layout rejected')
    if not any(s['rva'] <= fields['entry_rva'] < s['rva'] + s['size'] and
               s['characteristics'] & 0x20000000 for s in sections):
        raise RuntimeError('Bone module entry outside executable section')
    directories, directory_offsets, directory_sections = {}, {}, {}
    # These indices are PE32+ definitions: Import=1, Resource=2. The recorded
    # six-byte delta at 0x279 belongs to Resource; Import at 0x270 is immutable.
    for name, index in (('import', 1), ('resource', 2)):
        offset = optional + 112 + index * 8
        rva, size = struct.unpack_from('<II', raw, offset)
        section = None
        if rva or size:
            section = next((s for s in sections if s['characteristics'] & 0x40000000 and
                            s['rva'] <= rva and rva + size <= s['rva'] + s['size']), None)
            if not rva or not size or rva + size > image_size or section is None:
                raise RuntimeError('Bone module ' + name.title() + ' directory image/section bounds rejected')
        directories[name], directory_offsets[name], directory_sections[name] = (rva, size), offset, section
    return dict(fields=fields, sections=sections, directories=directories,
                directory_offsets=directory_offsets, directory_sections=directory_sections)


def _module(q, bridge, game):
    module = bridge.module
    if (q.r.base != module['base'] or q.r.profile['sha256'] != module['sha256'] or
            Path(module['path']).resolve() != Path(game['image']).resolve() or
            getattr(q.r, 'pid', None) != game['pid']):
        raise RuntimeError('Bone reader/current module identity mismatch')
    disk_headers = module['headers']
    disk = _pe_layout(disk_headers, module['disk_size'])
    # CoverQuery remembers the entire LIVE header and rereads it at every
    # closing boundary, including after private request write and GT completion.
    live_headers = q.read(module['base'], len(disk_headers))
    live = _pe_layout(live_headers, module['disk_size'])
    if (disk['fields'] != live['fields'] or disk['sections'] != live['sections'] or
            module['size'] != disk['fields']['size_of_image'] or module['sections'] != disk['sections'] or
            not 0 < module['base'] < (1 << 64) - module['size']):
        raise RuntimeError('Bone current module PE identity/section layout differs')
    offset = disk['directory_offsets']['resource']
    if disk_headers[:offset] != live_headers[:offset] or disk_headers[offset + 8:] != live_headers[offset + 8:]:
        raise RuntimeError('Bone current module immutable PE header bytes differ')
    policy = 'unchanged_disk_resource'
    if disk['directories']['resource'] != live['directories']['resource']:
        if RUNTIME_RESOURCE_LAYOUTS.get(module['sha256']) != (disk['directories']['resource'], live['directories']['resource']):
            raise RuntimeError('Bone current module Resource directory is not a reviewed runtime layout')
        section = live['directory_sections']['resource']
        if (section is None or (section['rva'], section['virtual_size']) != live['directories']['resource']):
            raise RuntimeError('Bone current module runtime Resource section layout differs')
        policy = 'reviewed_runtime_resource_relocation'
    result = {k: v for k, v in module.items() if k != 'headers'}
    result.update(disk_sha256=module['sha256'], sha256_scope='on_disk_executable',
                  pe_validation=dict(disk_header_sha256=hashlib.sha256(disk_headers).hexdigest(),
                                     live_header_sha256=hashlib.sha256(live_headers).hexdigest(),
                                     non_resource_header_bytes_equal=True, fields=live['fields'],
                                     disk_import=dict(zip(('rva', 'size'), disk['directories']['import'])),
                                     live_import=dict(zip(('rva', 'size'), live['directories']['import'])),
                                     disk_resource=dict(zip(('rva', 'size'), disk['directories']['resource'])),
                                     live_resource=dict(zip(('rva', 'size'), live['directories']['resource'])),
                                     resource_policy=policy, live_header_closing_reread_required=True))
    return result


def _children(q, owner):
    if not hasattr(q, '_bone_children'):
        q._bone_children = {}
    if owner['address'] in q._bone_children:
        return q._bone_children[owner['address']]
    address = struct.unpack('<Q', q.read(owner['address'] + q.r.profile['struct_children'], 8))[0]
    seen, children = set(), []
    while address:
        if address in seen or len(seen) >= 2048:
            raise RuntimeError('Bone reflected child chain cycle or bound')
        seen.add(address)
        obj = q.obj(address)
        if obj['outer'] != owner['address']:
            raise RuntimeError('Bone reflected child owner mismatch')
        children.append(obj)
        address = struct.unpack('<Q', q.read(address + q.r.profile['field_next'], 8))[0]
    q._bone_children[owner['address']] = children
    return children


def _matrix_shape(q, field):
    typ = q.obj(struct_type(q, field))
    if typ['name'] != 'Matrix' or 'ScriptStruct' not in lineage(q, typ) or field['size'] != 64:
        raise RuntimeError('Bone return is not a reflected 64-byte Matrix')
    shapes = []
    for index, name in enumerate(('XPlane', 'YPlane', 'ZPlane', 'WPlane')):
        plane = _field(q, typ, name, 'StructProperty', structure=True)
        nested = q.obj(struct_type(q, plane))
        if (plane['offset'] != index * 16 or plane['size'] != 16 or nested['name'] != 'Plane' or
                'ScriptStruct' not in lineage(q, nested)):
            raise RuntimeError('Bone Matrix Plane layout rejected')
        components = []
        for component, label in enumerate(('X', 'Y', 'Z', 'W')):
            scalar = _field(q, nested, label, 'FloatProperty', structure=True)
            if scalar['offset'] != component * 4 or scalar['size'] != 4:
                raise RuntimeError('Bone Plane Float layout rejected')
            components.append(scalar)
        shapes.append(dict(field=plane, structure=nested, components=components))
    return dict(identity=typ, planes=shapes)


def _certificate(path):
    if path is None:
        raise RuntimeError('Reviewed private certificate_path required for both bone getters')
    path = Path(path).resolve()
    if path.stat().st_size > 262144:
        raise RuntimeError('Bone getter certificate exceeds bounded size')
    raw = path.read_bytes()
    if len(raw) > 262144:
        raise RuntimeError('Bone getter certificate grew beyond bounded size')
    value = json.loads(raw)
    if (type(value) is not dict or type(value.get('schema')) is not int or value['schema'] != 1 or
            value.get('kind') != 'cover_bone_getter_certificate' or
            type(value.get('functions')) is not dict or set(value['functions']) != set(GETTERS) or
            type(value.get('game_sha256')) is not str or len(value['game_sha256']) != 64):
        raise RuntimeError('Bone getter certificate schema rejected')
    for name, function in value['functions'].items():
        if (type(function) is not dict or type(function.get('return_bytes')) is not int or
                function['return_bytes'] != GETTERS[name][-1] or
                type(function.get('chunks')) is not list or not 1 <= len(function['chunks']) <= 8):
            raise RuntimeError('Bone getter certificate signature/chunk bounds rejected')
        entry = _rva(function.get('entry_rva'))
        total, intervals = 0, []
        for chunk in function['chunks']:
            if type(chunk) is not dict or type(chunk.get('bytes')) is not str:
                raise RuntimeError('Bone certificate chunk shape rejected')
            start = _rva(chunk.get('rva'))
            data = bytes.fromhex(chunk['bytes'])
            if not 1 <= len(data) <= 8192 or hashlib.sha256(data).hexdigest() != chunk.get('sha256'):
                raise RuntimeError('Bone certificate chunk bytes/SHA256 rejected')
            total += len(data)
            intervals.append((start, start + len(data)))
        intervals.sort()
        if (total > 16384 or not any(start == entry for start, _ in intervals) or
                any(right > start for (_, right), (start, _) in zip(intervals, intervals[1:]))):
            raise RuntimeError('Bone certificate entry/overlap/total bounds rejected')
    return value, dict(path=str(path), sha256=hashlib.sha256(raw).hexdigest())


def _rva(value):
    if type(value) is not str or not value.startswith('0x') or not 1 <= len(value[2:]) <= 8:
        raise RuntimeError('Bone certificate RVA encoding rejected')
    if any(c not in '0123456789abcdefABCDEF' for c in value[2:]):
        raise RuntimeError('Bone certificate RVA encoding rejected')
    return int(value, 16)


def _function(q, mesh, name, module, certificates):
    function = None
    for owner in _chain(q, mesh['cls']):
        matches = [child for child in _children(q, owner) if child['name'] == name]
        if len(matches) > 1:
            raise RuntimeError('Ambiguous bone getter child identity')
        if matches:
            function = matches[0]
            break
    if function is None or 'Function' not in lineage(q, function):
        raise RuntimeError('Reflected bone Function unavailable: ' + name)
    header = q.read(function['address'], 0x11c)
    native = struct.unpack_from('<H', header, 0xf8)[0]
    size, return_offset = struct.unpack_from('<HH', header, 0x106)
    entry = struct.unpack_from('<Q', header, 0x114)[0]
    if native != 0 or not 0 < size <= PARAM_CAP:
        raise RuntimeError('Bone getter NativeIndex/ParmsSize rejected')
    argument, input_kind, input_size, output_kind, output_size = GETTERS[name]
    children = list(_children(q, function))
    if len(children) != 2 or {x['name'] for x in children} != {argument, 'ReturnValue'}:
        raise RuntimeError('Bone getter signature has missing/extra fields')
    fields = {}
    for label, kind, width in ((argument, input_kind, input_size), ('ReturnValue', output_kind, output_size)):
        p = _field(q, function, label, kind, structure=True)
        if (p['address'] not in {c['address'] for c in children} or p['size'] != width or
                p['offset'] < 0 or p['offset'] + width > size):
            raise RuntimeError('Bone getter field width/range rejected: ' + label)
        fields[label] = p
    arg, result = fields[argument], fields['ReturnValue']
    if (result['offset'] != return_offset or
            max(arg['offset'], result['offset']) < min(arg['offset'] + input_size, result['offset'] + output_size)):
        raise RuntimeError('Bone getter return offset/overlap rejected')
    shape = _matrix_shape(q, result) if name == 'GetBoneMatrix' else None
    certificate = certificates['functions'][name]
    rva = entry - module['base']
    if certificates['game_sha256'] != module['sha256'] or _rva(certificate['entry_rva']) != rva:
        raise RuntimeError('Bone reviewed entry certificate/range rejected')
    sources = []
    for chunk in certificate['chunks']:
        start, expected = _rva(chunk['rva']), bytes.fromhex(chunk['bytes'])
        section = next((s for s in module['sections'] if s['characteristics'] & 0x20000000 and
                        s['characteristics'] & 0x40000000 and s['name'] not in ('.themida', '.boot') and
                        s['rva'] <= start and start + len(expected) <= s['rva'] + s['size']), None)
        if section is None or not 0 <= start < module['size'] or start + len(expected) > module['size']:
            raise RuntimeError('Bone certificate chunk outside current executable text')
        current = q.read(module['base'] + start, len(expected))
        if current != expected:
            raise RuntimeError('Current bone wrapper/implementation chunk differs from reviewed certificate')
        sources.append(dict(rva=hex(start), bytes=current.hex(), sha256=hashlib.sha256(current).hexdigest(),
                            section=section, compared_in_current_process=True))
    return dict(identity=function, native_index=native, parms_size=size, return_offset=return_offset,
                entry=entry, rva=rva, argument=argument, fields=fields, return_structure=shape,
                source=dict(header_bytes=header.hex(), header_sha256=hashlib.sha256(header).hexdigest(),
                            chunks=sources, compared_in_current_process=True, module=module))


def _find_names(observer, names, deadline):
    """Bounded exact candidates; selected names are recertified by each Query.

    Unrelated names can exceed Resolver.name's 63-unit bound. Do not decode or
    impose selected-name constraints on them while finding a short exact name.
    """
    r = observer.resolver
    header = r.read(r.names_header, 16)
    data, count, capacity = struct.unpack('<Qii', header)
    if not data or not 0 < count <= capacity <= 4000000 or count * 8 > 2 * 1024 * 1024:
        raise RuntimeError('Bone name discovery registry bounds/budget rejected')
    pointers = struct.unpack('<' + str(count) + 'Q', r.read(data, count * 8))
    found = {}
    encoded = {name: (name + '\0').encode('utf-16-le') for name in names}
    width = max(map(len, encoded.values()))
    for index, entry in enumerate(pointers):
        _remaining(deadline)
        if not entry:
            continue
        address = entry + r.profile['name_entry_text']
        raw = b''
        while len(raw) < width:
            current = address + len(raw)
            raw += r.read(current, min(width - len(raw), 4096 - current % 4096))
            # A shorter name may terminate immediately before an unreadable
            # page. Only continue across that boundary for a possible target.
            if (any(raw[i:i + 2] == b'\0\0' for i in range(0, len(raw) - 1, 2)) or
                    not any(value.startswith(raw) for value in encoded.values())):
                break
        for name, expected in encoded.items():
            if raw[:len(expected)] == expected:
                found[name] = index
        if len(found) == len(names):
            break
    if r.read(r.names_header, 16) != header:
        raise RuntimeError('Bone name discovery registry changed')
    if len(found) != len(names):
        raise RuntimeError('Requested exact bone FName absent from current registry')
    return found


def _name(q, name, index):
    data, count, capacity = struct.unpack('<Qii', q.read(q.r.names_header, 16))
    if not data or not 0 <= index < count <= capacity <= 4000000:
        raise RuntimeError('Bone Name index outside current registry')
    entry = struct.unpack('<Q', q.read(data + index * 8, 8))[0]
    if not entry or struct.unpack('<I', q.read(entry + q.r.profile['name_entry_index'], 4))[0] != index:
        raise RuntimeError('Bone Name registry slot/index mismatch')
    expected = (name + '\0').encode('utf-16-le')
    if q.read(entry + q.r.profile['name_entry_text'], len(expected)) != expected:
        raise RuntimeError('Bone Name exact spelling changed')
    return dict(index=index, number=0, name=name, entry=entry)


def _call(observer, bridge, query_factory, roles, getter, bone, index, name_index, certificates, deadline, receipt):
    started = time.perf_counter_ns()
    row = dict(getter=getter, bone_name=bone, accepted=False, started_perf_counter_ns=started,
               allocation_retained=False, submission='not_attempted', completion_observed=False)
    receipt['calls'].append(row)
    address = None
    completed = False
    try:
        game = _ownership(observer, receipt)
        q = query_factory(observer)
        current = _mesh(q)
        if current != roles:
            raise RuntimeError('Bone controlled mesh/asset/SpaceBases identity changed between getters')
        module = _module(q, bridge, game)
        function = _function(q, roles['mesh'], getter, module, certificates)
        row['function'] = function
        if getter == 'MatchRefBone':
            row['bone_fname'] = _name(q, bone, name_index)
            argument = struct.pack('<II', name_index, 0)
        else:
            _name(q, bone, name_index)
            if type(index) is not int or not 0 <= index < current['space_bases']['count']:
                raise RuntimeError('BoneIndex outside current authenticated SpaceBases count')
            argument = struct.pack('<i', index)
        params = bytearray(PARAM_CAP)
        offset = function['fields'][function['argument']]['offset']
        params[offset:offset + len(argument)] = argument
        raw = struct.pack('<iIQQ', 0, function['parms_size'], roles['mesh']['address'], function['identity']['address']) + params
        q.finish()
        _remaining(deadline)
        address = bridge.allocate(REQUEST_SIZE)
        if type(address) is not int or not 0 < address < 1 << 64:
            raise RuntimeError('Bone request allocation address invalid')
        row.update(allocation_address=address, allocation_size=REQUEST_SIZE, allocation_retained=True,
                   allocation_release_condition='Only observed queue state 2; otherwise retain until ORIGINAL process exits')
        bridge.write(address, raw)
        q.finish()
        _ownership(observer, receipt)
        row.update(submission='attempted_result_unknown', pending_possible=True)
        result = bridge.submit(address, max(1, min(1000, int(_remaining(deadline) * 1000))))
        row.update(submit_result=result, submission={0: 'queued', 1: 'rejected_invalid', 2: 'rejected_busy'}.get(result, 'unknown_return'))
        row['pending_possible'] = result not in (1, 2)
        if result != 0:
            raise RuntimeError('PlaygroundSubmit did not accept request: ' + str(result))
        call_deadline = min(deadline, time.monotonic() + 1.5)
        while True:
            _ownership(observer, receipt)
            bridge.verify()
            state = struct.unpack('<i', bridge.read(address, 4))[0]
            row['queue_state'] = state
            if state == 2:
                completed = True
                row.update(completion_observed=True, pending_possible=False)
                break
            if state != 0:
                if state == -1:
                    row['pending_possible'] = False
                raise RuntimeError('Bone request queue rejected/unknown state: ' + str(state))
            if time.monotonic() >= call_deadline:
                raise RuntimeError('Bone request still pending; allocation retained; do not retry')
            time.sleep(min(.005, max(0, call_deadline - time.monotonic())))
        returned = bridge.read(address, REQUEST_SIZE)
        if (len(returned) != REQUEST_SIZE or returned[:24] != struct.pack('<iIQQ', 2, function['parms_size'], roles['mesh']['address'], function['identity']['address']) or
                returned[24 + offset:24 + offset + len(argument)] != argument):
            raise RuntimeError('Bone completed Request header/argument changed')
        begin = 24 + function['return_offset']
        width = function['fields']['ReturnValue']['size']
        value = returned[begin:begin + width]
        if getter == 'MatchRefBone':
            decoded = struct.unpack('<i', value)[0]
            if not 0 <= decoded < current['space_bases']['count']:
                raise RuntimeError('MatchRefBone returned unavailable/out-of-range index')
        else:
            decoded = list(struct.unpack('<16f', value))
            if not all(math.isfinite(v) for v in decoded):
                raise RuntimeError('GetBoneMatrix returned nonfinite values')
        q.finish()
        _ownership(observer, receipt)
        _remaining(deadline)
        row.update(accepted=True, value=decoded, return_bytes=value.hex())
        return decoded
    except SubmitFailure as error:
        row['pending_possible'] = error.pending_possible
        raise
    finally:
        if completed:
            # Native consumer has published completion. Never do this in an
            # unconditional cleanup, including Submit/metadata/identity failures.
            try:
                bridge.free(address)
                row['allocation_retained'] = False
            except (OSError, RuntimeError) as error:
                row['free_error'] = str(error)
        row['finished_perf_counter_ns'] = time.perf_counter_ns()


def capture(observer, dll_path, dll_sha256, bone_names, *, certificate_path=None,
            bridge_factory=_AttachedBridge, query_factory=CoverQuery):
    """Return private JSON diagnostics; guards reject before/after each GT call.

    Bridge factories are a test seam for the above minimal transport protocol,
    not alternative gameplay invocation backends. There is no automatic retry.
    """
    out = dict(schema=1, accepted=False, activation_allowed=False, diagnostic_getters_only=True,
               actor_or_mesh_fields_written=False, atomic_snapshot_verified=False,
               same_pose_epoch_verified=False, collision_verified=False,
               world_matrix_getbonelocation_pair_verified=False,
               started_perf_counter_ns=time.perf_counter_ns(), calls=[], bones=[],
               gaps=['GetBoneLocation paired runtime validation is not implemented',
                     'Getter matrices do not prove collision/hit/physics or renderer alignment'])
    deadline = time.monotonic() + CAPTURE_SECONDS
    bridge = None
    try:
        if (not isinstance(bone_names, (list, tuple)) or not 1 <= len(bone_names) <= MAX_BONES or
                any(type(x) is not str or not x or '\0' in x or len(x.encode('utf-16-le')) > 126 for x in bone_names) or
                len(set(bone_names)) != len(bone_names)):
            raise ValueError('One to eight unique exact bone names of at most 63 UTF16 units required')
        if (type(dll_sha256) is not str or len(dll_sha256) != 64 or
                any(x not in '0123456789abcdef' for x in dll_sha256)):
            raise ValueError('Exact lowercase Playground DLL SHA256 required')
        game = _ownership(observer)
        out.update(session_id=observer.session['session_id'], game_identity=copy.deepcopy(game),
                   owner_identity=copy.deepcopy(observer.identities['owner_identity']))
        q = query_factory(observer)
        roles = _mesh(q)
        q.finish()
        out['roles'] = roles
        certificates, out['certificate'] = _certificate(certificate_path)
        names = _find_names(observer, bone_names, deadline)
        _remaining(deadline)
        bridge = bridge_factory(game, dll_path, expected_sha256=dll_sha256, attach_only=True)
        out['playground_submit'] = bridge.submit_source
        for bone in bone_names:
            index = _call(observer, bridge, query_factory, roles, 'MatchRefBone', bone, None,
                          names[bone], certificates, deadline, out)
            matrix = _call(observer, bridge, query_factory, roles, 'GetBoneMatrix', bone, index,
                           names[bone], certificates, deadline, out)
            out['bones'].append(dict(name=bone, number=0, index=index, matrix=matrix,
                                     matrix_space='world per saved getter dataflow; live GetBoneLocation pair not checked',
                                     match_call=len(out['calls']) - 2, matrix_call=len(out['calls']) - 1))
        _ownership(observer, out)
        _remaining(deadline)
        out['accepted'] = True
    except (RuntimeError, OSError, ValueError, TypeError, KeyError, ImportError, struct.error, UnicodeError) as error:
        out.update(rejection=str(error), rejection_type=type(error).__name__)
        if out['calls']:
            out['calls'][-1].update(rejection=str(error), rejection_type=type(error).__name__)
    finally:
        if bridge is not None:
            try:
                bridge.close()  # CloseHandle does not release retained remote allocations.
            except (OSError, RuntimeError) as error:
                out.update(accepted=False, close_error=str(error))
                out.setdefault('rejection', 'Bone Bridge close failed: ' + str(error))
        out['retained_allocations'] = [dict(address=c['allocation_address'], size=c['allocation_size'],
                                            game_identity=out.get('game_identity'),
                                            pending_possible=c.get('pending_possible', c['submission'] in ('queued', 'attempted_result_unknown')),
                                            release_condition=c['allocation_release_condition'])
                                       for c in out['calls'] if c['allocation_retained']]
        out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out
