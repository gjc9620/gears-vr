"""Independent mocked RPM and disk artifacts; never opens a real process."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest import mock
import types

from hook_entry_attestation import HookEntryAttestation,AttestationRejected
import start_capture
import ui_observer_config as ui
from test_ui_observer_config import Fixture


class Memory:
    def __init__(self):self.memory={};self.reads=[]
    def put(self,address,data):self.memory.update({address+i:byte for i,byte in enumerate(data)})
    def read(self,address,size):
        self.reads.append((address,size))
        try:return bytes(self.memory[address+i] for i in range(size))
        except KeyError:return b''
    def identity(self,pid):return self.game if pid==123 else self.owner if pid==99 else None
    def handle_identity(self):return 123,12
    def modules(self):return self.loaded


class Tests(unittest.TestCase):
    def setUp(self):
        directory=tempfile.TemporaryDirectory();self.addCleanup(directory.cleanup)
        self.dll=Path(directory.name)/'GearsCapture.dll';self.path=self.dll.with_name('hook-entry-manifest.json')
        self.game_path=Path(directory.name)/'GOWDE-Steam.exe'
        pe=bytearray(512);pe[:2]=b'MZ';struct.pack_into('<I',pe,0x3c,0x80);pe[0x80:0x84]=b'PE\0\0'
        struct.pack_into('<H',pe,0x80+24,0x20b);struct.pack_into('<I',pe,0x80+80,0x10000)
        self.dll.write_bytes(pe);self.dll_hash=hashlib.sha256(pe).hexdigest()
        self.base=0x140000000;self.native=self.base+0x2000000
        self.backend=Memory()
        self.backend.game=dict(pid=123,creation_time=12,image=str(self.game_path.resolve()).casefold())
        self.backend.owner=dict(pid=99,creation_time=13,image='owner.exe')
        self.backend.loaded={'gowde-steam.exe':(self.base,self.game_path),'gearscapture.dll':(self.native,self.dll)}
        self.manifest=dict(schema=1,pid=123,game_creation=12,owner_pid=99,owner_creation=13,
                           module_base=self.base,native_base=self.native,manifest_generation=100,entries=[])
        self.add_entry(self.base+0x1000,bytes(range(32)),self.base+0x10000,self.native+0x100)
        self.write()
    def add_entry(self,target,original,relay,detour):
        patch=b'\xe9'+struct.pack('<i',relay-target-5)+original[5:]
        relay_bytes=bytes.fromhex('ff2500000000')+struct.pack('<Q',detour)
        self.manifest['entries'].append(dict(target=target,detour=detour,original=original.hex(),
                                            patched=patch.hex(),relay=relay,relay_bytes=relay_bytes.hex()))
        self.backend.put(target,patch);self.backend.put(relay,relay_bytes)
    def write(self):self.path.write_text(json.dumps(self.manifest),encoding='utf-8')
    def load(self,**kwargs):
        args=dict(pid=123,game_creation=12,owner_identity=dict(self.backend.owner),dll=self.dll,
                  expected_dll_sha256=self.dll_hash,backend=self.backend)
        args.update(kwargs);return HookEntryAttestation(self.path,**args)

    def test_exact_entry_only_original_read_and_real_fallback(self):
        proof=self.load();entry=proof.entries[0];fallback=mock.Mock(return_value=b'actual')
        for size in (1,16,32):self.assertEqual(proof.read_entry(entry.target,size,fallback),entry.original[:size])
        fallback.assert_not_called()
        self.assertEqual(proof.read_entry(entry.target+1,16,fallback),b'actual')
        fallback.assert_called_once_with(entry.target+1,16)
        for size in (0,33,-1,True,16.0):
            with self.assertRaises(AttestationRejected):proof.read_entry(entry.target,size,fallback)
        self.assertEqual(self.backend.read(entry.target,32),entry.patched)
        proof.close()
        with self.assertRaises(AttestationRejected):proof.read_entry(entry.target,16,fallback)

    def test_manifest_format_duplicates_and_branch_tampering_reject(self):
        original=copy.deepcopy(self.manifest)
        for field,value in (('schema',2),('schema',True),('manifest_generation',0),('pid',True),('owner_creation',14)):
            self.manifest=copy.deepcopy(original);self.manifest[field]=value;self.write()
            with self.assertRaises(AttestationRejected):self.load()
        for field,value in (('target',0),('relay',(1<<64)-1),('original','00'),
                            ('patched','90'*32),('relay_bytes','ff2400000000'+'00'*8)):
            self.manifest=copy.deepcopy(original);self.manifest['entries'][0][field]=value;self.write()
            with self.assertRaises(AttestationRejected):self.load()
        for field in ('patched','relay_bytes'):
            self.manifest=copy.deepcopy(original);raw=bytearray.fromhex(self.manifest['entries'][0][field]);raw[-1]^=1
            self.manifest['entries'][0][field]=raw.hex();self.write()
            with self.assertRaises(AttestationRejected):self.load()
        self.manifest=copy.deepcopy(original);self.manifest['entries']*=2;self.write()
        with self.assertRaisesRegex(AttestationRejected,'Duplicate target'):self.load()
        self.path.write_text('{"schema":1,"schema":1}',encoding='utf-8')
        with self.assertRaisesRegex(AttestationRejected,'Duplicate manifest key'):self.load()

    def test_identity_loaded_base_path_and_expected_dll_hash_reject(self):
        for changes in ({'game_creation':14},{'owner_identity':dict(self.backend.owner,creation_time=14)},
                        {'expected_dll_sha256':'0'*64}):
            with self.assertRaises(AttestationRejected):self.load(**changes)
        for key,value in (('gearscapture.dll',(self.native+1,self.dll)),
                          ('gearscapture.dll',(self.native,self.dll.with_name('Other.dll'))),
                          ('gowde-steam.exe',(self.base+1,self.game_path))):
            old=self.backend.loaded[key];self.backend.loaded[key]=value
            with self.assertRaises(AttestationRejected):self.load()
            self.backend.loaded[key]=old
        with mock.patch.object(self.backend,'handle_identity',return_value=(123,999)):
            with self.assertRaisesRegex(AttestationRejected,'RPM handle'):self.load()

    def test_reverification_rejects_disk_manifest_and_live_mutations(self):
        proof=self.load();entry=proof.entries[0]
        self.path.write_bytes(self.path.read_bytes()+b' ')
        with self.assertRaisesRegex(AttestationRejected,'Sealed manifest changed'):proof.reverify()
        self.write()
        for location,expected in ((entry.target,entry.patched),(entry.relay,entry.relay_bytes)):
            self.backend.put(location,bytes([expected[0]^1])+expected[1:])
            with self.assertRaisesRegex(AttestationRejected,'Live hook differs'):proof.reverify()
            self.backend.put(location,expected)
        self.dll.write_bytes(self.dll.read_bytes()+b'x')
        with self.assertRaisesRegex(AttestationRejected,'DLL disk hash'):proof.reverify()

    def test_stable_double_reads_and_non_game_targets(self):
        original=self.backend.read;entry=self.manifest['entries'][0];count=[0]
        def unstable(address,size):
            result=original(address,size)
            if address==entry['target']:
                count[0]+=1
                if count[0]%2==0:return b'x'+result[1:]
            return result
        with mock.patch.object(self.backend,'read',side_effect=unstable):
            with self.assertRaisesRegex(AttestationRejected,'unstable'):self.load()
        self.add_entry(0x7ff000001000,bytes(range(32)),0x7ff000002000,self.native+0x200)
        self.write();self.assertEqual(len(self.load().entries),2)

    def test_detour_outside_authenticated_image_rejects(self):
        self.manifest['entries']=[]
        self.add_entry(self.base+0x1000,bytes(range(32)),self.base+0x10000,self.native+0x10000)
        self.write()
        with self.assertRaisesRegex(AttestationRejected,'Detour outside'):self.load()

    def test_target_change_during_relay_read_rejects_original_overlay(self):
        proof=self.load();entry=proof.entries[0];read=self.backend.read
        def change_while_reading_relay(address,size):
            result=read(address,size)
            if address==entry.relay:self.backend.put(entry.target,b'x'+entry.patched[1:])
            return result
        with mock.patch.object(self.backend,'read',side_effect=change_while_reading_relay):
            with self.assertRaisesRegex(AttestationRejected,'unstable'):
                proof.read_entry(entry.target,16,read)

    def ui_proof(self):
        fixture=Fixture();self.manifest['entries']=[];self.backend.memory=fixture.memory
        for i,(label,(rva,expected)) in enumerate(ui.BINDINGS.items()):
            self.add_entry(fixture.base+rva,bytes.fromhex(expected)+bytes(16),self.base+0x1000000+i*0x1000,self.native+0x100+i*0x20)
        self.write();return fixture,self.load()

    def test_actual_ui_fixture_accepts_only_explicit_adapter_and_preserves_raw_read(self):
        fixture,proof=self.ui_proof();raw_read=fixture.read
        with self.assertRaisesRegex(ui.UIConfigRejected,'UI entry bytes mismatch'):
            ui.build_ui_observer_config(fixture,fixture.camera)
        calls=[]
        def entry(address,size,actual):calls.append((address,size));return proof.read_entry(address,size,actual)
        config,_=ui.build_ui_observer_config(fixture,fixture.camera,entry_reader=entry)
        self.assertEqual(config['bindings']['vm']['bytes'],ui.BINDINGS['vm'][1])
        for rva,_ in ui.BINDINGS.values():self.assertEqual(calls.count((fixture.base+rva,16)),3)
        self.assertEqual(fixture.read.__func__,raw_read.__func__)
        self.assertEqual(fixture.read(fixture.base+ui.BINDINGS['vm'][0],1),b'\xe9')

    def test_ui_final_entry_recheck_uses_live_attestation(self):
        fixture,proof=self.ui_proof();counts={}
        def entry(address,size,actual):
            counts[address]=counts.get(address,0)+1
            if counts[address]==3:self.backend.put(address,b'\x90')
            return proof.read_entry(address,size,actual)
        with self.assertRaisesRegex(AttestationRejected,'Live hook differs'):
            ui.build_ui_observer_config(fixture,fixture.camera,entry_reader=entry)

    def test_startup_signature_adapter_is_explicit_and_passed_to_ui(self):
        r=types.SimpleNamespace(base=self.base,p=types.SimpleNamespace(close=mock.Mock()),read=mock.Mock(return_value=b'patched'))
        camera={'module_base':hex(self.base),'local_player_address':'0x100'};r.resolve=mock.Mock(return_value=camera)
        def resolver(*args):return r
        entry=mock.Mock(return_value=b'original')
        with mock.patch.object(start_capture,'Resolver',side_effect=resolver):
            with self.assertRaisesRegex(RuntimeError,'Target prologue differs'):
                start_capture.acquire_observer_preparation(123,{},camera,{0x100:b'original'},Path('unused'))
            with mock.patch.object(ui,'build_ui_observer_config',return_value=({},{})) as builder,\
                 mock.patch.object(ui,'ui_observer_ini',return_value='ui'):
                start_capture.acquire_observer_preparation(123,{},camera,{0x100:b'original'},Path('unused'),ui_observer=True,entry_reader=entry)
                self.assertIs(builder.call_args.kwargs['entry_reader'],entry)
        entry.assert_called_once_with(self.base+0x100,8,r.read)


if __name__=='__main__':unittest.main()
