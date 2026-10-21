import pathlib
import tempfile
import unittest
import json
import copy
import sys
from unittest.mock import patch, Mock
from menu_display_monitor import MenuDisplayMonitor, PACKET, classify, loading_display, state_evidence, UNKNOWN, GAMEPLAY, DEATH_MENU, PAUSE_MENU, MAIN_MENU, LOADING_DISPLAY, ACQUISITION_WAIT


def sample(death=True):
    return dict(pid=11, read_only=True, consistent_reread=True, player_index=0, input_block_count=0,
                top=dict(name='DeltaGameover' if death else 'PenderCrossplayInvitePopUp',
                         tag='DeltaGameover' if death else 'PenderCrossplayInvitePopUp',
                         cls='WarUIScene' if death else 'WarUIScene_CrossplayInvitePopUp', input_mode=1 if death else 2))


def pause_sample():
    return dict(sample(), top=dict(name='DeltaPause-Campaign-Host', tag='DeltaPause-Campaign-Host',
                                  cls='WarUIScene', input_mode=1),
                focused_leaf=dict(name='LBtnResumeSingle', cls='UILabelButton'))


def main_menu_sample():
    root=dict(address='0x3f78a8dd0', name='DeltaMainMenu', number=2,
              cls='WarUIScene_MainMenu', outer='0x1438a180', input_mode=4)
    return dict(sample(), top=dict(root,tag='DeltaMainMenu',tag_number=0), active_scenes=[root])


def campaign_menu_sample(continue_game=False):
    state=main_menu_sample()
    campaign_type=dict(state['active_scenes'][0],name='DeltaCampaignType',
                       cls='WarUIScene_CampaignType',address='0x1631a44ae0')
    state['active_scenes'].append(campaign_type)
    if continue_game:
        state['active_scenes'].append(dict(campaign_type,name='DeltaCampaignMenu_ContinueGame',
            cls='WarUIScene_Campaign',address='0x1632ba2380'))
    state['top']=dict(state['active_scenes'][-1],tag=state['active_scenes'][-1]['name'],tag_number=0)
    return state


def chapter_menu_sample(depth=4):
    state = campaign_menu_sample(True)
    for index, (name, cls) in enumerate((('DeltaSelectAct', 'WarUIScene'),
            ('DeltaSelectMap', 'WarUIScene'), ('DeltaSelectDifficulty', 'WarUIScene'),
            ('WarfareConfirmation', 'UIScene_Confirmation')), 4):
        if index > depth:break
        state['active_scenes'].append(dict(address=hex(0x1000+index*0x100), name=name,
            cls=cls, number=index, outer=state['top']['outer'], input_mode=1))
    state['top'] = dict(state['active_scenes'][-1], tag=state['active_scenes'][-1]['name'], tag_number=0)
    return state


SETTINGS = ('DeltaPause-Options', 'DeltaGameConfiguration', 'DeltaVideoSettings',
            'PenderGraphicsSettings', 'DeltaAudioVideoSettings', 'DeltaInputSettings')
def settings_sample(name):
    names = ['DeltaPause-Campaign-Host', 'DeltaPause-Options']
    if name != 'DeltaPause-Options':names.append(name)
    scenes = [dict(name=n,cls='WarUIScene',input_mode=1,address=hex(0x100+i)) for i,n in enumerate(names)]
    return dict(pause_sample(),active_scenes=scenes,top=dict(scenes[-1],tag=name))


def return_confirmation_sample():
    root=dict(name='DeltaPause-Campaign-Host',cls='WarUIScene',input_mode=1,address='0x100')
    prompt=dict(name='WarfareConfirmation',cls='UIScene_Confirmation',input_mode=1,address='0x200')
    focus=dict(prompt,states=['UIState_Enabled'])
    return dict(sample(),top=dict(prompt,tag='WarfareConfirmation'),active_scenes=[root,prompt],
                dialog_text={'Title':'<Strings:WarfareGame.Generic.ConfirmSurrender_Title>',
                             'MessageText':'<Strings:WarfareGame.Generic.ConfirmSurrender>'},
                focus_chain=[focus],focused_leaf=focus)


def checkpoint_confirmation_sample():
    return dict(return_confirmation_sample(),dialog_text={
        'Title':'<Strings:WarfareGame.Generic.ConfirmCheckPointLoad_Title>',
        'MessageText':'<Strings:WarfareGame.Generic.ConfirmCheckPointLoad>'})


class Reader:
    closed=False
    def sample(self):return sample()
    def close(self):self.closed=True


def loading_sample(blocked=2):
    # Actual sequence233 structure from continuous-stereo-11428...bc269c4e.
    root=dict(address='0x8e8c9280',name='PenderCrossplayInvitePopUp',number=1,
              cls='WarUIScene_CrossplayInvitePopUp',outer='0x1438a180')
    focus=dict(root,states=['UIState_Enabled'])
    scenes=[dict(address='0x80115770',name='HUD_Toast',number=1,cls='WarUIScene_Toast',outer=root['outer'],input_mode=0),
            dict(address='0x8011dd20',name='MessageToast',number=2,cls='WarUIScene_Toast',outer=root['outer'],input_mode=0),
            dict(root,input_mode=2)]
    return dict(sample(False),input_block_count=blocked,top=dict(root,input_mode=2,tag=root['name'],tag_number=0),
                active_scenes=scenes,focus_chain=[copy.deepcopy(focus)],focused_leaf=copy.deepcopy(focus))


class Tests(unittest.TestCase):
    def test_chapter_selection_pages_keep_main_menu_display_without_input_authority(self):
        with patch.object(sys, 'path', [str(pathlib.Path(__file__).resolve().parents[1]/'game-io'), *sys.path]):
            from menu_control import allowed
        for depth in range(4, 8):
            for blocked in (0, 1):
                state = dict(chapter_menu_sample(depth), input_block_count=blocked)
                self.assertEqual(classify(state), MAIN_MENU)
                self.assertFalse(allowed(state, 11))
                # A standalone confirmation or a selector under Pause remains
                # outside the exact main-menu chapter chain.
                for index in range(len(state['active_scenes'])):
                    changed = copy.deepcopy(state);changed['active_scenes'].pop(index)
                    self.assertEqual(classify(changed), UNKNOWN)
                for index in range(len(state['active_scenes'])):
                    for key, value in (('name', 'Other'), ('cls', 'OtherClass'), ('input_mode', True),
                            ('input_mode', 4.0), ('outer', '0x999'), ('address', '0x0'), ('number', True)):
                        changed = copy.deepcopy(state);changed['active_scenes'][index][key] = value
                        self.assertEqual(classify(changed), UNKNOWN, (depth, index, key))
                for key, value in (('read_only', False), ('consistent_reread', False), ('input_block_count', 2),
                        ('player_index', -1), ('player_index', True), ('active_scenes_truncated', True),
                        ('focus_chain_truncated', True)):
                    self.assertEqual(classify(dict(state, **{key: value})), UNKNOWN)
                changed = copy.deepcopy(state);changed['active_scenes'][0]['address'] = changed['active_scenes'][1]['address']
                self.assertEqual(classify(changed), UNKNOWN)
                changed = copy.deepcopy(state);changed['active_scenes'][:2] = reversed(changed['active_scenes'][:2])
                self.assertEqual(classify(changed), UNKNOWN)
                changed = copy.deepcopy(state);changed['top']['number'] += 1
                self.assertEqual(classify(changed), UNKNOWN)

    def test_chapter_menu_publication_keeps_display_mode_until_gameplay(self):
        with tempfile.TemporaryDirectory() as d:
            path = pathlib.Path(d)/'menu.bin';reader = Reader();monitor = self.monitor(path, reader)
            states = [campaign_menu_sample(True), *[chapter_menu_sample(i) for i in range(4, 8)], sample(False)]
            try:
                for sequence, state in enumerate(states, 1):
                    reader.sample = lambda state=state: state
                    monitor.clock = lambda sequence=sequence: sequence*100
                    monitor.poll()
                    packet = PACKET.unpack(path.read_bytes())
                    self.assertEqual(packet[3], GAMEPLAY if sequence == len(states) else MAIN_MENU)
                    self.assertEqual(packet[-2:], (sequence, sequence*100))
            finally:monitor.close()

    @unittest.skipUnless((pathlib.Path(__file__).resolve().parents[2]/
        'analysis/runtime/continuous-stereo-30768-20261020-153714-f929944f.menu.jsonl').exists(),
        'local VDXR chapter-menu evidence unavailable')
    def test_saved_headset_chapter_pages_are_main_menu_display(self):
        path = pathlib.Path(__file__).resolve().parents[2]/'analysis/runtime/continuous-stereo-30768-20261020-153714-f929944f.menu.jsonl'
        observed = set()
        for line in path.read_text(encoding='utf-8').splitlines():
            row = json.loads(line)
            if not 461 <= row.get('sequence', 0) <= 1259:continue
            state = row.get('state_evidence')
            if not state:continue
            self.assertEqual(row['mode'], UNKNOWN)  # Original fault receipt.
            self.assertEqual(classify(state), MAIN_MENU)
            observed.add(state['top']['tag'])
        self.assertEqual(observed, {'DeltaSelectAct', 'DeltaSelectMap', 'DeltaSelectDifficulty', 'WarfareConfirmation'})

    def test_loading_display_exact_observed_stack_is_never_gameplay_or_input(self):
        # XR15 actual line9: same exact stack at block2 -> block1 -> block0.
        for blocked in (1,2):
            with self.subTest(blocked=blocked):
                self.assertEqual(LOADING_DISPLAY,5)
                base=loading_sample(blocked);self.assertTrue(loading_display(base));self.assertEqual(classify(base),LOADING_DISPLAY)
                for invalid_block in (3,4,100,-1,True,False,1.0,2.0,'1','2',None):
                    self.assertEqual(classify(dict(base,input_block_count=invalid_block)),UNKNOWN)
                self.assertEqual(classify(dict(base,input_block_count=0)),GAMEPLAY)
                for key,value in (('read_only',False),('consistent_reread',False),('player_index',-1),
                                  ('player_index',True),('player_index',0.0),('player_index',None)):
                    self.assertEqual(classify(dict(base,**{key:value})),UNKNOWN)
                with patch.object(sys,'path',[str(pathlib.Path(__file__).resolve().parents[1]/'game-io'),*sys.path]):
                    from menu_control import allowed
                self.assertFalse(allowed(base,11))
                # No address/instance number is a hardcoded authority.
                changed=copy.deepcopy(base)
                for node in [changed['top'],changed['active_scenes'][-1],changed['focused_leaf'],changed['focus_chain'][0]]:
                    node.update(address='0x123456',number=9)
                for node in [changed['top'],*changed['active_scenes'],changed['focused_leaf'],changed['focus_chain'][0]]:
                    node['outer']='0x987654'
                self.assertEqual(classify(changed),LOADING_DISPLAY)

    def test_loading_display_rejects_each_identity_shape_order_and_focus_mismatch(self):
        for blocked in (1,2):
            with self.subTest(blocked=blocked):
                self._assert_loading_structure_rejected(blocked)

    def _assert_loading_structure_rejected(self, blocked):
        base=loading_sample(blocked)
        paths=[('top',),('active_scenes',0),('active_scenes',1),('active_scenes',2),('focus_chain',0),('focused_leaf',)]
        for path in paths:
            for key,bad_values in (('address',(None,'','0x0','garbage',123,'0x10000000000000000')),
                    ('outer',(None,'','0x0',123,'0x999')),('number',(-1,True,1.0,'1',None)),
                    ('name',('',None,'OtherScene')),('cls',('',None,'OtherClass'))):
                for value in bad_values:
                    state=copy.deepcopy(base);node=state
                    for part in path:node=node[part]
                    node[key]=value
                    self.assertEqual(classify(state),UNKNOWN,(path,key,value))
        for path in [('top',),('active_scenes',0),('active_scenes',1),('active_scenes',2)]:
            for mode in (None,True,2.0,'2',4):
                state=copy.deepcopy(base);node=state
                for part in path:node=node[part]
                node['input_mode']=mode;self.assertEqual(classify(state),UNKNOWN,(path,mode))
        for key,value in (('tag','OtherScene'),('tag',None),('tag_number',True),('tag_number',-1),('tag_number',1)):
            state=copy.deepcopy(base);state['top'][key]=value;self.assertEqual(classify(state),UNKNOWN)
        for key in ('active_scenes_truncated','focus_chain_truncated'):
            self.assertEqual(classify(dict(base,**{key:False})),LOADING_DISPLAY)
            for value in (True,None,0,1,'False',[],{}):
                self.assertEqual(classify(dict(base,**{key:value})),UNKNOWN,(key,value))
        for scenes in (None,[],base['active_scenes'][:2],base['active_scenes']+[base['active_scenes'][0]],
                       list(reversed(base['active_scenes'])),[None]*3):
            self.assertEqual(classify(dict(base,active_scenes=scenes)),UNKNOWN)
        for chain in (None,[],base['focus_chain']*2,[None]):
            self.assertEqual(classify(dict(base,focus_chain=chain)),UNKNOWN)
        for path in [('focused_leaf',),('focus_chain',0)]:
            for states in (None,[],['UIState_Focused'],['UIState_Enabled','UIState_Focused'],('UIState_Enabled',)):
                state=copy.deepcopy(base);node=state
                for part in path:node=node[part]
                node['states']=states;self.assertEqual(classify(state),UNKNOWN)
        for key in ('top','focused_leaf'):
            for value in (None,{},'PenderCrossplayInvitePopUp'):
                self.assertEqual(classify(dict(base,**{key:value})),UNKNOWN)
        state=copy.deepcopy(base);state['active_scenes'][0]['address']=state['top']['address']
        self.assertEqual(classify(state),UNKNOWN)

    def test_loading_mode_packet_retains_schema_size_identity_and_acquisition_time(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();reader.sample=loading_sample;m=self.monitor(path,reader)
            try:
                m.poll();self.assertEqual(PACKET.unpack(path.read_bytes()),(0x47564d31,1,64,5,11,22,33,44,1,100))
                row=json.loads(path.with_suffix('.jsonl').read_text().splitlines()[0])
                self.assertEqual(row['state_evidence']['input_block_count'],2)
                reader.sample=lambda:dict(loading_sample(),input_block_count=0);m.clock=lambda:200;m.poll()
                self.assertEqual(PACKET.unpack(path.read_bytes()),(0x47564d31,1,64,GAMEPLAY,11,22,33,44,2,200))
            finally:m.close()

    def test_loading_block_transition_logs_2_1_0_without_repeated_same_block_rows(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            try:
                for sequence,blocked in enumerate((2,2,1,1,0),1):
                    reader.sample=lambda blocked=blocked:loading_sample(blocked)
                    m.clock=lambda sequence=sequence:sequence*100;m.poll()
                    packet=PACKET.unpack(path.read_bytes())
                    self.assertEqual(packet,(0x47564d31,1,64,LOADING_DISPLAY if blocked else GAMEPLAY,
                        11,22,33,44,sequence,sequence*100))
                records=[json.loads(line) for line in path.with_suffix('.jsonl').read_text().splitlines()]
                self.assertEqual([r['sequence'] for r in records],[1,3,5])
                self.assertEqual([r['mode'] for r in records],[LOADING_DISPLAY,LOADING_DISPLAY,GAMEPLAY])
                self.assertEqual([r['state_evidence']['input_block_count'] for r in records],[2,1,0])
                self.assertEqual({r['reason'] for r in records},{'PenderCrossplayInvitePopUp'})
            finally:m.close()

    def test_state_evidence_owns_bounded_scene_and_focus_fields(self):
        state=sample(False)
        state['top'].update(address='0x500',number=3,outer='0x100',tag_number=0)
        state['focused_leaf']=dict(state['top'],states=['UIState_Enabled'])
        state['active_scenes']=[dict(state['top'],address=hex(i)) for i in range(35)]
        state['focus_chain']=[dict(state['focused_leaf'],address=hex(i)) for i in range(19)]
        evidence=state_evidence(state)
        self.assertEqual(evidence['top'],state['top'])
        self.assertEqual(evidence['focused_leaf'],state['focused_leaf'])
        self.assertEqual(evidence['active_scenes'],state['active_scenes'][-32:])
        self.assertEqual(evidence['focus_chain'],state['focus_chain'][:16])
        for key,total,selection in (('active_scenes',35,'tail'),('focus_chain',19,'prefix')):
            self.assertEqual(evidence[key+'_total'],total)
            self.assertTrue(evidence[key+'_truncated'])
            self.assertEqual(evidence[key+'_selection'],selection)
        state['top']['address']='changed';state['focused_leaf']['states'].append('changed')
        state['active_scenes'][-1]['address']='changed';state['focus_chain'][0]['states'].append('changed')
        self.assertEqual(evidence['top']['address'],'0x500')
        self.assertEqual(evidence['focused_leaf']['states'],['UIState_Enabled'])
        self.assertEqual(evidence['active_scenes'][-1]['address'],hex(34))
        self.assertEqual(evidence['focus_chain'][0]['states'],['UIState_Enabled'])
        self.assertIsNone(state_evidence(None))

    def test_blocked_pender_unknown_then_gameplay_logs_evidence_without_more_rows_or_packet_change(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            state=sample(False);state['top'].update(address='0x500',number=3,outer='0x100',tag_number=0)
            state['focused_leaf']=dict(state['top'],states=['UIState_Enabled'])
            state['active_scenes']=[dict(state['top'])]
            state['focus_chain']=[dict(state['focused_leaf'])]
            try:
                for sequence,(now,blocked,mode) in enumerate(((100,1,UNKNOWN),(200,2,UNKNOWN),(300,0,GAMEPLAY)),1):
                    current=copy.deepcopy(state);current['input_block_count']=blocked
                    reader.sample=lambda current=current:current;m.clock=lambda now=now:now;m.poll()
                    self.assertEqual(PACKET.unpack(path.read_bytes()),(0x47564d31,1,64,mode,11,22,33,44,sequence,now))
                rows=[json.loads(line) for line in path.with_suffix('.jsonl').read_text().splitlines()]
                self.assertEqual(len(rows),2)
                self.assertEqual([r['mode'] for r in rows],[UNKNOWN,GAMEPLAY])
                self.assertEqual([r['observed_ns'] for r in rows],[100,300])
                self.assertEqual([r['state_evidence']['input_block_count'] for r in rows],[1,0])
                for row in rows:
                    evidence=row['state_evidence']
                    self.assertTrue(evidence['read_only'] and evidence['consistent_reread'])
                    self.assertEqual(evidence['player_index'],0)
                    self.assertEqual(evidence['top'],state['top'])
                    self.assertEqual(evidence['focused_leaf'],state['focused_leaf'])
                    self.assertEqual(evidence['active_scenes'],state['active_scenes'])
                    self.assertEqual(evidence['focus_chain'],state['focus_chain'])
                    self.assertFalse(evidence['active_scenes_truncated'] or evidence['focus_chain_truncated'])
            finally:m.close()

    def test_failed_acquisition_has_no_previous_or_partial_sample_evidence(self):
        for failure in ('read_error','wrong_pid'):
            with self.subTest(failure=failure),tempfile.TemporaryDirectory() as d:
                path=pathlib.Path(d)/'menu.bin';reader=Reader();reader.sample=lambda:sample(False)
                m=self.monitor(path,reader)
                try:
                    m.poll()
                    if failure=='read_error':reader.sample=Mock(side_effect=RuntimeError('read failed'))
                    else:reader.sample=lambda:dict(sample(False),pid=99)
                    m.clock=lambda:200;m.poll()
                    rows=[json.loads(line) for line in path.with_suffix('.jsonl').read_text().splitlines()]
                    self.assertEqual(rows[0]['mode'],GAMEPLAY)
                    self.assertIsNotNone(rows[0]['state_evidence'])
                    self.assertEqual(rows[1]['mode'],UNKNOWN)
                    self.assertIsNone(rows[1]['state_evidence'])
                    self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)
                    self.assertTrue(reader.closed)
                finally:m.close()

    def test_campaign_descendants_require_exact_ordered_observed_chain(self):
        for continued in (False,True):
            base=campaign_menu_sample(continued)
            self.assertEqual(classify(base),MAIN_MENU)
            for key,value in (('input_block_count',2),('player_index',-1),('player_index',False),
                              ('read_only',False),('consistent_reread',False)):
                self.assertEqual(classify(dict(base,**{key:value})),UNKNOWN)
            for index in range(len(base['active_scenes'])):
                state=copy.deepcopy(base);state['active_scenes'].pop(index)
                self.assertEqual(classify(state),UNKNOWN)
                for key,value in (('name','UnknownSettings'),('cls','WarUIScene'),('input_mode',1),
                                  ('input_mode',True),('input_mode',4.0)):
                    state=copy.deepcopy(base);state['active_scenes'][index][key]=value
                    self.assertEqual(classify(state),UNKNOWN)
            for key,value in (('tag','DeltaCampaignMenu_NewGame'),('number',99),('outer','0x999'),
                              ('address','0x999'),('name','DeltaCampaignTypeExtra'),('input_mode',4.0)):
                state=copy.deepcopy(base);state['top'][key]=value
                self.assertEqual(classify(state),UNKNOWN)
            state=copy.deepcopy(base);state['active_scenes'].reverse()
            self.assertEqual(classify(state),UNKNOWN)
            state=copy.deepcopy(base);state['active_scenes'].insert(-1,dict(name='UnknownSettings'))
            self.assertEqual(classify(state),UNKNOWN)
        state=campaign_menu_sample(True)
        state['active_scenes'][:2]=reversed(state['active_scenes'][:2])
        self.assertEqual(classify(state),UNKNOWN)

    @unittest.skipUnless((pathlib.Path(__file__).resolve().parents[2]/
        'analysis/runtime/world6-main-continue.jsonl').exists(),'local actual-game evidence unavailable')
    def test_saved_actual_campaign_states_allow_observed_block_transition(self):
        evidence=pathlib.Path(__file__).resolve().parents[2]/'analysis/runtime/world6-main-continue.jsonl'
        admitted=set()
        for line in evidence.read_text(encoding='utf-8').splitlines():
            state=json.loads(line).get('state')
            if state is None:continue
            self.assertIn(state['input_block_count'],(0,1))
            self.assertEqual(classify(state),MAIN_MENU)
            admitted.add(state['top']['tag'])
        self.assertEqual(admitted,{'DeltaMainMenu','DeltaCampaignType','DeltaCampaignMenu_ContinueGame'})

    def test_main_menu_exact_root_is_explicit_display_choice(self):
        self.assertEqual(MAIN_MENU,4)
        state=main_menu_sample()
        self.assertEqual(classify(state),MAIN_MENU)
        # Focus does not certify source format or identify the scene root.
        for focus in (None,dict(name='OptionsButton',cls='UIButtonIgnoreStart')):
            self.assertEqual(classify(dict(state,focused_leaf=focus)),MAIN_MENU)

    def test_main_menu_rejects_inconsistent_root_identity_and_guards(self):
        base=main_menu_sample()
        for key,value in (('player_index',-1),('player_index',1),('player_index',False),
                          ('input_block_count',2),('input_block_count',True),('input_block_count',0.0),
                          ('read_only',False),('consistent_reread',False)):
            self.assertEqual(classify(dict(base,**{key:value})),UNKNOWN)
        for key,value in (('tag','DeltaMainMenuExtra'),('name','DeltaPause-Options'),('cls','WarUIScene'),
                          ('input_mode',1),('input_mode',True),('input_mode',4.0),('address','0x999'),
                          ('address',None),('number',3),('number',True),('outer','0x999'),('outer',None)):
            state=copy.deepcopy(base);state['top'][key]=value
            self.assertEqual(classify(state),UNKNOWN,(key,value))
        for key in ('address','name','cls','input_mode','number','outer'):
            state=copy.deepcopy(base);state['active_scenes'][-1].pop(key)
            self.assertEqual(classify(state),UNKNOWN,key)
        state=copy.deepcopy(base);state['active_scenes'][-1]['number']=True
        self.assertEqual(classify(state),UNKNOWN)
        for scenes in (None,[],[None],[dict(name='DeltaMainMenu')]):
            self.assertEqual(classify(dict(base,active_scenes=scenes)),UNKNOWN)
        state=copy.deepcopy(base);state['active_scenes'].append(dict(name='DeltaVideoSettings'))
        self.assertEqual(classify(state),UNKNOWN) # Unobserved descendants are not admitted.
        for top in (None,{},'DeltaMainMenu'):
            self.assertEqual(classify(dict(base,top=top)),UNKNOWN)

    def test_block_one_main_menu_display_requires_authenticated_stack_and_does_not_allow_input(self):
        with patch.object(sys,'path',[str(pathlib.Path(__file__).resolve().parents[1]/'game-io'),*sys.path]):
            from menu_control import allowed
        for base in (main_menu_sample(),campaign_menu_sample(),campaign_menu_sample(True)):
            for blocked in (0,1):
                self.assertEqual(classify(dict(base,input_block_count=blocked)),MAIN_MENU)
            for blocked in (False,True,0.0,1.0,-1,2,None,'1'):
                self.assertEqual(classify(dict(base,input_block_count=blocked)),UNKNOWN)
            blocked_state=dict(base,input_block_count=1)
            self.assertFalse(allowed(blocked_state,11))
            for index in range(len(base['active_scenes'])):
                for key,value in (('name','UnknownSettings'),('cls','OtherScene'),('input_mode',1)):
                    state=copy.deepcopy(blocked_state);state['active_scenes'][index][key]=value
                    self.assertEqual(classify(state),UNKNOWN)
                state=copy.deepcopy(blocked_state);state['active_scenes'].pop(index)
                self.assertEqual(classify(state),UNKNOWN)
            for key,value in (('address','0x999'),('number',99),('outer','0x999'),('tag','UnknownSettings')):
                state=copy.deepcopy(blocked_state);state['top'][key]=value
                self.assertEqual(classify(state),UNKNOWN)
            if len(base['active_scenes'])>1:
                state=copy.deepcopy(blocked_state);state['active_scenes'].reverse()
                self.assertEqual(classify(state),UNKNOWN)
        for state in (sample(),sample(False),settings_sample('UnknownSettings')):
            self.assertEqual(classify(dict(state,input_block_count=1)),UNKNOWN)

    @unittest.skipUnless(all((pathlib.Path(__file__).resolve().parents[2]/'analysis/runtime'/name).exists()
        for name in ('mainmenu8-enter-campaign.json','mainmenu8-enter-single.json')),
        'local actual-game evidence unavailable')
    def test_saved_actual_mainmenu8_transitions_keep_display_during_block_one(self):
        def states(value):
            if isinstance(value,dict):
                if isinstance(value.get('top'),dict):yield value
                for item in value.values():yield from states(item)
            elif isinstance(value,list):
                for item in value:yield from states(item)
        observed_blocks=set();observed_scenes=set()
        for name in ('mainmenu8-enter-campaign.json','mainmenu8-enter-single.json'):
            path=pathlib.Path(__file__).resolve().parents[2]/'analysis/runtime'/name
            samples=list(states(json.loads(path.read_text(encoding='utf-8'))))
            self.assertTrue(samples)
            for state in samples:
                self.assertEqual(classify(state),MAIN_MENU)
                observed_blocks.add(state['input_block_count']);observed_scenes.add(state['top']['tag'])
        self.assertEqual(observed_blocks,{0,1})
        self.assertEqual(observed_scenes,{'DeltaMainMenu','DeltaCampaignType','DeltaCampaignMenu_ContinueGame'})

    def test_main_menu_publication_uses_fresh_observations_across_unknown_and_gameplay(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            try:
                for sequence,(now,state,mode) in enumerate(((100,main_menu_sample(),MAIN_MENU),
                        (200,main_menu_sample(),MAIN_MENU),
                        (300,dict(main_menu_sample(),consistent_reread=False),UNKNOWN),
                        (400,sample(False),GAMEPLAY),(500,main_menu_sample(),MAIN_MENU)),1):
                    reader.sample=lambda state=state:state;m.clock=lambda now=now:now;m.poll()
                    packet=PACKET.unpack(path.read_bytes())
                    self.assertEqual(packet,(0x47564d31,1,64,mode,11,22,33,44,sequence,now))
            finally:m.close()

    def test_exact_pause_return_prompt_is_display_only(self):
        state=return_confirmation_sample()
        for blocked in (0,1):self.assertEqual(classify(dict(state,input_block_count=blocked)),PAUSE_MENU)
        self.assertEqual(state['focused_leaf']['states'],['UIState_Enabled'])
        with patch.object(sys,'path',[str(pathlib.Path(__file__).resolve().parents[1]/'game-io'),*sys.path]):
            from menu_control import allowed
        self.assertFalse(allowed(state,11))
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();reader.sample=lambda:state;m=self.monitor(path,reader)
            try:
                m.poll();packet=PACKET.unpack(path.read_bytes())
                self.assertEqual((packet[3],packet[-2],packet[-1]),(PAUSE_MENU,1,100))
            finally:m.close()

    def test_return_prompt_rejects_other_dialogs_and_contexts(self):
        base=return_confirmation_sample()
        for text in (None,{},dict(base['dialog_text'],Title='Exit Game?'),
                     dict(base['dialog_text'],MessageText='<Strings:WarfareGame.Generic.ConfirmCheckpoint>'),
                     dict(base['dialog_text'],extra='unreviewed')):
            self.assertEqual(classify(dict(base,dialog_text=text)),UNKNOWN)
        for scenes in (None,[],base['active_scenes'][1:],list(reversed(base['active_scenes']))):
            self.assertEqual(classify(dict(base,active_scenes=scenes)),UNKNOWN)
        for key,value in (('name','DeltaMainMenu'),('cls','OtherScene'),('input_mode',2),('input_mode',True)):
            state=copy.deepcopy(base);state['active_scenes'][0][key]=value
            self.assertEqual(classify(state),UNKNOWN)

    def test_exact_checkpoint_prompt_is_display_only_and_rejects_mixed_text_or_context(self):
        base=checkpoint_confirmation_sample();surrender=return_confirmation_sample()
        for blocked in (0,1):self.assertEqual(classify(dict(base,input_block_count=blocked)),PAUSE_MENU)
        with patch.object(sys,'path',[str(pathlib.Path(__file__).resolve().parents[1]/'game-io'),*sys.path]):
            from menu_control import allowed
        self.assertFalse(allowed(base,11))
        for text in (None,{},dict(base['dialog_text'],Title=surrender['dialog_text']['Title']),
                     dict(base['dialog_text'],MessageText=surrender['dialog_text']['MessageText']),
                     dict(base['dialog_text'],Title='Load checkpoint?'),
                     dict(base['dialog_text'],MessageText='Load current checkpoint'),
                     dict(base['dialog_text'],extra='unreviewed')):
            self.assertEqual(classify(dict(base,dialog_text=text)),UNKNOWN)
        for scenes in (None,[],base['active_scenes'][1:],list(reversed(base['active_scenes']))):
            self.assertEqual(classify(dict(base,active_scenes=scenes)),UNKNOWN)
        for key,value in (('name','DeltaMainMenu'),('cls','OtherScene'),('input_mode',2),('input_mode',True)):
            state=copy.deepcopy(base);state['active_scenes'][0][key]=value
            self.assertEqual(classify(state),UNKNOWN)
        for key,value in (('tag','OtherConfirmation'),('name','OtherConfirmation'),('address','0x999'),
                          ('address',None),('cls','WarUIScene'),('input_mode',2),('input_mode',True)):
            state=copy.deepcopy(base);state['top'][key]=value
            self.assertEqual(classify(state),UNKNOWN)
        for key,value in (('address','0x999'),('name','OtherConfirmation'),('cls','WarUIScene'),('input_mode',True)):
            state=copy.deepcopy(base);state['active_scenes'][-1][key]=value
            self.assertEqual(classify(state),UNKNOWN)
        for key,value in (('player_index',1),('player_index',-1),('player_index',False),('read_only',False),
                          ('consistent_reread',False),('input_block_count',2),('input_block_count',True),
                          ('input_block_count',1.0)):
            self.assertEqual(classify(dict(base,**{key:value})),UNKNOWN)

    def test_return_prompt_rejects_top_identity_and_common_guard_mismatches(self):
        base=return_confirmation_sample()
        for key,value in (('tag','OtherConfirmation'),('name','OtherConfirmation'),('address','0x999'),
                          ('address',None),('cls','WarUIScene'),('input_mode',2),('input_mode',True)):
            state=copy.deepcopy(base);state['top'][key]=value
            self.assertEqual(classify(state),UNKNOWN)
        for key,value in (('address','0x999'),('name','OtherConfirmation'),('cls','WarUIScene'),('input_mode',True)):
            state=copy.deepcopy(base);state['active_scenes'][-1][key]=value
            self.assertEqual(classify(state),UNKNOWN)
        for key,value in (('player_index',1),('player_index',-1),('player_index',False),('read_only',False),
                          ('consistent_reread',False),('input_block_count',2),('input_block_count',True),
                          ('input_block_count',1.0)):
            self.assertEqual(classify(dict(base,**{key:value})),UNKNOWN)

    def test_block_one_allows_only_authenticated_pause_family_display(self):
        for state in [pause_sample()]+[settings_sample(name) for name in SETTINGS]:
            with self.subTest(top=state['top']['tag']):
                for blocked in (0,1):
                    self.assertEqual(classify(dict(state,input_block_count=blocked)),PAUSE_MENU)
                for blocked in (False,True,0.0,1.0,-1,2,None,'1'):
                    self.assertEqual(classify(dict(state,input_block_count=blocked)),UNKNOWN)
        for state in (sample(),sample(False),settings_sample('UnknownSettings')):
            self.assertEqual(classify(dict(state,input_block_count=1)),UNKNOWN)
        state=settings_sample('DeltaVideoSettings');state['active_scenes']=state['active_scenes'][1:]
        self.assertEqual(classify(dict(state,input_block_count=1)),UNKNOWN)
        for key,value in (('player_index',1),('read_only',False),('consistent_reread',False)):
            self.assertEqual(classify(dict(pause_sample(),input_block_count=1,**{key:value})),UNKNOWN)

    def test_input_control_remains_blocked_at_one(self):
        with patch.object(sys,'path',[str(pathlib.Path(__file__).resolve().parents[1]/'game-io'),*sys.path]):
            from menu_control import allowed
        leaf=dict(name='CampaignButton',cls='UIButtonIgnoreStart',states=['UIState_Enabled','UIState_Focused'])
        state=dict(sample(),top=dict(tag='DeltaMainMenu',cls='WarUIScene_MainMenu'),focus_chain=[leaf],focused_leaf=leaf)
        self.assertTrue(allowed(state,11))
        self.assertFalse(allowed(dict(state,input_block_count=1),11))
        self.assertFalse(allowed(dict(pause_sample(),input_block_count=1),11))

    def test_block_transition_publication_keeps_observation_time_and_resume_gate(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            try:
                def slow_sample():
                    m.clock=lambda:150  # A slow/internal retry must not refresh observed_ns.
                    return dict(pause_sample(),input_block_count=1)
                reader.sample=slow_sample;m.poll()
                packet=PACKET.unpack(path.read_bytes())
                self.assertEqual((packet[3],packet[-2],packet[-1]),(PAUSE_MENU,1,100))
                for now,state,mode in ((200,pause_sample(),PAUSE_MENU),
                                       (300,dict(sample(False),input_block_count=1),UNKNOWN),
                                       (400,sample(False),GAMEPLAY)):
                    reader.sample=lambda state=state:state;m.clock=lambda now=now:now;m.poll()
                    packet=PACKET.unpack(path.read_bytes())
                    self.assertEqual((packet[3],packet[-2],packet[-1]),(mode,now//100,now))
            finally:m.close()

    def test_observed_pause_settings_with_ordered_context(self):
        for name in SETTINGS:
            with self.subTest(name=name):
                state=settings_sample(name)
                self.assertEqual(classify(state),PAUSE_MENU)
                for focused in (None,dict(name='AnotherSetting',cls='UILabelButton')):
                    self.assertEqual(classify(dict(state,focused_leaf=focused)),PAUSE_MENU)

    def test_settings_reject_missing_or_reordered_campaign_context(self):
        for name in SETTINGS:
            base=settings_sample(name)
            variants=[[],base['active_scenes'][1:],None]
            if name != 'DeltaPause-Options':
                variants += [[base['active_scenes'][0],base['active_scenes'][-1]],
                             [base['active_scenes'][1],base['active_scenes'][0],base['active_scenes'][-1]]]
            for scenes in variants:
                with self.subTest(name=name,scenes=scenes):
                    self.assertEqual(classify(dict(base,active_scenes=scenes)),UNKNOWN)
            for index in range(len(base['active_scenes'])-1):
                for key,value in (('name','UnknownMenu'),('cls','AnotherScene'),('input_mode',True),('input_mode',2)):
                    state=copy.deepcopy(base);state['active_scenes'][index][key]=value
                    self.assertEqual(classify(state),UNKNOWN)

    def test_settings_reject_top_mismatch_unknown_names_and_common_failures(self):
        for name in SETTINGS:
            base=settings_sample(name)
            for key,value in (('address','0x999'),('address',None),('name','OtherMenu'),('tag',name+'Extra'),
                              ('cls','OtherScene'),('input_mode',True),('input_mode',2)):
                with self.subTest(name=name,key=key,value=value):
                    state=copy.deepcopy(base);state['top'][key]=value
                    self.assertEqual(classify(state),UNKNOWN)
            state=copy.deepcopy(base);state['active_scenes'][-1]['input_mode']=True
            self.assertEqual(classify(state),UNKNOWN)
            for key,value in (('player_index',1),('player_index',-1),('player_index',False),
                              ('input_block_count',2),('read_only',False),('consistent_reread',False)):
                self.assertEqual(classify(dict(base,**{key:value})),UNKNOWN)
        self.assertEqual(classify(settings_sample('DeltaUnknownSettings')),UNKNOWN)

    def test_settings_publish_pause_then_resume_with_new_sequence(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();reader.sample=lambda:settings_sample('DeltaVideoSettings')
            m=self.monitor(path,reader)
            try:
                m.poll();first=PACKET.unpack(path.read_bytes())
                reader.sample=lambda:sample(False);m.clock=lambda:200;m.poll()
                second=PACKET.unpack(path.read_bytes())
                self.assertEqual((first[3],first[-2],first[-1]),(PAUSE_MENU,1,100))
                self.assertEqual((second[3],second[-2],second[-1]),(GAMEPLAY,2,200))
                self.assertEqual(first[:3]+first[4:8],second[:3]+second[4:8])
            finally:m.close()

    def test_pause_exact_scene_independent_of_focus_selection(self):
        self.assertEqual((UNKNOWN,GAMEPLAY,DEATH_MENU,PAUSE_MENU),(0,1,2,3))
        self.assertEqual(classify(pause_sample()), PAUSE_MENU)
        for focus in (dict(name='DifferentOption',cls='UILabelButton'), None, {}):
            with self.subTest(focus=focus):
                self.assertEqual(classify(dict(pause_sample(),focused_leaf=focus)), PAUSE_MENU)
        state=pause_sample();state.pop('focused_leaf')
        self.assertEqual(classify(state), PAUSE_MENU)

    def test_pause_rejects_unverified_scene_variants_and_invalid_common_guards(self):
        for key,value in (('tag','DeltaPause-Campaign-HostExtra'),('tag','DeltaPause-Campaign-Client'),
                          ('tag','DeltaPause-Campaign-Host-Options'),('tag',None),
                          ('cls','WarUIScene_Pause'),('cls','WarUIScene_CrossplayInvitePopUp'),
                          ('input_mode',0),('input_mode',2),('input_mode',True),
                          ('input_mode',1.0),('input_mode','1')):
            with self.subTest(key=key,value=value):
                state=pause_sample();state['top'][key]=value
                self.assertEqual(classify(state),UNKNOWN)
        for key,value in (('player_index',-1),('player_index',1),('player_index',None),
                          ('player_index',False),('player_index',0.0),('player_index','0'),
                          ('read_only',False),('consistent_reread',False),('input_block_count',2)):
            with self.subTest(key=key,value=value):
                self.assertEqual(classify(dict(pause_sample(),**{key:value})),UNKNOWN)
        for top in (None,{}):
            self.assertEqual(classify(dict(pause_sample(),top=top)),UNKNOWN)

    def test_pause_publication_and_gameplay_resume_have_fresh_sequence_and_time(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();reader.sample=pause_sample
            m=self.monitor(path,reader)
            try:
                m.poll();pause=PACKET.unpack(path.read_bytes())
                self.assertEqual(pause,(0x47564d31,1,64,PAUSE_MENU,11,22,33,44,1,100))
                reader.sample=lambda:sample(False);m.clock=lambda:200;m.poll()
                resumed=PACKET.unpack(path.read_bytes())
                self.assertEqual(resumed[:3],pause[:3])
                self.assertEqual(resumed[4:8],pause[4:8])
                self.assertEqual((resumed[3],resumed[-2],resumed[-1]),(GAMEPLAY,2,200))
                reader.sample=pause_sample;m.clock=lambda:300;m.poll()
                self.assertEqual(PACKET.unpack(path.read_bytes())[-2:],(3,300))
                rows=[json.loads(line) for line in path.with_suffix('.jsonl').read_text().splitlines()]
                self.assertEqual([row['mode'] for row in rows],[PAUSE_MENU,GAMEPLAY,PAUSE_MENU])
            finally:m.close()

    def test_semantic_gates(self):
        self.assertEqual(classify(sample()), DEATH_MENU)
        self.assertEqual(classify(sample(False)), GAMEPLAY)
        for key, value in [('consistent_reread', False), ('read_only', False), ('player_index', 1), ('input_block_count', 1)]:
            self.assertEqual(classify(dict(sample(), **{key:value})), UNKNOWN)
        for key in ('tag','cls','input_mode'):
            s=sample();s['top'][key]='unknown';self.assertEqual(classify(s), UNKNOWN)

    def test_nonmodal_last_player_index_is_not_local_player_identity(self):
        self.assertEqual(classify(dict(sample(False), player_index=-1)), GAMEPLAY)
        self.assertEqual(classify(dict(sample(), player_index=-1)), UNKNOWN)
        for value in (-2, 1, None, False, 0.0, '-1'):
            for death in (False, True):
                self.assertEqual(classify(dict(sample(death), player_index=value)), UNKNOWN)
        for key, value in [('consistent_reread', False), ('read_only', False), ('input_block_count', 1)]:
            self.assertEqual(classify(dict(sample(False), player_index=-1, **{key:value})), UNKNOWN)
        for key in ('name', 'cls', 'input_mode'):
            s=dict(sample(False), player_index=-1);s['top'][key]='unknown'
            self.assertEqual(classify(s), UNKNOWN)

    def test_nonmodal_recovery_packet_uses_new_observation(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.poll()  # Confirmed death menu, then one invalid observation.
            reader.sample=lambda:dict(sample(False), player_index=None)
            m.clock=lambda:200;m.poll()
            self.assertEqual(PACKET.unpack(path.read_bytes())[3], UNKNOWN)
            reader.sample=lambda:dict(sample(False), player_index=-1)
            m.clock=lambda:300;m.poll()
            packet=PACKET.unpack(path.read_bytes())
            self.assertEqual((packet[3],packet[-2],packet[-1]),(GAMEPLAY,3,300))
            m.close()

    def monitor(self, path, reader, **kwargs):
        game=dict(pid=11,creation_time=22,image='game');owner=dict(pid=33,creation_time=44,image='owner')
        return MenuDisplayMonitor(path,game,owner,lambda pid:game,lambda:reader,
                                  kwargs.get('reader_identity',lambda r:(11,22)),lambda:100,movie_display=kwargs.get("movie_display",False))

    def test_movie_opt_in_independent_churn_and_explicit_menu_priority(self):
        from menu_state import SnapshotChanged
        from menu_display_monitor import MOVIE_DISPLAY
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader()
            movie=Mock();movie.sample.return_value=dict(state='progress',last_advance_ns=90)
            with patch('menu_display_monitor.attested_movie_reader',return_value=movie) as attest:
                m=self.monitor(path,reader,movie_display=True)
                reader.sample=Mock(side_effect=SnapshotChanged('churn'))
                m.poll();packet=PACKET.unpack(path.read_bytes())
                self.assertEqual(packet[3],MOVIE_DISPLAY);self.assertEqual(packet[-1],90)
                self.assertEqual(m.last_menu_mode,ACQUISITION_WAIT);self.assertFalse(reader.closed)
                reader.sample=lambda:sample();m.poll()
                self.assertEqual(PACKET.unpack(path.read_bytes())[3],DEATH_MENU)
                reader.sample=Mock(side_effect=OSError('lost'));m.poll()
                self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)
                self.assertTrue(reader.closed);attest.assert_called_once();m.close()
            reader=Reader()
            with patch('menu_display_monitor.attested_movie_reader') as attest:
                m=self.monitor(path,reader);m.poll();attest.assert_not_called();m.close()

    def test_movie_attestation_reads_code_not_identity_dict(self):
        import hashlib
        from menu_display_monitor import attested_movie_reader
        reader=Mock();reader.r.base=0x140000000;reader.r.read.return_value=b'abcd'
        anchor=(0x123,4,hashlib.sha256(b'abcd').hexdigest())
        with patch('menu_display_monitor.MOVIE_CODE',(anchor,)):
            self.assertIsNotNone(attested_movie_reader(reader,dict(pid=11,creation_time=22)))
            reader.r.read.assert_called_once_with(0x140000123,4)
            for bad in (b'abc',b'abce'):
                reader.r.read.return_value=bad
                with self.assertRaises(RuntimeError):attested_movie_reader(reader,dict(pid=11,creation_time=22))
            reader.r.base+=1
            with self.assertRaises(RuntimeError):attested_movie_reader(reader,dict(pid=11,creation_time=22))

    def test_packet_and_stop(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.poll();self.assertEqual(PACKET.unpack(path.read_bytes()),(0x47564d31,1,64,2,11,22,33,44,1,100))
            m.poll();self.assertEqual(PACKET.unpack(path.read_bytes())[-2],2)
            m.close();self.assertTrue(reader.closed);self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)

    def test_wrong_creation_never_announces_menu(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader,reader_identity=lambda r:(11,23))
            m.poll();self.assertTrue(reader.closed);self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)
            m.close()

    def test_read_failure_invalidates_prior_menu(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.poll()
            def fail():raise RuntimeError('changed')
            reader.sample=fail;m.poll();self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)
            m.close()

    def test_real_typed_snapshot_churn_keeps_reader_with_sample_snapshot_wait_only(self):
        for type_name in ('SnapshotChanged','DialogStorageChanged','InventoryChanged','RegistrySnapshotChanged'):
            with self.subTest(type_name=type_name),tempfile.TemporaryDirectory() as d:
                path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
                from menu_state import SnapshotChanged, DialogStorageChanged, InventoryChanged, RegistrySnapshotChanged
                error_type={'SnapshotChanged':SnapshotChanged,'DialogStorageChanged':DialogStorageChanged,
                    'InventoryChanged':InventoryChanged,'RegistrySnapshotChanged':RegistrySnapshotChanged}[type_name]
                factory=Mock(return_value=reader);m.factory=factory
                reader.sample=Mock(side_effect=[pause_sample(),error_type('changed'),error_type('changed'),checkpoint_confirmation_sample()])
                try:
                    churn_mode=ACQUISITION_WAIT if type_name=='SnapshotChanged' else UNKNOWN
                    for call,(now,mode) in enumerate(((100,PAUSE_MENU),(200,churn_mode),(300,churn_mode),(400,PAUSE_MENU)),1):
                        m.clock=lambda now=now:now;m.poll()
                        packet=PACKET.unpack(path.read_bytes())
                        self.assertEqual((packet[3],packet[-2],packet[-1]),(mode,call,now))
                        self.assertEqual(reader.sample.call_count,call) # No immediate retry.
                        self.assertIs(m.reader,reader);self.assertFalse(reader.closed)
                        self.assertEqual(m.retry_ns,0);factory.assert_called_once()
                    rows=[json.loads(line) for line in path.with_suffix('.jsonl').read_text().splitlines()]
                    self.assertEqual(len(rows),3) # Repeated same churn does not increase log rate.
                    self.assertIsNone(rows[1]['state_evidence'])
                finally:m.close()
                self.assertTrue(reader.closed)

    def test_typed_churn_and_slow_recovery_keep_acquisition_start_timestamps(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            from menu_state import SnapshotChanged
            now=[100];m.clock=lambda:now[0]
            def changed():
                now[0]=2000000100
                raise SnapshotChanged('second snapshot attempt changed')
            try:
                reader.sample=Mock(side_effect=changed);m.poll()
                self.assertEqual((PACKET.unpack(path.read_bytes())[3],PACKET.unpack(path.read_bytes())[-1]),(ACQUISITION_WAIT,100))
                reader.sample.assert_called_once();self.assertIs(m.reader,reader)
                def slow():
                    now[0]+=2000000000 # Already older than the unchanged 1.5 s render TTL.
                    return pause_sample()
                began=now[0];reader.sample=Mock(side_effect=slow);m.poll()
                packet=PACKET.unpack(path.read_bytes())
                self.assertEqual((packet[3],packet[-1]),(PAUSE_MENU,began))
                self.assertGreater(now[0]-packet[-1],1500000000)
                reader.sample.assert_called_once()
                now[0]+=1;fresh=now[0];reader.sample=Mock(return_value=pause_sample());m.poll()
                self.assertEqual(PACKET.unpack(path.read_bytes())[-1],fresh)
                self.assertFalse(reader.closed)
            finally:m.close()

    def test_exact_snapshot_exception_outside_sample_call_remains_unknown_without_evidence(self):
        for phase in ('factory','reader_identity','classification'):
            with self.subTest(phase=phase),tempfile.TemporaryDirectory() as d:
                path=pathlib.Path(d)/'menu.bin';reader=Reader();reader.sample=Mock(return_value=sample(False))
                m=self.monitor(path,reader)
                from menu_state import SnapshotChanged
                try:
                    if phase=='factory':m.factory=Mock(side_effect=SnapshotChanged('factory churn'))
                    if phase=='reader_identity':m.reader_identity=Mock(side_effect=SnapshotChanged('identity churn'))
                    with patch('menu_display_monitor.classify',side_effect=SnapshotChanged('classification churn')) if phase=='classification' else patch('menu_display_monitor.classify',wraps=classify):
                        m.poll()
                    self.assertEqual(PACKET.unpack(path.read_bytes()),(0x47564d31,1,64,UNKNOWN,11,22,33,44,1,100))
                    row=json.loads(path.with_suffix('.jsonl').read_text().splitlines()[0])
                    self.assertEqual(row['mode'],UNKNOWN);self.assertIsNone(row['state_evidence'])
                    self.assertEqual(reader.sample.call_count,1 if phase=='classification' else 0)
                    self.assertFalse(reader.closed);self.assertEqual(m.retry_ns,0)
                finally:m.close()

    def test_plain_failures_and_typed_subclasses_still_close_and_back_off(self):
        with patch.object(sys,'path',[str(pathlib.Path(__file__).resolve().parents[1]/'game-io'),*sys.path]):
            from menu_state import SnapshotChanged, DialogStorageChanged, InventoryChanged, RegistrySnapshotChanged
        class DerivedSnapshotChanged(SnapshotChanged):pass
        class DerivedDialogStorageChanged(DialogStorageChanged):pass
        class DerivedInventoryChanged(InventoryChanged):pass
        class DerivedRegistrySnapshotChanged(RegistrySnapshotChanged):pass
        for error in (RuntimeError('SnapshotChanged'),RuntimeError('Object registry identity changed'),
                      RuntimeError('Registry changed before snapshot verification'),
                      OSError('read failed'),ValueError('bad registry'),UnicodeError('bad name'),
                      DerivedSnapshotChanged('changed'),DerivedDialogStorageChanged('changed'),DerivedInventoryChanged('changed'),
                      DerivedRegistrySnapshotChanged('changed')):
            with self.subTest(error=type(error).__name__),tempfile.TemporaryDirectory() as d:
                path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
                factory=Mock(return_value=reader);m.factory=factory
                reader.sample=Mock(side_effect=error)
                try:
                    m.clock=lambda:100;m.poll()
                    self.assertEqual(PACKET.unpack(path.read_bytes())[3],UNKNOWN)
                    self.assertTrue(reader.closed);self.assertIsNone(m.reader);self.assertEqual(m.retry_ns,1000000100)
                    reader.sample.assert_called_once();factory.assert_called_once()
                    m.clock=lambda:200;m.poll()
                    self.assertEqual((PACKET.unpack(path.read_bytes())[3],PACKET.unpack(path.read_bytes())[-1]),(UNKNOWN,200))
                    factory.assert_called_once();reader.sample.assert_called_once()
                finally:m.close()

    def test_replace_failure_preserves_packet_and_recovers_with_new_sample(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.poll();previous=path.read_bytes()
            with patch('menu_display_monitor.os.replace', side_effect=PermissionError('busy')):
                m.clock=lambda:200
                m.poll()
                self.assertEqual(path.read_bytes(),previous)
                self.assertFalse(path.with_suffix('.tmp').exists())
            reader.sample=lambda:sample(False)
            m.clock=lambda:300
            m.poll()
            packet=PACKET.unpack(path.read_bytes())
            self.assertEqual((packet[3],packet[-1]),(GAMEPLAY,300))
            events=[json.loads(line).get('event') for line in path.with_suffix('.jsonl').read_text().splitlines()]
            self.assertEqual(events.count('publish_failed'),1)
            self.assertEqual(events.count('publish_recovered'),1)
            m.close()

    def test_initial_failure_is_bounded_and_close_cleans_up(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            with patch('menu_display_monitor.os.replace', side_effect=PermissionError('busy')):
                for _ in range(20):m.poll()
                self.assertFalse(path.exists())
                m.close()
            self.assertTrue(reader.closed)
            self.assertTrue(m.log.closed)
            self.assertFalse(path.exists())
            self.assertFalse(path.with_suffix('.tmp').exists())
            rows=[json.loads(line) for line in path.with_suffix('.jsonl').read_text().splitlines()]
            self.assertEqual(len(rows),1)
            self.assertEqual(rows[0]['event'],'publish_failed')

    def test_game_identity_change_still_raises_when_publish_fails(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'menu.bin';reader=Reader();m=self.monitor(path,reader)
            m.identity=lambda pid:None
            with patch('menu_display_monitor.os.replace', side_effect=PermissionError('busy')):
                with self.assertRaisesRegex(RuntimeError,'game process changed'):m.poll()
                m.close()

if __name__=='__main__':unittest.main()
