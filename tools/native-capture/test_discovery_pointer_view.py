"""Immutable UI discovery transport; selected identities stay independently live."""
from array import array
import ctypes as c
import gc
import json
import os
from pathlib import Path
import struct
from types import SimpleNamespace
import types
import unittest
from unittest import mock

import start_capture
import native_class_discovery as native
import world_rebind_runtime as runtime
from resolve_camera import Resolver,DiscoveryRegistryChanged
from ui_observer_config import build_ui_observer_config,ui_observer_ini,UIConfigRejected,UIRegistryChanged
from anchor_config import build_anchor_config
from test_ui_observer_config import Fixture
from test_mounted_integration import MountedUIFixture

DLL=os.environ.get('GEARSVR_CLASS_DISCOVERY_TEST_DLL')


def pointer_view(data,count):
    helper=native.NativeClassDiscovery(Path('Unused.dll'),'a'*64)
    with mock.patch.object(helper,'warmup',return_value=True):return helper.pointer_view(data,count)


def ui_fixture(factory,view=True,batch=False,helper=None):
    r=factory();options={}
    if factory is MountedUIFixture:options['mounted_anchor']=build_anchor_config(r,sample_pose=False)[0];camera=r.resolve()
    else:camera=r.camera
    if view:r.discovery_pointer_view=helper.pointer_view if helper is not None else pointer_view
    if batch:
        identity=r.identity;r.identity=lambda address,reader=None:identity(address)
        r.CLASS_HINT_BLOCK_SIZE=Resolver.CLASS_HINT_BLOCK_SIZE
        r.discovery_class_candidates=types.MethodType(Resolver.discovery_class_candidates,r)
    if helper:
        data,count,_=struct.unpack('<Qii',r.read(r.names_header,16));r.name_pointers=tuple(r.u64(data+i*8)for i in range(count))
        r._native_class_discovery=helper.discover
    return r,camera,options


class PointerViewTests(unittest.TestCase):
    def test_view_preserves_unsigned_order_duplicates_and_immutable_owner(self):
        expected=(0,1,0x8000000000000000,0xffffffffffffffff,1)
        data=struct.pack('<5Q',*expected)
        view=pointer_view(data,len(expected))
        self.assertEqual(tuple(view),expected);self.assertTrue(view.readonly);self.assertIs(view.obj,data)
        with self.assertRaises(TypeError):view[0]=9
        self.assertEqual(tuple(pointer_view(b'',0)),())

    def test_unsupported_source_endianness_count_or_length_uses_original_decoder(self):
        data=bytes(16)
        for value,count in ((bytearray(data),2),(memoryview(data),2),(data,True),(data,2.),(data,-1),(data,4000000),(data,1)):
            with self.subTest(source=type(value).__name__,count=count):self.assertIsNone(pointer_view(value,count))
        with mock.patch('native_class_discovery.sys.byteorder','big'):self.assertIsNone(pointer_view(data,2))
        helper=native.NativeClassDiscovery(Path('Unused.dll'),'a'*64)
        with mock.patch.object(helper,'warmup',return_value=False):self.assertIsNone(helper.pointer_view(data,2))

    def test_native_transport_preserves_all_values_through_callback_and_collection(self):
        values=(0,0x8000000000000000,7,7,0xffffffffffffffff)
        inputs=(list(values),pointer_view(struct.pack('<5Q',*values),5),
                memoryview(bytearray(struct.pack('<5Q',*values))).cast('Q'),
                memoryview(bytearray(struct.pack('<5Q',*values))).cast('Q').toreadonly(),
                memoryview(struct.pack('<10Q',*(values+values))).cast('Q')[::2],memoryview(b'\x00\xff\x03'))
        for pointers in inputs:
            expected=tuple(array('Q',pointers));captured=[]
            reader=SimpleNamespace(profile=dict(object_class=80,object_name=72,name_entry_index=0,name_entry_text=20),
                name_pointers=(3,4),names_header=0x1234,read=mock.Mock(return_value=b'live'))
            helper=native.NativeClassDiscovery(Path('Unused.dll'),'a'*64)
            def function(version,objects,count,names,ncount,header,read,context,output,capacity,written):
                captured.append(tuple((c.c_uint64*count).from_address(objects)));gc.collect()
                buffer=c.create_string_buffer(4);self.assertEqual(read(None,0x1234,4,c.addressof(buffer)),1)
                self.assertEqual(buffer.raw,b'live');self.assertEqual(tuple((c.c_uint64*count).from_address(objects)),expected)
                self.assertEqual(tuple((c.c_uint64*ncount).from_address(names)),(3,4))
                (c.c_uint64*capacity).from_address(output)[0]=expected[-1];written._obj.value=1
                return 0
            helper.function=function;helper.attempted=True
            with self.subTest(source=type(pointers).__name__):
                self.assertEqual(tuple(helper.discover(reader,pointers)),(expected[-1],));self.assertEqual(captured,[expected])

    def test_view_callback_registry_change_and_cancellation_remain_terminal(self):
        pointers=pointer_view(struct.pack('<2Q',1,2),2)
        for fault in ('registry','cancel'):
            error=DiscoveryRegistryChanged('Changed')if fault=='registry'else RuntimeError('Cancelled')
            reader=SimpleNamespace(profile=dict(object_class=80,object_name=72,name_entry_index=0,name_entry_text=20),
                name_pointers=(3,),names_header=0x1234,read=mock.Mock(side_effect=error))
            helper=native.NativeClassDiscovery(Path('Unused.dll'),'a'*64)
            def function(version,objects,count,names,ncount,header,read,context,output,capacity,written):
                buffer=c.create_string_buffer(4);self.assertEqual(read(None,0x1234,4,c.addressof(buffer)),-1);return 0
            helper.function=function;helper.attempted=True
            check=mock.Mock(side_effect=error)if fault=='cancel'else None
            with self.subTest(fault=fault),self.assertRaises(type(error))as rejected:helper.discover(reader,pointers,check)
            self.assertIs(rejected.exception,error)

    def test_full_ui_wire_and_read_schedule_equal_in_ordinary_and_python_batch_paths(self):
        for factory in (Fixture,MountedUIFixture):
            for batch in(False,True):
                results=[]
                for view in(False,True):
                    r,camera,options=ui_fixture(factory,view,batch);read=r.read;calls=[]
                    def observing(address,size):calls.append((address,size));return read(address,size)
                    r.read=observing;config,_=build_ui_observer_config(r,camera,**options)
                    results.append((config,ui_observer_ini(config),calls))
                with self.subTest(factory=factory.__name__,batch=batch):self.assertEqual(*results)

    def test_stable_table_is_checked_before_view_and_length_errors_stay_hard(self):
        for fault in ('changed','short','long'):
            r,camera,options=ui_fixture(Fixture);read=r.read;hits=0;view=mock.Mock(wraps=r.discovery_pointer_view)
            r.discovery_pointer_view=view
            def changing(address,size):
                nonlocal hits
                value=read(address,size)
                if (address,size)==(r.od,1101*8):
                    hits+=1
                    if fault=='changed'and hits==2:return value[:-1]+b'\x01'
                    if fault=='short':return value[:-8]
                    if fault=='long':return value+bytes(8)
                return value
            r.read=changing
            with self.subTest(fault=fault),self.assertRaises(UIConfigRejected if fault=='changed'else struct.error):
                build_ui_observer_config(r,camera)
            self.assertEqual(hits,2)
            if fault=='changed':view.assert_not_called()

    def test_selected_class_still_rejects_change_after_view_discovery(self):
        for factory in(Fixture,MountedUIFixture):
            r,camera,options=ui_fixture(factory,True,True);discover=r.discovery_class_candidates
            def changing(pointers):
                yield from discover(pointers)
                r.put(r.classes['Canvas']+76,struct.pack('<I',1))
            r.discovery_class_candidates=changing
            with self.subTest(factory=factory.__name__),self.assertRaises(UIRegistryChanged):
                build_ui_observer_config(r,camera,**options)

    def test_unavailable_native_export_keeps_the_original_python_discovery(self):
        results=[]
        for unavailable in(False,True):
            helper=native.NativeClassDiscovery(Path('Unused.dll'),'a'*64)if unavailable else None
            r,camera,options=ui_fixture(Fixture,view=unavailable,batch=True,helper=helper)
            with mock.patch.object(helper,'warmup',return_value=False)if helper else mock.patch.object(r,'unused',create=True):
                config,_=build_ui_observer_config(r,camera)
            results.append((config,ui_observer_ini(config)))
        self.assertEqual(*results)

    @unittest.skipUnless(DLL,'Pass the frozen default DLL for native offline checks')
    def test_real_native_class_export_and_complete_ui_wire_match_for_views(self):
        manifest=json.loads((start_capture.ROOT/'tools/vr-launcher/manifest.json').read_text())
        for factory in(Fixture,MountedUIFixture):
            results=[]
            for view in(False,True):
                helper=native.NativeClassDiscovery(Path(DLL),manifest['native_sha256']);self.assertTrue(helper.warmup())
                r,camera,options=ui_fixture(factory,view,True,helper);read=r.read;calls=[]
                if view:
                    del r.discovery_pointer_view
                    helper.pointer_view=mock.Mock(wraps=helper.pointer_view)
                    adapter=runtime.RuntimePreparation.__new__(runtime.RuntimePreparation)
                    adapter._native_discovery=helper;adapter._camera_hint=None;adapter._cinematic_class_hint=None
                    with mock.patch.object(r,'resolve',create=True,return_value=camera):adapter._preparation_camera(r)
                def observing(address,size):calls.append((address,size));return read(address,size)
                r.read=observing;config,_=build_ui_observer_config(r,camera,**options)
                if view:helper.pointer_view.assert_called_once()
                results.append((config,ui_observer_ini(config),calls))
            with self.subTest(factory=factory.__name__):self.assertEqual(*results)


if __name__=='__main__':unittest.main()
