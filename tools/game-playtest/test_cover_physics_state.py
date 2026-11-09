"""Actual typed source reader; synthetic memory/attestation seams, no game."""
import copy
import hashlib
import json
from pathlib import Path
import struct
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import cover_physics_state as physics
from cover_state import CoverQuery
from test_cover_surface_query import SurfaceMemory


class PhysicsMemory(SurfaceMemory):
    PA_CLASS, PI_CLASS, BI_CLASS, BS_CLASS, POINTER = range(0x3800000, 0x3805000, 0x1000)
    PA, PI = 0x3900000, 0x3901000
    BODY_TABLE, SETUP_TABLE = 0x4000000, 0x4100000
    VTABLE = 0x5100000

    def set_value(self, obj, name, value, fmt):
        values=value if isinstance(value,tuple) else (value,)
        self.put(self.value_address(obj,name),struct.pack(fmt,*values))

    def __init__(self, count=3):
        super().__init__()
        # This inherited native BoneAtom inner is not in properties.values();
        # migrate its synthetic B0 type link to the fixture's retail D0 too.
        space=self.properties[self.MESH_CLASS,'SpaceBases']
        inner=struct.unpack('<Q',self.read(space['address']+0xd0,8))[0]
        self.put(inner+0xd0,self.read(inner+0xb0,8))
        for address, name in [(self.PA_CLASS, 'PhysicsAsset'), (self.PI_CLASS, 'PhysicsAssetInstance'),
                              (self.BI_CLASS, 'RB_BodyInstance'), (self.BS_CLASS, 'RB_BodySetup')]:
            self.add_class(address, name, 0x20400)
        self.add_object(self.POINTER, 'Pointer', self.STRUCT_CLASS)
        self.put(self.POINTER + 0x88, struct.pack('<I', 8))
        self.add_object(self.PA, 'CurrentPhysics', self.PA_CLASS)
        self.add_object(self.PI, 'CurrentInstance', self.PI_CLASS, outer=self.PAWN)
        self.add_property(self.MESH_CLASS, 'PhysicsAsset', 'ObjectProperty', 0x248, 8, target=self.PA_CLASS)
        self.add_property(self.MESH_CLASS, 'PhysicsAssetInstance', 'ObjectProperty', 0x250, 8, target=self.PI_CLASS)
        self.add_property(self.PI_CLASS, 'Owner', 'ObjectProperty', 0x64, 8, target=self.PAWN_CLASS)
        self.add_property(self.PI_CLASS, 'RootBodyIndex', 'IntProperty', 0x60, 4)
        self.add_property(self.BI_CLASS, 'BodyIndex', 'IntProperty', 0x60, 4)
        self.add_property(self.BI_CLASS, 'OwnerComponent', 'ComponentProperty', 0x68, 8, target=self.MESH_CLASS)
        self.add_property(self.BI_CLASS, 'BodyData', 'StructProperty', 0x88, 8, target=self.POINTER)
        self.add_property(self.BI_CLASS, 'BoneSpring', 'StructProperty', 0x90, 8, target=self.POINTER)
        self.add_property(self.BS_CLASS, 'BoneName', 'NameProperty', 0xb4, 8)
        self.set_value(self.MESH, 'PhysicsAsset', self.PA, '<Q')
        self.set_value(self.MESH, 'PhysicsAssetInstance', self.PI, '<Q')
        self.set_value(self.PI, 'Owner', self.PAWN, '<Q')
        self.set_value(self.PI, 'RootBodyIndex', 0, '<i')
        self.bodies, self.setups = [], []
        for i in range(count):
            body, setup, receiver = 0x4200000 + i*0x1000, 0x4300000 + i*0x1000, 0x5000000 + i*0x100
            self.add_object(body, 'CurrentBody', self.BI_CLASS, outer=self.PI)
            self.add_object(setup, 'CurrentSetup', self.BS_CLASS, outer=self.PA)
            self.set_value(body, 'BodyIndex', i, '<i')
            self.set_value(body, 'OwnerComponent', self.MESH, '<Q')
            self.set_value(body, 'BodyData', receiver, '<Q')
            self.set_value(body, 'BoneSpring', 0, '<Q')
            self.set_value(setup, 'BoneName', (i%3+1, 0), '<II')
            self.put(receiver, struct.pack('<Q', self.VTABLE))
            self.bodies.append(body);self.setups.append(setup)
        self.make_physics_array(self.PI_CLASS, self.PI, 'Bodies', 0x6c, self.BI_CLASS, self.BODY_TABLE, self.bodies)
        self.make_physics_array(self.PA_CLASS, self.PA, 'BodySetup', 0x68, self.BS_CLASS, self.SETUP_TABLE, self.setups)
        for i, offset in enumerate((40, 104, 728)):
            target=self.BASE+0x3000+i*0x100
            self.put(self.VTABLE+offset, struct.pack('<Q', target))
            self.put(target, bytes([0x90+i])*32)
        for p in self.properties.values():self.objects[p['address']]['cls']=self.kind_classes[p['type']]
        for owner in {owner for owner, _ in self.properties}:
            addresses=self.functions.get(owner,[])+[p['address'] for (declaring,_),p in self.properties.items() if declaring==owner]
            self.put(owner+0x80,struct.pack('<Q',addresses[0]))
            for current,nxt in zip(addresses,addresses[1:]+[0]):self.put(current+0x68,struct.pack('<Q',nxt))
        self.chunks=[]
        for i,(rva,size,_) in enumerate(physics.PHYSICS_CHUNKS):
            raw=bytes([0x61+i])*size;self.put(self.BASE+rva,raw)
            self.chunks.append((rva,raw,hashlib.sha256(raw).hexdigest()))
        self.original=self.chunks[0][1][:32]
        self.patched=bytes.fromhex('e901020304')+self.original[5:]
        self.put(self.BASE+0xf26d4,self.patched)
        self.proofs=[];self.proof_hook=None

    def make_physics_array(self, cls, obj, name, offset, target, data, members):
        self.add_property(cls,name,'ArrayProperty',offset,16)
        field=self.properties[cls,name];inner=field['address']+0x100
        self.add_object(inner,name,self.kind_classes['ObjectProperty'],outer=field['address'])
        self.put(inner+0x70,struct.pack('<II',1,8));self.put(inner+0x94,bytes(4))
        self.put(inner+0xd0,struct.pack('<Q',target));self.put(field['address']+0xd0,struct.pack('<Q',inner))
        self.set_value(obj,name,(data,len(members),len(members)),'<Qii')
        self.put(data,struct.pack('<'+'Q'*len(members),*members))

    def attestation(self, manifest, **options):
        m=self
        class Proof:
            def __init__(self):
                self.module_base=m.BASE;self.by_target={m.BASE+0xf26d4:True};self.closed=False;self.checks=0
            def read_entry(self,address,size,actual_read):
                assert address==m.BASE+0xf26d4 and size==32
                self.reverify();return m.original
            def reverify(self):
                self.checks+=1
                if m.proof_hook:m.proof_hook(self)
                if m.read(m.BASE+0xf26d4,32)!=m.patched:raise RuntimeError('Host sealed hook changed')
            def close(self):self.closed=True
        assert manifest.name=='hook-entry-manifest.json'
        assert options['pid']==self.observer.identities['game_identity']['pid']
        assert options['game_creation']==self.observer.identities['game_identity']['creation_time']
        assert options['owner_identity']==self.observer.identities['owner_identity']
        assert options['expected_dll_sha256']==self.DLL_SHA
        proof=Proof();self.proofs.append(proof);return proof

    def capture_physics(self, **options):
        with patch.object(physics.surface,'_disk_module',return_value=copy.deepcopy(self.module)):
            return physics.capture(self.observer,Path('owned/Native.dll'),self.DLL_SHA,
                attestation_factory=self.attestation,historical_loader=lambda:copy.deepcopy(self.chunks),**options)


class PhysicsSourceTests(unittest.TestCase):
    def result(self, fixture, accepted=True, **options):
        before=copy.deepcopy(fixture.memory);row=fixture.capture_physics(**options)
        self.assertIs(row['accepted'],accepted,row.get('rejection'))
        for key in ['engine_calls','body_modified','activation_allowed','collision_verified','physical_pose_verified',
                    'hit_geometry_verified','active_physics_verified','world_synchronization_verified',
                    'update_coverage_verified','matched_bones_verified','virtual_target_module_verified',
                    'virtual_target_executable_verified',
                    'atomic_snapshot_verified','physical_headset_acceptance']:
            self.assertIs(row[key],False)
        self.assertEqual(row['queue_submissions'],0);self.assertEqual(fixture.bridges,[])
        self.assertTrue(all(p.closed for p in fixture.proofs))
        if fixture.before_read is None and fixture.at_verify is None:self.assertEqual(fixture.memory,before)
        json.dumps(row,allow_nan=False)
        return row

    def test_exact_current_sources_and_opaque_receivers_close_without_physics_permission(self):
        f=PhysicsMemory();row=self.result(f)
        self.assertEqual(len(row['bodies']),3);self.assertEqual(row['unique_main_receivers'],3)
        self.assertEqual([r['bone_name']['name'] for r in row['bodies']],['root','spine','head'])
        self.assertEqual(len(row['virtual_target_prefixes']),3)
        self.assertTrue(row['code_chunks'][0]['sealed_entry_used'])
        self.assertTrue(all(not r['auxiliary']['present'] for r in row['bodies']))
        self.assertTrue(all(not r['main']['methods_called'] for r in row['bodies']))
        self.assertLess(row['reads'],16384);self.assertLess(row['bytes'],2*1024*1024)

    def test_current_pawn_or_mesh_owner_is_exact_but_foreign_owner_and_body_index_are_rejected(self):
        f=PhysicsMemory();f.set_value(f.PI,'Owner',f.MESH,'<Q');self.result(f)
        for defect in ['instance_owner','body_owner','body_index','root_index','mesh_asset_null']:
            f=PhysicsMemory()
            if defect=='instance_owner':f.set_value(f.PI,'Owner',f.LINK,'<Q')
            elif defect=='body_owner':f.set_value(f.bodies[1],'OwnerComponent',f.PAWN,'<Q')
            elif defect=='body_index':f.set_value(f.bodies[1],'BodyIndex',0,'<i')
            elif defect=='root_index':f.set_value(f.PI,'RootBodyIndex',3,'<i')
            else:f.set_value(f.MESH,'PhysicsAssetInstance',0,'<Q')
            self.result(f,False)

    def test_array_schema_membership_size_and_native_offsets_are_not_guessed(self):
        for defect in ['array_offset','stride','inner_type','inner_target','null','duplicate','count','too_many']:
            f=PhysicsMemory();p=f.properties[f.PI_CLASS,'Bodies'];inner=p['address']+0x100
            if defect=='array_offset':p['offset']=0x70;f.put(p['address']+0x94,struct.pack('<I',0x70))
            elif defect=='stride':f.put(inner+0x74,struct.pack('<I',16))
            elif defect=='inner_type':f.objects[inner]['cls']=f.kind_classes['ComponentProperty']
            elif defect=='inner_target':f.put(inner+0xd0,struct.pack('<Q',f.BS_CLASS))
            elif defect=='null':f.put(f.BODY_TABLE,bytes(8))
            elif defect=='duplicate':f.put(f.BODY_TABLE+8,struct.pack('<Q',f.bodies[0]))
            elif defect=='count':f.set_value(f.PI,'Bodies',(f.BODY_TABLE,2,3),'<Qii')
            else:f.set_value(f.PI,'Bodies',(f.BODY_TABLE,65,65),'<Qii')
            self.result(f,False)

    def test_pointer_schema_and_current_receiver_bytes_are_checked(self):
        for defect in ['offset','structure','extent','unaligned','vtable','unreadable']:
            f=PhysicsMemory();p=f.properties[f.BI_CLASS,'BodyData']
            if defect=='offset':p['offset']=0x80;f.put(p['address']+0x94,struct.pack('<I',0x80))
            elif defect=='structure':f.objects[f.POINTER]['name']='UnrelatedPointer'
            elif defect=='extent':f.put(f.POINTER+0x88,struct.pack('<I',16))
            elif defect=='unaligned':f.set_value(f.bodies[0],'BodyData',0x5000001,'<Q')
            elif defect=='vtable':f.put(0x5000000,bytes(8))
            else:f.set_value(f.bodies[0],'BodyData',0x6000000,'<Q')
            self.result(f,False)

    def test_absent_or_aliased_receivers_never_become_distinct_active_body_proof(self):
        f=PhysicsMemory()
        for body in f.bodies:f.set_value(body,'BodyData',0,'<Q')
        row=self.result(f);self.assertEqual(row['unique_main_receivers'],0)
        f=PhysicsMemory()
        for body in f.bodies:f.set_value(body,'BodyData',0x5000000,'<Q')
        row=self.result(f);self.assertEqual(len(row['bodies']),3);self.assertEqual(row['unique_main_receivers'],1)

    def test_full_code_and_live_hook_are_verified_instead_of_skipping_prefixes(self):
        for defect in ['tail','cold_chunk','manifest','module']:
            f=PhysicsMemory()
            if defect=='tail':f.put(f.BASE+0xf26d4+33,b'\0')
            elif defect=='cold_chunk':f.put(f.BASE+0x8a018a,b'\0')
            elif defect=='manifest':
                def fail(proof):raise RuntimeError('Host manifest changed')
                f.proof_hook=fail
            else:
                def wrong(proof):proof.module_base+=4096
                f.proof_hook=wrong
            if defect=='module':
                original=f.attestation
                def factory(*args,**kwargs):
                    result=original(*args,**kwargs);result.module_base+=4096;return result
                f.attestation=factory
            self.result(f,False)

    def test_name_number_and_exact_registry_text_are_retained(self):
        f=PhysicsMemory();f.set_value(f.setups[0],'BoneName',(1,2),'<II');row=self.result(f)
        self.assertEqual(row['bodies'][0]['bone_name']['number'],2)
        f=PhysicsMemory();entry=0x160000+0x100;f.put(entry+20,'wrong\0'.encode('utf-16-le'));self.result(f,False)

    def test_closing_array_owner_name_receiver_and_target_changes_are_rejected(self):
        for defect in ['array','owner','name','receiver','target','code']:
            f=PhysicsMemory()
            def change(call):
                if call!=2:return
                if defect=='array':f.put(f.BODY_TABLE+8,struct.pack('<Q',f.bodies[0]))
                elif defect=='owner':f.set_value(f.bodies[0],'OwnerComponent',f.PAWN,'<Q')
                elif defect=='name':f.set_value(f.setups[0],'BoneName',(2,0),'<II')
                elif defect=='receiver':f.set_value(f.bodies[0],'BodyData',0,'<Q')
                elif defect=='target':f.put(f.VTABLE+40,struct.pack('<Q',f.BASE+0x3100))
                else:f.put(f.BASE+0x3000,b'\0')
            f.at_verify=change;self.result(f,False)

    def test_closing_registry_witness_and_original_budget_survive_failure(self):
        f=PhysicsMemory();queries=[]
        def factory(observer):q=CoverQuery(observer);queries.append(q);return q
        def change(call):
            if call==2:f.put(f.names_header+8,struct.pack('<i',101))
        f.at_verify=change;row=self.result(f,False,query_factory=factory)
        self.assertEqual(row['closing_rejection']['kind'],'registry')
        self.assertEqual(row['closing_rejection'],queries[0].closing_rejection)
        f=PhysicsMemory()
        def exhausted(observer):q=CoverQuery(observer);q.reads=16384;return q
        row=self.result(f,False,query_factory=exhausted)
        self.assertEqual(row['reads'],16385)

    def test_full_declared_body_cap_fits_the_original_read_budget_without_truncation(self):
        f=PhysicsMemory(64);row=self.result(f)
        self.assertEqual(len(row['bodies']),64)
        self.assertEqual(row['unique_main_receivers'],64)
        self.assertLess(row['reads'],16384)
        self.assertLess(row['bytes'],2*1024*1024)

    def test_final_hook_attestation_failure_does_not_accept_already_read_bodies(self):
        f=PhysicsMemory()
        def fail_at_end(proof):
            if proof.checks==2:raise RuntimeError('Host final Native envelope changed')
        f.proof_hook=fail_at_end;row=self.result(f,False)
        self.assertEqual(len(row['bodies']),3)
        self.assertIn('final Native envelope changed',row['rejection'])

    def test_current_cover_is_required_and_closing_cover_changes_reject_source_rows(self):
        f=PhysicsMemory();f.set_value(f.PAWN,'CoverType',0,'<B')
        row=self.result(f,False)
        self.assertIn('nonzero CoverType',row['rejection'])
        for name, value, fmt in [('CoverType',0,'<B'),('CoverAction',99,'<B'),
                                 ('CurrentLink',0,'<Q'),('CurrentSlotIdx',0,'<i')]:
            with self.subTest(field=name):
                f=PhysicsMemory()
                def change(call):
                    if call==2:f.set_value(f.PAWN,name,value,fmt)
                f.at_verify=change
                self.result(f,False)


if __name__=='__main__':unittest.main()
