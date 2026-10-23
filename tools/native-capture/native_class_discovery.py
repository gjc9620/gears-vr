"""Optional native loop over read-only discovery hints; no game calls or writes."""
from array import array
import ctypes as c
import hashlib
from pathlib import Path
import sys
import time

from resolve_camera import DiscoveryRegistryChanged

READ=c.WINFUNCTYPE(c.c_int,c.c_void_p,c.c_uint64,c.c_uint32,c.c_void_p)


class NativeClassDiscovery:
    def __init__(self,dll,expected_sha256):
        self.dll=Path(dll);self.expected_sha256=expected_sha256
        self.library=self.function=None;self.attempted=False;self.status='not_loaded'

    def warmup(self):
        if self.attempted:return self.function is not None
        self.attempted=True
        try:
            if hashlib.sha256(self.dll.read_bytes()).hexdigest()!=self.expected_sha256:
                self.status='hash_mismatch';return False
            library=c.WinDLL(str(self.dll))
            function=library.GearsCaptureDiscoveryClasses
            function.argtypes=[c.c_uint32,c.c_void_p,c.c_uint32,c.c_void_p,c.c_uint32,c.c_uint64,
                               READ,c.c_void_p,c.c_void_p,c.c_uint32,c.POINTER(c.c_uint32)]
            function.restype=c.c_uint32
            self.library,self.function,self.status=library,function,'available'
            return True
        except (OSError,AttributeError):self.status='unavailable';return False

    def pointer_view(self,data,count):
        """Offer immutable discovery bytes only for an available verified export."""
        if (type(data)is bytes and type(count)is int and 0<=count<4000000 and
                len(data)==count*8 and sys.byteorder=='little'and self.warmup()):
            return memoryview(data).cast('Q')
        return None

    def discover(self,resolver,pointers,check_live=None):
        profile=resolver.profile
        if tuple(profile.get(key)for key in ('object_class','object_name','name_entry_index','name_entry_text'))!=(80,72,0,20):return None
        if not self.warmup():return None
        names=getattr(resolver,'name_pointers',None)
        if names is None:return None
        if (isinstance(pointers,memoryview)and pointers.format=='Q'and pointers.itemsize==8 and
                pointers.ndim==1 and pointers.c_contiguous and pointers.readonly and
                type(pointers.obj)is bytes and sys.byteorder=='little'):
            # Copy the immutable discovery snapshot directly into the same
            # owned native input array; keep it alive through the callback.
            objects=array('Q');objects.frombytes(pointers.cast('B'))
        else:objects=array('Q',pointers)
        names=array('Q',names)
        if not 0<len(objects)<4000000 or not 0<len(names)<4000000:return None
        output=array('Q',[0])*len(objects);count=c.c_uint32();pending=[];next_check=[0.0]
        @READ
        def read(_context,address,size,destination):
            if pending:return -1
            try:
                now=time.monotonic()
                if check_live is not None and now>=next_check[0]:
                    check_live();next_check[0]=now+.02
                try:raw=resolver.read(address,size)
                except DiscoveryRegistryChanged:raise
                except (RuntimeError,OSError,ValueError):return 0
                if len(raw)!=size:return 0
                c.memmove(destination,raw,size);return 1
            except BaseException as error:pending.append(error);return -1
        # Keep arrays, callback and library alive for this synchronous call.
        result=self.function(1,objects.buffer_info()[0],len(objects),names.buffer_info()[0],len(names),
            resolver.names_header,read,None,output.buffer_info()[0],len(output),c.byref(count))
        if pending:raise pending[0]
        if result==1237:raise DiscoveryRegistryChanged('Name registry changed during native discovery')
        if result:raise RuntimeError('Native class discovery failed: '+str(result))
        if count.value>len(output):raise RuntimeError('Invalid native discovery count')
        if check_live is not None:check_live()
        return output[:count.value]
