"""Read generated payloads independently of the encoder; no engine execution."""
import hashlib,json,math,struct
from pathlib import Path
from build_engine_inputs import decode

ROOT=Path(__file__).resolve().parents[3]
ASSETS=ROOT/'assets/weapon-6dof/offline-v2'
DEST=ASSETS/'engine-input'
def sha(data):return hashlib.sha256(data).hexdigest()
def unpack_normal(data,offset):return tuple(v/127.5-1 for v in data[offset:offset+3])
def dot(a,b):return sum(x*y for x,y in zip(a,b))

def main():
 manifest=json.loads((DEST/'manifest.json').read_text(encoding='utf-8'))
 assert manifest['record_stride']==60 and not manifest['engine_resource'] and not manifest['runtime_enabled']
 checked=0;triangles=0;max_error=0
 for item in manifest['assets']:
  source=ASSETS/(item['id']+'.glb');assert sha(source.read_bytes())==item['source_glb_sha256']
  doc,meshes=decode(source);original=meshes[item['source_mesh_index']]
  verts=(DEST/item['vertex_file']).read_bytes();raw_indices=(DEST/item['index_file']).read_bytes()
  assert sha(verts)==item['vertex_sha256'] and sha(raw_indices)==item['index_sha256']
  n=item['vertices'];assert len(verts)==60*n and len(raw_indices)==2*n and n==3*item['triangles'] and 2*n<65536
  indices=struct.unpack('<'+'H'*n,raw_indices);assert list(indices)==list(range(n))
  cursor=0
  for sec in item['sections']:
   selected=[t for t in original if t['material']==sec['material_index']]
   assert sec['first_index']==cursor and sec['min_vertex']==cursor and sec['triangles']==len(selected)
   assert 0<=sec['material_index']<len(doc['materials']) and not sec['collision']
   for tri in selected:
    for corner in range(3):
     record=verts[indices[cursor]*60:(indices[cursor]+1)*60]
     pos=struct.unpack_from('<3f',record);uv=struct.unpack_from('<2f',record,24)
     assert pos==tri['p'][corner] and uv==tri['uv'][corner]
     assert all(math.isfinite(v) for v in pos+uv)
     x,y,z=[unpack_normal(record,o) for o in (12,16,20)]
     normal=tri['n'][corner];length=math.sqrt(dot(normal,normal));normal=tuple(v/length for v in normal)
     err=max(abs(a-b) for a,b in zip(z,normal));max_error=max(max_error,err)
     assert err<=1/255+1e-7
     assert all(abs(dot(a,a)-1)<.025 for a in (x,y,z))
     assert all(abs(dot(a,b))<.025 for a,b in ((x,y),(x,z),(y,z)))
     assert record[32:56]==bytes(24) and record[56:]==bytes([255])*4
     cursor+=1;checked+=1
   assert sec['max_vertex']==cursor-1
  assert cursor==n;triangles+=item['triangles']
 assert len(manifest['assets'])==11
 result={'status':'passed','payload_meshes':11,'triangles':triangles,'vertices_compared_to_glb':checked,'max_packed_normal_component_error':max_error,'native_calls':0,'runtime_validation':False}
 print(json.dumps(result))

if __name__=='__main__':main()
