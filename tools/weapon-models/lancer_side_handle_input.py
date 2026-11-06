"""Private original Lancer side-cap input, never a replacement charging rule.

The supported original LOD0 is hash pinned. This extracts only the identified
sixteen ROOT-weighted faces and retains their material, UV, color and winding.
It does not hide the original cap, create a GPU resource or animate anything.
Generated game-derived vertices stay in the owner's private cache.
"""
import hashlib,math,struct

MESH_SHA256='df356ff3dda45e060f87b5ed94c806d96bbfe36b74b21a0f1d2032596f9dcf4d'
LOW=(3.084918499,31.918554306,8.838219643)
HIGH=(4.973071098,33.428112030,10.022800446)

def unit(value):
    size=math.sqrt(sum(x*x for x in value))
    if not math.isfinite(size) or not .75<=size<=1.25:
        raise ValueError('Invalid original side-handle tangent')
    return tuple(x/size for x in value)

def build(raw,skin):
    if hashlib.sha256(raw).hexdigest()!=MESH_SHA256:
        raise ValueError('Unsupported original Lancer mesh')
    start=skin['vertex_offset_in_export'];count=skin['vertex_count']
    first=skin['index_offset_in_export'];size=skin['index_count']
    if count<=0 or count>65535 or size<=0 or size%3 or start<0 or start+count*64>len(raw) or first<0 or first+size*2>len(raw):
        raise ValueError('Invalid audited Lancer LOD0 bounds')
    source_indices=struct.unpack_from('<'+str(size)+'H',raw,first)
    positions=[struct.unpack_from('<3f',raw,start+i*64) for i in range(count)]
    for p in positions:
        if not all(math.isfinite(x) for x in p):raise ValueError('Nonfinite original Lancer vertex')
    selected=[];vertices=set()
    for offset in range(0,size,3):
        face=source_indices[offset:offset+3]
        if any(v>=count for v in face):raise ValueError('Original Lancer index out of bounds')
        if all(all(LOW[a]-.0001<=positions[v][a]<=HIGH[a]+.0001 for a in range(3)) for v in face):
            selected.append((offset,face));vertices.update(face)
    if len(selected)!=16:raise ValueError('Original side-handle face set changed')
    selected_offsets={offset for offset,_ in selected}
    shared=set()
    for offset in range(0,size,3):
        if offset not in selected_offsets:shared.update(vertices.intersection(source_indices[offset:offset+3]))
    if shared:
        # A later per-instance remap must not bend neighbouring rail triangles.
        # Do not silently duplicate a vertex and claim the original is hidden.
        raise ValueError('Side handle shares vertices with the original gun body')
    ordered=sorted(vertices);remap={old:new for new,old in enumerate(ordered)}
    out=bytearray()
    def packed(value):return bytes(max(0,min(255,round((x+1)*127.5))) for x in value)+b'\x80'
    for index in ordered:
        at=start+index*64
        maps=[c['bone_map'] for c in skin['chunks'] if c['first_vertex']<=index<c['last_vertex_exclusive']]
        if len(maps)!=1:raise ValueError('Ambiguous original Lancer chunk')
        active=[(i,w) for i,w in zip(raw[at+56:at+60],raw[at+60:at+64]) if w]
        if len(active)!=1 or active[0][1]!=255 or active[0][0]>=len(maps[0]) or maps[0][active[0][0]]!=0:
            raise ValueError('Side handle is not the verified rigid ROOT surface')
        x=unit(tuple(v/127.5-1 for v in raw[at+12:at+15]))
        n=unit(tuple(v/127.5-1 for v in raw[at+16:at+19]))
        sign=raw[at+19]
        if sign not in (0,255):raise ValueError('Unsupported original side-handle tangent sign')
        y=unit((n[1]*x[2]-n[2]*x[1],n[2]*x[0]-n[0]*x[2],n[0]*x[1]-n[1]*x[0]))
        if sign==0:y=tuple(-v for v in y)
        uv=raw[at+20:at+52]
        if not all(math.isfinite(v) and abs(v)<100000 for v in struct.unpack('<8f',uv)):
            raise ValueError('Invalid original side-handle UV')
        out.extend(struct.pack('<3f',*(p/100 for p in positions[index])))
        out.extend(packed(x)+packed(y)+packed(n)+uv+raw[at+52:at+56])
    indices=struct.pack('<48H',*(remap[v] for _,face in selected for v in face))
    materials=[]
    for offset,_ in selected:
        matches=[s['material_index'] for s in skin['sections']
                 if s['first_index']<=offset and offset+3<=s['first_index']+s['triangles']*3]
        if len(matches)!=1:raise ValueError('Ambiguous original side-handle material section')
        materials.append(matches[0])
    if len(set(materials))!=1 or materials[0]!=0:
        raise ValueError('Original side-handle material changed')
    report=dict(schema=1,private_game_derived=True,distribution_allowed=False,source_mesh_sha256=MESH_SHA256,
        source_vertex_indices=ordered,source_index_offsets=[i for i,_ in selected],original_cap_separate_vertices=True,
        vertices=len(ordered),triangles=16,basis='original_mesh_metres',source_units_per_metre=100,
        original_bone='b_AR_Root',original_cap_hidden=False,live_integration=False,
        sections=[dict(material_index=materials[0],first_index=0,triangles=16,min_vertex=0,max_vertex=len(ordered)-1)],
        vertex_sha256=hashlib.sha256(out).hexdigest(),index_sha256=hashlib.sha256(indices).hexdigest(),
        material_source='exact original LOD0 section at each source index offset; no black/default replacement material')
    return bytes(out),indices,report


def main():
    import argparse,json,stat
    from pathlib import Path
    import audit_skeletal_bones as audit
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source-manifest',required=True,type=Path)
    p.add_argument('--geometry-manifest',required=True,type=Path)
    p.add_argument('--names',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    args=p.parse_args()
    source=json.loads(args.source_manifest.read_text(encoding='utf-8'))
    asset=next(x for x in source['assets'] if x['path']=="SkeletalMesh'COG_AssaultRifle.Mesh.COGAssaultRifle'")
    geometry=next(x for x in json.loads(args.geometry_manifest.read_text(encoding='utf-8')) if x['name']=='Lancer')
    with Path(source['source_package']).open('rb') as stream:
        stream.seek(asset['package_offset']);raw=stream.read(asset['serialized_size'])
    if hashlib.sha256(raw).hexdigest()!=asset['sha256_actual'] or geometry['sha256']!=MESH_SHA256:
        raise ValueError('Original Lancer source inventories disagree')
    offset,bones,_,_=audit.skeletal_bones(raw,asset['tag_stream_end_in_export'],audit.name_table(args.names))
    skin=audit.lod0_skin(raw,bones,offset,geometry)
    vertices,indices,report=build(raw,skin)
    target=args.output.absolute()
    for item in (target,*target.parents):
        if item.exists():
            info=item.lstat()
            if stat.S_ISLNK(info.st_mode) or getattr(info,'st_file_attributes',0)&0x400:
                raise ValueError('Reparse private geometry output')
    target.mkdir(parents=True,exist_ok=True)
    for name,data in [('lancer-side-handle.vtx60',vertices),('lancer-side-handle.idx16',indices)]:
        path=target/name
        if path.exists():
            if path.is_symlink() or not path.is_file() or path.read_bytes()!=data:
                raise ValueError('Refuse replacing different existing private geometry: '+str(path))
        else:
            with path.open('xb') as stream:stream.write(data)
    report.update(source_package=source['source_package'],source_sections=skin['sections'])
    (target/'source.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(directory=str(target),vertices=report['vertices'],triangles=report['triangles'],
                         vertex_sha256=report['vertex_sha256'],live_integration=False)))


if __name__=='__main__':main()
