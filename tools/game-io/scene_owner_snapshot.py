"""Fresh bounded CPU owner-graph evidence; no rendering admission or GPU proof.

Resolver.property supplies addresses as discovery hints only. Fresh relevant
reads and actual class/field membership independently validate every selection.
Resolver discovery caches are not accepted evidence. The caller must supply an
already prepared, executable-build-verified Resolver; this helper does not hash
the EXE. Consistent rereads do not establish an atomic/ABA snapshot.
"""
import hashlib
import struct

GAME_SHA = 'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4'
LAYOUT = dict(object_internal_index=8, object_outer=64, object_name=72, object_class=80,
              field_super=96, field_next=104, struct_children=128,
              property_offset_internal=148, struct_property_struct=208,
              name_entry_index=0, name_entry_text=20)
MAX_READS = 32768
MAX_BYTES = 2 * 1024 * 1024
MAX_IDENTITIES = 4096
MAX_FIELDS = 4096
MAX_CLASS_FIELDS = 2048
MAX_CHAIN = 64
MAX_ARRAY_CAPACITY = 4096
MAX_REREAD_SPAN = 1024  # Largest admitted ActiveScenes member array (128 * 8).


class SceneOwnerRejected(RuntimeError):
    pass


def read_conflict_error(error_type, message, stage, address, expected, observed, headers):
    """Attach bounded evidence from the failing read; never read remote memory."""
    error = error_type(message)
    offset = next((i for i, (a, b) in enumerate(zip(expected, observed)) if a != b),
                  min(len(expected), len(observed)))
    error.read_conflict = dict(stage=stage, address=hex(address), size=len(expected),
        actual_size=len(observed), changed_offset=offset, changed_address=hex(address+offset),
        expected_window=expected[offset:offset+8].hex(), actual_window=observed[offset:offset+8].hex())
    # A 16-byte UI array or an enclosing merged span is not a registry header.
    registry = next((name for name, pointer in headers.items() if pointer == address), None)
    if registry is not None and len(expected) == len(observed) == 16:
        def decoded(raw):
            data, count, capacity = struct.unpack('<Qii', raw)
            return dict(data=hex(data), num=count, max=capacity)
        error.read_conflict.update(registry=registry, initial_raw=expected.hex(), observed_raw=observed.hex(),
                                   initial=decoded(expected), observed=decoded(observed))
    return error


def _reread_spans(reads):
    """Join only observed overlapping/touching bytes, without reading gaps.

    Keep the acquisition ledger/digest intact. Small adjacent name units and
    object-registry slots need one final read rather than one syscall per unit.
    Conflicting observations of an overlapping byte invalidate the snapshot.
    """
    start, pending = None, bytearray()
    for (address, size), raw in sorted(reads.items()):
        if len(raw) != size or not 0 < size <= MAX_REREAD_SPAN:
            raise SceneOwnerRejected('Invalid owner verification span')
        end = start + len(pending) if start is not None else 0
        if start is not None and address <= end:
            overlap = min(size, end - address)
            if pending[address-start:address-start+overlap] != raw[:overlap]:
                raise read_conflict_error(SceneOwnerRejected, 'Owner overlapping source changed during acquisition',
                    'owner_overlapping_reads', address, bytes(pending[address-start:address-start+overlap]), raw[:overlap], {})
            if address + size - start <= MAX_REREAD_SPAN:
                pending.extend(raw[overlap:])
                continue
        if start is not None:
            yield start, bytes(pending)
        start, pending = address, bytearray(raw)
    if start is not None:
        yield start, bytes(pending)


class _Capture:
    def __init__(self, resolver):
        self.r = resolver
        self.source_read = resolver.read
        p = resolver.profile
        if type(p) is not dict:
            raise SceneOwnerRejected('Unsupported scene owner profile')
        try:
            anchors = (int(p.get('names_rva', '0'), 0), int(p.get('objects_rva', '0'), 0))
        except (ValueError, TypeError) as error:
            raise SceneOwnerRejected('Invalid scene owner profile anchors') from error
        if (p.get('sha256') != GAME_SHA or any(type(p.get(key)) is not int or p.get(key) != value for key, value in LAYOUT.items()) or
                anchors != (0x1c34800, 0x1c24730)):
            raise SceneOwnerRejected('Unsupported scene owner profile')
        if type(resolver.base) is not int or not 0 < resolver.base < 1 << 64 or type(resolver.pid) is not int or resolver.pid <= 0:
            raise SceneOwnerRejected('Invalid scene owner base/PID')
        self.base, self.pid = resolver.base, resolver.pid
        self.reads, self.identities, self.names = {}, {}, {}
        self.name_entries = {}
        self.chains, self.classes, self.fields, self.properties = {}, {}, {}, {}
        self.calls = self.byte_count = self.field_count = 0
        self.hint_calls = self.hint_bytes = 0
        self.headers = {name: resolver.base + offset for name, offset in
                        (('objects', 0x1c24730), ('names', 0x1c34800))}
        self.registries = {}
        for name, address in self.headers.items():
            header = self.raw(address, 16)
            data, count, capacity = struct.unpack('<Qii', header)
            if not data or not 0 < count <= capacity < 4000000 or data > (1 << 64) - capacity * 8:
                raise SceneOwnerRejected('Invalid owner registry header: ' + name)
            self.registries[name] = (data, count, capacity)
        if self.name(0) != 'None':
            raise SceneOwnerRejected('Invalid owner name registry zero entry')

    def _read(self, address, size):
        if (type(address) is not int or type(size) is not int or size <= 0 or
                not 0 < address <= (1 << 64) - size):
            raise SceneOwnerRejected('Invalid owner read address/size')
        self.calls += 1
        self.byte_count += size
        if self.calls > MAX_READS or self.byte_count > MAX_BYTES:
            raise SceneOwnerRejected('Scene owner read budget exceeded')
        try:
            value = self.source_read(address, size)
        except SceneOwnerRejected:
            raise
        except (OSError, RuntimeError) as error:
            raise SceneOwnerRejected('Owner source read failed') from error
        if not isinstance(value, bytes) or len(value) != size:
            raise SceneOwnerRejected('Short owner read')
        return value

    def raw(self, address, size):
        value = self._read(address, size)
        key = address, size
        if key in self.reads and self.reads[key] != value:
            raise read_conflict_error(SceneOwnerRejected, 'Owner source changed during acquisition',
                'owner_repeated_read', address, self.reads[key], value, self.headers)
        self.reads[key] = value
        return value

    def pointer(self, address):
        return struct.unpack('<Q', self.raw(address, 8))[0]

    def name(self, index):
        if index in self.names:
            return self.names[index]
        data, count, _ = self.registries['names']
        if not 0 <= index < count:
            raise SceneOwnerRejected('Owner name index outside registry')
        entry = self.pointer(data + index * 8)
        if struct.unpack('<I', self.raw(entry, 4))[0] != index:
            raise SceneOwnerRejected('Owner name entry index mismatch')
        text = bytearray()
        for offset in range(0, 128, 2):
            unit = self.raw(entry + 20 + offset, 2)
            if unit == b'\0\0':
                try:
                    value = text.decode('utf-16-le', errors='strict')
                except UnicodeError as error:
                    raise SceneOwnerRejected('Invalid owner name text') from error
                if not value or any(char in value for char in '\r\n\0'):
                    raise SceneOwnerRejected('Invalid owner name text')
                self.names[index] = value
                self.name_entries[index] = dict(index=index, address=entry, text=value)
                return value
            text.extend(unit)
        raise SceneOwnerRejected('Owner name exceeds bounded UTF16 read')

    def identity(self, address):
        if type(address) is not int or not 0 < address <= (1 << 64) - 96:
            raise SceneOwnerRejected('Invalid owner object address')
        if address in self.identities:
            return self.identities[address]
        if len(self.identities) >= MAX_IDENTITIES:
            raise SceneOwnerRejected('Owner identity limit exceeded')
        index = struct.unpack('<I', self.raw(address + 8, 4))[0]
        data, count, _ = self.registries['objects']
        if not 0 <= index < count or self.pointer(data + index * 8) != address:
            raise SceneOwnerRejected('Owner object registry slot mismatch')
        name_index, number = struct.unpack('<II', self.raw(address + 72, 8))
        value = dict(address=address, internal_index=index, name_index=name_index,
                     name=self.name(name_index), number=number, cls=self.pointer(address + 80),
                     outer=self.pointer(address + 64))
        if not value['cls'] or value['name'].startswith('Default__'):
            raise SceneOwnerRejected('Default/invalid owner identity')
        self.identities[address] = value
        return value

    def class_identity(self, address):
        value = self.identity(address)
        meta = self.identity(value['cls'])
        if value['number'] or meta['name'] != 'Class' or meta['number'] or meta['cls'] != meta['address']:
            raise SceneOwnerRejected('Owner class metadata mismatch')
        return value

    def chain(self, address):
        if address in self.chains:
            return self.chains[address]
        result = []
        current = address
        while current:
            if current in result or len(result) >= MAX_CHAIN:
                raise SceneOwnerRejected('Invalid owner superclass chain')
            value = self.class_identity(current)
            parent = self.pointer(current + 96)
            self.classes[current] = dict(identity=value, super=parent)
            result.append(current)
            current = parent
        self.chains[address] = result
        return result

    def derives(self, value, name):
        if not any(self.identities[address]['name'] == name for address in self.chain(value['cls'])):
            raise SceneOwnerRejected('Owner class mismatch: ' + name)

    def class_fields(self, owner):
        if owner in self.fields:
            return self.fields[owner]
        current = self.pointer(owner + 128)
        result, seen = [], set()
        while current:
            if current in seen or len(seen) >= MAX_CLASS_FIELDS or self.field_count >= MAX_FIELDS:
                raise SceneOwnerRejected('Invalid owner field chain/limit')
            seen.add(current)
            # Unrelated fields need no name text or class decoding. Keep the
            # actual linked headers and live registry slots in the final ledger.
            if type(current) is not int or not 0 < current <= (1 << 64) - 112:
                raise SceneOwnerRejected('Invalid owner field address')
            index = struct.unpack('<I', self.raw(current + 8, 4))[0]
            data, count, _ = self.registries['objects']
            if not 0 <= index < count or self.pointer(data + index * 8) != current:
                raise SceneOwnerRejected('Owner field registry slot mismatch')
            name_index, number = struct.unpack('<II', self.raw(current + 72, 8))
            outer = self.pointer(current + 64)
            following = self.pointer(current + 104)
            if outer != owner or not 0 <= name_index < self.registries['names'][1]:
                raise SceneOwnerRejected('Owner reflected field outer/name mismatch')
            result.append(dict(address=current, internal_index=index, name_index=name_index,
                               number=number, outer=outer, next=following))
            self.field_count += 1
            current = following
        self.fields[owner] = result
        return result

    def property(self, obj, name, kind, declaring_owner):
        chain = self.chain(obj['cls'])
        # Cached reflection is permitted only as an address hint. In particular,
        # cached offset/type/owner values never enter the accepted result.
        original_read = self.r.read
        calls, byte_count = self.calls, self.byte_count
        try:
            # Account discovery reads against the same budget, without treating
            # cached-discovery bytes as accepted proof. Restore nested readers.
            self.r.read = self._read
            try:
                hint = self.r.property(obj['cls'], name, True)
            finally:
                self.r.read = original_read
                self.hint_calls += self.calls - calls
                self.hint_bytes += self.byte_count - byte_count
            needle = self.identity(hint['address'])
        except (KeyError, TypeError, ValueError, OSError, RuntimeError) as error:
            raise SceneOwnerRejected('Invalid owner property discovery hint: ' + name) from error
        if needle['name'] != name or needle['number']:
            raise SceneOwnerRejected('Owner property hint name mismatch: ' + name)
        field = None
        for owner in chain:
            # FName includes its numeric suffix. Unrelated numbered metadata is
            # legal; it cannot shadow the exact unnumbered property we require.
            matches = [value for value in self.class_fields(owner)
                       if (value['name_index'], value['number']) == (needle['name_index'], needle['number'])]
            if len(matches) > 1:
                raise SceneOwnerRejected('Duplicate owner reflected field: ' + name)
            if matches:
                candidate = self.identity(matches[0]['address'])
                candidate_kind = self.class_identity(candidate['cls'])
                if not candidate_kind['name'].endswith('Property'):
                    continue
                if candidate['address'] != needle['address']:
                    raise SceneOwnerRejected('Owner property hint shadowed by actual field: ' + name)
                field = candidate
                break
        if field is None:
            raise SceneOwnerRejected('Owner property hint is not reachable: ' + name)
        owner = self.class_identity(field['outer'])
        field_kind = self.class_identity(field['cls'])
        if field_kind['name'] != kind or owner['name'] != declaring_owner or owner['address'] not in chain:
            raise SceneOwnerRejected('Owner property type/defining owner mismatch: ' + name)
        dimension, element_size = struct.unpack('<II', self.raw(field['address'] + 112, 8))
        expected_size = 16 if kind == 'ArrayProperty' else 8
        offset = struct.unpack('<I', self.raw(field['address'] + 148, 4))[0]
        if (dimension, element_size) != (1, expected_size) or offset > 0x10000 - expected_size:
            raise SceneOwnerRejected('Owner property layout mismatch: ' + name)
        result = dict(identity=field, owner=owner, type=field_kind, offset=offset,
                      array_dim=dimension, element_size=element_size)
        if kind == 'ArrayProperty':
            inner = self.identity(self.pointer(field['address'] + 208))
            inner_kind = self.class_identity(inner['cls'])
            if inner['outer'] != field['address'] or inner['number'] or inner_kind['name'] != 'ObjectProperty':
                raise SceneOwnerRejected('Owner array inner outer/type mismatch: ' + name)
            if struct.unpack('<II', self.raw(inner['address'] + 112, 8)) != (1, 8):
                raise SceneOwnerRejected('Owner array inner layout mismatch: ' + name)
            result['inner'] = dict(identity=inner, type=inner_kind)
        self.properties[field['address']] = result
        return result

    def object_field(self, obj, name, owner):
        prop = self.property(obj, name, 'ObjectProperty', owner)
        return self.pointer(obj['address'] + prop['offset'])

    def array(self, obj, name, owner, maximum):
        prop = self.property(obj, name, 'ArrayProperty', owner)
        data, count, capacity = struct.unpack('<Qii', self.raw(obj['address'] + prop['offset'], 16))
        # UE3 reserves allocator slack (the live one-player array has capacity
        # 17). Only occupied members consume the logical/read budget.
        if (not 0 <= count <= maximum or not count <= capacity <= MAX_ARRAY_CAPACITY or (count and not data) or
                data > (1 << 64) - capacity * 8):
            raise SceneOwnerRejected('Invalid owner array: ' + name)
        values = list(struct.unpack('<' + 'Q' * count, self.raw(data, count * 8))) if count else []
        if len(set(values)) != len(values) or any(not value for value in values):
            raise SceneOwnerRejected('Duplicate/null owner array member: ' + name)
        for value in values:
            self.identity(value)
        return dict(property=prop['identity']['address'], data=data, count=count, capacity=capacity, members=values)

    def finish(self):
        # Close the class ancestry of every visited identity, including field
        # kinds/metaclasses, without consulting Resolver's identity cache.
        visited = set()
        while len(visited) < len(self.identities):
            for address in list(self.identities):
                if address not in visited:
                    self.chain(self.identities[address]['cls'])
                    visited.add(address)
        if self.r.base != self.base or self.r.pid != self.pid:
            raise SceneOwnerRejected('Owner resolver base/PID changed')
        self.final_read_spans = 0
        for address, expected in _reread_spans(self.reads):
            self.final_read_spans += 1
            observed = self._read(address, len(expected))
            if observed != expected:
                raise read_conflict_error(SceneOwnerRejected, 'Owner source changed at final boundary',
                    'owner_final_reread', address, expected, observed, self.headers)
        # Registry headers close after all names, fields, identity and link reads.
        for address in self.headers.values():
            observed = self._read(address, 16)
            if observed != self.reads[address, 16]:
                raise read_conflict_error(SceneOwnerRejected, 'Owner registry changed at final boundary',
                    'owner_registry_bracket', address, self.reads[address, 16], observed, self.headers)


def capture_scene_owner(resolver, client_address, scene_address, expected_controller, expected_local_player):
    """Return fresh consistent CPU graph evidence; failure yields no partial proof."""
    capture = _Capture(resolver)
    c = capture
    client, scene, pc, local = [c.identity(address) for address in
                              (client_address, scene_address, expected_controller, expected_local_player)]
    if len({client_address, scene_address, expected_controller, expected_local_player}) != 4:
        raise SceneOwnerRejected('Aliased scene owner roles')
    if c.class_identity(client['cls'])['name'] != 'GameUISceneClient':
        raise SceneOwnerRejected('Client is not exact GameUISceneClient')
    c.derives(client, 'GameUISceneClient')
    c.derives(scene, 'UIScene_PathChoice')
    c.derives(scene, 'UIScene')
    c.derives(pc, 'PlayerController')
    c.derives(local, 'LocalPlayer')
    interaction = c.identity(client['outer'])
    c.derives(interaction, 'UIInteraction')
    viewport = c.identity(interaction['outer'])
    c.derives(viewport, 'GameViewportClient')
    engine = c.identity(viewport['outer'])
    c.derives(engine, 'Engine')
    if len({value['address'] for value in (client, scene, pc, local, interaction, viewport, engine)}) != 7:
        raise SceneOwnerRejected('Aliased scene owner graph roles')
    if local['outer'] != engine['address']:
        raise SceneOwnerRejected('Local player engine owner mismatch')
    links = {}
    def link(label, obj, name, owner, expected):
        value = c.object_field(obj, name, owner)
        if value != expected:
            raise SceneOwnerRejected('Owner link mismatch: ' + label)
        links[label] = dict(object=obj['address'], property=name, value=value)
    link('interaction_scene_client', interaction, 'SceneClient', 'UIInteraction', client_address)
    link('viewport_ui_controller', viewport, 'UIController', 'GameViewportClient', interaction['address'])
    link('local_viewport', local, 'ViewportClient', 'LocalPlayer', viewport['address'])
    link('local_actor', local, 'Actor', 'Player', expected_controller)
    link('controller_player', pc, 'Player', 'PlayerController', expected_local_player)
    link('scene_client', scene, 'SceneClient', 'UIScene', client_address)
    players = c.array(engine, 'GamePlayers', 'Engine', 16)
    if players['members'] != [expected_local_player]:
        raise SceneOwnerRejected('Expected exactly one matching engine GamePlayer')
    active = c.array(client, 'ActiveScenes', 'GameUISceneClient', 128)
    if active['members'].count(scene_address) != 1:
        raise SceneOwnerRejected('Scene is not a unique active member')
    owner = c.object_field(scene, 'PlayerOwner', 'UIScene')
    if owner == expected_local_player:
        classification = 'local_player'
    elif owner == 0:
        classification = 'unowned_single_local_player'
    else:
        raise SceneOwnerRejected('Scene belongs to another player')
    links['scene_player_owner'] = dict(object=scene_address, property='PlayerOwner', value=owner)
    c.finish()
    digest = hashlib.sha256()
    for (address, size), raw in sorted(c.reads.items()):
        digest.update(struct.pack('<QQ', address, size))
        digest.update(raw)
    return dict(schema=1, kind='scene_owner_cpu_snapshot', pid=c.pid, module_base=c.base,
                profile_game_sha256=GAME_SHA, owner_classification=classification,
                classification_basis='explicit_local_player' if owner else 'null_owner_with_exact_single_player_graph',
                roles=dict(client=client, scene=scene, controller=pc, local_player=local,
                           interaction=interaction, viewport=viewport, engine=engine),
                identities=list(c.identities.values()), classes=list(c.classes.values()),
                name_entries=list(c.name_entries.values()),
                fields=[dict(owner=address, headers=fields) for address, fields in c.fields.items()],
                properties=list(c.properties.values()), links=links,
                arrays=dict(game_players=players, active_scenes=active),
                evidence=dict(method='fresh_relevant_fields_and_registry_slots_with_final_reread',
                              unique_reads=len(c.reads), total_read_calls=c.calls, total_read_bytes=c.byte_count,
                              max_read_calls=MAX_READS, max_read_bytes=MAX_BYTES,
                              discovery_hint_read_calls=c.hint_calls, discovery_hint_read_bytes=c.hint_bytes,
                              final_reread_spans=c.final_read_spans, max_reread_span=MAX_REREAD_SPAN,
                              source_bytes_sha256=digest.hexdigest(), registry_headers={
                                  name: dict(address=c.headers[name], data=value[0], count=value[1], capacity=value[2])
                                  for name, value in c.registries.items()}),
                consistent_reread=True, read_only=True, atomic_snapshot=False,
                allocation_identity_proven=False, render_admission=False, gpu_verified=False)
