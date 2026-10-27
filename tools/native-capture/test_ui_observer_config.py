"""Independent mocked memory certificates; never opens a game or XR runtime."""
import copy
import pathlib
import struct
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'package-inspector'))
from resolve_camera import _DiscoveryHintPages
from ui_observer_config import (GAME_SHA,SCOPES,BINDINGS,CANVAS,UIConfigRejected,
                               build_ui_observer_config,ui_observer_ini)

class Fixture:
    def __init__(self):
        self.pid=123;self.base=0x140000000;self.memory={};self.objects=[];self.names={};self.classes={};self.children={};self.props={}
        self.objects_header=0x2000;self.names_header=0x3000;self.od=0x100000;self.nd=0x200000
        self.profile=dict(sha256=GAME_SHA,object_internal_index=8,object_outer=64,object_name=72,
                          object_class=80,field_super=96,field_next=104,struct_children=128,
                          property_offset_internal=148,name_entry_index=0,name_entry_text=20)
        meta=self.obj('Class',0);self.classes['Class']=meta;self.put(meta+80,struct.pack('<Q',meta))
        root=self.cls('Object')
        for name in ('Function','FloatProperty','IntProperty','ObjectProperty','Actor','Pawn','WarPawn',
                     'Pawn_COGMarcus','PlayerController','WarPC','Camera','WarPlayerCamera',
                     'WarHUD','WarWeapon','Weap_AssaultRifle','Weap_LocustAssaultRifle','WarInventoryManager','Canvas'):
            self.cls(name,root)
        for name,parent in (('Pawn','Actor'),('WarPawn','Pawn'),('Pawn_COGMarcus','WarPawn'),
                            ('WarPC','PlayerController'),('WarPlayerCamera','Camera'),
                            ('WarWeapon','Actor'),('Weap_AssaultRifle','WarWeapon'),('Weap_LocustAssaultRifle','WarWeapon')):
            self.put(self.classes[name]+96,struct.pack('<Q',self.classes[parent]))
        for label,(name,kind) in CANVAS.items():self.prop('Canvas',name,kind,0x70+len(self.props)*4)
        for owner,name,offset in (('PlayerController','Pawn',0x224),('PlayerController','myHUD',0x41c),
                                  ('PlayerController','PlayerCamera',0x37c),('Camera','PCOwner',0x224),
                                  ('WarHUD','PlayerOwner',0x230),('WarHUD','Canvas',0x50c),
                                  ('Pawn','Controller',0x230),('Pawn','InvManager',0x434),('Actor','Instigator',0x220)):
            self.prop(owner,name,'ObjectProperty',offset)
        self.function_addresses={}
        for i,(owner,name,kind) in enumerate(SCOPES):
            address=self.function(owner,name,BINDINGS['vm'][0]);script=0x800000+i*0x100
            self.put(script,b'\x04\x0b\x53');self.put(address+0x8c,struct.pack('<Qii',script,3,3))
        for label,name in (('tile','DrawTile'),('material','DrawMaterialTile'),('text','DrawText')):
            self.function('Canvas',name,BINDINGS[label][0])
        for rva,code in BINDINGS.values():self.put(self.base+rva,bytes.fromhex(code))
        self.pc=self.obj('WarPC',self.classes['WarPC']);self.cam=self.obj('WarPlayerCamera',self.classes['WarPlayerCamera'])
        self.pawn=self.obj('Pawn_COGMarcus',self.classes['Pawn_COGMarcus']);self.hud=self.obj('WarHUD',self.classes['WarHUD'])
        self.inventory=self.obj('WarInventoryManager',self.classes['WarInventoryManager'])
        for address,offset,target in ((self.pc,0x224,self.pawn),(self.pc,0x41c,self.hud),(self.pc,0x37c,self.cam),
                                      (self.cam,0x224,self.pc),(self.pawn,0x230,self.pc),(self.pawn,0x434,self.inventory),
                                      (self.hud,0x230,self.pc)):
            self.put(address+offset,struct.pack('<Q',target))
        self.put(self.hud+0x50c,bytes(8)) # Outside rendering, null is expected.
        self.finish_registry()
        self.camera=dict(pid=self.pid,module_base=hex(self.base),sha256=GAME_SHA,
                         controller_address=hex(self.pc),camera_address=hex(self.cam))
    def put(self,address,data):
        for i,b in enumerate(data):self.memory[address+i]=b
    def read(self,address,size):
        try:return bytes(self.memory[address+i] for i in range(size))
        except KeyError:raise RuntimeError('Unmapped fixture read')
    def u32(self,address):return struct.unpack('<I',self.read(address,4))[0]
    def u64(self,address):return struct.unpack('<Q',self.read(address,8))[0]
    def obj(self,name,cls,outer=0):
        address=0x50000+len(self.objects)*0x1000;index=len(self.objects)
        self.put(address,bytes(0x140))
        if name not in self.names:self.names[name]=len(self.names)
        ni=self.names[name];self.put(address+8,struct.pack('<I',index));self.put(address+64,struct.pack('<Q',outer))
        self.put(address+72,struct.pack('<II',ni,0));self.put(address+80,struct.pack('<Q',cls))
        self.objects.append(address);return address
    def cls(self,name,parent=0):
        address=self.obj(name,self.classes['Class']);self.classes[name]=address
        self.put(address+96,struct.pack('<Q',parent));return address
    def prop(self,owner,name,kind,offset):
        address=self.obj(name,self.classes[kind],self.classes[owner]);self.put(address+148,struct.pack('<I',offset))
        self.children.setdefault(self.classes[owner],[]).append(address)
        self.props[self.classes[owner],name]=dict(address=address,offset=offset,type=kind)
    def function(self,owner,name,rva):
        address=self.obj(name,self.classes['Function'],self.classes[owner]);self.put(address+0x114,struct.pack('<Q',self.base+rva))
        self.children.setdefault(self.classes[owner],[]).append(address);self.function_addresses[owner,name]=address
        return address
    def finish_registry(self):
        self.put(self.objects_header,struct.pack('<Qii',self.od,1101,1101));self.put(self.names_header,struct.pack('<Qii',self.nd,1101,1101))
        self.put(self.od,struct.pack('<'+'Q'*1101,*(self.objects+[0]*(1101-len(self.objects)))))
        self.put(self.nd,bytes(1101*8))
        for name,ni in self.names.items():
            address=0x300000+ni*0x100;self.put(address,bytes(148));self.put(address,struct.pack('<I',ni))
            self.put(address+20,name.encode('utf-16-le')+b'\0\0');self.put(self.nd+ni*8,struct.pack('<Q',address))
    def identity(self,address,*,reader=None):
        raw=(reader or self.read)(address,0x60);ni=struct.unpack_from('<I',raw,72)[0]
        name=next(n for n,i in self.names.items() if i==ni)
        return dict(address=address,internal_index=struct.unpack_from('<I',raw,8)[0],name=name,
                    number=struct.unpack_from('<I',raw,76)[0],cls=struct.unpack_from('<Q',raw,80)[0],
                    outer=struct.unpack_from('<Q',raw,64)[0])
    def discovery_identity_reader(self):
        pages=_DiscoveryHintPages(self.read)
        return lambda address:self.identity(address,reader=pages.read)
    def fields(self,owner):
        return (self.identity(a) for a in self.children.get(owner,[]))
    def property(self,owner,name,inherited=False):
        seen=set()
        while owner and owner not in seen:
            seen.add(owner)
            if (owner,name) in self.props:return self.props[owner,name]
            if not inherited:break
            owner=self.u64(owner+96)
        raise RuntimeError('Fixture property not found')

class UIObserverConfigTests(unittest.TestCase):
    def test_false_discovery_class_hint_cannot_authorize_identity(self):
        r=Fixture();ordinary=r.identity
        def discover(address):
            item=ordinary(address)
            if address==r.classes['Canvas']:item=dict(item,number=0)
            return item
        r.discovery_identity_reader=lambda:discover
        r.put(r.classes['Canvas']+76,struct.pack('<I',1))
        with self.assertRaisesRegex(UIConfigRejected,'Required UI owner class missing'):
            build_ui_observer_config(r,r.camera)
    def test_duplicate_owner_class_rejected_with_discovery_batching(self):
        r=Fixture();r.obj('Canvas',r.classes['Class']);r.finish_registry()
        with self.assertRaisesRegex(UIConfigRejected,'Duplicate UI owner class'):
            build_ui_observer_config(r,r.camera)
    def test_discovery_reader_never_supplies_fresh_certificate_reads(self):
        r=Fixture();ordinary=r.identity;hints=[];fresh=[];read=r.read
        def discover(address):
            hints.append(address);return ordinary(address)
        r.discovery_identity_reader=lambda:discover
        def observe(address,size):
            if size==0x60:fresh.append(address)
            return read(address,size)
        r.read=observe
        config,_=build_ui_observer_config(r,r.camera)
        self.assertEqual(hints,r.objects)
        for item in config['identities']:
            self.assertGreaterEqual(fresh.count(item['address']),2)
    def test_success_scope_identity_script_and_flat_ini(self):
        r=Fixture();config,metadata=build_ui_observer_config(r,r.camera);text=ui_observer_ini(config)
        self.assertIn('[UIObserver]\nschema=1\n',text);self.assertIn('scope_count=7',text)
        self.assertTrue(metadata['accepted']);self.assertTrue(metadata['read_only'])
        self.assertFalse(metadata['frame_locals_are_native_arguments']);self.assertFalse(metadata['native_runtime_observation'])
        self.assertEqual(config['fields']['controller_hud'],0x41c)
        self.assertEqual(config['layout']['property_offset_internal'],148)
        self.assertNotIn('role_weapon=',text);self.assertIn('role_canvas_class=',text)
        self.assertNotIn('objects_data=',text)
        self.assertEqual(config['frame_node'],0x18);self.assertEqual(config['frame_code'],0x28)
        self.assertEqual(config['frame_previous'],0x38)
        self.assertIn('frame_previous=56\n',ui_observer_ini(config))
        self.assertEqual(len({s['function'] for s in config['scopes']}),7)
        for scope,(owner,name,kind) in zip(config['scopes'],SCOPES):
            self.assertEqual(config['identities'][scope['owner']]['name'],owner)
            self.assertEqual(config['identities'][scope['function']]['name'],name)
            self.assertEqual(scope['size'],3);self.assertEqual(scope['kind'],kind)
    def test_every_binding_is_exact_sixteen_bytes(self):
        self.assertEqual({len(bytes.fromhex(code)) for _,code in BINDINGS.values()},{16})
    def test_wrong_process_build_or_camera_base_rejected(self):
        for key,value in (('pid',999),('sha256','bad'),('module_base','0x1')):
            r=Fixture();r.camera[key]=value
            with self.subTest(key=key),self.assertRaises(UIConfigRejected):build_ui_observer_config(r,r.camera)
    def test_native_entry_or_dispatch_changed_rejected(self):
        for mutation in (lambda r:r.put(r.base+BINDINGS['tile'][0],b'\xcc'),
                         lambda r:r.put(r.function_addresses['WarHUD','DrawWeaponCrosshair']+0x114,struct.pack('<Q',r.base+1))):
            r=Fixture();mutation(r)
            with self.assertRaises(UIConfigRejected):build_ui_observer_config(r,r.camera)
    def test_exact_function_outer_and_registry_slot(self):
        for mutation in (lambda r:r.put(r.function_addresses['WarHUD','DrawWeaponCrosshair']+64,struct.pack('<Q',r.classes['WarWeapon'])),
                         lambda r:r.put(r.od+r.objects.index(r.hud)*8,bytes(8)),
                         lambda r:r.put(0x300000+r.names['Canvas']*0x100,struct.pack('<I',999))):
            r=Fixture();mutation(r)
            with self.assertRaises(UIConfigRejected):build_ui_observer_config(r,r.camera)
    def test_wrong_reflected_property_type_or_outer(self):
        for mutation in (lambda r:r.put(r.props[r.classes['Canvas'],'ClipX']['address']+80,struct.pack('<Q',r.classes['IntProperty'])),
                         lambda r:r.put(r.props[r.classes['Canvas'],'ClipX']['address']+64,struct.pack('<Q',r.classes['WarHUD']))):
            r=Fixture();mutation(r)
            with self.assertRaises(UIConfigRejected):build_ui_observer_config(r,r.camera)
    def test_broken_hud_or_pawn_relationship(self):
        for address_name,offset in (('hud',0x230),('pawn',0x230),('pc',0x41c)):
            r=Fixture();r.put(getattr(r,address_name)+offset,bytes(8))
            with self.assertRaises(UIConfigRejected):build_ui_observer_config(r,r.camera)
    def test_mutable_nonidentity_snapshot_bytes_allowed(self):
        r=Fixture();original=r.read;hits=0
        def read(address,size):
            nonlocal hits
            raw=original(address,size)
            if address==r.pawn and size==0x60:
                hits+=1;raw=bytearray(raw)
                for offset in (0x18,0x58):raw[offset]^=hits&1
                return bytes(raw)
            return raw
        r.read=read
        config,_=build_ui_observer_config(r,r.camera)
        self.assertGreater(hits,2)
        self.assertEqual(config['identities'][config['roles']['pawn']]['address'],r.pawn)
    def test_each_identity_field_changed_between_snapshots_rejected(self):
        for offset in (8,64,72,76,80):
            with self.subTest(offset=offset):
                r=Fixture();original=r.read;hits=0
                def read(address,size):
                    nonlocal hits
                    raw=original(address,size)
                    if address==r.pawn and size==0x60:
                        hits+=1;raw=bytearray(raw);raw[offset]^=hits&1
                        return bytes(raw)
                    return raw
                r.read=read
                with self.assertRaisesRegex(UIConfigRejected,'identity read changed'):
                    build_ui_observer_config(r,r.camera)
    def test_registry_slot_changed_after_identity_snapshots_rejected(self):
        r=Fixture();original=r.read;hits=0
        def read(address,size):
            nonlocal hits
            raw=original(address,size)
            if address==r.pawn and size==0x60:
                hits+=1
                # First read is the discovery hint; the next two are fresh().
                if hits==3:r.put(r.od+r.objects.index(r.pawn)*8,bytes(8))
            return raw
        r.read=read
        with self.assertRaisesRegex(UIConfigRejected,'registry slot'):
            build_ui_observer_config(r,r.camera)
    def test_role_reference_changed_after_capture_rejected(self):
        r=Fixture();original=r.read;link=r.pc+0x41c;hits=0
        def read(address,size):
            nonlocal hits
            raw=original(address,size)
            if address==link and size==8:
                hits+=1
                if hits==1:r.put(link,struct.pack('<Q',r.inventory))
            return raw
        r.read=read
        with self.assertRaisesRegex(UIConfigRejected,'field/link/entry changed'):
            build_ui_observer_config(r,r.camera)
    def test_empty_oversized_and_unreadable_script_rejected(self):
        for header in (struct.pack('<Qii',0,3,3),struct.pack('<Qii',0x800000,131073,131073),struct.pack('<Qii',0x900000,3,3)):
            r=Fixture();r.put(r.function_addresses['WarHUD','DrawWeaponCrosshair']+0x8c,header)
            with self.assertRaises((UIConfigRejected,RuntimeError)):build_ui_observer_config(r,r.camera)
    def test_valid_registry_growth_revalidated_at_final_boundary(self):
        r=Fixture();original=r.read;hits=0
        def read(address,size):
            nonlocal hits
            if address==r.objects_header:
                hits+=1
                if hits==3:r.put(address,struct.pack('<Qii',r.od,1102,1102))
            return original(address,size)
        r.read=read
        config,_=build_ui_observer_config(r,r.camera)
        self.assertEqual(config['objects_header'],r.objects_header)
    def test_registry_reallocation_with_changed_selected_slot_rejected(self):
        r=Fixture();original=r.read;hits=0
        r.put(0x900000,r.read(r.od,1101*8));r.put(0x900000+r.objects.index(r.hud)*8,bytes(8))
        def read(address,size):
            nonlocal hits
            if address==r.objects_header:
                hits+=1
                if hits==3:r.put(address,struct.pack('<Qii',0x900000,1101,1101))
            return original(address,size)
        r.read=read
        with self.assertRaisesRegex(UIConfigRejected,'registry slot'):build_ui_observer_config(r,r.camera)
    def test_supers_are_certified_and_cycle_rejected(self):
        r=Fixture();c,_=build_ui_observer_config(r,r.camera)
        table={c['identities'][s['identity']]['name']:s['parent'] for s in c['supers']}
        self.assertEqual(table['Weap_AssaultRifle'],r.classes['WarWeapon'])
        r=Fixture();r.put(r.classes['Canvas']+96,struct.pack('<Q',r.classes['Canvas']))
        with self.assertRaises(UIConfigRejected):build_ui_observer_config(r,r.camera)
    def test_serialization_rejects_newline_scope_outer_and_missing_fields(self):
        r=Fixture();config,_=build_ui_observer_config(r,r.camera)
        for mutation in (lambda c:c['identities'][0].update(name='Bad\nschema=0'),
                         lambda c:c['identities'][c['scopes'][0]['function']].update(outer=1),
                         lambda c:c['fields'].pop('canvas_clip_x')):
            c=copy.deepcopy(config);mutation(c)
            with self.assertRaises(UIConfigRejected):ui_observer_ini(c)

if __name__=='__main__':unittest.main()
