"""Synthetic geometry/metadata only; no game, process, DLL or input calls."""
import json
import math
import struct
import unittest

from cover_state import CoverQuery
from cover_geometry_state import capture, slot_world_candidates
from test_cover_state import MemoryFixture


class GeometryMemory(MemoryFixture):
    def __init__(self):
        super().__init__()
        self.add_object(0x20500, 'ScriptStruct', 0x20800)
        for owner, name, offset, target in (
                (self.LINK_CLASS, 'Location', 32, self.VECTOR),
                (self.LINK_CLASS, 'Rotation', 44, self.ROTATOR),
                (self.PAWN_CLASS, 'Location', 64, self.VECTOR),
                (self.PAWN_CLASS, 'Rotation', 76, self.ROTATOR),
                (self.LINK_CLASS, 'StandingLeanOffset', 64, self.VECTOR),
                (self.LINK_CLASS, 'CrouchLeanOffset', 76, self.VECTOR)):
            self.add_property(owner, name, 'StructProperty', offset, 12, target=target)
        self.add_property(self.LINK_CLASS, 'StandHeight', 'FloatProperty', 56, 4)
        self.add_property(self.LINK_CLASS, 'MidHeight', 'FloatProperty', 60, 4)
        self.set_value(self.LINK, 'Location', (100, 200, 300), '<3f')
        self.set_value(self.LINK, 'Rotation', (0, 16384, 0), '<3i')
        self.set_value(self.PAWN, 'Location', (95, 215, 310), '<3f')
        self.set_value(self.PAWN, 'Rotation', (0, -16384, 0), '<3i')
        self.set_value(self.LINK, 'StandHeight', 155.5, '<f')
        self.set_value(self.LINK, 'MidHeight', 83.25, '<f')
        self.set_value(self.LINK, 'StandingLeanOffset', (1, 2, 3), '<3f')
        self.set_value(self.LINK, 'CrouchLeanOffset', (4, 5, 6), '<3f')

    def property(self, owner, name, inherited):
        try:
            return super().property(owner, name, inherited)
        except KeyError:
            raise RuntimeError('Property ' + name + ' not found') from None

    def set_value(self, obj, name, value, fmt):
        values = value if isinstance(value, tuple) else (value,)
        self.put(self.value_address(obj, name), struct.pack(fmt, *values))

    def capture(self, **kwargs):
        return capture(self.observer, **kwargs)


class GeometryTests(unittest.TestCase):
    def result(self, fixture, accepted=True, **kwargs):
        row = fixture.capture(**kwargs)
        self.assertIs(row['accepted'], accepted, row.get('rejection'))
        self.assertTrue(row['read_only'])
        for key in ('engine_calls', 'activation_allowed', 'actor_or_mesh_fields_written',
                    'atomic_snapshot_verified', 'native_cover_state_certified',
                    'native_getter_verified', 'collision_verified', 'renderer_alignment_verified'):
            self.assertIs(row[key], False)
        json.dumps(row, allow_nan=False)
        if not accepted:self.assertIn('rejection', row)
        return row

    def test_current_link_pawn_raw_transforms_live_parameters_and_slot_layouts(self):
        f, queries = GeometryMemory(), []
        def factory(observer):
            q = CoverQuery(observer)
            queries.append(q)
            return q
        row = self.result(f, query_factory=factory)
        self.assertEqual(len(queries), 1)
        self.assertLess(queries[0].reads, 1024)
        self.assertEqual(row['link']['address'], f.LINK)
        self.assertEqual(row['pawn']['address'], f.PAWN)
        self.assertEqual(row['link_transform']['Location']['value'], [100., 200., 300.])
        self.assertEqual(row['link_transform']['Rotation']['value'], [0, 16384, 0])
        self.assertEqual(row['pawn_transform']['Location']['value'], [95., 215., 310.])
        self.assertEqual(row['pawn_transform']['Rotation']['value'], [0, -16384, 0])
        self.assertEqual(row['link_parameters']['StandHeight']['value'], 155.5)
        self.assertEqual(row['link_parameters']['MidHeight']['value'], 83.25)
        self.assertEqual(row['link_parameters']['StandingLeanOffset']['value'], [1., 2., 3.])
        self.assertEqual(row['link_parameters']['CrouchLeanOffset']['value'], [4., 5., 6.])
        self.assertEqual(row['unavailable'], [])
        for transform in (row['link_transform'], row['pawn_transform']):
            for name, kind in (('Location', 'FloatProperty'), ('Rotation', 'IntProperty')):
                proof = transform[name]
                self.assertEqual(len(bytes.fromhex(proof['raw'])), 12)
                self.assertEqual([p['offset'] for p in proof['layout']['components']], [0, 4, 8])
                self.assertEqual([p['type'] for p in proof['layout']['components']], [kind] * 3)
        self.assertEqual(row['booleans']['bIsInStationaryCover']['mask'], 0x100)
        self.assertIs(row['booleans']['bIsInStationaryCover']['value'], True)
        self.assertEqual(row['booleans']['bCircular']['mask_bytes'], '20000000')
        self.assertIs(row['booleans']['bCircular']['value'], False)
        self.assertEqual([s['index'] for s in row['slots']['rows']], [0, 1, 2])
        for slot in row['slots']['rows']:
            self.assertEqual(struct.unpack('<3f', bytes.fromhex(slot['raw_fields']['LocationOffset'])), tuple(slot['LocationOffset']))
            self.assertEqual(struct.unpack('<3i', bytes.fromhex(slot['raw_fields']['RotationOffset'])), tuple(slot['RotationOffset']))
        self.assertEqual(row['slots']['boolean_masks']['bCanPopUp']['mask'], 0x80)
        candidate = row['slot_world_candidates']
        self.assertTrue(candidate['candidate_only'])
        self.assertFalse(candidate['native_getter_verified'])
        self.assertFalse(candidate['collision_verified'])
        for actual, expected in zip(candidate['rows'][0]['position'], [78., 211., 333.]):
            self.assertAlmostEqual(actual, expected)

    def test_missing_optional_parameters_stay_unavailable_without_defaults(self):
        f = GeometryMemory()
        names = ('StandHeight', 'MidHeight', 'StandingLeanOffset', 'CrouchLeanOffset')
        for name in names:del f.properties[f.LINK_CLASS, name]
        row = self.result(f)
        self.assertEqual(row['unavailable'], list(names))
        for item in row['link_parameters'].values():
            self.assertFalse(item['available'])
            self.assertIn('not reflected', item['reason'])
            self.assertNotIn('value', item)
            self.assertNotIn('raw', item)

    def test_missing_required_transform_or_link_is_rejected(self):
        for defect in ('link', 'link_location', 'link_rotation', 'pawn_location', 'pawn_rotation'):
            with self.subTest(defect=defect):
                f = GeometryMemory()
                if defect == 'link':f.set_value(f.PAWN, 'CurrentLink', 0, '<Q')
                else:
                    obj, name = defect.split('_')
                    del f.properties[f.LINK_CLASS if obj == 'link' else f.PAWN_CLASS, name.title()]
                self.result(f, False)

    def test_possession_classes_and_every_world_reference_must_match(self):
        for obj, name, pointer in ((GeometryMemory.PC, 'Pawn', 0),
                                  (GeometryMemory.PAWN, 'Controller', GeometryMemory.OTHER),
                                  (GeometryMemory.PAWN, 'WorldInfo', 0),
                                  (GeometryMemory.PC, 'WorldInfo', GeometryMemory.OTHER),
                                  (GeometryMemory.LINK, 'WorldInfo', GeometryMemory.OTHER)):
            with self.subTest(obj=obj, name=name):
                f = GeometryMemory()
                f.set_value(obj, name, pointer, '<Q')
                self.result(f, False)
        for obj in (GeometryMemory.PAWN, GeometryMemory.LINK):
            f = GeometryMemory()
            f.objects[obj]['cls'] = 0x99999
            self.result(f, False)

    def test_struct_type_dimensions_components_and_scalar_types_are_explicit(self):
        for defect in ('vector_pointer', 'rotator_pointer', 'scriptstruct_class', 'struct_width',
                       'component_kind', 'component_offset', 'component_width', 'component_dim',
                       'height_kind', 'height_width', 'lean_kind', 'slot_vector_type', 'slot_rotator_kind'):
            with self.subTest(defect=defect):
                f = GeometryMemory()
                if defect in ('vector_pointer', 'rotator_pointer'):
                    name, target = ('Location', f.ROTATOR) if defect == 'vector_pointer' else ('Rotation', f.VECTOR)
                    f.put(f.properties[f.LINK_CLASS, name]['address'] + 0xb0, struct.pack('<Q', target))
                elif defect == 'scriptstruct_class':f.objects[0x20500]['name'] = 'Object'
                elif defect == 'struct_width':f.layout(f.PAWN_CLASS, 'Location', size=8)
                elif defect == 'component_kind':f.layout(f.VECTOR, 'X', kind='IntProperty')
                elif defect == 'component_offset':f.layout(f.ROTATOR, 'Yaw', offset=0)
                elif defect == 'component_width':f.layout(f.VECTOR, 'Z', size=8)
                elif defect == 'component_dim':f.put(f.properties[f.VECTOR, 'X']['address'] + 0x70, struct.pack('<I', 2))
                elif defect == 'height_kind':f.layout(f.LINK_CLASS, 'StandHeight', kind='IntProperty')
                elif defect == 'height_width':f.layout(f.LINK_CLASS, 'MidHeight', size=8)
                elif defect == 'lean_kind':f.layout(f.LINK_CLASS, 'CrouchLeanOffset', kind='IntProperty')
                elif defect == 'slot_vector_type':
                    f.put(f.properties[f.SLOT, 'LocationOffset']['address'] + 0xb0, struct.pack('<Q', f.ROTATOR))
                elif defect == 'slot_rotator_kind':f.layout(f.ROTATOR, 'Roll', kind='FloatProperty')
                self.result(f, False)

    def test_nonfinite_location_lean_height_slot_and_invalid_fraction_are_rejected(self):
        for name, obj in (('Location', GeometryMemory.LINK), ('Location', GeometryMemory.PAWN),
                          ('StandingLeanOffset', GeometryMemory.LINK), ('CrouchLeanOffset', GeometryMemory.LINK),
                          ('StandHeight', GeometryMemory.LINK), ('MidHeight', GeometryMemory.LINK)):
            for value in (float('nan'), float('inf'), -float('inf')):
                with self.subTest(name=name, obj=obj, value=value):
                    f = GeometryMemory()
                    f.put(f.value_address(obj, name), struct.pack('<f', value))
                    self.result(f, False)
        for value in (float('nan'), float('inf'), -.01, 1.01):
            f = GeometryMemory()
            f.set_value(f.PAWN, 'CurrentSlotPct', value, '<f')
            self.result(f, False)
        f = GeometryMemory()
        f.put(f.DATA + 48 + 20, struct.pack('<f', float('nan')))
        self.result(f, False)

    def test_boolean_masks_widths_and_selected_slot_bounds_remain_strict(self):
        for owner, name in ((GeometryMemory.PAWN_CLASS, 'bIsInStationaryCover'),
                            (GeometryMemory.LINK_CLASS, 'bCircular'), (GeometryMemory.SLOT, 'bLeanLeft')):
            for mask in (0, 3):
                f = GeometryMemory()
                f.put(f.properties[owner, name]['address'] + 0xd0, struct.pack('<I', mask))
                self.result(f, False)
            f = GeometryMemory()
            f.layout(owner, name, size=1)
            self.result(f, False)
        for index in (-1, 3):
            f = GeometryMemory()
            f.set_value(f.PAWN, 'CurrentSlotIdx', index, '<i')
            self.result(f, False)
        f = GeometryMemory()
        f.set_array(f.DATA, 3, 2)
        self.result(f, False)

    def test_shared_slot_boolean_word_and_repeated_current_selection_keep_independent_masks(self):
        f = GeometryMemory()
        names = ('bEnabled', 'bLeanLeft', 'bLeanRight', 'bCanPopUp')
        for name in names:f.layout(f.SLOT, name, offset=0)
        for name in ('CurrentSlotIdx', 'LeftSlotIdx', 'RightSlotIdx'):f.set_value(f.PAWN, name, 0, '<i')
        f.put(f.DATA, struct.pack('<I', 8 | 0x20000000 | 0x80))
        row = self.result(f)
        self.assertEqual(len(row['slots']['rows']), 1)
        slot = row['slots']['rows'][0]
        self.assertEqual([slot[name] for name in names], [True, False, True, True])
        self.assertEqual(len(set(slot['raw_fields'][name] for name in names)), 1)
        self.assertEqual([row['slots']['boolean_masks'][name]['mask'] for name in names], [8, 4, 0x20000000, 0x80])

    def test_closing_bracket_catches_raw_transform_parameter_slot_and_metadata_changes(self):
        for defect in ('link_location', 'link_rotation', 'pawn_location', 'pawn_rotation', 'height', 'lean',
                       'world', 'slot', 'mask', 'struct_link', 'component_layout', 'identity', 'registry'):
            with self.subTest(defect=defect):
                f = GeometryMemory()
                def mutate(call):
                    if call != 2:return
                    if defect in ('link_location', 'link_rotation', 'pawn_location', 'pawn_rotation'):
                        obj, name = defect.split('_')
                        f.put(f.value_address(f.LINK if obj == 'link' else f.PAWN, name.title()), bytes(4))
                        if name == 'rotation':
                            f.put(f.value_address(f.LINK if obj == 'link' else f.PAWN, 'Rotation') + 4, bytes(4))
                    elif defect == 'height':f.set_value(f.LINK, 'StandHeight', 999, '<f')
                    elif defect == 'lean':f.set_value(f.LINK, 'StandingLeanOffset', (9, 9, 9), '<3f')
                    elif defect == 'world':f.set_value(f.LINK, 'WorldInfo', f.OTHER, '<Q')
                    elif defect == 'slot':f.put(f.DATA + 20, struct.pack('<f', 999))
                    elif defect == 'mask':f.put(f.properties[f.SLOT, 'bCanPopUp']['address'] + 0xd0, struct.pack('<I', 4))
                    elif defect == 'struct_link':f.put(f.properties[f.LINK_CLASS, 'Location']['address'] + 0xb0, struct.pack('<Q', f.ROTATOR))
                    elif defect == 'component_layout':f.layout(f.VECTOR, 'X', offset=4)
                    elif defect == 'identity':f.objects[f.LINK]['internal_index'] += 1
                    elif defect == 'registry':f.put(f.names_header + 8, struct.pack('<i', 101))
                f.at_verify = mutate
                self.result(f, False)

    def test_read_read_and_second_metadata_barrier_are_not_relaxed(self):
        f = GeometryMemory()
        address = f.value_address(f.LINK, 'Location')
        def mutate(address_now, size, count):
            if (address_now, size, count) == (address, 12, 2):f.put(address, struct.pack('<f', 999))
        f.before_read = mutate
        self.result(f, False)
        f = GeometryMemory()
        def change_identity(call):
            if call == 3:f.objects[f.LINK]['internal_index'] += 1
        f.at_verify = change_identity
        self.result(f, False)

    def test_pure_candidate_rotation_is_explicit_and_composes_position_and_basis(self):
        slots = [dict(index=7, LocationOffset=[2., 3., 4.], RotationOffset=[0, 0, 0])]
        for rotation, expected in (([0, 0, 0], [12., 23., 34.]), ([0, 16384, 0], [7., 22., 34.]),
                                   ([0, 65536, 0], [12., 23., 34.]), ([16384, 0, 0], [6., 23., 32.])):
            row = slot_world_candidates([10., 20., 30.], rotation, slots)
            self.assertTrue(row['candidate_only'])
            for actual, value in zip(row['rows'][0]['position'], expected):self.assertAlmostEqual(actual, value)
        slots[0]['RotationOffset'] = [0, 16384, 0]
        row = slot_world_candidates([0, 0, 0], [0, 16384, 0], slots)['rows'][0]
        for name, expected in (('forward', [-1., 0., 0.]), ('right', [0., -1., 0.]), ('up', [0., 0., 1.])):
            for actual, value in zip(row[name], expected):self.assertAlmostEqual(actual, value)
        row = slot_world_candidates([0, 0, 0], [3000, -10000, 7000], slots)['rows'][0]
        basis = [row[name] for name in ('forward', 'right', 'up')]
        for i, axis in enumerate(basis):
            self.assertAlmostEqual(sum(x * x for x in axis), 1.)
            for other in basis[i + 1:]:self.assertAlmostEqual(sum(a * b for a, b in zip(axis, other)), 0.)


if __name__ == '__main__':
    unittest.main()
