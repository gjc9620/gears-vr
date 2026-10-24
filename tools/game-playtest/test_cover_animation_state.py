"""Synthetic reflected memory; no process, animation evaluation or engine calls."""
import struct
import unittest

from cover_animation_state import capture
from test_cover_state import MemoryFixture


class AnimationMemory(MemoryFixture):
    MESH, ROOT, COVER, BLEND, LEAF, LEAF2, ASSET, TEMPLATE = range(0x80000, 0x88000, 0x1000)
    MESH_CLASS, NODE_CLASS, BASE_CLASS, TREE_CLASS, COVER_CLASS, BLEND_CLASS, LEAF_CLASS, ASSET_CLASS, GENERIC_CLASS = range(0x90000, 0x90900, 0x100)
    CHILD, MASK, ATOM = 0xa0000, 0xa1000, 0xa2000

    def __init__(self):
        self.supers = {}
        super().__init__()
        self.profile['field_super'] = 0x60
        self.next_inner, self.next_data = 0xb0000, 0xc0000
        self.arrays = {}
        for address, name, parent in (
                (self.PAWN_CLASS, 'WarPawn', 0), (self.MESH_CLASS, 'SkeletalMeshComponent', 0),
                (self.NODE_CLASS, 'AnimNode', 0), (self.BASE_CLASS, 'AnimNodeBlendBase', self.NODE_CLASS),
                (self.TREE_CLASS, 'AnimTree', self.BASE_CLASS),
                (self.COVER_CLASS, 'WarAnim_CoverBlend', self.BASE_CLASS),
                (self.BLEND_CLASS, 'WarAnim_BlendPerBone', self.BASE_CLASS),
                (self.LEAF_CLASS, 'AnimNodeSequence', self.NODE_CLASS),
                (self.ASSET_CLASS, 'SkeletalMesh', 0),
                (self.GENERIC_CLASS, 'GenericBlend', self.BASE_CLASS)):
            self.add_class(address, name, parent)
        for address, name, cls in ((self.MESH, 'PawnMesh', self.MESH_CLASS),
                                    (self.ROOT, 'LiveTree', self.TREE_CLASS),
                                    (self.COVER, 'CoverInstance', self.COVER_CLASS),
                                    (self.BLEND, 'PerBoneInstance', self.BLEND_CLASS),
                                    (self.LEAF, 'CoverSequence', self.LEAF_CLASS),
                                    (self.LEAF2, 'SharedAimSequence', self.LEAF_CLASS),
                                    (self.ASSET, 'CharacterAsset', self.ASSET_CLASS),
                                    (self.TEMPLATE, 'TemplateTree', self.TREE_CLASS),
                                    (self.CHILD, 'AnimBlendChild', 0x20500),
                                    (self.MASK, 'PerBoneMaskInfo', 0x20500),
                                    (self.ATOM, 'BoneAtom', 0x20500)):
            self.add_object(address, name, cls)
        for owner, name, kind, offset, size in (
                (self.PAWN_CLASS, 'Mesh', 'ComponentProperty', 56, 8),
                (self.MESH_CLASS, 'Owner', 'ObjectProperty', 0, 8),
                (self.MESH_CLASS, 'Animations', 'ObjectProperty', 8, 8),
                (self.MESH_CLASS, 'AnimTreeTemplate', 'ObjectProperty', 16, 8),
                (self.MESH_CLASS, 'SkeletalMesh', 'ObjectProperty', 40, 8),
                (self.NODE_CLASS, 'SkelComponent', 'ComponentProperty', 0, 8),
                (self.NODE_CLASS, 'NodeTotalWeight', 'FloatProperty', 8, 4),
                (self.COVER_CLASS, 'WarPawnOwner', 'ObjectProperty', 16, 8),
                (self.COVER_CLASS, 'AnimCoverAction', 'ByteProperty', 40, 1),
                (self.COVER_CLASS, 'DesiredWeight', 'FloatProperty', 44, 4),
                (self.BLEND_CLASS, 'WarPawnOwner', 'ObjectProperty', 16, 8),
                (self.CHILD, 'Anim', 'ObjectProperty', 8, 8),
                (self.CHILD, 'Weight', 'FloatProperty', 16, 4),
                (self.MASK, 'DesiredWeight', 'FloatProperty', 0, 4),
                (self.MASK, 'BlendTimeToGo', 'FloatProperty', 4, 4)):
            self.add_property(owner, name, kind, offset, size)
        self.add_property(self.MASK, 'bPendingBlend', 'BoolProperty', 8, 4, mask=0x80)
        for obj, name, value in ((self.PAWN, 'Mesh', self.MESH), (self.MESH, 'Owner', self.PAWN),
                                  (self.MESH, 'Animations', self.ROOT),
                                  (self.MESH, 'AnimTreeTemplate', self.TEMPLATE),
                                  (self.MESH, 'SkeletalMesh', self.ASSET),
                                  (self.COVER, 'WarPawnOwner', self.PAWN),
                                  (self.BLEND, 'WarPawnOwner', self.PAWN)):
            self.set_value(obj, name, value, '<Q')
        for obj in (self.ROOT, self.COVER, self.BLEND, self.LEAF, self.LEAF2):
            self.set_value(obj, 'SkelComponent', self.MESH, '<Q')
            self.set_value(obj, 'NodeTotalWeight', 1, '<f')
        self.set_value(self.COVER, 'AnimCoverAction', 7, '<B')
        self.set_value(self.COVER, 'DesiredWeight', .5, '<f')
        self.set_children(self.ROOT, [self.COVER, self.BLEND])
        self.set_children(self.COVER, [self.LEAF, self.LEAF2], weights=[.75, .25])
        self.set_children(self.BLEND, [self.LEAF2])
        data = self.make_array(self.BLEND, 'MaskList', 40, self.MASK, 16, 2)
        self.put(data, struct.pack('<ffIIffII', .6, .2, 0x81, 0, .4, .3, 1, 0))
        # Opaque pose payload intentionally unmapped: the observer must not read
        # guessed native bone records or map root/spine/head by guessed indices.
        self.make_array(self.MESH, 'SpaceBases', 24, self.ATOM, 32, 100)
        self.make_array(self.ASSET, 'RefSkeleton', 0, None, 4, 100, kind='IntProperty')

    def add_class(self, address, name, parent):
        self.add_object(address, name, 0x99000)
        self.supers[address] = parent
        self.put(address + self.profile['field_super'], struct.pack('<Q', parent))

    def property(self, owner, name, inherited):
        while owner:
            if (owner, name) in self.properties:
                return dict(self.properties[owner, name])
            owner = self.supers.get(owner, 0) if inherited else 0
        raise RuntimeError('Property ' + name + ' not found')

    def value_address(self, obj, name):
        return obj + self.property(self.objects[obj]['cls'], name, True)['offset']

    def make_array(self, obj, name, offset, typ, stride, count, *, kind='StructProperty'):
        cls = self.objects[obj]['cls']
        try:
            field = self.property(cls, name, True)
        except RuntimeError:
            owner = self.BASE_CLASS if name == 'Children' else cls
            self.add_property(owner, name, 'ArrayProperty', offset, 16)
            field = self.property(cls, name, True)
            inner = self.next_inner
            self.next_inner += 0x100
            self.add_object(inner, name + 'Element', 0x20600, outer=field['address'])
            self.kinds[inner] = kind
            self.put(inner + 0x70, struct.pack('<II', 1, stride))
            self.put(field['address'] + self.profile['struct_property_struct'], struct.pack('<Q', inner))
            if typ is not None:
                self.put(inner + self.profile['struct_property_struct'], struct.pack('<Q', typ))
        data = self.next_data
        self.next_data += 0x1000
        self.put(obj + offset, struct.pack('<Qii', data, count, count))
        self.arrays[obj, name] = data
        return data

    def set_children(self, obj, pointers, *, weights=None):
        data = self.make_array(obj, 'Children', 24, self.CHILD, 32, len(pointers))
        for i, ptr in enumerate(pointers):
            row = bytearray(32)
            struct.pack_into('<Qf', row, 8, ptr, (weights or [1] * len(pointers))[i])
            self.put(data + i * 32, row)

    def generic(self, address, *, blend=False):
        self.add_object(address, 'RuntimeNode', self.GENERIC_CLASS if blend else self.LEAF_CLASS)
        self.set_value(address, 'SkelComponent', self.MESH, '<Q')
        if blend:
            self.set_children(address, [])
        return address

    def capture(self):
        return capture(self.observer)


class CoverAnimationTests(unittest.TestCase):
    def result(self, fixture, accepted):
        row = fixture.capture()
        self.assertIs(row['accepted'], accepted, row.get('rejection'))
        self.assertIs(row['complete'], accepted)
        self.assertTrue(row['read_only'])
        for key in ('atomic_snapshot_verified', 'renderer_alignment_verified',
                    'native_cover_state_certified', 'collision_verified', 'activation_allowed'):
            self.assertIs(row[key], False)
        if not accepted:
            self.assertIn('rejection', row)
        return row

    def test_live_owned_graph_shared_edges_weights_masks_and_explicit_bone_gap(self):
        f = AnimationMemory()
        row = self.result(f, True)
        self.assertEqual(row['visited_nodes'], 5)
        cover = next(n for n in row['nodes'] if n['identity']['address'] == f.COVER)
        self.assertEqual(cover['fields']['AnimCoverAction'], 7)
        self.assertEqual([c['Weight'] for c in cover['children']], [.75, .25])
        self.assertFalse(cover['children'][0]['DesiredWeight']['available'])
        blend = next(n for n in row['nodes'] if n['identity']['address'] == f.BLEND)
        self.assertAlmostEqual(blend['MaskList']['rows'][0]['DesiredWeight'], .6)
        self.assertEqual([m['bPendingBlend'] for m in blend['MaskList']['rows']], [True, False])
        self.assertFalse(row['bones']['available'])
        self.assertEqual(row['bones']['matrices'], [])
        self.assertEqual(row['bones']['RefSkeleton']['inner_kind'], 'IntProperty')
        self.assertNotIn((f.arrays[f.MESH, 'SpaceBases'], 3200), f.read_counts)

    def test_derived_cover_class_is_still_reported(self):
        f = AnimationMemory()
        cls = 0x99100
        f.add_class(cls, 'SpecialCoverBlend', f.COVER_CLASS)
        f.objects[f.COVER]['cls'] = cls
        self.assertEqual(self.result(f, True)['cover_nodes'][0]['address'], f.COVER)

    def test_missing_optional_fields_are_explicit_without_hiding_malformed_fields(self):
        f = AnimationMemory()
        del f.properties[f.BLEND_CLASS, 'MaskList']
        row = self.result(f, True)
        blend = next(n for n in row['nodes'] if n['identity']['address'] == f.BLEND)
        self.assertFalse(blend['MaskList']['available'])
        f = AnimationMemory()
        f.layout(f.COVER_CLASS, 'DesiredWeight', kind='IntProperty')
        self.result(f, False)

    def test_world_possession_mesh_animation_and_node_ownership_mismatches(self):
        cases = [(AnimationMemory.PAWN, 'Controller', AnimationMemory.OTHER),
                 (AnimationMemory.PC, 'WorldInfo', AnimationMemory.OTHER),
                 (AnimationMemory.MESH, 'Owner', AnimationMemory.OTHER),
                 (AnimationMemory.MESH, 'Animations', AnimationMemory.TEMPLATE),
                 (AnimationMemory.COVER, 'SkelComponent', 0),
                 (AnimationMemory.LEAF2, 'SkelComponent', AnimationMemory.OTHER),
                 (AnimationMemory.COVER, 'WarPawnOwner', AnimationMemory.OTHER)]
        for obj, name, pointer in cases:
            with self.subTest(name=name, obj=obj):
                f = AnimationMemory()
                f.set_value(obj, name, pointer, '<Q')
                self.result(f, False)
        f = AnimationMemory()
        f.objects[f.COVER]['name'] = 'Default__WarAnim_CoverBlend'
        self.result(f, False)

    def test_children_inner_stride_field_type_and_bounds_are_authenticated(self):
        for mode in ('count', 'null_data', 'stride', 'inner_owner', 'type', 'field_escape', 'structure'):
            with self.subTest(mode=mode):
                f = AnimationMemory()
                field = f.property(f.BASE_CLASS, 'Children', False)
                inner = f.u64(field['address'] + f.profile['struct_property_struct'])
                if mode == 'count':
                    f.put(f.ROOT + 24, struct.pack('<Qii', f.arrays[f.ROOT, 'Children'], 33, 33))
                elif mode == 'null_data':
                    f.put(f.ROOT + 24, struct.pack('<Qii', 0, 1, 1))
                elif mode == 'stride':
                    f.put(inner + 0x70, struct.pack('<II', 1, 1030))
                elif mode == 'inner_owner':
                    f.objects[inner]['outer'] = 0
                elif mode == 'type':
                    f.layout(f.CHILD, 'Anim', kind='IntProperty')
                elif mode == 'field_escape':
                    f.layout(f.CHILD, 'Anim', offset=28)
                else:
                    f.objects[f.CHILD]['name'] = 'UnrelatedStructure'
                self.result(f, False)

    def test_cycles_reject_but_shared_dag_edges_do_not(self):
        f = AnimationMemory()
        f.set_children(f.COVER, [f.ROOT])
        self.result(f, False)

    def test_null_child_and_missing_bone_metadata_remain_honest_diagnostics(self):
        f = AnimationMemory()
        f.set_children(f.COVER, [0, f.LEAF])
        del f.properties[f.MESH_CLASS, 'SpaceBases']
        del f.properties[f.ASSET_CLASS, 'RefSkeleton']
        row = self.result(f, True)
        cover = next(n for n in row['nodes'] if n['identity']['address'] == f.COVER)
        self.assertIsNone(cover['children'][0]['Anim'])
        self.assertFalse(row['bones']['SpaceBases']['available'])
        self.assertFalse(row['bones']['RefSkeleton']['available'])

    def test_reader_loss_does_not_return_an_accepted_partial_tree(self):
        f = AnimationMemory()
        def lose_edge(address, size, count):
            if address == f.arrays[f.COVER, 'Children'] + 8:
                raise OSError('Synthetic read loss')
        f.before_read = lose_edge
        row = self.result(f, False)
        self.assertTrue(row['nodes'])
        f = AnimationMemory()
        f.observer.resolver = None
        self.result(f, False)

    def test_exact_depth_limit(self):
        for length, accepted in ((32, True), (33, False)):
            with self.subTest(length=length):
                f = AnimationMemory()
                chain = [f.generic(0x400000 + i * 0x100, blend=True) for i in range(length)]
                f.set_children(f.ROOT, chain[:1])
                for left, right in zip(chain, chain[1:]):
                    f.set_children(left, [right])
                self.result(f, accepted)

    def test_exact_node_limit(self):
        for count, accepted in ((256, True), (257, False)):
            with self.subTest(count=count):
                f = AnimationMemory()
                groups = [f.generic(0x400000 + i * 0x100, blend=True) for i in range(8)]
                f.set_children(f.ROOT, groups)
                remaining, address = count - 9, 0x500000
                for group in groups:
                    leaves = [f.generic(address + i * 0x100) for i in range(min(31, remaining))]
                    f.set_children(group, leaves)
                    remaining -= len(leaves)
                    address += 0x10000
                self.result(f, accepted)

    def test_nonfinite_weights_and_malformed_boolean_masks_reject(self):
        for location in ('child', 'node', 'mask'):
            for value in (float('nan'), float('inf')):
                with self.subTest(location=location, value=value):
                    f = AnimationMemory()
                    address = {'child': f.arrays[f.COVER, 'Children'] + 16,
                               'node': f.COVER + 44, 'mask': f.arrays[f.BLEND, 'MaskList']}[location]
                    f.put(address, struct.pack('<f', value))
                    self.result(f, False)
        for mask in (0, 3):
            f = AnimationMemory()
            f.put(f.properties[f.MASK, 'bPendingBlend']['address'] + 0xd0, struct.pack('<I', mask))
            self.result(f, False)

    def test_closing_selected_value_metadata_edges_ancestry_owner_and_identity_mutation(self):
        for mode in ('weight', 'mask', 'field_metadata', 'edge', 'ancestry', 'owner',
                     'world', 'possession', 'identity', 'session'):
            with self.subTest(mode=mode):
                f = AnimationMemory()
                def mutate(call):
                    if call != 2:
                        return
                    if mode == 'weight':
                        f.put(f.arrays[f.COVER, 'Children'] + 16, struct.pack('<f', .125))
                    elif mode == 'mask':
                        f.put(f.properties[f.MASK, 'bPendingBlend']['address'] + 0xd0, struct.pack('<I', 4))
                    elif mode == 'field_metadata':
                        f.layout(f.CHILD, 'Weight', offset=20)
                    elif mode == 'edge':
                        f.put(f.arrays[f.COVER, 'Children'] + 8, struct.pack('<Q', f.BLEND))
                    elif mode == 'ancestry':
                        f.put(f.COVER_CLASS + f.profile['field_super'], struct.pack('<Q', 0))
                    elif mode == 'owner':
                        f.set_value(f.MESH, 'Owner', f.OTHER, '<Q')
                    elif mode == 'world':
                        f.set_value(f.PAWN, 'WorldInfo', f.OTHER, '<Q')
                    elif mode == 'possession':
                        f.set_value(f.PC, 'Pawn', f.OTHER, '<Q')
                    elif mode == 'identity':
                        f.objects[f.COVER]['internal_index'] += 1
                    else:
                        raise RuntimeError('Session replaced')
                f.at_verify = mutate
                self.result(f, False)

    def test_unselected_graph_weight_is_not_an_observed_cover_value(self):
        f = AnimationMemory()
        def mutate(call):
            if call == 2:
                f.put(f.arrays[f.ROOT, 'Children'] + 16, struct.pack('<f', .125))
        f.at_verify = mutate
        self.result(f, True)

    def test_optional_child_mirror_uses_typed_word_and_d0_mask(self):
        f = AnimationMemory()
        f.add_property(f.CHILD, 'bMirrorSkeleton', 'BoolProperty', 28, 4, mask=1)
        data = f.arrays[f.COVER, 'Children']
        f.put(data + 28, struct.pack('<I', 0x80000001))
        f.put(data + 32 + 28, struct.pack('<I', 0x80000000))
        row = self.result(f, True)
        cover = next(n for n in row['nodes'] if n['identity']['address'] == f.COVER)
        values = [child['bMirrorSkeleton'] for child in cover['children']]
        self.assertEqual([v['value'] for v in values], [True, False])
        self.assertTrue(all(v['mask'] == 1 and v['field_offset'] == 28 and v['child_stride'] == 32 for v in values))
        self.assertTrue(all(v['native_consumer_verified'] is False for v in values))

    def test_child_mirror_invalid_mask_and_element_extent_reject(self):
        for mask, offset, kind in ((0, 28, 'BoolProperty'), (3, 28, 'BoolProperty'),
                                   (1, 32, 'BoolProperty'), (1, 28, 'IntProperty')):
            with self.subTest(mask=mask, offset=offset, kind=kind):
                f = AnimationMemory()
                f.add_property(f.CHILD, 'bMirrorSkeleton', kind, offset, 4, mask=mask)
                self.result(f, False)

    def test_child_mirror_closing_mask_or_word_change_reject(self):
        for change_mask in (False, True):
            with self.subTest(change_mask=change_mask):
                f = AnimationMemory()
                f.add_property(f.CHILD, 'bMirrorSkeleton', 'BoolProperty', 28, 4, mask=1)
                def mutate(call):
                    if call == 2:
                        address = (f.properties[f.CHILD, 'bMirrorSkeleton']['address'] + 0xd0
                                   if change_mask else f.arrays[f.COVER, 'Children'] + 28)
                        f.put(address, struct.pack('<I', 2 if change_mask else 1))
                f.at_verify = mutate
                self.result(f, False)


if __name__ == '__main__':
    unittest.main()
