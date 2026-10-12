"""Load our import-inspection DLL into the version-validated game; no patching.

This is instrumentation, not strictly read-only: it allocates loader memory and
loads a DLL. The DLL only observes its own import function addresses.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import hashlib
import pathlib
import sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/native-capture'))
from start_capture import k,modules,remote_call,pefile
from resolve_camera import Resolver

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--build-dir',type=pathlib.Path,default=ROOT/'tools/gpu-capture/build')
    parser.add_argument('--dll-name',choices=('BridgeProbe.dll','BridgeProbe2.dll','BridgeProbe3.dll'),default='BridgeProbe.dll')
    parser.add_argument('--query-manifest',type=pathlib.Path,help='Use exact hash/entry bytes captured from our smoke process GetProcAddress import')
    args=parser.parse_args()
    profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    resolver=Resolver(args.pid,profile)
    try: resolver.prepare()
    finally: resolver.p.close()
    dll=(args.build_dir/args.dll_name).resolve()
    handle=k.OpenProcess(0x43a,False,args.pid)
    if not handle:raise c.WinError(c.get_last_error())
    memory=None;finished=False
    try:
        loaded=modules(handle)
        rd=loaded.get('renderdoc.dll')
        if not rd or rd[1].resolve()!=(ROOT/'tools/third-party/renderdoc/RenderDoc_1.46_64/renderdoc.dll').resolve():
            raise RuntimeError('Expected RenderDoc path not loaded')
        query_address=None
        if args.query_manifest:
            manifest=json.loads(args.query_manifest.read_text())
            if hashlib.sha256(rd[1].read_bytes()).hexdigest()!=manifest['renderdoc_sha256']:
                raise RuntimeError('RenderDoc manifest hash differs')
            entry=next(x for x in manifest['entries'] if x['name']=='GetProcAddress')
            query_address=rd[0]+int(entry['renderdoc_rva'],16)
            from runtime_probe import Process
            memory_reader=Process(args.pid)
            try:
                expected=bytes.fromhex(entry['entry_bytes'])
                if len(expected)!=32 or memory_reader.read(query_address,32)!=expected:
                    raise RuntimeError('RenderDoc query entry differs')
            finally: memory_reader.close()
        resident=loaded.get(dll.name.lower())
        if resident:raise RuntimeError('Probe already resident; do not reload blindly')
        with pefile.PE(str(dll)) as pe:
            export=next(x for x in pe.DIRECTORY_ENTRY_EXPORT.symbols if x.name==b'GearsRenderDocProbe')
            if export.forwarder:raise RuntimeError('Forwarded probe rejected')
            export_rva=export.address
        loader=k.GetProcAddress(k.GetModuleHandleW('kernel32.dll'),b'LoadLibraryW');owner=w.HMODULE()
        if not k.GetModuleHandleExW(6,loader,c.byref(owner)):raise c.WinError(c.get_last_error())
        filename=c.create_unicode_buffer(32768);k.GetModuleFileNameW(owner,filename,len(filename))
        remote=loaded[pathlib.Path(filename.value).name.lower()][0]+loader-owner.value
        encoded=(str(dll)+'\0').encode('utf-16-le')
        memory=k.VirtualAllocEx(handle,None,len(encoded),0x3000,0x04)
        if not memory:raise c.WinError(c.get_last_error())
        count=c.c_size_t();buffer=c.create_string_buffer(encoded)
        if not k.WriteProcessMemory(handle,memory,buffer,len(encoded),c.byref(count)) or count.value!=len(encoded):
            raise c.WinError(c.get_last_error())
        remote_call(handle,remote,memory);finished=True
        loaded=modules(handle);resident=loaded.get(dll.name.lower())
        if not resident or resident[1].resolve()!=dll:raise RuntimeError('Probe load not confirmed')
        result=remote_call(handle,resident[0]+export_rva,query_address)
        if result:raise RuntimeError(f'Probe failed {result}')
        output=ROOT/f'analysis/renderdoc/bridge-{args.pid}.txt'
        report=output.read_text(encoding='utf-8-sig');print(report)
        if args.query_manifest:
            import re
            from runtime_probe import Process
            memory_reader=Process(args.pid);entries=[]
            try:
                with pefile.PE(str(rd[1]),fast_load=True) as image: size=image.OPTIONAL_HEADER.SizeOfImage
                for name,address in re.findall(r'queried\.(\w+) address=([0-9A-Fa-f]+)',report):
                    pointer=int(address,16)
                    if not rd[0]<=pointer<rd[0]+size:raise RuntimeError('Queried address outside RenderDoc')
                    entries.append(dict(name=name,rva=hex(pointer-rd[0]),entry_bytes=memory_reader.read(pointer,32).hex()))
            finally:memory_reader.close()
            output.with_suffix('.json').write_text(json.dumps(dict(renderdoc_sha256=manifest['renderdoc_sha256'],
                entries=entries),indent=2),encoding='utf-8')
    finally:
        if memory and finished:k.VirtualFreeEx(handle,memory,0,0x8000)
        k.CloseHandle(handle)

if __name__=='__main__':main()
