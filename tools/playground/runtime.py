"""Version-gated loader for the playground's game-thread reflection bridge."""
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
from pathlib import Path
import struct
import sys
import time

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/native-capture'))
sys.path.insert(0,str(ROOT/'tools/package-inspector'))
from start_capture import k,modules,remote_call,pefile
from resolve_camera import Resolver
from run_stereo_continuous import WindowsControl


ENTRY_RVA=0x1d8dd8
ENTRY=bytes.fromhex('405541544155415641574881eca0000000488d6c243048899da00000004889b5a80000004889bdb0000000488b0536268f014833c5488945604d8be0488bda4c')


class InitializationError(RuntimeError):
    def __init__(self,code,entry_restored):
        super().__init__('Playground initialization failed: '+str(code))
        self.code=code
        self.entry_restored=entry_restored


class Bridge:
    def __init__(self,identity,dll,*,expected_sha256=None,reserve_only=False,attach_only=False):
        self.identity=identity;self.dll=Path(dll).resolve();self.control=WindowsControl()
        if expected_sha256 is not None and hashlib.sha256(self.dll.read_bytes()).hexdigest()!=expected_sha256:
            raise RuntimeError('Playground DLL SHA256 mismatch')
        self.r=Resolver(identity['pid'],json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text()))
        self.handle=None;self.pending_address=None
        try:
            self.verify()
            if reserve_only or attach_only:
                selected=Path(identity['image'])
                if selected.name.lower()!='gowde-steam.exe' or hashlib.sha256(selected.read_bytes()).hexdigest()!=self.r.profile['sha256']:
                    raise RuntimeError('Playground reservation executable differs from validated build')
            else:self.r.prepare()
            self.handle=k.OpenProcess(0x143a,False,identity['pid'])
            if not self.handle:raise c.WinError(c.get_last_error())
            self.exports={e.name.decode():e.address for e in pefile.PE(str(self.dll)).DIRECTORY_ENTRY_EXPORT.symbols if e.name and not e.forwarder}
            loaded=modules(self.handle)
            if reserve_only or attach_only:
                game=loaded.get('gowde-steam.exe')
                if not game or game[1].resolve()!=selected.resolve():raise RuntimeError('Playground reservation module identity mismatch')
                self.r.base=game[0]
                if self.r.read(self.r.base,2)!=b'MZ':raise RuntimeError('Playground reservation module header mismatch')
            if self.dll.name.lower() not in loaded:
                if attach_only:raise RuntimeError('Playground cleanup module absent; no injection attempted')
                # Validate live bytes against the retained entry certificate.
                if self.r.read(self.r.base+ENTRY_RVA,len(ENTRY))!=ENTRY:
                    raise RuntimeError('Playground ProcessEvent entry differs from validated source')
                local=k.GetProcAddress(k.GetModuleHandleW('kernel32.dll'),b'LoadLibraryW')
                owner=w.HMODULE()
                if not k.GetModuleHandleExW(6,local,c.byref(owner)):raise c.WinError(c.get_last_error())
                path=c.create_unicode_buffer(32768);k.GetModuleFileNameW(owner,path,len(path))
                owner_name=Path(path.value).name.lower()
                if owner_name not in loaded:raise RuntimeError('Remote loader owner absent')
                raw=(str(self.dll)+'\0').encode('utf-16-le');address=self.allocate(raw)
                loader_returned=False
                try:
                    remote_call(self.handle,loaded[owner_name][0]+local-owner.value,address);loader_returned=True
                finally:
                    # A timed-out remote loader may still read its argument.
                    if loader_returned:self.free(address)
                loaded=modules(self.handle)
                if self.dll.name.lower() not in loaded:raise RuntimeError('Playground DLL did not load')
                self.base,actual=loaded[self.dll.name.lower()]
                if actual.resolve()!=self.dll:raise RuntimeError('Playground module path mismatch')
            else:
                self.base,actual=loaded[self.dll.name.lower()]
                if actual.resolve()!=self.dll:raise RuntimeError('Playground module path mismatch')
            if not attach_only:
                status=self.export('PlaygroundReserve' if reserve_only else 'PlaygroundInitialize')
                if status:self.initialization_failed(status)
        except BaseException:self.close();raise
    def initialization_failed(self,status):
        # A failed optional component may be isolated only with independent live
        # proof that its full attested entry is still original. Read failure is
        # not restoration; the launcher must then retain its normal failure path.
        restored=False
        try:
            self.verify();restored=self.r.read(self.r.base+ENTRY_RVA,len(ENTRY))==ENTRY
        except (OSError,RuntimeError):pass
        raise InitializationError(status,restored)
    def verify(self):
        if self.control.identity(self.identity['pid'])!=self.identity:raise RuntimeError('Playground process identity changed')
    def allocate(self,raw):
        self.verify()
        address=k.VirtualAllocEx(self.handle,None,len(raw),0x3000,4)
        if not address:raise c.WinError(c.get_last_error())
        self.write(address,raw);return address
    def write(self,address,raw):
        self.verify();count=c.c_size_t();buffer=c.create_string_buffer(raw)
        if not k.WriteProcessMemory(self.handle,address,buffer,len(raw),c.byref(count)) or count.value!=len(raw):raise c.WinError(c.get_last_error())
    def free(self,address):
        if address and not k.VirtualFreeEx(self.handle,address,0,0x8000):raise c.WinError(c.get_last_error())
    def export(self,name,parameter=None):
        self.verify();return remote_call(self.handle,self.base+self.exports[name],parameter)
    def function(self,owner,name):
        seen=set()
        while owner and owner not in seen:
            seen.add(owner)
            for field in self.r.fields(owner):
                if field['name']==name and self.r.class_name(field)=='Function':return field
            owner=self.r.u64(owner+0x60)
        raise RuntimeError('Function not found: '+name)
    def call(self,object,name,timeout=10,**values):
        self.verify()
        obj=self.r.identity(object);function=self.function(obj['cls'],name);addr=function['address']
        size=struct.unpack('<H',self.r.read(addr+0x106,2))[0]
        if size>4096:raise ValueError('Playground parameter budget')
        buffer=bytearray(4096);allocations=[];returned=None;fields=[]
        for prop in self.r.fields(addr):
            flags=self.r.u64(prop['address']+0x78)
            if not flags&0x80:continue
            kind=self.r.class_name(prop);offset=self.r.u32(prop['address']+0x94)
            element=self.r.u32(prop['address']+0x74)
            if offset+element>size:raise RuntimeError('Function parameter outside declared size')
            fields.append((prop,kind,offset,element))
            if flags&0x400:returned=(prop,kind,offset,element);continue
            if prop['name'] not in values:continue
            value=values.pop(prop['name'])
            if kind in ('ObjectProperty','ClassProperty','ComponentProperty'):raw=struct.pack('<Q',value)
            elif kind=='NameProperty':raw=struct.pack('<II',*value)
            elif kind=='IntProperty':raw=struct.pack('<i',value)
            elif kind=='ByteProperty':raw=struct.pack('<B',value)
            elif kind=='BoolProperty':raw=struct.pack('<I',self.r.u32(prop['address']+0xd0) if value else 0)
            elif kind=='FloatProperty':raw=struct.pack('<f',value)
            elif kind=='StrProperty':
                encoded=(value+'\0').encode('utf-16-le');memory=self.allocate(encoded);allocations.append(memory)
                raw=struct.pack('<Qii',memory,len(encoded)//2,len(encoded)//2)
            elif kind=='StructProperty':raw=value
            else:raise RuntimeError('Unsupported parameter: '+kind)
            if len(raw)!=element:raise RuntimeError('Parameter element size mismatch: '+prop['name'])
            buffer[offset:offset+element]=raw
        if values:raise ValueError('Unknown parameters: '+str(sorted(values)))
        request=self.allocate(struct.pack('<iIQQ',0,size,object,addr)+buffer)
        done=False
        try:
            if self.export('PlaygroundSubmit',request):raise RuntimeError('Playground request rejected')
            self.pending_address=request
            deadline=time.monotonic()+timeout
            while time.monotonic()<deadline:
                self.verify();state=struct.unpack('<i',self.r.read(request,4))[0]
                if state:
                    done=True;self.pending_address=None
                    if state!=2:raise RuntimeError('Playground request failed: '+str(state))
                    raw=self.r.read(request+24,4096)
                    self.last_outputs={p['name']:raw[o:o+s].hex() for p,k,o,s in fields
                        if self.r.u64(p['address']+0x78)&0x100}
                    if not returned:return None
                    prop,kind,offset,element=returned;value=raw[offset:offset+element]
                    if kind in ('ObjectProperty','ClassProperty','ComponentProperty'):return struct.unpack('<Q',value)[0]
                    if kind=='IntProperty':return struct.unpack('<i',value)[0]
                    if kind=='BoolProperty':return bool(struct.unpack('<I',value)[0])
                    if kind=='FloatProperty':return struct.unpack('<f',value)[0]
                    return value.hex()
                time.sleep(.01)
            raise TimeoutError('Playground game-thread request pending; allocation retained; do not retry')
        finally:
            if done:
                self.free(request)
                for memory in allocations:self.free(memory)
    def close(self):
        if self.handle:k.CloseHandle(self.handle);self.handle=None
        if getattr(self,'r',None):self.r.p.close();self.r=None
