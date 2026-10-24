"""Synthetic reflected memory and private files; no process/bridge/game calls."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import cover_surface_query as surface
from cover_state import CoverQuery
from test_cover_bone_query import BoneMemory, pe_headers


class SurfaceMemory(BoneMemory):
    ACTOR, HIT = 0x28000, 0x29000
    LOCATION_FN, ROTATION_FN, TRACE_FN = 0x170000, 0x171000, 0x172000

    def __init__(self):
        super().__init__()
        # The older shared fixture uses B0 as a synthetic struct link. This
        # sampler records the supported retail D0 and therefore models D0 here.
        for p in self.properties.values():
            address = p['address']
            self.put(address + 0xd0, bytes(self.memory.get(address + 0xb0 + i, 0) for i in range(8)))
        self.put(self.INNER + 0xd0, struct.pack('<Q', self.SLOT))
        self.profile['struct_property_struct'] = 0xd0
        self.add_class(self.ACTOR, 'Actor', 0)
        self.add_class(self.PC_CLASS, 'WarPC', self.ACTOR)
        self.add_class(self.PAWN_CLASS, 'WarPawn', self.ACTOR)
        self.add_class(self.LINK_CLASS, 'CoverLink', self.ACTOR)
        for typ in (self.VECTOR, self.ROTATOR, self.SLOT):
            self.objects[typ]['cls'] = self.STRUCT_CLASS
            self.put(typ + 0x60, bytes(8))
            self.put(typ + 0x88, struct.pack('<I', 212 if typ == self.SLOT else 12))
        self.put(self.INNER + 0x70, struct.pack('<II', 1, 212))
        self.add_object(self.HIT, 'TraceHitInfo', self.STRUCT_CLASS, outer=self.ACTOR)
        self.put(self.HIT + 0x60, bytes(8))
        self.put(self.HIT + 0x88, struct.pack('<I', 40))
        for name, kind, size, offset in surface.HIT_SPECS:
            self.add_property(self.HIT, name, kind, offset, size,
                              target=0x20400 if kind in ('ObjectProperty', 'ComponentProperty') else None)
        self.functions = {self.LINK_CLASS: [self.LOCATION_FN, self.ROTATION_FN], self.ACTOR: [self.TRACE_FN]}
        self.make_function(self.LOCATION_FN, self.LINK_CLASS, 'GetSlotLocation', 0x2000, 48, 24,
                           [('SlotIdx', 'IntProperty', 4, 8, None), ('ReturnValue', 'StructProperty', 12, 24, self.VECTOR)])
        self.make_function(self.ROTATION_FN, self.LINK_CLASS, 'GetSlotRotation', 0x2100, 64, 32,
                           [('SlotIdx', 'IntProperty', 4, 16, None), ('ReturnValue', 'StructProperty', 12, 32, self.ROTATOR)])
        # Valid new offsets differ from the historical 0..116 parameter span.
        trace_fields, offset = [], 16
        for name, kind, width, shape in surface.TRACE_SPECS:
            target = self.HIT if shape == 'TraceHitInfo' else (self.VECTOR if shape == 'Vector' else (self.ACTOR if kind == 'ObjectProperty' else None))
            trace_fields.append((name, kind, width, offset, target))
            offset += width
        self.make_function(self.TRACE_FN, self.ACTOR, 'Trace', surface.TRACE_RVA, 160, 124, trace_fields, native=277)
        # Selected component/property bytes include opaque D0 words too. The
        # sampler records them but does not reinterpret scalar D0 as pointers.
        for p in self.properties.values():
            address = p['address']
            self.put(address + 0xd0, bytes(self.memory.get(address + 0xd0 + i, 0) for i in range(8)))
        for owner in {owner for owner, _ in self.properties} | set(self.functions):
            addresses = self.functions.get(owner, []) + [p['address'] for (declaring, _), p in self.properties.items() if declaring == owner]
            self.put(owner + 0x80, struct.pack('<Q', addresses[0] if addresses else 0))
            for current, nxt in zip(addresses, addresses[1:] + [0]):
                self.put(current + 0x68, struct.pack('<Q', nxt))
        sections = [dict(name='.text', rva=0x1000, size=0x131a000, virtual_size=0x131a000,
                         raw_size=0x131a000, raw_pointer=0x400, characteristics=0x60000020),
                    dict(name='.data', rva=0x1b86000, size=0x2000, virtual_size=0x2000,
                         raw_size=0x2000, raw_pointer=0x131a400, characteristics=0xc0000040)]
        self.header, disk_size = pe_headers(sections, image_size=0x1c00000)
        self.put(self.BASE, self.header)
        self.module.update(size=0x1c00000, headers=self.header, disk_size=disk_size, sections=sections)
        self.expected = []
        for i, (rva, size, _) in enumerate(surface.TRACE_CHUNKS):
            data = bytes([0x51 + i]) * size
            self.put(self.BASE + rva, data)
            self.expected.append((rva, data, hashlib.sha256(data).hexdigest()))
        self.table = self.BASE + surface.GNATIVES_RVA + surface.TRACE_INDEX * 8
        self.put(self.table, struct.pack('<Q', self.BASE + surface.TRACE_RVA))

    def make_function(self, address, owner, name, rva, size, return_offset, fields, native=0):
        self.put(address, bytes(surface.HEADER_SIZE))
        self.add_object(address, name, self.FUNCTION_CLASS, outer=owner)
        self.put(address + 0xf8, struct.pack('<H', native))
        self.put(address + 0xf4, struct.pack('<I', 0x400))
        self.put(address + 0xfc, struct.pack('<I', 17))
        self.put(address + 0x106, struct.pack('<HH', size, return_offset))
        self.put(address + 0x114, struct.pack('<Q', self.BASE + rva))
        for label, kind, width, offset, target in fields:
            self.add_property(address, label, kind, offset, width, target=target)
            p = self.properties[address, label]['address']
            self.put(p + 0x78, struct.pack('<Q', 0x580 if label == 'ReturnValue' else 0x80))
            if kind == 'BoolProperty':self.put(p + 0xd0, struct.pack('<I', 0x20))

    def capture_surface(self, **kwargs):
        with patch.object(surface, 'GAME_SHA256', 'a' * 64), patch.object(surface, '_disk_module', return_value=copy.deepcopy(self.module)), \
                patch.object(surface, '_historical', return_value=(dict(path='synthetic/historical.bin', sha256='c' * 64), self.expected)):
            return surface.capture(self.observer, **kwargs)


class SurfaceTests(unittest.TestCase):
    def result(self, fixture, accepted=True, **kwargs):
        before = copy.deepcopy(fixture.memory)
        row = fixture.capture_surface(**kwargs)
        self.assertIs(row['accepted'], accepted, row.get('rejection'))
        self.assertTrue(row['read_only'] and row['metadata_only'])
        for key in ('engine_calls', 'activation_allowed', 'actor_or_mesh_fields_written', 'collision_verified',
                    'native_getter_verified', 'native_impl_verified', 'atomic_snapshot_verified'):
            self.assertIs(row[key], False)
        self.assertEqual(row['queue_submissions'], 0)
        self.assertEqual(fixture.bridges, [])
        if fixture.before_read is None and fixture.at_verify is None:self.assertEqual(fixture.memory, before)
        if not accepted:self.assertIn('rejection', row)
        json.dumps(row, allow_nan=False)
        return row

    def test_current_headers_three_exact_schemas_and_trace_full_chunks(self):
        f, queries = SurfaceMemory(), []
        def factory(observer):
            q = CoverQuery(observer);queries.append(q);return q
        out = self.result(f, query_factory=factory)
        self.assertEqual(set(out['functions']), {'GetSlotLocation', 'GetSlotRotation', 'Trace'})
        self.assertEqual(set(out['structures']), {'Vector', 'Rotator', 'TraceHitInfo'})
        self.assertEqual(len(queries), 1)
        self.assertLess(queries[0].reads, 4096)
        for name, size, offset in (('GetSlotLocation', 48, 24), ('GetSlotRotation', 64, 32), ('Trace', 160, 124)):
            row = out['functions'][name]
            self.assertEqual((row['parms_size'], row['return_offset']), (size, offset))
            self.assertEqual(len(bytes.fromhex(row['header_bytes'])), 0x11c)
            self.assertTrue(row['metadata_accepted'])
            self.assertFalse(row['native_impl_verified'])
            self.assertEqual(row['wrapper_code_verified'], name == 'Trace')
        trace = out['functions']['Trace']
        self.assertEqual(trace['native_index'], 277)
        self.assertEqual(trace['entry_rva'], '0xcdacc')
        self.assertEqual([x['size'] for x in trace['chunks']], [1873, 186])
        self.assertTrue(all(x['equal'] and x['expected_sha256'] == x['current_sha256'] for x in trace['chunks']))
        self.assertEqual(next(p['bool_mask'] for p in trace['fields'] if p['name'] == 'bTraceActors'), 0x20)
        self.assertEqual(out['roles']['current_slot'], 1)
        self.assertTrue(out['trace_table']['equals_reflected_entry'])
        self.assertEqual([p['offset'] for p in out['structures']['Vector']['components']], [0, 4, 8])

    def test_missing_link_wrong_world_possession_and_slot_bounds(self):
        for obj, name, value in ((SurfaceMemory.PAWN, 'CurrentLink', 0), (SurfaceMemory.LINK, 'WorldInfo', SurfaceMemory.OTHER),
                                 (SurfaceMemory.PC, 'WorldInfo', SurfaceMemory.OTHER), (SurfaceMemory.PAWN, 'Controller', SurfaceMemory.OTHER),
                                 (SurfaceMemory.PAWN, 'CurrentSlotIdx', -1), (SurfaceMemory.PAWN, 'CurrentSlotIdx', 3)):
            with self.subTest(name=name, value=value):
                f = SurfaceMemory();f.set_value(obj, name, value, '<i' if name == 'CurrentSlotIdx' else '<Q');self.result(f, False)
        f = SurfaceMemory();f.put(f.INNER + 0x74, struct.pack('<I', 48));self.result(f, False)

    def test_source_game_lifecycle_readers_and_pe_are_required(self):
        for defect in ('readers', 'session', 'owner', 'profile', 'pe_machine', 'pe_section', 'module_pid'):
            with self.subTest(defect=defect):
                f = SurfaceMemory()
                if defect == 'readers':f.observer.sampler = None
                if defect == 'session':f.observer.session['session_id'] = ''
                if defect == 'owner':f.observer.session['owner_identity']['creation_time'] += 1
                if defect == 'profile':f.profile['sha256'] = 'd' * 64
                if defect == 'pe_machine':f.put(f.BASE + 0x84, struct.pack('<H', 0x14c))
                if defect == 'pe_section':f.put(f.BASE + 0x188 + 8, struct.pack('<I', 0x1319000))
                if defect == 'module_pid':f.pid += 1
                self.result(f, False)

    def test_function_header_bounds_trace_index_entry_and_table(self):
        for defect in ('size_zero', 'size_large', 'return_out', 'return_field', 'trace_index', 'entry', 'entry_out', 'table'):
            with self.subTest(defect=defect):
                f = SurfaceMemory()
                if defect == 'size_zero':f.put(f.LOCATION_FN + 0x106, bytes(2))
                if defect == 'size_large':f.put(f.LOCATION_FN + 0x106, struct.pack('<H', 4097))
                if defect == 'return_out':f.put(f.LOCATION_FN + 0x108, struct.pack('<H', 48))
                if defect == 'return_field':f.put(f.LOCATION_FN + 0x108, struct.pack('<H', 12))
                if defect == 'trace_index':f.put(f.TRACE_FN + 0xf8, struct.pack('<H', 276))
                if defect == 'entry':f.put(f.TRACE_FN + 0x114, struct.pack('<Q', f.BASE + 0xcdb00))
                if defect == 'entry_out':f.put(f.LOCATION_FN + 0x114, struct.pack('<Q', f.BASE + f.module['size']))
                if defect == 'table':f.put(f.table, struct.pack('<Q', f.BASE + 0x1d6ca0))
                self.result(f, False)

    def test_current_slot_getter_index_is_observed_without_inventing_certificate(self):
        f = SurfaceMemory();f.put(f.LOCATION_FN + 0xf8, struct.pack('<H', 511))
        out = self.result(f)
        self.assertEqual(out['functions']['GetSlotLocation']['native_index'], 511)
        self.assertFalse(out['functions']['GetSlotLocation']['wrapper_code_verified'])

    def test_parameter_dimensions_width_offsets_flags_d0_and_exact_membership(self):
        for defect in ('dimension', 'width', 'outside', 'overlap', 'parm_flag', 'return_flag', 'kind', 'owner', 'number', 'missing', 'cycle', 'bool_zero', 'bool_multi', 'd0'):
            with self.subTest(defect=defect):
                f = SurfaceMemory();p = f.properties[f.LOCATION_FN, 'SlotIdx']['address']
                if defect == 'dimension':f.put(p + 0x70, struct.pack('<I', 2))
                if defect == 'width':f.put(p + 0x74, struct.pack('<I', 8))
                if defect == 'outside':f.put(p + 0x94, struct.pack('<I', 47))
                if defect == 'overlap':f.put(p + 0x94, struct.pack('<I', 24))
                if defect == 'parm_flag':f.put(p + 0x78, bytes(8))
                if defect == 'return_flag':f.put(f.properties[f.LOCATION_FN, 'ReturnValue']['address'] + 0x78, struct.pack('<Q', 0x80))
                if defect == 'kind':f.objects[p]['cls'] = f.kind_classes['FloatProperty']
                if defect == 'owner':f.objects[p]['outer'] = f.LINK_CLASS
                if defect == 'number':f.objects[p]['number'] = 1
                if defect == 'missing':f.put(f.LOCATION_FN + 0x80, bytes(8))
                if defect == 'cycle':f.put(p + 0x68, struct.pack('<Q', p))
                if defect in ('bool_zero', 'bool_multi'):f.put(f.properties[f.TRACE_FN, 'bTraceActors']['address'] + 0xd0, struct.pack('<I', 0 if defect == 'bool_zero' else 3))
                if defect == 'd0':f.put(f.properties[f.LOCATION_FN, 'ReturnValue']['address'] + 0xd0, struct.pack('<Q', f.ROTATOR))
                self.result(f, False)

    def test_vector_rotator_hit_components_are_typed_and_bounded(self):
        for owner, name in ((SurfaceMemory.VECTOR, 'X'), (SurfaceMemory.ROTATOR, 'Yaw'), (SurfaceMemory.HIT, 'LevelIndex')):
            for defect in ('width', 'offset', 'type'):
                with self.subTest(name=name, defect=defect):
                    f = SurfaceMemory();p = f.properties[owner, name]['address']
                    if defect == 'width':f.put(p + 0x74, struct.pack('<I', 8))
                    if defect == 'offset':f.put(p + 0x94, struct.pack('<I', 1))
                    if defect == 'type':f.objects[p]['cls'] = f.kind_classes['ByteProperty']
                    self.result(f, False)
        f = SurfaceMemory();f.put(f.HIT + 0x88, struct.pack('<I', 44));self.result(f, False)

    def test_both_trace_chunks_including_last_byte_are_exact_and_partial_leads_survive(self):
        for index in (0, 1):
            f = SurfaceMemory();rva, raw, _ = f.expected[index];f.put(f.BASE + rva + len(raw) - 1, b'\xff')
            out = self.result(f, False)
            self.assertEqual(out['functions']['GetSlotRotation']['entry_rva'], '0x2100')
            self.assertFalse(out['functions']['Trace']['chunks'][-1]['equal'])
            self.assertFalse(out['functions']['Trace']['wrapper_code_verified'])

    def test_closing_bracket_rejects_header_value_component_table_code_and_registry_changes(self):
        for target, size in ((SurfaceMemory.LOCATION_FN, 0x11c), (SurfaceMemory.BASE + surface.TRACE_CHUNKS[1][0], 186),
                              (SurfaceMemory.BASE + surface.GNATIVES_RVA + 277 * 8, 8)):
            f = SurfaceMemory()
            def change(address, length, count):
                if (address, length) == (target, size) and count == 2:f.put(target, bytes([0xff]) * size)
            f.before_read = change;self.result(f, False)
        for defect in ('link', 'metadata', 'registry', 'session'):
            f = SurfaceMemory()
            def change(count):
                if count != 3:return
                if defect == 'link':f.set_value(f.PAWN, 'CurrentLink', 0, '<Q')
                if defect == 'metadata':f.put(f.properties[f.HIT, 'BoneName']['address'] + 0x94, struct.pack('<I', 23))
                if defect == 'registry':f.put(f.names_header, struct.pack('<Qii', 0x110000, 4, 16))
                if defect == 'session':f.observer.session['session_id'] = 'replaced'
            f.at_verify = change;self.result(f, False)

    def test_unreadable_and_existing_read_budget_fail_closed(self):
        f = SurfaceMemory();del f.memory[f.BASE + surface.TRACE_CHUNKS[0][0]];self.result(f, False)
        def limited(observer):
            q = CoverQuery(observer);q.reads = 16384;return q
        self.result(SurfaceMemory(), False, query_factory=limited)

    def test_no_transport_or_write_capability_is_imported_by_capture(self):
        names = surface.capture.__code__.co_names
        for name in ('Bridge', 'submit', 'allocate', 'write', 'CreateRemoteThread', 'WriteProcessMemory'):
            self.assertNotIn(name, names)


class HistoricalSourceTests(unittest.TestCase):
    def test_disk_module_hash_header_and_process_path_binding(self):
        sections = [dict(name='.text', rva=0x1000, size=0x2000, virtual_size=0x2000,
                         raw_size=0x2000, raw_pointer=0x400, characteristics=0x60000020)]
        header, size = pe_headers(sections)
        raw = header + bytes(size - len(header))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'GOWDE-Steam.exe';path.write_bytes(raw)
            digest = hashlib.sha256(raw).hexdigest()
            observer = SimpleNamespace(resolver=SimpleNamespace(profile=dict(sha256=digest), path=str(path), pid=7, base=0x140000000),
                                       identities=dict(game_identity=dict(image=str(path), pid=7)))
            with patch.object(surface, 'GAME_SHA256', digest):
                module = surface._disk_module(observer)
                self.assertEqual((module['headers'], module['disk_size'], module['sha256']), (header, size, digest))
                observer.resolver.pid = 8
                with self.assertRaisesRegex(RuntimeError, 'identity'):surface._disk_module(observer)
                observer.resolver.pid = 7
                path.write_bytes(raw[:-1] + b'\xff')
                with self.assertRaisesRegex(RuntimeError, 'SHA256'):surface._disk_module(observer)

    def test_blob_whole_source_and_each_selected_chunk_are_pinned(self):
        raw = bytes(range(256)) * 32
        specs = ((0x1000, 80, hashlib.sha256(raw[:80]).hexdigest()),
                 (0x1100, 90, hashlib.sha256(raw[256:346]).hexdigest()))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'saved.bin';path.write_bytes(raw)
            with patch.object(surface, 'HISTORICAL_SIZE', len(raw)), patch.object(surface, 'HISTORICAL_SHA256', hashlib.sha256(raw).hexdigest()), patch.object(surface, 'TRACE_CHUNKS', specs):
                source, chunks = surface._historical(path)
                self.assertEqual(source['sha256'], hashlib.sha256(raw).hexdigest())
                self.assertEqual([len(c[1]) for c in chunks], [80, 90])
                path.write_bytes(raw[:-1])
                with self.assertRaisesRegex(RuntimeError, 'size'):surface._historical(path)
                path.write_bytes(b'\xff' + raw[1:])
                with self.assertRaisesRegex(RuntimeError, 'SHA256'):surface._historical(path)
                path.write_bytes(raw)
                with patch.object(surface, 'TRACE_CHUNKS', ((0x1000, 80, '0' * 64),)):
                    with self.assertRaisesRegex(RuntimeError, 'full chunk'):surface._historical(path)
                with patch.object(surface, 'TRACE_CHUNKS', ((0, 80, specs[0][2]),)):
                    with self.assertRaisesRegex(RuntimeError, 'full chunk'):surface._historical(path)


if __name__ == '__main__':unittest.main()
