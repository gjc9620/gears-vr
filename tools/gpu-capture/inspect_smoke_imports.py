"""Read the actual wrapped entry addresses in our own D3D12 smoke process."""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
import pathlib
import struct
import sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/native-capture'))
from start_capture import k,modules,pefile
from runtime_probe import Process

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--build-dir',type=pathlib.Path,default=ROOT/'tools/gpu-capture/build-probe2')
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args();process=Process(args.pid)
    try:
        path=c.create_unicode_buffer(32768);length=w.DWORD(len(path))
        if not k.QueryFullProcessImageNameW(process.handle,0,path,c.byref(length)):raise c.WinError(c.get_last_error())
        exe=(args.build_dir/'D3D12Smoke.exe').resolve()
        if pathlib.Path(path.value).resolve()!=exe:raise RuntimeError('Not our expected smoke process')
        loaded=modules(process.handle);base,_=loaded['d3d12smoke.exe']
        rd_base,rd_path=loaded['renderdoc.dll']
        if rd_path.resolve()!=(ROOT/'tools/third-party/renderdoc/RenderDoc_1.46_64/renderdoc.dll').resolve():
            raise RuntimeError('Unexpected RenderDoc module')
        rows=[]
        with pefile.PE(str(exe)) as pe:
            for descriptor in pe.DIRECTORY_ENTRY_IMPORT:
                for entry in descriptor.imports:
                    if entry.name not in (b'D3D12CreateDevice',b'D3D12GetInterface',b'CreateDXGIFactory1',
                                          b'GetProcAddress',b'LoadLibraryW',b'LoadLibraryExW'):continue
                    slot=base+entry.address-pe.OPTIONAL_HEADER.ImageBase
                    pointer=struct.unpack('<Q',process.read(slot,8))[0]
                    rows.append(dict(name=entry.name.decode(),slot=hex(slot),pointer=hex(pointer),
                                     renderdoc_rva=hex(pointer-rd_base),entry_bytes=process.read(pointer,32).hex()))
        report=dict(pid=args.pid,renderdoc_sha256=hashlib.sha256(rd_path.read_bytes()).hexdigest(),entries=rows)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
    finally:process.close()
if __name__=='__main__':main()
