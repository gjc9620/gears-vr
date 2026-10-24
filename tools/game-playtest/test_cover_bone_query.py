"""Synthetic reflected memory and GT queue; no game, DLL or process writes."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import cover_bone_query as bone_query
from cover_bone_query import SubmitFailure, capture
from test_cover_animation_state import AnimationMemory


def pe_headers(sections, *, image_size=0x10000, import_directory=(0, 0), resource_directory=(0, 0)):
    """Small independent PE32+ fixture; its section rows are the disk manifest."""
    raw = bytearray(0x400)
    raw[:2] = b'MZ'
    struct.pack_into('<I', raw, 0x3c, 0x80)
    raw[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', raw, 0x84, 0x8664, len(sections), 0x12345678, 0, 0, 240, 0x22)
    optional = 0x98
    struct.pack_into('<H', raw, optional, 0x20b)
    struct.pack_into('<IIQII', raw, optional + 16, 0x1000, 0x1000, 0x140000000, 0x1000, 0x200)
    struct.pack_into('<IIIHH', raw, optional + 56, image_size, len(raw), 0x87654321, 2, 0x8160)
    struct.pack_into('<I', raw, optional + 108, 16)
    struct.pack_into('<II', raw, optional + 120, *import_directory)
    struct.pack_into('<II', raw, optional + 128, *resource_directory)
    for index, section in enumerate(sections):
        struct.pack_into('<8sIIIIIIHHI', raw, 0x188 + index * 40, section['name'].encode('ascii'),
                         section['virtual_size'], section['rva'], section['raw_size'], section['raw_pointer'],
                         0, 0, 0, 0, section['characteristics'])
    disk_size = max([len(raw)] + [s['raw_pointer'] + s['raw_size'] for s in sections])
    return bytes(raw), disk_size


class BoneMemory(AnimationMemory):
    MATRIX, PLANE = 0xe0000, 0xe1000
    FUNCTION_CLASS, STRUCT_CLASS, PROPERTY_CLASS, META_CLASS = 0xf0000, 0xf1000, 0xf2000, 0x99000
    MATCH, MATRIX_FN = 0x100000, 0x101000
    BASE, NAME_DATA = 0x140000000, 0x110000
    DLL_SHA = 'b' * 64

    def __init__(self):
        super().__init__()
        self.profile.update(field_next=0x68, struct_children=0x80, name_entry_index=0,
                            name_entry_text=20, sha256='a' * 64)
        self.pid, self.base = 77, self.BASE
        self.names_cache = {}
        # Real Query identity/type/outer checks stay in use. Unlike older
        # fixtures, every property has its actual reflected metaclass identity.
        self.add_class(self.META_CLASS, 'Class', 0)
        self.add_class(self.FUNCTION_CLASS, 'Function', 0)
        self.add_class(self.STRUCT_CLASS, 'ScriptStruct', 0)
        self.add_class(self.PROPERTY_CLASS, 'Property', 0)
        for address, name in ((self.PC_CLASS, 'WarPC'), (0x20300, 'WorldInfo'),
                               (0x20400, 'Object')):
            self.add_class(address, name, 0)
        kinds = sorted(set(self.kinds.values()) | {'NameProperty'})
        self.kind_classes = {}
        for i, kind in enumerate(kinds):
            cls = 0x120000 + i * 0x1000
            self.kind_classes[kind] = cls
            self.add_class(cls, kind, self.PROPERTY_CLASS)
        for address, kind in self.kinds.items():
            self.objects[address]['cls'] = self.kind_classes[kind]
        for address in (self.MATRIX, self.PLANE, self.VECTOR):
            name = {self.MATRIX: 'Matrix', self.PLANE: 'Plane', self.VECTOR: 'Vector'}[address]
            self.add_object(address, name, self.STRUCT_CLASS)
            self.put(address + 0x60, struct.pack('<Q', self.VECTOR if address == self.PLANE else 0))
        # Plane inherits Vector's X/Y/Z; W is declared on Plane. This catches
        # implementations that assume four directly declared float children.
        self.add_property(self.PLANE, 'W', 'FloatProperty', 12, 4)
        for index, name in enumerate(('XPlane', 'YPlane', 'ZPlane', 'WPlane')):
            self.add_property(self.MATRIX, name, 'StructProperty', index * 16, 16, target=self.PLANE)
        sections = [dict(name='.text', rva=0x1000, size=0x2000, virtual_size=0x2000,
                         raw_size=0x2000, raw_pointer=0x400, characteristics=0x60000020),
                    dict(name='.data', rva=0x4000, size=0x2000, virtual_size=0x2000,
                         raw_size=0x2000, raw_pointer=0x2400, characteristics=0xc0000040)]
        self.header, disk_size = pe_headers(sections)
        self.put(self.BASE, self.header)
        self.module = dict(base=self.BASE, path='synthetic/GOWDE-Steam.exe', sha256='a' * 64,
                           size=0x10000, headers=self.header, disk_size=disk_size, sections=sections)
        self.next_property = 0x140000
        self.function(self.MATCH, 'MatchRefBone', 0x1000, 24, 12,
                      [('BoneName', 'NameProperty', 0, 8, None), ('ReturnValue', 'IntProperty', 12, 4, None)])
        # Deliberately different from the historical 80-byte/return+16 hint.
        self.function(self.MATRIX_FN, 'GetBoneMatrix', 0x1100, 112, 32,
                      [('BoneIndex', 'IntProperty', 8, 4, None), ('ReturnValue', 'StructProperty', 32, 64, self.MATRIX)])
        self.put(self.MESH_CLASS + 0x80, struct.pack('<Q', self.MATCH))
        self.put(self.MATCH + 0x68, struct.pack('<Q', self.MATRIX_FN))
        self.put(self.MATRIX_FN + 0x68, bytes(8))
        owners = {owner for owner, _ in self.properties}
        for owner in owners:
            addresses = [p['address'] for (declaring, _), p in self.properties.items() if declaring == owner]
            if owner == self.MESH_CLASS:
                addresses = [self.MATCH, self.MATRIX_FN] + addresses
            self.put(owner + 0x80, struct.pack('<Q', addresses[0]))
            for current, nxt in zip(addresses, addresses[1:] + [0]):
                self.put(current + 0x68, struct.pack('<Q', nxt))
        self.bone_names = ['None', 'root', 'spine', 'head', 'root_extra']
        self.put(self.names_header, struct.pack('<Qii', self.NAME_DATA, len(self.bone_names), 16))
        for index, name in enumerate(self.bone_names):
            entry = 0x160000 + index * 0x100
            self.put(self.NAME_DATA + index * 8, struct.pack('<Q', entry))
            self.put(entry, bytes(148))
            self.put(entry, struct.pack('<I', index) + bytes(16) + (name + '\0').encode('utf-16-le'))
        owner = dict(pid=66, creation_time=123, image='synthetic/host.exe')
        game = dict(pid=self.pid, creation_time=456, image=self.module['path'])
        self.observer.identities = dict(owner_identity=owner, game_identity=game, xr_identity=dict(pid=88))
        self.observer.session = dict(session_id='owned-session', owner_identity=copy.deepcopy(owner),
                                     game_identity=copy.deepcopy(game))
        self.observer.action_owner = copy.deepcopy(owner)
        self.certificates = dict(schema=1, kind='cover_bone_getter_certificate', game_sha256='a' * 64, functions={})
        for name, fn in (('MatchRefBone', self.MATCH), ('GetBoneMatrix', self.MATRIX_FN)):
            entry = struct.unpack('<Q', self.read(fn + 0x114, 8))[0]
            prefix = self.read(entry, 64)
            tail_rva = 0x1200 if name == 'MatchRefBone' else 0x1300
            tail = bytes([0xee if name == 'MatchRefBone' else 0xdd]) * 16
            self.put(self.BASE + tail_rva, tail)
            self.certificates['functions'][name] = dict(entry_rva=hex(entry - self.BASE),
                                                       return_bytes=4 if name == 'MatchRefBone' else 64,
                                                       chunks=[dict(rva=hex(entry - self.BASE), bytes=prefix.hex(),
                                                                    sha256=hashlib.sha256(prefix).hexdigest()),
                                                               dict(rva=hex(tail_rva), bytes=tail.hex(),
                                                                    sha256=hashlib.sha256(tail).hexdigest())])
        self.bridges = []
        self.configure_bridge = None

    def reviewed_resource_layout(self):
        # Only the PE layout is synthetic. These directory values and this disk
        # identity are the exact values from the purchased-image t2 diagnostic.
        self.profile['sha256'] = self.module['sha256'] = self.certificates['game_sha256'] = (
            'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4')
        self.module['sections'] += [
            dict(name='.unpack', rva=0x1eae000, size=0xb37e8, virtual_size=0xb37e8,
                 raw_size=0, raw_pointer=0, characteristics=0x40000040),
            dict(name='.rsrc', rva=0x2013000, size=0x23000, virtual_size=0x23000,
                 raw_size=0, raw_pointer=0, characteristics=0x40000040),
            dict(name='.idata', rva=0x2011000, size=0x1000, virtual_size=0x1000,
                 raw_size=0, raw_pointer=0, characteristics=0xc0000040)]
        self.module['size'] = 0x2040000
        self.header, self.module['disk_size'] = pe_headers(self.module['sections'], image_size=self.module['size'],
                                                         import_directory=(0x2011755, 0x6b8),
                                                         resource_directory=(0x2013000, 0x226cc))
        self.module['headers'] = self.header
        live = bytearray(self.header)
        struct.pack_into('<II', live, 0x118, 0x1eae000, 0xb37e8)
        self.put(self.BASE, live)

    def reviewed_empty_raw_pointer_layout(self, *, resources=False):
        # Synthetic PE rows; only the purchased hash and one observed row's
        # scalars are from AR-A8. No shipped header or code bytes are embedded.
        self.profile['sha256'] = self.module['sha256'] = self.certificates['game_sha256'] = (
            'bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4')
        sections = copy.deepcopy(self.module['sections'])
        for i in range(2, 12):
            sections.append(dict(name='.p' + str(i), rva=0x10000 + i * 0x1000,
                                 size=0x1000, virtual_size=0x1000, raw_size=0,
                                 raw_pointer=0, characteristics=0xc0000040))
        sections.append(dict(name='.themida', rva=33775616, size=24985600,
                             virtual_size=24985600, raw_size=0, raw_pointer=11354624,
                             characteristics=3758096480))
        if resources:
            sections.extend([dict(name='.unpack', rva=0x1eae000, size=0xb37e8, virtual_size=0xb37e8,
                                  raw_size=0, raw_pointer=0, characteristics=0x40000040),
                             dict(name='.rsrc', rva=0x2013000, size=0x23000, virtual_size=0x23000,
                                  raw_size=0, raw_pointer=0, characteristics=0x40000040)])
        self.module['sections'] = sections
        self.module['size'] = 73691136
        self.header, self.module['disk_size'] = pe_headers(sections, image_size=self.module['size'],
                                                         resource_directory=(0x2013000, 0x226cc) if resources else (0, 0))
        self.module['headers'] = self.header
        live = bytearray(self.header)
        struct.pack_into('<I', live, 0x188 + 12 * 40 + 20, 0)
        if resources:
            struct.pack_into('<II', live, 0x118, 0x1eae000, 0xb37e8)
        self.put(self.BASE, live)

    def add_property(self, owner, name, kind, offset, size, **kwargs):
        super().add_property(owner, name, kind, offset, size, **kwargs)
        if hasattr(self, 'kind_classes'):
            address = self.properties[owner, name]['address']
            self.objects[address]['cls'] = self.kind_classes[kind]

    def function(self, address, name, rva, size, return_offset, fields):
        self.put(address, bytes(0x11c))
        self.add_object(address, name, self.FUNCTION_CLASS, outer=self.MESH_CLASS)
        self.put(address + 0x106, struct.pack('<HH', size, return_offset))
        self.put(address + 0x114, struct.pack('<Q', self.BASE + rva))
        self.put(self.BASE + rva, bytes([0x40 + rva // 0x100]) * 64)
        addresses = []
        for label, kind, offset, width, target in fields:
            self.add_property(address, label, kind, offset, width, target=target)
            prop = self.properties[address, label]['address']
            # Do not assume ordinary Parms flags. These intentionally do not
            # satisfy Bridge.call's rawflags & 0x80 filter.
            self.put(prop + 0x78, struct.pack('<Q', 0x1000000000000000))
            addresses.append(prop)
        self.put(address + 0x80, struct.pack('<Q', addresses[0]))
        for current, nxt in zip(addresses, addresses[1:] + [0]):
            self.put(current + 0x68, struct.pack('<Q', nxt))

    def name(self, index):
        return self.names_cache.get(index, self.bone_names[index])

    def factory(self, identity, dll_path, *, expected_sha256, attach_only):
        if expected_sha256 != self.DLL_SHA or attach_only is not True or identity != self.observer.identities['game_identity']:
            raise RuntimeError('Fake attach/hash/identity rejected')
        bridge = FakeBridge(self)
        self.bridges.append(bridge)
        if self.configure_bridge:
            self.configure_bridge(bridge)
        return bridge

    def capture(self, names=('root', 'head'), **kwargs):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'private-getter-certificate.json'
            path.write_text(json.dumps(self.certificates))
            return capture(self.observer, 'synthetic/Playground.dll', self.DLL_SHA, names,
                           certificate_path=path, bridge_factory=self.factory, **kwargs)


class FakeBridge:
    """A queue consumer with explicit remote storage and transport failure seams."""
    def __init__(self, fixture):
        self.f = fixture
        self.module = copy.deepcopy(fixture.module)
        self.submit_source = dict(entry=0x777000, bytes='00' * 32, dll_sha256=fixture.DLL_SHA)
        self.allocations, self.freed, self.submissions = {}, [], []
        self.closed, self.next_address = False, 0x200000
        self.submit_result = 0
        self.queue_state = 2
        self.index = None
        self.matrix = [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 10., 20., 30., 1.]
        self.error_at = None
        self.on_submit = self.on_read = None

    def verify(self):
        if self.error_at == 'verify':
            raise RuntimeError('Synthetic process creation identity changed')

    def allocate(self, size):
        if self.error_at == 'allocate':
            raise OSError('Synthetic VirtualAllocEx failed')
        assert size == bone_query.REQUEST_SIZE
        address = self.next_address
        self.next_address += 0x2000
        self.allocations[address] = bytearray(size)
        return address

    def write(self, address, raw):
        if self.error_at == 'write':
            raise OSError('Synthetic request initial write failed')
        self.allocations[address][:] = raw

    def submit(self, address, timeout_ms):
        assert 0 < timeout_ms <= 1000
        self.submissions.append(address)
        if self.error_at == 'create_thread':
            raise SubmitFailure('Synthetic CreateRemoteThread failed', pending_possible=False)
        if self.error_at == 'submit_timeout':
            raise SubmitFailure('Synthetic Submit thread did not return', pending_possible=True)
        if self.on_submit:
            self.on_submit(self, address)
        if self.submit_result:
            return self.submit_result
        raw = self.allocations[address]
        _, size, obj, fn = struct.unpack_from('<iIQQ', raw)
        assert obj == self.f.MESH
        return_offset = struct.unpack('<H', self.f.read(fn + 0x108, 2))[0]
        if fn == self.f.MATCH:
            prop = self.f.properties[fn, 'BoneName']
            name_index, number = struct.unpack_from('<II', raw, 24 + prop['offset'])
            assert number == 0
            value = self.index if self.index is not None else name_index + 2
            struct.pack_into('<i', raw, 24 + return_offset, value)
        else:
            assert fn == self.f.MATRIX_FN
            prop = self.f.properties[fn, 'BoneIndex']
            assert 0 <= struct.unpack_from('<i', raw, 24 + prop['offset'])[0] < 100
            struct.pack_into('<16f', raw, 24 + return_offset, *self.matrix)
        struct.pack_into('<i', raw, 0, self.queue_state)
        return 0

    def read(self, address, size):
        if self.on_read:
            self.on_read(self, address, size)
        return bytes(self.allocations[address][:size])

    def free(self, address):
        assert struct.unpack_from('<i', self.allocations[address])[0] == 2
        self.freed.append(address)
        del self.allocations[address]

    def close(self):
        self.closed = True


class BoneQueryTests(unittest.TestCase):
    def result(self, fixture, accepted, names=('root', 'head')):
        row = fixture.capture(names)
        self.assertIs(row['accepted'], accepted, row.get('rejection'))
        for key in ('activation_allowed', 'actor_or_mesh_fields_written', 'atomic_snapshot_verified',
                    'same_pose_epoch_verified', 'collision_verified', 'world_matrix_getbonelocation_pair_verified'):
            self.assertIs(row[key], False)
        json.dumps(row)  # Every source/partial/pending receipt must be serializable.
        for bridge in fixture.bridges:
            self.assertTrue(bridge.closed)
        if not accepted:
            self.assertIn('rejection', row)
        return row

    def test_real_query_layout_name_number_zero_gt_calls_and_private_source_receipt(self):
        f = BoneMemory()
        row = self.result(f, True)
        self.assertEqual([(x['name'], x['index']) for x in row['bones']], [('root', 3), ('head', 5)])
        self.assertEqual(row['bones'][0]['matrix'][12:15], [10., 20., 30.])
        self.assertEqual([c['getter'] for c in row['calls']], ['MatchRefBone', 'GetBoneMatrix'] * 2)
        b = f.bridges[0]
        self.assertEqual(len(b.submissions), 4)
        self.assertEqual(len(set(b.submissions)), 4)
        self.assertEqual(b.freed, b.submissions)
        self.assertEqual(row['retained_allocations'], [])
        matrix = row['calls'][1]['function']
        self.assertEqual((matrix['parms_size'], matrix['return_offset']), (112, 32))
        self.assertEqual(matrix['fields']['BoneIndex']['offset'], 8)
        self.assertTrue(matrix['source']['compared_in_current_process'])
        raw = bytes.fromhex(matrix['source']['chunks'][0]['bytes'])
        self.assertEqual(hashlib.sha256(raw).hexdigest(), matrix['source']['chunks'][0]['sha256'])
        self.assertEqual(len(matrix['source']['chunks']), 2)
        self.assertEqual([c['bone_fname']['number'] for c in row['calls'] if 'bone_fname' in c], [0, 0])
        # No opaque pose data or guessed native bone-name table is read.
        self.assertFalse(any(address == f.arrays[f.MESH, 'SpaceBases'] for address, _ in f.read_counts))
        self.assertFalse(any(address == f.arrays[f.ASSET, 'RefSkeleton'] for address, _ in f.read_counts))

    def test_reviewed_empty_raw_pointer_preserves_exact_live_receipt(self):
        f = BoneMemory()
        f.reviewed_empty_raw_pointer_layout()
        row = self.result(f, True, ['root'])
        evidence = row['calls'][0]['function']['source']['module']['pe_validation']
        self.assertFalse(evidence['non_resource_header_bytes_equal'])
        self.assertTrue(evidence['reviewed_non_resource_header_bytes_equal'])
        self.assertEqual(evidence['empty_raw_pointer_adaptations'], [
            dict(section=12, name='.themida', field='PointerToRawData', offset=0x188 + 12 * 40 + 20,
                 disk=11354624, live=0, initialized_disk_bytes=0, virtual_layout_unchanged=True)])
        self.assertNotEqual(evidence['disk_header_sha256'], evidence['live_header_sha256'])
        self.assertTrue(evidence['live_header_closing_reread_required'])

    def test_empty_raw_pointer_and_reviewed_resource_relocation_are_independent(self):
        f = BoneMemory()
        f.reviewed_empty_raw_pointer_layout(resources=True)
        row = self.result(f, True, ['root'])
        evidence = row['calls'][0]['function']['source']['module']['pe_validation']
        self.assertEqual(evidence['resource_policy'], 'reviewed_runtime_resource_relocation')
        self.assertEqual(len(evidence['empty_raw_pointer_adaptations']), 1)

    def test_empty_raw_pointer_never_masks_unknown_hash_or_other_section_fields(self):
        changes = [('hash', None, None, None), ('pointer', 20, 'I', 512), ('raw_size', 16, 'I', 512),
                   ('virtual_size', 8, 'I', 24981504), ('rva', 12, 'I', 33779712),
                   ('flags', 36, 'I', 0x60000020), ('name', 0, 'B', ord('X')),
                   ('other_pointer', None, None, None), ('header_padding', None, None, None)]
        for label, offset, fmt, value in changes:
            with self.subTest(label=label):
                f = BoneMemory()
                f.reviewed_empty_raw_pointer_layout()
                if label == 'hash':
                    f.profile['sha256'] = f.module['sha256'] = f.certificates['game_sha256'] = 'c' * 64
                elif label == 'other_pointer':
                    f.put(f.BASE + 0x19c, struct.pack('<I', 0x600))
                elif label == 'header_padding':
                    f.put(f.BASE + 0x3ff, b'X')
                else:
                    f.put(f.BASE + 0x188 + 12 * 40 + offset, struct.pack('<' + fmt, value))
                self.result(f, False, ['root'])
                self.assertFalse(f.bridges[0].allocations)
                self.assertFalse(f.bridges[0].submissions)

    def test_empty_raw_pointer_requires_the_exact_recorded_disk_row(self):
        f = BoneMemory()
        f.reviewed_empty_raw_pointer_layout()
        f.module['sections'][12]['raw_pointer'] += 512
        f.header, f.module['disk_size'] = pe_headers(f.module['sections'], image_size=f.module['size'])
        f.module['headers'] = f.header
        live = bytearray(f.header)
        struct.pack_into('<I', live, 0x188 + 12 * 40 + 20, 0)
        f.put(f.BASE, live)
        self.result(f, False, ['root'])
        self.assertFalse(f.bridges[0].submissions)

    def test_empty_raw_pointer_live_bytes_must_not_change_during_a_getter(self):
        f = BoneMemory()
        f.reviewed_empty_raw_pointer_layout()
        def configure(bridge):
            bridge.on_submit = lambda *_: f.put(f.BASE + 0x188 + 12 * 40 + 20, struct.pack('<I', 11354624))
        f.configure_bridge = configure
        row = self.result(f, False, ['root'])
        self.assertEqual(len(f.bridges[0].submissions), 1)
        self.assertEqual(len(f.bridges[0].freed), 1)
        self.assertFalse(row['bones'])

    def test_empty_raw_pointer_does_not_authorize_mismatched_current_getter_code(self):
        f = BoneMemory()
        f.reviewed_empty_raw_pointer_layout()
        f.put(f.BASE + 0x1000 + 31, b'X')
        self.result(f, False, ['root'])
        self.assertFalse(f.bridges[0].submissions)

    def test_reviewed_runtime_resource_relocation_preserves_disk_and_live_provenance(self):
        for relocated in (False, True):
            with self.subTest(relocated=relocated):
                f = BoneMemory()
                if relocated:
                    f.reviewed_resource_layout()
                row = self.result(f, True, ['root'])
                module = row['calls'][0]['function']['source']['module']
                evidence = module['pe_validation']
                self.assertEqual(module['disk_sha256'], f.profile['sha256'])
                self.assertEqual(module['sha256_scope'], 'on_disk_executable')
                self.assertEqual(evidence['disk_header_sha256'], hashlib.sha256(f.header).hexdigest())
                self.assertEqual(evidence['live_header_sha256'], hashlib.sha256(f.read(f.BASE, len(f.header))).hexdigest())
                self.assertTrue(evidence['non_resource_header_bytes_equal'])
                self.assertTrue(evidence['live_header_closing_reread_required'])
                self.assertEqual(evidence['fields']['machine'], 0x8664)
                if relocated:
                    self.assertEqual(evidence['resource_policy'], 'reviewed_runtime_resource_relocation')
                    self.assertEqual(evidence['disk_resource'], dict(rva=0x2013000, size=0x226cc))
                    self.assertEqual(evidence['live_resource'], dict(rva=0x1eae000, size=0xb37e8))
                    self.assertEqual(evidence['disk_import'], dict(rva=0x2011755, size=0x6b8))
                    self.assertEqual(evidence['live_import'], evidence['disk_import'])
                    self.assertNotEqual(evidence['disk_header_sha256'], evidence['live_header_sha256'])
                else:
                    self.assertEqual(evidence['resource_policy'], 'unchanged_disk_resource')
                    self.assertEqual(evidence['disk_header_sha256'], evidence['live_header_sha256'])
                self.assertGreater(f.read_counts[f.BASE, len(f.header)], 2)

    def test_resource_exemption_requires_exact_reviewed_disk_identity_and_directory_layout(self):
        for defect in ('unknown_disk_sha', 'different_disk_resource', 'different_live_rva',
                       'different_live_size', 'zero_rva', 'zero_size', 'outside_image',
                       'overflow', 'different_section_extent'):
            with self.subTest(defect=defect):
                f = BoneMemory()
                f.reviewed_resource_layout()
                if defect == 'unknown_disk_sha':
                    f.profile['sha256'] = f.module['sha256'] = f.certificates['game_sha256'] = 'a' * 64
                elif defect == 'different_disk_resource':
                    raw = bytearray(f.header)
                    struct.pack_into('<II', raw, 0x118, 0x2013014, 0x226b8)
                    f.module['headers'] = bytes(raw)
                elif defect == 'different_section_extent':
                    f.module['sections'][2]['virtual_size'] += 0x1000
                    f.module['sections'][2]['size'] += 0x1000
                    f.header, _ = pe_headers(f.module['sections'], image_size=f.module['size'],
                                            import_directory=(0x2011755, 0x6b8),
                                            resource_directory=(0x2013000, 0x226cc))
                    f.module['headers'] = f.header
                    f.put(f.BASE, f.header)
                    f.put(f.BASE + 0x118, struct.pack('<II', 0x1eae000, 0xb37e8))
                else:
                    values = {'different_live_rva': (0x1eae014, 0xb37d4),
                              'different_live_size': (0x1eae000, 0xb37e7),
                              'zero_rva': (0, 0xb37e8), 'zero_size': (0x1eae000, 0),
                              'outside_image': (f.module['size'] - 1, 2),
                              'overflow': (0xfffffff0, 0x40)}
                    f.put(f.BASE + 0x118, struct.pack('<II', *values[defect]))
                row = self.result(f, False, ['root'])
                self.assertIn('Resource', row['rejection'])
                self.assertFalse(f.bridges[0].submissions)
                self.assertFalse(f.bridges[0].allocations)

    def test_reviewed_resource_change_never_masks_other_header_or_section_changes(self):
        changes = (('dos', 0, 'H', 0), ('nt_offset', 0x3c, 'I', 0xfffffffc),
                   ('pe_signature', 0x80, 'I', 0), ('machine', 0x84, 'H', 0x14c),
                   ('section_count', 0x86, 'H', 4), ('timestamp', 0x88, 'I', 1),
                   ('optional_size', 0x94, 'H', 232), ('characteristics', 0x96, 'H', 0x23),
                   ('magic', 0x98, 'H', 0x10b), ('entry', 0xa8, 'I', 0x1001),
                   ('image_base', 0xb0, 'Q', 0x150000000), ('section_alignment', 0xb8, 'I', 0x2000),
                   ('file_alignment', 0xbc, 'I', 0x400), ('image_size', 0xd0, 'I', 0x2041000),
                   ('header_size', 0xd4, 'I', 0x200), ('checksum', 0xd8, 'I', 1),
                   ('subsystem', 0xdc, 'H', 3), ('dll_flags', 0xde, 'H', 0),
                   ('directory_count', 0x104, 'I', 15), ('another_directory', 0x108, 'I', 0x1000),
                   ('import_rva', 0x110, 'I', 0x2011754), ('import_size', 0x114, 'I', 0x6b7),
                   ('dos_stub', 0x70, 'B', 1), ('header_padding', 0x3ff, 'B', 1),
                   ('section_name', 0x188, 'B', ord('X')), ('section_virtual_size', 0x190, 'I', 0x2100),
                   ('section_rva', 0x194, 'I', 0x2000), ('section_raw_size', 0x198, 'I', 0x1e00),
                   ('section_raw_pointer', 0x19c, 'I', 0x600), ('section_flags', 0x1ac, 'I', 0xe0000020))
        for label, offset, fmt, value in changes:
            with self.subTest(field=label):
                f = BoneMemory()
                f.reviewed_resource_layout()
                f.put(f.BASE + offset, struct.pack('<' + fmt, value))
                row = self.result(f, False, ['root'])
                self.assertIn('module', row['rejection'])
                self.assertFalse(f.bridges[0].submissions)
                self.assertFalse(f.bridges[0].allocations)

    def test_matching_but_malformed_disk_and_live_headers_still_fail_structural_bounds(self):
        for defect in ('section_table_truncated', 'image_too_small', 'section_overlap', 'raw_past_file',
                       'raw_overflow', 'entry_in_gap', 'import_in_gap', 'unaligned_rva'):
            with self.subTest(defect=defect):
                f = BoneMemory()
                raw = bytearray(f.header)
                if defect == 'section_table_truncated': struct.pack_into('<H', raw, 0x86, 96)
                elif defect == 'image_too_small': struct.pack_into('<I', raw, 0xd0, 0x5000)
                elif defect == 'section_overlap': struct.pack_into('<I', raw, 0x188 + 40 + 12, 0x2000)
                elif defect == 'raw_past_file': f.module['disk_size'] -= 1
                elif defect == 'raw_overflow': struct.pack_into('<I', raw, 0x19c, 0xfffffe00)
                elif defect == 'entry_in_gap': struct.pack_into('<I', raw, 0xa8, 0x3000)
                elif defect == 'import_in_gap': struct.pack_into('<II', raw, 0x110, 0x3000, 20)
                elif defect == 'unaligned_rva': struct.pack_into('<I', raw, 0x194, 0x1001)
                f.module['headers'] = bytes(raw)
                f.put(f.BASE, raw)
                row = self.result(f, False, ['root'])
                self.assertIn('module', row['rejection'])
                self.assertFalse(f.bridges[0].submissions)
                self.assertFalse(f.bridges[0].allocations)

    def test_live_header_is_reread_after_request_write_and_after_getter_completion(self):
        for stage in ('write', 'completion'):
            with self.subTest(stage=stage):
                f = BoneMemory()
                f.reviewed_resource_layout()
                def change_header():
                    # Even another independently permitted Resource state cannot
                    # replace the exact live bytes within one getter interval.
                    f.put(f.BASE + 0x118, struct.pack('<II', 0x2013000, 0x226cc))
                def configure(bridge):
                    if stage == 'completion':
                        bridge.on_submit = lambda *_: change_header()
                    else:
                        original = bridge.write
                        def write_then_change(address, raw):
                            original(address, raw)
                            change_header()
                        bridge.write = write_then_change
                f.configure_bridge = configure
                row = self.result(f, False, ['root'])
                bridge = f.bridges[0]
                self.assertEqual(len(bridge.submissions), int(stage == 'completion'))
                self.assertEqual(len(bridge.freed), int(stage == 'completion'))
                self.assertEqual(len(row['retained_allocations']), int(stage == 'write'))
                self.assertEqual(row['bones'], [])
                self.assertFalse(row['calls'][0]['accepted'])

    def test_all_eight_reviewed_getter_chunks_remain_exact_current_and_closing_witnesses(self):
        for stage in ('before', 'completion'):
            for selected in range(8):
                with self.subTest(stage=stage, chunk=selected):
                    f = BoneMemory()
                    chunks = []
                    for i, (name, function) in enumerate(f.certificates['functions'].items()):
                        for extra in range(2):
                            rva, data = 0x1400 + i * 0x200 + extra * 0x100, bytes([0xa0 + i + extra]) * 16
                            f.put(f.BASE + rva, data)
                            function['chunks'].append(dict(rva=hex(rva), bytes=data.hex(), sha256=hashlib.sha256(data).hexdigest()))
                        chunks.extend((name, c) for c in function['chunks'])
                    name, selected_chunk = chunks[selected]
                    target = f.BASE + int(selected_chunk['rva'], 16)
                    fn_address = f.MATCH if name == 'MatchRefBone' else f.MATRIX_FN
                    if stage == 'before':
                        f.put(target, b'X')
                    else:
                        def mutate(bridge, address):
                            if struct.unpack_from('<Q', bridge.allocations[address], 16)[0] == fn_address:
                                f.put(target, b'X')
                        f.configure_bridge = lambda b: setattr(b, 'on_submit', mutate)
                    row = self.result(f, False, ['root'])
                    expected_calls = int(name == 'GetBoneMatrix') + int(stage == 'completion')
                    self.assertEqual(len(f.bridges[0].submissions), expected_calls)
                    self.assertEqual(len(f.bridges[0].freed), expected_calls)
                    self.assertFalse(row['retained_allocations'])

    def test_non_declaring_ancestors_do_not_add_thousands_of_unrelated_closing_witnesses(self):
        f, queries, unrelated_links = BoneMemory(), [], []
        # The retail graph has large derived classes above the classes that
        # actually declare Pawn/WorldInfo/Controller/Mesh. These siblings are
        # irrelevant to those inherited properties and to the mesh getters.
        for group, (obj, parent) in enumerate(((f.PC, f.PC_CLASS), (f.PAWN, f.PAWN_CLASS))):
            owner = 0x180000 + group * 0x1000
            f.add_class(owner, 'Derived' + str(group), parent)
            f.objects[obj]['cls'] = owner
            addresses = [0x300000 + group * 0x100000 + index * 0x100 for index in range(2048)]
            f.put(owner + f.profile['struct_children'], struct.pack('<Q', addresses[0]))
            for index, (address, nxt) in enumerate(zip(addresses, addresses[1:] + [0])):
                f.add_object(address, 'Unrelated' + str(index), f.FUNCTION_CLASS, outer=owner)
                link = address + f.profile['field_next']
                f.put(link, struct.pack('<Q', nxt))
                unrelated_links.append((link, 8))
        def factory(observer):
            query = bone_query.CoverQuery(observer)
            queries.append(query)
            return query
        row = f.capture(['root'], query_factory=factory)
        self.assertTrue(row['accepted'], row.get('rejection'))
        self.assertEqual(len(f.bridges[0].submissions), 2)
        self.assertFalse(any(link in f.read_counts for link in unrelated_links))
        self.assertLess(max(query.reads for query in queries), 1024)

    def test_child_list_memo_retains_first_links_for_closing_validation(self):
        f = BoneMemory()
        q = bone_query.CoverQuery(f.observer)
        owner = q.obj(f.MESH_CLASS)
        children = bone_query._children(q, owner)
        reads = q.reads
        self.assertIs(bone_query._children(q, owner), children)
        self.assertEqual(q.reads, reads)
        f.put(f.MATCH + f.profile['field_next'], bytes(8))
        with self.assertRaisesRegex(RuntimeError, 'Cover value changed'):
            q.finish()

    def test_eight_bone_limit_uses_sixteen_distinct_inline_name_requests(self):
        f = BoneMemory()
        f.bone_names += ['neck', 'left_hand', 'right_hand', 'pelvis']
        f.put(f.names_header, struct.pack('<Qii', f.NAME_DATA, len(f.bone_names), 16))
        for index, name in enumerate(f.bone_names[5:], 5):
            entry = 0x160000 + index * 0x100
            f.put(f.NAME_DATA + index * 8, struct.pack('<Q', entry))
            f.put(entry, bytes(148))
            f.put(entry, struct.pack('<I', index) + bytes(16) + (name + '\0').encode('utf-16-le'))
        row = self.result(f, True, f.bone_names[1:])
        self.assertEqual(len(row['bones']), 8)
        self.assertEqual(len(f.bridges[0].submissions), 16)
        self.assertEqual(len(set(f.bridges[0].freed)), 16)
        self.assertFalse(row['retained_allocations'])

    def test_authorization_readiness_names_and_hash_reject_before_transport(self):
        for defect in ('reader', 'sampler', 'owner', 'session_owner', 'session_game', 'session_id',
                       'empty', 'too_many', 'duplicate', 'nul', 'utf16_length', 'name_type', 'missing_certificate'):
            with self.subTest(defect=defect):
                f, names = BoneMemory(), ['root']
                if defect == 'reader': f.observer.resolver = None
                elif defect == 'sampler': f.observer.sampler = None
                elif defect == 'owner': f.observer.action_owner = dict(pid=99)
                elif defect == 'session_owner': f.observer.session['owner_identity']['pid'] = 99
                elif defect == 'session_game': f.observer.session['game_identity']['pid'] = 99
                elif defect == 'session_id': f.observer.session['session_id'] = ''
                elif defect == 'empty': names = []
                elif defect == 'too_many': names = list('abcdefghi')
                elif defect == 'duplicate': names = ['root', 'root']
                elif defect == 'nul': names = ['root\0']
                elif defect == 'utf16_length': names = ['\U0001f600' * 32]
                elif defect == 'name_type': names = [False]
                elif defect == 'missing_certificate': f.certificates['functions'] = {}
                self.result(f, False, names)
                self.assertFalse(f.bridges)
        f = BoneMemory()
        row = capture(f.observer, 'unused.dll', 'BAD', ['root'], bridge_factory=f.factory)
        self.assertFalse(row['accepted'])
        self.assertFalse(f.bridges)

    def test_body_mesh_world_asset_and_space_bounds_are_live_guards(self):
        for defect in ('possession', 'world', 'mesh_owner', 'mesh_class', 'asset_class', 'template_mesh',
                       'zero_count', 'oversized', 'invalid_capacity', 'space_inner_class'):
            with self.subTest(defect=defect):
                f = BoneMemory()
                if defect == 'possession': f.set_value(f.PAWN, 'Controller', f.OTHER, '<Q')
                elif defect == 'world': f.set_value(f.PAWN, 'WorldInfo', f.OTHER, '<Q')
                elif defect == 'mesh_owner': f.set_value(f.MESH, 'Owner', f.OTHER, '<Q')
                elif defect == 'mesh_class': f.objects[f.MESH]['cls'] = f.LEAF_CLASS
                elif defect == 'asset_class': f.objects[f.ASSET]['cls'] = f.LEAF_CLASS
                elif defect == 'template_mesh': f.objects[f.MESH]['name'] = 'Default__PawnMesh'
                elif defect in ('zero_count', 'oversized', 'invalid_capacity'):
                    count, capacity = {'zero_count': (0, 0), 'oversized': (513, 513), 'invalid_capacity': (4, 3)}[defect]
                    f.put(f.value_address(f.MESH, 'SpaceBases'), struct.pack('<Qii', 0x999000, count, capacity))
                elif defect == 'space_inner_class':
                    field = f.properties[f.MESH_CLASS, 'SpaceBases']
                    inner = f.u64(field['address'] + f.profile['struct_property_struct'])
                    f.objects[inner]['cls'] = f.kind_classes['IntProperty']
                self.result(f, False)
                self.assertFalse(f.bridges)

    def test_function_identity_layout_and_known_wrapper_bytes_gate_submission(self):
        defects = ('fn_class', 'fn_outer', 'child_owner', 'child_name', 'extra_child', 'field_class',
                   'dim', 'input_width', 'output_width', 'input_type', 'input_range', 'overlap',
                   'returnoff', 'parms_zero', 'parms_large', 'native_index', 'entry_outside',
                   'entry_data', 'entry_changed', 'cert_module',
                   'reader_module', 'reader_pid', 'pe_header')
        for defect in defects:
            with self.subTest(defect=defect):
                f = BoneMemory()
                arg = f.properties[f.MATCH, 'BoneName']
                ret = f.properties[f.MATCH, 'ReturnValue']
                if defect == 'fn_class': f.objects[f.MATCH]['cls'] = f.STRUCT_CLASS
                elif defect == 'fn_outer': f.objects[f.MATCH]['outer'] = f.ASSET_CLASS
                elif defect == 'child_owner': f.objects[arg['address']]['outer'] = f.MATRIX_FN
                elif defect == 'child_name': f.objects[arg['address']]['name'] = 'Other'
                elif defect == 'extra_child': f.put(ret['address'] + 0x68, struct.pack('<Q', f.MATRIX_FN))
                elif defect == 'field_class': f.objects[arg['address']]['cls'] = f.kind_classes['IntProperty']
                elif defect == 'dim': f.put(arg['address'] + 0x70, struct.pack('<I', 2))
                elif defect == 'input_width': f.layout(f.MATCH, 'BoneName', size=4)
                elif defect == 'output_width': f.layout(f.MATCH, 'ReturnValue', size=8)
                elif defect == 'input_type': f.layout(f.MATCH, 'BoneName', kind='IntProperty')
                elif defect == 'input_range': f.layout(f.MATCH, 'BoneName', offset=20)
                elif defect == 'overlap': f.layout(f.MATCH, 'BoneName', offset=8)
                elif defect == 'returnoff': f.put(f.MATCH + 0x108, struct.pack('<H', 16))
                elif defect == 'parms_zero': f.put(f.MATCH + 0x106, bytes(2))
                elif defect == 'parms_large': f.put(f.MATCH + 0x106, struct.pack('<H', 4097))
                elif defect == 'native_index': f.put(f.MATCH + 0xf8, struct.pack('<H', 1))
                elif defect == 'entry_outside': f.put(f.MATCH + 0x114, struct.pack('<Q', f.BASE + 0x10000))
                elif defect == 'entry_data':
                    f.put(f.MATCH + 0x114, struct.pack('<Q', f.BASE + 0x4000))
                    f.certificates['functions']['MatchRefBone']['entry_rva'] = '0x4000'
                    f.certificates['functions']['MatchRefBone']['chunks'][0]['rva'] = '0x4000'
                elif defect == 'entry_changed': f.put(f.BASE + 0x1000, b'X')
                elif defect == 'cert_module': f.certificates['game_sha256'] = '0' * 64
                elif defect == 'reader_module': f.base += 0x1000
                elif defect == 'reader_pid': f.pid += 1
                elif defect == 'pe_header': f.put(f.BASE, b'ZZ')
                row = self.result(f, False, ['root'])
                self.assertFalse(any(b.submissions for b in f.bridges))
                self.assertFalse(row['retained_allocations'])

    def test_matrix_nested_shape_must_be_authentic_even_when_outer_is_64_bytes(self):
        for defect in ('type', 'plane_type', 'plane_offset', 'plane_width', 'float_kind', 'float_offset', 'float_width', 'matrix_width'):
            with self.subTest(defect=defect):
                f = BoneMemory()
                if defect == 'type': f.objects[f.MATRIX]['name'] = 'Vector4x4'
                elif defect == 'plane_type': f.objects[f.PLANE]['name'] = 'Vector4'
                elif defect == 'plane_offset': f.layout(f.MATRIX, 'YPlane', offset=20)
                elif defect == 'plane_width': f.layout(f.MATRIX, 'YPlane', size=12)
                elif defect == 'float_kind': f.layout(f.VECTOR, 'X', kind='IntProperty')
                elif defect == 'float_offset': f.layout(f.PLANE, 'W', offset=8)
                elif defect == 'float_width': f.layout(f.PLANE, 'W', size=8)
                elif defect == 'matrix_width': f.layout(f.MATRIX_FN, 'ReturnValue', size=48)
                self.result(f, False, ['root'])
                self.assertEqual(len(f.bridges[0].submissions), 1)

    def test_exact_fname_cannot_use_prefix_wrong_number_or_stale_cache_as_proof(self):
        f = BoneMemory()
        self.result(f, False, ['roo'])
        self.assertFalse(f.bridges)
        for defect in ('text', 'entry_index', 'pointer'):
            with self.subTest(defect=defect):
                f = BoneMemory()
                if defect == 'text':
                    f.names_cache[1] = 'root'
                    f.put(0x160100 + 20, 'roOt\0'.encode('utf-16-le'))
                elif defect == 'entry_index': f.put(0x160100, struct.pack('<I', 9))
                elif defect == 'pointer': f.put(f.NAME_DATA + 8, struct.pack('<Q', 0x160400))
                row = self.result(f, False, ['root'])
                self.assertFalse(any(b.submissions for b in f.bridges))
                self.assertFalse(row['retained_allocations'])
        f = BoneMemory()
        f.names_cache[4] = 'root'  # A stale cached spelling cannot select index 4.
        row = self.result(f, True, ['root'])
        self.assertEqual(row['calls'][0]['bone_fname']['index'], 1)

    def test_unrelated_long_fname_does_not_block_short_exact_targets(self):
        f = BoneMemory()
        f.put(0x160000 + 20, ('X' * 80 + '\0').encode('utf-16-le'))
        def reject_global_name_decoder(index):
            raise RuntimeError('Name exceeds bounded UTF16 read')
        f.name = reject_global_name_decoder
        row = self.result(f, True, ['root', 'head'])
        self.assertEqual([c['bone_fname']['name'] for c in row['calls'] if 'bone_fname' in c], ['root', 'head'])
        self.assertNotIn((0x160000 + 20, 128), f.read_counts)

    def test_selected_overlong_or_unterminated_fname_is_not_a_prefix_match(self):
        for defect in ('overlong_input', 'overlong_entry', 'missing_terminator'):
            with self.subTest(defect=defect):
                f, names = BoneMemory(), ['root']
                if defect == 'overlong_input': names = ['x' * 64]
                elif defect == 'overlong_entry': f.put(0x160100 + 20, ('root' + 'X' * 64 + '\0').encode('utf-16-le'))
                else: f.put(0x160100 + 20, 'rootX'.encode('utf-16-le'))
                row = self.result(f, False, names)
                self.assertFalse(f.bridges)
                self.assertFalse(row['calls'])
                if defect != 'overlong_input':
                    q = bone_query.CoverQuery(f.observer)
                    with self.assertRaisesRegex(RuntimeError, 'exact spelling changed'):
                        bone_query._name(q, 'root', 1)

    def test_invalid_match_indices_and_nonfinite_matrix_stop_after_completed_free(self):
        for value in (-1, 100, 512, 2147483647):
            with self.subTest(index=value):
                f = BoneMemory()
                f.configure_bridge = lambda b, v=value: setattr(b, 'index', v)
                row = self.result(f, False, ['root'])
                self.assertEqual(len(f.bridges[0].submissions), 1)
                self.assertEqual(len(f.bridges[0].freed), 1)
                self.assertFalse(row['retained_allocations'])
        f = BoneMemory()
        f.configure_bridge = lambda b: b.matrix.__setitem__(3, float('nan'))
        row = self.result(f, False, ['root'])
        self.assertEqual(len(f.bridges[0].freed), 2)
        self.assertEqual(row['bones'], [])

    def test_unsubmitted_allocation_write_and_submit_failures_remain_distinct(self):
        for stage, allocated, submitted, pending in (('allocate', False, False, False),
                                                    ('write', True, False, False),
                                                    ('create_thread', True, True, False),
                                                    ('submit_timeout', True, True, True)):
            with self.subTest(stage=stage):
                f = BoneMemory()
                f.configure_bridge = lambda b, s=stage: setattr(b, 'error_at', s)
                row = self.result(f, False, ['root'])
                bridge = f.bridges[0]
                self.assertEqual(bool(bridge.allocations), allocated)
                self.assertEqual(bool(bridge.submissions), submitted)
                self.assertFalse(bridge.freed)
                self.assertEqual(bool(row['retained_allocations']), allocated)
                if allocated:
                    self.assertIs(row['retained_allocations'][0]['pending_possible'], pending)
                    self.assertIn('ORIGINAL process exits', row['retained_allocations'][0]['release_condition'])

    def test_busy_invalid_and_unknown_submit_returns_do_not_retry_or_free(self):
        for status in (1, 2, 99):
            with self.subTest(status=status):
                f = BoneMemory()
                f.configure_bridge = lambda b, s=status: setattr(b, 'submit_result', s)
                row = self.result(f, False, ['root'])
                self.assertEqual(row['calls'][0]['submit_result'], status)
                self.assertEqual(len(f.bridges[0].submissions), 1)
                self.assertFalse(f.bridges[0].freed)
                self.assertEqual(len(row['retained_allocations']), 1)

    def test_pending_and_failed_queue_states_keep_storage_after_close(self):
        for state in (0, -1, 1, 3):
            with self.subTest(state=state):
                f = BoneMemory()
                f.configure_bridge = lambda b, s=state: setattr(b, 'queue_state', s)
                # Deterministically exhaust the per-call poll interval without
                # spending real seconds; metadata Query still uses a real clock.
                counter = [0.]
                def tick():
                    counter[0] += .1
                    return counter[0]
                with patch.object(bone_query, 'time', SimpleNamespace(monotonic=tick, perf_counter_ns=lambda: 10,
                                                                      sleep=lambda _: None)):
                    row = self.result(f, False, ['root'])
                self.assertEqual(len(f.bridges[0].submissions), 1)
                self.assertEqual(row['calls'][0]['queue_state'], state)
                self.assertFalse(f.bridges[0].freed)
                self.assertEqual(len(row['retained_allocations']), 1)

    def test_closing_metadata_owner_and_values_races_reject_partial_diagnostics(self):
        for defect in ('metadata', 'mesh_owner', 'space_count', 'entry_prefix', 'name_text', 'registry'):
            with self.subTest(defect=defect):
                f = BoneMemory()
                def mutate(bridge, address, d=defect):
                    if d == 'metadata': f.layout(f.MATCH, 'BoneName', offset=4)
                    elif d == 'mesh_owner': f.set_value(f.MESH, 'Owner', f.OTHER, '<Q')
                    elif d == 'space_count':
                        f.put(f.value_address(f.MESH, 'SpaceBases') + 8, struct.pack('<i', 99))
                    elif d == 'entry_prefix': f.put(f.BASE + 0x1000, b'X')
                    elif d == 'name_text': f.put(0x160100 + 20, 'ROOT\0'.encode('utf-16-le'))
                    elif d == 'registry': f.put(f.names_header + 8, struct.pack('<i', 6))
                f.configure_bridge = lambda b: setattr(b, 'on_submit', mutate)
                row = self.result(f, False, ['root'])
                self.assertEqual(len(f.bridges[0].submissions), 1)
                self.assertEqual(len(f.bridges[0].freed), 1)
                self.assertEqual(row['bones'], [])
                self.assertFalse(row['calls'][0]['accepted'])

    def test_metadata_change_after_private_buffer_write_prevents_submit(self):
        f = BoneMemory()
        def configure(bridge):
            write = bridge.write
            def write_then_change(address, raw):
                write(address, raw)
                f.put(f.MATCH + 0xf8, struct.pack('<H', 1))
            bridge.write = write_then_change
        f.configure_bridge = configure
        row = self.result(f, False, ['root'])
        self.assertFalse(f.bridges[0].submissions)
        self.assertEqual(len(row['retained_allocations']), 1)
        self.assertFalse(row['retained_allocations'][0]['pending_possible'])

    def test_lifecycle_loss_after_submission_retains_unknown_completion_and_stops(self):
        f = BoneMemory()
        def on_submit(bridge, address):
            f.observer.action_owner = dict(pid=999)
        f.configure_bridge = lambda b: setattr(b, 'on_submit', on_submit)
        row = self.result(f, False, ['root'])
        self.assertFalse(row['calls'][0]['completion_observed'])
        self.assertEqual(len(row['retained_allocations']), 1)
        self.assertFalse(f.bridges[0].freed)

    def test_last_getter_cannot_accept_replaced_session_even_when_owner_and_pid_match(self):
        f = BoneMemory()
        def replace_on_matrix(bridge, address):
            fn = struct.unpack_from('<Q', bridge.allocations[address], 16)[0]
            if fn == f.MATRIX_FN:
                f.observer.session['session_id'] = 'new-session-same-process'
        f.configure_bridge = lambda b: setattr(b, 'on_submit', replace_on_matrix)
        row = self.result(f, False, ['root'])
        self.assertEqual(len(f.bridges[0].submissions), 2)
        self.assertEqual(len(f.bridges[0].freed), 1)
        self.assertEqual(len(row['retained_allocations']), 1)
        self.assertTrue(row['calls'][0]['accepted'])
        self.assertFalse(row['calls'][1]['accepted'])
        self.assertEqual(row['session_id'], 'owned-session')

    def test_request_completion_does_not_authenticate_mutated_object_or_argument(self):
        for location in (8, 24):
            with self.subTest(location=location):
                f = BoneMemory()
                def corrupt(bridge, address, size, offset=location):
                    if size == bone_query.REQUEST_SIZE:
                        bridge.allocations[address][offset] ^= 1
                f.configure_bridge = lambda b: setattr(b, 'on_read', corrupt)
                row = self.result(f, False, ['root'])
                self.assertEqual(len(f.bridges[0].freed), 1)
                self.assertFalse(row['retained_allocations'])

    def test_fresh_query_for_every_getter_and_no_same_pose_claim(self):
        f, queries = BoneMemory(), []
        def factory(observer):
            q = bone_query.CoverQuery(observer)
            queries.append(q)
            return q
        row = f.capture(query_factory=factory)
        self.assertTrue(row['accepted'], row.get('rejection'))
        self.assertEqual(len(queries), 5)  # Baseline plus four independent calls.
        self.assertEqual(len(set(id(q) for q in queries)), 5)
        self.assertFalse(row['same_pose_epoch_verified'])

    def test_certificate_schema_hashes_ranges_and_complete_function_set_are_required(self):
        defects = ('schema', 'kind', 'function_missing', 'function_extra', 'return_bool', 'return_width',
                   'empty_chunks', 'too_many_chunks', 'sha', 'bad_hex', 'oversize_chunk', 'entry_missing',
                   'overlap', 'decimal_rva', 'negative_rva', 'large_rva')
        for defect in defects:
            with self.subTest(defect=defect):
                f = BoneMemory()
                c = f.certificates
                fn = c['functions']['MatchRefBone']
                chunk = fn['chunks'][0]
                if defect == 'schema': c['schema'] = True
                elif defect == 'kind': c['kind'] = 'stable_prefix_only'
                elif defect == 'function_missing': del c['functions']['GetBoneMatrix']
                elif defect == 'function_extra': c['functions']['SetBoneMatrix'] = copy.deepcopy(fn)
                elif defect == 'return_bool': fn['return_bytes'] = True
                elif defect == 'return_width': fn['return_bytes'] = 8
                elif defect == 'empty_chunks': fn['chunks'] = []
                elif defect == 'too_many_chunks': fn['chunks'] *= 5
                elif defect == 'sha': chunk['sha256'] = '0' * 64
                elif defect == 'bad_hex': chunk['bytes'] = 'not hexadecimal'
                elif defect == 'oversize_chunk':
                    raw = b'X' * 8193
                    chunk.update(bytes=raw.hex(), sha256=hashlib.sha256(raw).hexdigest())
                elif defect == 'entry_missing': fn['entry_rva'] = '0x1001'
                elif defect == 'overlap': fn['chunks'][1]['rva'] = '0x1010'
                elif defect == 'decimal_rva': fn['entry_rva'] = '4096'
                elif defect == 'negative_rva': chunk['rva'] = '0x-1'
                elif defect == 'large_rva': chunk['rva'] = '0x100000000'
                self.result(f, False, ['root'])
                self.assertFalse(f.bridges)
        f = BoneMemory()
        row = capture(f.observer, 'unused.dll', f.DLL_SHA, ['root'], bridge_factory=f.factory)
        self.assertFalse(row['accepted'])
        self.assertIn('certificate_path', row['rejection'])
        self.assertFalse(f.bridges)

    def test_noncontiguous_audited_tail_and_property_links_are_closing_witnesses(self):
        for defect in ('cold_before', 'cold_during', 'property_unlinked_before', 'property_unlinked_during', 'property_ancestry'):
            with self.subTest(defect=defect):
                f = BoneMemory()
                arg = f.properties[f.MATCH, 'BoneName']
                if defect == 'cold_before': f.put(f.BASE + 0x1200, b'X')
                elif defect == 'property_unlinked_before':
                    f.put(f.MATCH + 0x80, struct.pack('<Q', f.properties[f.MATCH, 'ReturnValue']['address']))
                elif defect == 'property_ancestry':
                    f.put(f.kind_classes['NameProperty'] + 0x60, bytes(8))
                else:
                    def mutate(bridge, address, d=defect):
                        if d == 'cold_during': f.put(f.BASE + 0x1200, b'X')
                        else: f.put(f.MATCH + 0x80, struct.pack('<Q', f.properties[f.MATCH, 'ReturnValue']['address']))
                    f.configure_bridge = lambda b: setattr(b, 'on_submit', mutate)
                row = self.result(f, False, ['root'])
                bridge = f.bridges[0]
                self.assertEqual(len(bridge.submissions), 1 if defect.endswith('during') else 0)
                self.assertFalse(row['retained_allocations'])

    def test_each_getter_recertifies_mesh_asset_and_lifecycle_instead_of_reusing_match(self):
        for defect in ('asset', 'session'):
            with self.subTest(defect=defect):
                f = BoneMemory()
                def change_after_match(bridge, address, d=defect):
                    original_free = bridge.free
                    def free_then_change(pointer):
                        original_free(pointer)
                        if d == 'asset': f.set_value(f.MESH, 'SkeletalMesh', f.OTHER, '<Q')
                        else: f.observer.session['session_id'] = 'replacement-session'
                    bridge.free = free_then_change
                f.configure_bridge = lambda b: setattr(b, 'on_submit', change_after_match)
                row = self.result(f, False, ['root'])
                self.assertTrue(row['calls'][0]['accepted'])
                self.assertEqual(len(f.bridges[0].submissions), 1)
                self.assertEqual(row['bones'], [])
                self.assertFalse(row['retained_allocations'])

    def test_total_capture_budget_stops_before_new_allocation_or_dispatch(self):
        f = BoneMemory()
        clock = [0.]
        def expires_during_attach(*args, **kwargs):
            bridge = f.factory(*args, **kwargs)
            clock[0] = 14.1
            return bridge
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'certificate.json'
            path.write_text(json.dumps(f.certificates))
            with patch.object(bone_query, 'time', SimpleNamespace(monotonic=lambda: clock[0],
                                                                  perf_counter_ns=lambda: int(clock[0] * 1e9), sleep=lambda _: None)):
                row = capture(f.observer, 'fake.dll', f.DLL_SHA, ['root'], certificate_path=path,
                              bridge_factory=expires_during_attach)
        self.assertFalse(row['accepted'])
        self.assertIn('time budget', row['rejection'])
        self.assertFalse(f.bridges[0].allocations)
        self.assertFalse(f.bridges[0].submissions)


class AttachedTransportTests(unittest.TestCase):
    def test_unknown_pe_header_becomes_rejected_json_before_transport(self):
        class PEFormatError(Exception):
            pass
        def bad_bridge(*args, **kwargs):
            raise PEFormatError('unsupported PE header')
        runtime = SimpleNamespace(Bridge=bad_bridge, pefile=SimpleNamespace(PEFormatError=PEFormatError))
        spec = SimpleNamespace(loader=SimpleNamespace(exec_module=lambda _: None))
        f = BoneMemory()
        f.factory = bone_query._AttachedBridge
        with patch.object(bone_query.importlib.util, 'spec_from_file_location', return_value=spec), \
                patch.object(bone_query.importlib.util, 'module_from_spec', return_value=runtime):
            row = f.capture(['root'])
        self.assertFalse(row['accepted'])
        self.assertIn('PE format rejected', row['rejection'])
        self.assertFalse(row['calls'])
        self.assertFalse(row['retained_allocations'])

    def transport(self, *, create=123, wait=0, exit_ok=True, allocated=0x400000):
        events = []
        def get_exit(thread, code):
            code._obj.value = 0
            return exit_ok
        k = SimpleNamespace(VirtualAllocEx=lambda *args: allocated,
                            CreateRemoteThread=lambda *args: events.append(('create', args)) or create,
                            WaitForSingleObject=lambda thread, timeout: events.append(('wait', timeout)) or wait,
                            GetExitCodeThread=get_exit, CloseHandle=lambda thread: events.append(('close', thread)))
        obj = bone_query._AttachedBridge.__new__(bone_query._AttachedBridge)
        obj.runtime = SimpleNamespace(k=k)
        obj.bridge = SimpleNamespace(verify=lambda: None, handle=99,
                                     r=SimpleNamespace(read=lambda address, size: b'X' * size))
        obj.submit_source = dict(entry=0x123000, bytes=(b'X' * 32).hex())
        return obj, events

    def test_existing_export_is_called_once_with_bounded_wait_and_thread_handle_closed(self):
        obj, events = self.transport()
        self.assertEqual(obj.allocate(4120), 0x400000)
        self.assertEqual(obj.submit(0x400000, 37), 0)
        self.assertEqual([x[0] for x in events], ['create', 'wait', 'close'])
        self.assertEqual(events[0][1][3:5], (0x123000, 0x400000))
        self.assertEqual(events[1], ('wait', 37))

    def test_creation_vs_wait_vs_exit_code_failures_have_distinct_pending_semantics(self):
        for stage, pending, closes in (('create', False, False), ('wait', True, True), ('exit', True, True)):
            with self.subTest(stage=stage):
                obj, events = self.transport(create=0 if stage == 'create' else 123,
                                             wait=258 if stage == 'wait' else 0, exit_ok=stage != 'exit')
                with self.assertRaises(SubmitFailure) as caught:
                    obj.submit(0x400000, 37)
                self.assertIs(caught.exception.pending_possible, pending)
                self.assertEqual(any(e[0] == 'close' for e in events), closes)

    def test_allocation_failure_and_export_byte_mismatch_never_start_thread(self):
        obj, events = self.transport(allocated=0)
        with self.assertRaises(OSError):
            obj.allocate(4120)
        self.assertFalse(events)
        obj.submit_source['bytes'] = (b'Y' * 32).hex()
        with self.assertRaises(SubmitFailure) as caught:
            obj.submit(0x400000, 37)
        self.assertFalse(caught.exception.pending_possible)
        self.assertFalse(events)


if __name__ == '__main__':
    unittest.main()
