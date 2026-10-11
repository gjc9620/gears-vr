from runtime_probe import Process
import json
import pathlib
import struct
import sys

metadata_path=pathlib.Path(sys.argv[1])
metadata=json.loads(metadata_path.read_text())
fields=json.loads(metadata_path.with_name('camera-fields.json').read_text())
names={int(k):v for k,v in json.loads(metadata_path.with_name('runtime-names.json').read_text()).items()}
p=Process(metadata['pid'])
def ident(addr):
    b=p.read(addr,0x60)
    if len(b)!=0x60:return None
    idx,num=struct.unpack_from('<II',b,0x48)
    return dict(address=hex(addr),name=names.get(idx,'?'),number=num,
                cls=hex(struct.unpack_from('<Q',b,0x50)[0]))
try:
    cam=int(metadata['instances'][0]['address'],16)
    owner_field=next(f for f in fields['fields'] if f['owner']=='Camera' and f['name']=='PCOwner')
    offset=int(owner_field['offset_internal_candidate'],16)
    pc=struct.unpack('<Q',p.read(cam+offset,8))[0]
    pc_info=ident(pc)
    cls=int(pc_info['cls'],16)
    seen_classes=set()
    observations=[]
    while cls and cls not in seen_classes:
        seen_classes.add(cls)
        owner=ident(cls)
        raw=p.read(cls,0x88)
        field=struct.unpack_from('<Q',raw,0x80)[0]
        seen=set()
        while field and field not in seen:
            seen.add(field)
            info=ident(field)
            raw=p.read(field,0x120)
            if len(raw)!=0x120:break
            if info['name'] in ('PlayerCamera','Player','Pawn'):
                off=struct.unpack_from('<I',raw,0x94)[0]
                ptr=struct.unpack('<Q',p.read(pc+off,8))[0]
                observations.append(dict(owner=owner['name'],field=info['name'],offset=hex(off),value=hex(ptr),identity=ident(ptr)))
            field=struct.unpack_from('<Q',raw,0x68)[0]
        cls=struct.unpack('<Q',p.read(cls+0x60,8))[0]
    player_camera=next(v for v in observations if v['field']=='PlayerCamera')
    assert int(player_camera['value'],16)==cam,'Camera / PCOwner relationship mismatch'
    player=next(v for v in observations if v['field']=='Player')
    assert player['identity'] is not None
    player_class=ident(int(player['identity']['cls'],16))
    result=dict(pid=metadata['pid'],camera=ident(cam),pc_owner=pc_info,
                controller_fields=observations,player_class=player_class,
                reciprocal_camera_link=True,local_player_class_matches=player_class['name']=='WarLocalPlayer')
    pathlib.Path(sys.argv[2]).write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result,indent=2))
finally:
    p.close()
