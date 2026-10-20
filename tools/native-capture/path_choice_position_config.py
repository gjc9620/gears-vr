"""Pure authenticated MenuReader snapshot -> private position-trial INI.

The caller acquires/final-rereads panelproof.checks in the same fresh acquisition
as the sample. This serializer performs no process access or persistence. Its
checks prove CPU identities/ABI only; they do not establish GPU/headset behavior.
"""
import copy
import hashlib
import json
import math
import struct

import path_choice_trial_config as owner

require, integer = owner._require, owner._int


def owner_digest(snapshot):
    return hashlib.sha256(json.dumps(snapshot, sort_keys=True, separators=(',', ':'), ensure_ascii=True,
                                     allow_nan=False).encode('ascii')).hexdigest()


def _address(text):
    require(type(text) is str and text.startswith('0x') and 2 < len(text) <= 18, 'invalid sample address')
    return integer(int(text, 16), 1)


def _fixed_prefix_headers(checks, base):
    """Only the two complete certified headers receive the runtime Num policy."""
    headers = (base + 0x1c24730, base + 0x1c34800)
    found = {}
    for (address, size), raw in checks.spans.items():
        for header in headers:
            if address < header + 16 and header < address + size:
                require(address == header and size == 16 and header not in found,
                        'partial/overlapping/aliased registry header check')
                data, count, capacity = struct.unpack('<Qii', raw)
                require(data and 0 < count <= capacity < 4000000 and data <= owner.U64 - capacity * 8,
                        'invalid fixed-prefix registry header')
                found[header] = data
    require(set(found) == set(headers) and len(set(found.values())) == 2,
            'missing/aliased fixed-prefix registry headers')


def build(sample, panelproof, *, pid, creation_time, started_qpc, world_generation, nonce, duration_ms=3000,
          vertical_offset_reference_pixels=16, target_vertical_region=None):
    """Require fresh raw coverage, exact owner digest, registered panel/reflection.

    panelproof has schema/kind/pid/module_base/read_only/consistent_reread,
    owner_snapshot_sha256, identities/classes/name_entries (new registered objects
    only), vtable, vslots, children (root Children metadata/header/members), and
    checks [{address: integer, raw: hex}]. Only derived immutable checks are emitted.
    """
    try:
        return _build(sample, panelproof, pid=pid, creation_time=creation_time, started_qpc=started_qpc,
                      world_generation=world_generation, nonce=nonce, duration_ms=duration_ms,
                      vertical_offset_reference_pixels=vertical_offset_reference_pixels,target_vertical_region=target_vertical_region)
    except (KeyError, TypeError, AttributeError, UnicodeError, struct.error, OverflowError) as error:
        raise ValueError('malformed position panel evidence') from error


def required_reads(sample, panelproof, **session):
    """Return [{address, size}] for a bounded fresh raw collector; no I/O.

    Same session keys as build; duration_ms defaults to3000. Supply checks=[]
    until these reads have been final-reread and serialized into panelproof.
    Native code certification remains in the native wrapper.
    """
    session.setdefault('duration_ms',3000)
    try:
        return _build(sample,panelproof,required_only=True,**session)
    except (KeyError, TypeError, AttributeError, UnicodeError, struct.error, OverflowError) as error:
        raise ValueError('malformed position panel evidence') from error


def _build(sample, proof, required_only=False, vertical_offset_reference_pixels=16, target_vertical_region=None, **session):
    vertical_offset=integer(vertical_offset_reference_pixels,1,400)
    require(type(sample) is dict and sample.get('read_only') is True and sample.get('consistent_reread') is True,
            'unverified layout sample')
    snapshot = sample['scene_owner']
    # Validate the untouched owner graph before accepting panel extensions.
    owner.build(snapshot, **session)
    keys = {'schema','kind','pid','module_base','read_only','consistent_reread','owner_snapshot_sha256',
            'identities','classes','name_entries','vtable','vslots','checks','children'}
    require(type(proof) is dict and set(proof) == keys and type(proof['schema']) is int and proof['schema'] == 1 and
            proof['kind'] == 'path_choice_position_panel_proof' and proof['read_only'] is True and
            proof['consistent_reread'] is True and proof['pid'] == session['pid'] == sample['pid'] == snapshot['pid'] and
            proof['module_base'] == snapshot['module_base'] and proof['owner_snapshot_sha256'] == owner_digest(snapshot),
            'stale/mismatched panel and owner acquisition')
    merged = copy.deepcopy(snapshot)
    for key in ('identities','classes','name_entries'):
        require(type(proof[key]) is list and len(proof[key]) <= 128, 'panel evidence budget')
        merged[key].extend(copy.deepcopy(proof[key]))
    owner_ini = owner.build(merged, **session)
    fields = dict(line.split('=',1) for line in owner_ini.splitlines()[1:] if line)
    checks = owner._Checks()
    for index in range(int(fields['check_count'])):
        at, raw = fields['check_'+str(index)].split(':')
        checks.add(int(at,16),bytes.fromhex(raw))
    identities = {i['address']:i for i in merged['identities']}
    classes = {c['identity']['address']:c['super'] for c in merged['classes']}
    scene = snapshot['roles']['scene'];root = scene['address'];base = snapshot['module_base']
    require(_address(sample['client']) == snapshot['roles']['client']['address'] and
            _address(sample['top']['address']) == root and sample['top']['cls'] == 'UIScene_PathChoice', 'wrong top/client')
    tree = owner._list(sample['tree'],128)
    roots = [r for r in tree if r.get('parent') is None]
    panels = [r for r in tree if r.get('name') == 'UIWarSafeRegionPanel']
    require(len(roots) == 1 and _address(roots[0]['address']) == root and len(panels) == 1, 'missing/ambiguous panel/root')
    row = panels[0];panel = _address(row['address'])
    require(_address(row['parent']) == root and _address(row['outer']) == root and row['depth'] == 2 and
            row['cls'] == 'UIWarSafeRegionPanel' and panel in identities, 'panel not registered direct root child')
    identity = identities[panel]
    fit_fields, fit_reads = {}, []
    if target_vertical_region is not None:
        require(vertical_offset == 16, 'fit and legacy pixel offset are mutually exclusive')
        from path_choice_group_fit import inputs
        immutable_fit_reads=[]
        fit_fields, fit_reads = inputs(sample,row,target_vertical_region,require,immutable_reads=immutable_fit_reads)
        # Viewport storage/metadata stays immutable for this lease. Content and
        # panel bounds remain acquisition-only because our own layout moves them.
        for span in immutable_fit_reads:checks.add(span['address'],span['raw'])
    require(identity['name'] == row['name'] and identity['number'] == row['number'] and identity['outer'] == root and
            identities[identity['cls']]['name'] == 'UIWarSafeRegionPanel', 'panel identity mismatch')
    chain = {};address = identity['cls']
    while address:
        require(address in classes and address not in chain.values() and len(chain)<64,'panel class ancestry')
        chain[identities[address]['name']] = address;address = classes[address]
    require(all(name in chain for name in ('UIWarSafeRegionPanel','UIObject','UIScreenObject','UIRoot')),'panel class ancestry')

    def reflected(meta, name, kind, defining_owner, offset, dimension, size):
        require(type(meta) is dict and meta['name'] == name and meta['type'] == kind and
                _address(meta['owner']) == defining_owner and type(meta['offset']) is int and meta['offset'] == offset and
                type(meta['array_dim']) is int and meta['array_dim'] == dimension and
                type(meta['element_size']) is int and meta['element_size'] == size, 'unsupported reflected layout')
        address = _address(meta['property_address']);field = identities.get(address)
        require(field is not None and field['name'] == name and field['number'] == 0 and field['outer'] == defining_owner and
                identities[field['cls']]['name'] == kind, 'unregistered reflected property')
        checks.add(address+owner.PROPERTY_LAYOUT,struct.pack('<II',dimension,size))
        checks.number(address+owner.LAYOUT['property_offset_internal'],offset,'<I')
        return address

    details = row['layout_details']
    require(details['face_order'] == ['left','top','right','bottom'],'unsupported face order')
    position = details['position'];structure = position['structure'];struct_at = structure['address']
    require(struct_at in identities and all(structure.get(k)==identities[struct_at][k] for k in ('address','name','number','cls','outer','internal_index')) and structure['name'] == 'UIScreenValue_Bounds' and
            structure['number'] == 0 and structure['outer'] == chain['UIRoot'] and
            identities[structure['cls']]['name'] == 'ScriptStruct', 'unregistered Position structure')
    posprop = reflected(position['reflected'],'Position','StructProperty',chain['UIScreenObject'],0x60,1,28)
    checks.number(posprop+owner.LAYOUT['struct_property_struct'],struct_at)
    members = position['members']
    reflected(members['Value']['reflected'],'Value','FloatProperty',struct_at,0,4,4)
    reflected(members['ScaleType']['reflected'],'ScaleType','ByteProperty',struct_at,16,4,1)
    raw = bytes.fromhex(members['Value']['raw']);units = bytes.fromhex(members['ScaleType']['raw'])
    values = members['Value']['values']
    require(type(values) is list and len(values)==4 and all(type(v) in (float,int) and math.isfinite(v) for v in values) and
            len(raw)==16 and struct.pack('<4f',*values)==raw and units==b'\x04'*4 and
            members['ScaleType']['values']==[4]*4, 'invalid raw position/units')
    top,bottom = struct.unpack('<4I',raw)[1::2]
    require(0<=values[1]<values[3]<=1,'unsafe starting position')
    # Authenticate membership without including any sibling object's geometry.
    children = proof['children']
    require(type(children) is dict and set(children)=={'reflected','data','count','capacity','members'},'invalid root children proof')
    offset = integer(children['reflected']['offset'],0,0x10000-16)
    reflected(children['reflected'],'Children','ArrayProperty',chain['UIScreenObject'],offset,1,16)
    data,count,capacity = integer(children['data'],1),integer(children['count'],1,128),integer(children['capacity'],1,4096)
    members_list = children['members']
    require(type(members_list) is list and count==len(members_list) and count<=capacity and
            len(set(members_list))==count and all(type(v) is int and 0<v<=owner.U64 for v in members_list) and
            members_list.count(panel)==1 and data<=owner.U64-capacity*8, 'invalid root child membership')
    checks.add(root+offset,struct.pack('<Qii',data,count,capacity));checks.add(data,struct.pack('<'+'Q'*count,*members_list))
    vtable = integer(proof['vtable'],1,owner.U64-0x400)
    require(type(proof['vslots']) is dict and proof['vslots']=={'0x3d8':base+0x2988d8,'0x3b8':base+0x297338},'unsupported actual panel vslots')
    checks.number(panel,vtable)
    for slot,target in proof['vslots'].items():checks.number(vtable+int(slot,16),target)
    checks.add(panel+0x70,units)
    _fixed_prefix_headers(checks, base)
    if required_only:
        reads = owner._Checks()
        for (address,size),expected in checks.spans.items():reads.add(address,expected)
        reads.add(panel+0x60,raw)
        for span in fit_reads:reads.add(span['address'],span['raw'])
        return [dict(address=address,size=size) for address,size in sorted(reads.spans)]
    # All source bytes, including the original raw positions, must be covered by
    # the caller's fresh final-reread ledger. None are accepted on labels alone.
    source = owner._Checks()
    for entry in owner._list(proof['checks'],2048):
        require(type(entry) is dict and set(entry)=={'address','raw'},'invalid fresh raw check')
        require(type(entry['raw']) is str and 0<len(entry['raw'])<=512 and len(entry['raw'])%2==0,'fresh raw span budget')
        source.add(integer(entry['address'],1),bytes.fromhex(entry['raw']))
    def observed(address,size):
        require(all(address+i in source.cells for i in range(size)),'missing fresh raw coverage')
        return bytes(source.cells[address+i] for i in range(size))
    require(observed(panel+0x60,16)==raw and observed(panel+0x70,4)==units,'stale raw position/units')
    for span in fit_reads:require(observed(span['address'],len(span['raw']))==span['raw'],'stale content RenderBounds')
    for (address,size),expected in checks.spans.items():
        actual=observed(address,size)
        require(actual==expected,'fresh identity/owner/reflection check mismatch at 0x%X size=%d expected=%s actual=%s' %
                (address,size,expected[:32].hex(),actual[:32].hex()))
    # Reject any accidental overlap rather than silently remove identity bytes.
    excluded = ((panel+0x64,4),(panel+0x6c,4),(panel+0x74,4))
    for (address,size) in checks.spans:
        require(not any(address<at+n and at<address+size for at,n in excluded),'mutable bytes in immutable checks')
    result = {key:fields[key] for key in ('schema','nonce','pid','creation_time','started_qpc','world_generation','base','client','scene')}
    result.update(schema=2,registry_header_policy='fixed_prefix_v1',panel=hex(panel),panel_vtable=hex(vtable),top_bits=top,bottom_bits=bottom,
                  duration_ms=session['duration_ms'],check_count=len(checks.spans))
    if vertical_offset!=16:result['vertical_offset_reference_pixels']=vertical_offset
    result.update(fit_fields)
    require(len(result)==(16 if vertical_offset==16 else 17)+len(fit_fields),'unexpected configuration keys')
    lines=['[PathChoicePositionTrial]']+[key+'='+str(value) for key,value in result.items()]
    lines+=['check_%d=0x%X:%s'%(i,address,raw.hex()) for i,((address,size),raw) in enumerate(sorted(checks.spans.items()))]
    ini='\n'.join(lines)+'\n';require(len(ini)<=262144,'configuration byte budget');ini.encode('ascii')
    return ini
