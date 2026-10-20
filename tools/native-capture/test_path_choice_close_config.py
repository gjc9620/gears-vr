"""Pure same-acquisition CloseScene sidecar tests, no process/window."""
import copy
import struct
import unittest
from unittest import mock

import path_choice_close_config as close
import test_path_choice_position_config as fixtures
from test_path_choice_trial_config import fixture as owner_fixture

def fixture():
    def with_scene_client_class():
        s=owner_fixture();ids={row['identity']['name']:row['identity'] for row in s['classes']}
        ni=len(s['name_entries']);identity=dict(address=0xa10000,internal_index=len(s['identities']),name_index=ni,
            name='UISceneClient',number=0,cls=ids['Class']['address'],outer=0x900000)
        s['identities'].append(identity);s['classes'].append(dict(identity=identity,super=ids['Object']['address']))
        s['name_entries'].append(dict(index=ni,address=0xe10000,text='UISceneClient'))
        next(row for row in s['classes'] if row['identity']['name']=='GameUISceneClient')['super']=identity['address']
        return s
    with mock.patch.object(fixtures,'owner_fixture',with_scene_client_class):sample,proof=fixtures.fixture()
    snapshot=sample['scene_owner'];declaring=next(row['identity'] for row in snapshot['classes'] if row['identity']['name']=='UISceneClient')
    function=dict(address=0xd00000,internal_index=900,name='CloseScene',number=0,cls=0xd10000,outer=declaring['address'])
    raw=bytearray(304);struct.pack_into('<I',raw,8,900);struct.pack_into('<QQQ',raw,64,function['outer'],100,function['cls'])
    struct.pack_into('<Q',raw,0x114,snapshot['module_base']+0x34698c)
    meta=dict(read_only=True,consistent_final_reread=True,process_owner_verified=True,native_called=False,game_memory_written=False,
        pid=123,creation_time=456,module_base=hex(snapshot['module_base']),client=copy.deepcopy(snapshot['roles']['client']),
        owner_class=copy.deepcopy(declaring),function=function,header_hex=raw.hex(),owned_field_chain=[function],
        session_binding=dict(game=dict(pid=123,creation_time=456),phase='running',native_started_qpc=789))
    return sample,proof,meta

class Tests(unittest.TestCase):
    def test_separate_request_same_nonce_and_explicit_semantic_descriptor(self):
        sample,proof,meta=fixture();original=copy.deepcopy((sample,proof,meta))
        text=close.build(sample,proof,meta,**fixtures.SESSION)
        fields=dict(line.split('=',1) for line in text.splitlines()[1:]);active=sample['scene_owner']['arrays']['active_scenes']
        prop=next(p for p in sample['scene_owner']['properties'] if p['identity']['address']==active['property'])
        self.assertEqual(fields['nonce'],'1000');self.assertEqual(fields['duration_ms'],'8000')
        self.assertEqual(int(fields['active_header'],0),sample['scene_owner']['roles']['client']['address']+prop['offset'])
        self.assertEqual([int(fields['member_'+str(i)],0) for i in range(active['count'])],active['members'])
        self.assertEqual(len(fields),21+active['count']);self.assertEqual((sample,proof,meta),original)
    def test_rejects_stale_foreign_and_unknown_receipts(self):
        for key,value in [('pid',124),('creation_time',457),('read_only',False),('native_called',True),
                          ('consistent_final_reread',False),('process_owner_verified',False),('module_base','0x180000000')]:
            sample,proof,meta=fixture();meta[key]=value
            with self.assertRaises(ValueError):close.build(sample,proof,meta,**fixtures.SESSION)
        sample,proof,meta=fixture();meta['session_binding']['native_started_qpc']=790
        with self.assertRaises(ValueError):close.build(sample,proof,meta,**fixtures.SESSION)
    def test_function_metadata_and_current_native_pointer_must_match(self):
        for offset in (8,64,76,80,0x114):
            sample,proof,meta=fixture();raw=bytearray.fromhex(meta['header_hex']);raw[offset]^=1;meta['header_hex']=raw.hex()
            with self.assertRaises(ValueError):close.build(sample,proof,meta,**fixtures.SESSION)
        sample,proof,meta=fixture();meta['function']['name']='Other'
        with self.assertRaises(ValueError):close.build(sample,proof,meta,**fixtures.SESSION)
        sample,proof,meta=fixture();meta['owned_field_chain']=[]
        with self.assertRaises(ValueError):close.build(sample,proof,meta,**fixtures.SESSION)
    def test_position_proof_and_descriptor_bounds_are_not_relaxed(self):
        sample,proof,meta=fixture();proof['checks'].clear()
        with self.assertRaises(ValueError):close.build(sample,proof,meta,**fixtures.SESSION)
        for duration in (0,8001,True,1.5):
            sample,proof,meta=fixture()
            with self.assertRaises(ValueError):close.build(sample,proof,meta,observer_duration_ms=duration,**fixtures.SESSION)

if __name__=='__main__':unittest.main()
