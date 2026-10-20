"""Resolve the local Reloaded camera from module-relative engine registries.

No saved process addresses are consumed. OpenProcess is query/read-only.
The profile is strictly gated by executable SHA256 and validated on each run.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
import math
import pathlib
import struct
import sys
import time
from runtime_probe import Process, k

psapi=c.WinDLL('psapi',use_last_error=True)
psapi.EnumProcessModulesEx.argtypes=[w.HANDLE,c.POINTER(w.HMODULE),w.DWORD,c.POINTER(w.DWORD),w.DWORD]
psapi.EnumProcessModulesEx.restype=w.BOOL
k.QueryFullProcessImageNameW.argtypes=[w.HANDLE,w.DWORD,w.LPWSTR,c.POINTER(w.DWORD)]
k.QueryFullProcessImageNameW.restype=w.BOOL

class _DiscoveryHintPages:
    """One scan's bounded read hints; never use for certificate validation.

    Page reuse changes the discovery read schedule, as identity caches already
    do. It establishes neither current identity nor atomic global uniqueness.
    Every candidate still requires the caller's ordinary fresh validations.
    """
    PAGE_SIZE=4096
    MAX_PAGES=64
    def __init__(self,read):
        self.read_exact=read
        self.pages={}
    def read(self,address,size):
        page=address-address%self.PAGE_SIZE;offset=address-page
        if size!=0x60 or offset+size>self.PAGE_SIZE:
            return self.read_exact(address,size)
        if page not in self.pages:
            try:raw=self.read_exact(page,self.PAGE_SIZE)
            except (RuntimeError,OSError):return self.read_exact(address,size)
            if len(raw)!=self.PAGE_SIZE:return self.read_exact(address,size)
            if len(self.pages)>=self.MAX_PAGES:self.pages.pop(next(iter(self.pages)))
            self.pages[page]=raw
        return self.pages[page][offset:offset+size]

class DiscoveryRegistryChanged(RuntimeError):
    """A validated registry header changed across a discovery read."""

class Resolver:
    def __init__(self,pid,profile):
        self.p=Process(pid)
        self.pid=pid
        self.profile=profile
        self.names_cache={}
        self.ident_cache={}
    def read(self,address,size):
        raw=self.p.read(address,size)
        if len(raw)!=size:raise RuntimeError(f'Unreadable address {address:#x}; object/process may have changed')
        return raw
    def u64(self,address):return struct.unpack('<Q',self.read(address,8))[0]
    def u32(self,address):return struct.unpack('<I',self.read(address,4))[0]
    def prepare(self):
        path_buffer=c.create_unicode_buffer(32768)
        length=w.DWORD(len(path_buffer))
        if not k.QueryFullProcessImageNameW(self.p.handle,0,path_buffer,c.byref(length)):
            raise OSError(c.get_last_error(),'Could not query game executable')
        path=pathlib.Path(path_buffer.value)
        if path.name.lower()!='gowde-steam.exe':raise RuntimeError('Selected process is not GOWDE-Steam.exe')
        sha=hashlib.sha256(path.read_bytes()).hexdigest()
        if sha!=self.profile['sha256']:raise RuntimeError('Executable hash differs from validated build; re-discover offsets')
        modules=(w.HMODULE*1024)()
        required=w.DWORD()
        if not psapi.EnumProcessModulesEx(self.p.handle,modules,c.sizeof(modules),c.byref(required),3):
            raise OSError(c.get_last_error(),'Could not enumerate modules')
        self.base=modules[0]
        if self.read(self.base,2)!=b'MZ':raise RuntimeError('Invalid main module base')
        self.path=str(path)
        self.names_header=self.base+int(self.profile['names_rva'],16)
        self.objects_header=self.base+int(self.profile['objects_rva'],16)
        self.names_data,self.name_count,self.name_capacity=struct.unpack('<Qii',self.read(self.names_header,16))
        self.objects_data,self.object_count,self.object_capacity=struct.unpack('<Qii',self.read(self.objects_header,16))
        if not 1000<self.name_count<=self.name_capacity<4000000:raise RuntimeError('Invalid name array header')
        if not 1000<self.object_count<=self.object_capacity<4000000:raise RuntimeError('Invalid object array header')
        self.name_pointers=struct.unpack('<'+'Q'*self.name_count,self.read(self.names_data,self.name_count*8))
        self.object_pointers=struct.unpack('<'+'Q'*self.object_count,self.read(self.objects_data,self.object_count*8))
        if self.name(0)!='None':raise RuntimeError('Name registry zero entry is not None')
    def name(self,index):
        if index not in self.names_cache:
            if index<0:raise RuntimeError('Name index out of range')
            # Anchor guards refresh live bounds, not this discovery snapshot.
            # Newly registered objects can refer to names added after prepare().
            # Resolve those slots from a bounded current header; never index the
            # old tuple using a subsequently refreshed name_count.
            header=None
            if index<len(self.name_pointers):entry=self.name_pointers[index]
            else:
                header=self.read(self.names_header,16)
                data,count,capacity=struct.unpack('<Qii',header)
                if not data or not 1000<count<=capacity<4000000:
                    raise RuntimeError('Invalid name array header')
                if index>=count:raise RuntimeError('Name index out of range')
                entry=self.u64(data+index*8)
                if self.read(self.names_header,16)!=header:
                    raise DiscoveryRegistryChanged('Name registry changed during discovery')
            if not entry:raise RuntimeError('Null name entry')
            if self.u32(entry+self.profile['name_entry_index'])!=index:raise RuntimeError('Name entry index mismatch')
            # A short name can end before an unreadable neighboring page.
            # Keep the same bound, but do not request that page unless the
            # string actually continues into it. Exact-read failures propagate.
            address=entry+self.profile['name_entry_text']
            raw=b'';end=None
            while len(raw)<128:
                current=address+len(raw)
                size=min(128-len(raw),4096-current%4096)
                raw+=self.read(current,size)
                end=next((i for i in range(0,len(raw)-1,2) if raw[i:i+2]==b'\0\0'),None)
                if end is not None:break
            if end is None:raise RuntimeError('Name exceeds bounded UTF16 read')
            name=raw[:end].decode('utf-16-le',errors='strict')
            if header is not None and self.read(self.names_header,16)!=header:
                raise DiscoveryRegistryChanged('Name registry changed during discovery')
            self.names_cache[index]=name
        return self.names_cache[index]
    def identity(self,address,*,reader=None):
        if not address:return None
        if address not in self.ident_cache:
            raw=(reader or self.read)(address,0x60)
            index,number=struct.unpack_from('<II',raw,self.profile['object_name'])
            self.ident_cache[address]=dict(address=address,name=self.name(index),number=number,
                cls=struct.unpack_from('<Q',raw,self.profile['object_class'])[0],
                outer=struct.unpack_from('<Q',raw,self.profile['object_outer'])[0],
                internal_index=struct.unpack_from('<I',raw,self.profile['object_internal_index'])[0])
        return self.ident_cache[address]
    def discovery_identity_reader(self):
        """Create a new scan-local hint capability; fresh reads never use it."""
        pages=_DiscoveryHintPages(self.read)
        def discover(address):return self.identity(address,reader=pages.read)
        # Class discovery need not decode the name of every gameplay instance.
        # This uses the same scan-local pages and identity hints as discover;
        # selected objects still go through the caller's fresh validation.
        def class_identity(address):
            raw=pages.read(address,0x60)
            cls=struct.unpack_from('<Q',raw,self.profile['object_class'])[0]
            if not cls:raise RuntimeError('Null discovery class')
            return self.identity(cls,reader=pages.read)
        discover.class_identity=class_identity
        return discover
    def class_name(self,obj):return self.identity(obj['cls'])['name']
    def find_camera_class(self):
        discover=self.discovery_identity_reader()
        for index,address in enumerate(self.object_pointers):
            if not address:continue
            obj=discover(address)
            if obj['internal_index']!=index:raise RuntimeError('Object registry index mismatch')
            if obj['name']=='WarPlayerCamera' and obj['number']==0 and self.class_name(obj)=='Class':
                return address
        raise RuntimeError('WarPlayerCamera class not loaded')
    def fields(self,owner):
        ptr=self.u64(owner+self.profile['struct_children'])
        seen=set()
        while ptr:
            if ptr in seen or len(seen)>4096:raise RuntimeError('Invalid reflected field chain')
            seen.add(ptr)
            obj=self.identity(ptr)
            if obj['outer']!=owner:raise RuntimeError('Reflected field owner mismatch')
            yield obj
            ptr=self.u64(ptr+self.profile['field_next'])
    def property(self,owner,name,inherited=False):
        seen=set()
        while owner and owner not in seen:
            seen.add(owner)
            for field in self.fields(owner):
                if field['name']==name and self.class_name(field).endswith('Property'):
                    offset=self.u32(field['address']+self.profile['property_offset_internal'])
                    if offset>1024*1024:raise RuntimeError('Implausible property offset')
                    return dict(address=field['address'],offset=offset,type=self.class_name(field))
            if not inherited:break
            owner=self.u64(owner+self.profile['field_super'])
        raise RuntimeError(f'Property {name} not found')
    def struct_type(self,prop):
        if prop['type']!='StructProperty':raise RuntimeError('Expected reflected struct property')
        ptr=self.u64(prop['address']+self.profile['struct_property_struct'])
        if self.class_name(self.identity(ptr))!='ScriptStruct':raise RuntimeError('Invalid reflected struct type')
        return ptr
    def resolve(self):
        self.prepare()
        camera_class=self.find_camera_class()
        cache=self.property(camera_class,'CameraCache',True)
        owner_prop=self.property(camera_class,'PCOwner',True)
        cache_type=self.struct_type(cache)
        if self.identity(cache_type)['name']!='TCameraCache':raise RuntimeError('Unexpected CameraCache type')
        pov=self.property(cache_type,'POV')
        stamp=self.property(cache_type,'TimeStamp')
        pov_type=self.struct_type(pov)
        if self.identity(pov_type)['name']!='TPOV':raise RuntimeError('Unexpected POV type')
        location=self.property(pov_type,'Location')
        rotation=self.property(pov_type,'Rotation')
        fov=self.property(pov_type,'FOV')
        if (location['offset'],rotation['offset'],fov['offset'])!=(0,12,24):raise RuntimeError('Unsupported POV layout')
        candidates=[]
        for index,addr in enumerate(self.object_pointers):
            if not addr:continue
            if self.u64(addr+self.profile['object_class'])!=camera_class:continue
            obj=self.identity(addr)
            if obj['internal_index']!=index:raise RuntimeError('Camera instance index mismatch')
            if obj['name']!='WarPlayerCamera':continue  # Excludes default object
            pc=self.u64(addr+owner_prop['offset'])
            if not pc:continue
            pc_obj=self.identity(pc)
            player_camera=self.property(pc_obj['cls'],'PlayerCamera',True)
            player_prop=self.property(pc_obj['cls'],'Player',True)
            if self.u64(pc+player_camera['offset'])!=addr:continue
            player=self.u64(pc+player_prop['offset'])
            if not player or self.class_name(self.identity(player))!='WarLocalPlayer':continue
            cache_address=addr+cache['offset']
            pov_address=cache_address+pov['offset']
            raw=self.read(pov_address,28)
            x,y,z,pitch,yaw,roll,view_fov=struct.unpack('<3f3if',raw)
            if not all(math.isfinite(v) for v in (x,y,z,view_fov)) or not 1<view_fov<179:
                continue
            candidates.append(dict(camera_address=hex(addr),camera_number=obj['number'],controller_address=hex(pc),
                local_player_address=hex(player),pov_address=hex(pov_address),
                timestamp_address=hex(cache_address+stamp['offset']),
                offsets={name:hex(prop['offset']) for name,prop in [('CameraCache',cache),('POV',pov),('TimeStamp',stamp),('PCOwner',owner_prop),('PlayerCamera',player_camera),('Player',player_prop),('Location',location),('Rotation',rotation),('FOV',fov)]},
                observed_pov=dict(location=[x,y,z],rotation_ru=[pitch,yaw,roll],fov=view_fov),
                verification='registry indices + reflected types + reciprocal camera/controller link + WarLocalPlayer + plausible POV'))
        if len(candidates)!=1:raise RuntimeError(f'Expected one local camera, found {len(candidates)}; no address accepted')
        # Registry reallocation or a camera replacement during discovery invalidates this pass.
        if self.u64(self.names_header)!=self.names_data or self.u64(self.objects_header)!=self.objects_data:
            raise RuntimeError('Registry reallocated during discovery; retry')
        accepted=candidates[0]
        cam=int(accepted['camera_address'],16)
        pc=int(accepted['controller_address'],16)
        if self.u64(cam+owner_prop['offset'])!=pc or self.u64(pc+int(accepted['offsets']['PlayerCamera'],16))!=cam:
            raise RuntimeError('Camera relationship changed during discovery; retry')
        return dict(pid=self.pid,module_base=hex(self.base),executable=self.path,sha256=self.profile['sha256'],
            names_anchor_rva=self.profile['names_rva'],objects_anchor_rva=self.profile['objects_rva'],
            name_count=self.name_count,object_count=self.object_count,**accepted)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    parser.add_argument('--profile',type=pathlib.Path,default=pathlib.Path(__file__).with_name('reloaded-profile.json'))
    args=parser.parse_args()
    resolver=Resolver(args.pid,json.loads(args.profile.read_text()))
    start=time.monotonic()
    try:
        result=resolver.resolve()
        result['elapsed_seconds']=round(time.monotonic()-start,3)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(result,indent=2),encoding='utf-8')
        print(json.dumps(result,indent=2))
    except Exception as exc:
        error=dict(status='failed',pid=args.pid,error=str(exc))
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(error,indent=2),encoding='utf-8')
        print(json.dumps(error),file=sys.stderr)
        sys.exit(1)
    finally:
        resolver.p.close()

if __name__=='__main__':main()
