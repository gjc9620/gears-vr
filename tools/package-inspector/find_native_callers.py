"""Read-only direct CALL/JMP cross-reference search in ordinary executable sections."""
import argparse
import json
import pathlib
import struct
import sys
sys.path.insert(0,str(pathlib.Path(__file__).parent/'python-libs'))
import capstone as cs
import pefile
from resolve_camera import Resolver


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--target-rva',type=lambda v:int(v,0),required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    profile=json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    r=Resolver(args.pid,profile)
    try:
        r.resolve()
        target=r.base+args.target_rva
        decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64)
        refs=[]
        for section in pefile.PE(r.path).sections:
            if not section.Characteristics&0x20000000 or section.Name.rstrip(b'\0') in (b'.themida',b'.boot'):
                continue
            start=r.base+section.VirtualAddress
            raw=r.read(start,section.Misc_VirtualSize)
            for opcode in (b'\xe8',b'\xe9'):
                pos=raw.find(opcode)
                while 0<=pos<=len(raw)-5:
                    address=start+pos
                    if address+5+struct.unpack_from('<i',raw,pos+1)[0]==target:
                        boundary=raw.rfind(b'\xcc\xcc',max(0,pos-8192),pos)
                        beginning=boundary+2 if boundary>=0 else pos
                        while beginning<pos and raw[beginning]==0xcc:
                            beginning+=1
                        insns=list(decoder.disasm(raw[beginning:pos+160],start+beginning))
                        aligned=any(i.address==address and i.mnemonic in ('call','jmp') for i in insns)
                        refs.append(dict(reference_rva=hex(address-r.base),opcode='call' if opcode==b'\xe8' else 'jmp',
                            candidate_start_rva=hex(start+beginning-r.base),padding_boundary_found=boundary>=0,
                            linear_decode_reaches_reference=aligned,
                            context=[dict(rva=hex(i.address-r.base),mnemonic=i.mnemonic,operands=i.op_str)
                                     for i in insns if abs(i.address-address)<128]))
                    pos=raw.find(opcode,pos+1)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(dict(pid=args.pid,target_rva=hex(args.target_rva),references=refs,
            caveat='Byte candidates validated by linear decode from nearby padding; candidate starts are not guaranteed function boundaries.'),indent=2),encoding='utf-8')
        for item in refs:
            print(item['opcode'],item['reference_rva'],'start',item['candidate_start_rva'],'aligned',item['linear_decode_reaches_reference'])
    finally:
        r.p.close()


if __name__=='__main__':
    main()
