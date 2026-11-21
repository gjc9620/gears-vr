"""Read-only offline capture preview. Uses actual stored wrist/fingers and Marcus skin.
No pose fitting, recentering of hands, or writes to device/project files.
"""
from pathlib import Path
import argparse, json, zlib, base64, hashlib
import numpy as np
from prepare_workshop import decode, ASSETS

def rotation(q):
    x,y,z,w=np.asarray(q)/np.linalg.norm(q)
    return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],
                     [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],
                     [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])

def pose(p):
    m=np.eye(4);m[:3,:3]=rotation(p['rotation_xyzw']);m[:3,3]=p['position_metres'];return m

def transform(p,m):return p@m[:3,:3].T+m[:3,3]

def euler(d):
    m=np.eye(4)
    for a,angle in reversed(list(enumerate(d))):
        q=np.zeros(4);q[3]=np.cos(np.deg2rad(angle)/2);q[a]=np.sin(np.deg2rad(angle)/2)
        r=np.eye(4);r[:3,:3]=rotation(q);m=m@r
    return m

def skin(rig,hand):
    def fk(fingers):
        out=[np.eye(4)]
        for i,r in enumerate(rig['reference'][1:]):
            m=np.eye(4);m[:3,3]=r['position'];m[:3,:3]=rotation(fingers[i]);out.append(out[r['parent']]@m)
        return out
    bind=fk([r['orientation'] for r in rig['reference'][1:]])
    local=hand['local_fingers_xyzw'];corrections=hand.get('finger_correction_degrees',[[0,0,0]]*15)
    world=fk(local);delta=[w@np.linalg.inv(b) for w,b in zip(world,bind)]
    # Corrections are post-multiplied, as in the native renderer.
    if np.any(corrections):
        world=[np.eye(4)]
        for i,r in enumerate(rig['reference'][1:]):
            m=np.eye(4);m[:3,3]=r['position'];m[:3,:3]=rotation(local[i])@euler(corrections[i])[:3,:3];world.append(world[r['parent']]@m)
        delta=[w@np.linalg.inv(b) for w,b in zip(world,bind)]
    p=np.asarray(rig['vertices']);result=np.zeros_like(p)
    bones=np.asarray(rig['bones']);weights=np.asarray(rig['weights'])
    for b in range(16):
        w=(weights*(bones==b)).sum(axis=1);result+=transform(p,delta[b])*w[:,None]
    wrist=pose(hand['wrist_in_object']);wrist[:3,:3]=wrist[:3,:3]@euler(hand.get('wrist_correction_degrees',[0,0,0]))[:3,:3]
    wrist[:3,3]+=wrist[:3,:3]@np.asarray(hand.get('wrist_offset_metres',[0,0,0]))
    return transform(result,wrist),np.asarray(rig['triangles'])

def compact(p,t,limit):
    # Vertex clustering preserves connected surface regions; visual-only reduced mesh.
    original=p;faces=t;step=.0005
    while len(t)>limit:
        key=np.round(original/step).astype(int);_,inverse=np.unique(key,axis=0,return_inverse=True);counts=np.bincount(inverse)
        p=np.stack([np.bincount(inverse,weights=original[:,i])/counts for i in range(3)],axis=1)
        t=inverse[faces];t=t[(t[:,0]!=t[:,1])&(t[:,1]!=t[:,2])&(t[:,0]!=t[:,2])]
        _,unique=np.unique(np.sort(t,axis=1),axis=0,return_index=True);t=t[np.sort(unique)];step*=1.25
    ids,ix=np.unique(t,return_inverse=True)
    return [np.round(p[ids],5).tolist(),ix.reshape(-1,3).tolist()]

def gun_mesh(j):
    model=j['model'];stem=Path(model).stem;vertices=[];triangles=[]
    def append(name,m=np.eye(4)):
        p,t=decode(ASSETS/name);t=t+sum(len(a) for a in vertices);vertices.append(transform(p,m));triangles.append(t)
    if stem=='Lancer':
        append('Lancer.body.mesh');append('Lancer.receiver.mesh')
        charging=np.eye(4);charging[2,3]=j.get('charging_distance_metres',0);append('Lancer.charging.mesh',charging)
        info=json.loads((ASSETS/'Lancer.parts.json').read_text('utf-8'))
        pivot=np.array(info['starter_pivot_metres']);m=euler([0,-np.rad2deg(j.get('starter_angle_radians',0)),0]);m[:3,3]=pivot-m[:3,:3]@pivot+np.array([0,0,-j.get('starter_pull_metres',0)]);append('Lancer.starter.mesh',m)
        append('Lancer.magazine.mesh',pose(j.get('magazine_delta',dict(position_metres=[0,0,0],rotation_xyzw=[0,0,0,1]))))
    else:
        info=json.loads((ASSETS/(stem+'.parts.json')).read_text('utf-8'));world=[];preview=j.get('component_preview',[])
        for i,g in enumerate(info['groups']):
            row=preview[i] if i<len(preview) else {};m=euler(row.get('degrees',[0,0,0]));pivot=np.array(g['pivot']);m[:3,3]=np.array(row.get('offset',[0,0,0]))+pivot-m[:3,:3]@pivot
            m=world[g['parent']]@m if i else m;world.append(m)
            if g['mesh']:append(g.get('replacement_mesh',g['mesh']),m)
        if info['rest_mesh']:append(info['rest_mesh'])
    return np.concatenate(vertices),np.concatenate(triangles)

def export(root,output):
    marcus=json.loads((ASSETS/'marcus.json').read_text('utf-8'));data={'poses':[],'guns':{}}
    for path in sorted((root/'captures').glob('*.json')):
        j=json.loads(path.read_text('utf-8'));c=j['capture'];part=j['parts'][c['part']];side='left' if c['side']==0 else 'right';h=next(x for x in part['hands'] if x['side']==side)
        assert not h.get('authored_default',False) and h['hand_data_source']==1
        # Each archive retains its exact component preview, not the latest project transform.
        p,t=gun_mesh(j);key=path.stem;mesh=compact(p,t,1200);gun_key=hashlib.sha256(json.dumps(mesh,separators=(',',':')).encode()).hexdigest()[:16];data['guns'][gun_key]=mesh
        hp,ht=skin(marcus['hands'][c['side']],h)
        raw=[x['pose']['position_metres'] for x in h['raw_joints_in_object']]
        data['poses'].append(dict(id=key,gun=gun_key,model=j['model'],part=part['id'],name=c['name'],side=side,utc_ms=c['utc_unix_ms'],hand=compact(hp,ht,800),raw=raw,tracked=h['fully_tracked']))
    output.write_text(json.dumps(data,ensure_ascii=False,separators=(',',':')),'utf-8')
    packed=base64.b64encode(zlib.compress(output.read_bytes(),9)).decode();output.with_suffix('.b64').write_text(packed,'ascii')
    print(json.dumps(dict(captures=len(data['poses']),raw_bytes=output.stat().st_size,packed_bytes=len(packed))))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('root',type=Path);parser.add_argument('output',type=Path);a=parser.parse_args();export(a.root,a.output)
