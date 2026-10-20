"""One read-only PathChoice activation certificate; no scheduling or persistence.

Native startup/poll owns code and ABI attestation. This collector proves current
metadata only. verify_session(reader) must independently check live game, owner,
and reader process handles and return the current running session document.
Consistent rereads cannot establish an atomic/ABA snapshot.
"""
import copy
from dataclasses import dataclass
import json
import math
import os
from pathlib import Path
import struct
import sys

import path_choice_position_config as position
import path_choice_close_config as close
from scene_owner_snapshot import _Capture, _reread_spans

require = position.require


def _read_origin():
    """Bounded source locations only; never retain frames or arbitrary locals."""
    frame = sys._getframe(2)
    result = []
    try:
        for _ in range(6):
            if frame is None: break
            row = dict(file=Path(frame.f_code.co_filename).name[:64],
                       function=frame.f_code.co_name[:64], line=frame.f_lineno)
            name = frame.f_locals.get('name')
            if type(name) is str: row['field'] = name[:64]
            result.append(row)
            frame = frame.f_back
    finally:
        del frame
    return result


class AcquisitionSourceChanged(ValueError):
    """Terminal byte disagreement with bounded evidence for the next diagnosis."""
    def __init__(self, phase, address, expected, actual, origin):
        offset = next((i for i, (a, b) in enumerate(zip(expected, actual)) if a != b),
                      min(len(expected), len(actual)))
        self.read_conflict = dict(phase=phase, address=hex(address), size=len(expected),
            actual_size=len(actual), changed_offset=offset, changed_address=hex(address+offset),
            expected_window=expected[offset:offset+8].hex(), actual_window=actual[offset:offset+8].hex(),
            first_read=origin)
        super().__init__(f'{phase}: source changed during acquisition at {address:#x} '
                         f'size {len(expected)} offset {offset:#x}')


class _FixedPrefixReads:
    """Explicit logical read projection, private to one PathChoice acquisition.

    Complete known headers are live-validated against the refreshed INITIAL
    prefix on every read, then represented by that initial header in every
    nested ledger. Other bytes are untouched. This does not assert that remote
    header bytes stayed equal, that suffixes are append-only, or atomicity.
    """
    def __init__(self, resolver, source_read):
        self.source_read = source_read
        self.headers = {}
        self.observations = {}
        for prefix, singular, rva in (('objects','object',0x1c24730), ('names','name',0x1c34800)):
            address = resolver.base+rva
            require(getattr(resolver,prefix+'_header') == address, 'fixed-prefix header anchor mismatch')
            initial = (getattr(resolver,prefix+'_data'), getattr(resolver,singular+'_count'),
                       getattr(resolver,singular+'_capacity'))
            require(self.valid(initial), 'invalid initial fixed-prefix header')
            raw = struct.pack('<Qii',*initial)
            self.headers[address] = raw
            self.observations[prefix] = dict(address=hex(address), initial_raw=raw.hex(),
                live_reads=0, accepted_suffix_reads=0, first_live_headers=[], last_live_header=None)
        require(len({struct.unpack('<Qii',raw)[0] for raw in self.headers.values()}) == 2,
                'aliased fixed-prefix registries')

    @staticmethod
    def valid(header):
        data, count, capacity = header
        return (type(data) is int and type(count) is int and type(capacity) is int and
                0 < data <= (1 << 64)-1-capacity*8 and 0 < count <= capacity < 4000000)

    def read(self, address, size):
        overlaps = [(header,initial) for header,initial in self.headers.items()
                    if address < header+16 and header < address+size]
        # A merged final span can contain a whole header and ordinary bytes;
        # a partial/straddling header has no policy interpretation and rejects.
        require(all(address <= header and header+16 <= address+size for header,_ in overlaps),
                'partial fixed-prefix header read')
        raw = self.source_read(address,size)
        require(type(raw) is bytes and len(raw) == size, 'short fixed-prefix acquisition read')
        for header, initial in overlaps:
            offset = header-address
            observed = raw[offset:offset+16]
            first, live = struct.unpack('<Qii',initial), struct.unpack('<Qii',observed)
            if not (self.valid(live) and live[0] == first[0] and live[2] == first[2] and live[1] >= first[1]):
                raise AcquisitionSourceChanged('fixed_prefix_header',header,initial,observed,_read_origin())
            record = next(row for row in self.observations.values() if row['address'] == hex(header))
            record['live_reads'] += 1
            record['accepted_suffix_reads'] += int(live[1] != first[1])
            if len(record['first_live_headers']) < 8: record['first_live_headers'].append(observed.hex())
            record['last_live_header'] = observed.hex()
            raw = raw[:offset]+initial+raw[offset+16:]
        return raw

    def semantics(self):
        return dict(registry_header_policy='fixed_prefix_v1', normalized_fixed_prefix_headers=True,
                    consistency_semantics='logical_fixed_prefix_consistency', nonheader_bytes_exact=True)

def sample_fixed_prefix(reader, **options):
    """One PathChoice-only preliminary sample; default MenuReader stays exact.

    A fresh inventory/client bracket establishes this call's initial prefix.
    Existing snapshot, slot, layout and owner checks then consume that same
    validated logical read projection. No failed bytes or policy survive it.
    """
    reader.force_registry_refresh(registry_header_policy="fixed_prefix_v1")
    r = reader.r
    original_read = r.read
    policy = _FixedPrefixReads(r, original_read)
    r.read = policy.read
    try:
        sample = reader.sample(**options)
        sample.update(policy.semantics())
        sample['registry_header_observations'] = copy.deepcopy(policy.observations)
        if 'scene_owner' in sample:
            sample['scene_owner']['evidence'].update(policy.semantics(),
                source_bytes_semantics='digest_of_logical_fixed_prefix_read_ledger')
        return sample
    finally:
        r.read = original_read


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


def _geometry(sample, *, shared_fit=False):
    tree = sample['tree']
    require(type(tree) is list and len(tree) == 13, 'unsupported PathChoice node inventory')
    signature = []
    for row in tree:
        members = row['layout_details']['position']['members']
        signature.append((row['name'], row['cls'], members['Value']['raw'], members['ScaleType']['raw']))
    if not shared_fit:
        # Legacy fixed-reference-pixel policy retains its historical signature.
        require(tuple(sorted(signature)) == SUPPORTED_LAYOUT, 'unsupported PathChoice baseline geometry')
    else:
        require(sorted((row['name'],row['cls']) for row in tree) ==
                sorted((name,kind) for name,kind,_,_ in SUPPORTED_LAYOUT),
                'unsupported PathChoice content inventory')
        for row in tree:
            members=row['layout_details']['position']['members']
            value,units=members['Value'],members['ScaleType']
            require(len(value['values'])==4 and all(type(x) in (int,float) and math.isfinite(x) for x in value['values']) and
                    struct.pack('<4f',*value['values']).hex()==value['raw'] and
                    len(units['raw'])==8 and list(bytes.fromhex(units['raw']))==units['values'],
                    'invalid current PathChoice position storage')
    identities=tuple(sorted((row['name'], row['cls'], row['address'], row.get('parent'), row.get('outer'),
                             row.get('number'), row.get('depth')) for row in tree))
    if not shared_fit:return identities
    # A fit uses the CURRENT complete group's layout, not old per-widget Value
    # bytes. Still require the entire original snapshot to survive reacquisition:
    # metadata, positions, docking, evaluated bounds and class/owner evidence.
    return identities,tuple(sorted((row['address'],_json(row)) for row in tree))


def _viewport_domain(r, sample):
    """Fresh reflected UIScene storage; never infer dimensions from panel bounds."""
    cap=_Capture(r);source=r.read;scene=cap.identity(int(sample['top']['address'],0))
    require(scene==sample['scene_owner']['roles']['scene'],'viewport scene identity mismatch')
    chain=cap.chain(scene['cls']);r.read=cap.raw
    try:
        prop=r.property(scene['cls'],'CurrentViewportSize',True)
        field=cap.identity(prop['address']);owner=cap.class_identity(field['outer'])
        require(field['name']=='CurrentViewportSize' and field['number']==0 and prop['type']=='StructProperty' and
                cap.class_identity(field['cls'])['name']=='StructProperty' and owner['name']=='UIScene' and
                owner['number']==0 and owner['address'] in chain,'unsupported viewport property owner/type')
        package=cap.identity(owner['outer'])
        require(package['name']=='Engine' and package['number']==0 and package['outer']==0 and
                cap.class_identity(package['cls'])['name']=='Package','unsupported viewport Engine owner')
        require(sum(item['address']==field['address'] for item in cap.class_fields(owner['address']))==1,
                'unreachable viewport property')
        dimension,size=struct.unpack('<II',cap.raw(field['address']+0x70,8))
        offset=struct.unpack('<I',cap.raw(field['address']+r.profile['property_offset_internal'],4))[0]
        require(dimension==1 and 8<=size<=128 and offset==prop['offset'] and 0<=offset<=0x10000-size,
                'unsupported viewport property extent')
        structure=r.struct_type(prop)
        require(structure==cap.pointer(field['address']+r.profile['struct_property_struct']),
                'viewport struct pointer changed')
        struct_identity=cap.identity(structure)
        require(cap.class_identity(struct_identity['cls'])['name']=='ScriptStruct','invalid viewport struct type')
        members={};offsets=[]
        for name in ('X','Y'):
            member=r.property(structure,name,False);identity=cap.identity(member['address'])
            require(member['type']=='FloatProperty' and identity['name']==name and identity['number']==0 and
                    identity['outer']==structure and cap.class_identity(identity['cls'])['name']=='FloatProperty' and
                    sum(item['address']==identity['address'] for item in cap.class_fields(structure))==1,
                    'unsupported viewport member owner/type')
            dim,extent=struct.unpack('<II',cap.raw(identity['address']+0x70,8))
            at=struct.unpack('<I',cap.raw(identity['address']+r.profile['property_offset_internal'],4))[0]
            require((dim,extent)==(1,4) and at==member['offset'] and 0<=at<=size-4,
                    'unsupported viewport member extent')
            offsets.append(at);members[name]=dict(identity=identity,type='FloatProperty',offset=at,array_dim=dim,element_size=extent)
        require(abs(offsets[0]-offsets[1])>=4,'overlapping viewport members')
        address=scene['address']+offset;raw=cap.raw(address,size)
        values=[struct.unpack_from('<f',raw,at)[0] for at in offsets]
        require(all(math.isfinite(v) and v>0 for v in values),'invalid reflected viewport size')
    finally:
        r.read=source
    cap.finish()
    return dict(schema=1,kind='reflected_uiscene_viewport_domain',read_only=True,consistent_reread=True,
        scene=scene,property=dict(identity=field,owner=owner,type='StructProperty',offset=offset,array_dim=dimension,element_size=size),
        struct_type=struct_identity,members=members,value_address=address,raw=raw.hex(),values_xy=values,
        source_checks=[dict(address=at,raw=data.hex()) for at,data in _reread_spans(cap.reads)])


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


def collect(reader, sample, native_status, session, nonce, verify_session, *, registry_header_policy='exact', target_vertical_region=None):
    """Acquire once, fail closed, return no partial receipt and keep no baseline.

    The supplied sample must include layout details and owner evidence. Reacquire
    once under a tracked ledger after refreshing both full inventories, retaining
    its original client/roles/node identities. verify_session is a mandatory live
    process/session check, called before and after the acquisition; it returns a
    current session document. Code/ABI attestation is exclusively native.
    """
    require(registry_header_policy in ('exact','fixed_prefix_v1'), 'unsupported acquisition registry policy')
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
    shared_fit=target_vertical_region is not None
    expected_ids = _geometry(sample,shared_fit=shared_fit); roles = sample['scene_owner']['roles']
    original_client = copy.deepcopy(reader.client_identity)
    # MenuReader retains Resolver.identity()/registered()'s six-field identity.
    # The independently read owner role additionally contains name_index; keep
    # that richer identity intact for the exact fresh-role comparison below.
    menu_client = {key:roles['client'][key] for key in
                   ('address', 'name', 'number', 'cls', 'outer', 'internal_index')}
    require(reader.client == roles['client']['address'] and original_client == menu_client, 'original client mismatch')
    reader.force_registry_refresh(registry_header_policy=registry_header_policy)
    require(reader.client_identity == original_client and reader.client == original_client['address'], 'refresh replaced client')
    check()
    original_read = r.read; reads = {}; origins = {}; byte_count = 0
    policy = _FixedPrefixReads(r,original_read) if registry_header_policy == 'fixed_prefix_v1' else None
    acquisition_read = policy.read if policy else original_read
    def tracked(address, size):
        nonlocal byte_count
        require(type(address) is int and type(size) is int and 0 < address <= (1 << 64)-size and 0 < size <= 1024,
                'invalid bounded acquisition read')
        raw = acquisition_read(address, size)
        require(type(raw) is bytes and len(raw) == size, 'short acquisition read')
        key = address, size
        if key in reads and reads[key] != raw:
            raise AcquisitionSourceChanged('repeated_read', address, reads[key], raw, origins[key])
        byte_count += size
        require(len(reads) < 32768 and byte_count <= 8*1024*1024, 'acquisition read budget exceeded')
        if key not in reads: origins[key] = _read_origin()
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
        require(fresh['scene_owner']['roles'] == roles and _geometry(fresh,shared_fit=shared_fit) == expected_ids, 'original baseline changed')
        if shared_fit:fresh['viewport_domain']=_viewport_domain(r,fresh)
        if policy:
            for header in fresh['scene_owner']['evidence']['registry_headers'].values():
                require(policy.headers.get(header['address']) == struct.pack('<Qii',header['data'],header['count'],header['capacity']),
                        'serializer fixed-prefix baseline mismatch')
            fresh.update(policy.semantics())
            fresh['scene_owner']['evidence'].update(policy.semantics(),
                source_bytes_semantics='digest_of_logical_fixed_prefix_read_ledger')
        proof = _panel_proof(r, fresh)
        args = dict(pid=r.pid, creation_time=bound['game']['creation_time'], started_qpc=bound['native_started_qpc'],
            world_generation=position.integer(int(native_status['world_generation']), 1), nonce=nonce,
            duration_ms=6000, vertical_offset_reference_pixels=374)
        if target_vertical_region is not None:
            args.update(vertical_offset_reference_pixels=16,target_vertical_region=target_vertical_region)
        for span in position.required_reads(fresh, proof, **args):
            proof['checks'].append(dict(address=span['address'], raw=r.read(span['address'],span['size']).hex()))
        metadata = _close_metadata(r, fresh, bound)
        position_ini = position.build(fresh, proof, **args)
        # The flags describe the ledger verified below, not a native code check.
        metadata.update(consistent_final_reread=True, process_owner_verified=True, atomic_snapshot=False)
        if policy: metadata.update(policy.semantics())
        close_ini = close.build(fresh, proof, metadata, observer_duration_ms=8000, **args)
        for address, expected in _reread_spans(reads):
            actual = acquisition_read(address, len(expected))
            require(type(actual) is bytes, 'invalid whole-acquisition final reread')
            if actual != expected:
                offset = next((i for i, (a,b) in enumerate(zip(expected,actual)) if a != b),
                              min(len(expected),len(actual)))
                origin = next((origins[key] for key in reads
                               if key[0] <= address+offset < key[0]+key[1]), [])
                raise AcquisitionSourceChanged('final_reread', address, expected, actual, origin)
        check()
        evidence = dict(read_only=True, consistent_final_reread=True, process_owner_verified=True,
            atomic_snapshot=False, read_ranges=len(reads), read_bytes=sum(len(raw) for raw in reads.values()),
            native_attestation_source='native startup and poll; no Python code/ABI attestation',
            sample=fresh, panel_proof=proof, close_metadata=metadata)
        if policy:
            evidence.update(policy.semantics(), registry_header_observations=policy.observations)
        baseline = dict(roles=roles, nodes=expected_ids[0] if shared_fit else expected_ids, nonce=nonce, module_base=r.base,
                        owner_snapshot_sha256=position.owner_digest(fresh['scene_owner']))
        return ActivationCertificate(position_ini, close_ini, _json(baseline), _json(evidence))
    finally:
        r.read = original_read
