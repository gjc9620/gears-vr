"""Locate module FName/string candidates and RIP-relative LEA/MOV references.

These are search leads, not verified function identities or runtime call evidence.
"""
import argparse
import json
import pathlib
import re
import struct
import sys
sys.path.insert(0, str(pathlib.Path(__file__).parent/'python-libs'))
import capstone as cs
from capstone.x86 import X86_OP_MEM, X86_REG_RIP
import pefile
from resolve_camera import Resolver


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    parser.add_argument('--name',action='append',help='Exact reflected name to investigate (repeatable)')
    args=parser.parse_args()
    profile=json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    r=Resolver(args.pid,profile)
    try:
        camera=r.resolve()
        wanted={'GetPlayerViewPoint','GetCameraViewPoint','CalcSceneView','Draw','FillCameraCache'}
        if args.name: wanted=set(args.name)
        names={}
        unreadable_names=[]
        for index,pointer in enumerate(r.name_pointers):
            if not pointer:
                continue
            try:
                name=r.name(index)
            except (RuntimeError,UnicodeError):
                unreadable_names.append(index)
                continue
            if name in wanted:
                names[name]=index
        pe=pefile.PE(r.path)
        globals_found={}
        code=[]
        for section in pe.sections:
            if section.Name.rstrip(b'\0') in (b'.themida',b'.boot'):
                continue
            start=r.base+section.VirtualAddress
            raw=r.read(start,section.Misc_VirtualSize)
            if section.Characteristics & 0x20000000:
                code.append((start,raw));continue
            for name,index in names.items():
                needle=struct.pack('<II',index,0)
                pos=raw.find(needle)
                while pos>=0:
                    if pos%4==0:
                        globals_found[start+pos]=name
                    pos=raw.find(needle,pos+1)
            for name in wanted:
                for needle in ((name+'\0').encode('utf-16-le'), (name+'\0').encode('ascii')):
                    pos=raw.find(needle)
                    while pos>=0:
                        globals_found[start+pos]=name+' (string)'
                        pos=raw.find(needle,pos+1)
        decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64)
        decoder.detail=True
        refs=[]
        # RIP-relative LEA/MOV loads; the byte prefilter is validated by Capstone.
        pattern=re.compile(rb'[\x40-\x4f]?[\x8d\x8b][\x05\x0d\x15\x1d\x25\x2d\x35\x3d][\s\S]{4}')
        for start,raw in code:
            for match in pattern.finditer(raw):
                pos=match.start()
                address=start+pos
                target=address+len(match.group())+struct.unpack_from('<i',match.group(),len(match.group())-4)[0]
                if target not in globals_found:
                    continue
                insn=next(decoder.disasm(raw[pos:pos+16],address,count=1),None)
                if not insn or not any(op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP
                                      and insn.address+insn.size+op.mem.disp==target for op in insn.operands):
                    continue
                # Start only at a nearby INT3 boundary if one exists; otherwise
                # show from the proven LEA boundary, never pretend a guessed start is valid.
                boundary=raw.rfind(b'\xcc\xcc',max(0,pos-256),pos)
                beginning=boundary+2 if boundary>=0 else pos
                while beginning<pos and raw[beginning]==0xcc:
                    beginning+=1
                context=[dict(rva=hex(i.address-r.base),mnemonic=i.mnemonic,operands=i.op_str)
                         for i in decoder.disasm(raw[beginning:pos+192],start+beginning)]
                refs.append(dict(name=globals_found[target],name_global_rva=hex(target-r.base),
                                 reference_rva=hex(address-r.base),context_start_rva=hex(start+beginning-r.base),
                                 context_start_from_padding=boundary>=0,instructions=context))
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(dict(pid=args.pid,names=names,unreadable_name_indices=unreadable_names,
            global_candidates=[dict(rva=hex(a-r.base),name=n) for a,n in globals_found.items()],references=refs,
            caveat='FName value matches and LEA references require dataflow validation; padding is not a function boundary guarantee.'),indent=2),encoding='utf-8')
        print('Names:',names,'global candidates:',len(globals_found),'code references:',len(refs))
        for item in refs:
            print(item['name'],item['reference_rva'],'context start',item['context_start_rva'])
    finally:
        r.p.close()


if __name__=='__main__':
    main()
