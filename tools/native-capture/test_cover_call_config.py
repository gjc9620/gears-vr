"""Synthetic reflection/source certificates; no game, script assets or hooks."""
import copy
import hashlib
import json
import pathlib
import struct
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'package-inspector'))
from test_ui_observer_config import Fixture as UIFixture
import cover_call_config as cover


class Sampler:
    def __init__(self, fixture):
        self.r = fixture
        self.controller, self.camera, self.pawn, self.local = fixture.pc, fixture.cam, fixture.pawn, fixture.local
        self.resolved = dict(fixture.camera, executable='synthetic-retail.exe')

    def guard(self):
        r = self.r
        return dict(controller=r.identity(self.controller), camera=r.identity(self.camera),
                    pawn=r.identity(self.pawn), local_player=r.identity(self.local), level=dict(address=0x4321),
                    view_target=r.u64(r.cam + 0x700), timestamp_raw=r.read(r.cam + 0x710, 4).hex())


class Fixture(UIFixture):
    def __init__(self):
        super().__init__()
        self.profile['struct_property_struct'] = 0xd0
        for name in ('State', 'ByteProperty', 'BoolProperty', 'StructProperty', 'ScriptStruct', 'ArrayProperty',
                     'StrProperty', 'WarLocalPlayer'):
            self.cls(name, self.classes['Object'])
        self.local = self.obj('LocalPlayer', self.classes['WarLocalPlayer'])
        self.prop('PlayerController', 'Player', 'ObjectProperty', 0x390)
        self.put(self.pc + 0x390, struct.pack('<Q', self.local))
        for prop in self.props.values():
            width = 8 if prop['type'] == 'ObjectProperty' else 4
            self.put(prop['address'] + 0x70, struct.pack('<II', 1, width))
        self.put(self.cam + 0x700, struct.pack('<Q', self.pawn))
        self.put(self.cam + 0x710, struct.pack('<f', 10))
        self.vector = self.obj('Vector', self.classes['ScriptStruct'])
        self.slot = self.obj('CoverSlot', self.classes['ScriptStruct'])
        self.put(self.vector + 0x88, struct.pack('<I', 12))
        self.put(self.slot + 0x88, struct.pack('<I', 48))
        for name, offset in zip(('X', 'Y', 'Z'), (0, 4, 8)):
            self.field(self.vector, name, 'FloatProperty', offset, 4)
        self.field(self.slot, 'Location', 'StructProperty', 0, 12, self.vector)
        self.field(self.slot, 'bEnabled', 'BoolProperty', 12, 4)
        self.field(self.slot, 'bLeft', 'BoolProperty', 12, 4)
        self.field(self.slot, 'Action', 'ByteProperty', 16, 1)
        array = self.field(self.slot, 'Positions', 'ArrayProperty', 24, 16)
        inner = self.field(array, 'Positions', 'StructProperty', 0, 12, self.vector)
        self.put(array + 0xd0, struct.pack('<Q', inner))
        self.states, self.cover_functions, self.cover_fields = {}, {}, {}
        for name in cover.STATES:
            state = self.obj(name, self.classes['State'], self.classes['WarPC'])
            self.children.setdefault(self.classes['WarPC'], []).append(state)
            self.states[name] = state
            move = self.cover_function(state, name + '.PlayerMove', 'PlayerMove', 32)
            update = self.cover_function(state, name + '.UpdatePlayerPosture', 'UpdatePlayerPosture', 96)
            for label, kind, offset, width in (
                    ('PawnCT', 'ByteProperty', 0, 1), ('PawnCA', 'ByteProperty', 1, 1),
                    ('PawnCD', 'ByteProperty', 2, 1), ('bNewLeftSided', 'IntProperty', 4, 4),
                    ('DeltaTime', 'FloatProperty', 8, 4), ('WP', 'ObjectProperty', 16, 8)):
                self.cover_fields[name + '.PlayerMove.' + label] = self.field(move, label, kind, offset, width)
            for label, kind, offset, width in (
                    ('DeltaTime', 'FloatProperty', 0, 4), ('OutPawnCT', 'ByteProperty', 4, 1),
                    ('OutPawnCA', 'ByteProperty', 5, 1), ('OutPawnCD', 'ByteProperty', 6, 1),
                    ('bNewLeftSided', 'IntProperty', 8, 4)):
                self.cover_fields[name + '.UpdatePlayerPosture.' + label] = self.field(update, label, kind, offset, width)
            self.cover_fields[name + '.UpdatePlayerPosture.CurrentSlot'] = self.field(update, 'CurrentSlot', 'StructProperty', 16, 64, self.slot)
        for name, argument in zip(cover.SETTERS, ('NewCA', 'NewCD')):
            function = self.cover_function(self.classes['WarPC'], name, name, 32)
            self.field(function, argument, 'ByteProperty', 0, 1)
            self.field(function, 'WP', 'ObjectProperty', 8, 8)
            if name == 'SetPawnCoverAction':
                array = self.field(function, 'CoverActionParams', 'ArrayProperty', 16, 16)
                inner = self.field(array, 'CoverActionParams', 'StrProperty', 0, 16)
                self.put(array + 0xd0, struct.pack('<Q', inner))
        boundary = cover._boundary_certificate()
        self.put(self.base + boundary['rva'], boundary['bytes'])
        self.link_fields()
        self.finish_registry()
        self.sampler = Sampler(self)

    def field(self, owner, name, kind, offset, width, target=0, dimension=1):
        address = self.obj(name, self.classes[kind], owner)
        self.children.setdefault(owner, []).append(address)
        self.put(address + 0x70, struct.pack('<II', dimension, width))
        self.put(address + 0x78, bytes.fromhex('8877665544332211'))
        self.put(address + 0x94, struct.pack('<I', offset))
        self.put(address + 0xd0, struct.pack('<Q', target))
        return address

    def cover_function(self, owner, key, name, locals_size):
        address = self.obj(name, self.classes['Function'], owner)
        self.children.setdefault(owner, []).append(address)
        script = 0xd00000 + len(self.cover_functions) * 0x100
        raw = bytes((0x01, len(self.cover_functions), 0x02, 0x03))
        self.put(script, raw)
        self.put(address + 0x88, struct.pack('<I', locals_size))
        self.put(address + 0x8c, struct.pack('<Qii', script, len(raw), 8))
        self.put(address + 0x114, struct.pack('<Q', self.base + 0x1d6ca0))
        self.cover_functions[key] = address
        return address

    def link_fields(self):
        for owner, children in self.children.items():
            self.put(owner + 0x80, struct.pack('<Q', children[0] if children else 0))
            for index, address in enumerate(children):
                self.put(address + 0x68, struct.pack('<Q', children[index + 1] if index + 1 < len(children) else 0))


class CoverCallConfigTests(unittest.TestCase):
    def test_readonly_current_config_has_exact_functions_and_live_descriptors(self):
        r = Fixture()
        before = dict(r.memory)
        config, provenance = cover.prepare(r.sampler)
        self.assertEqual(r.memory, before)
        self.assertFalse(config['mutation_authorized'])
        self.assertFalse(config['callsite_aliasing_verified'])
        self.assertEqual(config['registry_closing_snapshots'], config['registry_snapshots'])
        self.assertEqual(config['registry_growth_policy'],
                         dict(header_size=16, headers=dict(objects=r.objects_header, names=r.names_header),
                              same_data_pointer=True, same_capacity=True, count_nondecreasing=True,
                              selection_uses_initial_counts=True, selected_sources_exact=True, cohort_atomic=False))
        self.assertTrue(provenance['read_only'])
        self.assertFalse(provenance['native_runtime_observation'])
        self.assertEqual(hashlib.sha256(json.dumps(config, sort_keys=True, separators=(',', ':')).encode('utf-8')).hexdigest(), provenance['config_sha256'])
        self.assertEqual(config['frame'], dict(node=0x18, object=0x20, code=0x28, locals=0x30, previous=0x38))
        self.assertEqual(len(config['functions']), 6)
        self.assertEqual({x['name'] for x in config['states']}, set(cover.STATES))
        self.assertEqual(len(config['structures']), 2)
        for function in config['functions']:
            script = function['script']
            self.assertEqual(hashlib.sha256(bytes.fromhex(script['bytes'])).hexdigest(), script['sha256'])
            self.assertEqual(bytes.fromhex(script['header_raw']), struct.pack('<Qii', script['address'], script['count'], script['capacity']))
            self.assertFalse(function['flags_classification_verified'])
            for field in function['fields']:
                self.assertEqual(field['flags_raw'], '8877665544332211')
                self.assertFalse(field['flags_interpreted'])
        updater = next(x for x in config['functions'] if x['key'] == cover.STATES[0] + '.UpdatePlayerPosture')
        self.assertEqual({x['name'] for x in updater['fields']}, {'DeltaTime', 'OutPawnCT', 'OutPawnCA', 'OutPawnCD', 'bNewLeftSided', 'CurrentSlot'})
        self.assertNotIn('code_pc', json.dumps(config))
        json.dumps((config, provenance), allow_nan=False)

    def test_updater_names_come_from_live_reflection_without_flag_decoding(self):
        r = Fixture()
        address = r.cover_fields[cover.STATES[0] + '.UpdatePlayerPosture.OutPawnCT']
        name = 'ActualLiveOutName'
        r.names[name] = len(r.names)
        r.put(address + 72, struct.pack('<II', r.names[name], 0))
        r.finish_registry()
        config, _ = cover.prepare(r.sampler)
        function = next(x for x in config['functions'] if x['key'] == cover.STATES[0] + '.UpdatePlayerPosture')
        self.assertIn(name, {x['name'] for x in function['fields']})
        self.assertNotIn('OutPawnCT', {x['name'] for x in function['fields']})

    def test_unrelated_numbered_children_and_numbered_target_siblings_are_discovery_only(self):
        r = Fixture()
        owner = r.classes['WarPC']
        for name, number in (('CONTEXT_SYNTHETIC', 481), ('CONTEXT_SYNTHETIC', 482), (cover.STATES[0], 77)):
            address = r.obj(name, r.classes['State'], owner)
            r.put(address + 76, struct.pack('<I', number))
            r.children[owner].append(address)
        r.link_fields()
        r.finish_registry()
        config, _ = cover.prepare(r.sampler)
        self.assertEqual({config['identities'][x['identity']]['number'] for x in config['states']}, {0})
        self.assertEqual(len(config['functions']), 6)

    def test_numbered_selected_state_function_and_property_are_rejected(self):
        for kind in ('state', 'function', 'property', 'setter'):
            with self.subTest(kind=kind):
                r = Fixture()
                name = cover.STATES[0]
                addresses = {'state': r.states[name], 'function': r.cover_functions[name + '.PlayerMove'],
                             'property': r.cover_fields[name + '.PlayerMove.PawnCT'],
                             'setter': r.cover_functions['SetCoverDirection']}
                r.put(addresses[kind] + 76, struct.pack('<I', 1))
                with self.assertRaises(cover.CoverCallConfigRejected):
                    cover.prepare(r.sampler)

    def test_duplicate_same_name_and_number_discovery_children_are_rejected(self):
        r = Fixture()
        owner = r.classes['WarPC']
        for _ in range(2):
            address = r.obj('CONTEXT_SYNTHETIC', r.classes['ByteProperty'], owner)
            r.put(address + 76, struct.pack('<I', 481))
            r.children[owner].append(address)
        r.link_fields()
        r.finish_registry()
        with self.assertRaisesRegex(cover.CoverCallConfigRejected, 'Duplicate'):
            cover.prepare(r.sampler)

    def test_rejects_shape_owner_duplicate_and_script_failures(self):
        cases = ('caller_width', 'caller_missing', 'caller_type', 'dimension', 'locals_bounds', 'overlap',
                 'duplicate', 'struct_containment', 'struct_member_bounds', 'struct_recursion', 'array_owner',
                 'state_owner', 'state_type', 'function_owner', 'script_bounds', 'entry', 'dispatcher', 'profile', 'layout', 'target')
        for case in cases:
            with self.subTest(case=case):
                r = Fixture()
                state = cover.STATES[0]
                move = r.cover_functions[state + '.PlayerMove']
                field = r.cover_fields[state + '.PlayerMove.PawnCT']
                if case == 'caller_width':
                    r.put(field + 0x74, struct.pack('<I', 4))
                elif case == 'caller_missing':
                    r.children[move].remove(field)
                elif case == 'caller_type':
                    r.put(field + 80, struct.pack('<Q', r.classes['FloatProperty']))
                    r.put(field + 0x74, struct.pack('<I', 4))
                elif case == 'dimension':
                    r.put(field + 0x70, struct.pack('<I', 0))
                elif case == 'locals_bounds':
                    r.put(move + 0x88, struct.pack('<I', 2))
                elif case == 'overlap':
                    other = r.cover_fields[state + '.PlayerMove.PawnCA']
                    r.put(other + 0x94, struct.pack('<I', 0))
                elif case == 'duplicate':
                    r.field(move, 'PawnCT', 'ByteProperty', 24, 1)
                elif case == 'struct_containment':
                    r.put(r.slot + 0x88, struct.pack('<I', 80))
                elif case == 'struct_member_bounds':
                    r.put(r.children[r.vector][2] + 0x94, struct.pack('<I', 12))
                elif case == 'struct_recursion':
                    member = r.children[r.slot][0]
                    r.put(member + 0xd0, struct.pack('<Q', r.slot))
                    r.put(member + 0x74, struct.pack('<I', 48))
                elif case == 'array_owner':
                    array = r.children[r.slot][-1]
                    inner = r.u64(array + 0xd0)
                    r.put(inner + 64, struct.pack('<Q', r.slot))
                elif case == 'state_owner':
                    r.put(r.states[state] + 64, struct.pack('<Q', r.classes['PlayerController']))
                elif case == 'state_type':
                    r.put(r.states[state] + 80, struct.pack('<Q', r.classes['Function']))
                elif case == 'function_owner':
                    r.put(move + 64, struct.pack('<Q', r.classes['WarPC']))
                elif case == 'script_bounds':
                    r.put(move + 0x8c, struct.pack('<Qii', 0xd00000, cover.MAX_SCRIPT + 1, cover.MAX_SCRIPT + 1))
                elif case == 'entry':
                    r.put(move + 0x114, struct.pack('<Q', r.base + 0x123))
                elif case == 'dispatcher':
                    r.put(r.base + cover._boundary_certificate()['rva'], b'\xff')
                elif case == 'profile':
                    r.profile['sha256'] = 'unknown'
                elif case == 'layout':
                    r.profile['object_class'] = 88
                elif case == 'target':
                    r.put(r.cam + 0x700, struct.pack('<Q', r.cam))
                r.link_fields()
                r.finish_registry()
                before = dict(r.memory)
                with self.assertRaises(cover.CoverCallConfigRejected):
                    cover.prepare(r.sampler)
                self.assertEqual(before, r.memory)

    def test_changes_at_closing_boundary_reject_whole_config(self):
        for kind in ('script', 'script_header', 'entry', 'locals_size', 'property_flags', 'identity', 'registry', 'field_link'):
            with self.subTest(kind=kind):
                r = Fixture()
                move = r.cover_functions[cover.STATES[0] + '.PlayerMove']
                prop = r.cover_fields[cover.STATES[0] + '.PlayerMove.PawnCT']
                addresses = {'script': (0xd00000, 4), 'script_header': (move + 0x8c, 16),
                             'entry': (move + 0x114, 8), 'locals_size': (move + 0x88, 4),
                             'property_flags': (prop + 0x78, 8), 'identity': (move + 72, 8),
                             'registry': (r.objects_header, 16), 'field_link': (prop + 0x68, 8)}
                selected = addresses[kind]
                read, calls = r.read, []
                def changing(address, size):
                    if (address, size) == selected:
                        calls.append(1)
                        if len(calls) == 3:
                            raw = bytearray(read(address, size))
                            raw[0] ^= 1
                            return bytes(raw)
                    return read(address, size)
                r.read = changing
                expected = cover.CoverCallRegistryChanged if kind == 'registry' else cover.CoverCallConfigRejected
                with self.assertRaises(expected):
                    cover.prepare(r.sampler)

    def test_same_vm_entry_does_not_authorize_wrong_state_owner(self):
        r = Fixture()
        wrong = r.cover_functions[cover.STATES[1] + '.PlayerMove']
        owner = r.states[cover.STATES[0]]
        r.children[owner][0] = wrong
        r.link_fields()
        with self.assertRaises(cover.CoverCallConfigRejected):
            cover.prepare(r.sampler)

    def test_same_allocation_registry_growth_preserves_initial_bounds_and_records_closing_headers(self):
        for registry in ('objects', 'names'):
            for phase in ('double_read', 'closing'):
                with self.subTest(registry=registry, phase=phase):
                    r = Fixture()
                    r.object_count = r.name_count = 1101  # Resolver caches must not be changed.
                    address = getattr(r, registry + '_header')
                    pointer = r.od if registry == 'objects' else r.nd
                    baseline = struct.pack('<Qii', pointer, 1101, 1300)
                    r.put(address, baseline)
                    read, counts, before = r.read, [], dict(r.memory)

                    def growing(a, size):
                        if (a, size) == (address, 16):
                            counts.append(1)
                            values = (1101, 1102, 1104) if phase == 'double_read' else (1101, 1101, 1103)
                            return struct.pack('<Qii', pointer, values[min(len(counts) - 1, 2)], 1300)
                        return read(a, size)

                    r.read = growing
                    config, _ = cover.prepare(r.sampler)
                    final_count = 1104 if phase == 'double_read' else 1103
                    self.assertEqual(config['registry_snapshots'][registry], baseline.hex())
                    self.assertEqual(config['registry_closing_snapshots'][registry],
                                     struct.pack('<Qii', pointer, final_count, 1300).hex())
                    check = next(x for x in config['source_checks'] if (x['address'], x['size']) == (address, 16))
                    self.assertEqual(check['bytes'], baseline.hex())
                    self.assertEqual(len(counts), 3)
                    self.assertEqual((r.object_count, r.name_count), (1101, 1101))
                    self.assertEqual(r.memory, before)
                    self.assertTrue(all(x['internal_index'] < 1101 and x['name_index'] < 1101
                                        for x in config['identities']))

    def test_registry_shrink_reallocation_capacity_and_invalid_bounds_reject(self):
        kinds = ('shrink', 'shrink_after_growth', 'pointer', 'capacity_grow', 'capacity_shrink',
                 'count_zero', 'count_negative', 'count_past_capacity', 'capacity_negative',
                 'capacity_limit', 'pointer_null', 'pointer_alignment', 'pointer_overflow')
        for registry in ('objects', 'names'):
            for phase in ('double_read', 'closing'):
                for kind in kinds:
                    with self.subTest(registry=registry, phase=phase, kind=kind):
                        r = Fixture()
                        address = getattr(r, registry + '_header')
                        pointer = r.od if registry == 'objects' else r.nd
                        r.put(address, struct.pack('<Qii', pointer, 1101, 1300))
                        read, hits = r.read, []

                        def changing(a, size):
                            if (a, size) != (address, 16):
                                return read(a, size)
                            hits.append(1)
                            observed = [pointer, 1101, 1300]
                            if kind == 'shrink_after_growth' and len(hits) == 2:
                                observed[1] = 1104
                            trigger = 3 if kind == 'shrink_after_growth' or phase == 'closing' else 2
                            if len(hits) >= trigger:
                                mutations = {'shrink': (1, 1100), 'shrink_after_growth': (1, 1102),
                                             'pointer': (0, pointer + 0x400000), 'capacity_grow': (2, 1301),
                                             'capacity_shrink': (2, 1299), 'count_zero': (1, 0),
                                             'count_negative': (1, -1), 'count_past_capacity': (1, 1301),
                                             'capacity_negative': (2, -1), 'capacity_limit': (2, 4000000),
                                             'pointer_null': (0, 0), 'pointer_alignment': (0, pointer + 1),
                                             'pointer_overflow': (0, (1 << 64) - 8)}
                                index, value = mutations[kind]
                                observed[index] = value
                            return struct.pack('<Qii', *observed)

                        r.read = changing
                        with self.assertRaises(cover.CoverCallRegistryChanged):
                            cover.prepare(r.sampler)

    def test_valid_registry_growth_does_not_relax_selected_sources(self):
        for kind in ('identity', 'object_slot', 'name_slot', 'name_entry', 'property_flags',
                     'script', 'script_header', 'field_link'):
            with self.subTest(kind=kind):
                r = Fixture()
                for header, pointer in ((r.objects_header, r.od), (r.names_header, r.nd)):
                    r.put(header, struct.pack('<Qii', pointer, 1101, 1300))
                move = r.cover_functions[cover.STATES[0] + '.PlayerMove']
                prop = r.cover_fields[cover.STATES[0] + '.PlayerMove.PawnCT']
                index, ni = r.u32(move + 8), r.u32(move + 72)
                entry = r.u64(r.nd + ni * 8)
                selected = {'identity': (move + 72, 8), 'object_slot': (r.od + index * 8, 8),
                            'name_slot': (r.nd + ni * 8, 8), 'name_entry': (entry + 20, 128),
                            'property_flags': (prop + 0x78, 8), 'script': (0xd00000, 4),
                            'script_header': (move + 0x8c, 16), 'field_link': (prop + 0x68, 8)}[kind]
                original_finish = cover._Capture.finish

                def change_at_finish(cap):
                    for header, pointer in ((r.objects_header, r.od), (r.names_header, r.nd)):
                        r.put(header, struct.pack('<Qii', pointer, 1102, 1300))
                    address, size = selected
                    raw = bytearray(r.read(address, size))
                    raw[0] ^= 1
                    r.put(address, raw)
                    return original_finish(cap)

                with patch.object(cover._Capture, 'finish', change_at_finish):
                    with self.assertRaises(cover.CoverCallConfigRejected) as rejected:
                        cover.prepare(r.sampler)
                self.assertNotIsInstance(rejected.exception, cover.CoverCallRegistryChanged)

    def test_growth_exception_has_exact_address_size_and_never_admits_new_indices(self):
        r = Fixture()
        for header, pointer in ((r.objects_header, r.od), (r.names_header, r.nd)):
            r.put(header, struct.pack('<Qii', pointer, 1101, 1300))
        read, hits = r.read, {r.objects_header: 0, r.names_header: 0}

        def growing(address, size):
            if size == 16 and address in hits:
                hits[address] += 1
                pointer = r.od if address == r.objects_header else r.nd
                return struct.pack('<Qii', pointer, 1100 + hits[address], 1300)
            return read(address, size)

        r.read = growing
        cap = cover._Capture(r.sampler)
        self.assertEqual((cap.object_count, cap.name_count), (1101, 1101))
        for target, offset in ((r.pc, 8), (r.cam, 72)):
            before = read(target + offset, 4)
            r.put(target + offset, struct.pack('<I', 1101))
            with self.assertRaisesRegex(cover.CoverCallConfigRejected, 'registry slot differs'):
                cap.fresh(target)
            r.put(target + offset, before)
        move = r.cover_functions[cover.STATES[0] + '.PlayerMove']
        for address, size, byte_offset in ((move + 0x8c, 16, 8), (r.objects_header + 8, 4, 0)):
            calls = []

            def other_changing(a, n):
                raw = read(a, n)
                if (a, n) == (address, size):
                    calls.append(1)
                    if len(calls) == 2:
                        changed = bytearray(raw)
                        changed[byte_offset] += 1
                        return bytes(changed)
                return raw

            r.read = other_changing
            with self.assertRaises(cover.CoverCallConfigRejected) as rejected:
                cap.stable(address, size)
            self.assertNotIsInstance(rejected.exception, cover.CoverCallRegistryChanged)

    def test_existing_boundary_source_certificate_change_rejects(self):
        r = Fixture()
        first = cover._boundary_certificate()
        changed = copy.deepcopy(first)
        changed['source_sha256'] = 'changed'
        with patch.object(cover, '_boundary_certificate', side_effect=[first, changed]):
            with self.assertRaisesRegex(cover.CoverCallConfigRejected, 'source certificate changed'):
                cover.prepare(r.sampler)


if __name__ == '__main__':
    unittest.main()
