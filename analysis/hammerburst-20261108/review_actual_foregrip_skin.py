"""Saved real bone observations + source-attested strict hand surface only.
No game access, pose writer, reference substitute for missing observed bones,
or assertion of GPU/anatomical/headset acceptance.
"""
from pathlib import Path
import json,hashlib,struct,sys,re,math
import numpy as np
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[2];A=Path(__file__).resolve().parent;O=R/'analysis/solo-m1-20261028'
OUT=A/'actual-foregrip-skin-58352';OUT.mkdir(exist_ok=True)
source=A/'runtime-58352-text/hammerburst-twohand.json';data=json.loads(source.read_text())
body=data['neutral_handguard_hand_skeleton'];assert body['game']==data['game']
assert [x['name'] for x in body['asset_chain']]==['COG_Marcus_Fenix_CamSkel','COG_MarcusFenix']
decoder=O/'inspect_hand_surface.py';prefix=decoder.read_text().split('header=args.header.read_text()')[0]
saved=sys.argv;sys.argv=[str(decoder)]
try:
    ctx={'__file__':str(decoder)};exec(compile(prefix,str(decoder),'exec'),ctx)
finally:sys.argv=saved
assert ctx['s']['source']['export']['Path']=="SkeletalMesh'COG_MarcusFenix.COG_Marcus_Fenix_CamSkel'"
header=A/'fresh-passive-pg-r1/source/tools/vr-common/hand_finger_pose.h';text=header.read_text()
bends=np.array(list(map(float,re.findall(r'-?\d+(?:\.\d+)?',re.search(r'constexpr double support\[4\]\[3\]=\{(.*?)\};',text)[1])))).reshape(4,3)
closed=ctx['fk']((bends,ctx['thumb_profile'](text,'support')));reference=ctx['bind']
def angle(a,b):
    def rotation(m):u,_,v=np.linalg.svd(m[:3,:3]);return u@v
    return math.degrees(math.acos(float(np.clip((np.trace(rotation(a).T@rotation(b))-1)/2,-1,1))))
def bone_matrix(item):
    m=np.eye(4);m[:3,:3]=np.asarray(item['model_axes']).reshape(3,4)[:,:3].T;m[:3,3]=item['model_xyz'];return m
modelroot=Path('J:/gearsVr/analysis/weapon-models-20261021')
manifest=json.loads((modelroot/'export-manifest.json').read_text());hb=next(x for x in manifest if x['name']=='Hammerburst')
package=json.loads((modelroot/'mesh-source-manifest.json').read_text())['source_package']
with open(package,'rb') as stream:stream.seek(hb['offset']);raw=stream.read(hb['size'])
assert hashlib.sha256(raw).hexdigest()==hb['sha256']
obj=modelroot/'Hammerburst.obj';gun=[];gun_tri=[]
for line in obj.read_text().splitlines():
    if line.startswith('v '):gun.append(list(map(float,line.split()[1:4])))
    elif line.startswith('f '):gun_tri.append([int(x.split('/')[0])-1 for x in line.split()[1:4]])
gun=np.asarray(gun);gun_tri=np.asarray(gun_tri)
decoded=np.array([struct.unpack_from('<3f',raw,hb['vertex_offset']+i*64) for i in range(hb['vertex_count'])])
assert gun.shape==decoded.shape and np.abs(gun-decoded).max()<.00001
assert len(gun_tri)==hb['triangles'] and gun_tri.min()==0 and gun_tri.max()==hb['max_index']
vertices=ctx['v'];indices=ctx['indices'];weights=ctx['weights'];tri=ctx['tri'];ids=np.unique(tri)
assert np.all(np.sum(weights[ids]*((indices[ids]>=11)&(indices[ids]<=26)),axis=1)>.999999)
state=data['states'][0];assert state['label']=='before_grip'
gun_world=np.array(state['gun']['mesh_component_to_world']).reshape(4,4).T
component=np.array(body['component']).reshape(4,4).T;to_gun=np.linalg.solve(gun_world,component)

def nearest(points,triangles):
    # Exact point/triangle Euclidean distance: plane projection if inside;
    # otherwise minimum of the three clamped edge distances. Bounded blocks.
    result=[];location=[]
    a,b,c=triangles[:,0],triangles[:,1],triangles[:,2]
    ab=b-a;ac=c-a;n=np.cross(ab,ac);nn=np.einsum('ij,ij->i',n,n)
    valid=nn>1e-20;den=np.where(valid,nn,1.)
    for chunk in np.array_split(np.asarray(points),max(1,math.ceil(len(points)/24))):
        ap=chunk[:,None,:]-a;project=chunk[:,None,:]-n*np.einsum('pij,ij->pi',ap,n)[:,:,None]/den[None,:,None]
        edge_signs=[]
        for u,v in ((a,b),(b,c),(c,a)):
            edge_signs.append(np.einsum('pij,ij->pi',np.cross(v-u,project-u),n))
        inside=valid[None,:]&np.logical_and.reduce([x>=-1e-8 for x in edge_signs])
        distance=np.where(inside,np.sum((project-chunk[:,None,:])**2,axis=2),np.inf)
        closest=project.copy()
        for u,v in ((a,b),(b,c),(c,a)):
            edge=v-u;length=np.einsum('ij,ij->i',edge,edge)
            t=np.clip(np.einsum('pij,ij->pi',chunk[:,None,:]-u,edge)/np.maximum(length,1e-20),0,1)
            q=u+t[:,:,None]*edge;d=np.sum((q-chunk[:,None,:])**2,axis=2);better=d<distance
            distance=np.minimum(distance,d);closest=np.where(better[:,:,None],q,closest)
        which=np.argmin(distance,axis=1);result.extend(np.sqrt(distance[np.arange(len(chunk)),which]).tolist())
        location.extend(closest[np.arange(len(chunk)),which].tolist())
    return np.asarray(result),np.asarray(location)
# Small analytic cases guard this offline distance oracle independently.
test=np.array([[[0.,0,0],[1,0,0],[0,1,0]]]);d,_=nearest([[.2,.2,2],[2,0,0],[-1,-1,0]],test)
assert np.allclose(d,[2,1,math.sqrt(2)],atol=1e-12)
rows=[];skins=[]
for sample,stamp in zip(body['samples'],body['sample_qpc_ns']):
    matrices={int(bone):bone_matrix(value) for bone,value in sample.items()}
    assert set(range(11,27)).issubset(matrices)
    skin=np.zeros((len(vertices),4))
    for bone in range(11,27):
        w=np.sum(weights*(indices==bone),axis=1);used=ids[w[ids]>0]
        if len(used):skin[used]+=(vertices[used]@(matrices[bone]@ctx['inverse'][bone]).T)*w[used,None]
    assert np.max(np.abs(skin[ids,3]-1))<1e-12
    xyz=(skin@to_gun.T)[:,:3];skins.append(xyz)
    errors={}
    for label,expected in (('support',closed),('reference_open',reference)):
        values=[]
        for bone in range(12,27):
            parent=ctx['ref'][bone]['parent'];actual_local=np.linalg.solve(matrices[parent],matrices[bone])
            wanted=np.linalg.solve(expected[parent],expected[bone]);values.append(angle(actual_local,wanted))
        errors[label]=dict(max_degrees=max(values),mean_degrees=float(np.mean(values)),local15_degrees=values)
    rows.append(dict(sample_ns=stamp,local_pose_errors=errors,wrist_in_gun=(to_gun@matrices[11]).tolist()))
last=skins[-1];distances,nearest_points=nearest(last[ids],gun[gun_tri])
groups={}
for name,bones in [('palm',[11]),('index',[12,13,14]),('middle',[15,16,17]),('ring',[18,19,20]),('little',[21,22,23]),('thumb',[24,25,26])]:
    group=np.sum(weights[ids]*np.isin(indices[ids],bones),axis=1)>.999
    values=distances[group];chosen=ids[group]
    assert len(values)
    best=int(np.argmin(values));groups[name]=dict(vertices=len(values),min_distance_raw=float(values.min()),median_distance_raw=float(np.median(values)),max_distance_raw=float(values.max()),within1mm=int((values<=.1).sum()),nearest_vertex=int(chosen[best]),nearest_vertex_in_gun=last[chosen[best]].tolist(),nearest_gun_surface=nearest_points[group][best].tolist())

# Static source-data views, explicitly separate from runtime pixel evidence.
font=lambda n:ImageFont.truetype('C:/Windows/Fonts/arial.ttf',n)
im=Image.new('RGB',(1800,1240),'#111b25');draw=ImageDraw.Draw(im)
draw.text((24,15),'Saved actual HB foregrip / original strict hand skin',font=font(28),fill='white')
draw.text((24,55),'Game58352, observed finger matrices. Offline geometry views; mixed wrist/forearm skin excluded.',font=font(20),fill='#adc8d9')
center=np.mean(last[ids],axis=0)
views=[('outer',[1,-.4,.25]),('inner',[-1,.2,.3]),('front',[.1,1,.15]),('rear',[.1,-1,.15]),('above',[.05,.2,1]),('below',[.05,.2,-1])]
for index,(label,view) in enumerate(views):
    view=np.asarray(view,dtype=float);view/=np.linalg.norm(view);right=np.cross(view,[0,0,1]);right/=np.linalg.norm(right);up=np.cross(right,view)
    panel=Image.new('RGB',(600,550),'#111b25');paint=ImageDraw.Draw(panel)
    x0=300;y0=270;scale=22.
    local_vertices=np.all((gun>=last[ids].min(axis=0)-6)&(gun<=last[ids].max(axis=0)+6),axis=1)
    local_tri=gun_tri[np.any(local_vertices[gun_tri],axis=1)]
    items=[(gun,t,np.array([75,160,184])) for t in local_tri]+[(last,t,np.array([210,174,134])) for t in tri]
    items.sort(key=lambda item:float(item[0][item[1]].mean(axis=0)@view))
    for points,t,colour in items:
        p=points[t];n=np.cross(p[1]-p[0],p[2]-p[0]);length=np.linalg.norm(n)
        shade=.4+.6*abs(n@view/length) if length>1e-10 else .4
        xy=np.column_stack([x0+(p@right-center@right)*scale,y0-(p@up-center@up)*scale])
        paint.polygon([tuple(v) for v in xy],fill=tuple((colour*shade).astype(int)))
    paint.text((20,12),label+' / fore-end detail',font=font(24),fill='#ffda78')
    im.paste(panel,((index%3)*600,100+(index//3)*550))
im.save(OUT/'six-views.png')
report=dict(game=data['game'],saved_source=str(source),saved_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),runtime_asset_chain=body['asset_chain'],body_source=ctx['s']['source'],body_canonical=ctx['s']['canonical_sha256'],gun_source=hb,gun_obj_sha256=hashlib.sha256(obj.read_bytes()).hexdigest(),gun_obj_vs_original_max_error=float(np.abs(gun-decoded).max()),candidate_finger_header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),observations=rows,groups=groups,strict_hand_vertices=len(ids),strict_hand_triangles=len(tri),distance_oracle_analytic_cases=3,limits='Uses all four positive influences for strict vertices only; no missing matrices replaced. Excludes mixed wrist/forearm, sleeves, collision and GPU buffer provenance. Gun transform is separately bracketed before-grip observation, not an atomic matched finger/gun frame. Distance is unsigned, not proof of penetration absence or full contact. Asset-name/reference correspondence is not a live full-blob hash.',game_started=False,full_fit_accepted=False,headset_accepted=False)
(OUT/'review.json').write_text(json.dumps(report,indent=2));np.savez_compressed(OUT/'actual-surface.npz',vertices=last,strict_triangles=tri,gun_vertices=gun,gun_triangles=gun_tri)
print(json.dumps(dict(groups=groups,max_actual_vs_support_degrees=max(r['local_pose_errors']['support']['max_degrees'] for r in rows),strict_vertices=len(ids),strict_triangles=len(tri)),indent=2))
