"""Bounded offline thumb opposition sweep on the exact observed baseline."""
from pathlib import Path
import json,sys,math
import numpy as np
A=Path(__file__).resolve().parent;review_script=A/'review_actual_foregrip_skin.py'
scope={'__file__':str(review_script)};exec(compile(review_script.read_text().split('rows=[];skins=[]')[0],str(review_script),'exec'),scope)
globals().update({k:v for k,v in scope.items() if k not in ('__file__','__name__')})
OUT=A.parent/'hammerburst-20261115'/'batch2'/'thumb-sweep';OUT.mkdir(parents=True,exist_ok=True)
saved_path=A/'actual-foregrip-skin-58352/actual-surface.npz';saved_review_path=saved_path.parent/'review.json'
input_paths=dict(review_script=review_script,saved_surface=saved_path,saved_review=saved_review_path,runtime_source=Path(source),finger_header=Path(header),gun_obj=Path(obj),export_manifest=Path(modelroot/'export-manifest.json'),mesh_source_manifest=Path(modelroot/'mesh-source-manifest.json'),fit_script=Path(__file__).resolve())
input_hashes_before={name:hashlib.sha256(path.read_bytes()).hexdigest() for name,path in input_paths.items()}
saved_review=json.loads(saved_review_path.read_text())
assert input_hashes_before['runtime_source']==saved_review['saved_source_sha256']
assert input_hashes_before['gun_obj']==saved_review['gun_obj_sha256']
assert input_hashes_before['finger_header']==saved_review['candidate_finger_header_sha256']
sample=body['samples'][-1];observed={int(b):bone_matrix(v) for b,v in sample.items()}
thumb_bones=(24,25,26)
observed_local={bone:np.linalg.solve(observed[ctx['ref'][bone]['parent']],observed[bone]) for bone in thumb_bones}
observed_rotation_orthonormal_error=float(np.max(np.abs(observed[24][:3,:3].T@observed[24][:3,:3]-np.eye(3))))
observed_rotation_determinant=float(np.linalg.det(observed[24][:3,:3]))
observed_rotation_error_bound=2*observed_rotation_orthonormal_error+abs(observed_rotation_determinant-1)
observed_model_translations={bone:np.asarray(sample[str(bone)]['model_xyz'],dtype=float) for bone in thumb_bones}
reference_local_translations={bone:np.asarray(ctx['ref'][bone]['position'],dtype=float) for bone in thumb_bones}
observed_chain_lengths={f'{a}->{b}':float(np.linalg.norm(observed[b][:3,3]-observed[a][:3,3])) for a,b in ((11,24),(24,25),(25,26))}
observed_chain_length_error_bound=4*observed_rotation_orthonormal_error
def qinverse(q):
    q=np.asarray(q,dtype=float);norm2=float(q@q);assert norm2>0
    return np.array([-q[0],-q[1],-q[2],q[3]])/norm2
def authored_thumb_base(opposition):
    q=np.asarray(ctx['ref'][24]['orientation'],dtype=float)
    return ctx['mul'](ctx['turn'](2,-15),ctx['mul'](ctx['turn'](1,-opposition),q))
authored_zero=authored_thumb_base(0)
def thumb_base_delta(opposition):return ctx['mul'](authored_thumb_base(opposition),qinverse(authored_zero))
zero_delta=thumb_base_delta(0)
assert np.max(np.abs(zero_delta-np.array([0.,0.,0.,1.])))<1e-12
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
def skin_surface(matrices):
    skin=np.zeros((len(vertices),4))
    for bone in range(11,27):
        w=np.sum(weights*(indices==bone),axis=1);used=ids[w[ids]>0]
        if len(used):skin[used]+=(vertices[used]@(matrices[bone]@ctx['inverse'][bone]).T)*w[used,None]
    assert np.max(np.abs(skin[ids,3]-1))<1e-12
    return (skin@to_gun.T)[:,:3]
def measure(xyz,opposition,exact_observed,delta,delta_rotation,matrices):
    distance,_=nearest(xyz[thumb_ids],gun[gun_tri]);pad_distance,_=nearest(xyz[pad_ids],gun[gun_tri])
    forward=crossings(xyz,thumb_edges,gun[gun_tri]);reverse=crossings(gun,edges(gun_tri),xyz[thumb_tri])
    self_cross=crossings(xyz,thumb_edges,xyz[other_tri])
    local_translation_error=max(float(np.max(np.abs(np.linalg.solve(matrices[ctx['ref'][bone]['parent']],matrices[bone])[:3,3]-observed_local[bone][:3,3]))) for bone in thumb_bones)
    chain_lengths={f'{a}->{b}':float(np.linalg.norm(matrices[b][:3,3]-matrices[a][:3,3])) for a,b in ((11,24),(24,25),(25,26))}
    chain_length_error=max(abs(chain_lengths[key]-observed_chain_lengths[key]) for key in chain_lengths)
    rotation=matrices[24][:3,:3];orthonormal_error=float(np.max(np.abs(rotation.T@rotation-np.eye(3))));determinant=float(np.linalg.det(rotation))
    delta_orthonormal_error=float(np.max(np.abs(delta_rotation.T@delta_rotation-np.eye(3))));delta_determinant=float(np.linalg.det(delta_rotation))
    candidate_rotation_vs_observed=float(np.max(np.abs(rotation-observed[24][:3,:3])))
    return dict(opposition_degrees=opposition,thumb_min_raw=float(distance.min()),thumb_median_raw=float(np.median(distance)),distal_pad_min_raw=float(pad_distance.min()),distal_pad_median_raw=float(np.median(pad_distance)),thumb_vertices_within1mm=int((distance<=.1).sum()),thumb_gun_forward_crossings=forward,gun_thumb_reverse_crossings=reverse,thumb_other_hand_crossings=self_cross,exact_observed_matrices=exact_observed,relative_thumb_base_delta_quaternion=delta.tolist(),observed_local_translations_kept=local_translation_error<1e-9,thumb_local_translation_max_error_raw=local_translation_error,observed_chain_lengths_kept=chain_length_error<=observed_chain_length_error_bound,observed_chain_length_max_error_raw=chain_length_error,observed_chain_length_error_bound=observed_chain_length_error_bound,chain_lengths_raw=chain_lengths,thumb_base_rotation_orthonormal_max_error=orthonormal_error,thumb_base_rotation_determinant=determinant,thumb_base_rotation_valid=bool(orthonormal_error<=observed_rotation_error_bound and abs(determinant-1)<=observed_rotation_error_bound),delta_rotation_orthonormal_max_error=delta_orthonormal_error,delta_rotation_determinant=delta_determinant,delta_rotation_valid=bool(delta_orthonormal_error<1e-12 and abs(delta_determinant-1)<1e-12),candidate_rotation_vs_observed_max_abs=candidate_rotation_vs_observed,wrist_matrix_unchanged=bool(np.array_equal(matrices[11],observed[11])),non_thumb_matrices_unchanged=bool(all(np.array_equal(matrices[b],observed[b]) for b in range(11,27) if b not in thumb_bones)))
def evaluate(opposition):
    matrices=dict(observed)
    if opposition:
        delta=thumb_base_delta(opposition);delta_rotation=ctx['rot'](delta);local=observed_local[24].copy();local[:3,:3]=delta_rotation@local[:3,:3]
        matrices[24]=matrices[11]@local;matrices[25]=matrices[24]@observed_local[25];matrices[26]=matrices[25]@observed_local[26]
        row=measure(skin_surface(matrices),opposition,False,delta,delta_rotation,matrices)
        assert row['observed_local_translations_kept'] and row['observed_chain_lengths_kept'] and row['wrist_matrix_unchanged'] and row['non_thumb_matrices_unchanged']
        assert row['thumb_base_rotation_valid'] and row['delta_rotation_valid']
    else:
        row=measure(skin_surface(matrices),0,True,np.array([0.,0.,0.,1.]),np.eye(3),matrices)
    return row
baseline_xyz=skin_surface(observed);initial=evaluate(0)
saved=np.load(saved_path)['vertices']
baseline_max_abs=float(np.abs(baseline_xyz[ids]-saved[ids]).max())
assert baseline_max_abs<.00001,'Opposition0 must reproduce the actual observed skin'
rows=[initial];candidates=[]
for opposition in range(2,37,2):
    row=evaluate(opposition);rows.append(row);print(json.dumps(row),flush=True)
    if not any(row[k] for k in ('thumb_gun_forward_crossings','gun_thumb_reverse_crossings','thumb_other_hand_crossings')):
        candidates.append((row['distal_pad_min_raw'],opposition,row))
best=min(candidates,key=lambda item:item[0]) if candidates else None
input_hashes_after={name:hashlib.sha256(path.read_bytes()).hexdigest() for name,path in input_paths.items()}
report=dict(baseline=initial,baseline_reproduction=dict(max_abs_raw=baseline_max_abs,guard_raw=.00001,passed=baseline_max_abs<.00001,exact_bitwise=bool(np.array_equal(baseline_xyz[ids],saved[ids]))),observed_local_matrices={str(bone):observed_local[bone].tolist() for bone in thumb_bones},observed_local_translations={str(bone):observed_local[bone][:3,3].tolist() for bone in thumb_bones},observed_model_translations={str(bone):observed_model_translations[bone].tolist() for bone in thumb_bones},reference_local_translations={str(bone):reference_local_translations[bone].tolist() for bone in thumb_bones},observed_vs_reference_local_translation_max_abs={str(bone):float(np.max(np.abs(observed_local[bone][:3,3]-reference_local_translations[bone]))) for bone in thumb_bones},observed_chain_lengths=observed_chain_lengths,observed_rotation_orthonormal_max_error=observed_rotation_orthonormal_error,observed_rotation_determinant=observed_rotation_determinant,zero_delta_quaternion=zero_delta.tolist(),sweep=rows,selected=best[2] if best else None,selected_candidate_accepted=False,actual_wrist_unchanged=True,other_finger_matrices_unchanged=True,observed_local_translations_kept=True,source_hashes_before=input_hashes_before,source_hashes_after=input_hashes_after,source_hashes_unchanged=input_hashes_before==input_hashes_after,game_started=False,skin_fit_accepted=False,limits='Exact observed matrices at zero. Nonzero rows alter only observed thumb-base local orientation by authored opposition delta; observed local translations, non-thumb matrices, wrist and chain lengths are asserted unchanged. Rotation validity allows the measured source-axis residual rather than demanding a synthetic orthonormalization. Reference values are provenance comparisons only. Unsigned distances and segment crossings on strict observed hand domain only; no solid-volume, full-body/GPU or headset claim. A candidate is not production acceptance.')
(OUT/'sweep.json').write_text(json.dumps(report,indent=2))
renderer=review_script.read_text();renderer=renderer[renderer.index('font=lambda n:'):renderer.index('report=dict(game=')]
def render(surface,title,subtitle,filename):
    text=renderer.replace('Saved actual HB foregrip / original strict hand skin',title).replace('Game58352, observed finger matrices. Offline geometry views; mixed wrist/forearm skin excluded.',subtitle).replace("im.save(OUT/'six-views.png')",f"im.save(OUT/'{filename}')")
    renderer_scope={'last':surface,'OUT':OUT,'Image':Image,'ImageDraw':ImageDraw,'ImageFont':ImageFont,'np':np,'gun':gun,'gun_tri':gun_tri,'tri':tri,'ids':ids}
    exec(compile(text,str(review_script)+'::'+filename,'exec'),renderer_scope)
render(baseline_xyz,'HB exact observed baseline / strict hand skin','Game58352 observed matrices; no authored thumb delta. Offline geometry, not headset acceptance.','baseline-six-views.png')
if best:
    matrices=dict(observed);delta=thumb_base_delta(best[1]);local=observed_local[24].copy();local[:3,:3]=ctx['rot'](delta)@local[:3,:3]
    matrices[24]=matrices[11]@local;matrices[25]=matrices[24]@observed_local[25];matrices[26]=matrices[25]@observed_local[26];candidate_xyz=skin_surface(matrices)
    np.savez_compressed(OUT/'candidate-surface.npz',vertices=candidate_xyz,strict_triangles=tri,gun_vertices=gun,gun_triangles=gun_tri)
    render(candidate_xyz,'HB thumb candidate / exact observed wrist and chain','Authored thumb-base opposition only; observed translations retained. Offline candidate, unaccepted.', 'six-views.png')
print(json.dumps(dict(baseline=initial,baseline_reproduction=report['baseline_reproduction'],selected=best[2] if best else None),indent=2))
