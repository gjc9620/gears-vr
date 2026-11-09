"""Real reflected reader against synthetic memory; never opens a game."""
import copy
import struct
import unittest

from cover_hit_source import read_setup_filters, FilterReader, historical_chunks
from cover_state import CoverQuery
from test_cover_physics_shapes import ShapeMemory


class HitSourceTests(unittest.TestCase):
    def fixture(self, bits=12, count=1):
        memory = ShapeMemory(count)
        for name, mask in [('bBlockZeroExtent', 4), ('bBlockNonZeroExtent', 8)]:
            memory.add_property(memory.BS_CLASS, name, 'BoolProperty', 0xbc, 4, mask=mask)
        for setup in memory.setups:
            memory.put(setup + 0xbc, struct.pack('<I', bits))
        memory.relink()
        return memory

    def seed_code(self, memory):
        for rva, raw, _ in historical_chunks():
            memory.put(memory.BASE + rva, raw)

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

    def test_reader_cache_is_bound_to_one_query_and_keeps_each_setup_word(self):
        m = self.fixture(count=3)
        q = CoverQuery(m.observer)
        reader = FilterReader(q)
        for index, address in enumerate(m.setups):
            m.put(address + 0xbc, struct.pack('<I', 4 if index == 1 else 8))
            out = read_setup_filters(q, q.obj(address), reader=reader)
            self.assertEqual(out['bBlockZeroExtent'], index == 1)
        self.assertEqual(len(reader.layouts), 1)
        q.finish()
        other = CoverQuery(m.observer)
        with self.assertRaisesRegex(RuntimeError, 'another query'):
            read_setup_filters(other, other.obj(m.setups[0]), reader=reader)

    def test_current_source_checks_original_filter_code_and_all_body_words(self):
        m = self.fixture(count=3)
        self.seed_code(m)
        before = copy.deepcopy(m.memory)
        out = m.capture_physics(include_hit_filters=True)
        self.assertTrue(out['accepted'], out.get('rejection'))
        self.assertTrue(out['hit_filters_observed'])
        self.assertEqual(len(out['code_chunks']), 6)
        self.assertTrue(all(row['hit_filters']['bBlockZeroExtent'] for row in out['bodies']))
        self.assertFalse(out['hit_geometry_verified'])
        self.assertEqual(m.memory, before)
        self.assertEqual(m.bridges, [])
        self.assertTrue(all(proof.closed for proof in m.proofs))

    def test_changed_current_filter_code_rejects_before_interpreting_body_flags(self):
        m = self.fixture()
        self.seed_code(m)
        rva, raw, _ = historical_chunks()[0]
        m.put(m.BASE + rva + len(raw) - 1, bytes([raw[-1] ^ 1]))
        out = m.capture_physics(include_hit_filters=True)
        self.assertFalse(out['accepted'])
        self.assertFalse(out['hit_filters_observed'])
        self.assertEqual(out['bodies'], [])
        self.assertIn('full code', out['rejection'])
        self.assertTrue(all(proof.closed for proof in m.proofs))

    def test_combined_close_rejects_filter_code_or_body_word_changes(self):
        for defect in ('code', 'word'):
            with self.subTest(defect=defect):
                m = self.fixture()
                self.seed_code(m)
                rva, raw, _ = historical_chunks()[0]
                target, size = (m.BASE + rva, len(raw)) if defect == 'code' else (m.setups[0] + 0xbc, 4)
                def change(address, length, count):
                    if (address, length, count) == (target, size, 2):
                        m.put(target, bytes(size))
                m.before_read = change
                out = m.capture_physics(include_hit_filters=True)
                self.assertFalse(out['accepted'])
                self.assertFalse(out['hit_filters_observed'])
                self.assertFalse(out['hit_geometry_verified'])
                self.assertTrue(all(proof.closed for proof in m.proofs))

    def test_full64_body_combined_observation_still_fits_existing_query_budget(self):
        from cover_physx_source import Artifact
        from test_cover_physx_source import LoadedFixture, ORIGINAL
        m = self.fixture(count=64)
        self.seed_code(m)
        f = LoadedFixture(m, Artifact(ORIGINAL))
        out = m.capture_physics(include_shapes=True, include_physx=True, include_hit_filters=True,
                                physx_factory=f.factory)
        self.assertTrue(out['accepted'], out.get('rejection'))
        self.assertEqual(len(out['bodies']), 64)
        self.assertLess(out['reads'], 16384)
        self.assertLess(out['bytes'], 2 * 1024 * 1024)
        self.assertTrue(out['hit_filters_observed'])
        self.assertFalse(out['activation_allowed'])


if __name__ == '__main__':
    unittest.main()
