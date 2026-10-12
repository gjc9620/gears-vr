"""Read a bounded candidate vtable in a hash-validated live game, without calls."""
import argparse
import json
import pathlib
import struct
import sys

sys.path.insert(0,str(pathlib.Path(__file__).with_name('python-libs')))
import pefile
from resolve_camera import Resolver


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--rva',type=lambda x:int(x,0),required=True)
    parser.add_argument('--slots',type=int,choices=range(1,17),default=4)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    profile=json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    resolver=Resolver(args.pid,profile)
    try:
        resolver.prepare()
        with pefile.PE(str(resolver.path)) as pe:
            size=pe.OPTIONAL_HEADER.SizeOfImage
            if args.rva<0 or args.rva+args.slots*8>size or args.rva%8:raise RuntimeError('Candidate outside aligned module data')
            sections=[s for s in pe.sections if s.VirtualAddress<=args.rva and args.rva+args.slots*8<=s.VirtualAddress+s.Misc_VirtualSize]
            if len(sections)!=1 or sections[0].Characteristics&0x20000000:raise RuntimeError('Candidate must lie in one non-executable section')
            values=struct.unpack('<'+'Q'*args.slots,resolver.read(resolver.base+args.rva,args.slots*8))
            entries=[]
            for i,address in enumerate(values):
                rva=address-resolver.base
                code=any(s.VirtualAddress<=rva<s.VirtualAddress+s.Misc_VirtualSize and s.Characteristics&0x20000000 and s.Name.rstrip(b'\0') not in (b'.themida',b'.boot') for s in pe.sections)
                entries.append({'slot':hex(i*8),'address':hex(address),'entry_rva':hex(rva) if 0<=rva<size else None,'ordinary_code':bool(code)})
        report={'pid':args.pid,'module_base':hex(resolver.base),'candidate_rva':hex(args.rva),'entries':entries,
            'limitation':'Bounded words at an inferred table address. Adjacent entries may belong to other tables; type, slot membership and execution are not proved.'}
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
    finally:resolver.p.close()


if __name__=='__main__':main()
