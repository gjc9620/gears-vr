"""Current-image checks against exact original DLL plus synthetic live memory.

No current game process, virtual method or engine query is used by these tests.
"""
import copy
from pathlib import Path
import struct
import unittest

from cover_state import CoverQuery
from cover_physx_source import Artifact, PhysXSource, CHUNKS, ACTOR_TARGETS, INTERNAL_SLOTS
from test_cover_physics_state import PhysicsMemory
from test_cover_physics_shapes import ShapeMemory

ORIGINAL = Path('J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/'
                'Gears of War Reloaded/Binaries_x64/PhysXCore.dll')


class LoadedFixture:
    BASE, VTABLE = 0x70000000, 0x2330a8

    def __init__(self, memory, artifact):
        self.m, self.artifact = memory, artifact
        self.path = Path(memory.observer.identities['game_identity']['image']).resolve().with_name('PhysXCore.dll')
        self.rows = [(self.BASE, self.path)]
        self.enumerations = 0
        self.at_enumerate = None
        memory.put(self.BASE, artifact.header)
        for rva, size, _ in CHUNKS:
            memory.put(self.BASE + rva, artifact.relocated(rva, size, self.BASE))
        for offset in set(ACTOR_TARGETS) | set(INTERNAL_SLOTS):
            rva = self.VTABLE + offset
            raw = artifact.relocated(rva, 8, self.BASE)
            memory.put(self.BASE + rva, raw)
            target = struct.unpack('<Q', raw)[0]
            memory.put(target, artifact.relocated(target - self.BASE, 32, self.BASE))
        for index, body in enumerate(memory.bodies):
            receiver, internal = 0x5000000 + index * 0x100, 0x5200000 + index * 0x100
            memory.put(receiver, struct.pack('<Q', self.BASE + self.VTABLE))
            memory.put(receiver + 0x40, struct.pack('<Q', internal))
            # Synthetic internal vtable intentionally uses readable original
            # slots; it is not a claim about a game's actual internal class.
            memory.put(internal, struct.pack('<Q', self.BASE + self.VTABLE))

    def enumerate(self, query):
        self.enumerations += 1
        if self.at_enumerate:
            self.at_enumerate(self.enumerations)
        return copy.deepcopy(self.rows)

    def factory(self, query, game_image):
        return PhysXSource(query, game_image, enumerate_modules=self.enumerate,
                           artifact_factory=lambda path: self.artifact)

    def read(self, *, finish=True):
        query = CoverQuery(self.m.observer)
        source = self.factory(query, self.m.observer.identities['game_identity']['image'])
        bodies = [source.receiver(0x5000000 + i * 0x100) for i in range(len(self.m.bodies))]
        source.reverify()
        if finish:
            query.finish()
        return source, bodies, query


class PhysXSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.artifact = Artifact(ORIGINAL)

    def fixture(self, count=3):
        return LoadedFixture(PhysicsMemory(count), self.artifact)

    def test_original_artifact_complete_chunks_and_relocation_are_certified(self):
        self.assertEqual(len(self.artifact.raw), 2711552)
        self.assertEqual(len(self.artifact.header), 1024)
        self.assertGreater(len(self.artifact.relocations), 0)
        self.artifact.reverify()
        for offset, target in ACTOR_TARGETS.items():
            raw = self.artifact.relocated(0x2330a8 + offset, 8, LoadedFixture.BASE)
            self.assertEqual(struct.unpack('<Q', raw)[0], LoadedFixture.BASE + target)

    def test_current_module_actor_and_internal_slots_close_without_calls_or_pose_permission(self):
        f = self.fixture()
        before = copy.deepcopy(f.m.memory)
        source, bodies, q = f.read()
        self.assertEqual(f.enumerations, 2)
        self.assertEqual(len(bodies), 3)
        self.assertEqual(len(source.result()['current_analyzed_code']), 8)
        self.assertEqual(set(bodies[0]['internal_slots']), {str(x) for x in INTERNAL_SLOTS})
        for row in bodies:
            self.assertFalse(row['methods_called'])
            self.assertFalse(row['physical_pose_verified'])
            self.assertFalse(row['active_physics_verified'])
        self.assertEqual(f.m.memory, before)
        self.assertEqual(f.m.bridges, [])
        self.assertLess(q.reads, 16384)

    def test_absent_internal_body_is_retained_and_never_called_a_dynamic_pose(self):
        f = self.fixture()
        f.m.put(0x5000000 + 0x40, bytes(8))
        _, bodies, _ = f.read()
        self.assertFalse(bodies[0]['internal_present'])
        self.assertNotIn('internal_slots', bodies[0])

    def test_missing_foreign_duplicate_and_incomplete_module_lists_reject(self):
        for defect in ('missing', 'foreign', 'duplicate_name', 'duplicate_base', 'over_capacity', 'unaligned'):
            with self.subTest(defect=defect):
                f = self.fixture()
                if defect == 'missing':
                    f.rows = [(0x10000, Path('other.dll'))]
                elif defect == 'foreign':
                    f.rows = [(f.BASE, Path('foreign/PhysXCore.dll'))]
                elif defect == 'duplicate_name':
                    f.rows.append((f.BASE + 0x1000000, Path('other/PhysXCore.dll')))
                elif defect == 'duplicate_base':
                    f.rows.append((f.BASE, Path('other.dll')))
                elif defect == 'over_capacity':
                    f.rows *= 2049
                else:
                    f.rows = [(f.BASE + 1, f.path)]
                with self.assertRaises(RuntimeError):
                    f.read()

    def test_exact_headers_full_code_tails_and_rebased_vtable_are_not_prefix_only(self):
        for defect in ('header', 'code_tail', 'slot', 'internal_target'):
            with self.subTest(defect=defect):
                f = self.fixture()
                if defect == 'header':
                    at = f.BASE + 0x120
                elif defect == 'code_tail':
                    at = f.BASE + 0x13c270 + 1204
                elif defect == 'slot':
                    at = f.BASE + f.VTABLE + 40
                else:
                    at = f.BASE + 0x135b00 + 17
                f.m.put(at, bytes([f.m.read(at, 1)[0] ^ 1]))
                with self.assertRaises(RuntimeError):
                    f.read()

    def test_internal_pointer_or_vtable_outside_image_is_not_guessed(self):
        for defect in ('pointer', 'table', 'unreadable'):
            f = self.fixture()
            if defect == 'pointer':
                f.m.put(0x5000000 + 0x40, struct.pack('<Q', 0x5200001))
            elif defect == 'table':
                f.m.put(0x5200000, struct.pack('<Q', f.BASE + 0x1000))
            else:
                f.m.put(0x5000000 + 0x40, struct.pack('<Q', 0x5300000))
            with self.assertRaises((RuntimeError, OSError)):
                f.read()

    def test_module_replacement_and_closing_body_or_code_changes_reject(self):
        f = self.fixture()
        def unload(n):
            if n == 2:
                f.rows = [(f.BASE + 0x10000, f.path)]
        f.at_enumerate = unload
        with self.assertRaises(RuntimeError):
            f.read()
        for defect in ('pointer', 'table', 'code'):
            f = self.fixture()
            def change(n):
                if n != 1:
                    return
                if defect == 'pointer':
                    f.m.put(0x5000000 + 0x40, bytes(8))
                elif defect == 'table':
                    f.m.put(0x5200000, bytes(8))
                else:
                    at = f.BASE + 0x13c270 + 1000
                    f.m.put(at, bytes([f.m.read(at, 1)[0] ^ 1]))
            f.m.at_verify = change
            with self.assertRaises(RuntimeError):
                f.read()

    def test_existing_budget_still_rejects_instead_of_truncating_observation(self):
        f = self.fixture()
        q = CoverQuery(f.m.observer)
        q.reads = 16384
        with self.assertRaisesRegex(RuntimeError, 'budget'):
            f.factory(q, f.m.observer.identities['game_identity']['image'])

    def test_combined_actual_source_path_binds_internal_receivers_before_closing(self):
        f = self.fixture()
        out = f.m.capture_physics(include_physx=True, physx_factory=f.factory)
        self.assertTrue(out['accepted'], out.get('rejection'))
        self.assertTrue(out['physx_source_observed'])
        self.assertTrue(out['virtual_target_module_verified'])
        self.assertTrue(out['virtual_target_executable_verified'])
        self.assertFalse(out['physical_pose_verified'])
        self.assertFalse(out['collision_verified'])
        self.assertFalse(out['world_synchronization_verified'])
        self.assertEqual(len(out['bodies']), 3)
        self.assertEqual(out['queue_submissions'], 0)
        self.assertTrue(all(p.closed for p in f.m.proofs))

    def test_combined_authored_shapes_and_internal_source_share_one_closed_query(self):
        m = ShapeMemory()
        f = LoadedFixture(m, self.artifact)
        out = m.capture_physics(include_shapes=True, include_physx=True, physx_factory=f.factory)
        self.assertTrue(out['accepted'], out.get('rejection'))
        self.assertTrue(out['authored_shapes_observed'])
        self.assertEqual(len(out['bodies'][0]['authored_shapes']['shapes']), 4)
        self.assertIn('internal_slots', out['bodies'][0]['main']['physx'])
        self.assertFalse(out['physical_pose_verified'])
        self.assertLess(out['reads'], 16384)
        self.assertLess(out['bytes'], 2 * 1024 * 1024)

    def test_full_body_cap_with_all_four_authored_kinds_fits_original_budget(self):
        m = ShapeMemory(64)
        f = LoadedFixture(m, self.artifact)
        out = m.capture_physics(include_shapes=True, include_physx=True, physx_factory=f.factory)
        self.assertTrue(out['accepted'], out.get('rejection'))
        self.assertEqual(len(out['bodies']), 64)
        self.assertEqual(sum(len(row['authored_shapes']['shapes']) for row in out['bodies']), 256)
        self.assertLess(out['reads'], 16384)
        self.assertLess(out['bytes'], 2 * 1024 * 1024)

    def test_combined_shape_payload_change_at_closing_rejects_the_whole_source(self):
        m = ShapeMemory()
        f = LoadedFixture(m, self.artifact)
        def change(n):
            if n == 2:
                m.put(m.arrays_shape[m.AGG, 'SphereElems']['data'] + 64, struct.pack('<f', 9))
        f.at_enumerate = change
        out = m.capture_physics(include_shapes=True, include_physx=True, physx_factory=f.factory)
        self.assertFalse(out['accepted'])
        self.assertFalse(out['authored_shapes_observed'])
        self.assertFalse(out['physx_source_observed'])
        self.assertIn('changed', out['rejection'])
        self.assertTrue(all(p.closed for p in m.proofs))


if __name__ == '__main__':
    unittest.main()
