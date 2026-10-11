"""Read reflected camera fields and native entry-pointer candidates."""
from runtime_probe import Process
import json
import pathlib
import struct
import sys

metadata_path=pathlib.Path(sys.argv[1])
metadata=json.loads(metadata_path.read_text())
names={int(k):v for k,v in json.loads(metadata_path.with_name('runtime-names.json').read_text()).items()}
p=Process(metadata['pid'])
def ident(addr):
    raw=p.read(addr,0x60)
    if len(raw)!=0x60:return None
    return dict(address=hex(addr),name=names.get(struct.unpack_from('<I',raw,0x48)[0],'?'),
                number=struct.unpack_from('<I',raw,0x4c)[0],cls=struct.unpack_from('<Q',raw,0x50)[0])
try:
    camera_class=int(metadata['instances'][0]['cls'],16)
    fields=[]
    classes=[]
    visited_classes=set()
    while camera_class and camera_class not in visited_classes:
        visited_classes.add(camera_class)
        owner=ident(camera_class)
        raw=p.read(camera_class,0x88)
        if len(raw)!=0x88:break
        classes.append(owner)
        ptr=struct.unpack_from('<Q',raw,0x80)[0]
        seen=set()
        while ptr and ptr not in seen and len(seen)<4096:
            seen.add(ptr)
            field=ident(ptr)
            if not field:break
            raw=p.read(ptr,0x120)
            if len(raw)!=0x120:break
            field['class_name']=ident(field.pop('cls'))
            field['owner']=owner['name']
            field['offset_internal_candidate']=hex(struct.unpack_from('<I',raw,0x94)[0])
            field['native_pointer_candidates']=[]
            if field['class_name'] and field['class_name']['name']=='Function':
                for offset in range(0xa0,0x119,4):
                    value=struct.unpack_from('<Q',raw,offset)[0]
                    if 0x140000000<=value<0x142036000:
                        field['native_pointer_candidates'].append(dict(field_offset=hex(offset),address=hex(value),rva=hex(value-0x140000000)))
            fields.append(field)
            ptr=struct.unpack_from('<Q',raw,0x68)[0]
        camera_class=struct.unpack_from('<Q',p.read(camera_class+0x60,8))[0]
    result=dict(pid=metadata['pid'],classes=classes,fields=fields)
    pathlib.Path(sys.argv[2]).write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('Classes:',[(v['name'],v['address']) for v in classes])
    print('Reflected fields:',len(fields))
    for f in fields:
        if f['name'] in ('PlayerUpdateCameraNative','GetCameraViewPoint','FillCameraCache','UpdateCamera','PlayerCamera','PCOwner','CameraCache','ViewTarget','PendingViewTarget','DefaultFOV'):
            print(f['owner'],f['name'],f['address'],f['offset_internal_candidate'],f['native_pointer_candidates'])
finally:
    p.close()
