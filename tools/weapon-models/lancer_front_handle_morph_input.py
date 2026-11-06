"""Private immutable morph data for the owner-marked ORIGINAL front handle.

No game launch, shared asset edit, geometry replacement or material change.
The guide-slot rearward stroke is authored VR behavior, not stock animation.
"""
import hashlib,json,pathlib,struct
import audit_skeletal_bones as audit

def build(raw,skin):
    expected='df356ff3dda45e060f87b5ed94c806d96bbfe36b74b21a0f1d2032596f9dcf4d'
    if hashlib.sha256(raw).hexdigest()!=expected:raise ValueError('Original mesh hash mismatch')
    ids=set(range(4133,4197));start=skin['vertex_offset_in_export'];faces=[]
    for offset in range(0,skin['index_count'],3):
        face=struct.unpack_from('<3H',raw,skin['index_offset_in_export']+offset*2)
        if not ids.intersection(face):continue
        if not set(face)<=ids:raise ValueError('Handle vertices share outside triangles')
        materials=[s['material_index'] for s in skin['sections'] if s['first_index']<=offset and offset+3<=s['first_index']+s['triangles']*3]
        if materials!=[0]:raise ValueError('Handle is not entirely original main material')
        faces.append(offset)
    if len(faces)!=54:raise ValueError('Original front handle triangle set changed')
    bounds=[]
    for i in sorted(ids):
        at=start+i*64;bone_map=audit.vertex_bone_map(skin,i)
        influences=[(bone_map[b],w) for b,w in zip(raw[at+56:at+60],raw[at+60:at+64]) if w]
        if influences!=[(0,255)]:raise ValueError('Handle is not original rigid ROOT geometry')
        bounds.append(struct.unpack_from('<3f',raw,at))
    low=[min(v[i] for v in bounds) for i in range(3)];high=[max(v[i] for v in bounds) for i in range(3)]
    if any(abs(a-b)>.0001 for a,b in zip(low,[2.1331593990,57.8399238586,12.7702350616])) or any(abs(a-b)>.0001 for a,b in zip(high,[4.3696632385,58.9116554260,13.7571115494])):raise ValueError('Front bounds changed')
    # Two distinct private MorphTarget objects use identical immutable shapes;
    # half weights add to exact translation and keep both active weights >0.
    rear=struct.unpack_from('<3f',raw,start+10401*64)
    if any(abs(a-b)>1e-6 for a,b in zip(rear,[2.7596969604492188,34.79851150512695,12.67965316772461])):
        raise ValueError('Original guide-slot rear-stop vertex changed')
    travel=struct.unpack('<f',struct.pack('<f',low[1]-rear[1]))[0]
    if abs(travel-23.041412353515625)>1e-9:raise ValueError('Guide-slot usable travel changed')
    data=b''.join(struct.pack('<3fIHH',0.,-travel,0.,0x80808080,i,0) for i in sorted(ids))
    return data,dict(source_mesh_sha256=expected,vertices=64,triangles=54,first_vertex=4133,last_vertex=4196,
        original_bone='b_AR_Root',original_material=0,source_index_offsets=faces,minimum=low,maximum=high,
        displacement_raw=[0,-travel,0],guide_slot_rear_vertex=10401,guide_slot_rear=rear,vr_authored_stroke_not_stock_animation=True,
        changes_other_vertices=False,changes_materials=False,delta_sha256=hashlib.sha256(data).hexdigest())

def main():
    root=pathlib.Path(__file__).resolve().parents[2];cache=pathlib.Path('J:/gearsVr/analysis/weapon-models-20261021')
    source=json.loads((cache/'mesh-source-manifest.json').read_text());asset=next(a for a in source['assets'] if a['path']=="SkeletalMesh'COG_AssaultRifle.Mesh.COGAssaultRifle'")
    geometry=next(a for a in json.loads((cache/'export-manifest.json').read_text()) if a['name']=='Lancer')
    with pathlib.Path(source['source_package']).open('rb') as f:f.seek(asset['package_offset']);raw=f.read(asset['serialized_size'])
    names=audit.name_table(pathlib.Path('J:/gearsVr/analysis/first-person/original-WarfareGame.tsv.names.tsv'))
    offset,bones,_,_=audit.skeletal_bones(raw,asset['tag_stream_end_in_export'],names);skin=audit.lod0_skin(raw,bones,offset,geometry)
    data,report=build(raw,skin);out=root/'analysis/weapon-part-cache/lancer-front-handle-df356ff3-p3';out.mkdir(parents=True,exist_ok=True)
    for name in ('pullA.delta20','pullB.delta20'):(out/name).write_bytes(data)
    (out/'manifest.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(dict(output=str(out),**report)))

if __name__=='__main__':main()
