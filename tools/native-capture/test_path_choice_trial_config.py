"""Pure snapshot-to-INI tests: synthetic graphs, no process or input APIs."""
import copy
import configparser
import json
from pathlib import Path
import struct
import unittest
from unittest import mock

import path_choice_trial_config as trial


def fixture():
    identities, names, classes, properties = [], {'None':0}, [], []
    def identity(address, name, cls, outer=0, number=0):
        if name not in names:names[name] = len(names)
        value = dict(address=address, internal_index=len(identities), name_index=names[name],
                     name=name, number=number, cls=cls, outer=outer)
        identities.append(value)
        return value
    class_names = ('Class', 'Object', 'UIScene', 'UIScene_PathChoice', 'GameUISceneClient', 'Player',
        'LocalPlayer', 'PlayerController', 'UIInteraction', 'GameViewportClient', 'Engine', 'ObjectProperty', 'ArrayProperty')
    addresses = {name:0x100000 + index * 0x200 for index, name in enumerate(class_names)}
    supers = {'Class':'Object', 'Object':None, 'UIScene':'Object', 'UIScene_PathChoice':'UIScene',
        'GameUISceneClient':'Object', 'Player':'Object', 'LocalPlayer':'Player', 'PlayerController':'Object',
        'UIInteraction':'Object', 'GameViewportClient':'Object', 'Engine':'Object', 'ObjectProperty':'Object', 'ArrayProperty':'Object'}
    class_ids = {}
    for name in class_names:
        value = identity(addresses[name], name, addresses['Class'], 0x900000)
        class_ids[name] = value
        classes.append(dict(identity=value, super=addresses[supers[name]] if supers[name] else 0))
    role_addresses = {role:0x200000 + index * 0x1000 for index, role in enumerate(trial.ROLE_CLASSES)}
    role_class = {'client':'GameUISceneClient','scene':'UIScene_PathChoice','controller':'PlayerController',
        'local_player':'LocalPlayer','interaction':'UIInteraction','viewport':'GameViewportClient','engine':'Engine'}
    outers = {'client':'interaction','interaction':'viewport','viewport':'engine','local_player':'engine','scene':'client'}
    roles = {role:identity(role_addresses[role], role, addresses[role_class[role]],
                          role_addresses[outers[role]] if role in outers else 0, 1)
             for role in role_addresses}
    toast = identity(0x280000, '提示', addresses['UIScene'], roles['client']['address'], 1)
    specifications = [('UIInteraction','SceneClient','ObjectProperty'),('GameViewportClient','UIController','ObjectProperty'),
        ('LocalPlayer','ViewportClient','ObjectProperty'),('Player','Actor','ObjectProperty'),
        ('PlayerController','Player','ObjectProperty'),('UIScene','SceneClient','ObjectProperty'),
        ('UIScene','PlayerOwner','ObjectProperty'),('Engine','GamePlayers','ArrayProperty'),
        ('GameUISceneClient','ActiveScenes','ArrayProperty')]
    groups = {}
    for index, (owner, name, kind) in enumerate(specifications):
        value = identity(0x300000 + index * 0x400, name, addresses[kind], addresses[owner])
        prop = dict(identity=value, owner=class_ids[owner], type=class_ids[kind],
                    offset=0x100 + index * 0x20, array_dim=1, element_size=16 if kind=='ArrayProperty' else 8)
        if kind=='ArrayProperty':
            inner = identity(0x380000 + index * 0x400, 'Inner', addresses['ObjectProperty'], value['address'])
            prop['inner'] = dict(identity=inner, type=class_ids['ObjectProperty'])
        properties.append(prop)
        headers = groups.setdefault(addresses[owner], [])
        if headers:headers[-1]['next'] = value['address']
        headers.append({key:value[key] for key in ('address','internal_index','name_index','number','outer')})
        headers[-1]['next'] = 0
    links = {}
    for label, (role, name, owner, target) in trial.LINKS.items():
        links[label] = dict(object=roles[role]['address'], property=name,
            value=roles[target]['address'] if target else roles['local_player']['address'])
    base = 0x140000000
    snapshot = dict(schema=1, kind='scene_owner_cpu_snapshot', pid=123, module_base=base,
        profile_game_sha256=trial.GAME_SHA, read_only=True, consistent_reread=True,
        owner_classification='local_player', classification_basis='explicit_local_player',
        identities=identities, roles=roles, classes=classes, properties=properties,
        fields=[dict(owner=owner, headers=headers) for owner,headers in groups.items()], links=links,
        arrays=dict(game_players=dict(property=properties[7]['identity']['address'], data=0x400000, count=1, capacity=17,
                                     members=[roles['local_player']['address']]),
                    active_scenes=dict(property=properties[8]['identity']['address'], data=0x401000, count=2, capacity=17,
                                       members=[roles['scene']['address'],toast['address']])),
        name_entries=[dict(index=index, address=0x500000 + index*0x100, text=text) for text,index in names.items()],
        evidence=dict(method='fresh_relevant_fields_and_registry_slots_with_final_reread',
            registry_headers=dict(objects=dict(address=base+0x1c24730,data=0x70000000,count=1024,capacity=2048),
                                  names=dict(address=base+0x1c34800,data=0x60000000,count=1024,capacity=2048))))
    return json.loads(json.dumps(snapshot))


def generate(snapshot, **kwargs):
    return trial.build(snapshot, **dict(pid=123, creation_time=456, started_qpc=789, world_generation=2, nonce=1000, **kwargs))


def parse(ini):
    parser = configparser.RawConfigParser()
    parser.read_string(ini)
    fields = dict(parser['PathChoiceTrial'])
    checks = {}
    for index in range(int(fields['check_count'])):
        address, value = fields['check_'+str(index)].split(':')
        checks[int(address,16)] = bytes.fromhex(value)
    return fields, checks


class TrialConfigTests(unittest.TestCase):
    def test_complete_fixture_serializes_minimal_ascii_checks_and_unicode_names(self):
        snapshot = fixture();original = copy.deepcopy(snapshot)
        ini = generate(snapshot, shift_x=-512, shift_y=512, duration_ms=8000)
        ini.encode('ascii')
        self.assertEqual(snapshot,original)
        fields, checks = parse(ini)
        self.assertEqual(fields['nonce'],'1000')
        self.assertEqual(fields['shift_x'],'-512')
        self.assertEqual(fields['base'],'0x140000000')
        self.assertLessEqual(int(fields['check_count']),2048)
        self.assertLessEqual(sum(map(len,checks.values())),32768)
        self.assertTrue(all(0 < len(value) <= 256 for value in checks.values()))
        entry = next(value for value in snapshot['name_entries'] if value['text']=='提示')
        self.assertEqual(checks[entry['address']+20],'提示'.encode('utf-16-le')+b'\0\0')
        header = snapshot['evidence']['registry_headers']['names']
        self.assertEqual(checks[header['data']+entry['index']*8],struct.pack('<Q',entry['address']))
        # The two SceneClient declarations have independent source objects/offsets.
        for label, role, owner in (('interaction_scene_client','interaction','UIInteraction'),('scene_client','scene','UIScene')):
            prop = next(p for p in snapshot['properties'] if p['identity']['name']=='SceneClient' and p['owner']['name']==owner)
            self.assertEqual(checks[snapshot['roles'][role]['address']+prop['offset']],
                             struct.pack('<Q',snapshot['roles']['client']['address']))
        self.assertEqual(ini,generate(snapshot,shift_x=-512,shift_y=512,duration_ms=8000))

    def test_null_owner_requires_exact_single_player_graph(self):
        snapshot = fixture();snapshot['owner_classification']='unowned_single_local_player'
        snapshot['classification_basis']='null_owner_with_exact_single_player_graph'
        snapshot['links']['scene_player_owner']['value']=0
        _, checks = parse(generate(snapshot))
        prop = next(p for p in snapshot['properties'] if p['identity']['name']=='PlayerOwner')
        self.assertEqual(checks[snapshot['roles']['scene']['address']+prop['offset']],bytes(8))
        snapshot['arrays']['game_players']['members'].append(snapshot['roles']['controller']['address'])
        snapshot['arrays']['game_players']['count']=2
        with self.assertRaises(ValueError):generate(snapshot)

    def test_snapshot_flags_profile_pid_and_missing_names_reject(self):
        changes = [('schema',True),('kind','unknown'),('read_only',False),('consistent_reread',False),
                   ('profile_game_sha256','0'*64),('pid',124),('owner_classification','other_player')]
        for key,value in changes:
            with self.subTest(key=key):
                snapshot=fixture();snapshot[key]=value
                with self.assertRaises(ValueError):generate(snapshot)
        snapshot=fixture();snapshot.pop('name_entries')
        with self.assertRaises(ValueError):generate(snapshot)

    def test_wrong_links_roles_arrays_and_class_chain_reject(self):
        for failure in ('owner','link_source','link_value','client_role','outer','active','array_property',
                        'duplicate_member','scene_class','class_cycle','metaclass'):
            with self.subTest(failure=failure):
                snapshot=fixture()
                if failure=='owner':snapshot['links']['scene_player_owner']['value']=0
                elif failure=='link_source':snapshot['links']['scene_client']['object']=snapshot['roles']['interaction']['address']
                elif failure=='link_value':snapshot['links']['controller_player']['value']=0
                elif failure=='client_role':snapshot['roles']['client']=snapshot['roles']['scene']
                elif failure=='outer':snapshot['roles']['local_player']['outer']=0
                elif failure=='active':snapshot['arrays']['active_scenes']['members']=[snapshot['roles']['controller']['address']];snapshot['arrays']['active_scenes']['count']=1
                elif failure=='array_property':snapshot['arrays']['active_scenes']['property']=snapshot['arrays']['game_players']['property']
                elif failure=='duplicate_member':snapshot['arrays']['active_scenes']['members']=[snapshot['roles']['scene']['address']]*2
                elif failure=='scene_class':snapshot['roles']['scene']['cls']=snapshot['roles']['client']['cls']
                elif failure=='class_cycle':snapshot['classes'][0]['super']=snapshot['classes'][0]['identity']['address']
                elif failure=='metaclass':snapshot['classes'][0]['identity']['cls']=snapshot['classes'][1]['identity']['address']
                with self.assertRaises(ValueError):generate(snapshot)

    def test_required_property_type_layout_owner_membership_and_inner_reject(self):
        for failure in ('missing','type','dimension','size','offset','owner','membership','inner','inner_type'):
            with self.subTest(failure=failure):
                snapshot=fixture()
                if failure=='missing':snapshot['properties'].pop()
                elif failure=='type':snapshot['properties'][0]['type']=snapshot['properties'][-1]['type']
                elif failure=='dimension':snapshot['properties'][0]['array_dim']=2
                elif failure=='size':snapshot['properties'][0]['element_size']=16
                elif failure=='offset':snapshot['properties'][0]['offset']=0x10000
                elif failure=='owner':snapshot['properties'][0]['owner']=snapshot['properties'][5]['owner']
                elif failure=='membership':snapshot['fields'][0]['headers']=[]
                elif failure=='inner':snapshot['properties'][-1]['inner']['identity']['outer']=0
                elif failure=='inner_type':snapshot['properties'][-1]['inner']['type']=snapshot['properties'][-1]['type']
                with self.assertRaises(ValueError):generate(snapshot)

    def test_integer_limits_nonce_shift_nan_and_overflow_reject(self):
        arguments = [('pid',0),('pid',True),('creation_time',0),('started_qpc',-1),('world_generation',0),
            ('nonce',0),('nonce','123'),('nonce',(1<<64)),('shift_x',513),('shift_y',-513),
            ('shift_x',float('nan')),('shift_y',float('inf')),('duration_ms',99),('duration_ms',8001),('duration_ms',3000.0)]
        defaults=dict(pid=123,creation_time=456,started_qpc=789,world_generation=2,nonce=1000)
        for key,value in arguments:
            with self.subTest(key=key,value=value):
                with self.assertRaises(ValueError):trial.build(fixture(),**dict(defaults,**{key:value}))
        for target in ('registry','array','identity'):
            snapshot=fixture()
            if target=='registry':snapshot['evidence']['registry_headers']['objects']['data']=(1<<64)-8
            elif target=='array':snapshot['arrays']['active_scenes']['data']=(1<<64)-8
            else:snapshot['identities'][0]['address']=(1<<64)-8
            with self.subTest(overflow=target),self.assertRaises(ValueError):generate(snapshot)

    def test_bad_name_mapping_registry_anchor_or_capacity_reject(self):
        for target in ('text','index','address','header','capacity'):
            with self.subTest(target=target):
                snapshot=fixture()
                if target=='text':snapshot['name_entries'][0]['text']='WrongNone'
                elif target=='index':snapshot['name_entries'][0]['index']=999999
                elif target=='address':snapshot['name_entries'][0]['address']=(1<<64)-1
                elif target=='header':snapshot['evidence']['registry_headers']['objects']['address']+=8
                else:snapshot['arrays']['game_players']['capacity']=4097
                with self.assertRaises(ValueError):generate(snapshot)

    def test_check_budget_and_overlap_conflicts_are_not_silently_truncated(self):
        for name,value in (('MAX_CHECKS',10),('MAX_BYTES',100)):
            with mock.patch.object(trial,name,value),self.assertRaises(ValueError):generate(fixture())
        checks=trial._Checks();checks.add(0x100,b'abc');checks.add(0x100,b'abc')
        self.assertEqual(len(checks.spans),1)
        checks.add(0x101,b'bc')
        with self.assertRaises(ValueError):checks.add(0x100,b'abd')
        with self.assertRaises(ValueError):checks.add(0x102,b'X')
        chunks=trial._Checks();chunks.add(0x1000,bytes(600))
        self.assertEqual([len(x) for x in chunks.spans.values()],[256,256,88])

    def test_saved_legacy_snapshot_without_name_entry_proof_is_rejected(self):
        path=Path(__file__).resolve().parents[2]/'analysis/runtime/vrdof129-path-layout-fixed2.json'
        if not path.is_file():self.skipTest('ignored historical snapshot is not installed')
        snapshot=json.loads(path.read_text())[0]['scene_owner']
        if 'name_entries' in snapshot:self.skipTest('historical sample has been superseded')
        with self.assertRaises(ValueError):trial.build(snapshot,pid=snapshot['pid'],creation_time=1,
            started_qpc=1,world_generation=1,nonce=1)


if __name__=='__main__':unittest.main()
