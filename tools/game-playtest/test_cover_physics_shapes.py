"""Production shape reader with typed synthetic memory; no game or engine calls."""
import copy
import json
import math
import struct
import unittest

from cover_state import CoverQuery
from cover_physics_shapes import read_setup_shapes, ShapeLayoutRejected
from test_cover_physics_state import PhysicsMemory


class ShapeMemory(PhysicsMemory):
    KM, AGG, SPHERE, BOX_ELEM, SPHYL, CONVEX, BOUNDS, VEC4 = range(0x5800000,0x5808000,0x1000)

    def __init__(self, count=1):
        super().__init__(count)
        self.add_class(self.KM,'KMeshProps',0x20400)
        self.supers[self.BS_CLASS]=self.KM
        self.put(self.BS_CLASS+0x60,struct.pack('<Q',self.KM))
        for at,name,size in [(self.AGG,'KAggregateGeom',72),(self.SPHERE,'KSphereElem',72),
                             (self.BOX_ELEM,'KBoxElem',80),(self.SPHYL,'KSphylElem',76),
                             (self.CONVEX,'KConvexElem',128),(self.BOUNDS,'Box',28),(self.VEC4,'Vector4',16)]:
            self.add_object(at,name,self.STRUCT_CLASS,outer=self.KM if at in (self.AGG,self.SPHERE,self.BOX_ELEM,self.SPHYL,self.CONVEX) else 0)
            self.put(at+0x60,bytes(8));self.put(at+0x88,struct.pack('<I',size))
        for at,size in [(self.VECTOR,12),(self.PLANE,16),(self.MATRIX,64)]:
            self.put(at+0x88,struct.pack('<I',size))
        self.add_property(self.KM,'AggGeom','StructProperty',0x60,72,target=self.AGG)
        self.add_property(self.AGG,'RenderInfo','StructProperty',64,8,target=self.POINTER)
        for at,dims in [(self.SPHERE,('Radius',)),(self.BOX_ELEM,('X','Y','Z')),
                        (self.SPHYL,('Radius','Length'))]:
            self.add_property(at,'TM','StructProperty',0,64,target=self.MATRIX)
            for i,name in enumerate(dims):self.add_property(at,name,'FloatProperty',64+i*4,4)
            self.add_property(at,'bNoRBCollision','BoolProperty',64+len(dims)*4,4,mask=4)
        for i,name in enumerate(('X','Y','Z','W')):self.add_property(self.VEC4,name,'FloatProperty',i*4,4)
        self.add_property(self.BOUNDS,'Min','StructProperty',0,12,target=self.VECTOR)
        self.add_property(self.BOUNDS,'Max','StructProperty',12,12,target=self.VECTOR)
        self.add_property(self.BOUNDS,'IsValid','ByteProperty',24,1)
        self.next_shape_data=0x6000000
        self.arrays_shape={}
        agg=self.setups[0]+0x60
        self.put(agg+64,struct.pack('<Q',0xfedcba9876543210))
        matrix=struct.pack('<16f',1,0,0,0,0,1,0,0,0,0,1,0,2,3,4,1)
        rows=[('SphereElems',self.SPHERE,matrix+struct.pack('<fI',2,0)),
              ('BoxElems',self.BOX_ELEM,matrix+struct.pack('<3fI',3,4,5,4)),
              ('SphylElems',self.SPHYL,matrix+struct.pack('<2fI',2,0,0)),
              ('ConvexElems',self.CONVEX,bytes(128))]
        for i,(name,typ,raw) in enumerate(rows):
            self.shape_array(self.AGG,agg,name,i*16,typ,len(raw),raw)
        convex=self.arrays_shape[self.AGG,'ConvexElems']['data']
        specs=[('VertexData',self.VECTOR,12,struct.pack('<12f',0,0,0,1,0,0,0,1,0,0,0,1)),
               ('PermutedVertexData',self.VEC4,16,struct.pack('<4f',1,2,3,4)),
               ('FaceTriData',None,4,struct.pack('<6i',0,1,2,0,2,3)),
               ('EdgeDirections',self.VECTOR,12,struct.pack('<3f',1,0,0)),
               ('FaceNormalDirections',self.VECTOR,12,struct.pack('<3f',0,0,1)),
               ('FacePlaneData',self.PLANE,16,struct.pack('<4f',0,0,1,1))]
        for i,(name,typ,stride,raw) in enumerate(specs):
            self.shape_array(self.CONVEX,convex,name,i*16,typ,stride,raw)
        self.add_property(self.CONVEX,'ElemBox','StructProperty',96,28,target=self.BOUNDS)
        self.put(convex+96,struct.pack('<6fB3x',0,0,0,1,1,1,1))
        for setup in self.setups[1:]:
            self.put(setup+0x60,self.read(agg,72))
        self.relink()

    def shape_array(self,owner,base,name,offset,typ,stride,raw):
        self.add_property(owner,name,'ArrayProperty',offset,16)
        field=self.properties[owner,name];inner=field['address']+0x100
        self.add_object(inner,name,self.kind_classes['StructProperty' if typ else 'IntProperty'],outer=field['address'])
        metadata=bytearray(0x28);struct.pack_into('<II',metadata,0,1,stride)
        self.put(inner+0x70,metadata)
        self.put(inner+0xd0,struct.pack('<Q',typ or 0));self.put(field['address']+0xd0,struct.pack('<Q',inner))
        assert len(raw)%stride==0
        data=self.next_shape_data;self.next_shape_data+=0x10000
        self.put(base+offset,struct.pack('<Qii',data,len(raw)//stride,len(raw)//stride))
        self.put(data,raw)
        self.arrays_shape[owner,name]=dict(header=base+offset,data=data,inner=inner,field=field)

    def relink(self):
        for p in self.properties.values():
            self.objects[p['address']]['cls']=self.kind_classes[p['type']]
            self.put(p['address']+0xd0,bytes(self.memory.get(p['address']+0xd0+i,0) for i in range(8)))
        for owner in {owner for owner,_ in self.properties}:
            fields=self.functions.get(owner,[])+[p['address'] for (o,_),p in self.properties.items() if o==owner]
            self.put(owner+0x80,struct.pack('<Q',fields[0]))
            for current,nxt in zip(fields,fields[1:]+[0]):self.put(current+0x68,struct.pack('<Q',nxt))

    def capture_shapes(self):
        q=CoverQuery(self.observer)
        out=read_setup_shapes(q,q.obj(self.setups[0]))
        q.finish()
        return out,q


class ShapeTests(unittest.TestCase):
    def test_render_info_is_observed_as_opaque_native_pointer_without_dereference(self):
        f = ShapeMemory()
        out, q = f.capture_shapes()
        self.assertEqual(out['render_info']['value'], 0xfedcba9876543210)
        self.assertEqual(out['render_info']['bytes'], '1032547698badcfe')
        self.assertFalse(out['render_info']['dereferenced'])
        self.assertFalse(any(at == 0xfedcba9876543210 for at, _ in q.values))
        self.assertFalse(out['activation_allowed'])

    def test_missing_extra_or_incompatible_render_field_rejects_with_layout_witness(self):
        for defect in ('missing', 'extra', 'target'):
            with self.subTest(defect=defect):
                f = ShapeMemory()
                if defect == 'missing':
                    del f.properties[f.AGG, 'RenderInfo']
                elif defect == 'extra':
                    f.add_property(f.AGG, 'Unknown', 'IntProperty', 68, 4)
                else:
                    field = f.properties[f.AGG, 'RenderInfo']
                    f.put(field['address'] + 0xd0, struct.pack('<Q', f.VECTOR))
                f.relink()
                out = f.capture_physics(include_shapes=True)
                self.assertFalse(out['accepted'])
                witness = out['source_rejection_witness']
                self.assertFalse(witness['closing_verified'])
                self.assertFalse(witness['activation_allowed'])
                if defect != 'target':
                    self.assertEqual(witness['kind'], 'physics_shape_structure_layout_mismatch')
                    self.assertIn('RenderInfo', witness['expected_field_names'])
                    self.assertEqual(len(witness['declared_fields']), 4 if defect == 'missing' else 6)

    def test_render_info_change_at_closing_rejects_instead_of_dropping_native_field(self):
        f = ShapeMemory()
        target = f.setups[0] + 0x60 + 64
        def change(address, length, count):
            if (address, length, count) == (target, 8, 2):
                f.put(target, bytes(8))
        f.before_read = change
        out = f.capture_physics(include_shapes=True)
        self.assertFalse(out['accepted'])
        self.assertFalse(out['authored_shapes_observed'])

    def test_all_authored_shape_kinds_and_inherited_plane_fields_are_read_without_permission(self):
        f=ShapeMemory();before=copy.deepcopy(f.memory);out,q=f.capture_shapes()
        self.assertEqual([r['structure']['name'] for r in out['shapes']],['KSphereElem','KBoxElem','KSphylElem','KConvexElem'])
        self.assertEqual(out['shapes'][0]['dimensions'],{'Radius':2})
        self.assertEqual(out['shapes'][0]['TM'][3],[2,3,4,1])
        self.assertTrue(out['shapes'][1]['bNoRBCollision'])
        self.assertEqual(out['shapes'][2]['dimensions']['Length'],0)
        self.assertEqual(len(out['shapes'][3]['arrays']['VertexData']['values']),4)
        self.assertEqual(out['shapes'][3]['arrays']['FaceTriData']['values'],[0,1,2,0,2,3])
        self.assertTrue(out['closing_required'])
        for key in ['active_physics_verified','world_pose_verified','collision_volume_verified','sweep_verified','occupancy_verified','activation_allowed']:
            self.assertIs(out[key],False)
        self.assertEqual(f.memory,before);self.assertEqual(f.bridges,[])
        self.assertLess(q.reads,16384);json.dumps(out,allow_nan=False)

    def test_empty_aggregate_arrays_are_absence_not_an_invented_body_volume(self):
        f=ShapeMemory()
        for name in ('SphereElems','BoxElems','SphylElems','ConvexElems'):
            f.put(f.arrays_shape[f.AGG,name]['header'],bytes(16))
        out,_=f.capture_shapes();self.assertEqual(out['shapes'],[])

    def test_nonfinite_dimensions_matrix_and_convex_vertices_reject(self):
        for defect in ['radius','length','matrix','vertex']:
            with self.subTest(defect=defect):
                f=ShapeMemory()
                if defect=='radius':at=f.arrays_shape[f.AGG,'SphereElems']['data']+64;value=0.
                elif defect=='length':at=f.arrays_shape[f.AGG,'SphylElems']['data']+68;value=-1.
                elif defect=='matrix':at=f.arrays_shape[f.AGG,'BoxElems']['data'];value=math.inf
                else:at=f.arrays_shape[f.CONVEX,'VertexData']['data'];value=math.nan
                f.put(at,struct.pack('<f',value))
                with self.assertRaises(RuntimeError):f.capture_shapes()

    def test_array_schema_bounds_inner_class_and_target_extent_reject(self):
        for defect in ['count','overflow','stride','inner','target','owner']:
            with self.subTest(defect=defect):
                f=ShapeMemory();array=f.arrays_shape[f.AGG,'SphereElems']
                if defect=='count':f.put(array['header'],struct.pack('<Qii',array['data'],65,65))
                elif defect=='overflow':f.put(array['header'],struct.pack('<Qii',(1<<64)-4,1,1))
                elif defect=='stride':f.put(array['inner']+0x74,struct.pack('<I',64))
                elif defect=='inner':f.objects[array['inner']]['cls']=f.kind_classes['IntProperty']
                elif defect=='target':f.put(array['inner']+0xd0,struct.pack('<Q',f.BOX_ELEM))
                else:f.objects[array['inner']]['outer']=f.AGG
                with self.assertRaises(RuntimeError):f.capture_shapes()

    def test_array_target_failure_preserves_only_already_read_declarations(self):
        for defect in ('stride', 'name', 'number', 'class'):
            with self.subTest(defect=defect):
                f = ShapeMemory()
                array = f.arrays_shape[f.AGG, 'SphereElems']
                if defect == 'stride':
                    f.put(array['inner'] + 0x74, struct.pack('<I', 64))
                elif defect == 'name':
                    f.put(array['inner'] + 0xd0, struct.pack('<Q', f.BOX_ELEM))
                elif defect == 'number':
                    f.objects[f.SPHERE]['number'] = 1
                else:
                    f.objects[f.SPHERE]['cls'] = f.kind_classes['StructProperty']
                q = CoverQuery(f.observer)
                with self.assertRaises(ShapeLayoutRejected) as raised:
                    read_setup_shapes(q, q.obj(f.setups[0]))
                reads, byte_count = q.reads, q.bytes
                witness = raised.exception.source_witness
                self.assertEqual(witness['kind'], 'physics_shape_array_target_extent_mismatch')
                self.assertEqual(witness['field']['name'], 'SphereElems')
                self.assertEqual(witness['expected_structure_names'], ['KSphereElem'])
                self.assertEqual(witness['inner']['address'], array['inner'])
                self.assertEqual(witness['count'], 1)
                self.assertEqual(witness['additional_process_reads'], 0)
                self.assertFalse(witness['closing_verified'])
                self.assertFalse(witness['source_accepted'])
                self.assertFalse(witness['activation_allowed'])
                self.assertEqual(witness['stride'], 64 if defect == 'stride' else 72)
                self.assertEqual(witness['target_extent_observed'], 72 if defect == 'stride' else None)
                target_at = f.BOX_ELEM if defect == 'name' else f.SPHERE
                self.assertEqual((target_at + 0x88, 4) in q.values, defect == 'stride')
                self.assertEqual(witness['target_kind_observed'],
                                 'ScriptStruct' if defect == 'stride' else 'StructProperty' if defect == 'class' else None)
                json.dumps(witness, allow_nan=False)
                self.assertEqual((q.reads, q.bytes), (reads, byte_count))

    def test_array_target_failure_survives_combined_capture_without_acceptance(self):
        f = ShapeMemory()
        array = f.arrays_shape[f.AGG, 'SphereElems']
        f.put(array['inner'] + 0x74, struct.pack('<I', 64))
        out = f.capture_physics(include_shapes=True)
        self.assertFalse(out['accepted'])
        self.assertEqual(out['rejection_type'], 'ShapeLayoutRejected')
        self.assertEqual(out['source_rejection_witness']['target_extent_observed'], 72)
        for key in ('authored_shapes_observed', 'physical_pose_verified', 'activation_allowed'):
            self.assertFalse(out[key])

    def test_complete_field_sets_overlap_and_boolean_mask_reject(self):
        for defect in ['missing','extra','overlap','mask','foreign_setup']:
            with self.subTest(defect=defect):
                f=ShapeMemory()
                if defect=='missing':f.put(f.SPHERE+0x80,bytes(8))
                elif defect=='extra':f.add_property(f.SPHERE,'UnknownShapeField','FloatProperty',72,4);f.relink()
                elif defect=='overlap':f.put(f.properties[f.SPHERE,'Radius']['address']+0x94,struct.pack('<I',4))
                elif defect=='mask':f.put(f.properties[f.SPHERE,'bNoRBCollision']['address']+0xd0,struct.pack('<I',3))
                else:f.objects[f.setups[0]]['cls']=f.PA_CLASS
                with self.assertRaises(RuntimeError):f.capture_shapes()

    def test_current_query_closes_array_payload_and_metadata_changes(self):
        for defect in ['payload','header','metadata','registry']:
            with self.subTest(defect=defect):
                f=ShapeMemory()
                def change(call):
                    if call!=1:return
                    if defect=='payload':f.put(f.arrays_shape[f.AGG,'SphereElems']['data']+64,struct.pack('<f',8))
                    elif defect=='header':f.put(f.arrays_shape[f.CONVEX,'VertexData']['header']+8,struct.pack('<i',3))
                    elif defect=='metadata':f.put(f.properties[f.SPHYL,'Length']['address']+0x94,struct.pack('<I',64))
                    else:f.put(f.names_header+8,struct.pack('<i',101))
                f.at_verify=change
                with self.assertRaises(RuntimeError):f.capture_shapes()

    def test_full_shape_cap_is_read_and_combined_excess_is_not_truncated(self):
        f=ShapeMemory();sphere=f.arrays_shape[f.AGG,'SphereElems']
        raw=f.read(sphere['data'],72)
        f.put(sphere['data'],raw*64);f.put(sphere['header'],struct.pack('<Qii',sphere['data'],64,64))
        for name in ('BoxElems','SphylElems','ConvexElems'):
            f.put(f.arrays_shape[f.AGG,name]['header'],bytes(16))
        out,q=f.capture_shapes();self.assertEqual(len(out['shapes']),64);self.assertLess(q.reads,16384)
        box=f.arrays_shape[f.AGG,'BoxElems'];f.put(box['header'],struct.pack('<Qii',box['data'],1,1))
        with self.assertRaisesRegex(RuntimeError,'aggregate shape count'):f.capture_shapes()

    def test_exact_aggregate_owners_and_original_budget_are_required(self):
        for defect in ('class','aggregate','element'):
            f=ShapeMemory()
            if defect=='class':f.objects[f.KM]['name']='UnrelatedMeshProps'
            elif defect=='aggregate':f.objects[f.AGG]['outer']=f.BS_CLASS
            else:f.objects[f.SPHYL]['outer']=f.BS_CLASS
            with self.assertRaises(RuntimeError):f.capture_shapes()
        f=ShapeMemory();q=CoverQuery(f.observer);q.reads=16384
        with self.assertRaisesRegex(RuntimeError,'read budget'):
            read_setup_shapes(q,q.obj(f.setups[0]))

    def test_shared_layout_reader_requires_the_same_query(self):
        from cover_physics_shapes import ShapeReader
        f=ShapeMemory()
        first,second=CoverQuery(f.observer),CoverQuery(f.observer)
        with self.assertRaisesRegex(RuntimeError,'another query'):
            read_setup_shapes(second,second.obj(f.setups[0]),reader=ShapeReader(first))


if __name__=='__main__':unittest.main()
