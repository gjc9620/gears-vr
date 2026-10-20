"""Serialize an authenticated CPU owner snapshot into a private bounded trial INI.

This is neither runtime admission nor a rendering/GPU proof. No process is read,
no file is written, and unknown/missing evidence is rejected rather than inferred.
"""
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'game-io'))
from scene_owner_snapshot import GAME_SHA, LAYOUT

U64 = (1 << 64) - 1
MAX_CHECKS, MAX_BYTES, MAX_SPAN = 2048, 32768, 256
PROPERTY_LAYOUT = 112  # ArrayDim/ElementSize, the layout authenticated by capture_scene_owner.
IDENTITY_KEYS = ('address', 'internal_index', 'name_index', 'name', 'number', 'cls', 'outer')
LINKS = {
    'interaction_scene_client':('interaction', 'SceneClient', 'UIInteraction', 'client'),
    'viewport_ui_controller':('viewport', 'UIController', 'GameViewportClient', 'interaction'),
    'local_viewport':('local_player', 'ViewportClient', 'LocalPlayer', 'viewport'),
    'local_actor':('local_player', 'Actor', 'Player', 'controller'),
    'controller_player':('controller', 'Player', 'PlayerController', 'local_player'),
    'scene_client':('scene', 'SceneClient', 'UIScene', 'client'),
    'scene_player_owner':('scene', 'PlayerOwner', 'UIScene', None),
}
ROLE_CLASSES = {'client':('GameUISceneClient',), 'scene':('UIScene_PathChoice', 'UIScene'),
    'controller':('PlayerController',), 'local_player':('LocalPlayer', 'Player'),
    'interaction':('UIInteraction',), 'viewport':('GameViewportClient',), 'engine':('Engine',)}


def _require(condition, message):
    if not condition:
        raise ValueError(message)


def _int(value, minimum=0, maximum=U64):
    _require(type(value) is int and minimum <= value <= maximum, 'invalid integer/bounds')
    return value


def _list(value, maximum):
    _require(type(value) is list and len(value) <= maximum, 'invalid evidence collection/budget')
    return value


class _Checks:
    def __init__(self):
        self.spans, self.cells, self.size = {}, {}, 0

    def add(self, address, value):
        _int(address, 1)
        _require(type(value) is bytes and value and len(value) <= U64 - address, 'invalid check span/overflow')
        for offset in range(0, len(value), MAX_SPAN):
            at, raw = address + offset, value[offset:offset + MAX_SPAN]
            key = at, len(raw)
            if key in self.spans:
                _require(self.spans[key] == raw, 'conflicting duplicate check')
                continue
            for index, byte in enumerate(raw):
                _require(at + index not in self.cells or self.cells[at + index] == byte, 'conflicting overlapping check')
            _require(len(self.spans) < MAX_CHECKS and self.size + len(raw) <= MAX_BYTES, 'check budget exceeded')
            self.spans[key] = raw
            self.cells.update((at + index, byte) for index, byte in enumerate(raw))
            self.size += len(raw)

    def number(self, address, value, fmt='<Q'):
        self.add(address, struct.pack(fmt, value))


def build(snapshot, *, pid, creation_time, started_qpc, world_generation, nonce,
          shift_x=0, shift_y=0, duration_ms=3000):
    """Return ASCII INI for a bounded private candidate; caller owns persistence."""
    try:
        return _build(snapshot, pid=pid, creation_time=creation_time, started_qpc=started_qpc,
            world_generation=world_generation, nonce=nonce, shift_x=shift_x, shift_y=shift_y, duration_ms=duration_ms)
    except (KeyError, TypeError, AttributeError, UnicodeError, struct.error, OverflowError) as error:
        raise ValueError('malformed snapshot evidence') from error


def _build(snapshot, *, pid, creation_time, started_qpc, world_generation, nonce, shift_x, shift_y, duration_ms):
    _int(pid, 1, (1 << 32) - 1)
    for value in (creation_time, started_qpc, world_generation, nonce):
        _int(value, 1)
    _int(shift_x, -512, 512); _int(shift_y, -512, 512); _int(duration_ms, 100, 8000)
    _require(type(snapshot) is dict and type(snapshot.get('schema')) is int and snapshot['schema'] == 1 and
             snapshot.get('kind') == 'scene_owner_cpu_snapshot' and snapshot.get('read_only') is True and
             snapshot.get('consistent_reread') is True and snapshot.get('profile_game_sha256') == GAME_SHA,
             'unsupported or unverified scene-owner snapshot')
    _require(snapshot['pid'] == pid and type(snapshot['pid']) is int, 'snapshot PID mismatch')
    base = _int(snapshot['module_base'], 1)
    evidence = snapshot['evidence']
    _require(evidence['method'] == 'fresh_relevant_fields_and_registry_slots_with_final_reread', 'unsupported proof method')
    headers = evidence['registry_headers']
    registries, checks = {}, _Checks()
    for name, rva in (('objects', 0x1c24730), ('names', 0x1c34800)):
        header = headers[name]
        _require(header['address'] == base + rva, 'registry anchor mismatch')
        data, count, capacity = _int(header['data'], 1), _int(header['count'], 1, 3999999), _int(header['capacity'], 1, 3999999)
        _require(count <= capacity and data <= U64 - capacity * 8, 'registry capacity/overflow')
        checks.add(header['address'], struct.pack('<Qii', data, count, capacity))
        registries[name] = data, count
    _require(registries['objects'][0] != registries['names'][0], 'aliased registries')

    identities = {}
    indexes = set()
    for identity in _list(snapshot['identities'], 4096):
        _require(type(identity) is dict and all(key in identity for key in IDENTITY_KEYS), 'incomplete identity')
        address = _int(identity['address'], 1)
        index = _int(identity['internal_index'], 0, registries['objects'][1] - 1)
        _int(identity['name_index'], 0, registries['names'][1] - 1)
        _int(identity['number'], 0, (1 << 32) - 1)
        _int(identity['cls'], 1); _int(identity['outer'])
        text = identity['name']
        _require(type(text) is str and text and '\0' not in text and not text.startswith('Default__') and
                 len(text.encode('utf-16-le')) <= 126, 'invalid identity name')
        _require(address not in identities and index not in indexes, 'duplicate identity/index')
        identities[address] = identity
        indexes.add(index)
        checks.number(address + LAYOUT['object_internal_index'], index, '<I')
        checks.number(address + LAYOUT['object_outer'], identity['outer'])
        checks.add(address + LAYOUT['object_name'], struct.pack('<II', identity['name_index'], identity['number']))
        checks.number(address + LAYOUT['object_class'], identity['cls'])
        checks.number(registries['objects'][0] + index * 8, address)
    _require(identities, 'missing identities')

    def canonical(identity):
        _require(type(identity) is dict and identity.get('address') in identities and
                 identity == identities[identity['address']], 'identity association mismatch')
        return identity

    names = {}
    for entry in _list(snapshot['name_entries'], 4096):
        index = _int(entry['index'], 0, registries['names'][1] - 1)
        address, text = _int(entry['address'], 1), entry['text']
        _require(index not in names and type(text) is str and text and '\0' not in text and
                 len(text.encode('utf-16-le')) <= 126, 'invalid/duplicate name entry')
        names[index] = address, text
    needed = {0:'None'}
    for identity in identities.values():
        index = identity['name_index']
        _require(index not in needed or needed[index] == identity['name'], 'conflicting name identity')
        needed[index] = identity['name']
    for index, text in needed.items():
        _require(index in names and names[index][1] == text, 'missing/mismatched authenticated name entry')
        address = names[index][0]
        checks.number(registries['names'][0] + index * 8, address)
        checks.number(address + LAYOUT['name_entry_index'], index, '<I')
        checks.add(address + LAYOUT['name_entry_text'], text.encode('utf-16-le') + b'\0\0')

    classes = {}
    for row in _list(snapshot['classes'], 4096):
        identity = canonical(row['identity'])
        address = identity['address']
        _require(address not in classes and identity['number'] == 0, 'duplicate/numbered class')
        parent = _int(row['super'])
        classes[address] = parent
        checks.number(address + LAYOUT['field_super'], parent)
    meta = set()
    for address in classes:
        identity = identities[address]
        kind = identities.get(identity['cls'])
        _require(kind is not None and kind['name'] == 'Class' and kind['number'] == 0 and
                 kind['cls'] == kind['address'] and kind['address'] in classes, 'invalid class metaclass')
        meta.add(kind['address'])
    _require(len(meta) == 1, 'conflicting metaclasses')

    def chain(address):
        out = []
        while address:
            _require(address in classes and address not in out and len(out) < 64, 'unknown/cyclic class ancestry')
            out.append(address)
            address = classes[address]
        return out

    for identity in identities.values():
        chain(identity['cls'])
    for address in classes:
        chain(address)
    roles = snapshot['roles']
    _require(type(roles) is dict and set(roles) == set(ROLE_CLASSES), 'invalid roles')
    for role, required in ROLE_CLASSES.items():
        identity = canonical(roles[role])
        ancestors = [identities[address]['name'] for address in chain(identity['cls'])]
        _require(all(name in ancestors for name in required), 'role class mismatch')
    _require(identities[roles['client']['cls']]['name'] == 'GameUISceneClient', 'client must be exact class')
    _require(len({value['address'] for value in roles.values()}) == 7, 'aliased roles')
    for role, parent in (('client', 'interaction'), ('interaction', 'viewport'), ('viewport', 'engine'), ('local_player', 'engine')):
        _require(roles[role]['outer'] == roles[parent]['address'], 'role outer graph mismatch')

    properties = _list(snapshot['properties'], 64)
    _require(len(properties) == 9, 'missing/unexpected property evidence')
    selected, property_addresses = {}, set()
    for prop in properties:
        field, owner, kind = (canonical(prop[key]) for key in ('identity', 'owner', 'type'))
        _require(field['address'] not in property_addresses and field['number'] == 0 and
                 field['outer'] == owner['address'] and field['cls'] == kind['address'] and owner['address'] in classes and
                 kind['address'] in classes, 'property identity/owner/type mismatch')
        property_addresses.add(field['address'])
        key = owner['name'], field['name']
        _require(key not in selected, 'ambiguous defining property owner')
        selected[key] = prop
        expected_size = 16 if kind['name'] == 'ArrayProperty' else 8
        _require(kind['name'] in ('ObjectProperty', 'ArrayProperty') and prop['array_dim'] == 1 and
                 type(prop['array_dim']) is int and prop['element_size'] == expected_size and
                 type(prop['element_size']) is int, 'property layout/type mismatch')
        offset = _int(prop['offset'], 0, 0x10000 - expected_size)
        checks.add(field['address'] + PROPERTY_LAYOUT, struct.pack('<II', 1, expected_size))
        checks.number(field['address'] + LAYOUT['property_offset_internal'], offset, '<I')
        if kind['name'] == 'ArrayProperty':
            inner = canonical(prop['inner']['identity']); inner_type = canonical(prop['inner']['type'])
            _require(inner['outer'] == field['address'] and inner['number'] == 0 and
                     inner['cls'] == inner_type['address'] and inner_type['name'] == 'ObjectProperty', 'array inner mismatch')
            checks.number(field['address'] + LAYOUT['struct_property_struct'], inner['address'])
            checks.add(inner['address'] + PROPERTY_LAYOUT, struct.pack('<II', 1, 8))

    # Validate membership using the saved authenticated traversal, without turning
    # its thousands of unrelated field reads into native per-call checks.
    field_headers = {}
    field_count = 0
    for group in _list(snapshot['fields'], 64):
        owner = _int(group['owner'], 1)
        _require(owner in classes and owner not in field_headers, 'invalid field owner')
        field_headers[owner] = _list(group['headers'], 2048)
        field_count += len(field_headers[owner])
        _require(field_count <= 4096, 'field evidence budget exceeded')
    for prop in properties:
        field = prop['identity']
        matches = [header for header in field_headers.get(field['outer'], []) if header.get('address') == field['address']]
        _require(len(matches) == 1 and all(matches[0].get(key) == field[key]
                 for key in ('internal_index', 'name_index', 'number', 'outer')), 'selected property not in authenticated owner fields')

    def property_for(role, name, owner, kind):
        _require((owner, name) in selected, 'missing required property')
        prop = selected[owner, name]
        _require(prop['type']['name'] == kind and prop['owner']['address'] in chain(roles[role]['cls']), 'defining property/source mismatch')
        return prop

    classification = snapshot['owner_classification']
    _require(classification in ('local_player', 'unowned_single_local_player'), 'unsupported owner classification')
    player_owner = roles['local_player']['address'] if classification == 'local_player' else 0
    _require(snapshot['classification_basis'] == ('explicit_local_player' if player_owner else 'null_owner_with_exact_single_player_graph'), 'owner classification basis mismatch')
    links = snapshot['links']
    _require(type(links) is dict and set(links) == set(LINKS), 'missing/unexpected links')
    for label, (role, name, owner, expected_role) in LINKS.items():
        expected = roles[expected_role]['address'] if expected_role else player_owner
        link = links[label]
        _require(type(link) is dict and type(link.get('object')) is int and link.get('object') == roles[role]['address'] and
                 link.get('property') == name and type(link.get('value')) is int and link['value'] == expected, 'dynamic owner graph mismatch')
        prop = property_for(role, name, owner, 'ObjectProperty')
        checks.number(roles[role]['address'] + prop['offset'], expected)
    arrays = snapshot['arrays']
    _require(type(arrays) is dict and set(arrays) == {'game_players', 'active_scenes'}, 'missing/unexpected arrays')
    for label, role, name, owner, maximum in (('game_players', 'engine', 'GamePlayers', 'Engine', 16),
                                              ('active_scenes', 'client', 'ActiveScenes', 'GameUISceneClient', 128)):
        array = arrays[label]
        prop = property_for(role, name, owner, 'ArrayProperty')
        _require(type(array['property']) is int and array['property'] == prop['identity']['address'], 'array property mismatch')
        data, count, capacity = _int(array['data']), _int(array['count'], 0, maximum), _int(array['capacity'], 0, 4096)
        members = _list(array['members'], maximum)
        _require(count == len(members) and count <= capacity and (not count or data) and data <= U64 - capacity * 8,
                 'invalid array header/capacity/overflow')
        _require(len(set(members)) == count and all(type(value) is int and value in identities for value in members), 'unregistered/duplicate array member')
        if label == 'game_players':
            _require(members == [roles['local_player']['address']], 'not exact single local player')
        else:
            _require(members.count(roles['scene']['address']) == 1, 'scene not uniquely active')
        checks.add(roles[role]['address'] + prop['offset'], struct.pack('<Qii', data, count, capacity))
        if count:
            checks.add(data, struct.pack('<' + 'Q' * count, *members))

    values = dict(schema=1, pid=pid, creation_time=creation_time, started_qpc=started_qpc,
        world_generation=world_generation, nonce=nonce, base='0x%X' % base,
        client='0x%X' % roles['client']['address'], scene='0x%X' % roles['scene']['address'],
        shift_x=shift_x, shift_y=shift_y, duration_ms=duration_ms, check_count=len(checks.spans))
    lines = ['[PathChoiceTrial]'] + [key + '=' + str(value) for key, value in values.items()]
    lines += ['check_%d=0x%X:%s' % (index, address, raw.hex())
              for index, ((address, size), raw) in enumerate(sorted(checks.spans.items()))]
    result = '\n'.join(lines) + '\n'
    result.encode('ascii')
    return result
