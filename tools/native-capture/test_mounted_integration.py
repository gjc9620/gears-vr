"""Mounted integration uses synthetic reflected memory; never opens a process."""
import copy
import struct
import unittest
from unittest import mock
from test_mounted_context import MountedFixture
from anchor_config import build_anchor_config,anchor_ini,LiveRegistryAnchorSampler,AnchorRejected
from ui_observer_config import build_ui_observer_config,ui_observer_ini,SCOPES,CANVAS,BINDINGS,GAME_SHA
from mounted_context import mounted_crosshair_suspension,mounted_crosshair_ini
from world_rebind_runtime import snapshot


class MountedUIFixture(MountedFixture):
    def __init__(self):
        super().__init__();self.pid=123;self.profile['sha256']=GAME_SHA;self.next_address=0xc0000
        self.classes={value['name']:address for address,value in self.identities.items()
                      if self.u64(address+80)==0x1000}
        self.children={}
        for name in ('WarHUD','WarWeapon','Weap_AssaultRifle','Weap_LocustAssaultRifle','Canvas','FloatProperty','IntProperty','Function'):
            if name not in self.classes:self.classes[name]=self.new(name,0x1000,parent=0x8000)
        self.ptr(self.classes['WarWeapon']+96,self.classes['Actor'])
        for label,(name,kind) in CANVAS.items():self.prop('Canvas',name,kind,0x100+len(self.properties)*4)
        for owner,name,offset in (('Controller','myHUD',0x41c),('WarHUD','PlayerOwner',0x230),('WarHUD','Canvas',0x50c),('Actor','Instigator',0x220)):
            self.prop(owner,name,'ObjectProperty',offset)
        self.hud=self.new('HUD_0',self.classes['WarHUD'])
        self.ptr(0x1041c,self.hud);self.ptr(self.hud+0x230,0x10000);self.ptr(self.hud+0x50c,0)
        for index,(owner,name,kind) in enumerate(SCOPES):
            a=self.new(name,self.classes['Function'],outer=self.classes[owner]);self.children.setdefault(self.classes[owner],[]).append(a)
            self.ptr(a+0x114,self.base+BINDINGS['vm'][0]);script=0x500000+index*0x100
            self.put(script,b'\x04\x0b\x53');self.put(a+0x8c,struct.pack('<Qii',script,3,3))
        for key,name in (('tile','DrawTile'),('material','DrawMaterialTile'),('text','DrawText')):
            a=self.new(name,self.classes['Function'],outer=self.classes['Canvas']);self.children.setdefault(self.classes['Canvas'],[]).append(a)
            self.ptr(a+0x114,self.base+BINDINGS[key][0])
        for rva,code in BINDINGS.values():self.put(self.base+rva,bytes.fromhex(code))
        old=self.object_count
        object_bytes=self.read(self.objects_data,old*8);name_bytes=self.read(self.names_data,old*8)
        self.objects_data=0x600000;self.names_data=0x610000
        self.put(self.objects_data,object_bytes);self.put(self.names_data,name_bytes)
        self.put(self.objects_data+old*8,bytes((1101-old)*8));self.put(self.names_data+old*8,bytes((1101-old)*8))
        self.object_count=self.object_capacity=self.name_count=self.name_capacity=1101
        for header,data in ((self.objects_header,self.objects_data),(self.names_header,self.names_data)):
            self.put(header,struct.pack('<Qii',data,1101,1101))

    def new(self,name,cls,outer=0,parent=0):
        a=self.next_address;self.next_address+=0x2000;self.add(a,name,cls,outer,parent);return a

    def prop(self,owner,name,kind,offset):
        a=self.new(name,self.classes[kind],outer=self.classes[owner])
        self.properties[self.classes[owner],name]=dict(address=a,offset=offset,type=kind)
        self.word(a+0x70,1);self.word(a+0x74,8 if kind=='ObjectProperty' else 4);self.word(a+0x94,offset)

    def fields(self,owner):return [self.identity(a) for a in self.children.get(owner,())]

    def resolve(self):
        return dict(super().resolve(),pid=self.pid,module_base=hex(self.base),sha256=GAME_SHA)

    def dismount(self):
        self.ptr(0x10224,0x20000);self.ptr(0x20250,0x10000);self.ptr(0x20810,0);self.ptr(0x30500,0x20000)


class MountedIntegrationTests(unittest.TestCase):
    def test_owner_wire_contract_rejects_legacy_and_wrong_containment(self):
        f=MountedUIFixture();ac,_=build_anchor_config(f)
        text=anchor_ini(ac)
        self.assertIn('schema=4\n',text)
        self.assertIn('mounted_property9_role=',text)
        self.assertIn('mounted_property9_offset=140',text)
        old=copy.deepcopy(ac);del old['mounted']['properties']['Owner']
        with self.assertRaises(AnchorRejected):anchor_ini(old)
        bad=copy.deepcopy(ac)
        bad['identities'][bad['mounted']['role_inventory']]['outer']=0x20000
        with self.assertRaises(AnchorRejected):anchor_ini(bad)

    def test_streamed_levels_ini_ui_cinematic_and_rebind(self):
        import cinematic_anchor
        f=MountedUIFixture().stream_levels()
        with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
            ac,meta=build_anchor_config(f,cinematic_camera=True)
        uc,_=build_ui_observer_config(f,f.resolve(),mounted_anchor=ac)
        text=anchor_ini(ac)
        self.assertIn('schema=4\n',text);self.assertIn('cinematic_enabled=1',text)
        self.assertEqual(uc['schema'],2)
        old=snapshot(ac)
        self.assertEqual(len(set(old.mounted_levels)),5)
        self.assertEqual(old.mounted_levels[0],old.level)
        import world_rebind_runtime as runtime
        adapter=runtime.RuntimePreparation.__new__(runtime.RuntimePreparation)
        adapter.game={'pid':f.pid};adapter.profile=f.profile;f.p=mock.Mock()
        with mock.patch.object(runtime,'Resolver',return_value=f),mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
            self.assertEqual(adapter.observe(lambda:None),old)
        f.p.close.assert_called_once()
        # Actor identities and pointers stay unchanged; a Level registry identity
        # changes and must still trigger a new monitor preparation/commit.
        f.word(f.levels['driver']+76,1)
        current,_=build_anchor_config(f)
        self.assertTrue(snapshot(current).changed_from(old))
        for label in ('controlled','driver','controller','camera','world_info'):
            bad=copy.deepcopy(ac)
            owner=next(item for item in bad['identities'] if item['address']==f.levels[label])
            owner['cls']=0x8000
            with self.assertRaises(AnchorRejected):anchor_ini(bad)
        bad=copy.deepcopy(ac)
        bad['identities']=[item for item in bad['identities'] if item['address']!=f.levels['camera']]
        with self.assertRaises(AnchorRejected):anchor_ini(bad)

    def test_cinematic_mounted_validation_remains_twice_outside_inner_frame(self):
        import cinematic_anchor
        f=MountedUIFixture().stream_levels();events=[];read=f.read
        target=cinematic_anchor.CinematicTargetCertificate.target
        def reading(address,size):
            if (address,size)==(0x30600,4):events.append('stamp')
            return read(address,size)
        def targeting(*args):
            events.append('target');return target(*args)
        with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)), \
             mock.patch.object(cinematic_anchor.CinematicTargetCertificate,'target',targeting), \
             mock.patch.object(f,'read',side_effect=reading):
            sampler=LiveRegistryAnchorSampler(f,cinematic_camera=True)
        self.assertEqual(events,['target','stamp','stamp','stamp','target'])
        self.assertIn('target_proof',sampler.initial)
        # Optional cinematic validation still rejects a change between its
        # independently executed outer endpoints, even with a valid Turret.
        calls=0
        def changed(*args):
            nonlocal calls
            value=target(*args);calls+=1
            if calls==2:value=dict(value,cinematic=not value['cinematic'])
            return value
        with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)), \
             mock.patch.object(cinematic_anchor.CinematicTargetCertificate,'target',changed):
            with self.assertRaises(AnchorRejected):sampler.guard()

    def test_exact_pov_and_human_inventory_roles(self):
        f=MountedUIFixture();ac,meta=build_anchor_config(f)
        uc,ui=build_ui_observer_config(f,f.resolve(),mounted_anchor=ac)
        self.assertEqual(ac['schema'],3);self.assertEqual(uc['schema'],2)
        self.assertEqual(ac['identities'][ac['roles']['pawn']]['address'],f.TURRET)
        self.assertEqual(uc['identities'][uc['roles']['pawn']]['address'],0x20000)
        self.assertEqual(uc['identities'][uc['roles']['controlled']]['address'],f.TURRET)
        self.assertEqual(meta['sample']['candidate_eye_hypothesis'],[100.,200.,300.])
        self.assertEqual(meta['sample']['geometry_source'],'CameraCache.POV')
        self.assertIn('mounted_enabled=1',anchor_ini(ac));self.assertIn('role_controlled=',ui_observer_ini(uc))
        # No sample read depends on either Driver.Location or Turret.Location.
        self.assertNotIn('pawn_location',meta['sample']['raw_values'])

    def test_mount_suspend_dismount_restore_and_monitor(self):
        f=MountedUIFixture();mounted,_=build_anchor_config(f)
        proof=mounted_crosshair_suspension(mounted,f.pid,f.base,GAME_SHA)
        self.assertIn('native_contract=2',mounted_crosshair_ini(proof))
        mounted_snapshot=snapshot(mounted)
        f.dismount()
        # Normal/cinematic restoration retains the normal schema and roles.
        import cinematic_anchor
        with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
            normal,_=build_anchor_config(f,cinematic_camera=True)
        uc,_=build_ui_observer_config(f,f.resolve())
        self.assertEqual(normal['schema'],2);self.assertEqual(uc['schema'],1)
        self.assertNotIn('mounted',normal)
        self.assertTrue(snapshot(normal).changed_from(mounted_snapshot))
        self.assertTrue(mounted_snapshot.changed_from(snapshot(normal)))
        with self.assertRaises((AnchorRejected,KeyError)):mounted_crosshair_suspension(normal,f.pid,f.base,GAME_SHA)

    def test_same_world_new_turret_requires_monitor_rebind(self):
        f=MountedUIFixture();old,_=build_anchor_config(f)
        other=f.new('Turret_1',0x71000,outer=0x50000)
        for address,value in ((0x10224,other),(other+0x250,0x10000),(other+0x800,0x20000),(other+f.WORLD_OFFSET,f.WORLD_INFO),(0x20810,other),(0x30500,other),(f.TURRET+0x250,0)):
            f.ptr(address,value)
        current,_=build_anchor_config(f)
        self.assertEqual(snapshot(current).level,snapshot(old).level)
        self.assertTrue(snapshot(current).changed_from(snapshot(old)))

    def test_requested_assignment_suspends_then_restores_builder(self):
        import start_capture
        import crosshair_assignment_config as assignment
        import cinematic_anchor
        from pathlib import Path
        f=MountedUIFixture();f.p=mock.Mock();camera=f.resolve()
        with mock.patch.object(start_capture,'Resolver',return_value=f),mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)),mock.patch.object(assignment,'build_crosshair_assignment_config',return_value={'normal':True}) as build,mock.patch.object(assignment,'crosshair_assignment_ini',return_value='[CrosshairAssignment]\nnormal=1\n'):
            mounted=start_capture.acquire_observer_preparation(f.pid,f.profile,camera,{},Path('Unused.dll'),anchor_observer=True,ui_observer=True,cinematic_camera=True,crosshair_assignment_observer=True)
            build.assert_not_called()
            self.assertIn('crosshair_assignment_suspended',mounted[3]);self.assertIn('native_contract=2',mounted[4])
            self.assertNotIn('crosshair_assignment',mounted[3])
            f.dismount()
            normal=start_capture.acquire_observer_preparation(f.pid,f.profile,camera,{},Path('Unused.dll'),anchor_observer=True,ui_observer=True,cinematic_camera=True,crosshair_assignment_observer=True)
            build.assert_called_once();self.assertIn('crosshair_assignment',normal[3])
            self.assertNotIn('crosshair_assignment_suspended',normal[3]);self.assertIn('normal=1',normal[4])
        self.assertEqual(f.p.close.call_count,2)

    def test_invalid_mounted_config_and_new_turret_require_new_certificate(self):
        f=MountedUIFixture();ac,_=build_anchor_config(f);s=LiveRegistryAnchorSampler(f)
        bad=copy.deepcopy(ac);bad['mounted']['role_driver']=bad['roles']['pawn']
        with self.assertRaises(AnchorRejected):anchor_ini(bad)
        f.dismount()
        with self.assertRaises(AnchorRejected):s.guard()
        f=MountedUIFixture();s=LiveRegistryAnchorSampler(f)
        # Driver/controller race remains terminal and is never retried as time.
        f.ptr(0x20250,0x10000)
        with self.assertRaises(AnchorRejected):s.guard()


def export_native_fixture(directory):
    from pathlib import Path
    f=MountedUIFixture();ac,_=build_anchor_config(f);uc,_=build_ui_observer_config(f,f.resolve(),mounted_anchor=ac)
    proof=mounted_crosshair_suspension(ac,f.pid,f.base,GAME_SHA)
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=True)
    for name,mounted in (('mounted',True),('streamed',True),('streamed_cinematic',True),('normal',False)):
        if name.startswith('streamed'):
            if name=='streamed':f.stream_levels()
            import cinematic_anchor
            with mock.patch.object(cinematic_anchor,'BOOL_CODE',tuple(f.code)):
                ac,_=build_anchor_config(f,cinematic_camera=name=='streamed_cinematic')
            uc,_=build_ui_observer_config(f,f.resolve(),mounted_anchor=ac)
        if not mounted:
            # Return to the ordinary single-Level fixture for dismount coverage.
            f=MountedUIFixture()
            f.dismount();ac,_=build_anchor_config(f);uc,_=build_ui_observer_config(f,f.resolve())
        text=anchor_ini(ac)+ui_observer_ini(uc)+(mounted_crosshair_ini(proof) if mounted else '')
        (directory/(name+'.ini')).write_text(text,encoding='utf-16')
        with (directory/(name+'.memory')).open('wb') as out:
            for address,value in sorted(f.memory.items()):out.write(struct.pack('<QB',address,value))


if __name__=='__main__':
    import sys
    if len(sys.argv)==3 and sys.argv[1]=='--export-native':export_native_fixture(sys.argv[2])
    else:unittest.main()
