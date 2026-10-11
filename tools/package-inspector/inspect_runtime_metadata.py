from runtime_probe import Process, readable_heap
import json
import pathlib
import struct
import sys

pid, candidates_path, output = int(sys.argv[1]), pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3])
data = json.loads(candidates_path.read_text())
names = {int(i): name for i,name in json.loads(candidates_path.with_name('runtime-names.json').read_text()).items()}
p = Process(pid)
def identity(address):
    h=p.read(address,0x60)
    if len(h)<0x60: return None
    idx,number=struct.unpack_from('<II',h,0x48)
    return dict(address=hex(address), name=names.get(idx,'?'), number=number,
                outer=hex(struct.unpack_from('<Q',h,0x40)[0]),
                cls=hex(struct.unpack_from('<Q',h,0x50)[0]))
try:
    valid=[]
    for o in data['objects']:
        if o['name_offset']!='0x48': continue
        address=int(o['address'],16)
        h=p.read(address,0x110)
        info=identity(address)
        info['class_identity']=identity(int(info['cls'],16))
        info['outer_identity']=identity(int(info['outer'],16))
        info['dwords_80_110']={hex(pos):hex(struct.unpack_from('<I',h,pos)[0]) for pos in range(0x80,len(h),4)}
        valid.append(info)
    camera_class=next(int(v['address'],16) for v in valid if v['name']=='WarPlayerCamera' and v['class_identity']['name']=='Class')
    needle=struct.pack('<Q',camera_class)
    instances=[]
    for r in p.regions():
        if not readable_heap(r) or r['base']>0x140000000: continue
        for offset in range(0,r['size'],4*1024*1024):
            base=r['base']+offset
            b=p.read(base,min(4*1024*1024+8,r['size']-offset))
            pos=b.find(needle)
            while pos>=0:
                address=base+pos-0x50
                if address%8==0:
                    info=identity(address)
                    if info and info['name']=='WarPlayerCamera' and int(info['cls'],16)==camera_class:
                        header=p.read(address,8)
                        if len(header)==8 and 0x140000000<=struct.unpack('<Q',header)[0]<0x142036000:
                            info['outer_identity']=identity(int(info['outer'],16))
                            instances.append(info)
                pos=b.find(needle,pos+8)
    result=dict(pid=pid,metadata=valid,instances=instances)
    output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('Camera instances:',json.dumps(instances,indent=2))
    for v in valid:
        if v['name'] in ('CameraCache','TCameraCache','POV','TPOV','Location','Rotation','FOV'):
            print(v['name'],v['address'],'outer',v['outer_identity'],v['dwords_80_110'])
finally:
    p.close()
