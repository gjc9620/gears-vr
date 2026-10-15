"""Explicit entry-only original bytes backed by a sealed, live hook proof.

No Resolver patching, process writes, injection, or generic read substitution.
The caller owns the attestation lease and must close it after candidate work.
"""
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
from pathlib import Path
import struct
from dataclasses import dataclass


class AttestationRejected(RuntimeError):pass


def address(value,label,size=1):
    if type(value) is not int or not 0 < value <= (1 << 64)-size:
        raise AttestationRejected('Invalid '+label)
    return value


def unique_object(pairs):
    result={}
    for key,value in pairs:
        if key in result:raise AttestationRejected('Duplicate manifest key')
        result[key]=value
    return result


def hexadecimal(value,size):
    if not isinstance(value,str) or len(value)!=size*2 or any(ch not in '0123456789abcdefABCDEF' for ch in value):
        raise AttestationRejected('Invalid sealed byte string')
    return bytes.fromhex(value)


class WindowsReadOnly:
    def __init__(self,pid):
        from runtime_probe import Process
        from run_stereo_continuous import WindowsControl
        self.process=Process(pid)
        try:self.control=WindowsControl()
        except BaseException:self.process.close();raise
    def read(self,address,size):return self.process.read(address,size)
    def identity(self,pid):return self.control.identity(pid)
    def modules(self):
        from start_capture import modules
        return modules(self.process.handle)
    def handle_identity(self):
        kernel=c.WinDLL('kernel32',use_last_error=True)
        kernel.GetProcessId.argtypes=[w.HANDLE];kernel.GetProcessId.restype=w.DWORD
        kernel.GetProcessTimes.argtypes=[w.HANDLE,*([c.POINTER(w.FILETIME)]*4)];kernel.GetProcessTimes.restype=w.BOOL
        stamps=[w.FILETIME() for _ in range(4)]
        pid=kernel.GetProcessId(self.process.handle)
        if not pid or not kernel.GetProcessTimes(self.process.handle,*(c.byref(t) for t in stamps)):
            raise AttestationRejected('Read-only handle identity unavailable')
        return pid,(stamps[0].dwHighDateTime << 32)|stamps[0].dwLowDateTime
    def close(self):self.process.close()


@dataclass(frozen=True)
class Entry:
    target:int
    detour:int
    original:bytes
    patched:bytes
    relay:int
    relay_bytes:bytes


class HookEntryAttestation:
    def __init__(self,manifest_path,*,pid,game_creation,owner_identity,dll,expected_dll_sha256,backend=None):
        self.pid=address(pid,'PID');self.game_creation=address(game_creation,'game creation')
        if pid>0xffffffff:raise AttestationRejected('Invalid PID')
        self.owner=dict(owner_identity)
        address(self.owner['pid'],'owner PID');address(self.owner['creation_time'],'owner creation')
        if self.owner['pid']>0xffffffff or self.owner['pid']==pid or not self.owner.get('image'):
            raise AttestationRejected('Invalid owner identity')
        self.dll=Path(dll).resolve();self.path=Path(manifest_path).resolve()
        if self.path != self.dll.with_name('hook-entry-manifest.json'):
            raise AttestationRejected('Manifest is not the fixed DLL-local file')
        if not isinstance(expected_dll_sha256,str) or len(expected_dll_sha256)!=64 or any(ch not in '0123456789abcdef' for ch in expected_dll_sha256):
            raise AttestationRejected('Invalid expected DLL hash')
        self.dll_hash=expected_dll_sha256
        self.backend=backend if backend is not None else WindowsReadOnly(pid)
        self.owns_backend=backend is None;self.closed=False
        try:
            self.raw=self._manifest_bytes()
            manifest=json.loads(self.raw,object_pairs_hook=unique_object)
            fields={'schema','pid','game_creation','owner_pid','owner_creation','module_base','native_base','manifest_generation','entries'}
            if not isinstance(manifest,dict) or set(manifest)!=fields or type(manifest['schema']) is not int or manifest['schema']!=1:
                raise AttestationRejected('Invalid manifest schema')
            for key in fields-{'entries','schema'}:address(manifest[key],key)
            if (manifest['pid'],manifest['game_creation'],manifest['owner_pid'],manifest['owner_creation'])!=(pid,game_creation,self.owner['pid'],self.owner['creation_time']):
                raise AttestationRejected('Manifest process/owner identity mismatch')
            self.module_base=manifest['module_base'];self.native_base=manifest['native_base']
            self.generation=manifest['manifest_generation']
            entries=manifest['entries']
            if not isinstance(entries,list) or not 1<=len(entries)<=128:raise AttestationRejected('Invalid entry count')
            parsed=[];targets=set();relays=set()
            for item in entries:
                if not isinstance(item,dict) or set(item)!={'target','detour','original','patched','relay','relay_bytes'}:
                    raise AttestationRejected('Invalid entry fields')
                target=address(item['target'],'target',32);relay=address(item['relay'],'relay',14)
                detour=address(item['detour'],'detour')
                original=hexadecimal(item['original'],32);patched=hexadecimal(item['patched'],32);relay_bytes=hexadecimal(item['relay_bytes'],14)
                if target in targets or relay in relays:raise AttestationRejected('Duplicate target/relay')
                targets.add(target);relays.add(relay)
                if patched[0]!=0xe9 or target+5+struct.unpack('<i',patched[1:5])[0]!=relay or patched[5:]!=original[5:]:
                    raise AttestationRejected('Invalid target rel32/tail proof')
                if relay_bytes[:6]!=bytes.fromhex('ff2500000000') or struct.unpack('<Q',relay_bytes[6:])[0]!=detour:
                    raise AttestationRejected('Invalid absolute relay proof')
                parsed.append(Entry(target,detour,original,patched,relay,relay_bytes))
            self.entries=tuple(parsed);self.by_target={entry.target:entry for entry in parsed}
            self.reverify()
        except BaseException:
            self.close();raise

    def _manifest_bytes(self):
        with self.path.open('rb') as stream:data=stream.read(262145)
        if not data or len(data)>262144:raise AttestationRejected('Manifest size limit')
        return data

    def _envelope(self):
        if self.closed:raise AttestationRejected('Attestation closed')
        if self.backend.handle_identity()!=(self.pid,self.game_creation):raise AttestationRejected('RPM handle identity changed')
        game=self.backend.identity(self.pid)
        if not game or game['pid']!=self.pid or game['creation_time']!=self.game_creation:
            raise AttestationRejected('Game identity changed')
        if self.backend.identity(self.owner['pid'])!=self.owner:raise AttestationRejected('Owner identity changed')
        loaded=self.backend.modules();native=loaded.get(self.dll.name.lower());main=loaded.get('gowde-steam.exe')
        if (not native or native[0]!=self.native_base or Path(native[1]).resolve()!=self.dll or
                not main or main[0]!=self.module_base or
                str(Path(main[1]).resolve()).casefold()!=str(game['image']).casefold()):
            raise AttestationRejected('Loaded module identity mismatch')
        with self.dll.open('rb') as stream:data=stream.read(67108865)
        if len(data)>67108864 or hashlib.sha256(data).hexdigest()!=self.dll_hash:
            raise AttestationRejected('Loaded DLL disk hash mismatch')
        # Authenticate the detour range from this exact expected PE, not a guessed
        # module size or a DLL basename alone.
        if len(data)<64 or data[:2]!=b'MZ':raise AttestationRejected('Invalid DLL PE')
        nt=struct.unpack_from('<I',data,0x3c)[0]
        if nt>len(data)-84 or data[nt:nt+4]!=b'PE\0\0' or struct.unpack_from('<H',data,nt+24)[0]!=0x20b:
            raise AttestationRejected('Invalid DLL PE64')
        size=struct.unpack_from('<I',data,nt+80)[0]
        address(self.native_base,'native image',size)
        if not size or any(not self.native_base<=entry.detour<self.native_base+size for entry in self.entries):
            raise AttestationRejected('Detour outside authenticated DLL')

    def _stable(self,address,size):
        first=self.backend.read(address,size);second=self.backend.read(address,size)
        if len(first)!=size or first!=second:raise AttestationRejected('RPM bytes missing/unstable')
        return first

    def _entry(self,entry):
        # Bracket the relay reads with the same target proof. A change while
        # reading the relay must not be accepted as one coherent entry.
        first=self.backend.read(entry.target,32)
        relay_first=self.backend.read(entry.relay,14)
        relay_last=self.backend.read(entry.relay,14)
        last=self.backend.read(entry.target,32)
        if first!=last or relay_first!=relay_last:
            raise AttestationRejected('RPM bytes missing/unstable')
        if first!=entry.patched or last!=entry.patched or relay_first!=entry.relay_bytes or relay_last!=entry.relay_bytes:
            raise AttestationRejected('Live hook differs from sealed bytes')

    def reverify(self):
        self._envelope()
        if self._manifest_bytes()!=self.raw:raise AttestationRejected('Sealed manifest changed')
        for entry in self.entries:self._entry(entry)
        self._envelope()
        if self._manifest_bytes()!=self.raw:raise AttestationRejected('Sealed manifest changed at final boundary')

    def read_entry(self,address,size,actual_read):
        if self.closed:raise AttestationRejected('Attestation closed')
        entry=self.by_target.get(address)
        if entry is None:return actual_read(address,size)
        if type(size) is not int or not 1<=size<=32:raise AttestationRejected('Entry read outside 1..32 bytes')
        self._entry(entry)
        return entry.original[:size]

    def close(self):
        if not self.closed:
            self.closed=True
            if self.owns_backend:self.backend.close()
    def __enter__(self):return self
    def __exit__(self,*args):self.close()
