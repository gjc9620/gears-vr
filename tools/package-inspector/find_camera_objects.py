"""Empirically validate name-array evidence and find UObject header candidates."""
from runtime_probe import Process, readable_heap
import json
import pathlib
import struct
import sys

pid, table_candidates, output = int(sys.argv[1]), pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3])
p = Process(pid)
try:
    refs = json.loads(table_candidates.read_text())
    bases = []
    for ref in refs:
        index = struct.unpack('<I', p.read(ref['entry'], 4))[0]
        bases.append(int(ref['pointer_location'],16) - index*8)
    assert len(set(bases)) == 1, ('Name table bases disagree', bases)
    base = bases[0]
    pointers = p.read(base, 65536*8)
    names = {}
    for i in range(len(pointers)//8):
        ptr = struct.unpack_from('<Q',pointers,i*8)[0]
        if not ptr:
            continue
        entry = p.read(ptr, 128)
        if len(entry) < 22 or struct.unpack_from('<I',entry)[0] != i:
            continue
        name = entry[20:].decode('utf-16-le',errors='replace').split('\0')[0]
        names[i] = name
    wanted = {i: name for i,name in names.items() if name in (
        'WarPlayerCamera','Default__WarPlayerCamera','CameraCache','WarPC','WarLocalPlayer',
        'PlayerUpdateCameraNative','TCameraCache','POV','TPOV','Location','Rotation','FOV')}
    needles = {struct.pack('<II',i,0): name for i,name in wanted.items()}
    objects=[]
    for r in p.regions():
        if not readable_heap(r) or r['base'] > 0x40000000:
            continue
        for offset in range(0,r['size'],4*1024*1024):
            addr=r['base']+offset
            data=p.read(addr,min(4*1024*1024+128,r['size']-offset))
            for needle, name in needles.items():
                pos=data.find(needle)
                while pos>=0:
                    name_address=addr+pos
                    if name_address % 8 == 0:
                        for name_offset in range(24,121,8):
                            obj_addr=name_address-name_offset
                            header=p.read(obj_addr,128)
                            if len(header)!=128:
                                continue
                            vtable=struct.unpack_from('<Q',header)[0]
                            if not 0x140000000<=vtable<0x142036000:
                                continue
                            first_method=p.read(vtable,8)
                            if len(first_method)!=8:
                                continue
                            fn=struct.unpack('<Q',first_method)[0]
                            if not 0x140000000<=fn<0x142036000:
                                continue
                            objects.append(dict(name=name,address=hex(obj_addr),name_offset=hex(name_offset),
                                                words=[hex(v) for v in struct.unpack('<16Q',header)]))
                    pos=data.find(needle,pos+8)
    result=dict(pid=pid,name_table_base=hex(base),validated_name_count=len(names),name_ids=wanted,objects=objects)
    output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    output.with_name('runtime-names.json').write_text(json.dumps(names),encoding='utf-8')
    print(json.dumps(result,indent=2))
finally:
    p.close()
