"""Offline authored-geometry input for the researched engine bridge. No runtime loader.

60-byte vertex records retain GLB metres/axes. The future adapter must convert
positions, tangent axes and winding once, then use certified engine allocation.
These files are not cooked packages or directly loadable game resources.
"""
from pathlib import Path
import collections,hashlib,json,math,struct
ROOT=Path(__file__).resolve().parents[3]
ASSETS=ROOT/'assets/weapon-6dof/offline-v2'
DEST=ASSETS/'engine-input'
IDS=['gnasher-round','longshot-round','boltok-round','boltok-cylinder','lancer-starter']
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def face(p):
 a=[p[1][i]-p[0][i] for i in range(3)];b=[p[2][i]-p[0][i] for i in range(3)]
 n=(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]);s=math.sqrt(dot(n,n));assert s>1e-12
 return tuple(x/s for x in n)
def decode(path):
 raw=path.read_bytes();magic,version,total=struct.unpack_from('<4sII',raw);assert magic==b'glTF' and version==2 and total==len(raw)
 n,kind=struct.unpack_from('<I4s',raw,12);assert kind==b'JSON';d=json.loads(raw[20:20+n]);size,kind=struct.unpack_from('<I4s',raw,20+n);assert kind==b'BIN\0';binary=raw[28+n:];assert len(binary)==size
 vals=[]
 for a in d['accessors']:
  v=d['bufferViews'][a['bufferView']];fmt,width={5126:('f',4),5125:('I',4),5123:('H',2)}[a['componentType']];k={'SCALAR':1,'VEC2':2,'VEC3':3}[a['type']]
  step=v.get('byteStride',k*width);start=v.get('byteOffset',0)+a.get('byteOffset',0)
  assert start+(a['count']-1)*step+k*width<=v.get('byteOffset',0)+v['byteLength']<=len(binary)
  vals.append([struct.unpack_from('<'+fmt*k,binary,start+i*step) for i in range(a['count'])])
 meshes=[]
 for m in d['meshes']:
  tris=[]
  for p in m['primitives']:
   assert p.get('mode',4)==4
   at=p['attributes'];positions=vals[at['POSITION']];normals=vals[at['NORMAL']];uv=vals[at['TEXCOORD_0']] if 'TEXCOORD_0' in at else [(0.,0.)]*len(positions);ix=[x[0] for x in vals[p['indices']]];assert len(ix)%3==0
   for j in range(0,len(ix),3):
    ids=ix[j:j+3];tris.append({'p':[positions[i] for i in ids],'n':[normals[i] for i in ids],'uv':[uv[i] for i in ids],'material':p.get('material',0)})
  meshes.append(tris)
 # Authored sources have identity mesh nodes; starter has three movable parts.
 for node in d['nodes']:
  if 'mesh' in node:assert not any(k in node for k in ['matrix','translation','rotation','scale'])
 return d,meshes

def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def unit(v):
 length=math.sqrt(dot(v,v));assert length>1e-12
 return tuple(x/length for x in v)
def pack(v):return bytes(max(0,min(255,round((x+1)*127.5))) for x in v)+bytes([128])
def encode(tris):
 vertices=bytearray();sections=[]
 for mat in sorted({t['material'] for t in tris}):
  group=[t for t in tris if t['material']==mat];first=len(vertices)//60
  for t in group:
   e1=tuple(b-a for a,b in zip(t['p'][0],t['p'][1]));e2=tuple(b-a for a,b in zip(t['p'][0],t['p'][2]))
   uv1=tuple(b-a for a,b in zip(t['uv'][0],t['uv'][1]));uv2=tuple(b-a for a,b in zip(t['uv'][0],t['uv'][2]))
   det=uv1[0]*uv2[1]-uv1[1]*uv2[0]
   raw=tuple((e1[j]*uv2[1]-e2[j]*uv1[1])/det for j in range(3)) if abs(det)>1e-12 else e1
   bitan=tuple((e2[j]*uv1[0]-e1[j]*uv2[0])/det for j in range(3)) if abs(det)>1e-12 else cross(face(t['p']),e1)
   for pos,normal,uv in zip(t['p'],t['n'],t['uv']):
    normal=unit(normal);tx=tuple(raw[j]-normal[j]*dot(raw,normal) for j in range(3))
    if dot(tx,tx)<1e-16:
     axis=min([(1,0,0),(0,1,0),(0,0,1)],key=lambda a:abs(dot(a,normal)));tx=cross(axis,normal)
    tx=unit(tx);ty=cross(normal,tx)
    if dot(ty,bitan)<0:ty=tuple(-x for x in ty)
    row=bytearray(60);struct.pack_into('<3f',row,0,*pos);row[12:16]=pack(tx);row[16:20]=pack(ty);row[20:24]=pack(normal)
    struct.pack_into('<2f',row,24,*uv);struct.pack_into('<I',row,56,0xffffffff);vertices.extend(row)
  sections.append({'material_index':mat,'first_index':first,'triangles':len(group),'min_vertex':first,'max_vertex':len(vertices)//60-1,'collision':False})
 n=len(vertices)//60;assert n<32768
 return bytes(vertices),struct.pack('<'+'H'*n,*range(n)),sections

def main():
 DEST.mkdir(exist_ok=True);rows=[]
 for name in IDS:
  src=ASSETS/(name+'.glb');d,meshes=decode(src)
  for mesh_index,tris in enumerate(meshes):
   lod=0 if name=='lancer-starter' else mesh_index
   part=d['nodes'][mesh_index]['name'] if name=='lancer-starter' else None
   suffix=part if part else f'lod{lod}'
   vertices,indices,sections=encode(tris);vf=f'{name}-{suffix}.vtx60';ixf=f'{name}-{suffix}.idx16'
   (DEST/vf).write_bytes(vertices);(DEST/ixf).write_bytes(indices)
   rows.append({'id':name,'lod':lod,'source_mesh_index':mesh_index,'part':part,'uv_policy':'zero UV for constant-color starter; source UV for other assets','vertex_file':vf,'index_file':ixf,'triangles':len(tris),'vertices':len(vertices)//60,'vertex_sha256':hashlib.sha256(vertices).hexdigest(),'index_sha256':hashlib.sha256(indices).hexdigest(),'source_glb_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'sections':sections,'source_materials':d['materials']})
 manifest={'schema':'gearsvr.offline-engine-input.v2','engine_resource':False,'runtime_enabled':False,'source_geometry_copied':False,'units':'GLB local metres; adapter must convert positions, tangent basis and winding exactly once','record_stride':60,'fields':{'position_f32x3':0,'packed_tangent_x':12,'packed_tangent_y':16,'packed_normal':20,'uv_f32x2':[24,32,40,48],'color_u32':56},'packed_normal_decode':'byte / 127.5 - 1; fourth byte placeholder, native initializer computes tangent sign','uv_channels_used':1,'normal_policy':'Preserve per-corner authored normals, quantized to original packed-normal format; do not call RawTriangles Build.','native_initializer_policy':'position and tangent/UV initializers internally create 2*N storage; input count and section indices stay N. Do not duplicate input.','material_policy':'PBR metadata is authoring reference, not native shader parameter names; bind compatible game MaterialInterfaces.','runtime_required':['certify current native bytes, ABI and object ownership','convert local metres/axes once and recompute packed tangent basis after conversion','construct independent transient UStaticMesh and LOD using original engine','initialize native position, tangent/UV and color buffers from temporary 60-byte TArray','fill engine-owned sections, indices, bounds; retain constructor defaults for optional streams','InitResources, then bind owned component; release through native render fence and GC'],'assets':rows}
 (DEST/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 print(json.dumps({'payload_meshes':len(rows),'triangles':sum(r['triangles'] for r in rows),'vertices':sum(r['vertices'] for r in rows),'runtime_verified':False}))
if __name__=='__main__':main()
