"""Real CoverQuery + original code against synthetic memory; no game calls."""
import copy
import hashlib
import json
import math
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import cover_spatial_root as spatial
from cover_state import CoverQuery
from test_cover_bone_query import pe_headers
from test_cover_surface_query import SurfaceMemory


class SpatialMemory(SurfaceMemory):
    LEVEL_CLASS, WORLD_CLASS = 0x300000, 0x301000
    LEVEL, UWORLD = 0x310000, 0x311000
    HASH, NODE, CHILDREN = 0x400000, 0x401000, 0x402008
    DATA, GRANDCHILDREN = 0x410000, 0x420008

    def __init__(self, wrapper_code, tree_code):
        super().__init__()
        self.add_class(self.LEVEL_CLASS, 'Level', 0)
        self.add_class(self.WORLD_CLASS, 'World', 0)
        self.add_object(self.LEVEL, 'PersistentLevel', self.LEVEL_CLASS, outer=self.UWORLD)
        self.add_object(self.UWORLD, 'CurrentWorld', self.WORLD_CLASS)
        self.objects[self.WORLD]['outer'] = self.LEVEL
        self.module['sha256'] = self.profile['sha256'] = spatial.surface.GAME_SHA256
        sections = [copy.deepcopy(self.module['sections'][0]),
            dict(name='.rdata', rva=0x131b000, size=0x7d5000, virtual_size=0x7d5000,
                 raw_size=0x7d5000, raw_pointer=0x131a400, characteristics=0x40000040),
            dict(name='.data', rva=0x1af0000, size=0x140000, virtual_size=0x140000,
                 raw_size=0x140000, raw_pointer=0x1aef400, characteristics=0xc0000040)]
        self.header, disk_size = pe_headers(sections, image_size=0x2040000)
        self.put(self.BASE, self.header)
        self.module.update(size=0x2040000, headers=self.header, disk_size=disk_size, sections=sections)
        for rva, body, _ in wrapper_code + tree_code:
            self.put(self.BASE + rva, body)
        self.put(self.BASE + spatial.GLOBAL_WORLD_RVA, struct.pack('<Q', self.UWORLD))
        self.put(self.UWORLD + spatial.WORLD_HASH_OFFSET, struct.pack('<Q', self.HASH))
        self.put(self.HASH, struct.pack('<QQ', self.BASE + spatial.VTABLE_RVA, self.NODE))
        self.put(self.BASE + spatial.VTABLE_RVA, struct.pack('<8Q',
                 *(self.BASE + rva for rva in spatial.VTABLE_TARGET_RVAS)))
        self.put(self.NODE, struct.pack('<QiiQ', 0, 0, 0, 0))
        self.put(self.BASE + spatial.ROOT_CELL_RVA, struct.pack('<4f', 0, 0, 0, 1048576.))
        self.put(self.BASE + spatial.HALF_RVA, struct.pack('<f', .5))
        self.put(self.BASE + spatial.MINIMUM_RVA, struct.pack('<f', 100.))

    def children(self):
        self.put(self.NODE, struct.pack('<QiiQ', self.DATA, 1, 2, self.CHILDREN))
        self.put(self.CHILDREN - 8, struct.pack('<Q', 8))
        self.put(self.CHILDREN, bytes(8 * 24))


class SpatialRootTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not Path(spatial.surface.DEFAULT_BLOB).is_file():
            raise unittest.SkipTest('Retained original code required for root regression')
        cls.wrapper_code = spatial.world_code._historical()
        cls.tree_code = spatial._historical()

    def fixture(self, *, children=False):
        f = SpatialMemory(self.wrapper_code, self.tree_code)
        if children:
            f.children()
        return f, CoverQuery(f.observer)

    def observe(self, f, q, info=None):
        with patch.object(spatial.world_code, '_historical', return_value=self.wrapper_code), \
                patch.object(spatial, '_historical', return_value=self.tree_code):
            return spatial.observe(q, f.module, copy.deepcopy(info if info is not None else f.objects[f.WORLD]))

    def test_original_code_contains_complete_constructor_loop(self):
        self.assertEqual(sum(len(body) for _, body, _ in self.tree_code), 4077)
        rva, body, _ = self.tree_code[-1]
        self.assertEqual((rva, len(body)), (0x53eb20, 112))
        self.assertEqual(body[0x50:0x57].hex(), '48895c2428ebda')

    def test_leaf_links_use_uworld_not_world_info_and_close_same_query(self):
        f, q = self.fixture(); before = copy.deepcopy(f.memory)
        out = self.observe(f, q)
        self.assertEqual(out['global_world']['address'], f.UWORLD)
        self.assertEqual(out['outer_level']['address'], f.LEVEL)
        self.assertEqual(out['hash_receiver'], f.HASH)
        self.assertEqual(out['root']['address'], f.NODE)
        self.assertEqual(out['child_headers'], [])
        self.assertEqual(out['root_cell'], dict(center=[0., 0., 0.], half=1048576.))
        self.assertNotIn((f.WORLD + spatial.WORLD_HASH_OFFSET, 8), q.values)
        self.assertIn((f.UWORLD + spatial.WORLD_HASH_OFFSET, 8), q.values)
        self.assertTrue(out['root_links_observed'] and out['read_only'] and out['closing_required'])
        for key in ('component_payload_observed', 'complete_tree_observed', 'complete_scene_verified',
                    'bsp_or_special_coverage_verified', 'engine_calls', 'virtual_calls',
                    'atomic_snapshot_verified', 'world_synchronization_verified',
                    'path_or_occupancy_verified', 'activation_allowed'):
            self.assertFalse(out[key], key)
        q.finish()
        self.assertEqual(f.memory, before)
        self.assertEqual(f.bridges, [])
        json.dumps(out, allow_nan=False)

    def test_child_headers_are_exactly_eight_and_no_component_payload_read(self):
        f, q = self.fixture(children=True)
        # A child may have further nodes. This observer deliberately does not
        # traverse or certify them and must never infer a valid empty result.
        f.put(f.CHILDREN + 7 * 24, struct.pack('<QiiQ', 0, 0, 0, f.GRANDCHILDREN))
        out = self.observe(f, q)
        self.assertEqual([c['address'] for c in out['child_headers']], [f.CHILDREN + 24 * i for i in range(8)])
        self.assertEqual([c['index'] for c in out['child_headers']], list(range(8)))
        self.assertEqual(out['root']['count'], 1)
        self.assertIn((f.CHILDREN, 192), q.values)
        self.assertFalse(any(address == f.DATA or address == f.GRANDCHILDREN for address, _ in q.values))
        self.assertTrue(out['first_child_headers_only'])
        self.assertFalse(out['complete_tree_observed'])
        q.finish()

    def test_role_identity_class_outer_cycles_and_defaults_reject(self):
        for defect in ('stale', 'worldinfo_class', 'level_class', 'world_class',
                       'outer_null', 'outer_self', 'class_cycle', 'class_number', 'metaclass',
                       'default_info', 'default_level', 'default_world'):
            with self.subTest(defect=defect):
                f, q = self.fixture(); info = copy.deepcopy(f.objects[f.WORLD])
                if defect == 'stale':info['internal_index'] += 1
                elif defect == 'worldinfo_class':f.objects[f.WORLD]['cls'] = f.WORLD_CLASS
                elif defect == 'level_class':f.objects[f.LEVEL]['cls'] = f.WORLD_CLASS
                elif defect == 'world_class':f.objects[f.UWORLD]['cls'] = f.LEVEL_CLASS
                elif defect == 'outer_null':f.objects[f.WORLD]['outer'] = 0
                elif defect == 'outer_self':f.objects[f.LEVEL]['outer'] = f.LEVEL
                elif defect == 'class_cycle':f.put(f.WORLD_CLASS + f.profile['field_super'], struct.pack('<Q', f.WORLD_CLASS))
                elif defect == 'class_number':f.objects[f.WORLD_CLASS]['number'] = 1
                elif defect == 'metaclass':f.objects[f.WORLD_CLASS]['cls'] = f.LEVEL_CLASS
                else:f.objects[{'default_info': f.WORLD, 'default_level': f.LEVEL, 'default_world': f.UWORLD}[defect]]['name'] = 'Default__Object'
                if defect != 'stale':info = copy.deepcopy(f.objects[f.WORLD])
                with self.assertRaises(RuntimeError):self.observe(f, q, info)

    def test_wrong_global_world_rejects_before_reading_hash(self):
        f, q = self.fixture()
        f.put(f.BASE + spatial.GLOBAL_WORLD_RVA, struct.pack('<Q', f.WORLD))
        with self.assertRaisesRegex(RuntimeError, 'global World'):self.observe(f, q)
        self.assertNotIn((f.UWORLD + spatial.WORLD_HASH_OFFSET, 8), q.values)

    def test_pointer_bounds_alignment_and_nulls_reject(self):
        for field in ('hash', 'root', 'children'):
            for value in (0, 0x10001, 1 << 47, (1 << 64) - 8):
                if field == 'children' and value == 0:
                    continue
                with self.subTest(field=field, value=value):
                    f, q = self.fixture()
                    if field == 'hash':f.put(f.UWORLD + spatial.WORLD_HASH_OFFSET, struct.pack('<Q', value))
                    elif field == 'root':f.put(f.HASH + 8, struct.pack('<Q', value))
                    else:f.put(f.NODE + 16, struct.pack('<Q', value))
                    with self.assertRaises(RuntimeError):self.observe(f, q)

    def test_receiver_table_address_and_each_target_reject(self):
        f, q = self.fixture(); f.put(f.HASH, struct.pack('<Q', f.BASE + spatial.VTABLE_RVA + 8))
        with self.assertRaisesRegex(RuntimeError, 'vtable address'):self.observe(f, q)
        for index in range(8):
            with self.subTest(index=index):
                f, q = self.fixture(); f.put(f.BASE + spatial.VTABLE_RVA + index * 8, struct.pack('<Q', f.BASE + 0x1000))
                with self.assertRaisesRegex(RuntimeError, 'vtable targets'):self.observe(f, q)

    def test_node_capacity_data_and_child_header_bounds_reject(self):
        for row in ((0, -1, 0, 0), (0, 2, 1, 0), (0, 0, -1, 0),
                    (0, 0, spatial.MAX_NODE_CAPACITY + 1, 0), (0, 0, 1, 0),
                    (0x410000, 0, 0, 0), (0x410001, 0, 1, 0)):
            for child in (False, True):
                with self.subTest(row=row, child=child):
                    f, q = self.fixture(children=child)
                    f.put(f.CHILDREN + 3 * 24 if child else f.NODE, struct.pack('<QiiQ', *row))
                    with self.assertRaisesRegex(RuntimeError, 'node|pointer'):self.observe(f, q)

    def test_cookie_and_known_header_cycles_reject(self):
        for defect in ('cookie0', 'cookie7', 'cookie9', 'root_overlap', 'child_self', 'child_sibling', 'child_to_root'):
            with self.subTest(defect=defect):
                f, q = self.fixture(children=True)
                if defect.startswith('cookie'):f.put(f.CHILDREN - 8, struct.pack('<Q', int(defect[6:])))
                elif defect == 'root_overlap':f.put(f.NODE + 16, struct.pack('<Q', f.NODE + 8))
                else:f.put(f.CHILDREN + 16, struct.pack('<Q', {'child_self':f.CHILDREN, 'child_sibling':f.CHILDREN + 24, 'child_to_root':f.NODE}[defect]))
                with self.assertRaisesRegex(RuntimeError, 'cookie|overlap|cycle'):self.observe(f, q)

    def test_every_tree_code_range_and_old_metadata_gap_mismatch_reject(self):
        for rva, body, _ in self.tree_code:
            with self.subTest(rva=hex(rva)):
                f, q = self.fixture(); changed = bytearray(body); changed[-1] ^= 1
                f.put(f.BASE + rva, changed)
                with self.assertRaises(spatial.world_code.CodeRejected) as caught:self.observe(f, q)
                self.assertEqual(caught.exception.source_witness['rva'], hex(rva))
                self.assertFalse(caught.exception.source_witness['activation_allowed'])
        f, q = self.fixture(); f.put(f.BASE + 0x53eb75, b'\x90')
        with self.assertRaises(spatial.world_code.CodeRejected):self.observe(f, q)

    def test_each_observed_link_and_header_change_rejects_same_query_close(self):
        for field in ('global', 'world_hash', 'hash_root', 'root_header', 'cookie', 'eighth_child', 'table', 'cell', 'constant', 'code', 'outer_identity'):
            with self.subTest(field=field):
                f, q = self.fixture(children=True); self.observe(f, q)
                if field == 'outer_identity':f.objects[f.LEVEL]['outer'] = 0
                else:
                    address = {'global':f.BASE + spatial.GLOBAL_WORLD_RVA, 'world_hash':f.UWORLD + spatial.WORLD_HASH_OFFSET,
                        'hash_root':f.HASH + 8, 'root_header':f.NODE + 8, 'cookie':f.CHILDREN - 8,
                        'eighth_child':f.CHILDREN + 7 * 24 + 8, 'table':f.BASE + spatial.VTABLE_RVA,
                        'cell':f.BASE + spatial.ROOT_CELL_RVA, 'constant':f.BASE + spatial.HALF_RVA,
                        'code':f.BASE + 0x53eb75}[field]
                    f.put(address, bytes([f.memory[address] ^ 1]))
                with self.assertRaisesRegex(RuntimeError, 'changed'):q.finish()

    def test_nonfinite_cell_nonpositive_half_and_constants_reject(self):
        for index,value in ((0,math.nan),(1,math.inf),(2,-math.inf),(3,0.),(3,-1.),(3,math.nan)):
            f, q = self.fixture(); cube = [0.,0.,0.,1048576.];cube[index] = value
            f.put(f.BASE + spatial.ROOT_CELL_RVA, struct.pack('<4f', *cube))
            with self.assertRaisesRegex(RuntimeError, 'cell'):self.observe(f, q)
        for rva in (spatial.HALF_RVA, spatial.MINIMUM_RVA):
            f, q = self.fixture(); f.put(f.BASE + rva, struct.pack('<f', 0.))
            with self.assertRaisesRegex(RuntimeError, 'constants'):self.observe(f, q)

    def test_module_identity_section_permissions_and_extent_reject(self):
        for defect in ('base', 'disk', 'profile', 'short', 'data_executable', 'vtable_unreadable', 'code_not_executable'):
            with self.subTest(defect=defect):
                f, q = self.fixture()
                if defect == 'base':f.module['base'] += 0x1000
                elif defect == 'disk':f.module['sha256'] = '0' * 64
                elif defect == 'profile':f.profile['sha256'] = '0' * 64
                elif defect == 'short':f.module['size'] = spatial.GLOBAL_WORLD_RVA
                elif defect == 'data_executable':f.module['sections'][-1]['characteristics'] |= 0x20000000
                elif defect == 'vtable_unreadable':f.module['sections'][1]['characteristics'] &= ~0x40000000
                else:f.module['sections'][0]['characteristics'] &= ~0x20000000
                with self.assertRaises(RuntimeError):self.observe(f, q)

    def test_bad_original_file_rejects_without_substitution(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'wrong.bin';path.write_bytes(b'not original')
            with self.assertRaisesRegex(RuntimeError, 'historical'):spatial._historical(path)


if __name__ == '__main__':
    unittest.main()
