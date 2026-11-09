"""Real reflected reader against synthetic memory; never opens a game."""
import copy
import struct
import unittest

from cover_hit_source import read_setup_filters
from cover_state import CoverQuery
from test_cover_physics_shapes import ShapeMemory


class HitSourceTests(unittest.TestCase):
    def fixture(self, bits=12):
        memory = ShapeMemory()
        for name, mask in [('bBlockZeroExtent', 4), ('bBlockNonZeroExtent', 8)]:
            memory.add_property(memory.BS_CLASS, name, 'BoolProperty', 0xbc, 4, mask=mask)
        memory.put(memory.setups[0] + 0xbc, struct.pack('<I', bits))
        memory.relink()
        return memory

    def read(self, memory, *, close=True):
        q = CoverQuery(memory.observer)
        setup = q.obj(memory.setups[0])
        result = read_setup_filters(q, setup)
        if close:
            q.finish()
        return result, q

    def test_both_independent_filters_and_disabled_values_are_observed_without_permission(self):
        for bits in (0, 4, 8, 12, 0xfffffff3):
            with self.subTest(bits=bits):
                m = self.fixture(bits)
                before = copy.deepcopy(m.memory)
                result, _ = self.read(m)
                self.assertEqual(result['bBlockZeroExtent'], bool(bits & 4))
                self.assertEqual(result['bBlockNonZeroExtent'], bool(bits & 8))
                self.assertEqual(result['word_bytes'], struct.pack('<I', bits).hex())
                self.assertEqual(m.memory, before)
                self.assertEqual(m.bridges, [])
                for name in ('narrow_phase_selected', 'hit_geometry_verified', 'active_physics_verified',
                             'world_synchronization_verified', 'activation_allowed', 'engine_calls',
                             'selected_route_code_currently_verified'):
                    self.assertFalse(result[name])

    def test_wrong_offset_width_or_mask_is_not_interpreted_as_original_filter(self):
        for defect in ('offset', 'width', 'swapped_mask', 'compound_mask', 'zero_mask'):
            with self.subTest(defect=defect):
                m = self.fixture()
                p = m.properties[m.BS_CLASS, 'bBlockZeroExtent']
                if defect == 'offset':
                    p['offset'] = 0xc0
                    m.put(p['address'] + 0x94, struct.pack('<I', 0xc0))
                elif defect == 'width':
                    p['size'] = 8
                    m.put(p['address'] + 0x74, struct.pack('<I', 8))
                else:
                    m.put(p['address'] + 0xd0, struct.pack('<I', dict(swapped_mask=8, compound_mask=12, zero_mask=0)[defect]))
                with self.assertRaises(RuntimeError):
                    self.read(m)

    def test_missing_filter_is_not_inferred_from_other_shape_flags(self):
        m = self.fixture()
        del m.properties[m.BS_CLASS, 'bBlockNonZeroExtent']
        m.relink()
        with self.assertRaises(RuntimeError):
            self.read(m)

    def test_actual_declaring_ancestor_is_used_instead_of_guessed_property_owner(self):
        m = self.fixture()
        for name in ('bBlockZeroExtent', 'bBlockNonZeroExtent'):
            field = m.properties.pop((m.BS_CLASS, name))
            m.properties[m.KM, name] = field
            m.objects[field['address']]['outer'] = m.KM
        m.relink()
        result, _ = self.read(m)
        self.assertEqual(result['setup_base_class']['address'], m.BS_CLASS)
        self.assertTrue(all(x['address'] == m.KM for x in result['declaring_classes'].values()))

    def test_changed_filter_word_or_metadata_cannot_close_the_observation(self):
        for defect in ('word', 'mask'):
            with self.subTest(defect=defect):
                m = self.fixture()
                result, q = self.read(m, close=False)
                self.assertTrue(result['closing_required'])
                at = (m.setups[0] + 0xbc if defect == 'word' else
                      m.properties[m.BS_CLASS, 'bBlockZeroExtent']['address'] + 0xd0)
                m.put(at, struct.pack('<I', 0))
                with self.assertRaises(RuntimeError):
                    q.finish()

    def test_default_or_replaced_setup_identity_is_rejected(self):
        for defect in ('default', 'replaced'):
            m = self.fixture()
            if defect == 'default':
                m.objects[m.setups[0]]['name'] = 'Default__RB_BodySetup'
            q = CoverQuery(m.observer)
            setup = copy.deepcopy(q.obj(m.setups[0]))
            if defect == 'replaced':
                setup['outer'] += 8
            with self.assertRaises(RuntimeError):
                read_setup_filters(q, setup)


if __name__ == '__main__':
    unittest.main()
