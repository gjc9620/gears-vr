"""Synthetic memory/navigation tests only. Never sends OS messages."""
import copy
from pathlib import Path
import struct
import sys
import unittest
from unittest import mock
from types import SimpleNamespace
sys.path.insert(0,str(Path(__file__).resolve().parent))
import chapter_selection as chapter

class Fixture:
    def __init__(self):
        self.memory={};self.identities={};self.kinds={};self.props={};self.time=0.;self.sent=[];self.samples=0
        self.r=SimpleNamespace(ident_cache={},names_cache={},objects_header=0x100,names_header=0x120,
            struct_type=self.struct_type,property=self.property,class_name=lambda obj:self.kinds[obj['address']],u64=lambda at:struct.unpack('<Q',self.r.read(at,8))[0])
        self.r.read=self.read
        self.reader=SimpleNamespace(r=self.r,properties={},sample=self.sample,integer=self.integer,prop=self.prop,array=lambda *args:(0x3000,3,dict(address=0x2000)))
        self.put(0x100,struct.pack('<Qii',0x9000,1001,2000));self.put(0x120,struct.pack('<Qii',0xa000,1001,2000))
        self.add(0x1000,'scene','WarUIScene',0x800);self.add(0x1100,'lstMaps','UIList',0x1000);self.add(0x1200,'component','Component',0x1100)
        self.put(0x1140,struct.pack('<i',0));self.put(0x1148,struct.pack('<Q',0x1200));self.put(0x2070,struct.pack('<II',1,32))
        for i,values in enumerate([dict(FriendlyName='FIRST',MapName='a',ChapterPointID='LoadChapter=0'),dict(FriendlyName='SECOND',MapName='b',ChapterPointID='LoadChapter=1'),chapter.HAMMER]):
            address=0x4000+i*0x100;self.add(address,'provider'+str(i),'UIDataProvider_WarfareMaps',0x1200)
            self.put(0x3000+i*32+8,struct.pack('<Q',address))
            for j,name in enumerate(chapter.FIELDS):
                self.props[address,name]=dict(address=0x6000+i*0x100+j*16,offset=0x30+j*16,type='StrProperty')
                text=(values[name]+'\0').encode('utf-16-le');pointer=0x8000+i*0x100+j*0x40
                self.put(pointer,text);self.put(address+0x30+j*16,struct.pack('<Qii',pointer,len(text)//2,len(text)//2))
        self.props[0x1100,'CellDataComponent']=dict(address=0x6100,offset=0x48,type='ComponentProperty')
        self.source_read=self.r.read
    def put(self,address,raw):self.memory.update((address+i,b)for i,b in enumerate(raw))
    def add(self,address,name,kind,outer):
        self.identities[address]=dict(address=address,name=name,number=0,cls=address+0x100000,outer=outer,internal_index=address//8)
        self.kinds[address]=kind;self.put(address,b'identity')
    def read(self,address,size):return bytes(self.memory[address+i]for i in range(size))
    def registered(self,r,address):r.read(address,8);return copy.deepcopy(self.identities[address])
    def prop(self,obj,name,kind):
        result=self.props[obj['address'],name];chapter.require(result['type']==kind,'bad fixture reflected property');return result
    def property(self,owner,name):
        return dict(address=0x2100 if name=='DataSource' else 0x2200,offset=0 if name=='DataSource' else 8,
                    type='StructProperty'if name=='DataSource'else'InterfaceProperty')
    def struct_type(self,prop):return 0x2500 if prop['address']==0x2000 else 0x2600
    def integer(self,obj,name):return struct.unpack('<i',self.r.read(obj['address']+0x40,4))[0]
    def sample(self):
        self.samples+=1
        def summary(address):
            obj=self.identities[address]
            return dict(address=hex(address),name=obj['name'],number=0,cls=self.kinds[address],outer=hex(obj['outer']))
        top=summary(0x1000);top.update(tag='DeltaSelectMap',input_mode=1)
        leaf=summary(0x1100);leaf['states']=['UIState_Enabled','UIState_Focused']
        return dict(pid=7,read_only=True,consistent_reread=True,player_index=0,input_block_count=0,
                    client='0x99',top=top,focused_leaf=leaf,focus_chain=[leaf],active_scenes=[top])
    def send(self,action,window):
        self.sent.append((action,window));index=self.integer(self.identities[0x1100],'Index')
        if action!='confirm':self.put(0x1140,struct.pack('<i',index+(1 if action=='down' else -1)))
    def capture(self):return chapter.capture_chapters(self.reader,(7,70),lambda:None,10,now=lambda:self.time,identity=lambda r:(7,70))
    def select(self,**kwargs):
        options=dict(verify=lambda:None,window=lambda:(7,77),send=self.send,deadline=10,
                     now=lambda:self.time,sleep=lambda sec:setattr(self,'time',self.time+sec),identity=lambda r:(7,70))
        options.update(kwargs)
        return chapter.select_chapter(self.reader,(7,70),dict(chapter.HAMMER),**options)

class ChapterTests(unittest.TestCase):
    def setUp(self):
        self.f=Fixture();self.patch=mock.patch('chapter_selection.registered',side_effect=self.f.registered);self.patch.start();self.addCleanup(self.patch.stop)
    def test_current_reflected_inventory_and_down_then_confirm(self):
        rows=self.f.capture();self.assertEqual(rows['rows'][2]['values'],chapter.HAMMER)
        result=self.f.select();self.assertEqual([a for a,w in self.f.sent],['down','down','confirm'])
        self.assertTrue(result['chapter_enter_dispatched']);self.assertFalse(result['chapter_load_completed'])
        self.assertTrue(result['next_screen_requires_independent_review']);self.assertIs(self.f.r.read,self.f.source_read)
    def test_provider_order_is_live_not_historical_row5(self):
        first=self.f.read(0x3008,8);last=self.f.read(0x3048,8)
        self.f.put(0x3008,last);self.f.put(0x3048,first);self.f.put(0x1140,struct.pack('<i',2))
        self.f.select();self.assertEqual([a for a,w in self.f.sent],['up','up','confirm'])
    def test_missing_or_duplicate_target_no_input(self):
        for duplicate in (False,True):
            f=Fixture()
            with self.subTest(duplicate=duplicate),mock.patch('chapter_selection.registered',side_effect=f.registered):
                if duplicate:
                    for offset in (0x30,0x40,0x50):f.put(0x4100+offset,f.read(0x4200+offset,16))
                else:f.put(0x8200,('ABSENT\0').encode('utf-16-le'))
                with self.assertRaisesRegex(chapter.ChapterSelectionRejected,'exactly one matching'):f.select()
                self.assertEqual(f.sent,[])
    def test_wrong_screen_confirmation_or_blocked_rejects(self):
        sample=self.f.sample()
        for field,value in [('input_block_count',1),('player_index',-1),('consistent_reread',False)]:
            bad=copy.deepcopy(sample);bad[field]=value
            with mock.patch.object(self.f.reader,'sample',return_value=bad),self.assertRaises(chapter.ChapterSelectionRejected):self.f.select()
        sample['top']['tag']='WarfareConfirmation'
        with mock.patch.object(self.f.reader,'sample',return_value=sample),self.assertRaises(chapter.ChapterSelectionRejected):self.f.select()
        self.assertEqual(self.f.sent,[])
    def test_provider_mutation_between_final_captures_no_input(self):
        original=chapter.capture_chapters;count=0
        def capture(*args,**kwargs):
            nonlocal count
            count+=1
            if count==2:self.f.identities[0x4200]['number']=1
            return original(*args,**kwargs)
        with mock.patch('chapter_selection.capture_chapters',side_effect=capture),self.assertRaises(chapter.ChapterSelectionRejected):self.f.select()
        self.assertEqual(self.f.sent,[])
    def test_same_read_final_byte_drift_rejects_and_restores(self):
        source=self.f.source_read;count=0
        def read(address,size):
            nonlocal count
            raw=source(address,size)
            if (address,size)==(0x3008,8):
                count+=1
                if count==2:return struct.pack('<Q',0x4100)
            return raw
        self.f.r.read=read
        with self.assertRaisesRegex(chapter.ChapterSelectionRejected,'final reread'):self.f.capture()
        self.assertIs(self.f.r.read,read);self.assertEqual(self.f.sent,[])
    def test_bad_stride_type_and_string_bounds_reject_no_input(self):
        self.f.put(0x2070,struct.pack('<II',1,8))
        with self.assertRaises(chapter.ChapterSelectionRejected):self.f.capture()
        self.f.put(0x2070,struct.pack('<II',1,32));self.f.put(0x4030,struct.pack('<Qii',0x8000,4000,4000))
        with self.assertRaises(chapter.ChapterSelectionRejected):self.f.capture()
        self.assertEqual(self.f.sent,[])
    def test_wrong_identity_window_deadline_and_action_budget(self):
        for options in [dict(identity=lambda r:(7,71)),dict(window=lambda:(8,77)),dict(deadline=0)]:
            with self.subTest(options=options),self.assertRaises(chapter.ChapterSelectionRejected):self.f.select(**options)
        with self.assertRaisesRegex(chapter.ChapterSelectionRejected,'budget'):self.f.select(max_actions=1)
        self.assertEqual([a for a,w in self.f.sent],['down'])
    def test_reflected_provider_kind_window_rebind_and_expiry_stop_before_input(self):
        original=self.f.r.property
        def wrong(owner,name):
            result=original(owner,name)
            if name=='DataSourceProvider':result['type']='ObjectProperty'
            return result
        self.f.r.property=wrong
        with self.assertRaises(chapter.ChapterSelectionRejected):self.f.select()
        self.f.r.property=original
        calls=0
        def window():
            nonlocal calls
            calls+=1
            return (7,77 if calls==1 else 78)
        with self.assertRaises(chapter.ChapterSelectionRejected):self.f.select(window=window)
        source=self.f.r.read
        def timed(address,size):self.f.time+=1;return source(address,size)
        self.f.r.read=timed
        with self.assertRaisesRegex(chapter.ChapterSelectionRejected,'deadline'):self.f.capture()
        self.assertIs(self.f.r.read,timed);self.assertEqual(self.f.sent,[])

    def test_unacknowledged_or_uncertain_key_is_never_retried(self):
        def no_ack(action,window):self.f.sent.append((action,window))
        with self.assertRaisesRegex(chapter.ChapterSelectionRejected,'acknowledgement'):self.f.select(send=no_ack)
        self.assertEqual(len(self.f.sent),1);self.f.sent=[]
        def partial(action,window):self.f.sent.append((action,window));raise OSError('uncertain key pair')
        with self.assertRaises(OSError):self.f.select(send=partial)
        self.assertEqual(len(self.f.sent),1)

    def test_delayed_arrow_ack_old_index_until_exact_expected(self):
        pending=[None];reads=[0];original=self.f.sample
        def send(action,window):
            self.f.sent.append((action,window))
            if action!='confirm':pending[0]=self.f.integer(self.f.identities[0x1100],'Index')+1;reads[0]=0
        def sample():
            if pending[0]is not None:
                reads[0]+=1
                if reads[0]>=5:self.f.put(0x1140,struct.pack('<i',pending[0]));pending[0]=None
            return original()
        self.f.reader.sample=sample
        result=self.f.select(send=send)
        self.assertEqual([a for a,w in self.f.sent],['down','down','confirm'])
        self.assertTrue(all(a['arrow_ack']['reads']>=3 and a['arrow_ack']['acknowledged']for a in result['actions'][:-1]))

    def test_ack_timeout_retains_indices_and_only_one_key(self):
        def send(action,window):self.f.sent.append((action,window))
        with self.assertRaises(chapter.ChapterSelectionRejected)as error:self.f.select(send=send)
        evidence=error.exception.detail['arrow_ack']
        self.assertEqual((evidence['from_index'],evidence['expected_index'],evidence['observed_index']),(0,1,0))
        self.assertEqual(len(self.f.sent),1);self.assertGreaterEqual(self.f.time,5)

    def test_ack_skip_reverse_and_provider_drift_fatal(self):
        for change in ('skip','reverse','provider','fingerprint','window'):
            with self.subTest(change=change):
                f=Fixture();window=[(7,77)]
                if change=='reverse':f.put(0x1140,struct.pack('<i',1))
                def send(action,target):
                    f.sent.append((action,target))
                    if change=='skip':f.put(0x1140,struct.pack('<i',2))
                    elif change=='reverse':f.put(0x1140,struct.pack('<i',0))
                    elif change=='provider':f.identities[0x4200]['number']=1
                    elif change=='fingerprint':f.identities[0x1000]['outer']=0x999
                    else:window[0]=(7,78)
                with mock.patch('chapter_selection.registered',side_effect=f.registered),self.assertRaises(chapter.ChapterSelectionRejected):
                    f.select(send=send,window=lambda:window[0])
                self.assertEqual(len(f.sent),1)

    def test_unknown_snapshot_error_is_fatal_not_retried(self):
        calls=[0];evidence={};baseline=dict(index=0,rows=[],fingerprint='same')
        def snapshot():calls[0]+=1;raise ValueError('unknown data')
        with self.assertRaisesRegex(ValueError,'unknown data')as error:
            chapter.wait_arrow_ack(snapshot,baseline,1,(7,77),lambda:(7,77),5,evidence,now=lambda:0,sleep=lambda sec:None)
        self.assertEqual(calls[0],1);self.assertIsNone(error.exception.detail['arrow_ack']['observed_index'])

if __name__=='__main__':unittest.main()
