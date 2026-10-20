"""Read-only bounded search for RIP-relative LEA/MOV references to native data.

Instruction-byte candidates are decoded individually and from nearby padding.
Neither test establishes a function boundary; use the control-flow scanner next.
"""
import argparse
import json
import pathlib
import re
import struct
import sys
sys.path.insert(0,str(pathlib.Path(__file__).parent/'python-libs'))
import capstone as cs
from capstone.x86 import X86_OP_MEM, X86_REG_RIP
import pefile
from resolve_camera import Resolver


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pid',type=int,required=True)
    p.add_argument('--target-rva',type=lambda v:int(v,0),required=True)
    p.add_argument('--output',type=pathlib.Path,required=True)
    a=p.parse_args()
    r=Resolver(a.pid,json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text()))
    try:
        r.prepare();target=r.base+a.target_rva
        decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64);decoder.detail=True
        refs=[];scanned=0;chunk=1024*1024
        pe=pefile.PE(r.path,fast_load=True)
        try:sections=[s for s in pe.sections if s.Characteristics&0x20000000 and s.Name.rstrip(b'\0') not in (b'.themida',b'.boot')]
        finally:pe.close()
        total=sum(s.Misc_VirtualSize for s in sections)
        if total>128*1024*1024:raise RuntimeError('Ordinary-code scan exceeds 128 MiB bound')
        pattern=re.compile(rb'[\x40-\x4f][\x8d\x8b][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]')
        for section in sections:
            start=r.base+section.VirtualAddress;size=section.Misc_VirtualSize
            for offset in range(0,size,chunk):
                primary=min(chunk,size-offset)
                raw=r.read(start+offset,min(primary+6,size-offset));scanned+=primary
                for match in pattern.finditer(raw):
                    pos=match.start()
                    if pos>=primary or pos+7>len(raw):continue
                    address=start+offset+pos
                    if address+7+struct.unpack_from('<i',raw,pos+3)[0]!=target:continue
                    insn=next(decoder.disasm(raw[pos:pos+15],address,count=1),None)
                    if not insn or not any(op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP and insn.address+insn.size+op.mem.disp==target for op in insn.operands):continue
                    if len(refs)>=256:raise RuntimeError('Reference result exceeds 256 candidates')
                    begin=max(start,address-8192);context=r.read(begin,address-begin+7)
                    boundary=context.rfind(b'\xcc\xcc',0,address-begin)
                    linear_start=boundary+2 if boundary>=0 else address-begin
                    while linear_start<address-begin and context[linear_start]==0xcc:linear_start+=1
                    decoded=list(decoder.disasm(context[linear_start:],begin+linear_start))
                    refs.append({'reference_rva':hex(address-r.base),'mnemonic':insn.mnemonic,'operands':insn.op_str,
                        'candidate_start_rva':hex(begin+linear_start-r.base),'padding_boundary_found':boundary>=0,
                        'linear_decode_reaches_reference':any(i.address==address for i in decoded),
                        'context':[{'rva':hex(i.address-r.base),'mnemonic':i.mnemonic,'operands':i.op_str} for i in decoded if address-96<=i.address<=address]})
        report={'pid':a.pid,'target_rva':hex(a.target_rva),'bytes_scanned':scanned,'references':refs,
            'limits':'REX-prefixed LEA/MOV RIP-relative references only. Padding and byte decoding are leads, not proven function starts.'}
        a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2))
        print(json.dumps({k:v for k,v in report.items() if k!='references'}))
        for item in refs:print(item['reference_rva'],item['candidate_start_rva'],item['linear_decode_reaches_reference'])
    finally:r.p.close()


if __name__=='__main__':main()
