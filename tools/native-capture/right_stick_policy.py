"""Session-owned, reflected right-stick policy. Never changes left-stick gates.

The input object's look dead zone is the one user-controlled gate; legacy
right-axis command dead zones become zero. Original bytes are restored only
while their identities, allocations and our installed values still match.
"""
import ctypes as c
from ctypes import wintypes as w
import json
import math
from pathlib import Path
import re
import struct
import time

ROOT=Path(__file__).resolve().parents[2]


def percent(value):
    if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not 0<=value<=50:
        raise ValueError('Right stick dead zone must be between 0 and 50 percent')
    return float(value)


def zero_axis_deadzone(command):
    # Only named look axes. Keep the FString allocation and length unchanged.
    if not re.search(r'\bAxis\s+a(?:Turn|LookUp)\b',command,re.I):
        raise ValueError('Not a right-stick look-axis command')
    def zero(match):return match.group(1)+'0'.ljust(len(match.group(2)))
    return re.sub(r'(\bDeadZone\s*=\s*)([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)',zero,command,flags=re.I)


class Writer:
    def __init__(self,pid,identity,check_identity):
        self.identity,self.check_identity=identity,check_identity
        self.k=c.WinDLL('kernel32',use_last_error=True)
        self.k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];self.k.OpenProcess.restype=w.HANDLE
        self.k.CloseHandle.argtypes=[w.HANDLE]
        self.k.WriteProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
        self.k.WriteProcessMemory.restype=w.BOOL
        self.handle=self.k.OpenProcess(0x438,False,pid)
        if not self.handle:raise c.WinError(c.get_last_error())
    def write(self,address,raw):
        from runtime_probe import MBI
        if self.check_identity(self.identity['pid'])!=self.identity:raise RuntimeError('Dead-zone process identity changed')
        self.k.VirtualQueryEx.argtypes=[w.HANDLE,c.c_void_p,c.POINTER(MBI),c.c_size_t]
        self.k.VirtualQueryEx.restype=c.c_size_t
        region=MBI()
        if (not raw or len(raw)>4096 or not self.k.VirtualQueryEx(self.handle,address,c.byref(region),c.sizeof(region)) or
            region.State!=0x1000 or region.Type!=0x20000 or region.Protect not in (4,8) or
            not region.BaseAddress<=address<=region.BaseAddress+region.RegionSize-len(raw)):
            raise RuntimeError('Dead-zone target is not a writable private allocation')
        written=c.c_size_t();data=c.create_string_buffer(raw,len(raw))
        if not self.k.WriteProcessMemory(self.handle,address,data,len(raw),c.byref(written)) or written.value!=len(raw):
            raise c.WinError(c.get_last_error())
    def close(self):
        if self.handle:self.k.CloseHandle(self.handle);self.handle=None


class Policy:
    def __init__(self,pid,value,check_identity):
        import sys
        sys.path.insert(0,str(ROOT/'tools/package-inspector'))
        from resolve_camera import Resolver
        self.pid,self.percent,self.check_identity=pid,percent(value),check_identity
        self.identity=check_identity(pid)
        if not self.identity:raise RuntimeError('Dead-zone policy needs a live owned game')
        self.r=Resolver(pid,json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text()))
        try:
            self.r.prepare();self.writer=Writer(pid,self.identity,check_identity)
        except BaseException:
            self.r.p.close()
            raise
        self.entries=[];self.current=None;self.next_poll=0.;self.closed=False
        self.report=dict(percent=self.percent,applied=False,applications=0,restored=0,stale_restore_skipped=0,errors=[])
    def fresh(self,address):
        r=self.r;p=r.profile;raw=r.read(address,0x60)
        index=struct.unpack_from('<I',raw,p['object_internal_index'])[0]
        data,count,capacity=struct.unpack('<Qii',r.read(r.objects_header,16))
        if not data or not 0<=index<count<=capacity<4000000 or r.u64(data+8*index)!=address:
            raise RuntimeError('Dead-zone UObject slot changed')
        return (address,index,raw[p['object_outer']:p['object_outer']+8],
                raw[p['object_name']:p['object_name']+8],raw[p['object_class']:p['object_class']+8])
    def plan(self):
        r=self.r;camera=r.resolve();controller=int(camera['controller_address'],0)
        identities={};metadata={}
        def obj(a):
            value=self.fresh(a);identities[a]=value;return dict(address=a,cls=struct.unpack('<Q',value[-1])[0])
        def field(owner,name,kind,structure=False):
            p=r.property(owner['address'] if structure else owner['cls'],name,not structure)
            if p['type']!=kind:raise RuntimeError('Dead-zone property type differs: '+name)
            obj(p['address']);raw=r.read(p['address']+0x70,0x28);dim,size=struct.unpack_from('<II',raw)
            if dim!=1 or not 0<size<=1024 or struct.unpack_from('<I',raw,0x24)[0]!=p['offset']:
                raise RuntimeError('Dead-zone property layout differs: '+name)
            metadata[p['address']+0x70]=raw;return dict(p,size=size)
        pc=obj(controller);input_field=field(pc,'PlayerInput','ObjectProperty')
        if input_field['size']!=8:raise RuntimeError('PlayerInput pointer size differs')
        pi=obj(r.u64(controller+input_field['offset']))
        look=field(pi,'LookStickDeadZone','FloatProperty')
        if look['size']!=4:raise RuntimeError('LookStickDeadZone is not float32')
        bindings=field(pi,'Bindings','ArrayProperty')
        if bindings['size']!=16:raise RuntimeError('Input bindings header differs')
        bh=pi['address']+bindings['offset'];header=r.read(bh,16);data,count,capacity=struct.unpack('<Qii',header)
        if not data or not 0<count<=capacity<=4096:raise RuntimeError('Input binding array bounds differ')
        inner_link=bindings['address']+r.profile['struct_property_struct']
        inner_address=r.u64(inner_link);inner=obj(inner_address)
        metadata[inner_link]=struct.pack('<Q',inner_address)
        if r.class_name(r.identity(inner_address))!='StructProperty':raise RuntimeError('Input binding inner is not struct')
        stride=r.u32(inner_address+0x74)
        structure_address=r.u64(inner_address+r.profile['struct_property_struct']);structure=obj(structure_address)
        metadata[inner_address+0x74]=struct.pack('<I',stride)
        metadata[inner_address+r.profile['struct_property_struct']]=struct.pack('<Q',structure_address)
        if not 24<=stride<=256:raise RuntimeError('Input binding stride differs')
        n=field(structure,'Name','NameProperty',True);cmd=field(structure,'Command','StrProperty',True)
        if n['size']!=8 or cmd['size']!=16 or max(n['offset']+8,cmd['offset']+16)>stride:
            raise RuntimeError('Input binding name/command layout differs')
        guards={controller+input_field['offset']:struct.pack('<Q',pi['address']),bh:header}
        changes=[];seen=set()
        for i in range(count):
            base=data+i*stride;name_raw=r.read(base+n['offset'],8);index,number=struct.unpack('<II',name_raw)
            name=r.name(index)
            if name not in ('XboxTypeS_RightX','XboxTypeS_RightY','Gamepad_RightX','Gamepad_RightY'):continue
            if number:raise RuntimeError('Unexpected right-stick binding instance')
            sh=r.read(base+cmd['offset'],16);pointer,length,allocated=struct.unpack('<Qii',sh)
            if not pointer or not 1<length<=allocated<=2048:raise RuntimeError('Right-stick command allocation differs')
            raw=r.read(pointer,length*2)
            if not raw.endswith(b'\0\0'):raise RuntimeError('Right-stick command is not terminated UTF16')
            text=raw[:-2].decode('utf-16-le');updated=zero_axis_deadzone(text).encode('utf-16-le')+b'\0\0'
            if len(updated)!=len(raw):raise RuntimeError('Right-stick command resize refused')
            guards[base+n['offset']]=name_raw;guards[base+cmd['offset']]=sh
            if updated!=raw:changes.append((pointer,raw,updated))
            seen.add(name)
        if not any(x.endswith('RightX') for x in seen) or not any(x.endswith('RightY') for x in seen):
            raise RuntimeError('Both right-stick axis bindings are required')
        target=pi['address']+look['offset'];old=r.read(target,4);original=struct.unpack('<f',old)[0]
        if not math.isfinite(original) or not 0<=original<=1:raise RuntimeError('Original look-stick dead zone invalid')
        changes.append((target,old,struct.pack('<f',self.percent/100)))
        def guard():
            return (self.check_identity(self.pid)==self.identity and
                all(self.fresh(a)==v for a,v in identities.items()) and
                all(r.read(a,len(v))==v for a,v in metadata.items()) and
                all(r.read(a,len(v))==v for a,v in guards.items()))
        return guard,changes,dict(controller=controller,input=pi['address'],original=original,axis_bindings=sorted(seen))
    def apply(self):
        guard,changes,info=self.plan()
        if not guard():raise RuntimeError('Dead-zone certificate changed before write')
        installed=[]
        try:
            for address,old,new in changes:
                if not guard() or self.r.read(address,len(old))!=old:raise RuntimeError('Dead-zone value changed before write')
                self.writer.write(address,new);installed.append((address,old,new))
                if self.r.read(address,len(new))!=new:raise RuntimeError('Dead-zone write verification failed')
        except BaseException:
            self.entries.append((guard,installed));self.restore();raise
        self.entries.append((guard,installed));self.current=(guard,installed)
        self.report.update(applied=True,applications=self.report['applications']+1,last=info)
    def tick(self):
        if self.closed or time.monotonic()<self.next_poll:return
        self.next_poll=time.monotonic()+2
        try:
            if self.current:
                guard,installed=self.current
                if guard() and all(self.r.read(a,len(new))==new for a,old,new in installed):return
                self.restore();self.current=None
            self.apply()
        except (RuntimeError,OSError,ValueError,struct.error) as error:
            if len(self.report['errors'])<8:self.report['errors'].append(str(error))
    def restore(self):
        pending=self.entries;self.entries=[]
        for guard,installed in reversed(pending):
            for address,old,new in reversed(installed):
                try:
                    if not guard() or self.r.read(address,len(new))!=new:self.report['stale_restore_skipped']+=1;continue
                    self.writer.write(address,old);self.report['restored']+=1
                except (RuntimeError,OSError):self.report['stale_restore_skipped']+=1
    def close(self):
        if self.closed:return
        try:self.restore()
        finally:self.writer.close();self.r.p.close();self.closed=True
        return dict(self.report)
