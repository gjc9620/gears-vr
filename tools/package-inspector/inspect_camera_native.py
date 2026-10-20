"""Disassemble the reflected native entry candidate in ordinary code only."""
import json
import pathlib
import struct
import sys
sys.path.insert(0,str(pathlib.Path(__file__).parent/'python-libs'))
import capstone
from runtime_probe import Process

fields_path=pathlib.Path(sys.argv[1])
data=json.loads(fields_path.read_text())
p=Process(data['pid'])
try:
    function=next(f for f in data['fields'] if f['name']=='PlayerUpdateCameraNative' and f['owner']=='WarPlayerCamera')
    entry=next(v for v in function['native_pointer_candidates'] if v['field_offset']=='0x114')
    address=int(entry['address'],16)
    assert 0x140000000<=address<0x142036000
    region=next(r for r in p.regions() if r['base']<=address<r['base']+r['size'])
    assert region['protect']&0xff in (0x10,0x20,0x40,0x80)
    code=p.read(address,256)
    dis=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
    instructions=[]
    for insn in dis.disasm(code,address):
        instructions.append(f'{insn.address:#x}: {insn.mnemonic} {insn.op_str}')
        if insn.mnemonic=='ret':break
    result=dict(pid=data['pid'],function=function['name'],entry_candidate=entry,
                region_protection=hex(region['protect']),instructions=instructions,
                caveat='Reflected VM/native dispatch entry candidate, not a proven final render-side camera writer.')
    pathlib.Path(sys.argv[2]).write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result,indent=2))
finally:
    p.close()
