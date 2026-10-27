"""Owned fake-memory checks of the production pair reader and real Query.

No game, processes, engine calls or runtime writes. The pose callback is a
fixture seam; these tests do not claim to test the existing live pose reader.
"""
import copy
import hashlib
import json
import math
import struct
import unittest
from unittest.mock import patch

import weapon_component_observation as component
from world_state import Query

IDENTITY = [1.,0.,0.,0., 0.,1.,0.,0., 0.,0.,1.,0., 0.,0.,0.,1.]


class Fixture:
    def __init__(self):
        self.memory, self.objects, self.props, self.classes = {}, {}, {}, {}
        self.next_address = 0x100000
        self.read_hook = None
        self.pose_hook = None
        self.verify_hook = None
        self.pose_calls = self.verifies = 0
        self.resolver = self.sampler = self
        self.profile = {'struct_property_struct': 0xd0}
        self.objects_header, self.names_header = self.alloc(), self.alloc()
        self.put(self.objects_header, struct.pack('<Qii', self.alloc(), 100, 200))
        self.put(self.names_header, struct.pack('<Qii', self.alloc(), 100, 200))
        self.pawn = self.obj('Pawn', 'Pawn')
        self.mesh = self.obj('AvatarMesh', 'SkeletalMeshComponent')
        self.weapon = self.obj('Lancer', 'Weap_AssaultRifle')
        self.gun = self.obj('GunMesh', 'SkeletalMeshComponent')
        self.asset = self.obj('COGAssaultRifle', 'SkeletalMesh')
        self.avatar_asset = self.obj('AvatarAsset', 'SkeletalMesh')
        self.matrix_type = self.obj('Matrix', 'ScriptStruct')
        self.vector_type = self.obj('Vector', 'ScriptStruct')
        self.rotator_type = self.obj('Rotator', 'ScriptStruct')
        self.attachment_type = self.obj('SkelComponentAttachment', 'ScriptStruct')
        for typ, size in [(self.matrix_type,64),(self.vector_type,12),
                          (self.rotator_type,12),(self.attachment_type,52)]:
            self.put(typ['address']+0x88, struct.pack('<I', size))
        self.field(self.pawn,'Mesh','ComponentProperty',0x100,struct.pack('<Q',self.mesh['address']))
        self.field(self.pawn,'Weapon','ObjectProperty',0x108,struct.pack('<Q',self.weapon['address']))
        self.field(self.weapon,'Instigator','ObjectProperty',0x100,struct.pack('<Q',self.pawn['address']))
        self.field(self.weapon,'Mesh','ComponentProperty',0x108,struct.pack('<Q',self.gun['address']))
        self.field(self.weapon,'bDeleteMe','BoolProperty',0x110,bytes(4))
        self.gun_world = list(IDENTITY)
        self.gun_world[12:15] = [12., 24., 36.]
        self.avatar_world = list(IDENTITY)
        self.avatar_world[12:15] = [2., 4., 6.]
        for obj, asset, matrix in [(self.mesh,self.avatar_asset,self.avatar_world),
                                   (self.gun,self.asset,self.gun_world)]:
            self.field(obj,'SkeletalMesh','ObjectProperty',0x218,struct.pack('<Q',asset['address']))
            self.field(obj,'LocalToWorld','StructProperty',0xa0,struct.pack('<16f',*matrix),self.matrix_type)
            self.field(obj,'HiddenGame','BoolProperty',0x200,bytes(4))
            for i,name in enumerate(('AbsoluteTranslation','AbsoluteRotation','AbsoluteScale')):
                self.field(obj,name,'BoolProperty',0x204+i*4,bytes(4))
            self.field(obj,'Translation','StructProperty',0x180,bytes(12),self.vector_type)
            self.field(obj,'Rotation','StructProperty',0x18c,bytes(12),self.rotator_type)
        self.reference, self.space, self.local = self.alloc(), self.alloc(), self.alloc()
        reference = bytearray(115*68)
        reference[52*68:52*68+8] = struct.pack('<II', 52, 0)
        self.put(self.reference, reference)
        self.put(self.avatar_asset['address']+0xa4,struct.pack('<Qii',self.reference,115,115))
        self.put(self.space,struct.pack('<16f',*IDENTITY)*115)
        self.put(self.local,bytes(115*32))
        self.put(self.mesh['address']+0x26c,struct.pack('<QiiQiiQiiQ',self.space,115,115,self.local,115,115,0,0,0,0))
        self.put(self.mesh['address']+0x338,struct.pack('<I',12))
        self.attachment_data = self.alloc()
        self.attachment_raw = struct.pack('<QII3f3i3f',self.gun['address'],52,0,0.,0.,0.,0,0,0,1.,1.,1.)
        self.put(self.attachment_data,self.attachment_raw)
        attachment = self.field(self.mesh,'Attachments','ArrayProperty',0x250,struct.pack('<Qii',self.attachment_data,1,1))
        inner = self.obj('AttachmentInner','StructProperty',outer=attachment['address'])
        self.put(attachment['address']+0xd0,struct.pack('<Q',inner['address']))
        self.put(inner['address']+0x70,struct.pack('<II',1,52))
        self.put(inner['address']+0xd0,struct.pack('<Q',self.attachment_type['address']))
        for name,kind,offset,size in [('Component','ComponentProperty',0,8),('BoneName','NameProperty',8,8),
                                      ('RelativeLocation','StructProperty',16,12),('RelativeRotation','StructProperty',28,12),
                                      ('RelativeScale','StructProperty',40,12)]:
            self.field(self.attachment_type,name,kind,offset,bytes(size),structure=True)

    def alloc(self):
        self.next_address += 0x10000
        return self.next_address

    def put(self, address, raw):
        self.memory.update({address+i:b for i,b in enumerate(raw)})

    def read(self, address, size):
        if self.read_hook:
            self.read_hook(address,size)
        try:
            return bytes(self.memory[address+i] for i in range(size))
        except KeyError as error:
            raise OSError('Unreadable owned memory') from error

    def obj(self, name, kind, outer=0):
        if kind not in self.classes:
            self.classes[kind] = self.alloc()
            self.put(self.classes[kind]+0x88,struct.pack('<I',0x1000))
        value = dict(address=self.alloc(),cls=self.classes[kind],name=name,outer=outer)
        self.objects[value['address']] = value
        return value

    def fresh(self, address):
        if address not in self.objects:
            raise RuntimeError('Missing owned object')
        return dict(self.objects[address])

    def derives(self, cls, label):
        return self.classes.get(label) == cls

    def field(self, obj, name, kind, offset, raw, definition=None, structure=False):
        owner = obj['address'] if structure else obj['cls']
        key = owner, name
        if key not in self.props:
            p = self.obj(name,kind,outer=owner)
            self.props[key] = dict(address=p['address'],offset=offset,type=kind)
            header = bytearray(0x28)
            struct.pack_into('<II',header,0,1,len(raw))
            struct.pack_into('<I',header,0x24,offset)
            self.put(p['address']+0x70,header)
            if kind == 'BoolProperty':
                self.put(p['address']+0xd0,struct.pack('<I',1))
            if definition:
                self.put(p['address']+0xd0,struct.pack('<Q',definition['address']))
        if not structure:
            self.put(obj['address']+offset,raw)
        return self.props[key]

    def property(self, owner, name, inherited):
        # Assert direct attachment members are re-resolved with their real mode.
        if owner == self.attachment_type['address'] and inherited:
            raise RuntimeError('Unexpected inherited attachment lookup')
        return dict(self.props[(owner,name)])

    def struct_type(self, prop):
        return struct.unpack('<Q',self.read(prop['address']+0xd0,8))[0]

    def verify(self):
        self.verifies += 1
        if self.verify_hook:
            self.verify_hook(self.verifies)

    def pose(self, observer):
        self.pose_calls += 1
        if self.pose_hook:
            self.pose_hook(self.pose_calls)
        matrix = list(struct.unpack('<16f',self.read(self.mesh['address']+0xa0,64)))
        return dict(pawn=dict(self.pawn),mesh=dict(self.mesh),pose_epoch=struct.unpack('<I',self.read(self.mesh['address']+0x338,4))[0],
                    reference_sha256=hashlib.sha256(self.read(self.reference,115*68)).hexdigest(),
                    space_sha256=hashlib.sha256(self.read(self.space,115*64)).hexdigest(),
                    local_sha256=hashlib.sha256(self.read(self.local,115*32)).hexdigest(),ctw=matrix,
                    bones={'weapon_end':{'world_matrix':matrix}},original_pose_marker='preserved')


class PairChecks(unittest.TestCase):
    def read_pair(self, fixture):
        original = fixture.pose(fixture)
        fixture.pose_calls = 0
        result = component.pair_probe(fixture,fixture.pose)
        self.assertEqual({k:v for k,v in result.items() if k!='weapon_component_pair'},original)
        return result['weapon_component_pair']

    def rejected(self, fixture, message):
        pair = self.read_pair(fixture)
        self.assertFalse(pair['accepted'])
        self.assertIn(message,pair['error'])
        self.assertEqual(pair['writes'],0)
        return pair

    def test_stable_and_real_query_default_entry(self):
        f = Fixture()
        result = component.wrap_probe(f.pose)(f)
        p = result['weapon_component_pair']
        self.assertTrue(p['accepted'])
        self.assertEqual(p['gun_relative_to_weapon_end'][12:15],[10.,20.,30.])
        self.assertTrue(p['same_C0_not_proved'] and p['final_render_phase_not_proved'])
        self.assertEqual((p['engine_calls'],p['writes']),(0,0))
        self.assertEqual(f.pose_calls,2)

    def test_fresh_pose_can_advance_during_discovery(self):
        f = Fixture()
        f.pose_hook = lambda count: f.put(f.mesh['address']+0x338,struct.pack('<I',13)) if count==2 else None
        result = component.pair_probe(f, f.pose)
        self.assertTrue(result['weapon_component_pair']['accepted'])
        self.assertEqual(result['pose_epoch'],13)
        self.assertEqual(result['weapon_component_pair']['pose_epoch'],13)
        pair=result['weapon_component_pair']
        self.assertLessEqual(pair['metadata_verified_ns'],pair['pair_window_begin_ns'])
        self.assertLessEqual(pair['pair_window_begin_ns'],pair['pair_window_end_ns'])
        self.assertLessEqual(pair['pair_window_end_ns'],pair['end_ns'])
        self.assertFalse(pair['dynamic_pose_required_stable_through_metadata_close'])

    def test_pose_epoch_changes_inside_dynamic_window(self):
        f = Fixture()
        def change(address,size):
            if address==f.gun['address']+0xa0 and size==64:
                f.put(f.mesh['address']+0x338,struct.pack('<I',13))
        f.read_hook=change
        self.rejected(f,'Pose changed at component close')

    def test_pose_bytes_change_without_epoch(self):
        for target in ('space','local','reference'):
            with self.subTest(target=target):
                f = Fixture()
                def change(address,size):
                    if address==f.gun['address']+0xa0 and size==64:
                        f.put(getattr(f,target),b'\x01')
                f.read_hook=change
                self.rejected(f,'metadata/registry changed' if target=='reference' else 'Pose changed at component close')

    def test_gun_motion_before_dynamic_capture_uses_current_matrix(self):
        f = Fixture()
        f.pose_hook = lambda count: f.put(f.gun['address']+0xa0+48,struct.pack('<f',99.)) if count==2 else None
        result=component.pair_probe(f,f.pose)['weapon_component_pair']
        self.assertTrue(result['accepted'])
        self.assertEqual(result['gun_matrix'][12],99.)

    def test_gun_motion_between_dynamic_repeats_rejected(self):
        f=Fixture();reads=0
        def change(address,size):
            nonlocal reads
            if address==f.gun['address']+0xa0 and size==64:
                reads+=1
                if reads==2:f.put(address+48,struct.pack('<f',99.))
        f.read_hook=change
        self.rejected(f,'Gun matrix changed during dynamic pair window')

    def test_close_pose_catches_late_change(self):
        f = Fixture()
        reads=0
        def change(address,size):
            nonlocal reads
            if address==f.gun['address']+0xa0 and size==64:
                reads+=1
                if reads==2:f.put(f.space,b'\x01')
        f.read_hook=change
        self.rejected(f,'Pose changed at component close')

    def test_post_window_motion_preserves_the_recorded_past_cohort(self):
        f = Fixture()
        f.verify_hook = lambda count: f.put(f.gun['address']+0xa0+48,struct.pack('<f',99.)) if count==2 else None
        result=component.pair_probe(f,f.pose)['weapon_component_pair']
        self.assertTrue(result['accepted'])
        self.assertEqual(result['gun_matrix'][12],12.)
        self.assertEqual(struct.unpack('<f',f.read(f.gun['address']+0xa0+48,4))[0],99.)
        self.assertTrue(result['same_C0_not_proved'])

    def test_post_window_body_motion_does_not_replace_the_saved_pair(self):
        f=Fixture();expected=f.pose(f);f.pose_calls=0
        def change(count):
            if count!=2:return
            f.put(f.mesh['address']+0x338,struct.pack('<I',13))
            f.put(f.space,b'\x01');f.put(f.local,b'\x01')
            f.put(f.mesh['address']+0xa0+48,struct.pack('<f',99.))
        f.verify_hook=change
        result=component.pair_probe(f,f.pose);pair=result['weapon_component_pair']
        self.assertTrue(pair['accepted'])
        for key in ('pose_epoch','space_sha256','local_sha256','ctw','bones'):
            self.assertEqual(result[key],expected[key])
        self.assertEqual(pair['pose_epoch'],12)
        self.assertEqual(pair['avatar_matrix'],expected['ctw'])
        self.assertEqual(struct.unpack('<I',f.read(f.mesh['address']+0x338,4))[0],13)

    def test_avatar_matrix_motion_inside_dynamic_window_rejected(self):
        f=Fixture()
        def change(address,size):
            if address==f.gun['address']+0xa0 and size==64:
                f.put(f.mesh['address']+0xa0+48,struct.pack('<f',99.))
        f.read_hook=change
        self.rejected(f,'Avatar component matrix changed around pose read')

    def test_static_metadata_still_closes_after_dynamic_window(self):
        for target in ('attachment','typed_layout','component_extent'):
            with self.subTest(target=target):
                f=Fixture()
                def change(count):
                    if count!=2:return
                    if target=='attachment':f.put(f.attachment_data+8,struct.pack('<II',51,0))
                    elif target=='typed_layout':
                        prop=f.props[(f.gun['cls'],'LocalToWorld')]
                        f.put(prop['address']+0x70+0x24,struct.pack('<I',0xb0))
                    else:f.put(f.gun['cls']+0x88,struct.pack('<I',4))
                f.verify_hook=change
                self.rejected(f,'metadata/registry changed')

    def test_expensive_selector_and_query_closing_outside_dynamic_reads(self):
        f=Fixture();events=[]
        original_property=f.property
        def prop(owner,name,inherited):
            events.append(('property',owner,name))
            return original_property(owner,name,inherited)
        f.property=prop
        def reads(address,size):
            if address==f.gun['address']+0xa0 and size==64:events.append(('gun_dynamic',))
        f.read_hook=reads
        original_close=component._close_pose
        def close(q,body):
            events.append(('pose_close_begin',))
            original_close(q,body)
            events.append(('pose_close_end',))
        f.verify_hook=lambda count:events.append(('verify',count))
        with patch.object(component,'_close_pose',side_effect=close):
            self.assertTrue(component.pair_probe(f,f.pose)['weapon_component_pair']['accepted'])
        begin=next(i for i,e in enumerate(events) if e[0]=='gun_dynamic')
        end=next(i for i,e in enumerate(events) if e[0]=='pose_close_end')
        self.assertEqual([e for e in events[begin:end+1] if e[0] in ('property','verify')],[])
        self.assertTrue(any(e[0]=='property' for e in events[:begin]))
        self.assertTrue(any(e[0]=='property' for e in events[end+1:]))

    def test_selector_changes(self):
        f = Fixture()
        f.pose_hook = lambda count: f.props[(f.pawn['cls'],'Weapon')].update(offset=0x118) if count==2 else None
        self.rejected(f,'selector changed')

    def test_identity_replaced(self):
        f = Fixture()
        f.pose_hook = lambda count: f.objects[f.weapon['address']].update(name='Other') if count==2 else None
        self.rejected(f,'metadata/registry changed')

    def test_registry_growth_is_rejected(self):
        f = Fixture()
        f.pose_hook = lambda count: f.put(f.objects_header+8,struct.pack('<i',101)) if count==2 else None
        self.rejected(f,'metadata/registry changed')

    def test_attachment_wrong_bone_and_duplicate(self):
        for duplicate in (False,True):
            with self.subTest(duplicate=duplicate):
                f=Fixture()
                if duplicate:
                    f.put(f.mesh['address']+0x250,struct.pack('<Qii',f.attachment_data,2,2))
                    f.put(f.attachment_data+52,f.attachment_raw)
                else:
                    f.put(f.attachment_data+8,struct.pack('<II',51,0))
                self.rejected(f,'not uniquely attached')

    def test_component_observation_failure_preserves_original(self):
        f=Fixture()
        del f.memory[f.gun['address']+0xa0]
        self.rejected(f,'Unreadable owned memory')

    def test_original_pose_failure_still_propagates(self):
        f=Fixture()
        def failed(_):
            raise RuntimeError('original pose rejected')
        with self.assertRaisesRegex(RuntimeError,'original pose rejected'):
            component.wrap_probe(failed)(f)

    def test_second_pose_failure_is_only_extra_rejection(self):
        f=Fixture()
        def failure(count):
            if count==2:
                raise RuntimeError('second pose rejected')
        f.pose_hook=failure
        self.rejected(f,'second pose rejected')

    def test_discovery_hints_rejected(self):
        f=Fixture()
        f._field_hint_properties={}
        self.rejected(f,'Discovery-only')

    def test_malformed_matrices(self):
        for value in (math.nan,math.inf):
            with self.subTest(value=value):
                f=Fixture()
                f.put(f.gun['address']+0xa0,struct.pack('<f',value))
                self.rejected(f,'Nonfinite')
        f=Fixture()
        f.put(f.gun['address']+0xa0,bytes(64))
        self.rejected(f,'Non-affine')

    def test_inverse_rotated_scaled_affine(self):
        m=[0.,2.,0.,0., -3.,0.,0.,0., 0.,0.,4.,0., 7.,8.,9.,1.]
        for product in (component.mul(m,component.inverse_affine(m)),component.mul(component.inverse_affine(m),m)):
            for a,b in zip(product,IDENTITY):
                self.assertAlmostEqual(a,b,places=10)


if __name__ == '__main__':
    unittest.main(verbosity=2)
