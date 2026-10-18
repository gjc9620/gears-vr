"""One read-only PathChoice activation certificate; no scheduling or persistence.

Native startup/poll owns code and ABI attestation. This collector proves current
metadata only. verify_session(reader) must independently check live game, owner,
and reader process handles and return the current running session document.
Consistent rereads cannot establish an atomic/ABA snapshot.
"""
import copy
from dataclasses import dataclass
import json
import os
from pathlib import Path
import struct

import path_choice_position_config as position
import path_choice_close_config as close
from scene_owner_snapshot import _Capture, _reread_spans

require = position.require

# Neutral supported content signature: names/classes and reflected Value/ScaleType
# bytes only, extracted from the validated baseline. No heap or image addresses.
SUPPORTED_LAYOUT = (
    ('BottomLeftBackground','UIImage','8943463ed2a8c03d46079c3ed61f013e','05050505'),
    ('BottomRightBackground','UIImage','21b47044d2a8c03dd8619c3ed61f013e','01050505'),
    ('DeltaChoosePath','UIScene_PathChoice','00000000000000000000803f0000803f','05050505'),
    ('LblLeftButton','UILabel','d712623e7ba3263e5ddf9f3e753a103d','05050505'),
    ('LblLeftDesc','UILabel','b107803e4926f53d835b803e8e5f0b3d','05050505'),
    ('LblRightButton','UILabel','dec8ec3e7ba3263e4640a53e46080f3d','05050505'),
    ('LblRightDesc','UILabel','5d31003f5226f53d165b803e8e5f0b3d','05050505'),
    ('Title','UILabel','4b20453e618a323db9a51d3f58c51b3d','05050505'),
    ('TopBackground','UIImage','8943463eb22d033d882d1d3f7fbe6d3d','05050505'),
    ('UIImage','UIImage','7fffbf3e39631d3ed0001243b8fe8f41','05050303'),
    ('UIWarSafeRegionPanel','UIWarSafeRegionPanel','29b91c3ebe30193db5d1583ff56c763f','04040404'),
    ('imgLeftArrow','UIImage','aaee613e315cdd3d0fb9193db2f5cc3d','05050505'),
    ('imgRightArrow','UIImage','a871473fab5bdd3d0fb919bdb2f5cc3d','05050505'),
)


def _json(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)


@dataclass(frozen=True)
class ActivationCertificate:
    """Immutable values for exactly one nonce; caller owns their short lifetime."""
    position_ini: str
    close_ini: str
    baseline_ids_json: str
    evidence_json: str


def _identity(value):
    require(type(value) is dict, 'missing process identity')
    pid = position.integer(value['pid'], 1, (1 << 32)-1)
    created = position.integer(value['creation_time'], 1)
    image = value['image']
    require(type(image) is str and Path(image).is_absolute(), 'absolute process image required')
    return dict(pid=pid, creation_time=created, image=os.path.normcase(str(Path(image).resolve())))


def _binding(session):
    require(session['phase'] == 'running', 'session is not running')
    return dict(game=_identity(session['game_identity']), owner=_identity(session['owner_identity']),
                native_started_qpc=position.integer(int(session['native_started_qpc']), 1), phase='running')


def _geometry(sample):
    tree = sample['tree']
    require(type(tree) is list and len(tree) == 13, 'unsupported PathChoice node inventory')
    signature = []
    for row in tree:
        members = row['layout_details']['position']['members']
        signature.append((row['name'], row['cls'], members['Value']['raw'], members['ScaleType']['raw']))
    require(tuple(sorted(signature)) == SUPPORTED_LAYOUT, 'unsupported PathChoice baseline geometry')
    return tuple(sorted((row['name'], row['cls'], row['address'], row.get('parent'), row.get('outer'),
                         row.get('number'), row.get('depth')) for row in tree))


def _panel_proof(r, sample):
    snapshot = sample['scene_owner']; cap = _Capture(r)
    node = next(row for row in sample['tree'] if row['name'] == 'UIWarSafeRegionPanel')
    panel = int(node['address'], 0); scene = int(sample['top']['address'], 0)
    obj = cap.identity(panel); cap.chain(obj['cls'])
    p = node['layout_details']['position']
    for address in (p['structure']['address'], int(p['reflected']['property_address'], 0),
                    int(p['members']['Value']['reflected']['property_address'], 0),
                    int(p['members']['ScaleType']['reflected']['property_address'], 0)):
        cap.identity(address)
    prop = cap.property(cap.identity(scene), 'Children', 'ArrayProperty', 'UIScreenObject')
    data, count, capacity = struct.unpack('<Qii', cap.raw(scene+prop['offset'], 16))
    require(data and 0 < count <= 128 and count <= capacity <= 4096, 'invalid root Children')
    members = list(struct.unpack('<'+'Q'*count, cap.raw(data, count*8)))
    vt = cap.pointer(panel); slots = {hex(slot):cap.pointer(vt+slot) for slot in (0x3d8, 0x3b8)}
    cap.finish()
    proof = dict(schema=1, kind='path_choice_position_panel_proof', pid=r.pid, module_base=r.base,
        read_only=True, consistent_reread=True, owner_snapshot_sha256=position.owner_digest(snapshot),
        vtable=vt, vslots=slots, checks=[], children=dict(data=data, count=count, capacity=capacity, members=members,
        reflected=dict(name='Children', type='ArrayProperty', owner=hex(prop['owner']['address']),
            property_address=hex(prop['identity']['address']), offset=prop['offset'], array_dim=1, element_size=16)))
    sources = dict(identities=cap.identities.values(), classes=cap.classes.values(), name_entries=cap.name_entries.values())
    keys = dict(identities=lambda x:x['address'], classes=lambda x:x['identity']['address'], name_entries=lambda x:x['index'])
    for name, values in sources.items():
        known = {keys[name](x):x for x in snapshot[name]}; proof[name] = []
        for item in values:
            key = keys[name](item)
            if key in known: require(known[key] == item, 'owner/panel identity drift')
            else: proof[name].append(item)
    return proof


def _close_metadata(r, sample, binding):
    cap = _Capture(r); client = cap.identity(sample['scene_owner']['roles']['client']['address'])
    require(client == sample['scene_owner']['roles']['client'], 'CloseScene client changed')
    require(cap.class_identity(client['cls'])['name'] == 'GameUISceneClient', 'wrong SceneClient class')
    owners = [cap.identities[address] for address in cap.chain(client['cls'])
              if cap.identities[address]['name'] == 'UISceneClient' and cap.identities[address]['number'] == 0]
    require(len(owners) == 1, 'ambiguous UISceneClient ancestry')
    declaring = owners[0]; package = cap.identity(declaring['outer'])
    require(package['name'] == 'Engine' and package['number'] == 0 and package['outer'] == 0 and
            cap.class_identity(package['cls'])['name'] == 'Package', 'wrong UISceneClient package')
    fields = []
    for field in cap.class_fields(declaring['address']):
        fields.append(cap.identity(field['address']))
    found = [field for field in fields if field['name'] == 'CloseScene' and field['number'] == 0 and
             field['outer'] == declaring['address'] and cap.class_identity(field['cls'])['name'] == 'Function']
    require(len(found) == 1, 'exact directly owned CloseScene Function required')
    function = found[0]; raw = cap.raw(function['address'], 304)
    require(struct.unpack_from('<Q', raw, 0x114)[0] == r.base+0x34698c, 'unsupported CloseScene native pointer')
    cap.finish()
    return dict(read_only=True, native_called=False, game_memory_written=False, pid=r.pid,
        creation_time=binding['game']['creation_time'], session_binding=binding, module_base=hex(r.base),
        client=client, owner_class=declaring, owned_field_chain=fields, function=function, header_hex=raw.hex(),
        native_pointer_field='0x114', metadata_only=True)


def collect(reader, sample, native_status, session, nonce, verify_session):
    """Acquire once, fail closed, return no partial receipt and keep no baseline.

    The supplied sample must include layout details and owner evidence. Reacquire
    once under a tracked ledger after refreshing both full inventories, retaining
    its original client/roles/node identities. verify_session is a mandatory live
    process/session check, called before and after the acquisition; it returns a
    current session document. Code/ABI attestation is exclusively native.
    """
    sample = copy.deepcopy(sample); native_status = copy.deepcopy(native_status)
    bound = _binding(session); r = reader.r; original_base = r.base
    require(callable(verify_session), 'live session verifier required')
    def check():
        require(_binding(verify_session(reader)) == bound, 'live session binding changed')
        require(r.pid == bound['game']['pid'], 'reader PID mismatch')
        require(r.base == original_base, 'reader module base changed')
    check()
    require(native_status['phase'] == 'running' and int(native_status['started_qpc']) == bound['native_started_qpc'],
            'native session/start mismatch')
    for name in ('identity_lost','ownership_lost','setter_failed','pending_restore','unresolved'):
        require(native_status['path_position_'+name] == '0', 'native position lease is not clean')
    expected_ids = _geometry(sample); roles = sample['scene_owner']['roles']
    original_client = copy.deepcopy(reader.client_identity)
    # MenuReader retains Resolver.identity()/registered()'s six-field identity.
    # The independently read owner role additionally contains name_index; keep
    # that richer identity intact for the exact fresh-role comparison below.
    menu_client = {key:roles['client'][key] for key in
                   ('address', 'name', 'number', 'cls', 'outer', 'internal_index')}
    require(reader.client == roles['client']['address'] and original_client == menu_client, 'original client mismatch')
    reader.force_registry_refresh()
    require(reader.client_identity == original_client and reader.client == original_client['address'], 'refresh replaced client')
    check()
    original_read = r.read; reads = {}; byte_count = 0
    def tracked(address, size):
        nonlocal byte_count
        require(type(address) is int and type(size) is int and 0 < address <= (1 << 64)-size and 0 < size <= 1024,
                'invalid bounded acquisition read')
        raw = original_read(address, size)
        require(type(raw) is bytes and len(raw) == size, 'short acquisition read')
        key = address, size
        require(key not in reads or reads[key] == raw, 'source changed during acquisition')
        byte_count += size
        require(len(reads) < 32768 and byte_count <= 8*1024*1024, 'acquisition read budget exceeded')
        reads[key] = raw
        return raw
    r.read = tracked
    try:
        r.ident_cache.clear(); r.names_cache.clear(); reader.properties.clear()
        for prefix, singular in (('objects','object'), ('names','name')):
            current = struct.unpack('<Qii', r.read(getattr(r,prefix+'_header'), 16))
            require(current == (getattr(r,prefix+'_data'), getattr(r,singular+'_count'), getattr(r,singular+'_capacity')),
                    'registry changed after refresh')
        fresh = reader._sample_once(include_tree=True, include_layout=True, include_layout_details=True, include_owner=True,
            expected_controller=roles['controller']['address'], expected_local_player=roles['local_player']['address'])
        require(fresh['scene_owner']['roles'] == roles and _geometry(fresh) == expected_ids, 'original baseline changed')
        proof = _panel_proof(r, fresh)
        args = dict(pid=r.pid, creation_time=bound['game']['creation_time'], started_qpc=bound['native_started_qpc'],
            world_generation=position.integer(int(native_status['world_generation']), 1), nonce=nonce,
            duration_ms=6000, vertical_offset_reference_pixels=374)
        for span in position.required_reads(fresh, proof, **args):
            proof['checks'].append(dict(address=span['address'], raw=r.read(span['address'],span['size']).hex()))
        metadata = _close_metadata(r, fresh, bound)
        position_ini = position.build(fresh, proof, **args)
        # The flags describe the ledger verified below, not a native code check.
        metadata.update(consistent_final_reread=True, process_owner_verified=True, atomic_snapshot=False)
        close_ini = close.build(fresh, proof, metadata, observer_duration_ms=8000, **args)
        for address, expected in _reread_spans(reads):
            require(original_read(address, len(expected)) == expected, 'whole-acquisition final reread changed')
        check()
        evidence = dict(read_only=True, consistent_final_reread=True, process_owner_verified=True,
            atomic_snapshot=False, read_ranges=len(reads), read_bytes=sum(len(raw) for raw in reads.values()),
            native_attestation_source='native startup and poll; no Python code/ABI attestation',
            sample=fresh, panel_proof=proof, close_metadata=metadata)
        baseline = dict(roles=roles, nodes=expected_ids, nonce=nonce, module_base=r.base,
                        owner_snapshot_sha256=position.owner_digest(fresh['scene_owner']))
        return ActivationCertificate(position_ini, close_ini, _json(baseline), _json(evidence))
    finally:
        r.read = original_read
