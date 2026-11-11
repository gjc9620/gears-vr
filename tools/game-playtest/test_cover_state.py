"""Synthetic reflected memory only; no process reads, game calls or input."""
import copy
import struct
from types import SimpleNamespace
import unittest

from cover_state import CoverQuery, boolean, capture, slot_snapshot


class MemoryFixture:
    """Exercise the real Query bounds, metadata, Boolean and closing checks."""
    PC, PAWN, WORLD, LINK, OTHER = 0x10000, 0x11000, 0x12000, 0x13000, 0x14000
    PC_CLASS, PAWN_CLASS, LINK_CLASS = 0x20000, 0x20100, 0x20200
    SLOT, VECTOR, ROTATOR, INNER, DATA = 0x21000, 0x22000, 0x23000, 0x60000, 0x70000

    def __init__(self):
        self.memory, self.objects, self.properties, self.kinds = {}, {}, {}, {}
        self.read_counts, self.verify_calls = {}, 0
        self.before_read = self.at_verify = None
        self.objects_header, self.names_header = 0x100, 0x120
        self.profile = dict(struct_property_struct=0xb0)
        self.next_property = 0x30000
        self.put(self.objects_header, struct.pack('<Qii', 0x1000, 100, 256))
        self.put(self.names_header, struct.pack('<Qii', 0x2000, 100, 256))
        for address, name, cls in ((self.PC, 'LocalController', self.PC_CLASS),
                                   (self.PAWN, 'LocalPawn', self.PAWN_CLASS),
                                   (self.WORLD, 'World', 0x20300),
                                   (self.LINK, 'CurrentCover', self.LINK_CLASS),
                                   (self.OTHER, 'OtherObject', 0x20400),
                                   (self.SLOT, 'CoverSlot', 0x20500),
                                   (self.VECTOR, 'Vector', 0x20500),
                                   (self.ROTATOR, 'Rotator', 0x20500)):
            self.add_object(address, name, cls)
        for owner, name, offset in ((self.PC_CLASS, 'Pawn', 0), (self.PC_CLASS, 'WorldInfo', 8),
                                    (self.PAWN_CLASS, 'Controller', 0), (self.PAWN_CLASS, 'WorldInfo', 8),
                                    (self.PAWN_CLASS, 'CurrentLink', 16), (self.LINK_CLASS, 'WorldInfo', 0)):
            self.add_property(owner, name, 'ObjectProperty', offset, 8)
        for name, kind, offset, size, value, fmt in (
                ('CoverType', 'ByteProperty', 24, 1, 2, '<B'),
                ('CoverAction', 'ByteProperty', 25, 1, 7, '<B'),
                ('CoverDirection', 'ByteProperty', 26, 1, 1, '<B'),
                ('CurrentSlotIdx', 'IntProperty', 32, 4, 1, '<i'),
                ('LeftSlotIdx', 'IntProperty', 36, 4, 0, '<i'),
                ('RightSlotIdx', 'IntProperty', 40, 4, 2, '<i'),
                ('CurrentSlotPct', 'FloatProperty', 44, 4, .25, '<f')):
            self.add_property(self.PAWN_CLASS, name, kind, offset, size)
            self.set_value(self.PAWN, name, value, fmt)
        self.add_property(self.PAWN_CLASS, 'bIsInStationaryCover', 'BoolProperty', 48, 4, mask=0x100)
        self.set_value(self.PAWN, 'bIsInStationaryCover', 0x101, '<I')
        self.add_property(self.LINK_CLASS, 'bCircular', 'BoolProperty', 8, 4, mask=0x20)
        self.set_value(self.LINK, 'bCircular', 1, '<I')  # Other set bits are not this Boolean.
        self.add_property(self.LINK_CLASS, 'Slots', 'ArrayProperty', 16, 16, target=self.INNER)
        self.add_object(self.INNER, 'SlotElement', 0x20600,
                        outer=self.properties[self.LINK_CLASS, 'Slots']['address'])
        self.kinds[self.INNER] = 'StructProperty'
        self.put(self.INNER + 0x70, struct.pack('<II', 1, 48))
        self.put(self.INNER + self.profile['struct_property_struct'], struct.pack('<Q', self.SLOT))
        for name, offset, mask in (('bEnabled', 0, 8), ('bLeanLeft', 4, 4),
                                    ('bLeanRight', 8, 0x20000000), ('bCanPopUp', 12, 0x80)):
            self.add_property(self.SLOT, name, 'BoolProperty', offset, 4, mask=mask)
        self.add_property(self.SLOT, 'CoverType', 'ByteProperty', 16, 1)
        self.add_property(self.SLOT, 'LocationOffset', 'StructProperty', 20, 12, target=self.VECTOR)
        self.add_property(self.SLOT, 'RotationOffset', 'StructProperty', 32, 12, target=self.ROTATOR)
        for typ, names, kind in ((self.VECTOR, ('X', 'Y', 'Z'), 'FloatProperty'),
                                  (self.ROTATOR, ('Pitch', 'Yaw', 'Roll'), 'IntProperty')):
            for i, name in enumerate(names):
                self.add_property(typ, name, kind, i * 4, 4)
        for i in range(3):
            row = bytearray(48)
            struct.pack_into('<4I', row, 0, 8, 4 if i != 1 else 0,
                             0x20000000 if i == 1 else 0, 0x80 if i != 1 else 0)
            row[16] = i + 1
            struct.pack_into('<3f', row, 20, 11 + i, 22 + 2 * i, 33 + 3 * i)
            struct.pack_into('<3i', row, 32, 10000 + i, -20000 - i, 30000 + i)
            self.put(self.DATA + i * 48, row)
        self.set_array(self.DATA, 3, 3)
        for obj, name, pointer in ((self.PC, 'Pawn', self.PAWN), (self.PC, 'WorldInfo', self.WORLD),
                                    (self.PAWN, 'Controller', self.PC), (self.PAWN, 'WorldInfo', self.WORLD),
                                    (self.PAWN, 'CurrentLink', self.LINK), (self.LINK, 'WorldInfo', self.WORLD)):
            self.set_value(obj, name, pointer, '<Q')
        sampler = SimpleNamespace(controller=self.PC, fresh=self.fresh, derives=self.derives)
        self.observer = SimpleNamespace(resolver=self, sampler=sampler, verify=self.verify)

    def put(self, address, raw):
        self.memory.update((address + i, value) for i, value in enumerate(raw))

    def read(self, address, size):
        key = address, size
        self.read_counts[key] = self.read_counts.get(key, 0) + 1
        if self.before_read:
            self.before_read(address, size, self.read_counts[key])
        try:
            return bytes(self.memory[address + i] for i in range(size))
        except KeyError as error:
            raise OSError('Unmapped synthetic memory') from error

    def add_object(self, address, name, cls, *, outer=0):
        self.objects[address] = dict(address=address, name=name, cls=cls, number=0,
                                     outer=outer, internal_index=address // 8)

    def add_property(self, owner, name, kind, offset, size, *, mask=None, target=None):
        address = self.next_property
        self.next_property += 0x200
        self.add_object(address, name, 0x20700, outer=owner)
        self.kinds[address] = kind
        self.properties[owner, name] = dict(address=address, offset=offset, type=kind)
        metadata = bytearray(0x28)
        struct.pack_into('<II', metadata, 0, 1, size)
        struct.pack_into('<I', metadata, 0x24, offset)
        self.put(address + 0x70, metadata)
        if mask is not None:
            self.put(address + 0xd0, struct.pack('<I', mask))
        if target is not None:
            self.put(address + self.profile['struct_property_struct'], struct.pack('<Q', target))

    def property(self, owner, name, inherited):
        return dict(self.properties[owner, name])

    def fresh(self, address):
        return copy.deepcopy(self.objects[address])

    def derives(self, cls, name):
        return (cls, name) in ((self.PAWN_CLASS, 'WarPawn'), (self.LINK_CLASS, 'CoverLink'))

    def class_name(self, obj):
        return self.kinds[obj['address']]

    def u64(self, address):
        return struct.unpack('<Q', self.read(address, 8))[0]

    def struct_type(self, prop):
        return self.u64(prop['address'] + self.profile['struct_property_struct'])

    def value_address(self, obj, name):
        return obj + self.properties[self.objects[obj]['cls'], name]['offset']

    def set_value(self, obj, name, value, fmt):
        self.put(self.value_address(obj, name), struct.pack(fmt, value))

    def set_array(self, pointer, count, capacity):
        self.put(self.value_address(self.LINK, 'Slots'), struct.pack('<Qii', pointer, count, capacity))

    def layout(self, owner, name, *, offset=None, size=None, kind=None):
        prop = self.properties[owner, name]
        if offset is not None:
            prop['offset'] = offset
            self.put(prop['address'] + 0x94, struct.pack('<I', offset))
        if size is not None:
            self.put(prop['address'] + 0x74, struct.pack('<I', size))
        if kind is not None:
            prop['type'] = kind

    def verify(self):
        self.verify_calls += 1
        if self.at_verify:
            self.at_verify(self.verify_calls)

    def capture(self):
        return capture(self.observer)


class CoverObservationTests(unittest.TestCase):
    def diagnostic(self, result, *, accepted):
        self.assertIs(result['accepted'], accepted)
        self.assertIs(result['read_only'], True)
        self.assertIs(result['native_cover_state_certified'], False)
        self.assertIs(result['atomic_snapshot_verified'], False)
        self.assertIs(result['renderer_alignment_verified'], False)
        self.assertGreaterEqual(result['finished_perf_counter_ns'], result['started_perf_counter_ns'])
        if not accepted:
            self.assertIn('rejection', result)

    def test_readable_cover_is_only_diagnostic_and_preserves_bit_and_numeric_types(self):
        result = MemoryFixture().capture()
        self.diagnostic(result, accepted=True)
        self.assertIs(result['values']['bIsInStationaryCover'], True)
        self.assertIs(result['values']['bCircular'], False)
        rows = result['slots']['rows']
        self.assertEqual([r['index'] for r in rows], [0, 1, 2])
        self.assertEqual(rows[2]['LocationOffset'], [13., 26., 39.])
        self.assertEqual(rows[2]['RotationOffset'], [10002, -20002, 30002])
        self.assertTrue(all(type(x) is float for x in rows[2]['LocationOffset']))
        self.assertTrue(all(type(x) is int for x in rows[2]['RotationOffset']))
        self.assertEqual([r['bLeanLeft'] for r in rows], [True, False, True])
        self.assertEqual([r['bLeanRight'] for r in rows], [False, True, False])
        self.assertTrue(all(type(r['bCanPopUp']) is bool for r in rows))

    def test_selected_indices_are_unique_sorted_and_cannot_escape_live_array(self):
        fixture = MemoryFixture()
        query = CoverQuery(fixture.observer)
        result = slot_snapshot(query, query.obj(fixture.LINK), [2, 0, 2])
        self.assertEqual([r['index'] for r in result['rows']], [0, 2])
        query.finish()
        for selected in ([], [-1], [3], [0, 3], [True], [1.0], ['1'], [None]):
            with self.subTest(selected=selected):
                fixture = MemoryFixture()
                query = CoverQuery(fixture.observer)
                with self.assertRaises(RuntimeError):
                    slot_snapshot(query, query.obj(fixture.LINK), selected)

    def test_invalid_array_headers_and_element_stride_reject_without_admission(self):
        for pointer, count, capacity, stride, dim in (
                (0, 1, 1, 48, 1), (0x70000, -1, 3, 48, 1), (0x70000, 3, 2, 48, 1),
                (0x70000, 257, 257, 48, 1), (0x70000, 3, 3, 0, 1),
                (0x70000, 3, 3, 1025, 1), (0x70000, 3, 3, 48, 2),
                (0x70000, 128, 128, 1024, 1)):
            with self.subTest(count=count, capacity=capacity, stride=stride, dim=dim):
                fixture = MemoryFixture()
                fixture.set_array(pointer, count, capacity)
                fixture.put(fixture.INNER + 0x70, struct.pack('<II', dim, stride))
                self.diagnostic(fixture.capture(), accepted=False)

    def test_slot_fields_and_geometry_must_fit_their_typed_element(self):
        for defect in ('overflow', 'bool_width', 'slot_type', 'inner_type', 'vector_type', 'rotator_type',
                       'geometry_width', 'component_kind', 'component_offset', 'component_width'):
            with self.subTest(defect=defect):
                fixture = MemoryFixture()
                if defect == 'overflow':
                    fixture.layout(fixture.SLOT, 'bEnabled', offset=47)
                elif defect == 'bool_width':
                    fixture.layout(fixture.SLOT, 'bLeanLeft', size=1)
                elif defect == 'slot_type':
                    fixture.objects[fixture.SLOT]['name'] = 'UnrelatedStructure'
                elif defect == 'inner_type':
                    fixture.kinds[fixture.INNER] = 'IntProperty'
                elif defect == 'vector_type':
                    prop = fixture.properties[fixture.SLOT, 'LocationOffset']
                    fixture.put(prop['address'] + 0xb0, struct.pack('<Q', fixture.ROTATOR))
                elif defect == 'rotator_type':
                    prop = fixture.properties[fixture.SLOT, 'RotationOffset']
                    fixture.put(prop['address'] + 0xb0, struct.pack('<Q', fixture.VECTOR))
                elif defect == 'geometry_width':
                    fixture.layout(fixture.SLOT, 'RotationOffset', size=8)
                elif defect == 'component_kind':
                    fixture.layout(fixture.VECTOR, 'X', kind='IntProperty')
                elif defect == 'component_offset':
                    fixture.layout(fixture.ROTATOR, 'Yaw', offset=0)
                elif defect == 'component_width':
                    fixture.layout(fixture.VECTOR, 'Z', size=8)
                self.diagnostic(fixture.capture(), accepted=False)

    def test_unknown_boolean_semantics_are_never_coerced_to_truth(self):
        for owner, name in ((MemoryFixture.PAWN_CLASS, 'bIsInStationaryCover'),
                            (MemoryFixture.LINK_CLASS, 'bCircular'), (MemoryFixture.SLOT, 'bLeanLeft')):
            for mask in (0, 3):
                with self.subTest(name=name, mask=mask):
                    fixture = MemoryFixture()
                    fixture.put(fixture.properties[owner, name]['address'] + 0xd0, struct.pack('<I', mask))
                    self.diagnostic(fixture.capture(), accepted=False)
        fixture = MemoryFixture()
        fixture.set_value(fixture.PAWN, 'bIsInStationaryCover', 1, '<I')
        query = CoverQuery(fixture.observer)
        self.assertIs(boolean(query, query.obj(fixture.PAWN), 'bIsInStationaryCover'), False)

    def test_possession_and_world_links_must_agree(self):
        for obj, name, pointer in ((MemoryFixture.PC, 'Pawn', 0),
                                    (MemoryFixture.PAWN, 'Controller', MemoryFixture.OTHER),
                                    (MemoryFixture.PAWN, 'WorldInfo', 0),
                                    (MemoryFixture.PC, 'WorldInfo', MemoryFixture.OTHER),
                                    (MemoryFixture.LINK, 'WorldInfo', MemoryFixture.OTHER)):
            with self.subTest(obj=obj, name=name, pointer=pointer):
                fixture = MemoryFixture()
                fixture.set_value(obj, name, pointer, '<Q')
                self.diagnostic(fixture.capture(), accepted=False)
        for obj in (MemoryFixture.PAWN, MemoryFixture.LINK):
            fixture = MemoryFixture()
            fixture.objects[obj]['cls'] = 0x99999
            # Pawn class is checked before properties; link class after its pointer.
            self.diagnostic(fixture.capture(), accepted=False)

    def test_absent_link_can_be_observed_but_has_no_fabricated_slots_or_activation(self):
        fixture = MemoryFixture()
        fixture.set_value(fixture.PAWN, 'CurrentLink', 0, '<Q')
        for name in ('CurrentSlotIdx', 'LeftSlotIdx', 'RightSlotIdx'):
            fixture.set_value(fixture.PAWN, name, -1, '<i')
        result = fixture.capture()
        self.diagnostic(result, accepted=True)
        self.assertIsNone(result['link'])
        self.assertIsNone(result['slots'])
        self.assertNotIn('bCircular', result['values'])

    def test_invalid_selected_index_can_retain_values_only_as_rejected_diagnostics(self):
        for name in ('CurrentSlotIdx', 'LeftSlotIdx', 'RightSlotIdx'):
            for index in (-1, 3):
                with self.subTest(name=name, index=index):
                    fixture = MemoryFixture()
                    fixture.set_value(fixture.PAWN, name, index, '<i')
                    result = fixture.capture()
                    self.diagnostic(result, accepted=False)
                    self.assertEqual(result['values'][name], index)
                    self.assertEqual(result['link']['address'], fixture.LINK)
                    self.assertNotIn('rows', result)

    def test_nonfinite_geometry_and_out_of_range_interpolation_reject(self):
        for value in (float('nan'), float('inf'), -.01, 1.01):
            fixture = MemoryFixture()
            fixture.set_value(fixture.PAWN, 'CurrentSlotPct', value, '<f')
            self.diagnostic(fixture.capture(), accepted=False)
        for value in (float('nan'), float('inf'), -float('inf')):
            fixture = MemoryFixture()
            fixture.put(fixture.DATA + 48 + 20, struct.pack('<f', value))
            self.diagnostic(fixture.capture(), accepted=False)
        for fraction in (0., 1.):
            fixture = MemoryFixture()
            fixture.set_value(fixture.PAWN, 'CurrentSlotPct', fraction, '<f')
            self.diagnostic(fixture.capture(), accepted=True)

    def test_closing_boundary_rechecks_cover_values_not_just_object_identity(self):
        for changed in ('action', 'link', 'slot_boolean', 'location', 'rotation'):
            with self.subTest(changed=changed):
                fixture = MemoryFixture()
                def mutate(call):
                    if call != 2:
                        return
                    if changed == 'action':
                        fixture.set_value(fixture.PAWN, 'CoverAction', 99, '<B')
                    elif changed == 'link':
                        fixture.set_value(fixture.PAWN, 'CurrentLink', 0, '<Q')
                    elif changed == 'slot_boolean':
                        fixture.put(fixture.DATA + 4, struct.pack('<I', 0))
                    elif changed == 'location':
                        fixture.put(fixture.DATA + 48 + 20, struct.pack('<f', 999))
                    elif changed == 'rotation':
                        fixture.put(fixture.DATA + 96 + 32, struct.pack('<i', -99))
                fixture.at_verify = mutate
                self.diagnostic(fixture.capture(), accepted=False)

    def test_metadata_identity_registry_and_type_pointer_races_never_admit(self):
        for changed in ('identity', 'registry_append', 'mask', 'geometry_type', 'array_inner'):
            with self.subTest(changed=changed):
                fixture = MemoryFixture()
                def mutate(call):
                    if call != 2:
                        return
                    if changed == 'identity':
                        fixture.objects[fixture.PAWN]['internal_index'] += 1
                    elif changed == 'registry_append':
                        fixture.put(fixture.objects_header, struct.pack('<Qii', 0x1000, 101, 256))
                    elif changed == 'mask':
                        fixture.put(fixture.properties[fixture.SLOT, 'bLeanLeft']['address'] + 0xd0, struct.pack('<I', 8))
                    elif changed == 'geometry_type':
                        fixture.put(fixture.properties[fixture.SLOT, 'LocationOffset']['address'] + 0xb0,
                                    struct.pack('<Q', fixture.ROTATOR))
                    elif changed == 'array_inner':
                        fixture.put(fixture.properties[fixture.LINK_CLASS, 'Slots']['address'] + 0xb0, struct.pack('<Q', 0))
                fixture.at_verify = mutate
                self.diagnostic(fixture.capture(), accepted=False)

    def test_boolean_mask_cannot_change_semantics_between_selected_rows(self):
        fixture = MemoryFixture()
        mask_address = fixture.properties[fixture.SLOT, 'bLeanLeft']['address'] + 0xd0
        def mutate(address, size, count):
            if (address, size, count) == (mask_address, 4, 2):
                fixture.put(address, struct.pack('<I', 8))
        fixture.before_read = mutate
        self.diagnostic(fixture.capture(), accepted=False)

    def test_second_closing_barrier_catches_changes_during_value_verification(self):
        fixture = MemoryFixture()
        def mutate(call):
            if call == 3:
                fixture.objects[fixture.PAWN]['internal_index'] += 1
        fixture.at_verify = mutate
        self.diagnostic(fixture.capture(), accepted=False)

    def test_closing_reuses_only_a_fresh_containing_recorded_window(self):
        fixture = MemoryFixture()
        address = 0x90000
        fixture.put(address, bytes(range(16)))
        query = CoverQuery(fixture.observer)
        for start, size in ((4, 4), (0, 16), (0, 8), (1, 2), (12, 4)):
            query.read(address + start, size)
        before = dict(fixture.read_counts)
        query.finish()
        self.assertEqual(fixture.read_counts[address, 16] - before[address, 16], 1)
        for start, size in ((4, 4), (0, 8), (1, 2), (12, 4)):
            self.assertEqual(fixture.read_counts[address + start, size], before[address + start, size])
        # A later finish cannot reuse the previous closing buffer.
        fixture.put(address + 5, b'\xff')
        with self.assertRaises(RuntimeError):
            query.finish()

    def test_closing_checks_every_byte_of_the_container_and_nested_witnesses(self):
        for changed in range(16):
            with self.subTest(changed=changed):
                fixture = MemoryFixture()
                address = 0x90000
                fixture.put(address, bytes(range(16)))
                query = CoverQuery(fixture.observer)
                query.read(address, 16)
                query.read(address + 4, 4)
                fixture.put(address + changed, b'\xff')
                with self.assertRaises(RuntimeError):
                    query.finish()
        # Contained first witnesses are compared even if a larger window now
        # agrees with its own first witness. Incoherent overlap is not erased.
        fixture = MemoryFixture()
        fixture.put(0x90000, bytes(range(16)))
        query = CoverQuery(fixture.observer)
        query.read(0x90000, 16)
        query.remember(0x90004, b'xxxx')
        with self.assertRaises(RuntimeError):
            query.finish()

    def test_partial_overlaps_still_get_their_own_full_closing_read(self):
        fixture = MemoryFixture()
        fixture.put(0x90000, bytes(range(24)))
        query = CoverQuery(fixture.observer)
        query.read(0x90000, 16)
        query.read(0x90008, 16)
        query.read(0x90014, 4)
        before = dict(fixture.read_counts)
        query.finish()
        self.assertEqual(fixture.read_counts[0x90000, 16] - before[0x90000, 16], 1)
        self.assertEqual(fixture.read_counts[0x90008, 16] - before[0x90008, 16], 1)
        self.assertEqual(fixture.read_counts[0x90014, 4], before[0x90014, 4])

    def test_fresh_closing_container_read_fault_is_not_accepted(self):
        fixture = MemoryFixture()
        fixture.put(0x90000, bytes(range(16)))
        query = CoverQuery(fixture.observer)
        query.read(0x90000, 16)
        query.read(0x90004, 4)
        del fixture.memory[0x9000f]
        with self.assertRaises(OSError):
            query.finish()

    def test_reader_loss_and_partial_decode_failure_remain_rejected_diagnostics(self):
        for reader in ('resolver', 'sampler'):
            fixture = MemoryFixture()
            setattr(fixture.observer, reader, None)
            self.diagnostic(fixture.capture(), accepted=False)
        fixture = MemoryFixture()
        address = fixture.value_address(fixture.PAWN, 'CoverDirection')
        del fixture.memory[address]
        result = fixture.capture()
        self.diagnostic(result, accepted=False)
        self.assertEqual(result['rejection_type'], 'OSError')


if __name__ == '__main__':
    unittest.main()
