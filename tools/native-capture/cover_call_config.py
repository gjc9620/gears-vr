"""Fresh passive cover-call configuration; no process opening, calls or writes.

The caller supplies its prepared Resolver/LiveRegistryAnchorSampler. Script bytes
are returned in private JSON for later native revalidation, never saved here.
Properties include raw flags; parameter/out/local classification and call-site
aliasing are deliberately unproven. This does not create or authorize a hook.
"""
import copy
import hashlib
import json
from pathlib import Path
import re
import struct
import time

GAME_SHA = 'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4'
STATES = ('PlayerTakingCover', 'PlayerTakingCover_Stationary')
SETTERS = ('SetPawnCoverAction', 'SetCoverDirection')
CALLER_LOCALS = {'PawnCT': ('ByteProperty', 1), 'PawnCA': ('ByteProperty', 1),
                 'PawnCD': ('ByteProperty', 1), 'bNewLeftSided': ('IntProperty', 4)}
FRAME = dict(node=0x18, object=0x20, code=0x28, locals=0x30, previous=0x38)
FUNCTION = dict(locals_size=0x88, script=0x8c, entry=0x114)
WIDTHS = {'ByteProperty': 1, 'IntProperty': 4, 'FloatProperty': 4, 'BoolProperty': 4,
          'ObjectProperty': 8, 'ComponentProperty': 8, 'ClassProperty': 8,
          'NameProperty': 8, 'StrProperty': 16, 'ArrayProperty': 16, 'InterfaceProperty': 16,
          'DelegateProperty': 16}
BOUNDARY_HEADER = Path(__file__).with_name('ui_call_boundary.h')
MAX_SCRIPT = 131072
MAX_FIELDS = 256
MAX_IDENTITIES = 1024
LAYOUT = dict(object_internal_index=8, object_outer=64, object_name=72, object_class=80,
              field_super=96, field_next=104, struct_children=128, name_entry_index=0,
              name_entry_text=20, property_offset_internal=148, struct_property_struct=208)


class CoverCallConfigRejected(RuntimeError):
    pass


class CoverCallRegistryChanged(CoverCallConfigRejected):
    """Reject registry replacement, invalid bounds or decreasing observed count."""


def _require(condition, reason):
    if not condition:
        raise CoverCallConfigRejected(reason)


def _boundary_certificate():
    raw = BOUNDARY_HEADER.read_bytes()
    source = raw.decode('utf-8')
    rva = re.search(r'\brva\s*=\s*(0x[0-9a-fA-F]+)\s*;', source)
    entry = re.search(r'\bentry_bytes\s*\{\{(.*?)\}\}\s*;', source, re.S)
    _require(rva is not None and entry is not None, 'Existing UI boundary certificate is not readable')
    values = re.findall(r'0x([0-9a-fA-F]{2})\b', entry.group(1))
    _require(len(values) == 32, 'Existing UI boundary entry certificate is not 32 bytes')
    return dict(rva=int(rva.group(1), 16), bytes=bytes(int(x, 16) for x in values),
                source_sha256=hashlib.sha256(raw).hexdigest())


class _Capture:
    def __init__(self, sampler):
        self.sampler, self.r, self.p = sampler, sampler.r, dict(sampler.r.profile)
        self.checks, self.ids, self.by_address, self.supers, self.structures = {}, [], {}, {}, {}
        self.header_addresses = dict(objects=self.r.objects_header, names=self.r.names_header)
        _require(self.header_addresses['objects'] != self.header_addresses['names'],
                 'Cover-call registry headers share an address')
        self.header_latest, self.closing_headers = {}, {}
        self.object_header = self.stable(self.header_addresses['objects'], 16)
        self.name_header = self.stable(self.header_addresses['names'], 16)
        self.objects, self.object_count = self.registry(self.object_header)
        self.names, self.name_count = self.registry(self.name_header)

    @staticmethod
    def registry(raw):
        pointer, count, capacity = struct.unpack('<Qii', raw)
        _require(pointer and not pointer % 8 and 1000 < count <= capacity < 4000000
                 and pointer <= (1 << 64) - 1 - capacity * 8, 'Invalid cover-call registry')
        return pointer, count

    def registry_observation(self, address, raw):
        if len(raw) != 16:
            raise CoverCallRegistryChanged('Incomplete cover-call registry header')
        try:
            self.registry(raw)
        except CoverCallConfigRejected as error:
            raise CoverCallRegistryChanged('Invalid cover-call registry header') from error
        previous = self.header_latest.get(address)
        if previous is not None:
            data, count, capacity = struct.unpack('<Qii', raw)
            old_data, old_count, old_capacity = struct.unpack('<Qii', previous)
            if data != old_data or capacity != old_capacity or count < old_count:
                raise CoverCallRegistryChanged('Cover-call registry allocation/capacity changed or count decreased')
        self.header_latest[address] = raw

    def stable(self, address, size):
        _require(type(address) is int and type(size) is int and 0 < size <= MAX_SCRIPT
                 and 0 < address < (1 << 64) - size, 'Invalid cover-call read range')
        first, second = self.r.read(address, size), self.r.read(address, size)
        key = address, size
        _require(len(self.checks) < 32768 or key in self.checks, 'Cover-call source-check bound exceeded')
        if size == 16 and address in self.header_addresses.values():
            self.registry_observation(address, first)
            self.registry_observation(address, second)
            # Keep the earliest count as the selected-index bound and original
            # source-check bytes; later growth never admits additional indices.
            return self.checks.setdefault(key, first)
        _require(len(first) == len(second) == size, 'Incomplete cover-call source read')
        if first != second or (key in self.checks and self.checks[key] != first):
            raise CoverCallConfigRejected('Cover-call source changed during acquisition')
        self.checks[key] = first
        return first

    def number(self, address, fmt='<Q'):
        return struct.unpack(fmt, self.stable(address, struct.calcsize(fmt)))[0]

    def fresh(self, address):
        _require(type(address) is int and address > 0, 'Null cover-call object')
        p = self.p
        index = self.number(address + p['object_internal_index'], '<I')
        ni, number = struct.unpack('<II', self.stable(address + p['object_name'], 8))
        cls = self.number(address + p['object_class'])
        outer = self.number(address + p['object_outer'])
        _require(index < self.object_count and ni < self.name_count
                 and self.number(self.objects + index * 8) == address, 'Cover-call object registry slot differs')
        entry = self.number(self.names + ni * 8)
        _require(entry and self.number(entry + p['name_entry_index'], '<I') == ni, 'Cover-call name slot differs')
        text = self.stable(entry + p['name_entry_text'], 128)
        end = next((i for i in range(0, 128, 2) if text[i:i + 2] == b'\0\0'), None)
        _require(end is not None, 'Cover-call name exceeds bounded UTF16 read')
        name = text[:end].decode('utf-16-le', errors='strict')
        _require(name and not any(x in name for x in '\0\r\n='), 'Invalid cover-call reflected name')
        return dict(address=address, internal_index=index, name_index=ni, number=number, name=name, cls=cls, outer=outer)

    def add(self, address):
        item = self.fresh(address)
        if address in self.by_address:
            index = self.by_address[address]
            _require(self.ids[index] == item, 'Cover-call selected identity changed')
            return index
        _require(len(self.ids) < MAX_IDENTITIES, 'Cover-call identity bound exceeded')
        index = len(self.ids)
        self.by_address[address] = index
        self.ids.append(item)
        return index

    def exact(self, address, name, kind, outer=None):
        item = self.fresh(address)
        actual = self.fresh(item['cls'])
        _require(item['name'] == name and not item['number'] and actual['name'] == kind
                 and not actual['number'] and self.fresh(actual['cls'])['name'] == 'Class'
                 and (outer is None or item['outer'] == outer), 'Cover-call exact owner/type differs: ' + name)
        self.add(address)
        self.add(item['cls'])
        return item

    def ancestors(self, cls):
        result, seen = [], set()
        while cls:
            _require(cls not in seen and len(seen) < 64, 'Cover-call class chain is cyclic/too deep')
            seen.add(cls)
            item = self.fresh(cls)
            self.exact(cls, item['name'], 'Class')
            parent = self.number(cls + self.p['field_super'])
            self.supers[cls] = parent
            result.append(item)
            cls = parent
        return result

    def children(self, owner):
        pointer = self.number(owner + self.p['struct_children'])
        result, seen, names = [], set(), set()
        while pointer:
            _require(pointer not in seen and len(seen) < 4096, 'Cover-call field chain is cyclic/too long')
            seen.add(pointer)
            item = self.fresh(pointer)
            if item['outer'] != owner:
                raise CoverCallConfigRejected('Cover-call reflected child has wrong owner: '
                                              + 'expected_owner=' + repr(self.fresh(owner)) + '; child=' + repr(item))
            key = item['name'], item['number']
            _require(key not in names, 'Duplicate cover-call reflected child name/number')
            names.add(key)
            result.append(item)
            pointer = self.number(pointer + self.p['field_next'])
        return result

    def child(self, owner, name, kind):
        candidates = [x for x in self.children(owner) if x['name'] == name and not x['number']]
        _require(len(candidates) == 1, 'Missing exact cover-call child: ' + name)
        return self.exact(candidates[0]['address'], name, kind, owner)

    def link(self, source, name, target):
        owner = self.fresh(source)
        prop = self.r.property(owner['cls'], name, True)
        field = self.exact(prop['address'], name, 'ObjectProperty')
        _require(field['outer'] in {x['address'] for x in self.ancestors(owner['cls'])}, 'Cover-call link owner differs')
        shape = self.stable(prop['address'] + 0x70, 8)
        offset = self.number(prop['address'] + 0x94, '<I')
        _require(prop['type'] == 'ObjectProperty' and struct.unpack('<II', shape) == (1, 8)
                 and offset == prop['offset'] and offset <= 65528, 'Cover-call link layout differs')
        _require(self.number(source + offset) == target, 'Cover-call controlled-role link differs: ' + name)

    def descriptor(self, item, owner, container_size, stack=()):
        address = item['address']
        kind = self.fresh(item['cls'])['name']
        _require(kind in WIDTHS or kind == 'StructProperty', 'Unsupported cover-call property type: ' + kind)
        self.exact(address, item['name'], kind, owner)
        dimension, width = struct.unpack('<II', self.stable(address + 0x70, 8))
        flags = self.stable(address + 0x78, 8)
        offset = self.number(address + 0x94, '<I')
        _require(0 < dimension <= 256 and 0 < width <= 65536 and offset < 65536,
                 'Invalid cover-call local/property dimensions')
        _require(container_size is None or offset + dimension * width <= container_size,
                 'Cover-call local/property exceeds containing struct')
        result = dict(identity=self.by_address[address], name=item['name'], type=kind, owner=self.add(owner),
                      offset=offset, width=width, dimension=dimension, storage_width=dimension * width,
                      flags_raw=flags.hex(), flags_interpreted=False)
        if kind in WIDTHS:
            _require(width == WIDTHS[kind], 'Cover-call property scalar/header width differs')
        if kind == 'StructProperty':
            structure = self.number(address + self.p['struct_property_struct'])
            identity = self.fresh(structure)
            self.exact(structure, identity['name'], 'ScriptStruct')
            _require(structure not in stack and len(stack) < 8, 'Cover-call struct recursion exceeds bound')
            size = self.number(structure + 0x88, '<I')
            _require(0 < size <= width, 'Cover-call embedded struct does not fit its element')
            result['structure'] = self.by_address[structure]
            if structure not in self.structures:
                fields = self.fields(structure, size, stack + (structure,))
                self.structures[structure] = dict(identity=self.by_address[structure], size=size, fields=fields)
        elif kind == 'ArrayProperty':
            _require(len(stack) < 8, 'Cover-call array recursion exceeds bound')
            inner = self.number(address + self.p['struct_property_struct'])
            value = self.fresh(inner)
            _require(value['outer'] == address, 'Cover-call array inner owner differs')
            result['inner'] = self.descriptor(value, address, None, stack + (address,))
            _require(result['inner']['offset'] == 0, 'Cover-call array element offset is not zero')
        return result

    def fields(self, owner, size, stack=()):
        children = self.children(owner)
        _require(len(children) <= MAX_FIELDS, 'Cover-call local/member bound exceeded')
        fields = [self.descriptor(item, owner, size, stack) for item in children]
        for i, field in enumerate(fields):
            for other in fields[:i]:
                overlap = field['offset'] < other['offset'] + other['storage_width'] and other['offset'] < field['offset'] + field['storage_width']
                packed_bool = (field['type'] == other['type'] == 'BoolProperty' and field['offset'] == other['offset']
                               and field['width'] == other['width'] == 4 and field['dimension'] == other['dimension'] == 1)
                _require(not overlap or packed_bool, 'Cover-call local/member storage overlaps')
        return fields

    def function(self, owner, name, key):
        item = self.child(owner, name, 'Function')
        address = item['address']
        header = self.stable(address + 0x8c, 16)
        pointer, count, capacity = struct.unpack('<Qii', header)
        _require(pointer and 0 < count <= capacity <= MAX_SCRIPT, 'Invalid cover-call script range')
        entry = self.number(address + 0x114)
        _require(entry == self.r.base + 0x1d6ca0, 'Cover-call function is not the expected script VM entry')
        size = self.number(address + 0x88, '<I')
        _require(0 < size <= 65536, 'Cover-call function locals size is invalid')
        raw = self.stable(pointer, count)
        fields = self.fields(address, size)
        if name == 'PlayerMove':
            by_name = {x['name']: x for x in fields}
            for label, (kind, width) in CALLER_LOCALS.items():
                value = by_name.get(label)
                _require(value is not None and value['type'] == kind and value['width'] == width
                         and value['dimension'] == 1, 'Required PlayerMove local differs: ' + label)
        return dict(key=key, owner=self.add(owner), identity=self.by_address[address], entry=entry,
                    locals_size=size, fields=fields, flags_classification_verified=False,
                    script=dict(address=pointer, count=count, capacity=capacity, header_raw=header.hex(),
                                bytes=raw.hex(), sha256=hashlib.sha256(raw).hexdigest()))

    def finish(self):
        cursor = 0
        while cursor < len(self.ids):
            item = self.ids[cursor]
            self.add(item['cls'])
            if self.fresh(item['cls'])['name'] == 'Class':
                self.ancestors(item['address'])
            cursor += 1
        for item in self.ids:
            _require(self.fresh(item['address']) == item, 'Cover-call selected identity changed at closing boundary')
        for (address, size), expected in self.checks.items():
            if size == 16 and address in self.header_addresses.values():
                continue
            if self.r.read(address, size) != expected:
                raise CoverCallConfigRejected('Cover-call source changed before completion')
        _require(self.header_addresses == dict(objects=self.r.objects_header, names=self.r.names_header),
                 'Cover-call Resolver registry header addresses changed')
        for name, address in self.header_addresses.items():
            raw = self.r.read(address, 16)
            self.registry_observation(address, raw)
            self.closing_headers[name] = raw.hex()


def prepare(sampler):
    """Return (JSON-safe config, runtime provenance); never writes or invokes game code."""
    started = time.perf_counter_ns()
    try:
        r, p = sampler.r, sampler.r.profile
        camera = sampler.resolved
        _require(p.get('sha256') == camera.get('sha256') == GAME_SHA and camera.get('pid') == r.pid
                 and int(camera['module_base'], 0) == r.base, 'Cover-call current process/build mismatch')
        _require(all(p.get(key) == value for key, value in LAYOUT.items()),
                 'Unsupported cover-call reflected layout')
        before = sampler.guard()
        _require(before.get('view_target') == sampler.pawn, 'Cover-call requires the controlled Pawn view target')
        cap = _Capture(sampler)
        roles = {name: cap.add(getattr(sampler, attribute)) for name, attribute in
                 (('controller', 'controller'), ('camera', 'camera'), ('pawn', 'pawn'), ('local_player', 'local'))}
        for index in roles.values():
            _require(not cap.ids[index]['name'].startswith('Default__'), 'Default cover-call controlled role')
        pc, pawn, cam = sampler.controller, sampler.pawn, sampler.camera
        controller_chain = cap.ancestors(cap.fresh(pc)['cls'])
        matches = [x for x in controller_chain if x['name'] == 'WarPC']
        _require(len(matches) == 1, 'Current controller does not have one exact WarPC ancestor')
        war_pc = matches[0]['address']
        roles['war_pc'] = cap.add(war_pc)
        for address, expected in ((pawn, 'WarPawn'), (cam, 'WarPlayerCamera'), (sampler.local, 'WarLocalPlayer')):
            _require(expected in {x['name'] for x in cap.ancestors(cap.fresh(address)['cls'])}, 'Cover-call role class differs')
        for source, name, target in ((pc, 'Pawn', pawn), (pawn, 'Controller', pc), (pc, 'PlayerCamera', cam),
                                     (cam, 'PCOwner', pc), (pc, 'Player', sampler.local)):
            cap.link(source, name, target)
        boundary = _boundary_certificate()
        _require(cap.stable(r.base + boundary['rva'], 32) == boundary['bytes'], 'Current UI call-boundary entry bytes differ')
        states, functions, chains = [], [], []
        for name in STATES:
            state = cap.child(war_pc, name, 'State')
            state_id = cap.by_address[state['address']]
            states.append(dict(name=name, identity=state_id, owner=roles['war_pc']))
            for function_name in ('PlayerMove', 'UpdatePlayerPosture'):
                functions.append(cap.function(state['address'], function_name, name + '.' + function_name))
            chains.append(dict(state=state_id, caller=name + '.PlayerMove', updater=name + '.UpdatePlayerPosture',
                               setters=list(SETTERS), callsite_aliasing_verified=False))
        for name in SETTERS:
            functions.append(cap.function(war_pc, name, name))
        after = sampler.guard()
        _require(all(before.get(key) == after.get(key) for key in
                     ('pawn', 'controller', 'camera', 'local_player', 'level', 'view_target')),
                 'Cover-call controlled context changed before completion')
        _require(dict(r.profile) == cap.p, 'Cover-call Resolver profile changed')
        cap.finish()
        _require(_boundary_certificate() == boundary, 'Existing UI boundary source certificate changed')
        config = dict(schema=1, pid=r.pid, base=r.base, sha256=GAME_SHA,
                      mutation_authorized=False, callsite_aliasing_verified=False, runtime_observation=False,
                      objects_header=r.objects_header, names_header=r.names_header,
                      registry_snapshots=dict(objects=cap.object_header.hex(), names=cap.name_header.hex()),
                      registry_closing_snapshots=cap.closing_headers,
                      registry_growth_policy=dict(header_size=16, headers=cap.header_addresses,
                                                  same_data_pointer=True, same_capacity=True, count_nondecreasing=True,
                                                  selection_uses_initial_counts=True, selected_sources_exact=True,
                                                  cohort_atomic=False),
                      layout=LAYOUT.copy(),
                      frame=FRAME.copy(), function_layout=FUNCTION.copy(), identities=cap.ids, roles=roles,
                      supers=[dict(identity=cap.by_address[a], parent=b) for a, b in sorted(cap.supers.items())],
                      states=states, functions=functions, structures=list(cap.structures.values()), candidate_chains=chains,
                      dispatcher=dict(rva=boundary['rva'], entry=boundary['bytes'].hex(),
                                      header_source_sha256=boundary['source_sha256'], compared_in_current_process=True),
                      source_checks=[dict(address=a, size=n, bytes=value.hex()) for (a, n), value in cap.checks.items()])
        provenance = dict(accepted=True, read_only=True, mutation_authorized=False, callsite_aliasing_verified=False,
                          native_runtime_observation=False, pid=r.pid, sha256=GAME_SHA, module_base=hex(r.base),
                          executable=camera.get('executable', getattr(r, 'path', None)),
                          config_sha256=hashlib.sha256(json.dumps(config, sort_keys=True, separators=(',', ':')).encode('utf-8')).hexdigest(),
                          started_perf_counter_ns=started, finished_perf_counter_ns=time.perf_counter_ns(),
                          camera_stamp_before=before.get('timestamp_raw'), camera_stamp_after=after.get('timestamp_raw'),
                          flags_interpretation='raw bytes only; parameter/out/local flag semantics are not certified',
                          limits='Fresh reflection/source snapshot only; monotonic same-allocation registry count growth does not make the selected cohort atomic. No FFrame observations, code PCs, callsite aliasing, setter enum semantics, hooks or writes are certified.')
        return copy.deepcopy(config), provenance
    except CoverCallConfigRejected:
        raise
    except (AttributeError, KeyError, TypeError, ValueError, RuntimeError, OSError, struct.error, UnicodeError) as error:
        raise CoverCallConfigRejected('Fresh cover-call config rejected: ' + str(error)) from error
