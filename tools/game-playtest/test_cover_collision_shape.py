"""Production typed/closing reader against synthetic reflected memory; Game=0."""
import copy
import json
import struct
import unittest

from cover_collision_shape import capture
from cover_state import CoverQuery
from test_cover_surface_query import SurfaceMemory


class CylinderMemory(SurfaceMemory):
    CYLINDER_CLASS, CYLINDER = 0x3800000, 0x3900000

    def __init__(self):
        super().__init__()
        self.add_class(self.CYLINDER_CLASS, 'CylinderComponent', self.MESH_CLASS)
        self.add_object(self.CYLINDER, 'CurrentCylinder', self.CYLINDER_CLASS, outer=self.PAWN)
        self.add_property(self.PAWN_CLASS, 'CylinderComponent', 'ComponentProperty', 0x40, 8)
        self.add_property(self.PAWN_CLASS, 'CollisionComponent', 'ComponentProperty', 0x48, 8)
        self.add_property(self.CYLINDER_CLASS, 'CollisionRadius', 'FloatProperty', 0x300, 4)
        self.add_property(self.CYLINDER_CLASS, 'CollisionHeight', 'FloatProperty', 0x304, 4)
        self.add_property(self.CYLINDER_CLASS, 'LocalToWorld', 'StructProperty', 0x200, 64, target=self.MATRIX)
        self.put(self.MATRIX + 0x88, struct.pack('<I', 64))
        self.put(self.PLANE + 0x88, struct.pack('<I', 16))
        self.set_value(self.PAWN, 'CylinderComponent', self.CYLINDER, '<Q')
        self.set_value(self.PAWN, 'CollisionComponent', self.CYLINDER, '<Q')
        self.set_value(self.CYLINDER, 'Owner', self.PAWN, '<Q')
        self.set_value(self.CYLINDER, 'CollisionRadius', 34., '<f')
        self.set_value(self.CYLINDER, 'CollisionHeight', 80., '<f')
        self.put(self.CYLINDER + 0x200, struct.pack('<16f', 1, 0, 0, 0, 0, 1, 0, 0,
                                                0, 0, 1, 0, 10, 20, 30, 1))
        for p in self.properties.values():
            self.objects[p['address']]['cls'] = self.kind_classes[p['type']]
        for owner in {owner for owner, _ in self.properties}:
            addresses = [p['address'] for (declaring, _), p in self.properties.items() if declaring == owner]
            self.put(owner + 0x80, struct.pack('<Q', addresses[0]))
            for current, nxt in zip(addresses, addresses[1:] + [0]):
                self.put(current + 0x68, struct.pack('<Q', nxt))


class CylinderTests(unittest.TestCase):
    def result(self, fixture, accepted=True, **kwargs):
        before = copy.deepcopy(fixture.memory)
        result = capture(fixture.observer, **kwargs)
        self.assertIs(result['accepted'], accepted, result.get('rejection'))
        for name in ('engine_calls', 'body_modified', 'activation_allowed', 'collision_verified',
                     'shape_semantics_verified', 'final_body_coverage_verified',
                     'world_synchronization_verified', 'atomic_snapshot_verified', 'physical_headset_acceptance'):
            self.assertIs(result[name], False)
        self.assertEqual(result['queue_submissions'], 0)
        self.assertEqual(fixture.bridges, [])
        if fixture.before_read is None and fixture.at_verify is None:
            self.assertEqual(fixture.memory, before)
        json.dumps(result, allow_nan=False)
        return result

    def test_raw_named_fields_and_typed_transform_close_without_shape_permission(self):
        f = CylinderMemory();result = self.result(f)
        self.assertEqual(result['cylinder']['address'], f.CYLINDER)
        self.assertEqual(result['dimensions']['CollisionRadius']['value'], 34)
        self.assertEqual(result['dimensions']['CollisionHeight']['value'], 80)
        self.assertEqual(result['surface']['component'], result['cylinder'])
        self.assertEqual(len(bytes.fromhex(result['surface']['local_to_world_bytes'])), 64)
        self.assertLess(result['reads'], 16384)
        self.assertLess(result['bytes'], 2 * 1024 * 1024)
        self.assertNotIn('extent', result)
        self.assertNotIn('half_height', result)

    def test_named_and_selected_components_must_be_the_same_current_owned_cylinder(self):
        for defect in ('named_null', 'selected_null', 'selected_other', 'outer', 'owner', 'class', 'default'):
            with self.subTest(defect=defect):
                f = CylinderMemory()
                if defect == 'named_null':f.set_value(f.PAWN, 'CylinderComponent', 0, '<Q')
                elif defect == 'selected_null':f.set_value(f.PAWN, 'CollisionComponent', 0, '<Q')
                elif defect == 'selected_other':f.set_value(f.PAWN, 'CollisionComponent', f.MESH, '<Q')
                elif defect == 'outer':f.objects[f.CYLINDER]['outer'] = f.LINK
                elif defect == 'owner':f.set_value(f.CYLINDER, 'Owner', f.LINK, '<Q')
                elif defect == 'class':f.objects[f.CYLINDER]['cls'] = f.MESH_CLASS
                else:f.objects[f.CYLINDER]['name'] = 'Default__CylinderComponent'
                self.result(f, False)

    def test_missing_nonfinite_zero_negative_and_oversized_dimensions_do_not_get_defaults(self):
        for name in ('CollisionRadius', 'CollisionHeight'):
            for value in (0, -1, 301, float('nan'), float('inf')):
                with self.subTest(name=name, value=value):
                    f = CylinderMemory();f.set_value(f.CYLINDER, name, value, '<f')
                    result = self.result(f, False)
                    self.assertNotIn('dimensions', result)
            f = CylinderMemory();del f.properties[f.CYLINDER_CLASS, name]
            self.result(f, False)

    def test_dimension_type_width_dimension_and_property_membership_are_authenticated(self):
        for defect in ('kind', 'width', 'dim', 'detached', 'alias', 'transform_overlap'):
            with self.subTest(defect=defect):
                f = CylinderMemory();p = f.properties[f.CYLINDER_CLASS, 'CollisionRadius']
                if defect == 'kind':p['type'] = 'IntProperty'
                elif defect == 'width':f.put(p['address'] + 0x74, struct.pack('<I', 8))
                elif defect == 'dim':f.put(p['address'] + 0x70, struct.pack('<I', 2))
                elif defect == 'detached':f.put(f.CYLINDER_CLASS + 0x80, bytes(8))
                else:
                    p = f.properties[f.CYLINDER_CLASS, 'CollisionHeight']
                    p['offset'] = 0x300 if defect == 'alias' else 0x200
                    f.put(p['address'] + 0x94, struct.pack('<I', p['offset']))
                self.result(f, False)

    def test_invalid_transform_or_schema_cannot_certify_dimension_binding(self):
        for defect in ('matrix_size', 'nan', 'non_affine'):
            f = CylinderMemory()
            if defect == 'matrix_size':f.put(f.MATRIX + 0x88, struct.pack('<I', 60))
            else:f.put(f.CYLINDER + 0x200 + (0 if defect == 'nan' else 12), struct.pack('<f', float('nan') if defect == 'nan' else 1))
            self.result(f, False)

    def test_closing_dimension_transform_and_component_changes_are_rejected(self):
        for defect in ('radius', 'transform', 'selected', 'owner'):
            with self.subTest(defect=defect):
                f = CylinderMemory()
                def change(call):
                    if call != 2:return
                    if defect == 'radius':f.set_value(f.CYLINDER, 'CollisionRadius', 35, '<f')
                    elif defect == 'transform':f.put(f.CYLINDER + 0x200 + 48, struct.pack('<f', 11))
                    elif defect == 'selected':f.set_value(f.PAWN, 'CollisionComponent', f.MESH, '<Q')
                    else:f.set_value(f.CYLINDER, 'Owner', f.LINK, '<Q')
                f.at_verify = change
                self.result(f, False)

    def test_closing_registry_witness_is_preserved(self):
        f = CylinderMemory();queries = []
        def factory(observer):
            q = CoverQuery(observer);queries.append(q);return q
        def change(call):
            if call == 2:f.put(f.names_header + 8, struct.pack('<i', 101))
        f.at_verify = change
        result = self.result(f, False, query_factory=factory)
        self.assertEqual(result['closing_rejection']['kind'], 'registry')
        self.assertEqual(result['closing_rejection'], queries[0].closing_rejection)

    def test_actual_session_and_game_owner_must_remain_exact(self):
        for key in ('game_identity', 'owner_identity'):
            f = CylinderMemory()
            def change(call):
                if call == 3:f.observer.identities[key]['pid'] += 1
            f.at_verify = change
            self.result(f, False)


if __name__ == '__main__':
    unittest.main()
