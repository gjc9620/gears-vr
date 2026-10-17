"""Synthetic reflected memory and byte certificates; never opens a process."""
import hashlib
import copy
import struct
import unittest
from unittest import mock

from test_anchor_config import Fixture
from anchor_config import (AnchorRejected, LiveRegistryAnchorSampler,
                           anchor_ini, build_anchor_config, validate_anchor_config)
import cinematic_anchor


class CinematicFixture(Fixture):
    BOOL_CLASS = 0x60000
    CAMERA_CLASS = 0x61000
    PROPERTY = 0x62000
    TARGET = 0x63000
    OTHER_LEVEL = 0x64000
    WORLD_CLASS = 0x66000
    WORLD_PROPERTY = 0x67000
    WORLD_INFO = 0x68000
    WORLD_OFFSET = 0xd0
    OFFSET = 0x700
    MASK = 0x20

    def __init__(self):
        super().__init__()
        self.base = 0x140000000
        self.ident_cache = {}
        self.object_pointers = list(self.identities)
        self.add(self.BOOL_CLASS, 'BoolProperty', 0x1000)
        self.add(self.CAMERA_CLASS, 'CameraActor', 0x1000, parent=0x4000)
        self.add(self.PROPERTY, 'bCinematicMode', self.BOOL_CLASS, outer=0x6000)
        self.add(self.TARGET, 'CameraActor_0', self.CAMERA_CLASS, outer=0x50000)
        self.add(self.OTHER_LEVEL, 'OtherLevel', 0x7000)
        self.add(0x69000, 'ObjectProperty', 0x1000)
        self.add(self.WORLD_CLASS, 'WorldInfo', 0x1000, parent=0x4000)
        self.add(self.WORLD_PROPERTY, 'WorldInfo', 0x69000, outer=0x4000)
        self.add(self.WORLD_INFO, 'WorldInfo', self.WORLD_CLASS, outer=0x50000)
        self.properties[0x4000, 'WorldInfo'] = dict(address=self.WORLD_PROPERTY,
            offset=self.WORLD_OFFSET, type='ObjectProperty')
        for offset,value in ((0x70,1),(0x74,8),(0x94,self.WORLD_OFFSET)):
            self.word(self.WORLD_PROPERTY+offset,value)
        for actor in (0x10000,0x20000,self.TARGET):self.ptr(actor+self.WORLD_OFFSET,self.WORLD_INFO)
        self.properties[0x6000, 'bCinematicMode'] = dict(
            address=self.PROPERTY, offset=self.OFFSET, type='BoolProperty')
        self.word(self.PROPERTY + 0x70, 1)
        self.word(self.PROPERTY + 0x74, 4)
        self.word(self.PROPERTY + 0x94, self.OFFSET)
        self.word(self.PROPERTY + 0xd0, self.MASK)
        self.vtable = self.base + 0x2000000
        self.ptr(self.PROPERTY, self.vtable)
        self.ptr(self.vtable + 0x2a0, self.base + 0x1daba0)
        self.ptr(self.base + 0x1b86280 + 0x2d * 8, self.base + 0x1d6ea4)
        # Synthetic functions keep the production SHA verification active;
        # only the expected byte certificates are fixture-specific.
        self.code = []
        for rva, size, _ in cinematic_anchor.BOOL_CODE:
            data = bytes((i * 13 + size) & 255 for i in range(size))
            self.put(self.base + rva, data)
            self.code.append((rva, size, hashlib.sha256(data).hexdigest()))
        self.cinematic(True)
        self.ptr(0x30500, self.TARGET)
        self.identity(self.CAMERA_CLASS)

    def ptr(self, address, value):
        self.put(address, struct.pack('<Q', value))

    def word(self, address, value):
        self.put(address, struct.pack('<I', value))

    def cinematic(self, enabled):
        self.word(0x10000 + self.OFFSET, self.MASK if enabled else 0)

    def add(self, address, name, cls, outer=0, parent=0):
        index = self.object_count
        raw = bytearray(0x68)
        struct.pack_into('<I', raw, 8, index)
        struct.pack_into('<QIIQQ', raw, 64, outer, index, 0, cls, 0)
        struct.pack_into('<Q', raw, 96, parent)
        self.put(address, raw)
        self.ptr(self.objects_data + index * 8, address)
        self.names[index] = name
        self.identities[address] = dict(name=name, cls=cls)
        self.object_pointers.append(address)
        entry = 0x300000 + index * 0x100
        self.ptr(self.names_data + index * 8, entry)
        raw = bytearray(148)
        struct.pack_into('<I', raw, 0, index)
        text = (name + '\0').encode('utf-16-le')
        raw[20:20 + len(text)] = text
        self.put(entry, raw)
        self.object_count = self.object_capacity = index + 1
        self.name_count = self.name_capacity = index + 1
        self.put(self.objects_header, struct.pack('<Qii', self.objects_data, index + 1, index + 1))
        self.put(self.names_header, struct.pack('<Qii', self.names_data, index + 1, index + 1))

    def identity(self, address):
        raw = self.read(address, 0x60)
        index = struct.unpack_from('<I', raw, 8)[0]
        name, number = struct.unpack_from('<II', raw, 72)
        result = dict(address=address, internal_index=index, name=self.name(name), number=number,
                      cls=struct.unpack_from('<Q', raw, 80)[0],
                      outer=struct.unpack_from('<Q', raw, 64)[0])
        self.ident_cache[address] = result
        return result


class CinematicAnchorTests(unittest.TestCase):
    def setUp(self):
        self.r = CinematicFixture()
        self.codes = mock.patch.object(cinematic_anchor, 'BOOL_CODE', tuple(self.r.code))
        self.codes.start()
        self.addCleanup(self.codes.stop)

    def sampler(self):
        return LiveRegistryAnchorSampler(self.r, cinematic_camera=True)

    def test_cold_discovery_precedes_timestamp_and_both_verifications_stay_fresh(self):
        events=[];read=self.r.read
        init=cinematic_anchor.CinematicTargetCertificate.__init__
        verify=cinematic_anchor.CinematicTargetCertificate.verify
        def constructing(contract,sampler):
            events.append('construct_begin');init(contract,sampler);events.append('construct_end')
        def verifying(contract,sampler):
            events.append('verify');return verify(contract,sampler)
        def reading(address,size):
            if (address,size)==(0x30600,4):events.append('stamp')
            return read(address,size)
        with mock.patch.object(cinematic_anchor.CinematicTargetCertificate,'__init__',constructing),\
             mock.patch.object(cinematic_anchor.CinematicTargetCertificate,'verify',verifying),\
             mock.patch.object(self.r,'read',reading):
            sampler=self.sampler()
            self.assertLess(events.index('construct_end'),events.index('stamp'))
            self.assertEqual(events[events.index('stamp'):].count('verify'),2)
            self.assertEqual(events.count('stamp'),3)
            events.clear();sampler.guard()
            self.assertEqual(events.count('verify'),2)
            self.assertNotIn('construct_begin',events)

    def test_cold_discovery_consumes_existing_guard_deadline(self):
        clock=[0];init=cinematic_anchor.CinematicTargetCertificate.__init__
        def constructing(contract,sampler):
            init(contract,sampler);clock[0]=201000000
        with mock.patch.object(cinematic_anchor.CinematicTargetCertificate,'__init__',constructing):
            with self.assertRaisesRegex(AnchorRejected,'deadline exceeded'):
                LiveRegistryAnchorSampler(self.r,lambda:clock[0],cinematic_camera=True)

    def test_second_verification_rejects_layout_name_and_registry_mutations(self):
        for mutation in ('bool_size','world_count','name','slot'):
            with self.subTest(mutation=mutation):
                self.r=CinematicFixture();sampler=self.sampler()
                verify=sampler.cinematic_contract.verify;calls=0
                identity=sampler.cinematic_contract.camera_class
                entry=self.r.u64(self.r.names_data+self.r.u32(identity['address']+72)*8)
                def changing(current):
                    nonlocal calls
                    calls+=1
                    if calls==2:
                        if mutation=='bool_size':self.r.word(self.r.PROPERTY+0x74,8)
                        elif mutation=='world_count':self.r.word(self.r.WORLD_PROPERTY+0x70,2)
                        elif mutation=='name':self.r.put(entry+20,'Changed\0'.encode('utf-16-le'))
                        else:self.r.ptr(self.r.objects_data+identity['internal_index']*8,0)
                    return verify(current)
                with mock.patch.object(sampler.cinematic_contract,'verify',changing):
                    with self.assertRaises(AnchorRejected):sampler.guard()
                self.assertEqual(calls,2)

    def test_verify_reads_both_property_shape_words_together(self):
        sampler=self.sampler()
        with mock.patch.object(self.r,'read',wraps=self.r.read) as read:
            sampler.cinematic_contract.verify(sampler)
        for prop in (self.r.PROPERTY,self.r.WORLD_PROPERTY):
            self.assertIn(mock.call(prop+0x70,8),read.call_args_list)
            self.assertNotIn(mock.call(prop+0x70,4),read.call_args_list)
            self.assertNotIn(mock.call(prop+0x74,4),read.call_args_list)

    def test_same_level_camera_schema2_retains_controlled_pawn(self):
        config, metadata = build_anchor_config(self.r, cinematic_camera=True)
        self.assertEqual(config['schema'], 2)
        self.assertEqual(validate_anchor_config(config), config)
        self.assertEqual(config['identities'][config['roles']['pawn']]['address'], 0x20000)
        proof = metadata['sample']['guard_before']['target_proof']
        self.assertEqual(proof['identity']['address'], self.r.TARGET)
        self.assertTrue(proof['cinematic'])
        self.assertEqual(proof['target_level']['address'], 0x50000)
        self.assertIn('cinematic_enabled=1\n', anchor_ini(config))
        self.assertEqual(config['cinematic']['cinematic_mask'], self.r.MASK)
        camera_class=config['identities'][config['cinematic']['role_camera_actor_class']]
        self.assertEqual(camera_class['check_super'],1)
        self.assertEqual(camera_class['super'],0x4000)
        incomplete=copy.deepcopy(config)
        incomplete['identities'][config['cinematic']['role_camera_actor_class']]['check_super']=0
        with self.assertRaisesRegex(AnchorRejected,'class ancestry'):
            validate_anchor_config(incomplete)

    def test_pawn_target_allows_flag_off_and_on_default_is_schema1(self):
        self.r.ptr(0x30500, 0x20000)
        for enabled in (False, True):
            with self.subTest(enabled=enabled):
                self.r.cinematic(enabled)
                sample = self.sampler().sample()
                self.assertTrue(sample['accepted'])
                self.assertEqual(sample['guard_before']['target_proof']['cinematic'], enabled)
        config, _ = build_anchor_config(self.r)
        self.assertEqual(config['schema'], 1)
        self.assertNotIn('cinematic_enabled', anchor_ini(config))

    def test_camera_rejected_without_opt_in_or_cinematic_flag(self):
        with self.assertRaisesRegex(AnchorRejected, 'not controlled Pawn'):
            LiveRegistryAnchorSampler(self.r)
        self.r.cinematic(False)
        with self.assertRaisesRegex(AnchorRejected, 'not controlled Pawn'):
            self.sampler()

    def test_fake_default_cross_world_and_unregistered_targets_rejected(self):
        mutations = (
            lambda: self.r.ptr(self.r.TARGET + 80, 0x4000),
            lambda: self.r.add(0x65000, 'Default__CameraActor', self.r.CAMERA_CLASS, 0x50000),
            lambda: self.r.ptr(self.r.TARGET + self.r.WORLD_OFFSET, self.r.OTHER_LEVEL),
            lambda: self.r.ptr(self.r.objects_data + self.r.identity(self.r.TARGET)['internal_index'] * 8, 0),
        )
        for i, mutate in enumerate(mutations):
            with self.subTest(case=i):
                self.r = CinematicFixture()
                mutate()
                if i == 1:
                    self.r.ptr(0x30500, 0x65000)
                with self.assertRaises(AnchorRejected):
                    self.sampler()

    def test_streamed_level_requires_same_world_info_and_full_level_identity(self):
        self.r.ptr(self.r.TARGET+64,self.r.OTHER_LEVEL)
        sampler=self.sampler()
        proof=sampler.guard()['target_proof']
        self.assertEqual(proof['target_level']['address'],self.r.OTHER_LEVEL)
        self.assertEqual(proof['world_info']['address'],self.r.WORLD_INFO)
        self.r.ptr(self.r.OTHER_LEVEL+80,0x4000)
        with self.assertRaises(AnchorRejected):sampler.guard()

    def test_world_links_layout_and_identity_changes_reject(self):
        mutations=(lambda:self.r.ptr(0x10000+self.r.WORLD_OFFSET,0),
            lambda:self.r.ptr(0x20000+self.r.WORLD_OFFSET,0),
            lambda:self.r.ptr(self.r.TARGET+self.r.WORLD_OFFSET,0),
            lambda:self.r.ptr(self.r.WORLD_INFO+64,self.r.OTHER_LEVEL),
            lambda:self.r.word(self.r.WORLD_PROPERTY+0x94,self.r.WORLD_OFFSET+8),
            lambda:self.r.word(self.r.WORLD_PROPERTY+0x74,4),
            lambda:self.r.word(self.r.WORLD_PROPERTY+0x70,2))
        for i,change in enumerate(mutations):
            with self.subTest(case=i):
                self.r=CinematicFixture();sampler=self.sampler();change()
                with self.assertRaises(AnchorRejected):sampler.guard()

    def test_reflected_layout_mask_dispatch_and_native_bytes_rejected(self):
        mutations = (
            lambda: self.r.word(self.r.PROPERTY + 0x70, 2),
            lambda: self.r.word(self.r.PROPERTY + 0x74, 8),
            lambda: self.r.word(self.r.PROPERTY + 0x94, self.r.OFFSET + 4),
            lambda: self.r.word(self.r.PROPERTY + 0xd0, 0),
            lambda: self.r.word(self.r.PROPERTY + 0xd0, 3),
            lambda: self.r.ptr(self.r.PROPERTY + 64, 0x7000),
            lambda: self.r.ptr(self.r.base + 0x1b86280 + 0x2d * 8, self.r.base + 0x1d6ea5),
            lambda: self.r.ptr(self.r.vtable + 0x2a0, self.r.base + 0x1daba1),
            lambda: self.r.put(self.r.base + 0x1d6ea4, b'\xff'),
            lambda: self.r.put(self.r.base + 0x1daba0, b'\xff'),
        )
        for i, mutate in enumerate(mutations):
            with self.subTest(case=i):
                self.r = CinematicFixture()
                mutate()
                with self.assertRaises(AnchorRejected):
                    self.sampler()

    def test_live_certificate_mutation_after_initial_guard_rejected(self):
        for offset, value in ((0xd0, 0x40), (0x94, self.r.OFFSET + 4)):
            with self.subTest(offset=offset):
                self.r = CinematicFixture()
                sampler = self.sampler()
                self.r.word(self.r.PROPERTY + offset, value)
                self.assertFalse(sampler.sample()['accepted'])

    def test_live_static_identity_name_slot_and_code_changes_rejected(self):
        mutations = (
            lambda: self.r.word(self.r.CAMERA_CLASS + 76, 1),
            lambda: self.r.ptr(self.r.PROPERTY + 80, 0x1000),
            lambda: self.r.ptr(self.r.names_data +
                               self.r.u32(self.r.TARGET + 72) * 8, 0),
            lambda: self.r.put(self.r.base + 0x1daba0, b'\xff'),
        )
        for i, mutate in enumerate(mutations):
            with self.subTest(case=i):
                self.r = CinematicFixture()
                sampler = self.sampler()
                mutate()
                with self.assertRaises(AnchorRejected):
                    sampler.guard()

    def test_schema2_rejects_multibit_mask_and_wrong_static_roles(self):
        config, _ = build_anchor_config(self.r, cinematic_camera=True)
        import copy
        for key, value in (('cinematic_mask', 3),
                           ('role_camera_actor_class', config['roles']['camera']),
                           ('role_cinematic_property', config['roles']['controller'])):
            with self.subTest(key=key):
                changed = copy.deepcopy(config)
                changed['cinematic'][key] = value
                with self.assertRaises(AnchorRejected):
                    validate_anchor_config(changed)

    def test_first_last_flag_target_identity_and_outer_changes_rejected(self):
        changes = (
            lambda: self.r.cinematic(False),
            lambda: self.r.ptr(0x30500, 0x20000),
            lambda: self.r.word(self.r.TARGET + 76, 1),
            lambda: self.r.ptr(self.r.TARGET + 64, self.r.OTHER_LEVEL),
        )
        for i, change in enumerate(changes):
            with self.subTest(case=i):
                self.r = CinematicFixture()
                sampler = self.sampler()
                original = sampler.cinematic_contract.target
                calls = 0
                def changing(*args):
                    nonlocal calls
                    proof = original(*args)
                    calls += 1
                    if calls == 1:
                        change()
                    return proof
                with mock.patch.object(sampler.cinematic_contract, 'target', changing):
                    with self.assertRaises(AnchorRejected):
                        sampler.guard()

    def test_cinematic_flag_change_at_final_timestamp_boundary_rejected(self):
        sampler = self.sampler()
        read = self.r.read
        count = 0
        def changing(address, size):
            nonlocal count
            data = read(address, size)
            if address == sampler.stamp and size == 4:
                count += 1
                if count == 2:
                    self.r.cinematic(False)
            return data
        with mock.patch.object(self.r, 'read', changing):
            with self.assertRaises(AnchorRejected):
                sampler.guard()


if __name__ == '__main__':
    unittest.main()
