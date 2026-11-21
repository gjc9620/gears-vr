"""Package all twelve retained, hash-attested retail LOD0 models and bone groups.
Uses existing audited offsets, not a new parser or a live game reader.
"""
from pathlib import Path
import hashlib,json,struct
import numpy as np
from prepare_assets import write_glb,rotation,HERE

ROOT=HERE.parents[1]
RECORDS=ROOT/'analysis/weapon-specs-20261028'
RETAINED=Path('J:/gearsVr/analysis/weapon-models-20261021')
TITLES={'Lancer':'Lancer 电锯步枪','Gnasher':'Gnasher 霰弹枪','COG_Pistol':'Snub 手枪',
        'Longshot':'Longshot 狙击枪','Boltock':'Boltok 左轮','Hammerburst':'Hammerburst 锤爆步枪',
        'Frag_Grenade':'破片手雷','Smoke_Grenade':'烟雾手雷','Hammer_of_Dawn':'黎明之锤',
        'Troika_turret':'Troika 固定机枪','Boomshot':'Boomshot 榴弹枪','Torque_Bow':'扭力弓'}
def label(bone):
    b=bone.lower()
    for key,text in [('maglatch','弹匣锁扣'),('magazine','弹匣'),('_mag','弹匣'),('lever','杠杆'),
                     ('slide','套筒'),('bolt','枪栓'),('lenscap','镜盖'),('scope','瞄准镜'),
                     ('handle_l','左握把'),('handle_r','右握把'),('handleroot','握把支架'),
                     ('latitude','俯仰机构'),('longitude','转向机构'),('spinner_l','左旋转枪管'),
                     ('spinner_r','右旋转枪管'),('barrel','枪管'),('trigger','扳机'),
                     ('port_r','右抛壳口'),('port_l','左抛壳口'),('muzzle_r','右枪口'),
                     ('muzzle_l','左枪口'),('chain','链节'),('counter','配重'),('_bow','弓臂'),
                     ('root','主体'),('playerref','玩家参考点')]:
        if key in b:return text+' · '+bone
    return '原件 · '+bone

def build():
    exports=json.loads((RETAINED/'export-manifest.json').read_text('utf-8-sig'))
    by_path={x['path']:x['name'] for x in exports};catalog=[]
    for source_kind in ('core','map'):
        audit=json.loads((RECORDS/(source_kind+'-bone-audit.json')).read_text('utf-8-sig'))
        sources=json.loads((RECORDS/(source_kind+'-mesh-source.json')).read_text('utf-8-sig'))
        for a in audit['assets']:
            name=by_path[a['path']].replace(' ','_');source=next(x for x in sources['assets'] if x['path']==a['path'])
            with Path(sources['source_package']).open('rb') as f:f.seek(source['package_offset']);raw=f.read(source['serialized_size'])
            assert hashlib.sha256(raw).hexdigest()==a['source_sha256']==source['sha256_actual']
            skin=a['lod0_skin'];n=skin['vertex_count'];v=np.array([struct.unpack_from('<3f',raw,skin['vertex_offset_in_export']+i*64) for i in range(n)])
            tris=np.array(struct.unpack_from('<'+'H'*skin['index_count'],raw,skin['index_offset_in_export'])).reshape(-1,3)
            span=np.ptp(v,axis=0);forward=0 if span[0]>span[1] else 1
            A=np.array([[0,1,0],[0,0,1],[-1,0,0]]) if forward==0 else np.array([[-1,0,0],[0,0,1],[0,-1,0]])
            p=v@A.T*.01;faces=tris[:,[0,2,1]]
            row=dict(id=name,model=name+'.glb',title=TITLES[name],source=a['path'],source_sha256=a['source_sha256'],
                     forward_axis_raw='+X' if forward==0 else '+Y',raw_to_xr=A.tolist(),units_per_raw=.01,
                     triangles=len(tris),bounds_min=p.min(axis=0).tolist(),bounds_max=p.max(axis=0).tolist(),bones=a['bone_names'],interaction='lancer' if name=='Lancer' else 'pose_authoring')
            if name=='Lancer':catalog.append(row);continue
            write_glb(HERE/'assets'/row['model'],p,faces,row['title'])
            weights=np.zeros((n,len(a['bone_names'])))
            for chunk in skin['chunks']:
                for i in range(chunk['first_vertex'],chunk['last_vertex_exclusive']):
                    at=skin['vertex_offset_in_export']+i*64
                    for b,w in zip(raw[at+56:at+60],raw[at+60:at+64]):
                        if w:weights[i,chunk['bone_map'][b]]+=w
            assert np.all(weights.sum(axis=1)==255)
            # Mixed-skin faces stay with their original skeleton: never rigidly cut them.
            owner=np.argmax(weights[tris].sum(axis=1),axis=1)
            rigid=np.array([len(set(np.flatnonzero(weights[t].sum(axis=0))))==1 for t in tris])
            owner[~rigid]=-1
            groups=[];frames=[];parts=[]
            center=(p.min(axis=0)+p.max(axis=0))*.5
            for pid,title,point in [('main_grip','主握把',center),('support','辅助握持',center+np.array([0,0,-.15]))]:
                parts.append(dict(id=pid,label=title,position=point.tolist(),bone=-1,radius=.12))
            for i,bone in enumerate(a['bone_names']):
                ref=a['reference_bones'][i];m=np.eye(4);m[:3,:3]=rotation(ref['orientation_xyzw']);m[:3,3]=ref['position_parent_local_raw'];parent=ref['parent_index'];frames.append(frames[parent]@m if i else m)
                pivot=A@frames[-1][:3,3]*.01;mask=owner==i;filename=f'{name}.bone{i}.mesh';count=int(mask.sum())
                if count:write_glb(HERE/'assets'/filename,p,faces[mask],bone)
                groups.append(dict(bone=bone,parent=parent,pivot=pivot.tolist(),mesh=filename if count else '',triangles=count))
                point=p[np.unique(faces[mask])].mean(axis=0) if count else pivot
                parts.append(dict(id='bone_'+str(i),label=label(bone),position=point.tolist(),bone=i,radius=.08))
            static_mask=owner==-1;static_file=name+'.skinned-rest.mesh'
            if static_mask.any():write_glb(HERE/'assets'/static_file,p,faces[static_mask],'Original mixed skin at rest')
            assert sum(g['triangles'] for g in groups)+int(static_mask.sum())==len(faces)
            info=dict(schema=1,parts=parts,groups=groups,rest_mesh=static_file if static_mask.any() else '',
                      mixed_skin_triangles=int(static_mask.sum()),mixed_skin_policy='rest-only; no rigid deformation',
                      initial_regions='bounding-derived authoring defaults; not calibrated grips')
            (HERE/'assets'/(name+'.parts.json')).write_text(json.dumps(info,ensure_ascii=False,indent=2),'utf-8')
            row['parts']=len(parts);row['mixed_skin_triangles']=int(static_mask.sum());catalog.append(row)
    assert len(catalog)==12
    catalog.sort(key=lambda x:(x['id']!='Lancer',x['id']))
    (HERE/'assets/weapon-catalog.json').write_text(json.dumps(dict(schema=1,weapons=catalog),ensure_ascii=False,indent=2),'utf-8')
    print(json.dumps([dict(id=x['id'],bones=len(x['bones']),triangles=x['triangles'],parts=x.get('parts',5)) for x in catalog]))
if __name__=='__main__':build()
