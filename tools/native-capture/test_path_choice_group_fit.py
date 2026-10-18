import copy
import struct
import unittest
import path_choice_group_fit as group
import path_choice_position_config as position
from test_path_choice_position_config import fixture, SESSION


def sample_fixture():
    sample,proof=fixture();root,panel=sample['tree'][0],sample['tree'][1]
    def bounds(values):
        values=list(struct.unpack('<4f',struct.pack('<4f',*values)))
        return dict(order=['left','top','right','bottom'],values=values,ordered_values=values,
            raw=struct.pack('<4f',*values).hex(),reflected=dict(type='FloatProperty',offset=676,array_dim=4,element_size=4))
    faces=panel['layout_details']['position']['members']['Value']['values']
    panel['render_bounds']=bounds([v*(1920 if i%2==0 else 1080) for i,v in enumerate(faces)])
    children=[dict(address=hex(0xd00000+i*0x1000),name='child'+str(i),cls='UILabel',parent=panel['address'],
        outer=panel['address'],depth=3,render_bounds=bounds([550+i,70+i,1370-i,260-i])) for i in range(10)]
    extra=dict(address=hex(0xb00000),name='decoration',cls='UIImage',parent=root['address'],outer=root['address'],depth=2)
    sample['tree']=[root,panel,*children,extra]
    for row in [panel,*children]:proof['checks'].append(dict(address=int(row['address'],0)+676,raw=row['render_bounds']['raw']))
    return sample,proof


class FitTests(unittest.TestCase):
    def test_opt_in_and_complete_subtree(self):
        sample,proof=sample_fixture();before=copy.deepcopy((sample,proof))
        fields,reads=group.inputs(sample,sample['tree'][1],(.25,.75),position.require)
        self.assertEqual(len(reads),11);self.assertEqual(fields['group_fit'],'vertical_v1')
        self.assertAlmostEqual(struct.unpack('<f',struct.pack('<I',fields['fit_top_bits']))[0],70/1080,places=6)
        self.assertNotIn('group_fit=',position.build(sample,proof,**SESSION))
        fit=position.build(sample,proof,**SESSION,target_vertical_region=(.25,.75))
        self.assertIn('group_fit=vertical_v1\n',fit);self.assertNotIn('vertical_offset_reference_pixels=',fit)
        self.assertEqual((sample,proof),before)
        self.assertEqual(len(position.required_reads(sample,proof,**SESSION,target_vertical_region=(.25,.75))),
            len(position.required_reads(sample,proof,**SESSION))+11)

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
