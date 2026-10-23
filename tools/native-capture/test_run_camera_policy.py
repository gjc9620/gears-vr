import copy
import hashlib
import json
import struct
import unittest
from unittest.mock import patch

import run_camera_policy as run


class Memory:
    """Synthetic reflected object graph; contains no game code or resources."""
    def __init__(self):
        self.pid, self.base = 123, 0x140000000
        self.profile = dict(sha256=run.SUPPORTED_SHA256, field_super=0x60,
                            property_offset_internal=0x94, struct_property_struct=0xd0)
        self.bytes, self.identities, self.properties, self.classes = {}, {}, {}, {}
        self.next_address = 0x100000

    def put(self, address, raw):
        self.bytes.update((address + i, byte) for i, byte in enumerate(raw))

    def read(self, address, size):
        try:
            return bytes(self.bytes[address + i] for i in range(size))
        except KeyError as error:
            raise OSError('synthetic unmapped read') from error

    def u64(self, address):
        return struct.unpack('<Q', self.read(address, 8))[0]

    def u32(self, address):
        return struct.unpack('<I', self.read(address, 4))[0]

    def pointer(self, address, value):
        self.put(address, struct.pack('<Q', value))

    def integer(self, address, value):
        self.put(address, struct.pack('<I', value))

    def floating(self, address, value):
        self.put(address, struct.pack('<f', value))

    def obj(self, name, cls=0, outer=0, number=0):
        address = self.next_address
        self.next_address += 0x1000
        self.put(address, bytes(0x1000))
        self.identities[address] = dict(address=address, internal_index=len(self.identities),
                                        name_index=len(self.identities), name=name, number=number,
                                        cls=cls, outer=outer)
        return address

    def add_class(self, name, parent=None):
        address = self.obj(name, self.classes.get('Class', 0))
        self.classes[name] = address
        self.pointer(address + 0x60, self.classes.get(parent, 0))
        return address

    def field(self, owner, name, kind, size, offset):
        owner_address = self.classes[owner]
        address = self.obj(name, self.classes[kind], owner_address)
        self.put(address + 0x70, struct.pack('<II', 1, size))
        self.integer(address + 0x94, offset)
        prop = dict(address=address, offset=offset, type=kind)
        self.properties[owner_address, name] = prop
        return prop

    def property(self, owner, name, inherited=False):
        seen = set()
        while owner and owner not in seen:
            seen.add(owner)
            if (owner, name) in self.properties:
                return dict(self.properties[owner, name])
            if not inherited:
                break
            owner = self.u64(owner + 0x60)
        raise RuntimeError('Missing synthetic field: ' + name)

    def struct_type(self, prop):
        return self.u64(prop['address'] + 0xd0)


class Sampler:
    def __init__(self, fixture, pawn=None, mesh=None):
        self.r = fixture.r
        self.camera, self.controller, self.local = fixture.camera, fixture.pc, fixture.local
        self.pawn = fixture.pawn if pawn is None else pawn
        self.mesh = fixture.mesh if mesh is None else mesh
        self.stamp = self.camera + 0x680
        self.view_target = self.camera + 0x620
        self.unreadable = set()

    def fresh(self, address):
        if address in self.unreadable or address not in self.r.identities:
            raise RuntimeError('Synthetic object slot is not readable')
        if hasattr(self.r, 'objects_header'):
            data, count, _ = struct.unpack('<Qii', self.r.read(self.r.objects_header, 16))
            index = self.r.identities[address]['internal_index']
            if not index < count or self.r.u64(data + index * 8) != address:
                raise RuntimeError('Synthetic object registry slot differs')
        return copy.deepcopy(self.r.identities[address])

    def guard(self):
        return dict(pawn=self.fresh(self.pawn), controller=self.fresh(self.controller),
                    camera=self.fresh(self.camera), local_player=self.fresh(self.local),
                    level=dict(address=0x9000), view_target=self.r.u64(self.view_target),
                    timestamp_raw=self.r.read(self.stamp, 4).hex())


class FakeWriter:
    def __init__(self, fixture):
        self.fixture = fixture
        self.calls, self.failures = [], {}
        self.after_write = None
        self.closed = False

    def write(self, address, raw):
        if address not in (self.fixture.ambient_address, self.fixture.roadie_address) or len(raw) != 4:
            raise AssertionError('Policy attempted an unauthorized write')
        self.calls.append((address, raw))
        failure = self.failures.pop(len(self.calls), None)
        if failure is not None:
            count, known = failure
            self.fixture.r.put(address, raw[:count])
            if known:
                raise run.WriteFailure('synthetic partial write', count)
            raise OSError('synthetic write failure with unknown prefix')
        self.fixture.r.put(address, raw)
        if self.after_write:
            self.after_write(address, raw)

    def close(self):
        self.closed = True


class Fixture:
    def __init__(self, ambient=False, factor=.6):
        self.r = r = Memory()
        for name, parent in (
                ('Object', None), ('Class', 'Object'), ('Actor', 'Object'), ('Pawn', 'Actor'),
                ('WarPawn', 'Pawn'), ('Controller', 'Actor'), ('PlayerController', 'Controller'),
                ('WarPC', 'PlayerController'), ('LocalPlayer', 'Object'), ('WarLocalPlayer', 'LocalPlayer'),
                ('Camera', 'Actor'), ('WarPlayerCamera', 'Camera'), ('CameraModifier', 'Object'),
                ('WarCameraModifier', 'CameraModifier'), ('WarCamMod_CameraBone', 'WarCameraModifier'),
                ('SkeletalMeshComponent', 'Object'), ('ObjectProperty', 'Object'), ('ComponentProperty', 'Object'),
                ('ArrayProperty', 'Object'), ('BoolProperty', 'Object'), ('FloatProperty', 'Object'),
                ('ByteProperty', 'Object'), ('StructProperty', 'Object'), ('ScriptStruct', 'Object')):
            r.add_class(name, parent)
        for address in r.classes.values():
            r.identities[address]['cls'] = r.classes['Class']
        self.camera = r.obj('WarPlayerCamera', r.classes['WarPlayerCamera'], number=1)
        self.pc = r.obj('WarPC', r.classes['WarPC'])
        self.local = r.obj('WarLocalPlayer', r.classes['WarLocalPlayer'])
        self.pawn = r.obj('Marcus', r.classes['WarPawn'])
        self.mesh = r.obj('WarPawnMesh', r.classes['SkeletalMeshComponent'], self.pawn)
        self.modifier = r.obj('WarCamMod_CameraBone', r.classes['WarCamMod_CameraBone'], self.camera)
        for owner, name, kind, size, offset in (
                ('PlayerController', 'PlayerCamera', 'ObjectProperty', 8, 0x350),
                ('Camera', 'PCOwner', 'ObjectProperty', 8, 0x224),
                ('PlayerController', 'Player', 'ObjectProperty', 8, 0x368),
                ('Controller', 'Pawn', 'ObjectProperty', 8, 0x224),
                ('Pawn', 'Controller', 'ObjectProperty', 8, 0x230),
                ('Pawn', 'Mesh', 'ComponentProperty', 8, 0x3a0),
                ('WarPlayerCamera', 'WarCamMod_CameraBone', 'ObjectProperty', 8, 0x548),
                ('CameraModifier', 'CameraOwner', 'ObjectProperty', 8, 0x60),
                ('WarCamMod_CameraBone', 'Pawn', 'ObjectProperty', 8, 0x78),
                ('WarCamMod_CameraBone', 'Mesh', 'ComponentProperty', 8, 0x80),
                ('Camera', 'ModifierList', 'ArrayProperty', 16, 0x444),
                ('WarCamMod_CameraBone', run.AMBIENT, 'BoolProperty', 4, 0x100),
                ('WarCamMod_CameraBone', run.ROADIE, 'FloatProperty', 4, 0x108),
                ('WarCamMod_CameraBone', 'CameraAnimOption', 'ByteProperty', 1, 0x110),
                ('WarCameraModifier', 'Alpha', 'FloatProperty', 4, 0x114),
                ('WarPawn', 'CameraBoneMotionScale', 'FloatProperty', 4, 0x500),
                ('WarPawn', 'SpecialMove', 'ByteProperty', 1, 0x510),
                ('Actor', 'Velocity', 'StructProperty', 12, 0x150)):
            r.field(owner, name, kind, size, offset)
        for address, target in ((self.pc + 0x350, self.camera), (self.camera + 0x224, self.pc),
                                (self.pc + 0x368, self.local), (self.pc + 0x224, self.pawn),
                                (self.pawn + 0x230, self.pc), (self.pawn + 0x3a0, self.mesh),
                                (self.camera + 0x548, self.modifier), (self.modifier + 0x60, self.camera),
                                (self.modifier + 0x78, self.pawn), (self.modifier + 0x80, self.mesh),
                                (self.camera + 0x620, self.pawn)):
            r.pointer(address, target)
        self.list_data = r.obj('synthetic array storage')
        r.put(self.camera + 0x444, struct.pack('<Qii', self.list_data, 2, 2))
        r.put(self.list_data, struct.pack('<2Q', self.modifier, 0))
        array = r.property(r.classes['Camera'], 'ModifierList')
        inner = r.obj('ModifierList', r.classes['ObjectProperty'], array['address'])
        r.put(inner + 0x70, struct.pack('<II', 1, 8))
        r.pointer(array['address'] + 0xd0, inner)
        self.mask = 4
        self.ambient_property = r.property(r.classes['WarCamMod_CameraBone'], run.AMBIENT)
        self.roadie_property = r.property(r.classes['WarCamMod_CameraBone'], run.ROADIE)
        self.ambient_address, self.roadie_address = self.modifier + 0x100, self.modifier + 0x108
        r.integer(self.ambient_property['address'] + 0xd0, self.mask)
        self.vtable = 0x800000
        r.pointer(self.ambient_property['address'], self.vtable)
        r.pointer(self.vtable + 0x2a0, r.base + 0x1daba0)
        r.pointer(r.base + 0x1b86280 + 0x2d * 8, r.base + 0x1d6ea4)
        self.code = ((0x1d6ea4, b'synthetic bool dispatch'), (0x1daba0, b'synthetic bool accessor'))
        for rva, raw in self.code:
            r.put(r.base + rva, raw)
        self.code_cert = tuple((rva, len(raw), hashlib.sha256(raw).hexdigest()) for rva, raw in self.code)
        r.integer(self.ambient_address, 0xa0 | (self.mask if ambient else 0))
        r.floating(self.roadie_address, factor)
        r.put(self.modifier + 0x110, b'\x02')
        r.floating(self.modifier + 0x114, .4)
        r.floating(self.pawn + 0x500, .4)
        r.put(self.pawn + 0x510, b'\x04')
        r.floating(self.camera + 0x680, 12.0)
        r.put(self.pawn + 0x150, struct.pack('<3f', 100, 0, 0))
        vector = r.obj('Vector', r.classes['ScriptStruct'])
        velocity = r.property(r.classes['Actor'], 'Velocity')
        r.pointer(velocity['address'] + 0xd0, vector)
        for name, offset in zip(('X', 'Y', 'Z'), (0, 4, 8)):
            r.properties[vector, name] = dict(address=0, type='FloatProperty', offset=offset)
        self.identity = dict(pid=r.pid, creation_time=987, image='synthetic.exe')
        self.sampler = Sampler(self)
        self.writer = FakeWriter(self)

    def checker(self, pid):
        return copy.deepcopy(self.identity) if self.identity and pid == self.identity['pid'] else None

    def policy(self):
        return run.Policy(self.r.pid, self.checker, writer=self.writer)

    def new_world(self):
        pawn = self.r.obj('NewMarcus', self.r.classes['WarPawn'])
        mesh = self.r.obj('NewMesh', self.r.classes['SkeletalMeshComponent'], pawn)
        for address, value in ((self.pc + 0x224, pawn), (pawn + 0x230, self.pc), (pawn + 0x3a0, mesh),
                                (self.modifier + 0x78, pawn), (self.modifier + 0x80, mesh),
                                (self.camera + 0x620, pawn)):
            self.r.pointer(address, value)
        return Sampler(self, pawn, mesh)


class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.f = Fixture()
        self.code_patch = patch.object(run, 'BOOL_CODE', self.f.code_cert)
        self.code_patch.start()
        self.addCleanup(self.code_patch.stop)

    def prepared(self):
        policy = self.f.policy()
        plan = policy.prepare(self.f.sampler)
        return policy, plan

    def test_readonly_observation_and_detached_metadata(self):
        before = dict(self.f.r.bytes)
        with patch.object(run, 'Writer', side_effect=AssertionError('writer opened')):
            result = run.observe(self.f.sampler, self.f.checker)
            policy = run.Policy(self.f.r.pid, self.f.checker)
            plan = policy.prepare(self.f.sampler)
        self.assertTrue(result['accepted'])
        self.assertEqual(result['diagnostics']['CameraAnimOption']['value'], 2)
        self.assertEqual(result['diagnostics']['SpecialMove']['value'], 4)
        self.assertEqual(result['diagnostics']['Velocity']['value'], [100., 0., 0.])
        self.assertFalse(result['runtime_acceptance'])
        result['modifier']['name'] = 'tampered'
        exported = plan.export()
        exported['fields'][run.ROADIE]['raw'] = 'bad'
        self.assertNotEqual(plan.export()['fields'][run.ROADIE]['raw'], 'bad')
        self.assertEqual(before, self.f.r.bytes)
        json.dumps(result)

    def test_only_two_fields_are_written_and_originals_restored(self):
        policy, plan = self.prepared()
        before = dict(self.f.r.bytes)
        policy.apply(plan)
        self.assertTrue(policy.check())
        self.assertEqual(set(a for a, _ in self.f.writer.calls), {self.f.ambient_address, self.f.roadie_address})
        changed = {a for a, value in before.items() if self.f.r.bytes[a] != value}
        allowed = {base + i for base in (self.f.ambient_address, self.f.roadie_address) for i in range(4)}
        self.assertLessEqual(changed, allowed)
        self.assertEqual(self.f.r.read(self.f.modifier + 0x110, 1), b'\x02')
        self.assertEqual(self.f.r.read(self.f.modifier + 0x114, 4), struct.pack('<f', .4))
        self.assertTrue(policy.restore()['released'])
        self.assertEqual(before, self.f.r.bytes)
        self.assertEqual(policy.report['restored'], 2)

    def test_neighbor_bits_can_change_before_apply_and_restore(self):
        policy, _ = self.prepared()
        self.f.r.integer(self.f.ambient_address, 0x1a0)
        policy.apply()
        self.assertEqual(self.f.r.u32(self.f.ambient_address), 0x1a4)
        self.f.r.integer(self.f.ambient_address, 0x3a4)
        self.assertTrue(policy.check())
        policy.restore()
        self.assertEqual(self.f.r.u32(self.f.ambient_address), 0x3a0)

    def test_noop_baselines_never_open_writer_or_claim_ownership(self):
        self.f.r.integer(self.f.ambient_address, 0xa4)
        self.f.r.floating(self.f.roadie_address, -0.)
        policy = run.Policy(self.f.r.pid, self.f.checker)
        plan = policy.prepare(self.f.sampler)
        self.assertEqual(plan.export()['planned_writes'], [])
        with patch.object(run, 'Writer', side_effect=AssertionError('writer opened')):
            self.assertEqual(policy.apply()['owned_fields'], [])
            self.assertTrue(policy.check())
            policy.close()
        self.assertEqual(policy.report['restored'], 0)

    def test_external_float_change_is_preserved_and_same_context_reapply_refused(self):
        policy, _ = self.prepared()
        policy.apply()
        self.f.r.floating(self.f.roadie_address, .25)
        self.assertFalse(policy.check())
        policy.restore()
        self.assertEqual(self.f.r.read(self.f.roadie_address, 4), struct.pack('<f', .25))
        self.assertEqual(policy.report['external_changes'], 1)
        calls = len(self.f.writer.calls)
        with self.assertRaises(run.PolicyRejected):
            policy.rebind(self.f.sampler)
        self.assertEqual(len(self.f.writer.calls), calls)

    def test_external_owned_bit_undo_is_not_written_back(self):
        policy, _ = self.prepared()
        policy.apply()
        self.f.r.integer(self.f.ambient_address, 0x2a0)
        calls = len(self.f.writer.calls)
        policy.restore()
        self.assertEqual(self.f.r.u32(self.f.ambient_address), 0x2a0)
        self.assertEqual(len(self.f.writer.calls), calls + 1)  # float only
        self.assertEqual(policy.report['external_changes'], 1)

    def test_recycled_target_is_retired_without_writes(self):
        policy, _ = self.prepared()
        policy.apply()
        self.f.r.identities[self.f.modifier]['number'] += 1
        calls = len(self.f.writer.calls)
        self.assertTrue(policy.restore()['released'])
        self.assertEqual(len(self.f.writer.calls), calls)
        self.assertEqual(policy.report['retired_fields'], 2)

    def test_confirmed_absent_live_registry_slot_retires_without_writes(self):
        r = self.f.r
        r.objects_header = 0x900000
        data, count = 0x910000, len(r.identities)
        r.put(r.objects_header, struct.pack('<Qii', data, count, count))
        for address, identity in r.identities.items():
            r.pointer(data + identity['internal_index'] * 8, address)
        policy, _ = self.prepared()
        policy.apply()
        r.pointer(data + r.identities[self.f.modifier]['internal_index'] * 8, 0)
        calls = len(self.f.writer.calls)
        self.assertTrue(policy.restore()['released'])
        self.assertEqual(len(self.f.writer.calls), calls)
        self.assertEqual(policy.report['retired_fields'], 2)

    def test_live_invalid_ownership_blocks_restore_and_rebind(self):
        policy, _ = self.prepared()
        policy.apply()
        self.f.r.pointer(self.f.modifier + 0x60, self.f.pc)
        calls = len(self.f.writer.calls)
        self.assertFalse(policy.restore()['released'])
        with self.assertRaises(run.PolicyRejected):
            policy.rebind(self.f.sampler)
        self.assertEqual(len(self.f.writer.calls), calls)
        self.f.r.pointer(self.f.modifier + 0x60, self.f.camera)
        self.assertTrue(policy.restore()['released'])

    def test_removed_membership_blocks_writes_and_restore(self):
        policy, _ = self.prepared()
        policy.apply()
        self.f.r.pointer(self.f.list_data, 0)
        calls = len(self.f.writer.calls)
        self.assertFalse(policy.restore()['released'])
        self.assertEqual(len(self.f.writer.calls), calls)

    def test_unknown_unreadable_object_is_not_assumed_retired(self):
        policy, _ = self.prepared()
        policy.apply()
        self.f.sampler.unreadable.add(self.f.modifier)
        self.assertFalse(policy.restore()['released'])
        self.assertEqual(policy.report['retired_fields'], 0)

    def test_partial_failure_before_second_write_rolls_back_first(self):
        policy, _ = self.prepared()
        original = (self.f.r.read(self.f.ambient_address, 4), self.f.r.read(self.f.roadie_address, 4))
        self.f.writer.failures[2] = (0, True)
        with self.assertRaises(run.WriteFailure):
            policy.apply()
        self.assertEqual(original, (self.f.r.read(self.f.ambient_address, 4), self.f.r.read(self.f.roadie_address, 4)))
        self.assertEqual(policy.report['pending_fields'], [])
        self.assertEqual(policy.report['applications'], 0)

    def test_known_partial_float_write_is_conditionally_rolled_back(self):
        policy, _ = self.prepared()
        before = dict(self.f.r.bytes)
        self.f.writer.failures[2] = (2, True)
        with self.assertRaises(run.WriteFailure):
            policy.apply()
        self.assertEqual(before, self.f.r.bytes)
        self.assertEqual(policy.report['restored'], 2)

    def test_unknown_full_write_failure_is_recoverable_by_exact_readback(self):
        policy, _ = self.prepared()
        before = dict(self.f.r.bytes)
        self.f.writer.failures[2] = (4, False)
        with self.assertRaises(OSError):
            policy.apply()
        self.assertEqual(before, self.f.r.bytes)

    def test_unknown_partial_write_is_kept_pending_without_guessing(self):
        policy, _ = self.prepared()
        self.f.writer.failures[2] = (2, False)
        with self.assertRaises(OSError):
            policy.apply()
        self.assertEqual(policy.report['pending_fields'], [run.ROADIE])
        self.assertEqual(self.f.r.u32(self.f.ambient_address) & self.f.mask, 0)

    def test_failed_restore_keeps_receipt_for_retry(self):
        policy, _ = self.prepared()
        original = self.f.r.read(self.f.roadie_address, 4)
        policy.apply()
        self.f.writer.failures[3] = (0, True)
        self.assertFalse(policy.restore()['released'])
        self.assertEqual(policy.report['pending_fields'], [run.ROADIE])
        self.assertTrue(policy.restore()['released'])
        self.assertEqual(self.f.r.read(self.f.roadie_address, 4), original)

    def test_known_partial_restore_keeps_exact_prefix_receipt_for_retry(self):
        policy, _ = self.prepared()
        original = self.f.r.read(self.f.roadie_address, 4)
        policy.apply()
        self.f.writer.failures[3] = (2, True)
        self.assertFalse(policy.restore()['released'])
        self.assertEqual(policy.report['pending_fields'], [run.ROADIE])
        self.assertTrue(policy.restore()['released'])
        self.assertEqual(self.f.r.read(self.f.roadie_address, 4), original)

    def test_unknown_partial_restore_stays_pending_without_guessing(self):
        policy, _ = self.prepared()
        policy.apply()
        self.f.writer.failures[3] = (2, False)
        self.assertFalse(policy.restore()['released'])
        calls = len(self.f.writer.calls)
        self.assertFalse(policy.restore()['released'])
        self.assertEqual(len(self.f.writer.calls), calls)
        self.assertEqual(policy.report['pending_fields'], [run.ROADIE])

    def test_context_change_during_apply_keeps_receipt_until_owned_context_is_reestablished(self):
        policy, _ = self.prepared()
        def change_owner(address, raw):
            if address == self.f.ambient_address:
                self.f.r.pointer(self.f.modifier + 0x60, self.f.pc)
        self.f.writer.after_write = change_owner
        with self.assertRaises(run.PolicyRejected):
            policy.apply()
        self.assertEqual(policy.report['pending_fields'], [run.AMBIENT])
        self.assertEqual(len(self.f.writer.calls), 1)
        self.f.writer.after_write = None
        self.f.r.pointer(self.f.modifier + 0x60, self.f.camera)
        self.assertTrue(policy.restore()['released'])
        self.assertEqual(self.f.r.u32(self.f.ambient_address) & self.f.mask, 0)

    def test_fresh_world_same_modifier_releases_old_baseline_before_reapply(self):
        policy, _ = self.prepared()
        original = self.f.r.read(self.f.roadie_address, 4)
        policy.apply()
        fresh = self.f.new_world()
        self.assertFalse(policy.check())
        metadata = policy.rebind(fresh)
        self.assertEqual(metadata['fields'][run.ROADIE]['raw'], original.hex())
        self.assertTrue(policy.check())
        policy.close()
        self.assertEqual(self.f.r.read(self.f.roadie_address, 4), original)
        self.assertEqual(self.f.r.u32(self.f.ambient_address) & self.f.mask, 0)

    def test_same_context_rebind_is_idempotent(self):
        policy, _ = self.prepared()
        policy.apply()
        policy.rebind(self.f.sampler)
        self.assertEqual(len(self.f.writer.calls), 2)

    def test_process_recycle_prevents_writes_and_retires_old_receipt(self):
        policy, _ = self.prepared()
        policy.apply()
        self.f.identity['creation_time'] += 1
        calls = len(self.f.writer.calls)
        self.assertFalse(policy.check())
        self.assertTrue(policy.restore()['released'])
        with self.assertRaises(run.PolicyRejected):
            policy.prepare(self.f.sampler)
        self.assertEqual(len(self.f.writer.calls), calls)

    def test_optional_diagnostic_unavailable_is_explicit_and_not_a_write_target(self):
        del self.f.r.properties[self.f.r.classes['WarCameraModifier'], 'Alpha']
        policy, plan = self.prepared()
        self.assertFalse(plan.export()['diagnostics']['Alpha']['available'])
        self.assertIn('reason', plan.export()['diagnostics']['Alpha'])
        policy.apply()
        self.assertEqual(set(a for a, _ in self.f.writer.calls), {self.f.ambient_address, self.f.roadie_address})

    def test_preparation_rejects_bad_type_layout_mask_route_and_context(self):
        cases = ('type', 'dimension', 'size', 'offset', 'mask_zero', 'mask_multibit',
                 'route', 'code', 'membership', 'duplicate', 'backlink', 'pawn', 'mesh', 'default', 'profile',
                 'target', 'array_inner', 'array_bounds', 'nan_factor', 'negative_factor')
        for case in cases:
            with self.subTest(case=case):
                f = Fixture()
                r = f.r
                if case == 'type':
                    r.properties[r.classes['WarCamMod_CameraBone'], run.AMBIENT]['type'] = 'FloatProperty'
                elif case == 'dimension':
                    r.integer(f.ambient_property['address'] + 0x70, 2)
                elif case == 'size':
                    r.integer(f.roadie_property['address'] + 0x74, 8)
                elif case == 'offset':
                    r.integer(f.ambient_property['address'] + 0x94, 0x200)
                elif case.startswith('mask'):
                    r.integer(f.ambient_property['address'] + 0xd0, 0 if case == 'mask_zero' else 3)
                elif case == 'route':
                    r.pointer(f.vtable + 0x2a0, r.base + 1)
                elif case == 'code':
                    r.put(r.base + 0x1d6ea4, b'X')
                elif case == 'membership':
                    r.pointer(f.list_data, 0)
                elif case == 'duplicate':
                    r.pointer(f.list_data + 8, f.modifier)
                elif case == 'backlink':
                    r.pointer(f.modifier + 0x60, f.pc)
                elif case == 'pawn':
                    r.pointer(f.modifier + 0x78, f.pc)
                elif case == 'mesh':
                    r.pointer(f.modifier + 0x80, f.pc)
                elif case == 'default':
                    r.identities[f.modifier]['name'] = 'Default__WarCamMod_CameraBone'
                elif case == 'profile':
                    r.profile['sha256'] = 'unverified'
                elif case == 'target':
                    r.pointer(f.sampler.view_target, f.camera)
                elif case == 'array_inner':
                    array = r.property(r.classes['Camera'], 'ModifierList')
                    inner = r.u64(array['address'] + 0xd0)
                    r.identities[inner]['cls'] = r.classes['StructProperty']
                elif case == 'array_bounds':
                    r.put(f.camera + 0x444, struct.pack('<Qii', f.list_data, 3, 2))
                elif case == 'nan_factor':
                    r.floating(f.roadie_address, float('nan'))
                elif case == 'negative_factor':
                    r.floating(f.roadie_address, -.1)
                before = dict(r.bytes)
                with patch.object(run, 'BOOL_CODE', f.code_cert):
                    with self.assertRaises((run.PolicyRejected, RuntimeError)):
                        f.policy().prepare(f.sampler)
                self.assertEqual(r.bytes, before)
                self.assertEqual(f.writer.calls, [])

    def test_supplied_plan_cannot_be_replaced_or_applied_twice(self):
        policy, plan = self.prepared()
        policy.apply(plan)
        with self.assertRaises(run.PolicyRejected):
            policy.apply(plan)
        with self.assertRaises(run.PolicyRejected):
            policy.prepare(self.f.sampler)


if __name__ == '__main__':
    unittest.main()
