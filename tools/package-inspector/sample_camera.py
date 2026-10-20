"""Resolve observed runtime property metadata and sample the camera read-only."""
from runtime_probe import Process
import argparse
import csv
import json
import math
import pathlib
import struct
import time

parser=argparse.ArgumentParser()
parser.add_argument('metadata',type=pathlib.Path)
parser.add_argument('output',type=pathlib.Path)
parser.add_argument('--seconds',type=float,default=5)
args=parser.parse_args()
metadata=json.loads(args.metadata.read_text())
p=Process(metadata['pid'])
def prop(name,owner):
    rows=[v for v in metadata['metadata'] if v['name']==name and v['outer_identity'] and v['outer_identity']['name']==owner and v['class_identity'] and v['class_identity']['name'].endswith('Property')]
    assert len(rows)==1,(name,owner,len(rows))
    ptr=int(rows[0]['address'],16)
    raw=p.read(ptr,0xd8)
    assert len(raw)==0xd8
    idx=struct.unpack_from('<I',raw,0x48)[0]
    assert struct.unpack_from('<I',raw,0x4c)[0]==0
    # Offset_Internal identified empirically using Location=0, Rotation=12,
    # FOV=24 in the linked TPOV reflected structure.
    offset=struct.unpack_from('<I',raw,0x94)[0]
    return ptr,offset,struct.unpack_from('<Q',raw,0xd0)[0]
try:
    camera_cache=prop('CameraCache','Camera')
    pov=prop('POV','TCameraCache')
    location=prop('Location','TPOV')
    rotation=prop('Rotation','TPOV')
    fov=prop('FOV','TPOV')
    assert (location[1],rotation[1],fov[1])==(0,12,24)
    assert camera_cache[2]==int(next(v['address'] for v in metadata['metadata'] if v['name']=='TCameraCache'),16)
    assert pov[2]==int(next(v['address'] for v in metadata['metadata'] if v['name']=='TPOV'),16)
    assert len(metadata['instances'])==1
    obj=metadata['instances'][0]
    camera=int(obj['address'],16)
    header=p.read(camera,0x60)
    assert struct.unpack_from('<Q',header,0x50)[0]==int(obj['cls'],16)
    destination=camera+camera_cache[1]+pov[1]
    rows=[]
    start=time.perf_counter()
    while time.perf_counter()-start<args.seconds:
        cache=p.read(camera+camera_cache[1],32)
        if len(cache)!=32: raise RuntimeError('Camera became unreadable')
        timestamp,x,y,z,pitch,yaw,roll,field_of_view=struct.unpack('<4f3if',cache)
        assert all(math.isfinite(v) for v in (timestamp,x,y,z,field_of_view))
        assert 1<field_of_view<179
        rows.append([time.perf_counter()-start,timestamp,x,y,z,pitch,yaw,roll,field_of_view])
        time.sleep(0.01)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with args.output.open('w',newline='') as f:
        writer=csv.writer(f)
        writer.writerow(['elapsed','game_timestamp','x','y','z','pitch_ru','yaw_ru','roll_ru','fov'])
        writer.writerows(rows)
    report=dict(pid=metadata['pid'],camera_object=hex(camera),cache_offset=hex(camera_cache[1]),
                pov_offset=hex(pov[1]),pov_address=hex(destination),samples=len(rows),
                first=rows[0],last=rows[-1],unique_poses=len({tuple(row[2:]) for row in rows}),
                ranges={key:[min(row[i] for row in rows),max(row[i] for row in rows)] for i,key in enumerate(['elapsed','timestamp','x','y','z','pitch','yaw','roll','fov'])})
    args.output.with_suffix('.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
finally:
    p.close()
