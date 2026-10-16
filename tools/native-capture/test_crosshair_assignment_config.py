"""Synthetic memory only; no game process or historical heap authority."""
import copy
import json
import struct
import unittest

from test_ui_observer_config import Fixture as UIBase
from crosshair_assignment_config import (NATIVE_BYTES, SLOTS, CrosshairConfigRejected, CrosshairRegistryChanged,
    build_crosshair_assignment_config, validate_crosshair_assignment_config,
    crosshair_assignment_json, crosshair_assignment_ini)


class Fixture(UIBase):
    def __init__(self):
        super().__init__()
        self.profile['struct_property_struct'] = 0xd0
        for kind in ('StructProperty', 'ScriptStruct', 'State'):
            self.cls(kind, self.classes['Object'])
        self.weapon = self.obj('Rifle', self.classes['Weap_AssaultRifle'])
        self.prop('Pawn', 'Weapon', 'ObjectProperty', 0x440)
        self.put(self.pawn + 0x440, struct.pack('<Q', self.weapon))
        self.put(self.weapon + 0x220, struct.pack('<Q', self.pawn))
        self.vector = self.obj('Vector', self.classes['ScriptStruct'])
        self.impact_struct = self.obj('ImpactInfo', self.classes['ScriptStruct'])
        self.writer = self.function('WarPC', 'UpdateHUDActions', 0x1d6ca0)
        self.lhs = self.field(self.classes['WarPC'], 'LastCrosshairHitLocation', 0x136c, 12, self.vector)
        self.member = self.field(self.impact_struct, 'HitLocation', 8, 12, self.vector)
        self.impact = self.field(self.writer, 'Impact', 0x40, 84, self.impact_struct)
        self.ww = self.field(self.writer, 'WW', 0x10, 8, self.classes['WarWeapon'], 'ObjectProperty')
        self.rotator = self.obj('Rotator', self.classes['ScriptStruct'])
        self.view = {}
        for name, offset, structure in (('CamLoc',0xa0,self.vector),('CamRot',0xb0,self.rotator),
                                        ('StartTrace',0xc0,self.vector),('TraceDir',0xd0,self.vector)):
            self.view[name] = self.field(self.writer, name, offset, 12, structure)
        self.script = 0xa00000
        self.put(self.script, bytes(0x16a7))
        tokens = b'\x0f\x01' + struct.pack('<Q', self.lhs) + b'\x35' + struct.pack('<QQ', self.member, self.impact_struct) + bytes(3) + struct.pack('<Q', self.impact)
        self.put(self.script + 0x359, tokens)
        self.put(self.writer + 0x8c, struct.pack('<Qii', self.script, 0x16a7, 0x16a7))
        for rva, raw in NATIVE_BYTES.items(): self.put(self.base+rva, bytes.fromhex(raw))
        for op, rva in SLOTS.items(): self.put(self.base+0x1b86280+op*8, struct.pack('<Q', self.base+rva))
        self.put(self.member, struct.pack('<Q', 0xb00000))
        self.put(0xb002a8, struct.pack('<Q', self.base+0x1d5ec8))
        self.tick = self.function('WarPC', 'PlayerTick', 0x1d6ca0)
        state = self.obj('PlayerDriving', self.classes['State'], self.classes['WarPC'])
        self.children.setdefault(self.classes['WarPC'], []).append(state)
        self.driving_tick = self.obj('PlayerTick', self.classes['Function'], state)
        self.children[state] = [self.driving_tick]
        for n, fn in enumerate((self.tick, self.driving_tick)):
            self.put(fn+0x114, struct.pack('<Q', self.base+0x1d6ca0))
            self.put(fn+0x8c, struct.pack('<Qii', 0xc00000+n*0x100, 3, 3))
            self.put(0xc00000+n*0x100, b'\x04\x0b\x53')
        self.finish_registry()

    def field(self, owner, name, offset, size, target, kind='StructProperty'):
        a = self.obj(name, self.classes[kind], owner)
        self.children.setdefault(owner, []).append(a)
        self.props[owner, name] = dict(address=a, offset=offset, type=kind)
        for off, fmt, value in ((0x94, '<I', offset), (0x70, '<I', 1), (0x74, '<I', size),
                                (0x78, '<Q', 0), (0xd0, '<Q', target)):
            self.put(a+off, struct.pack(fmt, value))
        return a


class CertificateTests(unittest.TestCase):
    def test_attested_execlet_entry_is_narrow_and_independently_bracketed(self):
        r=Fixture();original=bytes.fromhex(NATIVE_BYTES[0x1d7ad0]);calls=[]
        r.put(r.base+0x1d7ad0,b'\xe9'+bytes(31))
        def entry(address,size,raw):
            calls.append((address,size));self.assertEqual((address,size),(r.base+0x1d7ad0,32))
            self.assertEqual(raw(address,size),b'\xe9'+bytes(31));return original
        c=build_crosshair_assignment_config(r,r.camera,include_tick_roots=True,include_writer_view=True,entry_reader=entry)
        self.assertEqual(len(calls),3);self.assertEqual(c['nativebyteschecks'][0]['bytes'],original.hex())
        with self.assertRaises(CrosshairConfigRejected):build_crosshair_assignment_config(r,r.camera)
        # The capability cannot substitute any other native window, slot or script.
        for change in (lambda x:x.put(x.base+0x1d5efb,b'\x00'),
                       lambda x:x.put(x.base+0x1b86280+0x35*8,struct.pack('<Q',x.base+1)),
                       lambda x:x.put(x.script+0x359,b'\x00')):
            x=Fixture();x.put(x.base+0x1d7ad0,b'\xe9'+bytes(31));change(x)
            with self.assertRaises(CrosshairConfigRejected):
                build_crosshair_assignment_config(x,x.camera,entry_reader=lambda a,n,raw:original)
        for mode in ('changed_original','proof_failed'):
            r=Fixture();calls=[]
            def failed_entry(address,size,raw):
                calls.append((address,size))
                if len(calls)==3:
                    if mode=='proof_failed':raise RuntimeError('Sealed entry changed')
                    return bytes(32)
                return original
            with self.assertRaises(CrosshairConfigRejected):
                build_crosshair_assignment_config(r,r.camera,entry_reader=failed_entry)
            self.assertEqual(len(calls),3)

    def test_only_complete_registry_header_changes_are_retryable(self):
        for header_name in ('objects_header','names_header'):
            for phase in ('double','final'):
                with self.subTest(header=header_name,phase=phase):
                    r=Fixture();read=r.read;header=getattr(r,header_name);count=0
                    def changing(a,n):
                        nonlocal count
                        if a==header and n==16:
                            count+=1
                            if count==(2 if phase=='double' else 3):
                                ptr,total,capacity=struct.unpack('<Qii',read(a,n))
                                r.put(a,struct.pack('<Qii',ptr,total,capacity+1))
                        return read(a,n)
                    r.read=changing
                    with self.assertRaises(CrosshairRegistryChanged) as raised:
                        build_crosshair_assignment_config(r,r.camera)
                    self.assertIn(f'address={header:#x} size=16',str(raised.exception))
        # A short read does not establish a complete changed registry header.
        r=Fixture();read=r.read
        r.read=lambda a,n:read(a,n)[:-1] if a==r.objects_header else read(a,n)
        with self.assertRaises(CrosshairConfigRejected) as raised:
            build_crosshair_assignment_config(r,r.camera)
        self.assertIs(type(raised.exception),CrosshairConfigRejected)

    def test_nonregistry_final_changes_remain_hard_even_with_header_churn(self):
        for field in ('identity','link','script'):
            for churn in (False,True):
                with self.subTest(field=field,churn=churn):
                    r=Fixture();read=r.read;count=0
                    def changing(a,n):
                        nonlocal count
                        if a==r.objects_header and n==16:
                            count+=1
                            if count==3:
                                if churn:
                                    ptr,total,capacity=struct.unpack('<Qii',read(a,n))
                                    r.put(a,struct.pack('<Qii',ptr,total,capacity+1))
                                if field=='identity':r.put(r.weapon+80,struct.pack('<Q',r.classes['Object']))
                                if field=='link':r.put(r.pawn+0x440,struct.pack('<Q',r.pc))
                                if field=='script':r.put(r.script+0x359,b'\x00')
                        return read(a,n)
                    r.read=changing
                    with self.assertRaises(CrosshairConfigRejected) as raised:
                        build_crosshair_assignment_config(r,r.camera)
                    self.assertIs(type(raised.exception),CrosshairConfigRejected)
                    self.assertIn('Certificate changed before completion: address=',str(raised.exception))

    def test_optional_writer_view_roundtrip_and_legacy(self):
        r=Fixture(); old=build_crosshair_assignment_config(r,r.camera)
        self.assertEqual(old['writer_view'],dict(enabled=False))
        self.assertIn('view_enabled=0\n',crosshair_assignment_ini(old))
        del old['writer_view']; validate_crosshair_assignment_config(old)
        c=build_crosshair_assignment_config(r,r.camera,include_writer_view=True)
        self.assertEqual(c,json.loads(crosshair_assignment_json(c)))
        view=c['writer_view'];self.assertTrue(view['enabled'])
        self.assertEqual(c['identities'][view['roles']['cam_loc']]['address'],r.view['CamLoc'])
        self.assertEqual(c['identities'][view['roles']['rotator_struct']]['address'],r.rotator)
        ini=crosshair_assignment_ini(c)
        self.assertIn('view_enabled=1\n',ini);self.assertIn('view_trace_dir_offset=208\n',ini)

    def test_writer_view_fresh_shape_owner_and_overlap_rejected(self):
        for name,change in (
            ('dim',lambda r:r.put(r.view['CamLoc']+0x70,struct.pack('<I',2))),
            ('size',lambda r:r.put(r.view['CamRot']+0x74,struct.pack('<I',16))),
            ('structure',lambda r:r.put(r.view['CamRot']+0xd0,struct.pack('<Q',r.vector))),
            ('owner',lambda r:r.put(r.view['TraceDir']+64,struct.pack('<Q',r.impact_struct))),
            ('class',lambda r:r.put(r.view['StartTrace']+80,struct.pack('<Q',r.classes['ObjectProperty']))),
            ('missing',lambda r:r.children[r.writer].remove(r.view['CamLoc'])),
            ('duplicate',lambda r:r.children[r.writer].append(r.view['CamLoc'])),
        ):
            with self.subTest(name=name):
                r=Fixture();change(r)
                build_crosshair_assignment_config(r,r.camera) # Optional path adds no dependency.
                with self.assertRaises(CrosshairConfigRejected):
                    build_crosshair_assignment_config(r,r.camera,include_writer_view=True)
        for offset in (0x10,0x40,0xb0,65524):
            r=Fixture();r.put(r.view['CamLoc']+0x94,struct.pack('<I',offset))
            r.props[r.writer,'CamLoc']['offset']=offset
            with self.assertRaises(CrosshairConfigRejected):
                build_crosshair_assignment_config(r,r.camera,include_writer_view=True)

    def test_writer_view_serialized_tampering_rejected(self):
        r=Fixture();good=build_crosshair_assignment_config(r,r.camera,include_writer_view=True)
        mutations=(lambda c:c['writer_view'].update(enabled=1),
            lambda c:c['writer_view']['roles'].update(rotator_struct=c['roles']['vector_struct']),
            lambda c:c['writer_view']['offsets'].update(cam_loc=0x10),
            lambda c:c['writer_view']['offsets'].update(cam_loc=65524),
            lambda c:c['writer_view']['shapes']['cam_rot'].update(size=16),
            lambda c:c['writer_view']['shapes']['cam_loc'].update(structure=c['roles']['impact_struct']),
            lambda c:c['writer_view']['roles'].pop('trace_dir'),
            lambda c:c['identities'][c['writer_view']['roles']['cam_loc']].update(outer=0),
            lambda c:c['writer_view'].update(enabled=False))
        for mutate in mutations:
            c=copy.deepcopy(good);mutate(c)
            with self.assertRaises(CrosshairConfigRejected):validate_crosshair_assignment_config(c)

    def test_success_json_and_optional_roots(self):
        r = Fixture()
        c = build_crosshair_assignment_config(r, r.camera, include_tick_roots=True)
        self.assertEqual(c, json.loads(crosshair_assignment_json(c)))
        self.assertEqual(c['script'], r.script)
        self.assertEqual(c['identities'][c['roles']['weapon']]['address'], r.weapon)
        self.assertEqual([x['name'] for x in c['tick_roots']],
                         ['WarPC.PlayerTick', 'WarPC.PlayerDriving.PlayerTick'])
        self.assertFalse(c['runtime_coverage_proven'])
        self.assertEqual(len(c['nativebyteschecks']), 4)
        self.assertEqual(build_crosshair_assignment_config(r, r.camera)['tick_roots'], [])

    def test_ini_serializes_owned_layout_and_roots(self):
        r=Fixture(); c=build_crosshair_assignment_config(r,r.camera,include_tick_roots=True)
        import configparser
        ini=configparser.ConfigParser(); ini.read_string(crosshair_assignment_ini(c))
        s=ini['CrosshairAssignment']
        self.assertEqual(s.getint('native_contract'),1)
        self.assertEqual(s.getint('root1_function'),c['tick_roots'][1]['function'])
        self.assertEqual(s.getint('identity_count'),len(c['identities']))
        self.assertEqual(s.getint('pawn_weapon_property'),c['fields']['pawn_weapon']['property'])
        self.assertEqual(s.getint('property_structure'),0xd0)
        self.assertEqual(s.getint('script_size'),c['script_size'])

    def test_identity_type_pointer_script_and_native_fail_closed(self):
        mutations = {
            'registry slot': lambda r: r.put(r.od+r.objects.index(r.weapon)*8, bytes(8)),
            'wrong owner': lambda r: r.put(r.lhs+64, struct.pack('<Q', r.classes['WarPawn'])),
            'wrong type': lambda r: r.put(r.lhs+80, struct.pack('<Q', r.classes['ObjectProperty'])),
            'wrong vector': lambda r: r.put(r.lhs+0xd0, struct.pack('<Q', r.impact_struct)),
            'old pawn': lambda r: r.put(r.pc+0x224, struct.pack('<Q', r.weapon)),
            'wrong instigator': lambda r: r.put(r.weapon+0x220, struct.pack('<Q', r.pc)),
            'script token': lambda r: r.put(r.script+0x359, b'\x00'),
            'script property': lambda r: r.put(r.script+0x35b, struct.pack('<Q', r.member)),
            'script dispatch': lambda r: r.put(r.writer+0x114, struct.pack('<Q', r.base+1)),
            'current slot': lambda r: r.put(r.base+0x1b862f8, struct.pack('<Q', r.base+1)),
            'patched entry': lambda r: r.put(r.base+0x1d7ad0, b'\xe9'),
            'copy flags': lambda r: r.put(r.member+0x78, struct.pack('<Q', 0x400000)),
            'wrong PID': lambda r: r.camera.update(pid=999),
        }
        for name, mutate in mutations.items():
            with self.subTest(name=name):
                r=Fixture(); mutate(r)
                with self.assertRaises(CrosshairConfigRejected): build_crosshair_assignment_config(r, r.camera)

    def test_mutating_memory_rejected(self):
        r=Fixture(); original=r.read; count=0
        def read(a, n):
            nonlocal count
            if a == r.script and n == 0x16a7:
                count += 1
                if count == 2: r.put(r.script+0x359, b'\x00')
            return original(a, n)
        r.read=read
        with self.assertRaises(CrosshairConfigRejected): build_crosshair_assignment_config(r, r.camera)

    def test_late_link_change_rejected(self):
        r=Fixture(); original=r.read
        def read(a, n):
            if a == r.base+0x1d7ad0: r.put(r.weapon+0x220, bytes(8))
            return original(a, n)
        r.read=read
        with self.assertRaises(CrosshairConfigRejected): build_crosshair_assignment_config(r, r.camera)

    def test_optional_root_must_be_exact_script(self):
        r=Fixture(); r.put(r.driving_tick+0x114, struct.pack('<Q', r.base+0xcdacc))
        build_crosshair_assignment_config(r, r.camera)
        with self.assertRaises(CrosshairConfigRejected):
            build_crosshair_assignment_config(r, r.camera, include_tick_roots=True)

    def test_serialized_schema_rejects_tampering(self):
        r=Fixture(); good=build_crosshair_assignment_config(r, r.camera)
        for mutate in (lambda c:c['roles'].update(writer=999),
                       lambda c:c['nativebyteschecks'][0].update(sha256='bad'),
                       lambda c:c.update(runtime_coverage_proven=True),
                       lambda c:c['fields']['pawn_weapon'].update(offset=-1),
                       lambda c:c['identities'].append(copy.deepcopy(c['identities'][0]))):
            c=copy.deepcopy(good); mutate(c)
            with self.assertRaises(CrosshairConfigRejected): validate_crosshair_assignment_config(c)


if __name__ == '__main__':
    unittest.main()
