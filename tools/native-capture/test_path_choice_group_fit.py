import copy
import struct
import unittest
import path_choice_group_fit as group
import path_choice_position_config as position
from test_path_choice_position_config import fixture, SESSION


def viewport_fixture(sample, dimensions=(1920.,1080.)):
    scene=copy.deepcopy(sample['scene_owner']['roles']['scene'])
    owner=next(c['identity'] for c in sample['scene_owner']['classes'] if c['identity']['name']=='UIScene')
    field=dict(address=0xeb0000,name='CurrentViewportSize',number=0,outer=owner['address'])
    structure=dict(address=0xea0000,name='Vector2D')
    members={name:dict(identity=dict(address=0xec0000+i*0x1000,name=name,number=0,outer=structure['address']),
        type='FloatProperty',offset=i*4,array_dim=1,element_size=4) for i,name in enumerate(('X','Y'))}
    checks=[]
    def check(address,raw):checks.append(dict(address=address,raw=raw.hex()))
    check(field['address']+0x70,struct.pack('<II',1,8));check(field['address']+148,struct.pack('<I',0x400))
    check(field['address']+208,struct.pack('<Q',structure['address']))
    for member in members.values():
        check(member['identity']['address']+0x70,struct.pack('<II',1,4))
        check(member['identity']['address']+148,struct.pack('<I',member['offset']))
    raw=struct.pack('<2f',*dimensions);address=scene['address']+0x400;check(address,raw)
    return dict(schema=1,kind='reflected_uiscene_viewport_domain',read_only=True,consistent_reread=True,
        scene=scene,property=dict(identity=field,owner=owner,type='StructProperty',offset=0x400,array_dim=1,element_size=8),
        struct_type=structure,members=members,value_address=address,raw=raw.hex(),
        values_xy=list(struct.unpack('<2f',raw)),source_checks=checks)


def sample_fixture(dimensions=(1920.,1080.)):
    sample,proof=fixture();root,panel=sample['tree'][0],sample['tree'][1]
    def bounds(values):
        values=list(struct.unpack('<4f',struct.pack('<4f',*values)))
        return dict(order=['left','top','right','bottom'],values=values,ordered_values=values,
            raw=struct.pack('<4f',*values).hex(),reflected=dict(type='FloatProperty',offset=676,array_dim=4,element_size=4))
    faces=panel['layout_details']['position']['members']['Value']['values']
    panel['render_bounds']=bounds([v*dimensions[i%2] for i,v in enumerate(faces)])
    children=[dict(address=hex(0xd00000+i*0x1000),name='child'+str(i),cls='UILabel',parent=panel['address'],
        outer=panel['address'],depth=3,render_bounds=bounds([(550+i)*dimensions[0]/1920,(70+i)*dimensions[1]/1080,
            (1370-i)*dimensions[0]/1920,(260-i)*dimensions[1]/1080])) for i in range(10)]
    extra=dict(address=hex(0xb00000),name='decoration',cls='UIImage',parent=root['address'],outer=root['address'],depth=2)
    sample['tree']=[root,panel,*children,extra]
    sample['viewport_domain']=viewport_fixture(sample,dimensions)
    proof['checks'].extend(copy.deepcopy(sample['viewport_domain']['source_checks']))
    for row in [panel,*children]:proof['checks'].append(dict(address=int(row['address'],0)+676,raw=row['render_bounds']['raw']))
    return sample,proof


class FitTests(unittest.TestCase):
    def test_opt_in_and_complete_subtree(self):
        sample,proof=sample_fixture();before=copy.deepcopy((sample,proof))
        fields,reads=group.inputs(sample,sample['tree'][1],(.25,.75),position.require)
        self.assertEqual(len(reads),11+len(sample['viewport_domain']['source_checks']));self.assertEqual(fields['group_fit'],'vertical_v1')
        self.assertAlmostEqual(struct.unpack('<f',struct.pack('<I',fields['fit_top_bits']))[0],70/1080,places=6)
        self.assertNotIn('group_fit=',position.build(sample,proof,**SESSION))
        fit=position.build(sample,proof,**SESSION,target_vertical_region=(.25,.75))
        self.assertIn('group_fit=vertical_v1\n',fit);self.assertNotIn('vertical_offset_reference_pixels=',fit)
        self.assertEqual((sample,proof),before)
        self.assertEqual(len(position.required_reads(sample,proof,**SESSION,target_vertical_region=(.25,.75))),
            len(position.required_reads(sample,proof,**SESSION))+11+len(sample['viewport_domain']['source_checks']))

    def test_independently_reflected_multiple_domains_have_same_normalized_group(self):
        recipes=[]
        for dimensions in ((1920.,1080.),(3840.,2160.),(1280.,1400.)):
            sample,proof=sample_fixture(dimensions)
            recipes.append(group.inputs(sample,sample['tree'][1],(.25,.75),position.require)[0])
            ini=position.build(sample,proof,**SESSION,target_vertical_region=(.25,.75))
            domain=sample['viewport_domain']
            self.assertIn('0x%X:%s'%(domain['value_address'],domain['raw']),ini)
        self.assertEqual(recipes[0],recipes[1])
        for key in recipes[0]:
            if key=='group_fit':self.assertEqual(recipes[0][key],recipes[2][key])
            else:self.assertAlmostEqual(struct.unpack('<f',struct.pack('<I',recipes[0][key]))[0],
                struct.unpack('<f',struct.pack('<I',recipes[2][key]))[0],places=6)

    def test_no_inferred_or_fallback_domain_and_changed_domain_or_metadata_rejects(self):
        for fault in ('missing','value','offset','struct','member','scene','coverage','zero'):
            sample,proof=sample_fixture((3840.,2160.));v=sample['viewport_domain']
            if fault=='missing':del sample['viewport_domain']
            elif fault=='value':v['values_xy'][0]=1920.
            elif fault=='offset':v['property']['offset']+=4
            elif fault=='struct':v['struct_type']['address']+=8
            elif fault=='member':v['members']['Y']['offset']=0
            elif fault=='scene':v['scene']['address']+=8
            elif fault=='coverage':v['source_checks'].pop()
            else:v['raw']=struct.pack('<2f',0,0).hex();v['values_xy']=[0.,0.]
            with self.subTest(fault=fault),self.assertRaises(ValueError):
                position.build(sample,proof,**SESSION,target_vertical_region=(.25,.75))

    def test_reflected_domain_must_also_match_all_four_panel_faces(self):
        sample,proof=sample_fixture((3840.,2160.))
        # Complete independent 1920x1080 evidence cannot certify 4K panel faces.
        sample['viewport_domain']=viewport_fixture(sample,(1920.,1080.))
        with self.assertRaisesRegex(ValueError,'panel faces disagree'):
            group.inputs(sample,sample['tree'][1],(.25,.75),position.require)

    def test_graph_not_names_or_decoration(self):
        sample,_=sample_fixture();panel=sample['tree'][1]
        expected=group.inputs(sample,panel,(.25,.75),position.require)
        sample['tree'][-1]['render_bounds']={'unrelated':'outside panel'}
        for row in sample['tree'][2:]:row['name']='renamed'
        self.assertEqual(group.inputs(sample,panel,(.25,.75),position.require),expected)
        sample['tree'][2]['parent']=sample['top']['address']
        with self.assertRaises(ValueError):group.inputs(sample,panel,(.25,.75),position.require)

    def test_offset_panel_is_not_a_viewport_extent(self):
        sample,_=sample_fixture();panel=sample['tree'][1]
        # Keep the centered face sums unchanged while shrinking its panel.
        b=panel['render_bounds'];b['values'][0]+=5;b['values'][2]-=5
        b['ordered_values']=list(b['values']);b['raw']=struct.pack('<4f',*b['values']).hex()
        with self.assertRaises(ValueError):group.inputs(sample,panel,(.25,.75),position.require)

    def test_reject_scaling_stale_bounds_and_mixed_policy(self):
        sample,proof=sample_fixture()
        for target in ((.49,.51),(.1,.2),(float('nan'),.8),(-1,.8),(.5,1.1)):
            with self.subTest(target=target):
                with self.assertRaises(ValueError):position.build(sample,proof,**SESSION,target_vertical_region=target)
        with self.assertRaises(ValueError):position.build(sample,proof,**SESSION,target_vertical_region=(.25,.75),vertical_offset_reference_pixels=374)
        proof['checks'].pop()
        with self.assertRaises(ValueError):position.build(sample,proof,**SESSION,target_vertical_region=(.25,.75))


if __name__=='__main__':unittest.main()
