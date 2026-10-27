"""Fresh, read-only UI observer certificate. No game calls or saved heap addresses.

Frame.Locals describes script locals, never decoded native Canvas arguments.
The caller supplies its prepared, SHA-gated Resolver and current camera result.
"""
import hashlib
import struct
import time
from resolve_camera import DiscoveryRegistryChanged

GAME_SHA = 'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4'
LAYOUT = ('object_internal_index','object_name','object_class','object_outer',
          'field_super','name_entry_index','name_entry_text','property_offset_internal')
SCOPES = (('WarHUD','DrawWeaponCrosshair',1),('WarHUD','DrawWeaponCrosshairGOW3',1),
          ('WarWeapon','DrawAdditionalCrosshair',1),('Weap_AssaultRifle','DrawAdditionalCrosshair',1),
          ('Weap_LocustAssaultRifle','DrawAdditionalCrosshair',1),
          ('WarInventoryManager','DrawWeaponSlots',2),('WarHUD','PostRender',3))
BINDINGS = {
    'vm': (0x1d6ca0,'4053555641564881eca8000000488b05'),
    'tile': (0x1d52b4,'488bc448895808555657488d68a14881'),
    'material': (0x3a4d3c,'488bc448895808488970185557415648'),
    'text': (0x2482b8,'488bc44889581855565741564157488d'),
}
CANVAS = {'clip_x':('ClipX','FloatProperty'),'clip_y':('ClipY','FloatProperty'),
          'cur_x':('CurX','FloatProperty'),'cur_y':('CurY','FloatProperty'),
          'org_x':('OrgX','FloatProperty'),'org_y':('OrgY','FloatProperty'),
          'size_x':('SizeX','IntProperty'),'size_y':('SizeY','IntProperty')}

class UIConfigRejected(RuntimeError):
    pass

class UIRegistryChanged(UIConfigRejected):
    """Only the final concurrent registry-header change permits reacquisition."""

class UIControlledPawnUnavailable(UIConfigRejected):
    """A fresh registered controller is unpossessed; no certificate is usable."""
    def __init__(self,controller,field,offset):
        super().__init__('UI controlled Pawn unavailable; reacquire')
        self.evidence=dict(controller=controller,field=field,offset=offset,pointer=0)

def build_ui_observer_config(resolver, camera, *, entry_reader=None,mounted_anchor=None,
                             reuse_assembly_identities=True,coalesce_reads=True):
    r=resolver;p=r.profile;started=time.perf_counter_ns()
    if type(reuse_assembly_identities) is not bool:
        raise ValueError('reuse_assembly_identities must be boolean')
    if type(coalesce_reads) is not bool:raise ValueError('coalesce_reads must be boolean')
    mounted=mounted_anchor is not None
    if mounted:
        from anchor_config import validate_anchor_config
        validate_anchor_config(mounted_anchor)
        if mounted_anchor['schema']!=3:raise UIConfigRejected('Mounted UI requires mounted anchor')
    if p.get('sha256')!=GAME_SHA or camera.get('sha256')!=GAME_SHA or camera.get('pid')!=r.pid:
        raise UIConfigRejected('UI observer requires the validated current game build/PID')
    if int(camera['module_base'],0)!=r.base:
        raise UIConfigRejected('Camera/module identity mismatch')
    def stable(address,size):
        value=r.read(address,size)
        if r.read(address,size)!=value:raise UIConfigRejected('UI certificate read changed')
        return value
    def read_entry(address,size):
        return entry_reader(address,size,r.read) if entry_reader is not None else r.read(address,size)
    def stable_entry(address,size):
        value=read_entry(address,size)
        if read_entry(address,size)!=value:raise UIConfigRejected('UI certificate read changed')
        return value
    names_header=stable(r.names_header,16);objects_header=stable(r.objects_header,16)
    names,nc,ncap=struct.unpack('<Qii',names_header);objects,oc,ocap=struct.unpack('<Qii',objects_header)
    if not names or not objects or not 1000<nc<=ncap<4000000 or not 1000<oc<=ocap<4000000:
        raise UIConfigRejected('Invalid UI registry headers')
    def fresh_read(address):
        if not address:raise UIConfigRejected('Null UI object')
        def identity_fields(raw):
            return (struct.unpack_from('<I',raw,p['object_internal_index'])[0],
                    *struct.unpack_from('<II',raw,p['object_name']),
                    struct.unpack_from('<Q',raw,p['object_class'])[0],
                    struct.unpack_from('<Q',raw,p['object_outer'])[0])
        # Object flags/state outside these fields may change during play. Keep
        # two complete snapshots, but compare only the certified identity tuple.
        fields=identity_fields(r.read(address,0x60))
        if identity_fields(r.read(address,0x60))!=fields:
            raise UIConfigRejected('UI object identity read changed')
        index,ni,number,cls,outer=fields
        if index>=oc or r.u64(objects+index*8)!=address:raise UIConfigRejected('UI object registry slot mismatch')
        if ni>=nc:raise UIConfigRejected('UI name index outside registry')
        entry=r.u64(names+ni*8)
        if not entry:raise UIConfigRejected('UI name registry identity mismatch')
        if coalesce_reads and (p['name_entry_index'],p['name_entry_text'])==(0,20):
            # Compare precisely the same selected name bytes in two fresh
            # snapshots; padding/state between index and text is not identity.
            first=r.read(entry,148);second=r.read(entry,148)
            if len(first)!=148 or len(second)!=148:raise UIConfigRejected('Incomplete UI name read')
            if first[:4]!=second[:4] or first[20:]!=second[20:]:
                raise UIConfigRejected('UI certificate read changed')
            entry_index=struct.unpack_from('<I',first)[0];text=first[20:]
        else:
            entry_index=r.u32(entry+p['name_entry_index'])
            text=stable(entry+p['name_entry_text'],128)
        if entry_index!=ni:raise UIConfigRejected('UI name registry identity mismatch')
        end=next((i for i in range(0,128,2) if text[i:i+2]==b'\0\0'),None)
        if end is None:raise UIConfigRejected('UI name exceeds bounded read')
        name=text[:end].decode('utf-16-le')
        if not name or any(c in name for c in '\r\n\0='):
            raise UIConfigRejected('Invalid UI certificate name')
        return dict(address=address,internal_index=index,name_index=ni,number=number,name=name,
                    cls=cls,outer=outer)
    assembly_identities={};assembly_uncached_reads=0
    def fresh(address):
        # Repeated class/property lookups use this attempt's first complete,
        # double-read identity. This is an assembly hint, not final authority.
        # Every selected identity is read again below without this dictionary,
        # against the closing registry headers, including slots and live names.
        nonlocal assembly_uncached_reads
        if not reuse_assembly_identities:return fresh_read(address)
        if address in assembly_identities:return assembly_identities[address]
        item=fresh_read(address)
        if len(assembly_identities)<256:assembly_identities[address]=item
        else:
            # Function discovery can visit many unselected fields. Exhausting
            # this optional hint cache must not reject an otherwise valid
            # certificate; retain its bound and use the original live read.
            assembly_uncached_reads+=1
        return item
    ids=[];by_address={};supers={};checks=[];entry_checks=[]
    def add(address):
        item=fresh(address)
        if address in by_address:
            index=by_address[address]
            if ids[index]!=item:raise UIConfigRejected('UI identity changed during assembly')
            return index
        if len(ids)>=96:raise UIConfigRejected('UI identity limit exceeded')
        index=len(ids);by_address[address]=index;ids.append(item);return index
    def is_class(address,name=None):
        item=fresh(address)
        if fresh(item['cls'])['name']!='Class' or item['number'] or (name is not None and item['name']!=name):
            raise UIConfigRejected('UI owner is not the exact registered class')
        add(address);return item
    class_names={x[0] for x in SCOPES}|{'Canvas'};classes={}
    # One bounded registry traversal; no heap scan or generic UObject dump.
    raw_pointers=stable(objects,oc*8)
    view=getattr(r,'discovery_pointer_view',None)
    pointers=view(raw_pointers,oc)if callable(view)else None
    if pointers is None:pointers=struct.unpack(f'<{oc}Q',raw_pointers)
    del raw_pointers
    # This capability batches discovery hints only. It has no authority over
    # fresh(), selected identities, registry bounds or the final checks below.
    # Providers without this optional capability retain their ordinary path.
    discover=getattr(r,'discovery_identity_reader',lambda:r.identity)()
    discover_class=getattr(discover,'class_identity',None)
    def ordinary_candidates():
        for address in pointers:
            if not address:continue
            try:
                if discover_class is not None and discover_class(address)['name']!='Class':continue
                yield address,discover(address)
            except DiscoveryRegistryChanged:raise
            except (RuntimeError,OSError,ValueError,UnicodeError):continue
    batched=getattr(r,'discovery_class_candidates',None)
    candidates=batched(pointers) if callable(batched) else ordinary_candidates()
    found={}
    for address,hint in candidates:
        if hint['name'] not in class_names:continue
        item=fresh(address)
        if item['number'] or fresh(item['cls'])['name']!='Class':continue
        if item['name'] in classes:raise UIConfigRejected('Duplicate UI owner class')
        classes[item['name']]=address;found[item['name']]=item
    if set(classes)!=class_names:raise UIConfigRejected('Required UI owner class missing')
    # Keep the original registry ordering in the authoritative certificate.
    # Re-prove the complete first identity before recording its ordered ID.
    for name,item in sorted(found.items(),key=lambda pair:pair[1]['internal_index']):
        if fresh_read(item['address'])!=item:raise UIRegistryChanged('UI class changed during discovery; reacquire')
        is_class(item['address'],name)
    def property_info(owner,name,kind):
        prop=r.property(owner,name,True);field=fresh(prop['address'])
        if field['name']!=name or fresh(field['cls'])['name']!=kind or prop['type']!=kind:
            raise UIConfigRejected('UI reflected property type/name mismatch: '+name)
        offset=r.u32(prop['address']+p['property_offset_internal'])
        if type(offset)!=int or not 0<=offset<0x10000 or offset!=prop['offset']:
            raise UIConfigRejected('Unsupported UI reflected offset: '+name)
        # The defining owner must be in this class's actual superclass chain.
        current=owner;seen=set()
        while current and current not in seen and len(seen)<40:
            is_class(current);seen.add(current)
            parent=r.u64(current+p['field_super']);supers[current]=parent
            if current==field['outer']:break
            current=parent
        else:raise UIConfigRejected('UI reflected property outer mismatch: '+name)
        add(prop['address']);checks.append((prop['address']+p['property_offset_internal'],struct.pack('<I',offset)))
        return offset,by_address[prop['address']]
    fields={};field_ids={};roles={};links=[]
    def prop(label,owner,name,kind):
        fields[label],field_ids[label]=property_info(owner,name,kind)
        return fields[label]
    pc=int(camera['controller_address'],0);cam=int(camera['camera_address'],0)
    for label,address in (('controller',pc),('camera',cam)):
        if fresh(address)['name'].startswith('Default__'):raise UIConfigRejected('Default UI role')
        roles[label]=add(address)
    def link(label,source,name,target=None):
        offset=prop(label,fresh(source)['cls'],name,'ObjectProperty')
        pointer=r.u64(source+offset)
        if label=='controller_pawn' and not pointer:
            # Death/loading can unpossess the controller after camera/anchor
            # acquisition. Classify only this null live link, after rechecking
            # its source identity and reflected field; discard all certificates.
            controller=ids[by_address[source]];field=ids[field_ids[label]]
            if fresh(source)!=controller or fresh(field['address'])!=field:
                raise UIConfigRejected('UI identity changed during assembly')
            if r.u32(field['address']+p['property_offset_internal'])!=offset:
                raise UIConfigRejected('UI field/link/entry changed during assembly')
            if r.u64(source+offset):raise UIConfigRejected('UI certificate read changed')
            raise UIControlledPawnUnavailable(controller.copy(),field.copy(),offset)
        if (not pointer and target!=0) or (target is not None and pointer!=target):raise UIConfigRejected('UI reference mismatch: '+label)
        links.append((source+offset,struct.pack('<Q',pointer)))
        if pointer:add(pointer)
        return pointer
    controlled=link('controller_pawn',pc,'Pawn');pawn=controlled
    if mounted:
        expected=mounted_anchor['identities'][mounted_anchor['roles']['pawn']]
        if any(fresh(controlled)[key]!=expected[key] for key in fresh(controlled)):
            raise UIConfigRejected('Mounted UI controlled role changed')
        pawn=link('controlled_driver',controlled,'Driver')
        expected=mounted_anchor['identities'][mounted_anchor['mounted']['role_driver']]
        if any(fresh(pawn)[key]!=expected[key] for key in fresh(pawn)):
            raise UIConfigRejected('Mounted UI Driver changed')
        roles['controlled']=add(controlled)
        link('pawn_driven_vehicle',pawn,'DrivenVehicle',controlled)
        link('controlled_controller',controlled,'Controller',pc)
    hud=link('controller_hud',pc,'myHUD')
    inventory=link('pawn_inventory',pawn,'InvManager')
    roles.update(pawn=add(pawn),hud=add(hud),inventory=add(inventory),canvas_class=add(classes['Canvas']))
    link('player_camera',pc,'PlayerCamera',cam);link('camera_owner',cam,'PCOwner',pc)
    link('hud_owner',hud,'PlayerOwner',pc);link('pawn_controller',pawn,'Controller',0 if mounted else pc)
    def derives(address,ancestor):
        current=fresh(address)['cls'];seen=set()
        while current and current not in seen and len(seen)<40:
            is_class(current);seen.add(current)
            parent=r.u64(current+p['field_super']);supers[current]=parent
            if current==ancestor:return
            current=parent
        raise UIConfigRejected('UI role class mismatch')
    derives(hud,classes['WarHUD']);derives(inventory,classes['WarInventoryManager'])
    prop('hud_canvas',fresh(hud)['cls'],'Canvas','ObjectProperty')
    prop('weapon_instigator',classes['WarWeapon'],'Instigator','ObjectProperty')
    for label,(name,kind) in CANVAS.items():prop('canvas_'+label,classes['Canvas'],name,kind)
    scopes=[];functions={}
    def function(owner_name,name):
        owner=classes[owner_name];found=[]
        for field in r.fields(owner):
            item=fresh(field['address'])
            if item['name']==name and fresh(item['cls'])['name']=='Function':found.append(item)
        if len(found)!=1 or found[0]['outer']!=owner or found[0]['number']:
            raise UIConfigRejected('Exact UI owner/function lookup rejected: '+owner_name+'.'+name)
        item=found[0];add(item['address']);functions[owner_name+'.'+name]=item['address'];return item
    vm_rva,vm_bytes=BINDINGS['vm'];bindings={}
    def binding(label,address):
        expected_rva,expected=BINDINGS[label]
        if address!=r.base+expected_rva:raise UIConfigRejected('UI binding RVA mismatch: '+label)
        raw=stable_entry(address,16)
        if raw.hex()!=expected:raise UIConfigRejected('UI entry bytes mismatch: '+label)
        bindings[label]=dict(rva=expected_rva,bytes=raw.hex());entry_checks.append((address,raw))
    for owner,name,kind in SCOPES:
        item=function(owner,name);address=item['address']
        entry=stable(address+0x114,8)
        if struct.unpack('<Q',entry)[0]!=r.base+vm_rva:raise UIConfigRejected('UI script dispatch mismatch')
        header=stable(address+0x8c,16);script,size,capacity=struct.unpack('<Qii',header)
        if not script or not 0<size<=capacity<=131072:raise UIConfigRejected('Invalid UI Script range')
        stable(script,size) # Admission verifies the complete bounded readable range.
        checks.extend(((address+0x114,entry),(address+0x8c,header)))
        scopes.append(dict(kind=kind,owner=add(classes[owner]),function=add(address),script=script,size=size))
    binding('vm',r.base+vm_rva)
    for label,name in (('tile','DrawTile'),('material','DrawMaterialTile'),('text','DrawText')):
        item=function('Canvas',name);entry=stable(item['address']+0x114,8)
        checks.append((item['address']+0x114,entry));binding(label,struct.unpack('<Q',entry)[0])
        bindings[label]['function']=add(item['address'])
    # Certify class identity closure; only selected ancestors need super checks.
    cursor=0
    while cursor<len(ids):
        item=ids[cursor];add(item['cls'])
        if fresh(item['cls'])['name']=='Class':
            current=item['address'];seen=set()
            while current:
                if current in seen or len(seen)>=40:raise UIConfigRejected('Invalid UI superclass chain')
                seen.add(current);is_class(current)
                parent=r.u64(current+p['field_super']);supers[current]=parent
                if parent:add(parent)
                current=parent
        cursor+=1
    # Registry growth/reallocation is normal during loading. Refresh bounds and
    # prove every selected slot/name again; never reuse an old table as authority.
    names_header=stable(r.names_header,16);objects_header=stable(r.objects_header,16)
    names,nc,ncap=struct.unpack('<Qii',names_header);objects,oc,ocap=struct.unpack('<Qii',objects_header)
    if not names or not objects or not 1000<nc<=ncap<4000000 or not 1000<oc<=ocap<4000000:
        raise UIConfigRejected('Invalid final UI registry headers')
    for item in ids:
        if fresh_read(item['address'])!=item:raise UIConfigRejected('UI final identity mismatch')
    for address,parent in supers.items():
        if r.u64(address+p['field_super'])!=parent:raise UIConfigRejected('UI superclass changed')
    for address,value in checks+links:
        if r.read(address,len(value))!=value:raise UIConfigRejected('UI field/link/entry changed during assembly')
    for address,value in entry_checks:
        if read_entry(address,len(value))!=value:raise UIConfigRejected('UI field/link/entry changed during assembly')
    if r.read(r.names_header,16)!=names_header or r.read(r.objects_header,16)!=objects_header:
        raise UIRegistryChanged('UI registry changed during assembly; reacquire')
    config=dict(schema=1,pid=r.pid,base=r.base,sha256=GAME_SHA,objects_header=r.objects_header,
                names_header=r.names_header,layout={key:p[key] for key in LAYOUT},identities=ids,
                supers=[dict(identity=by_address[a],parent=b) for a,b in sorted(supers.items())],roles=roles,
                fields=fields,field_ids=field_ids,scopes=scopes,bindings=bindings,
                frame_node=0x18,frame_object=0x20,frame_code=0x28,frame_locals=0x30,frame_previous=0x38,
                function_script=0x8c,function_entry=0x114)
    if mounted:config['schema']=2
    validate_ui_observer_config(config)
    text=ui_observer_ini(config)
    metadata=dict(schema=1,accepted=True,read_only=True,pid=r.pid,sha256=GAME_SHA,
                  acquired_perf_counter_ns=started,finished_perf_counter_ns=time.perf_counter_ns(),
                  assembly_identity_hints=dict(enabled=reuse_assembly_identities,capacity=256,
                      entries=len(assembly_identities),uncached_reads=assembly_uncached_reads),
                  ini_sha256=hashlib.sha256(text.encode('utf-8')).hexdigest(),config=config,
                  native_runtime_observation=False,frame_locals_are_native_arguments=False,
                  limitation='Fresh activation certificate, not a live draw observation. Native must revalidate selected registry slots/names, owners, scripts and links before recording.')
    return config,metadata

def validate_ui_observer_config(c):
    def integer(v,label,maximum,positive=False):
        if type(v)!=int or not int(positive)<=v<maximum:raise UIConfigRejected('Invalid UI integer: '+label)
    if c.get('schema') not in (1,2) or type(c.get('schema'))!=int or c.get('sha256')!=GAME_SHA:
        raise UIConfigRejected('Invalid UI schema/build')
    for key in ('pid','base','objects_header','names_header'):integer(c[key],key,1<<64,True)
    if set(c['layout'])!=set(LAYOUT):raise UIConfigRejected('Invalid UI layout')
    for v in c['layout'].values():integer(v,'layout',0x10000)
    ids=c['identities']
    if not 1<=len(ids)<=96:raise UIConfigRejected('Invalid UI identity bound')
    addresses=set();slots=set()
    for x in ids:
        for key in ('address','cls'):integer(x[key],key,1<<64,True)
        integer(x['outer'],'outer',1<<64)
        for key in ('internal_index','name_index','number'):integer(x[key],key,4000000)
        if not isinstance(x['name'],str) or not x['name'] or len(x['name'].encode('utf-16-le'))>126 or any(ch in x['name'] for ch in '\r\n\0='):
            raise UIConfigRejected('Invalid UI identity name')
        if x['address'] in addresses or x['internal_index'] in slots:raise UIConfigRejected('Duplicate UI identity')
        addresses.add(x['address']);slots.add(x['internal_index'])
    if any(x['cls'] not in addresses for x in ids):raise UIConfigRejected('UI class closure incomplete')
    mounted=c['schema']==2
    if set(c['roles'])!=({'controller','camera','pawn','hud','inventory','canvas_class'}|({'controlled'} if mounted else set())):
        raise UIConfigRejected('Invalid UI roles')
    for index in c['roles'].values():integer(index,'role',len(ids))
    if len(set(c['roles'].values()))!=len(c['roles']):raise UIConfigRejected('Aliased UI roles')
    expected_fields={'controller_pawn','controller_hud','pawn_inventory','player_camera','camera_owner',
                     'hud_owner','pawn_controller','hud_canvas','weapon_instigator'}|{'canvas_'+x for x in CANVAS}
    if mounted:expected_fields|={'controlled_driver','pawn_driven_vehicle','controlled_controller'}
    if set(c['fields'])!=expected_fields or set(c['fields'])!=set(c['field_ids']):raise UIConfigRejected('UI field identities incomplete')
    for label,offset in c['fields'].items():
        integer(offset,label,0x10000);integer(c['field_ids'][label],label,len(ids))
    if not 1<=len(c['supers'])<=96:raise UIConfigRejected('Invalid UI superclass count')
    super_indices=set()
    for item in c['supers']:
        integer(item['identity'],'super identity',len(ids));integer(item['parent'],'super parent',1<<64)
        if item['identity'] in super_indices or (item['parent'] and item['parent'] not in addresses):
            raise UIConfigRejected('Invalid UI superclass closure')
        super_indices.add(item['identity'])
    if len(c['scopes'])!=len(SCOPES):raise UIConfigRejected('Invalid UI scope count')
    for scope,(owner,name,kind) in zip(c['scopes'],SCOPES):
        integer(scope['owner'],'scope owner',len(ids));integer(scope['function'],'scope function',len(ids))
        integer(scope['script'],'script',1<<64,True);integer(scope['size'],'script size',131073,True)
        if type(scope['kind'])!=int or scope['kind']!=kind or ids[scope['owner']]['name']!=owner or ids[scope['function']]['name']!=name or ids[scope['function']]['outer']!=ids[scope['owner']]['address']:
            raise UIConfigRejected('UI scope identity mismatch')
    if set(c['bindings'])!=set(BINDINGS):raise UIConfigRejected('UI bindings missing')
    for label,(rva,entry) in BINDINGS.items():
        if c['bindings'][label]['rva']!=rva or c['bindings'][label]['bytes']!=entry:
            raise UIConfigRejected('UI binding proof mismatch')
        if label!='vm':
            index=c['bindings'][label]['function'];integer(index,'binding function',len(ids))
            expected={'tile':'DrawTile','material':'DrawMaterialTile','text':'DrawText'}[label]
            if ids[index]['name']!=expected or ids[index]['outer']!=ids[c['roles']['canvas_class']]['address']:
                raise UIConfigRejected('UI native binding identity mismatch')
    for key,value in {'frame_node':0x18,'frame_object':0x20,'frame_code':0x28,'frame_locals':0x30,'frame_previous':0x38,'function_script':0x8c,'function_entry':0x114}.items():
        if c[key]!=value:raise UIConfigRejected('UI frame/script layout mismatch')
    return c

def ui_observer_ini(config):
    """Simple bounded flat fields; hexadecimal bytes, decimal identity records."""
    c=validate_ui_observer_config(config);rows=['[UIObserver]',f"schema={c['schema']}"]
    for key in ('pid','base','sha256','objects_header','names_header','frame_node','frame_object','frame_code','frame_locals','frame_previous','function_script','function_entry'):
        rows.append(f'{key}={c[key]}')
    for key,value in c['layout'].items():rows.append(f'{key}={value}')
    rows.append(f"identity_count={len(c['identities'])}")
    for i,item in enumerate(c['identities']):
        for key,value in item.items():rows.append(f'id{i}_{"index" if key=="internal_index" else key}={value}')
    rows.append(f"super_count={len(c['supers'])}")
    for i,item in enumerate(c['supers']):
        for key,value in item.items():rows.append(f'super{i}_{key}={value}')
    for role,index in c['roles'].items():rows.append(f'role_{role}={index}')
    for label,value in c['fields'].items():
        rows.extend((f'{label}={value}',f'{label}_property={c["field_ids"][label]}'))
    rows.append(f"scope_count={len(c['scopes'])}")
    for i,item in enumerate(c['scopes']):
        for key,value in item.items():rows.append(f'scope{i}_{key}={value}')
    for label,item in c['bindings'].items():
        for key,value in item.items():rows.append(f'binding_{label}_{key}={value}')
    return '\n'.join(rows)+'\n'
