"""Focused selected-dependency registry checks for bounded Actor.Trace reads."""
import struct
import unittest
from pathlib import Path
from types import SimpleNamespace
import sys
import copy
from unittest.mock import patch

from cover_trace_query import TraceQuery, capture_edge_interval, _surface_state, SurfaceOwnershipRejected


class Memory:
    def __init__(self):
        self.values = {}
        self.objects_header, self.names_header = 0x100, 0x200
        self.objects_data, self.names_data = 0x1000, 0x2000
        self.object_count, self.object_capacity = 10, 100
        self.name_count, self.name_capacity = 10, 100
        self.name_pointers = [0] * 10
        self.profile = dict(object_internal_index=0, object_name=4,
                            object_class=16, object_outer=24,
                            name_entry_index=0, name_entry_text=20)
        self.put(self.objects_header, struct.pack('<Qii', self.objects_data, 10, 100))
        self.put(self.names_header, struct.pack('<Qii', self.names_data, 10, 100))

    def put(self, address, raw):
        self.values[address, len(raw)] = raw

    def read(self, address, size):
        return self.values[address, size]

    def object(self, *, address=0x5000, index=3, name_index=4, name='CoverLink'):
        entry = 0x6000
        identity = dict(address=address, internal_index=index, name=name,
                        name_index=name_index, number=1, cls=0x7000, outer=0x8000)
        self.put(address, struct.pack('<I', index))
        self.put(address + 4, struct.pack('<II', name_index, 1))
        self.put(address + 16, struct.pack('<Q', identity['cls']))
        self.put(address + 24, struct.pack('<Q', identity['outer']))
        self.put(self.objects_data + index * 8, struct.pack('<Q', address))
        self.put(self.names_data + name_index * 8, struct.pack('<Q', entry))
        self.put(entry, struct.pack('<I', name_index))
        self.put(entry + 20, name.encode('utf-16-le') + b'\0\0')
        self.name_pointers[name_index] = entry
        return identity


class LiveRegistryAnchorSampler:
    """Fixture with the selected slot/name checks required of the real sampler."""
    __module__ = 'anchor_config'

    def __init__(self, resolver, identity):
        self.r, self.identity = resolver, identity

    def fresh(self, address):
        if address != self.identity['address']:
            raise RuntimeError('Unknown selected object')
        index, name_index = self.identity['internal_index'], self.identity['name_index']
        if struct.unpack('<Q', self.r.read(self.r.objects_data + index * 8, 8))[0] != address:
            raise RuntimeError('Selected object registry slot changed')
        # Production fresh() validates the *current* entry's index/text, but
        # does not compare its address to the resolver's original name slot.
        entry = struct.unpack('<Q', self.r.read(self.r.names_data + name_index * 8, 8))[0]
        expected_text = self.identity['name'].encode('utf-16-le') + b'\0\0'
        try:
            valid = (struct.unpack('<I', self.r.read(entry, 4))[0] == name_index and
                     self.r.read(entry + 20, len(expected_text)) == expected_text)
        except KeyError:
            valid = False
        if not valid:
            raise RuntimeError('Selected name entry changed')
        return dict(self.identity)


class Observer:
    def __init__(self, resolver, identity):
        self.resolver = resolver
        self.sampler = LiveRegistryAnchorSampler(resolver, identity)

    def verify(self):
        return None


class TraceRegistryTests(unittest.TestCase):
    def fixture(self):
        resolver = Memory()
        identity = resolver.object()
        query = TraceQuery(Observer(resolver, identity))
        self.assertEqual(query.obj(identity['address']), identity)
        return resolver, query

    def test_object_tail_growth_keeps_selected_slot_and_name_fixed(self):
        resolver, query = self.fixture()
        resolver.put(resolver.objects_header,
                     struct.pack('<Qii', resolver.objects_data, 11, 100))
        query.finish()
        self.assertEqual(query.last_object_count, 11)
        self.assertTrue(query.registry_observations)
        self.assertEqual(len(query.identities), 1)

    def test_name_growth_and_count_rollback_are_rejected(self):
        resolver, query = self.fixture()
        resolver.put(resolver.names_header,
                     struct.pack('<Qii', resolver.names_data, 11, 100))
        with self.assertRaisesRegex(RuntimeError, 'selected registry growth'):
            query.finish()
        resolver.put(resolver.names_header,
                     struct.pack('<Qii', resolver.names_data, 10, 100))
        resolver.put(resolver.objects_header,
                     struct.pack('<Qii', resolver.objects_data, 11, 100))
        query.finish()
        resolver.put(resolver.objects_header,
                     struct.pack('<Qii', resolver.objects_data, 10, 100))
        with self.assertRaisesRegex(RuntimeError, 'selected registry growth'):
            query.finish()

    def test_observed_growth_cannot_disappear_between_finish_passes(self):
        for header in ('objects', 'names'):
            with self.subTest(header=header):
                resolver, query = self.fixture()
                original_read = resolver.read
                counts = iter((11, 10))
                selected = (resolver.objects_header if header == 'objects'
                            else resolver.names_header)
                data = resolver.objects_data if header == 'objects' else resolver.names_data

                def read(address, size):
                    if address == selected and size == 16:
                        return struct.pack('<Qii', data, next(counts), 100)
                    return original_read(address, size)

                resolver.read = read
                with self.assertRaisesRegex(RuntimeError, 'selected registry growth'):
                    query.finish()

    def test_old_object_or_name_slot_change_is_rejected_despite_growth(self):
        for changed in ('object', 'name'):
            with self.subTest(changed=changed):
                resolver, query = self.fixture()
                resolver.put(resolver.objects_header,
                             struct.pack('<Qii', resolver.objects_data, 11, 100))
                address = (resolver.objects_data + 3 * 8 if changed == 'object'
                           else resolver.names_data + 4 * 8)
                resolver.put(address, struct.pack('<Q', 0x9999))
                with self.assertRaises(RuntimeError):
                    query.finish()

    def test_same_name_text_at_a_new_entry_pointer_is_rejected(self):
        resolver, query = self.fixture()
        replacement = 0x6100
        resolver.put(replacement, struct.pack('<I', 4))
        resolver.put(replacement + 20, 'CoverLink'.encode('utf-16-le') + b'\0\0')
        resolver.put(resolver.names_data + 4 * 8, struct.pack('<Q', replacement))
        # The fixture's sampler still accepts this same-index/same-text entry.
        self.assertEqual(query.s.fresh(0x5000)['name'], 'CoverLink')
        with self.assertRaises(RuntimeError):
            query.finish()

    def test_actual_live_sampler_accepts_same_text_swap_but_trace_rejects(self):
        root = Path(__file__).resolve().parents[2]
        sys.path[:0] = [str(root / 'tools/native-capture'),
                        str(root / 'tools/package-inspector')]
        from anchor_config import LiveRegistryAnchorSampler as ActualSampler
        from test_anchor_config import Fixture as ActualResolverFixture

        resolver = ActualResolverFixture()
        # The real Resolver caches this pointer tuple during discovery.
        resolver.name_pointers = tuple(resolver.u64(resolver.names_data + i * 8)
                                       for i in range(resolver.name_count))
        sampler = ActualSampler(resolver)
        query = TraceQuery(SimpleNamespace(resolver=resolver, sampler=sampler,
                                           verify=lambda: None))
        address = 0x20000
        original = query.obj(address)
        name_index = original['name_index']
        replacement = 0x900000
        resolver.put(replacement, resolver.read(resolver.name_pointers[name_index], 148))
        resolver.put(resolver.names_data + name_index * 8, struct.pack('<Q', replacement))
        self.assertEqual(sampler.fresh(address), original)
        with self.assertRaises(RuntimeError):
            query.finish()

    def test_new_tail_object_cannot_become_trace_dependency(self):
        resolver = Memory()
        identity = resolver.object(index=10)
        query = TraceQuery(Observer(resolver, identity))
        with self.assertRaisesRegex(RuntimeError, 'newly registered tail'):
            query.obj(identity['address'])


class EdgeIntervalTests(unittest.TestCase):
    def run_probe(self, *, delta=(0, 0, 64), direction=(1, 0, 0), mutate=None, **options):
        calls = []

        def trace(observer, path, digest, start, end, **kwargs):
            observed = kwargs.pop('snapshot_surface')
            actor = kwargs.pop('surface_actor')
            component = kwargs.pop('surface_component')
            self.assertIs(observed, True)
            self.assertEqual(component, dict(address=90) if actor is not None else None)
            calls.append((start, end, kwargs))
            fraction = sum(start[i] * delta[i] for i in range(3)) / sum(x * x for x in delta)
            blocked = fraction < .6
            receipt = dict(accepted=True, trace_calls=1, pending_possible=False,
                           allocation_retained=False, session_id='owned',
                           game_identity=dict(pid=100, created=200), owner_identity=dict(pid=300),
                           roles=dict(pawn=dict(address=1), controller=dict(address=2),
                                      link=dict(address=3), world=dict(address=4), current_slot=0),
                           pawn_position=(0, 0, 0), module=dict(base=10, sha256='module'),
                           function=dict(address=20),
                           inputs=dict(start=start, end=end, **kwargs),
                           hit=dict(actor_address=9 if blocked else 0,
                                    location=tuple((start[i] + end[i]) / 2 for i in range(3)) if blocked else (0, 0, 0),
                                    normal=tuple(-x for x in direction) if blocked else (0, 0, 0)))
            if blocked:
                receipt['hit']['actor_identity'] = dict(address=9, name='ActualWall', index=12)
            state = dict(actor=dict(address=9, name='ActualWall', index=12),
                         component=dict(address=90), world=receipt['roles']['world'],
                         local_to_world_bytes='same_measured_transform')
            receipt['surface_after'] = copy.deepcopy(state)
            if actor is not None:
                receipt['surface_before'] = copy.deepcopy(state)
            if mutate:
                mutate(receipt, len(calls))
            return receipt

        result = capture_edge_interval(None, 'already-loaded.dll', 'digest', (0, 0, 0),
                                       tuple(100 * x for x in direction), delta,
                                       capture_ray=trace, **options)
        return result, calls

    def test_top_and_both_side_directions_use_bounded_current_rays(self):
        for delta, direction in (((0, 0, 64), (1, 0, 0)),
                                 ((0, -64, 0), (1, 0, 0)),
                                 ((0, 64, 0), (1, 0, 0)),
                                 ((-64, 0, 0), (0, 1, 0)),
                                 ((0, 64, 0), (-1, 0, 0))):
            with self.subTest(delta=delta, direction=direction):
                result, calls = self.run_probe(delta=delta, direction=direction)
                self.assertTrue(result['accepted'], result)
                self.assertLessEqual(len(calls), 8)
                self.assertLess(result['blocked_fraction'], .6)
                self.assertGreaterEqual(result['clear_fraction'], .6)
                self.assertLessEqual(result['interval_width'], 1)
                self.assertFalse(result['grab_authorized'])
                self.assertFalse(result['collision_verified'])
                self.assertFalse(result['body_modified'])
                for _, _, kwargs in calls:
                    self.assertEqual(kwargs, dict(extent=(0, 0, 0), trace_actors=False, extra_flags=8))

    def test_invalid_plan_is_rejected_before_any_call(self):
        for options in (dict(tolerance=0), dict(tolerance=float('nan')), dict(tolerance=True),
                        dict(tolerance=64), dict(maximum_calls=9), dict(maximum_calls=True),
                        dict(maximum_calls=2), dict(maximum_seconds=31), dict(maximum_seconds=True),
                        dict(delta=(64, 0, 0)), dict(delta=(0, 0, 201))):
            with self.subTest(options=options):
                result, calls = self.run_probe(**options)
                self.assertFalse(result['accepted'])
                self.assertEqual(calls, [])

    def test_failed_or_pending_trace_never_becomes_empty_and_is_not_retried(self):
        for change in (dict(accepted=False), dict(trace_calls=0), dict(pending_possible=True),
                       dict(allocation_retained=True)):
            with self.subTest(change=change):
                result, calls = self.run_probe(mutate=lambda r, n: r.update(change) if n == 2 else None)
                self.assertFalse(result['accepted'])
                self.assertEqual(len(calls), 2)
                self.assertEqual(len(result['rays']), 2)

    def test_selected_binding_change_stops_search(self):
        for field in ('session_id', 'game_identity', 'owner_identity', 'roles',
                      'pawn_position', 'module', 'function'):
            with self.subTest(field=field):
                def mutate(receipt, n):
                    if n == 2:
                        receipt[field] = 'changed'
                result, calls = self.run_probe(mutate=mutate)
                self.assertFalse(result['accepted'])
                self.assertIn('binding changed', result['rejection'])
                self.assertEqual(len(calls), 2)

    def test_input_changes_are_rejected(self):
        for field, value in (('start', (1, 2, 3)), ('extent', (1, 0, 0)),
                             ('trace_actors', True), ('extra_flags', 0)):
            with self.subTest(field=field):
                result, calls = self.run_probe(mutate=lambda r, n: r['inputs'].update({field: value}))
                self.assertFalse(result['accepted'])
                self.assertEqual(len(calls), 1)

    def test_self_cover_marker_floor_and_bad_ray_hits_are_rejected(self):
        for change in (dict(actor_address=1, actor_identity=dict(address=1)),
                       dict(actor_address=2, actor_identity=dict(address=2)),
                       dict(actor_address=3, actor_identity=dict(address=3)),
                       dict(normal=(0, 0, 1)), dict(normal=(1, 0, 0)),
                       dict(normal=(-2, 0, 0)), dict(location=(150, 0, 0)),
                       dict(location=(50, 1, 0)), dict(location=(0, 0, 0)),
                       dict(location=(float('nan'), 0, 0)),
                       dict(actor_identity=dict(address=11))):
            with self.subTest(change=change):
                result, calls = self.run_probe(mutate=lambda r, n: r['hit'].update(change))
                self.assertFalse(result['accepted'])
                self.assertEqual(len(calls), 1)

    def test_different_wall_during_bisection_stops(self):
        def mutate(receipt, n):
            if n == 3:
                receipt['hit'].update(actor_address=10, actor_identity=dict(address=10))
        result, calls = self.run_probe(mutate=mutate)
        self.assertFalse(result['accepted'])
        self.assertIn('different collision actor', result['rejection'])
        self.assertEqual(len(calls), 3)

    def test_unexplained_empty_hit_data_stops(self):
        def mutate(receipt, n):
            if n == 2:
                receipt['hit']['normal'] = (-1, 0, 0)
        result, calls = self.run_probe(mutate=mutate)
        self.assertFalse(result['accepted'])
        self.assertIn('unexplained hit data', result['rejection'])
        self.assertEqual(len(calls), 2)

    def test_empty_start_and_blocked_outside_do_not_infer_edge(self):
        def empty(receipt, n):
            receipt['hit'] = dict(actor_address=0, location=(0, 0, 0), normal=(0, 0, 0))
        result, calls = self.run_probe(mutate=empty)
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 1)

        def wall(receipt, n):
            start, end = receipt['inputs']['start'], receipt['inputs']['end']
            receipt['hit'] = dict(actor_address=9, actor_identity=dict(address=9, name='ActualWall', index=12),
                                  location=tuple((a + b) / 2 for a, b in zip(start, end)), normal=(-1, 0, 0))
        result, calls = self.run_probe(mutate=wall)
        self.assertFalse(result['accepted'])
        self.assertIn('still blocked', result['rejection'])
        self.assertEqual(len(calls), 2)

    def test_elapsed_budget_stops_after_current_call_without_retry(self):
        with patch('cover_trace_query.time.monotonic', side_effect=(0, 0, 31)):
            result, calls = self.run_probe()
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 1)
        self.assertEqual(result['rejection_type'], 'TimeoutError')

    def test_receipts_are_frozen_and_missing_evidence_is_rejected(self):
        saved = []
        def mutate(receipt, n):
            if saved:
                saved[0]['hit']['actor_address'] = 99
            saved.append(receipt)
        result, _ = self.run_probe(mutate=mutate)
        self.assertTrue(result['accepted'], result)
        self.assertEqual(result['rays'][0]['receipt']['hit']['actor_address'], 9)
        result, calls = self.run_probe(mutate=lambda r, n: r.pop('roles'))
        self.assertFalse(result['accepted'])
        self.assertEqual(len(calls), 1)

    def test_surface_motion_or_component_replacement_stops_even_on_an_empty_ray(self):
        for side in ('surface_before', 'surface_after'):
            for field, value in (('component', dict(address=91)), ('local_to_world_bytes', 'moved')):
                with self.subTest(side=side, field=field):
                    def mutate(receipt, n):
                        if n == 2:
                            receipt[side][field] = value
                    result, calls = self.run_probe(mutate=mutate)
                    self.assertFalse(result['accepted'])
                    self.assertIn('surface changed', result['rejection'])
                    self.assertEqual(len(calls), 2)


class SurfaceSnapshotTests(unittest.TestCase):
    def fixture(self):
        from test_cover_surface_query import SurfaceMemory
        from cover_state import CoverQuery
        m = SurfaceMemory()
        wall, component = 0x18000, 0x19000
        m.add_object(wall, 'MeasuredWall', m.ACTOR)
        m.add_object(component, 'MeasuredCollision', m.MESH_CLASS)
        m.add_property(m.ACTOR, 'WorldInfo', 'ObjectProperty', 0x180, 8)
        m.add_property(m.ACTOR, 'CollisionComponent', 'ComponentProperty', 0x188, 8)
        m.add_property(m.MESH_CLASS, 'Owner', 'ObjectProperty', 0x180, 8)
        m.add_property(m.MESH_CLASS, 'LocalToWorld', 'StructProperty', 0x200, 64, target=m.MATRIX)
        m.put(m.MATRIX + 0x88, struct.pack('<I', 64))
        m.put(m.PLANE + 0x88, struct.pack('<I', 16))
        m.set_value(wall, 'WorldInfo', m.WORLD, '<Q')
        m.set_value(wall, 'CollisionComponent', component, '<Q')
        m.set_value(component, 'Owner', wall, '<Q')
        m.put(component + 0x200, struct.pack('<16f', 1, 0, 0, 0, 0, 1, 0, 0,
                                            0, 0, 1, 0, 10, 20, 30, 1))
        for (owner, _), p in m.properties.items():
            kind = p['type']
            address = next(a for a, obj in m.objects.items() if obj['name'] == kind and
                           obj['cls'] == m.META_CLASS)
            m.objects[p['address']]['cls'] = address
        for owner in {owner for owner, _ in m.properties}:
            addresses = [p['address'] for (declaring, _), p in m.properties.items() if declaring == owner]
            m.put(owner + 0x80, struct.pack('<Q', addresses[0]))
            for current, nxt in zip(addresses, addresses[1:] + [0]):
                m.put(current + 0x68, struct.pack('<Q', nxt))
        return m, CoverQuery(m.observer), dict(world=m.fresh(m.WORLD)), m.fresh(wall), component

    def collection_fixture(self):
        m, q, roles, actor, component = self.fixture()
        cls, inner, data = 0x2a000, 0x2b000, 0x2c000
        m.add_class(cls, 'StaticMeshCollectionActor', m.ACTOR)
        m.objects[actor['address']]['cls'] = cls
        m.objects[m.MESH_CLASS]['name'] = 'StaticMeshComponent'
        m.add_property(cls, 'StaticMeshComponents', 'ArrayProperty', 0x190, 16)
        field = m.properties[cls, 'StaticMeshComponents']
        m.objects[field['address']]['cls'] = m.kind_classes['ArrayProperty']
        m.put(cls + m.profile['struct_children'], struct.pack('<Q', field['address']))
        m.put(field['address'] + m.profile['field_next'], bytes(8))
        kind = next(a for a, obj in m.objects.items() if obj['name'] == 'ComponentProperty'
                    and obj['cls'] == m.META_CLASS)
        m.add_object(inner, 'StaticMeshComponents', kind, outer=field['address'])
        m.put(inner + 0x70, struct.pack('<II', 1, 8))
        m.put(inner + m.profile['struct_property_struct'], struct.pack('<Q', m.MESH_CLASS))
        m.put(field['address'] + m.profile['struct_property_struct'], struct.pack('<Q', inner))
        m.put(actor['address'] + 0x190, struct.pack('<Qii', data, 2, 2))
        m.put(data, struct.pack('<QQ', component, 0))
        m.set_value(actor['address'], 'CollisionComponent', 0, '<Q')
        return m, q, roles, m.fresh(actor['address']), m.fresh(component), inner, data

    def test_collection_requires_explicit_unique_typed_hit_component(self):
        for changed in ('members', 'inner_link', 'element_class', 'owner', 'transform'):
            with self.subTest(closing_change=changed):
                m, q, roles, actor, component, inner, data = self.collection_fixture()
                # A collection name or a null Actor root never selects a component.
                with self.assertRaises(SurfaceOwnershipRejected):
                    _surface_state(q, roles, actor)
                result = _surface_state(q, roles, actor, component)
                self.assertEqual(result['component'], component)
                self.assertIn('collection_membership', result)
                q.finish()
                if changed == 'members':
                    m.put(data, struct.pack('<QQ', 0, component['address']))
                elif changed == 'inner_link':
                    field = m.properties[actor['cls'], 'StaticMeshComponents']
                    m.put(field['address'] + m.profile['struct_property_struct'], bytes(8))
                elif changed == 'element_class':
                    m.put(inner + m.profile['struct_property_struct'], struct.pack('<Q', m.ACTOR))
                elif changed == 'owner':
                    m.set_value(component['address'], 'Owner', m.PAWN, '<Q')
                else:
                    m.put(component['address'] + 0x230, struct.pack('<f', 11))
                with self.assertRaises(RuntimeError):
                    q.finish()

    def test_collection_identity_membership_type_and_owner_fail_closed(self):
        for bad in ('actor_class', 'not_member', 'duplicate', 'inner_kind', 'inner_owner',
                    'stride', 'element_class', 'component_class', 'owner', 'selected_identity', 'world'):
            with self.subTest(bad=bad):
                m, q, roles, actor, component, inner, data = self.collection_fixture()
                if bad == 'actor_class':
                    m.objects[actor['cls']]['name'] = 'StaticMeshActor'
                elif bad == 'not_member':
                    m.put(data, bytes(16))
                elif bad == 'duplicate':
                    m.put(data, struct.pack('<QQ', component['address'], component['address']))
                elif bad == 'inner_kind':
                    m.objects[inner]['cls'] = m.kind_classes['ObjectProperty']
                elif bad == 'inner_owner':
                    m.objects[inner]['outer'] = actor['cls']
                elif bad == 'stride':
                    m.put(inner + 0x74, struct.pack('<I', 4))
                elif bad == 'element_class':
                    m.put(inner + m.profile['struct_property_struct'], struct.pack('<Q', m.ACTOR))
                elif bad == 'component_class':
                    m.objects[component['address']]['cls'] = m.ACTOR
                    component = m.fresh(component['address'])
                elif bad == 'owner':
                    m.set_value(component['address'], 'Owner', m.PAWN, '<Q')
                elif bad == 'selected_identity':
                    component = dict(component, number=1)
                elif bad == 'world':
                    m.set_value(actor['address'], 'WorldInfo', m.PAWN, '<Q')
                with self.assertRaises(RuntimeError):
                    _surface_state(q, roles, actor, component)

    def test_explicit_single_collision_component_keeps_original_snapshot(self):
        m, q, roles, actor, component = self.fixture()
        original = _surface_state(q, roles, actor)
        self.assertEqual(_surface_state(q, roles, actor, m.fresh(component)), original)
        with self.assertRaisesRegex(RuntimeError, 'differs from Actor collision component'):
            _surface_state(q, roles, actor, m.fresh(m.PAWN))
        q.finish()

    def test_inherited_world_hint_does_not_become_a_declared_collection_field(self):
        m, q, roles, actor, component = self.fixture()
        collection_class = 0x2a000
        m.add_class(collection_class, 'StaticMeshCollectionActor', m.ACTOR)
        m.put(collection_class + m.profile['struct_children'], bytes(8))
        m.objects[actor['address']]['cls'] = collection_class
        actor = m.fresh(actor['address'])

        # Match the actual metadata-only path: an inherited WorldInfo hint is
        # read before _surface_state authenticates its declaring Actor class.
        self.assertEqual(q.pointer(actor, 'WorldInfo'), roles['world'])
        result = _surface_state(q, roles, actor)
        self.assertEqual(result['component']['address'], component)
        self.assertEqual(result['actor'], actor)
        with self.assertRaisesRegex(RuntimeError, '^Property WorldInfo not found$'):
            q.prop(q.obj(collection_class), 'WorldInfo', structure=True)
        q.finish()
        m.put(collection_class + m.profile['field_super'], bytes(8))
        with self.assertRaises(RuntimeError):
            q.finish()

    def test_typed_collision_snapshot_and_closing_transform_change(self):
        m, q, roles, actor, component = self.fixture()
        result = _surface_state(q, roles, actor)
        self.assertEqual(result['component']['address'], component)
        self.assertEqual(result['actor'], actor)
        q.finish()
        raw = bytes.fromhex(result['local_to_world_bytes'])
        m.put(component + 0x200, raw[:48] + struct.pack('<f', 11) + raw[52:])
        with self.assertRaises(RuntimeError):
            q.finish()

    def test_bad_world_owner_matrix_and_plane_are_rejected(self):
        for bad in ('world', 'owner', 'size', 'plane', 'scalar', 'nonfinite', 'perspective'):
            with self.subTest(bad=bad):
                m, q, roles, actor, component = self.fixture()
                if bad == 'world':
                    m.set_value(actor['address'], 'WorldInfo', m.PAWN, '<Q')
                if bad == 'owner':
                    m.set_value(component, 'Owner', m.PAWN, '<Q')
                if bad == 'size':
                    m.put(m.MATRIX + 0x88, struct.pack('<I', 80))
                if bad == 'plane':
                    p = m.properties[m.MATRIX, 'XPlane']
                    m.put(p['address'] + 0x94, struct.pack('<I', 4))
                if bad == 'scalar':
                    p = m.properties[m.VECTOR, 'X']
                    m.put(p['address'] + 0x94, struct.pack('<I', 4))
                if bad == 'nonfinite':
                    m.put(component + 0x200, struct.pack('<f', float('nan')))
                if bad == 'perspective':
                    m.put(component + 0x20c, struct.pack('<f', .5))
                with self.assertRaises(RuntimeError):
                    _surface_state(q, roles, actor)

    def test_inherited_plane_and_structure_links_remain_closing_dependencies(self):
        for address_kind in ('super', 'structure'):
            with self.subTest(address_kind=address_kind):
                m, q, roles, actor, _ = self.fixture()
                _surface_state(q, roles, actor)
                address = (m.PLANE + m.profile['field_super'] if address_kind == 'super' else
                           m.properties[m.MATRIX, 'XPlane']['address'] + m.profile['struct_property_struct'])
                m.put(address, bytes(8))
                with self.assertRaises(RuntimeError):
                    q.finish()

    def test_rejected_ownership_observation_distinguishes_null_and_wrong_owner(self):
        for missing in (False, True):
            with self.subTest(missing_component=missing):
                m, q, roles, actor, component = self.fixture()
                if missing:
                    m.set_value(actor['address'], 'CollisionComponent', 0, '<Q')
                else:
                    m.set_value(component, 'Owner', m.PAWN, '<Q')
                with self.assertRaises(SurfaceOwnershipRejected) as caught:
                    _surface_state(q, roles, actor)
                witness = caught.exception.observation
                self.assertFalse(witness['accepted'])
                self.assertFalse(witness['collision_verified'])
                self.assertEqual(witness['actor'], actor)
                if missing:
                    self.assertIsNone(witness['collision_component'])
                    self.assertIsNone(witness['component_owner'])
                else:
                    self.assertEqual(witness['collision_component']['address'], component)
                    self.assertEqual(witness['component_owner']['address'], m.PAWN)


if __name__ == '__main__':
    unittest.main()
