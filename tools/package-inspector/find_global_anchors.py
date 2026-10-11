"""Discover module-relative name/object array anchors from validated objects."""
from runtime_probe import Process, readable_heap
import json
import pathlib
import struct
import sys
sys.path.insert(0,str(pathlib.Path(__file__).parent/'python-libs'))
import pefile

metadata_path=pathlib.Path(sys.argv[1])
metadata=json.loads(metadata_path.read_text())
objects=json.loads(metadata_path.with_name('camera-object-candidates.json').read_text())
image=json.loads(pathlib.Path(sys.argv[2]).read_text())
pe=pefile.PE(image['path'])
base=int(image['image_base'],16)
p=Process(metadata['pid'])
try:
    table=int(objects['name_table_base'],16)
    known=[v for v in metadata['metadata'] if v['name'] in ('WarPlayerCamera','WarPC','WarLocalPlayer') and v['class_identity'] and v['class_identity']['name']=='Class']
    pointers={int(v['address'],16):struct.unpack('<I',p.read(int(v['address'],16)+8,4))[0] for v in known}
    print('Validated class/internal-index pairs',[(hex(ptr),index) for ptr,index in pointers.items()],flush=True)
    obj_bases=set()
    primary=next(int(v['address'],16) for v in known if v['name']=='WarPlayerCamera')
    index=pointers[primary]
    needle=struct.pack('<Q',primary)
    for r in p.regions():
        if not readable_heap(r):continue
        # Object pointer arrays are modest allocations; skip giant buffers.
        if r['size']>32*1024*1024:continue
        for offset in range(0,r['size'],4*1024*1024):
            addr=r['base']+offset
            raw=p.read(addr,min(4*1024*1024+8,r['size']-offset))
            pos=raw.find(needle)
            while pos>=0:
                loc=addr+pos
                candidate=loc-index*8
                if loc%8==0 and candidate>0 and all(p.read(candidate+i*8,8)==struct.pack('<Q',ptr) for ptr,i in pointers.items()):
                    obj_bases.add(candidate)
                pos=raw.find(needle,pos+8)
        if obj_bases:break
    print('Validated object-array data bases',[hex(v) for v in obj_bases],flush=True)
    targets={struct.pack('<Q',table):'names'}
    targets.update({struct.pack('<Q',b):'objects' for b in obj_bases})
    anchors=[]
    for section in pe.sections:
        name=section.Name.rstrip(b'\0').decode(errors='replace')
        if name in ('.themida','.boot'):continue
        section_base=base+section.VirtualAddress
        raw=p.read(section_base,section.Misc_VirtualSize)
        for needle,kind in targets.items():
            pos=raw.find(needle)
            while pos>=0:
                loc=section_base+pos
                header=p.read(loc,16)
                if len(header)==16 and loc%8==0:
                    data_ptr,num,capacity=struct.unpack('<Qii',header)
                    if 1000<num<=capacity<4000000:
                        anchors.append(dict(kind=kind,address=hex(loc),rva=hex(loc-base),data=hex(data_ptr),num=num,capacity=capacity,section=name))
                pos=raw.find(needle,pos+8)
    result=dict(pid=metadata['pid'],module_base=hex(base),sha256=image['sha256'],object_array_data_candidates=[hex(v) for v in obj_bases],anchors=anchors)
    pathlib.Path(sys.argv[3]).write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result,indent=2))
finally:
    p.close()
