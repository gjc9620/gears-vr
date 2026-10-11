"""Read-only bounded CFG analysis of reflected camera native entry points."""
import argparse
import json
import pathlib
import struct
import sys
sys.path.insert(0,str(pathlib.Path(__file__).parent/'python-libs'))
import capstone as cs
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
import pefile
from resolve_camera import Resolver

parser=argparse.ArgumentParser()
parser.add_argument('--pid',type=int,required=True)
parser.add_argument('--output',type=pathlib.Path,required=True)
args=parser.parse_args()
profile=json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
r=Resolver(args.pid,profile)
try:
    camera=r.resolve()
    pe=pefile.PE(r.path)
    code_ranges=[(r.base+s.VirtualAddress,r.base+s.VirtualAddress+s.Misc_VirtualSize)
                 for s in pe.sections if s.Characteristics & 0x20000000
                 and s.Name.rstrip(b'\0') not in (b'.themida',b'.boot')]
    def ordinary_code(address):return any(lo<=address<hi for lo,hi in code_ranges)
    decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64)
    decoder.detail=True
    def graph(start,limit=2500):
        pending=[start]
        instructions={}
        edges=[]
        blocked=[]
        while pending and len(instructions)<limit:
            address=pending.pop()
            while address not in instructions and len(instructions)<limit:
                if not ordinary_code(address):
                    blocked.append(hex(address));break
                raw=r.p.read(address,16)
                insn=next(decoder.disasm(raw,address,count=1),None)
                if insn is None:blocked.append(hex(address));break
                operands=[]
                for operand in insn.operands:
                    if operand.type==X86_OP_MEM:
                        operands.append(dict(base=insn.reg_name(operand.mem.base),index=insn.reg_name(operand.mem.index),
                            displacement=operand.mem.disp,access=operand.access,size=operand.size))
                instructions[address]=dict(address=hex(address),rva=hex(address-r.base),mnemonic=insn.mnemonic,
                    operands=insn.op_str,memory_operands=operands)
                if insn.group(cs.CS_GRP_CALL):
                    target=insn.operands[0].imm if insn.operands and insn.operands[0].type==X86_OP_IMM else None
                    edges.append(dict(kind='call',source=hex(address),target=hex(target) if target is not None else insn.op_str))
                if insn.group(cs.CS_GRP_RET) or insn.mnemonic in ('int3','ud2'):
                    break
                if insn.group(cs.CS_GRP_JUMP):
                    target=insn.operands[0].imm if insn.operands and insn.operands[0].type==X86_OP_IMM else None
                    edges.append(dict(kind='branch',source=hex(address),target=hex(target) if target is not None else insn.op_str))
                    if target is not None and ordinary_code(target):pending.append(target)
                    if insn.mnemonic=='jmp':break
                address+=insn.size
        return dict(entry=hex(start),instruction_count=len(instructions),limit_reached=bool(pending) or len(instructions)>=limit,
                    instructions=[instructions[a] for a in sorted(instructions)],edges=edges,
                    blocked_addresses=sorted(set(blocked)))
    cam_class=r.u64(int(camera['camera_address'],16)+profile['object_class'])
    native=next(f for f in r.fields(cam_class) if f['name']=='PlayerUpdateCameraNative' and r.class_name(f)=='Function')
    entry=r.u64(native['address']+0x114)
    if not ordinary_code(entry):raise RuntimeError('Native entry outside ordinary executable sections')
    wrapper=graph(entry,500)
    body_candidates=sorted({int(e['target'],16) for e in wrapper['edges'] if e['kind']=='call' and e['target'].startswith('0x')})
    bodies=[graph(address) for address in body_candidates if ordinary_code(address)]
    view_target=r.property(cam_class,'ViewTarget',True)
    vt_struct=r.struct_type(view_target)
    vt_fields=[]
    for field in r.fields(vt_struct):
        if r.class_name(field).endswith('Property'):
            vt_fields.append(dict(name=field['name'],type=r.class_name(field),offset=hex(r.u32(field['address']+profile['property_offset_internal']))))
    result=dict(pid=args.pid,module_base=hex(r.base),sha256=profile['sha256'],camera=camera,
        view_target_type=r.identity(vt_struct)['name'],view_target_fields=vt_fields,
        reflected_function_object=hex(native['address']),entry_pointer_field='0x114',wrapper=wrapper,bodies=bodies,
        caveat='Reachable ordinary-code branch graph, not proof of runtime cadence or final renderer consumption.')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    lines=[]
    for label,g in [('wrapper',wrapper)]+[(f'body-{i}',g) for i,g in enumerate(bodies)]:
        lines.append(f'{label}: {g["entry"]}, {g["instruction_count"]} instructions, limit={g["limit_reached"]}')
        lines.extend(f'{v["address"]}: {v["mnemonic"]} {v["operands"]}' for v in g['instructions'])
    args.output.with_suffix('.asm.txt').write_text('\n'.join(lines),encoding='utf-8')
    print('ViewTarget:',result['view_target_type'],vt_fields)
    print('Wrapper',wrapper['entry'],'instructions',wrapper['instruction_count'],'body candidates',[hex(v) for v in body_candidates])
    for body in bodies:
        print('Body',body['entry'],'instructions',body['instruction_count'],'limit',body['limit_reached'])
        print('Calls',[e for e in body['edges'] if e['kind']=='call'])
        interesting=[i for i in body['instructions'] if any(m['displacement'] in (0x10,0x1c,0x28,0x3ac,0x3b0,0x3cc) for m in i['memory_operands'])]
        print('Layout-related memory accesses',interesting[:30])
finally:
    r.p.close()
