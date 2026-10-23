"""Real Resolver preparation with synthetic file/module/read APIs; no process."""
import ctypes
import hashlib
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

from start_capture import Resolver
import resolve_camera as camera


class RegistryPreparationTests(unittest.TestCase):
    def setUp(self):
        directory=tempfile.TemporaryDirectory();self.addCleanup(directory.cleanup)
        self.exe=Path(directory.name)/'GOWDE-Steam.exe';self.exe.write_bytes(b'synthetic executable identity')
        self.base=0x140000000;self.names_data=0x100000;self.objects_data=0x200000

    def registry(self,count=1001,objects=1003):
        pattern=(0x40000,0,0x123456789abcdef,0xffffffffffffffff,17,17,0x100000001)
        self.names=tuple(pattern[i%len(pattern)] for i in range(count))
        self.objects=tuple(pattern[-1-i%len(pattern)] for i in range(objects))
        self.spans={self.base:b'MZ',self.base+0x100:struct.pack('<Qii',self.names_data,count,count),
            self.base+0x200:struct.pack('<Qii',self.objects_data,objects,objects),
            self.names_data:struct.pack('<'+'Q'*count,*self.names),self.objects_data:struct.pack('<'+'Q'*objects,*self.objects)}
        self.reads=[]
        def read(address,size):self.reads.append((address,size));return self.spans.get(address,b'')[:size]
        r=Resolver.__new__(Resolver);r.p=SimpleNamespace(handle=7,read=read);r.pid=123
        r.profile=dict(sha256=hashlib.sha256(self.exe.read_bytes()).hexdigest(),names_rva='0x100',objects_rva='0x200')
        r.name=mock.Mock(return_value='None');r.names_cache={};r.ident_cache={};r._field_hint_links=None
        return r

    def prepare(self,r):
        def image(handle,flags,buffer,length):buffer.value=str(self.exe);length._obj.value=len(str(self.exe));return True
        def modules(handle,values,size,required,mode):values[0]=self.base;required._obj.value=ctypes.sizeof(camera.w.HMODULE);return True
        with mock.patch.object(camera.k,'QueryFullProcessImageNameW',side_effect=image),\
             mock.patch.object(camera.psapi,'EnumProcessModulesEx',side_effect=modules):r.prepare()

    def test_all_pointer_values_order_duplicates_and_bounded_format_objects(self):
        r=self.registry();unpack=struct.unpack
        with mock.patch.object(camera.struct,'unpack',wraps=unpack) as decode:self.prepare(r)
        self.assertIs(type(r.name_pointers),tuple);self.assertIs(type(r.object_pointers),tuple)
        self.assertEqual(r.name_pointers,self.names);self.assertEqual(r.object_pointers,self.objects)
        self.assertEqual(self.reads,[(self.base,2),(self.base+0x100,16),(self.base+0x200,16),
            (self.names_data,len(self.names)*8),(self.objects_data,len(self.objects)*8)])
        arrays=[call.args for call in decode.call_args_list if len(call.args[1])>16]
        self.assertEqual(len(arrays),2)
        for fmt,raw in arrays:
            self.assertLessEqual(len(fmt),32);self.assertLessEqual(struct.Struct(fmt).__sizeof__(),1024)
            self.assertEqual(struct.calcsize(fmt),len(raw))
        r.name.assert_called_once_with(0)

    def test_registry_bounds_reject_before_pointer_array_reads(self):
        for label,offset,data in (('name',0x100,self.names_data),('object',0x200,self.objects_data)):
            for count,capacity in ((1000,1000),(-1,1001),(1001,1000),(1001,4000000)):
                r=self.registry();self.spans[self.base+offset]=struct.pack('<Qii',data,count,capacity)
                with self.assertRaisesRegex(RuntimeError,'Invalid '+label+' array header'):self.prepare(r)
                self.assertFalse(any(address in (self.names_data,self.objects_data)for address,_ in self.reads))
                r.name.assert_not_called()

    def test_short_pointer_buffers_still_reject(self):
        for address in (self.names_data,self.objects_data):
            r=self.registry();self.spans[address]=self.spans[address][:-1]
            with self.assertRaisesRegex(RuntimeError,'Unreadable address'):self.prepare(r)
            r.name.assert_not_called()

    def test_executable_hash_and_name_zero_validation_remain_required(self):
        r=self.registry();r.profile['sha256']='0'*64
        with self.assertRaisesRegex(RuntimeError,'Executable hash differs'):self.prepare(r)
        self.assertEqual(self.reads,[])
        r=self.registry();r.name.return_value='Changed'
        with self.assertRaisesRegex(RuntimeError,'Name registry zero entry'):self.prepare(r)


if __name__=='__main__':unittest.main()
