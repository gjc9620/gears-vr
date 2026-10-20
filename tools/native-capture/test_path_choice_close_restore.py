"""Independent complete-span CloseScene restoration tests; synthetic bytes."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import verify_stopped as verify

class Tests(unittest.TestCase):
    def setUp(self):
        self.base=0x180000000;self.pointer=0x700114;self.memory={}
        self.close=bytes(range(62));self.exec=bytes((i+3)%256 for i in range(160))
        self.memory[self.base+0x346574]=self.close
        self.memory[self.base+0x34698c]=self.exec[:128];self.memory[self.base+0x90dbe6]=self.exec[128:]
        self.memory[self.base+0x1429d58+0x228]=struct.pack('<Q',self.base+0x346574)
        self.memory[self.pointer]=struct.pack('<Q',self.base+0x34698c)
        self.patches=[mock.patch.object(verify,'PATH_CLOSE_SHA256',hashlib.sha256(self.close).hexdigest()),
                      mock.patch.object(verify,'PATH_CLOSE_EXEC_SHA256',hashlib.sha256(self.exec).hexdigest())]
        for p in self.patches:p.start();self.addCleanup(p.stop)
    def read(self,address,size):return self.memory[address][:size]
    def checks(self):return verify.check_path_close(self.read,self.base,self.base+0x346574,[self.pointer])
    def test_complete_body_exec_cold_slot_and_native_pointer(self):
        rows=self.checks();self.assertTrue(all(r['matches'] for r in rows));self.assertEqual([r['bytes_checked'] for r in rows],[62,160,8,8])
    def test_every_body_and_exec_byte_including_tail_is_independently_checked(self):
        for rva,size in ((0x346574,62),(0x34698c,128),(0x90dbe6,32)):
            at=self.base+rva;original=self.memory[at]
            for index in range(size):
                bad=bytearray(original);bad[index]^=1;self.memory[at]=bytes(bad)
                self.assertFalse(all(r['matches'] for r in self.checks()),(rva,index))
            self.memory[at]=original
    def test_slot_pointer_and_short_chunk_rejected(self):
        for at in (self.base+0x1429d58+0x228,self.pointer):
            original=self.memory[at];self.memory[at]=b'\0'*8
            self.assertFalse(all(r['matches'] for r in self.checks()));self.memory[at]=original
        at=self.base+0x90dbe6;self.memory[at]=self.memory[at][:-1]
        self.assertFalse(all(r['matches'] for r in self.checks()))
    def test_main_recognizes_only_hooked_target_and_verifies_loaded_pointer(self):
        self.memory[self.base+0x296178]=bytes.fromhex('498b00498bc848ffa0a8040000')
        closed=mock.Mock();resolver=SimpleNamespace(base=self.base,prepare=mock.Mock(),read=self.read,p=SimpleNamespace(handle=123,close=closed))
        with tempfile.TemporaryDirectory() as tmp:
            status=Path(tmp)/'status.txt';output=Path(tmp)/'result.json'
            status.write_text('capturing pid=321 flags=test\n'+
                ''.join('hook_target=%X create=MH_OK queue=MH_OK\n'%(self.base+rva) for rva in (0x296178,0x346574))+
                'path_close_loaded nonce=1 duration_ms=8000 native_pointer=%d own_prefix_certified=1 observation_only=1\n'%self.pointer+
                'stopped disable_status=0 active=0\n')
            with mock.patch.object(sys,'argv',['verify_stopped.py','--pid','321','--status',str(status),'--output',str(output)]), \
                 mock.patch.object(verify,'Resolver',return_value=resolver),mock.patch.object(verify,'modules',return_value={}), \
                 contextlib.redirect_stdout(io.StringIO()):verify.main()
            data=json.loads(output.read_text());self.assertTrue(data['all_match']);self.assertTrue(any(r.get('target')==hex(self.pointer) for r in data['checks']))
        closed.assert_called_once()

if __name__=='__main__':unittest.main()
