"""Native candidate export on synthetic memory only; never opens a process."""
import hashlib
import os
from pathlib import Path
import struct
import types
import unittest
from unittest import mock

from test_ui_observer_config import Fixture,build_ui_observer_config,ui_observer_ini
from test_mounted_integration import MountedUIFixture
from anchor_config import build_anchor_config
from resolve_camera import Resolver,DiscoveryRegistryChanged
from native_class_discovery import NativeClassDiscovery

DLL=os.environ.get('GEARSVR_CLASS_DISCOVERY_TEST_DLL')


def large_fixture(count=100000):
    r=Resolver.__new__(Resolver)
    r.profile=dict(object_internal_index=8,object_outer=64,object_name=72,object_class=80,
                   name_entry_index=0,name_entry_text=20)
    r.ident_cache={};r.names_cache={};r._field_hint_links=None
    end=0x10000+count*0x60;name_start=(end+0x1ffff)//65536*65536
    data=bytearray(name_start+65536);r.names_header=name_start;r.name_pointers=tuple(name_start+0x100+i*0x100 for i in range(4))
    texts=('Class','Pawn','Canvas','Instance');r.name=texts.__getitem__;calls=[0]
    def obj(a,cls,name,index):
        struct.pack_into('<I',data,a+8,index);struct.pack_into('<IIQ',data,a+72,name,0,cls)
    for i,a in enumerate((0x1000,0x1100,0x1200)):obj(a,0x1000,i,i)
    pointers=[0x1000,0x1100,0x1200]
    for i in range(count):
        a=0x10000+i*0x60;obj(a,0x1100,3,i+3);pointers.append(a)
    for i,a in enumerate(r.name_pointers):
        struct.pack_into('<I',data,a,i);raw=(texts[i]+'\0').encode('utf-16-le');data[a+20:a+20+len(raw)]=raw
    def read(a,n):
        calls[0]+=1
        if a<0 or a+n>len(data):raise OSError('Unmapped synthetic span')
        return bytes(data[a:a+n])
    r.read=read
    return r,pointers,data,calls


class FallbackTests(unittest.TestCase):
    def test_wrong_hash_never_loads_code(self):
        import tempfile
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'Fixture.dll';path.write_bytes(b'fixture')
            reader=NativeClassDiscovery(path,'0'*64)
            with mock.patch('native_class_discovery.c.WinDLL') as load:self.assertFalse(reader.warmup())
            load.assert_not_called();self.assertEqual(reader.status,'hash_mismatch')

    def test_unavailable_export_retains_python_discovery(self):
        r,pointers,_,_=large_fixture(50)
        expected=list(r.discovery_class_candidates(pointers));r.ident_cache.clear()
        r._native_class_discovery=lambda *_:None
        self.assertEqual(list(r.discovery_class_candidates(pointers)),expected)


@unittest.skipUnless(DLL,'Pass an explicitly verified candidate DLL for native offline checks')
class NativeTests(unittest.TestCase):
    def helper(self):
        path=Path(DLL).resolve();return NativeClassDiscovery(path,hashlib.sha256(path.read_bytes()).hexdigest())

    def test_same_complete_candidates_including_duplicates(self):
        r,pointers,_,_=large_fixture(1000);pointers.extend([0,0x1100])
        expected=list(r.discovery_class_candidates(pointers));r.ident_cache.clear()
        helper=self.helper();r._native_class_discovery=helper.discover
        self.assertEqual(list(r.discovery_class_candidates(pointers)),expected);self.assertEqual(helper.status,'available')

    def test_complete_ui_wire_equal(self):
        for factory in (Fixture,MountedUIFixture):
            rows=[]
            for enabled in (False,True):
                r=factory();r.CLASS_HINT_BLOCK_SIZE=65536;options={}
                if factory is MountedUIFixture:
                    options['mounted_anchor']=build_anchor_config(r,sample_pose=False)[0]
                    camera=r.resolve()
                else:camera=r.camera
                data,count,_=struct.unpack('<Qii',r.read(r.names_header,16));names=[]
                for index in range(count):
                    try:names.append(r.u64(data+index*8))
                    except (RuntimeError,OSError):names.append(0)
                r.name_pointers=tuple(names)
                identity=r.identity
                r.identity=lambda address,reader=None,_identity=identity:_identity(address)
                r.discovery_class_candidates=types.MethodType(Resolver.discovery_class_candidates,r)
                if enabled:r._native_class_discovery=self.helper().discover
                c,_=build_ui_observer_config(r,camera,**options);rows.append((c,ui_observer_ini(c)))
            self.assertEqual(rows[0],rows[1])

    def test_callback_registry_change_and_cancellation_propagate(self):
        for registry in (False,True):
            r,pointers,_,_=large_fixture(100);helper=self.helper()
            if registry:
                r.read=mock.Mock(side_effect=DiscoveryRegistryChanged('changed'))
                with self.assertRaises(DiscoveryRegistryChanged):helper.discover(r,pointers)
            else:
                check=mock.Mock(side_effect=RuntimeError('cancelled'))
                with self.assertRaisesRegex(RuntimeError,'cancelled'):helper.discover(r,pointers,check)

    def test_unknown_layout_uses_python_fallback(self):
        r,pointers,_,_=large_fixture(100);r.profile['name_entry_text']=24
        helper=self.helper();self.assertIsNone(helper.discover(r,pointers));self.assertFalse(helper.attempted)

    def test_new_name_slots_are_fresh_and_registry_change_rejects(self):
        for changed in (False,True):
            r,pointers,data,_=large_fixture(100);helper=self.helper()
            header=r.names_header;table=header+0x4000;entry=header+0x2000
            struct.pack_into('<Qii',data,header,table,1101,1101)
            struct.pack_into('<Q',data,table+4*8,entry);struct.pack_into('<I',data,entry,4)
            data[entry+20:entry+32]='Class\0'.encode('utf-16-le');struct.pack_into('<I',data,0x1000+72,4)
            r.name=types.MethodType(Resolver.name,r);read=r.read
            def observing(a,n):
                raw=read(a,n)
                if changed and (a,n)==(table+4*8,8):struct.pack_into('<Qii',data,header,table,1101,1102)
                return raw
            r.read=observing
            with self.subTest(changed=changed):
                if changed:
                    with self.assertRaises(DiscoveryRegistryChanged):helper.discover(r,pointers)
                else:
                    expected=list(r.discovery_class_candidates(pointers));r.ident_cache.clear();r.names_cache.clear()
                    r._native_class_discovery=helper.discover
                    self.assertEqual(list(r.discovery_class_candidates(pointers)),expected)


if __name__=='__main__':unittest.main()
