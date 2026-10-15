"""Synthetic memory only; no process, game or XR is opened."""
import copy
import struct
import unittest
from unittest import mock
import sample_player_anchor as anchor


class MemoryResolver:
    def __init__(self,view_target=True):
        self.profile={'object_internal_index':8,'object_name':72,'object_class':80,'object_outer':64,'field_super':96}
        self.memory={};self.identities={};self.properties={};self.names={};self.timestamp_reads=0
        self.timestamp_changes=False;self.pawn_changes=False
        items=[(0x1000,'Class',0),(0x2000,'WarPawn',0x3000),(0x3000,'Pawn',0x4000),
               (0x4000,'Actor',0x8000),(0x5000,'WarPC',0x6000),(0x6000,'Controller',0x4000),
               (0x7000,'Level',0x8000),(0x8000,'Object',0),(0x9000,'WarPlayerCamera',0x4000),
               (0xa000,'WarLocalPlayer',0x8000),(0xc000,'TViewTarget',0),(0xd000,'Vector',0),(0xe000,'Rotator',0),
               (0x10000,'WarPC',0),(0x20000,'Pawn_COGMarcus',0),(0x30000,'WarPlayerCamera',0),
               (0x40000,'WarLocalPlayer',0),(0x50000,'PersistentLevel',0)]
        self.objects_header=0x200000;self.objects_data=0x201000;self.object_count=self.object_capacity=len(items)
        self.names_header=0x202000;self.names_data=0x203000;self.name_count=self.name_capacity=len(items)
        self.put(self.objects_header,struct.pack('<Qii',self.objects_data,len(items),len(items)))
        self.put(self.names_header,struct.pack('<Qii',self.names_data,len(items),len(items)))
        for index,(address,name,parent) in enumerate(items):
            cls={0x10000:0x5000,0x20000:0x2000,0x30000:0x9000,0x40000:0xa000,0x50000:0x7000}.get(address,0x1000)
            outer=0x50000 if address==0x20000 else 0
            raw=bytearray(0x68);struct.pack_into('<I',raw,8,index);struct.pack_into('<Q',raw,64,outer)
            struct.pack_into('<II',raw,72,index,0);struct.pack_into('<Q',raw,80,cls);struct.pack_into('<Q',raw,96,parent)
            self.put(address,raw);self.put(self.objects_data+index*8,struct.pack('<Q',address));self.names[index]=name
            self.identities[address]={'name':name,'cls':cls}
        def prop(owner,name,offset,kind,structure=None):
            self.properties[owner,name]={'address':owner+offset,'offset':offset,'type':kind,'structure':structure}
        prop(0x6000,'Pawn',0x224,'ObjectProperty');prop(0x3000,'Controller',0x250,'ObjectProperty')
        prop(0x4000,'Location',0x180,'StructProperty',0xd000);prop(0x4000,'Rotation',0x190,'StructProperty',0xe000)
        prop(0x3000,'EyeHeight',0x260,'FloatProperty');prop(0x3000,'BaseEyeHeight',0x264,'FloatProperty')
        if view_target:prop(0x9000,'ViewTarget',0x500,'StructProperty',0xc000)
        prop(0xc000,'Target',0,'ObjectProperty')
        for i,k in enumerate(('X','Y','Z')):prop(0xd000,k,i*4,'FloatProperty')
        for i,k in enumerate(('Pitch','Yaw','Roll')):prop(0xe000,k,i*4,'IntProperty')
        for address,value in ((0x10224,0x20000),(0x20250,0x10000),(0x1037c,0x30000),
                              (0x30224,0x10000),(0x10374,0x40000),(0x30500,0x20000)):
            self.put(address,struct.pack('<Q',value))
        self.put(0x20180,struct.pack('<3f',10,20,30));self.put(0x20190,struct.pack('<3i',1,2,3))
        self.put(0x10190,struct.pack('<3i',4,5,6));self.put(0x20260,struct.pack('<2f',60,62))
        self.put(0x30600,struct.pack('<f',10));self.put(0x30604,struct.pack('<3f3if',100,200,300,4,5,6,80))
    def put(self,address,data):self.memory.update({address+i:v for i,v in enumerate(data)})
    def read(self,address,size):
        if address==0x30600:
            self.timestamp_reads+=1
            if self.timestamp_changes and self.timestamp_reads%2==0:self.put(address,struct.pack('<f',11))
            if self.pawn_changes and self.timestamp_reads%2==0:self.put(0x10224,struct.pack('<Q',0x40000))
        try:return bytes(self.memory[address+i] for i in range(size))
        except KeyError as error:raise RuntimeError('Unreadable synthetic memory') from error
    def u64(self,address):return struct.unpack('<Q',self.read(address,8))[0]
    def name(self,index):return self.names[index]
    def identity(self,address):return self.identities[address]
    def property(self,owner,name,inherited=False):
        while owner:
            if (owner,name) in self.properties:return copy.deepcopy(self.properties[owner,name])
            if not inherited:break
            owner=self.u64(owner+96)
        raise RuntimeError('Property '+name+' not found')
    def struct_type(self,prop):return prop['structure']
    def resolve(self):return {'camera_address':'0x30000','controller_address':'0x10000','local_player_address':'0x40000',
        'timestamp_address':'0x30600','pov_address':'0x30604','offsets':{'PlayerCamera':'0x37c','PCOwner':'0x224','Player':'0x374'}}


class AnchorTests(unittest.TestCase):
    def test_prepared_camera_skips_discovery_but_keeps_live_guards(self):
        r=MemoryResolver();prepared=r.resolve()
        with mock.patch.object(r,'resolve',side_effect=AssertionError('duplicate discovery')) as resolve:
            sampler=anchor.AnchorSampler(r,prepared_camera=prepared)
            self.assertTrue(sampler.sample()['accepted']);resolve.assert_not_called()
        prepared['offsets']['PlayerCamera']='0x0'
        self.assertEqual(sampler.resolved['offsets']['PlayerCamera'],'0x37c')
        for address,value in ((0x10224,0x40000),(0x30224,0x40000),(0x30500,0x40000)):
            with self.subTest(address=address):
                r=MemoryResolver();prepared=r.resolve();r.put(address,struct.pack('<Q',value))
                with mock.patch.object(r,'resolve',side_effect=AssertionError('duplicate discovery')) as resolve:
                    with self.assertRaises(anchor.AnchorRejected):anchor.AnchorSampler(r,prepared_camera=prepared)
                    resolve.assert_not_called()

    def test_default_still_discovers(self):
        r=MemoryResolver()
        with mock.patch.object(r,'resolve',wraps=r.resolve) as resolve:
            anchor.AnchorSampler(r);resolve.assert_called_once_with()

    def test_consistent_bracket_raw_layout_and_hypothesis_only(self):
        sampler=anchor.AnchorSampler(MemoryResolver());sample=sampler.sample()
        self.assertTrue(sample['accepted']);self.assertEqual(sample['candidate_eye_hypothesis'],[10,20,90])
        self.assertEqual(sample['values']['controller_rotation'],[4,5,6])
        self.assertEqual(sampler.metadata['layout']['eye_height']['type'],'FloatProperty')
        self.assertEqual(sample['guard_before'],sample['guard_after'])
        self.assertFalse(sampler.metadata['renderer_alignment_verified']);self.assertFalse(sampler.metadata['standing_verified'])

    def test_cross_frame_timestamp_rejected(self):
        r=MemoryResolver();sampler=anchor.AnchorSampler(r);r.timestamp_reads=0;r.timestamp_changes=True
        result=sampler.sample();self.assertFalse(result['accepted']);self.assertIn('Cross-frame',result['rejection'])

    def test_pawn_switch_during_read_rejected(self):
        r=MemoryResolver();sampler=anchor.AnchorSampler(r);r.timestamp_reads=0;r.pawn_changes=True
        self.assertFalse(sampler.sample()['accepted'])
        self.assertFalse(sampler.sample()['accepted'])

    def test_registry_reallocation_and_identity_reuse_rejected(self):
        for mutate in (lambda r:r.put(r.objects_header,struct.pack('<Q',0x999999)),
                       lambda r:r.put(0x20000+72,struct.pack('<I',0)),
                       lambda r:r.put(0x20250,struct.pack('<Q',0x40000)),
                       lambda r:r.put(0x30500,struct.pack('<Q',0x40000))):
            r=MemoryResolver();sampler=anchor.AnchorSampler(r);mutate(r)
            self.assertFalse(sampler.sample()['accepted'])

    def test_missing_view_target_reported_without_fabrication(self):
        sampler=anchor.AnchorSampler(MemoryResolver(view_target=False))
        self.assertTrue(sampler.sample()['accepted']);self.assertFalse(sampler.metadata['view_target_readable'])
        self.assertIsNone(sampler.sample()['guard_before']['view_target'])

    def test_nonfinite_values_and_wrong_level_rejected(self):
        r=MemoryResolver();sampler=anchor.AnchorSampler(r);r.put(0x20260,struct.pack('<f',float('nan')))
        self.assertFalse(sampler.sample()['accepted'])
        r=MemoryResolver();r.put(0x20000+64,struct.pack('<Q',0x40000))
        with self.assertRaisesRegex(anchor.AnchorRejected,'no Level'):anchor.AnchorSampler(r)


if __name__=='__main__':unittest.main()
