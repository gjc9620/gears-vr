"""Offline evidence validation and resolution-independent PathChoice centering."""
import copy
import json
from pathlib import Path
import struct
import unittest

from path_choice_layout import CONTENT, GAME_SHA, LINKS, SAFE, centered_translation


def fixture(scale=1):
    meta = 0x100000
    parents = {'Object':None, 'Class':'Object', 'UIScene':'Object',
        'UIScene_PathChoice':'UIScene', 'GameUISceneClient':'Object', 'Player':'Object',
        'LocalPlayer':'Player', 'PlayerController':'Object', 'UIInteraction':'Object',
        'GameViewportClient':'Object', 'Engine':'Object', 'UIObject':'Object',
        SAFE:'UIObject', 'UIImage':'UIObject', 'UILabel':'UIObject'}
    addresses = {name:meta+0x100*(index+1) for index,name in enumerate(parents)}
    addresses['Class'] = meta
    def identity(address, name, cls, outer=0, number=0):
        return dict(address=address,name=name,number=number,cls=cls,outer=outer,internal_index=address//16)
    classes = {name:identity(address,name,meta) for name,address in addresses.items()}
    class_rows = [dict(identity=value,super=addresses[parents[name]] if parents[name] else 0)
                  for name,value in classes.items()]
    role_classes = dict(client='GameUISceneClient',scene='UIScene_PathChoice',controller='PlayerController',
        local_player='LocalPlayer',interaction='UIInteraction',viewport='GameViewportClient',engine='Engine')
    roles = {role:identity(0x200000+index*0x1000,'DeltaChoosePath' if role=='scene' else role,
                          addresses[cls],number=1 if role=='scene' else 0)
             for index,(role,cls) in enumerate(role_classes.items())}
    for role,parent in [('client','interaction'),('interaction','viewport'),('viewport','engine'),
                        ('local_player','engine'),('scene','client')]:
        roles[role]['outer'] = roles[parent]['address']
    links = {label:dict(object=roles[role]['address'],property=name,
        value=roles[target or 'local_player']['address'])
        for label,(role,name,declaring,target) in LINKS.items()}
    scene = roles['scene']
    owner = dict(schema=1,kind='scene_owner_cpu_snapshot',pid=123,read_only=True,consistent_reread=True,
        profile_game_sha256=GAME_SHA,evidence=dict(method='fresh_relevant_fields_and_registry_slots_with_final_reread'),
        identities=list(classes.values())+list(roles.values()),classes=class_rows,roles=roles,links=links,
        owner_classification='local_player',classification_basis='explicit_local_player',
        arrays={name:dict(members=members,count=len(members),capacity=len(members)) for name,members in
            [('game_players',[roles['local_player']['address']]),('active_scenes',[scene['address']])]})
    def proof(cls, base):
        chain=[];name=cls
        while name:
            chain.append(dict(identity=copy.deepcopy(classes[name]),metaclass=copy.deepcopy(classes['Class']),
                              super=addresses[parents[name]] if parents[name] else 0))
            name=parents[name]
        return dict(class_chain=chain,base=dict(identity=copy.deepcopy(classes[base]),
            package=dict(name='Engine',number=0,outer=0),package_type=dict(name='Package',number=0,cls=meta)))
    def bounds(cls, values):
        raw=struct.pack('<4f',*[v*scale for v in values]);v=list(struct.unpack('<4f',raw))
        return dict(order=['left','top','right','bottom'],values=v,raw=raw.hex(),
            ordered_values=[min(v[0],v[2]),min(v[1],v[3]),max(v[0],v[2]),max(v[1],v[3])],
            x_reversed=v[0]>v[2],y_reversed=v[1]>v[3],class_ancestry=proof(cls,'UIObject'),
            reflected=dict(property_address='0x900000',type='FloatProperty',offset=676,array_dim=4,element_size=4,
                           array_layout_raw=struct.pack('<II',4,4).hex()))
    def node(address,name,cls,parent,depth,rectangle):
        return dict(address=hex(address),name=name,cls=cls,number=0,parent=hex(parent),outer=hex(parent),
                    depth=depth,render_bounds=bounds(cls,rectangle))
    root=dict(address=hex(scene['address']),name='DeltaChoosePath',cls='UIScene_PathChoice',number=1,
        outer=hex(scene['outer']),parent=None,depth=1,render_bounds=None,
        render_bounds_absence=dict(proof=proof('UIScene_PathChoice','UIScene')))
    safe_address=0x300000
    tree=[root,node(safe_address,SAFE,SAFE,scene['address'],2,[0,0,1000,800])]
    rectangles=[[200,50,800,100],[200,100,500,250],[500,100,800,250],[210,55,790,95],
                [250,130,490,180],[510,130,750,180],[240,200,490,240],[510,200,760,240],
                [220,120,250,220],[780,120,750,220]]
    for index,((name,cls),rectangle) in enumerate(zip(CONTENT.items(),rectangles)):
        tree.append(node(0x301000+index*0x1000,name,cls,safe_address,3,rectangle))
    # A real, validated root-level image must not enlarge the selected content union.
    tree.append(node(0x400000,'UIImage','UIImage',scene['address'],2,[0,0,1000,800]))
    return dict(pid=123,client=hex(roles['client']['address']),read_only=True,consistent_reread=True,
        input_block_count=0,layout_evidence='consistent CPU layout snapshot; not actual draw/scissor proof',
        top=dict(address=hex(scene['address']),name='DeltaChoosePath',cls='UIScene_PathChoice',number=1,
                 outer=hex(scene['outer']),tag='DeltaChoosePath',tag_number=0),
        active_scenes=[dict(address=hex(scene['address']))],scene_owner=owner,tree=tree)


class CenteredTranslationTests(unittest.TestCase):
    def test_resolution_scaling_and_excluded_image(self):
        for scale,expected in [(0.5,(0,125)),(1,(0,250)),(1.5,(0,375))]:
            with self.subTest(scale=scale):
                self.assertEqual(centered_translation(fixture(scale)),expected)

    def test_directed_arrow_is_not_reinterpreted(self):
        sample=fixture()
        arrow=next(n for n in sample['tree'] if n['name']=='imgRightArrow')
        self.assertTrue(arrow['render_bounds']['x_reversed'])
        self.assertEqual(centered_translation(sample),(0,250))
        arrow['render_bounds']['x_reversed']=False
        with self.assertRaises(ValueError):centered_translation(sample)

    def test_reject_duplicate_missing_and_parent_graph(self):
        for mutation in ('duplicate','missing','parent','outer','depth','unknown'):
            sample=fixture()
            if mutation=='duplicate':sample['tree'].append(copy.deepcopy(sample['tree'][2]))
            elif mutation=='missing':sample['tree'].pop(2)
            elif mutation=='parent':sample['tree'][2]['parent']=sample['tree'][0]['address']
            elif mutation=='outer':sample['tree'][2]['outer']=sample['tree'][0]['address']
            elif mutation=='depth':sample['tree'][2]['depth']=4
            else:sample['tree'][-1]['cls']='UILabel'
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):centered_translation(sample)

    def test_reject_nonfinite_inconsistent_and_missing_metadata(self):
        for mutation in ('nan','inf','ordered','raw','type','dimension','missing','ancestry'):
            sample=fixture();b=sample['tree'][2]['render_bounds']
            if mutation in ('nan','inf'):b['values'][0]=float(mutation)
            elif mutation=='ordered':b['ordered_values'][0]+=1
            elif mutation=='raw':b['raw']='00'*16
            elif mutation=='type':b['reflected']['type']='IntProperty'
            elif mutation=='dimension':b['reflected']['array_dim']=3
            elif mutation=='missing':del b['reflected']
            else:b['class_ancestry']['class_chain'][0]['super']=123
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):centered_translation(sample)

    def test_reject_owner_and_freshness_mismatch(self):
        for mutation in ('pid','scene','class','link','players','owner','readonly','consistent','hash'):
            sample=fixture();owner=sample['scene_owner']
            if mutation=='pid':sample['pid']+=1
            elif mutation=='scene':sample['top']['address']='0x1234'
            elif mutation=='class':sample['top']['cls']='UIScene'
            elif mutation=='link':owner['links']['scene_player_owner']['value']=0
            elif mutation=='players':owner['arrays']['game_players']['members']=[]
            elif mutation=='owner':owner['owner_classification']='unknown'
            elif mutation=='readonly':sample['read_only']=False
            elif mutation=='consistent':owner['consistent_reread']=False
            else:owner['profile_game_sha256']='0'*64
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):centered_translation(sample)

    def test_unowned_single_player_is_explicitly_supported(self):
        sample=fixture();owner=sample['scene_owner']
        owner['owner_classification']='unowned_single_local_player'
        owner['classification_basis']='null_owner_with_exact_single_player_graph'
        owner['links']['scene_player_owner']['value']=0
        self.assertEqual(centered_translation(sample),(0,250))

    def test_safe_domain_and_budget(self):
        with self.assertRaises(ValueError):centered_translation(fixture(3))
        sample=fixture();b=sample['tree'][1]['render_bounds']
        b['values'][2]=100;b['ordered_values'][2]=100
        b['raw']=struct.pack('<4f',*b['values']).hex()
        with self.assertRaises(ValueError):centered_translation(sample)

    def test_horizontal_translation_and_directed_axes(self):
        sample=fixture()
        for node in sample['tree']:
            if node['name'] not in CONTENT:continue
            b=node['render_bounds'];v=b['values']
            v[0]+=100;v[2]+=100
            if node['name']=='imgLeftArrow':v[1],v[3]=v[3],v[1]
            b['raw']=struct.pack('<4f',*v).hex()
            b['ordered_values']=[min(v[0],v[2]),min(v[1],v[3]),max(v[0],v[2]),max(v[1],v[3])]
            b['y_reversed']=v[1]>v[3]
        self.assertEqual(centered_translation(sample),(-100,250))

    def test_saved_actual_sample_if_available(self):
        path=Path(__file__).resolve().parents[2]/'analysis/runtime/vrdof131-path-proof-374.json'
        if not path.is_file():self.skipTest('private runtime evidence unavailable')
        with path.open(encoding='utf-8') as stream:sample=json.load(stream)
        self.assertEqual(centered_translation(sample),(-1,374))


if __name__=='__main__':unittest.main()
