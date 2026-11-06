"""Read exported GLBs independently of the generator; never starts game or browser."""
from pathlib import Path
import collections, hashlib, json, math, struct

ROOT = Path(__file__).resolve().parents[3]
ASSETS = ROOT / 'assets/weapon-6dof/offline-v2'

def sub(a, b): return tuple(x-y for x, y in zip(a, b))
def dot(a, b): return sum(x*y for x, y in zip(a, b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

def read_glb(path):
    raw = path.read_bytes()
    magic, version, total = struct.unpack_from('<4sII', raw)
    assert (magic, version, total) == (b'glTF', 2, len(raw)), path
    chunks = []; offset = 12
    while offset < len(raw):
        size, kind = struct.unpack_from('<I4s', raw, offset)
        assert size % 4 == 0 and offset+8+size <= len(raw)
        chunks.append((kind, raw[offset+8:offset+8+size])); offset += 8+size
    assert offset == total and [c[0] for c in chunks] == [b'JSON', b'BIN\0']
    doc = json.loads(chunks[0][1]); binary = chunks[1][1]
    assert len(doc['buffers']) == 1 and doc['buffers'][0]['byteLength'] <= len(binary)
    assert len(binary)-doc['buffers'][0]['byteLength'] <= 3
    values = []
    for acc in doc['accessors']:
        assert 'sparse' not in acc
        view = doc['bufferViews'][acc['bufferView']]
        assert view.get('buffer', 0) == 0
        fmt, size = {5121:('B',1),5123:('H',2),5125:('I',4),5126:('f',4)}[acc['componentType']]
        width = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[acc['type']]
        stride = view.get('byteStride', size*width)
        base = view.get('byteOffset', 0)+acc.get('byteOffset', 0)
        end = base + (acc['count']-1)*stride + size*width
        assert stride >= size*width and base % size == 0
        assert end <= view.get('byteOffset', 0)+view['byteLength'] <= len(binary)
        data = [struct.unpack_from('<'+fmt*width, binary, base+i*stride) for i in range(acc['count'])]
        assert all(math.isfinite(v) for row in data for v in row)
        for k, fn in [('min', min), ('max', max)]:
            if k in acc:
                assert all(abs(fn(p[i] for p in data)-v) < 1e-6 for i,v in enumerate(acc[k]))
        values.append(data)
    meshes = []
    for mesh in doc['meshes']:
        tris = []
        for primitive in mesh['primitives']:
            assert primitive.get('mode', 4) == 4
            attrs = primitive['attributes']; p = values[attrs['POSITION']]
            n = values[attrs['NORMAL']]
            assert len(n) == len(p) and all(abs(dot(v,v)-1) < .002 for v in n)
            if 'TEXCOORD_0' in attrs: assert len(values[attrs['TEXCOORD_0']]) == len(p)
            indices = [i[0] for i in values[primitive['indices']]] if 'indices' in primitive else list(range(len(p)))
            assert len(indices)%3 == 0 and all(0 <= i < len(p) for i in indices)
            assert 0 <= primitive.get('material', 0) < len(doc['materials'])
            for i in range(0, len(indices), 3):
                ids = indices[i:i+3]; tri = [p[j] for j in ids]
                face = cross(sub(tri[1],tri[0]), sub(tri[2],tri[0]))
                assert dot(face,face) > 1e-26
                assert dot(face,tuple(sum(n[j][k] for j in ids) for k in range(3))) > 0
                tris.append(tri)
        meshes.append(tris)
    return doc, meshes

def topology(tris):
    edges = collections.Counter(); directed = collections.Counter(); volume = 0
    for tri in tris:
        volume += dot(tri[0], cross(tri[1], tri[2]))/6
        points = [tuple(round(v,8) for v in p) for p in tri]
        for a,b in zip(points, points[1:]+points[:1]):
            edges[tuple(sorted((a,b)))] += 1; directed[(a,b)] += 1
    assert all(v==2 for v in edges.values())
    assert all(directed[(a,b)] == directed[(b,a)] for a,b in edges)
    assert volume > 0
    return {'triangles':len(tris),'closed':True,'consistent_winding':True,'positive_volume':True}

def hits_z(tris, x, y):
    # Moller-Trumbore intersections, checking holes in the exported triangle data.
    origin = (x,y,-.2); direction = (0,0,1); hits = []
    for a,b,c in tris:
        e1,e2 = sub(b,a),sub(c,a); h = cross(direction,e2); det = dot(e1,h)
        if abs(det)<1e-12: continue
        inv = 1/det; s = sub(origin,a); u = inv*dot(s,h)
        if u < -1e-8 or u > 1+1e-8: continue
        q = cross(s,e1); v = inv*dot(direction,q)
        if v < -1e-8 or u+v > 1+1e-8: continue
        t = inv*dot(e2,q)
        if t >= 0: hits.append(round(t,7))
    return len(set(hits))

def main():
    manifest = json.loads((ASSETS/'manifest.json').read_text())
    parsed = {}; results = []
    for asset in manifest['assets']:
        path = ASSETS/asset['file']
        assert hashlib.sha256(path.read_bytes()).hexdigest() == asset['sha256']
        assert path.stat().st_size == asset['bytes']
        doc, meshes = read_glb(path); parsed[asset['id']] = (doc,meshes)
        result = {'file':path.name,'sha256_verified':True,'glb_structure_and_accessors':True}
        if 'lods' in asset:
            assert len(doc['scenes']) == len(meshes) == 2 and doc['scene'] == 0
            for i,scene in enumerate(doc['scenes']):
                assert [doc['nodes'][j]['mesh'] for j in scene['nodes'] if 'mesh' in doc['nodes'][j]] == [i]
            result['lods'] = [topology(t) for t in meshes]
            assert [len(t) for t in meshes] == [l['triangles'] for l in asset['lods']]
            assert set(asset['markers']) <= {n['name'] for n in doc['nodes']}
        results.append(result)
    cdoc, cylinders = parsed['boltok-cylinder']; nodes = [n for n in cdoc['nodes'] if n['name'].startswith('chamber_')]
    assert len(nodes) == 6
    rays = 0
    for tris in cylinders:
        assert hits_z(tris,.004,.002) == 2  # Solid central material as a positive control.
        for node in nodes:
            x,y,_ = node['translation']
            for r in [0,.006,.0078]:
                for j in range(8):
                    assert hits_z(tris,x+r*math.cos(j*math.pi/4),y+r*math.sin(j*math.pi/4)) == 0
                    rays += 1
    assembly = json.loads((ASSETS/manifest['assembly']).read_text())
    assert len(assembly['roundInstances']) == 6 and assembly['roundBaseOffsetFromChamber_m'] == [0,0,-.003]
    for node,instance in zip(nodes,assembly['roundInstances']):
        assert instance['chamber'] == node['extras']['index']
        assert all(abs(instance['translation'][i]-node['translation'][i]-assembly['roundBaseOffsetFromChamber_m'][i]) < 1e-9 for i in range(3))
    for tris in parsed['boltok-round'][1]:
        interior = [p for tri in tris for p in tri if p[2]-.003 >= -1e-8]
        assert max(math.hypot(p[0],p[1]) for p in interior) < .0082*math.cos(math.pi/16)
        assert max(p[2]-.003 for p in interior) < .07474
    report = {'scope':'independent exported-file validation only; no game, XR or browser UI',
              'assets':results,'six_open_holes_each_lod':True,'clear_hole_sample_rays':rays,
              'seated_round_body_clearance':True,'rim_remains_outside_loading_face':True,
              'lods_are_separate_scenes':True,'runtime_verified':False,'browser_ui_verified':False}
    output = ROOT/'analysis/weapon-specs-20261028/review-v2/asset-file-validation.json'
    output.parent.mkdir(parents=True,exist_ok=True); output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'assets':len(results),'closed_mesh_lods':8,'clear_hole_sample_rays':rays,'result':'PASS','report':str(output)}))

if __name__ == '__main__': main()
