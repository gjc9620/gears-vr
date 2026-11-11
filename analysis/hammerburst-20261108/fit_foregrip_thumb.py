"""Bounded offline thumb opposition sweep, fixed observed wrist/all lengths."""
from pathlib import Path
import json,sys,math
import numpy as np
A=Path(__file__).resolve().parent;source=A/'review_actual_foregrip_skin.py'
scope={'__file__':str(source)};exec(compile(source.read_text().split('rows=[];skins=[]')[0],str(source),'exec'),scope)
globals().update({k:v for k,v in scope.items() if k not in ('__file__','__name__')})
OUT=A/'foregrip-thumb-opposition-sweep';OUT.mkdir(exist_ok=True)
sample=body['samples'][-1];observed={int(b):bone_matrix(v) for b,v in sample.items()}
thumb_weight=np.sum(weights*np.isin(indices,[24,25,26]),axis=1)
thumb_ids=ids[thumb_weight[ids]>.999];pad_ids=ids[np.sum(weights*(indices==26),axis=1)[ids]>.999]
assert len(thumb_ids) and len(pad_ids)
thumb_tri=tri[np.any(thumb_weight[tri]>0,axis=1)]
other_tri=tri[np.all(thumb_weight[tri]==0,axis=1)]
def edges(faces):return np.unique(np.sort(np.concatenate([faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]]]),axis=1),axis=0)
thumb_edges=edges(thumb_tri)
def crossings(points,segments,triangles):
    # Segment/triangle intersection with conservative AABB rejection. Open
    # endpoints excluded; this is a crossing count, not a solid-volume proof.
    low=triangles.min(axis=1);high=triangles.max(axis=1);total=0
    for i,j in segments:
        origin=points[i];end=points[j];direction=end-origin
        selected=np.all(high>=np.minimum(origin,end)-1e-9,axis=1)&np.all(low<=np.maximum(origin,end)+1e-9,axis=1)
        t=triangles[selected]
        if not len(t):continue
        e1=t[:,1]-t[:,0];e2=t[:,2]-t[:,0];h=np.cross(direction,e2);det=np.einsum('ij,ij->i',e1,h)
        valid=np.abs(det)>1e-10;inverse=np.divide(1.,det,out=np.zeros_like(det),where=valid)
        s=origin-t[:,0];u=np.einsum('ij,ij->i',s,h)*inverse;q=np.cross(s,e1)
        v=(q@direction)*inverse;along=np.einsum('ij,ij->i',e2,q)*inverse
        total+=int(np.sum(valid&(u>1e-8)&(v>1e-8)&(u+v<1-1e-8)&(along>1e-7)&(along<1-1e-7)))
    return total
# Analytic transversal and parallel cases for the independent crossing reader.
triangle=np.array([[[0.,0,0],[1,0,0],[0,1,0]]])
assert crossings(np.array([[.2,.2,-1],[.2,.2,1]]),[[0,1]],triangle)==1
assert crossings(np.array([[.2,.2,1],[.5,.2,1]]),[[0,1]],triangle)==0
def evaluate(opposition):
    matrices=dict(observed)
    for bone in (24,25,26):
        item=ctx['ref'][bone];q=np.asarray(item['orientation'])
        if bone==24:q=ctx['mul'](ctx['turn'](2,-15),ctx['mul'](ctx['turn'](1,-opposition),q))
        else:q=ctx['mul'](q,ctx['turn'](2,-(15 if bone==25 else 10)))
        local=np.eye(4);local[:3,:3]=ctx['rot'](q);local[:3,3]=item['position'];matrices[bone]=matrices[item['parent']]@local
    skin=np.zeros((len(vertices),4))
    for bone in range(11,27):
        w=np.sum(weights*(indices==bone),axis=1);used=ids[w[ids]>0]
        if len(used):skin[used]+=(vertices[used]@(matrices[bone]@ctx['inverse'][bone]).T)*w[used,None]
    xyz=(skin@to_gun.T)[:,:3]
    distance,_=nearest(xyz[thumb_ids],gun[gun_tri]);pad_distance,_=nearest(xyz[pad_ids],gun[gun_tri])
    forward=crossings(xyz,thumb_edges,gun[gun_tri]);reverse=crossings(gun,edges(gun_tri),xyz[thumb_tri])
    self_cross=crossings(xyz,thumb_edges,xyz[other_tri])
    return xyz,dict(opposition_degrees=opposition,thumb_min_raw=float(distance.min()),thumb_median_raw=float(np.median(distance)),distal_pad_min_raw=float(pad_distance.min()),distal_pad_median_raw=float(np.median(pad_distance)),thumb_vertices_within1mm=int((distance<=.1).sum()),thumb_gun_forward_crossings=forward,gun_thumb_reverse_crossings=reverse,thumb_other_hand_crossings=self_cross)
baseline,initial=evaluate(0)
saved=np.load(A/'actual-foregrip-skin-58352/actual-surface.npz')['vertices']
assert np.abs(baseline[ids]-saved[ids]).max()<.00001,'Opposition0 must reproduce the actual observed skin'
rows=[initial];candidates=[]
for opposition in range(2,37,2):
    xyz,row=evaluate(opposition);rows.append(row);print(json.dumps(row),flush=True)
    if not any(row[k] for k in ('thumb_gun_forward_crossings','gun_thumb_reverse_crossings','thumb_other_hand_crossings')):
        candidates.append((row['distal_pad_min_raw'],opposition,xyz,row))
best=min(candidates,key=lambda item:item[0]) if candidates else None
report=dict(baseline=initial,sweep=rows,selected=best[3] if best else None,actual_wrist_unchanged=True,other_finger_matrices_unchanged=True,bone_translations_and_lengths_unchanged=True,game_started=False,skin_fit_accepted=False,limits='Unsigned distances and segment crossings on strict observed hand domain only. Mixed wrist/forearm excluded; no solid-volume, full-body/GPU or headset claim. A candidate is not production acceptance.')
(OUT/'sweep.json').write_text(json.dumps(report,indent=2))
if best:
    last=best[2];np.savez_compressed(OUT/'candidate-surface.npz',vertices=last,strict_triangles=tri,gun_vertices=gun,gun_triangles=gun_tri)
    renderer=source.read_text();renderer=renderer[renderer.index('font=lambda n:'):renderer.index('report=dict(game=')]
    renderer=renderer.replace('Saved actual HB foregrip / original strict hand skin','HB thumb candidate / saved wrist and fixed bones').replace('Game58352, observed finger matrices. Offline geometry views; mixed wrist/forearm skin excluded.',f'Authored thumb opposition{best[1]}deg only; other fingers/wrist observed. Offline candidate, unaccepted.')
    exec(compile(renderer,str(source)+'::candidate_renderer','exec'),globals())
print(json.dumps(dict(baseline=initial,selected=best[3] if best else None),indent=2))
