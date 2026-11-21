"""Curated interactive parts, authoring grip defaults and standalone mechanics.
Run after prepare_catalog.py; original model files and bone inventories stay intact.
"""
from pathlib import Path
import json,struct,hashlib
import numpy as np
from prepare_assets import HERE,write_glb

ASSETS=HERE/'assets';ROOT=HERE.parents[1]
def decode(path):
    raw=path.read_bytes();size=struct.unpack_from('<I',raw,12)[0];j=json.loads(raw[20:20+size]);start=28+size
    # First mesh is the authored LOD0, not all LOD alternatives superimposed.
    vertices=[];faces=[]
    for prim in j['meshes'][0]['primitives']:
        def arr(index,fmt,width):
            a=j['accessors'][index];b=j['bufferViews'][a['bufferView']];at=start+b.get('byteOffset',0)+a.get('byteOffset',0)
            return np.array([struct.unpack_from(fmt,raw,at+i*width) for i in range(a['count'])])
        p=arr(prim['attributes']['POSITION'],'<3f',12);t=arr(prim['indices'],'<I' if j['accessors'][prim['indices']]['componentType']==5125 else '<H',4 if j['accessors'][prim['indices']]['componentType']==5125 else 2).reshape(-1,3)
        faces.extend(t+len(vertices));vertices.extend(p)
    return np.array(vertices),np.array(faces)

def ammunition():
    for name in ['gnasher-round','longshot-round','boltok-round','boltok-cylinder']:
        path=ROOT/'assets/weapon-6dof/offline-v2'/(name+'.glb');p,t=decode(path)
        if name!='boltok-cylinder':p[:,2]=-p[:,2];p[:,2]-=p[:,2].min();t=t[:,[0,2,1]]
        write_glb(ASSETS/(name+'.mesh'),p,t,name)
    # Existing original Boomshot carrier cache, already metre-encoded vertices and player .75 scale.
    cache=ROOT/'analysis/weapon-part-cache/boomshot-carrier-3f0f49cb-p1'
    meta=json.loads((cache/'source.json').read_text('utf-8'))
    for name,sha in meta['hashes'].items():assert hashlib.sha256((cache/name).read_bytes()).hexdigest()==sha
    raw=(cache/'boomshot-carrier.vtx60').read_bytes();p=np.array([struct.unpack_from('<3f',raw,i*60) for i in range(meta['vertices'])]);t=np.frombuffer((cache/'boomshot-carrier.idx16').read_bytes(),'<u2').reshape(-1,3)
    p=p[:,[0,2,1]]*[-.75,.75,-.75];p-=p.mean(axis=0);write_glb(ASSETS/'boomshot-carrier.mesh',p,t[:,[0,2,1]],'Original Boomshot carrier')
    # Preview arrow dimensions follow retained original tip/shaft metadata. Authoring mesh,
    # not a claim to be the original arrow export (which is not available as cached geometry).
    p=[];t=[]
    for z,r in [(0,0),(.035,.016),(.055,.005),(.34,.005)]:
        for k in range(12):p.append([r*np.cos(k*np.pi/6),r*np.sin(k*np.pi/6),z])
    for row in range(3):
        for k in range(12):a=row*12+k;b=row*12+(k+1)%12;t.extend([[a,b,a+12],[b,b+12,a+12]])
    for axis in range(3):
        a=axis*2*np.pi/3;u=np.array([np.cos(a),np.sin(a),0]);n=len(p)
        p.extend([[0,0,.25],(u*.035+[0,0,.29]).tolist(),(u*.035+[0,0,.34]).tolist(),[0,0,.34]]);t.extend([[n,n+1,n+2],[n,n+2,n+3]])
    write_glb(ASSETS/'bow-arrow-preview.mesh',np.array(p),np.array(t),'Authored arrow preview')

def build():
    ammunition();catalog=json.loads((ASSETS/'weapon-catalog.json').read_text('utf-8'));details=[]
    # Raw points are authored using original rig/contact references. They remain editable.
    specs={
      'Hammerburst':('magazine',78,702,[-3,0,0],[2.91,44.08,3.81],'b_HB_Mag','b_HB_MagLatch','弹匣锁扣',.07),
      'COG_Pistol':('pistol',12,60,[0,0,0],[0,12,1],'b_HG_Magazine','b_HG_Slide','套筒',.065),
      'Gnasher':('lever',8,31,[8.54,1.8,-.7],[57.19,-2.94,5.14],None,'b_Shotgun_Lever','下压杠杆',.16),
      'Longshot':('bolt',1,23,[-3,0,0],[.768,43.251,3.375],None,'b_LS_Bolt','枪栓',.12),
      'Boltock':('revolver',6,24,[0,8.7,0],[0,14,3],None,'Object03','弹巢',.055),
      'Boomshot':('breech',1,11,[0,0,0],[0,27,2],'Boomshot_Magazine','Boomshot_Barrel','装填机构',.10),
      'Torque_Bow':('bow',1,11,[-2.323,0,0],[0,30,5],None,None,'',0),
      'Hammer_of_Dawn':('beam',0,0,[0,0,0],[0,24,4],None,None,'',0),
      'Troika_turret':('turret',150,600,None,None,None,None,'',0),
      'Frag_Grenade':('frag',1,3,None,None,None,None,'',0),
      'Smoke_Grenade':('smoke',1,3,None,None,None,None,'',0)}
    for row in catalog['weapons']:
        if row['id']=='Lancer':continue
        name=row['id'];path=ASSETS/(name+'.parts.json');j=json.loads(path.read_text('utf-8'));j.setdefault('legacy_parts',j['parts']);A=np.array(row['raw_to_xr']);bones=row['bones'];groups=j['groups']
        def raw(p):return (A@np.array(p)*.01).tolist()
        def group(b):return bones.index(b)
        def descendants(b):
            idx=group(b);out=[idx]
            for i,g in enumerate(groups):
                if i!=idx and g['parent'] in out:out.append(i)
            return out
        kind,C,S,main,support,mag,mechanism,title,travel=specs[name]
        if kind=='turret':main=groups[group('b_troika_handle_R')]['pivot'];support=groups[group('b_troika_handle_L')]['pivot']
        elif kind in ('frag','smoke'):main=groups[-1]['pivot'];support=None
        else:main=raw(main);support=raw(support)
        parts=[]
        def part(pid,label,point,bone=-1,role='',side=0,radius=.10):
            parts.append(dict(id=pid,label=label,position=point,bone=bone,role=role,side=side,radius=radius,legacy_id='bone_'+str(bone) if bone>=0 else pid))
        part('main_grip','右握把' if kind=='turret' else '握把' if kind in ('revolver','pistol') else '主握把',main,role='main',side=1,radius=.13)
        if support is not None:
            part('support','左握把' if kind=='turret' else '副手握持',main if kind in ('revolver','pistol') else support,role='support',radius=.13)
        feed=np.array(support or main)+[0,.025,.07];mag_bone=group(mag) if mag else -1
        if mag:
            old=next(p for p in j['legacy_parts'] if p['bone']==mag_bone);feed=np.array(groups[mag_bone]['pivot']);ammo_point=old['position']
        elif kind=='revolver':feed=np.array(groups[group('Object02')]['pivot'])+[0,0,.038];ammo_point=feed.tolist()
        elif kind=='bolt':feed=np.array(groups[group('b_LS_Bolt')]['pivot'])+[0,0,-.045];ammo_point=feed.tolist()
        elif kind=='lever':feed=np.array(raw([24,0,2]));ammo_point=feed.tolist()
        else:ammo_point=feed.tolist()
        if kind not in ('beam','turret','frag','smoke'):part('ammo','装填弹巢' if kind=='revolver' else '子弹',ammo_point,mag_bone,'ammo')
        mech_bones=[];mech_pivot=feed.tolist()
        if mechanism:
            idx=group(mechanism);mech_bones=descendants(mechanism);mech_pivot=groups[idx]['pivot'];pos=next(p['position'] for p in j['legacy_parts'] if p['bone']==idx)
            if kind=='revolver':pos=feed.tolist()
            part('mechanism',title,pos,idx,'mechanism',1 if kind=='lever' else 0,radius=.13)
            if kind=='lever':parts[-1]['editor_visible']=False;parts[-1]['pose_alias']='main_grip'
        mesh={'lever':'gnasher-round.mesh','bolt':'longshot-round.mesh','revolver':'Boltock.reload-cylinder.mesh','bow':'bow-arrow-preview.mesh','breech':'boomshot-carrier.mesh'}.get(kind,'')
        if kind in ('magazine','pistol'):
            v,t=decode(ASSETS/groups[mag_bone]['mesh']);v-=feed;mesh=name+'.ammo.mesh';write_glb(ASSETS/mesh,v,t,'Original weapon magazine')
        if kind=='revolver':
            cv,ct=decode(ASSETS/'boltok-cylinder.mesh');length=np.ptp(cv[:,2]);cv[:,2]=-cv[:,2]+length*.5;cv+=np.array(groups[group('Object02')]['pivot']);write_glb(ASSETS/'Boltock.open-cylinder.mesh',cv,ct[:,[0,2,1]],'Authored six-chamber cylinder');groups[group('Object02')]['replacement_mesh']='Boltock.open-cylinder.mesh';write_glb(ASSETS/'Boltock.reload-cylinder.mesh',cv-feed,ct[:,[0,2,1]],'Whole reload cylinder in feed frame')
        if kind=='pistol':mech_bones=[group(b) for b in ('b_HG_Slide','b_HG_Wing_L','b_HG_Wing_R')]
        cfg=dict(id=name,kind=kind,capacity=C,reserve=S,feed=feed.tolist(),mechanism_pivot=mech_pivot,mechanism_bones=mech_bones,magazine_bone=mag_bone,
                 travel=travel,hinge_axis=[0,0,-1] if kind=='revolver' else [1,0,0],open_axis=[-1,0,0] if kind=='revolver' else [0,0,1],
                 insertion_axis=[0,1,0] if kind in ('pistol','magazine','lever') else [0,0,-1],ammo_mesh=mesh,
                 initial_loaded=1 if C==1 else min(C,max(1,C//2)),fire_interval=.7 if kind=='lever' else .18 if kind=='revolver' else .15 if kind=='pistol' else .1,
                 mechanism_requires_reload=kind in ('revolver','bolt','breech'),hand_pose_provenance='editable authored starting templates; physical fitting pending',
                 fastloader=False,group_round_loading=False,reload_item='cylinder' if kind=='revolver' else 'ammunition',cylinder_bone=group('Object02') if kind=='revolver' else -1,shared_grip=kind in ('revolver','pistol'),hammerburst_no_charge=kind=='magazine')
        full,_=decode(ASSETS/row['model']);front=full[full[:,2]<full[:,2].min()+.012];cfg['muzzle']=front.mean(axis=0).tolist();j['parts']=parts;j['practice']=cfg;j['curated']=True;path.write_text(json.dumps(j,ensure_ascii=False,indent=2),'utf-8');row['parts']=len(parts);row['interaction']='workshop';details.append(dict(weapon=name,parts=[p['label'] for p in parts],kind=kind))
    (ASSETS/'weapon-catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2),'utf-8')
    print(json.dumps(details,ensure_ascii=False))
if __name__=='__main__':build()
