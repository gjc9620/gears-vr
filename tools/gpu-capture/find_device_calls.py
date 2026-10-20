"""Bounded read-only search for graphics export pointers and API names.

Only the validated game's ordinary sections are read, in 1 MiB chunks. The
results are leads: pointer occurrences do not prove calls were executed.
"""
import argparse
import json
import pathlib
import re
import struct
import sys
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/native-capture'))
from start_capture import modules, pefile
from resolve_camera import Resolver
import capstone as cs

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    resolver=Resolver(args.pid,profile)
    try:
        resolver.prepare(); loaded=modules(resolver.p.handle)
        patterns=[]; exports=[]
        wanted={'d3d12.dll':('D3D12CreateDevice','D3D12GetInterface'),
                'dxgi.dll':('CreateDXGIFactory','CreateDXGIFactory1','CreateDXGIFactory2')}
        for module,names in wanted.items():
            module_base,path=loaded[module]
            with pefile.PE(str(path)) as pe:
                for export in pe.DIRECTORY_ENTRY_EXPORT.symbols:
                    if export.name and export.name.decode('ascii') in names:
                        name=export.name.decode('ascii')
                        address=module_base+export.address
                        exports.append(dict(name=name,address=hex(address),forwarder=repr(export.forwarder),
                                            entry_bytes=resolver.p.read(address,32).hex()))
                        patterns.append((name+'.pointer',struct.pack('<Q',address)))
                        patterns.append((name+'.name',export.name+b'\0'))
        hits={name:[] for name,_ in patterns}; total=0; sections=[]
        with pefile.PE(resolver.path,fast_load=True) as pe:
            for section in pe.sections:
                name=section.Name.rstrip(b'\0').decode('ascii','replace')
                if name in ('.themida','.boot') or not section.Characteristics&0x40000000: continue
                size=section.Misc_VirtualSize
                if size>128*1024*1024 or total+size>256*1024*1024: raise RuntimeError('Section scan bound exceeded')
                sections.append(dict(name=name,rva=hex(section.VirtualAddress),size=size))
                for offset in range(0,size,1024*1024):
                    address=resolver.base+section.VirtualAddress+offset
                    data=resolver.p.read(address,min(size-offset,1024*1024+64));total+=len(data)
                    for label,pattern in patterns:
                        pos=data.find(pattern)
                        while pos>=0:
                            rva=hex(address+pos-resolver.base)
                            if rva not in hits[label] and len(hits[label])<64: hits[label].append(rva)
                            pos=data.find(pattern,pos+1)
        pointer_slots={resolver.base+int(rva,16):label for label,values in hits.items()
                       if label.endswith('.pointer') for rva in values}
        references=[]; seen=set(); decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64);decoder.detail=True
        candidate=re.compile(rb'\xff[\x15\x25]|[\x48\x4c]\x8b[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]')
        with pefile.PE(resolver.path,fast_load=True) as pe:
            for section in pe.sections:
                if not section.Characteristics&0x20000000 or section.Name.rstrip(b'\0') in (b'.themida',b'.boot'):continue
                if section.Misc_VirtualSize>128*1024*1024:raise RuntimeError('Code scan bound exceeded')
                for offset in range(0,section.Misc_VirtualSize,1024*1024):
                    address=resolver.base+section.VirtualAddress+offset
                    data=resolver.p.read(address,min(section.Misc_VirtualSize-offset,1024*1024+16))
                    for match in candidate.finditer(data):
                        ip=address+match.start()
                        instruction=next(decoder.disasm(data[match.start():match.start()+16],ip,count=1),None)
                        if instruction is None:continue
                        for operand in instruction.operands:
                            if operand.type==cs.x86.X86_OP_MEM and operand.mem.base==cs.x86.X86_REG_RIP:
                                target=ip+instruction.size+operand.mem.disp
                                if target in pointer_slots and ip not in seen:
                                    seen.add(ip);references.append(dict(rva=hex(ip-resolver.base),
                                        slot_rva=hex(target-resolver.base),api=pointer_slots[target],
                                        mnemonic=instruction.mnemonic,operands=instruction.op_str))
        report=dict(pid=args.pid,mode='read-only',bytes_read=total,sections=sections,exports=exports,hits=hits,references=references)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
        print(json.dumps(report,indent=2))
    finally: resolver.p.close()
if __name__=='__main__':main()
