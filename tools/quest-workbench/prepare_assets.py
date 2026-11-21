"""Convert the retained, hash-checked original Lancer/Marcus geometry for local authoring."""
from pathlib import Path
import argparse
import hashlib
import json
import math
import re
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def write_glb(path, vertices, triangles, label):
    ids = np.unique(triangles)
    remap = np.full(len(vertices), -1, int)
    remap[ids] = np.arange(len(ids))
    vertices, triangles = vertices[ids], remap[triangles]
    position, indices = vertices.astype('<f4').tobytes(), triangles.astype('<u2').tobytes()
    blob = position+indices
    blob += b'\0'*(-len(blob) % 4)
    gltf = dict(asset=dict(version='2.0', generator='GearsVR attested original geometry converter'),
                scene=0, scenes=[dict(nodes=[0])], nodes=[dict(name=label, mesh=0)],
                meshes=[dict(primitives=[dict(attributes={'POSITION': 0}, indices=1)])],
                buffers=[dict(byteLength=len(blob))],
                bufferViews=[dict(buffer=0, byteOffset=0, byteLength=len(position)),
                             dict(buffer=0, byteOffset=len(position), byteLength=len(indices))],
                accessors=[dict(bufferView=0, componentType=5126, count=len(vertices), type='VEC3',
                                min=vertices.min(0).tolist(), max=vertices.max(0).tolist()),
                           dict(bufferView=1, componentType=5123, count=triangles.size, type='SCALAR')])
    text = json.dumps(gltf, separators=(',', ':')).encode()
    text += b' '*(-len(text) % 4)
    path.write_bytes(struct.pack('<III', 0x46546c67, 2, 28+len(text)+len(blob))+
                     struct.pack('<II', len(text), 0x4e4f534a)+text+
                     struct.pack('<II', len(blob), 0x004e4942)+blob)


def reflect_joint(q):
    x, y, z, w = q
    return [x, -y, -z, w]


def rotation(q):
    x, y, z, w = np.asarray(q)/np.linalg.norm(q)
    return np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                     [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                     [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]])


def original_assets(output):
    manifest = json.loads(Path('J:/gearsVr/analysis/weapon-models-20261021/mesh-source-manifest.json').read_text())
    asset = next(a for a in manifest['assets'] if a['path'] == "SkeletalMesh'COG_AssaultRifle.Mesh.COGAssaultRifle'")
    with Path(manifest['source_package']).open('rb') as f:
        f.seek(asset['package_offset'])
        raw = f.read(asset['serialized_size'])
    assert hashlib.sha256(raw).hexdigest() == asset['sha256_actual']
    # The attested original LOD0 layout already used by the project's mesh/skin readers.
    vertices = np.array([struct.unpack_from('<3f', raw, 832177+i*64) for i in range(12721)])
    triangles = np.array(struct.unpack_from('<40191H', raw, 831)).reshape(-1, 3)
    # Original component +Y=muzzle, +Z=up, +X=LEFT. glTF/OpenXR are right handed.
    # Reflect X as well as exchanging Y/Z; reverse winding after determinant -1.
    vertices = vertices[:, [0, 2, 1]] * np.array([-.01, .01, -.01])
    triangles = triangles[:, [0, 2, 1]]
    write_glb(output/'Lancer.glb', vertices, triangles, 'Complete original Lancer LOD0')
    component_ids = dict(starter=np.arange(2230, 2915), charging=np.arange(4133, 4197))
    # Receiver/Receiver_Spin are original rigid influences, not guessed geometry.
    receiver, magazine = [], []
    for first, last, mapping in ((0, 12547, [8,0,5,6,7,4,2,1]), (12547,12721,[1,2])):
        for i in range(first, last):
            at = 832177+i*64
            influences = [mapping[b] for b,w in zip(raw[at+56:at+60],raw[at+60:at+64]) if w]
            if influences and all(b in (5,6) for b in influences): receiver.append(i)
            if influences and all(b==8 for b in influences): magazine.append(i)
    component_ids['receiver'] = np.array(receiver)
    component_ids['magazine'] = np.array(magazine)
    claimed = np.zeros(len(triangles), bool)
    counts = {}
    for name, ids in component_ids.items():
        membership = np.isin(triangles, ids)
        assert not np.any(np.any(membership,axis=1) != np.all(membership,axis=1)), name+' crosses component boundary'
        mask = np.all(membership, axis=1)
        assert not np.any(claimed & mask)
        claimed |= mask
        write_glb(output/f'Lancer.{name}.mesh', vertices, triangles[mask], name)
        counts[name] = int(mask.sum())
    write_glb(output/'Lancer.body.mesh', vertices, triangles[~claimed], 'Original stationary assembly')
    counts['body'] = int((~claimed).sum())
    assert sum(counts.values()) == len(triangles)
    metadata = json.loads((ROOT/'analysis/solo-m1-20261028/marcus-skin-metadata.json').read_text())
    source = metadata['source']
    with Path(source['package']).open('rb') as f:
        f.seek(int(source['export']['Offset']))
        raw = f.read(int(source['export']['Size']))
    assert hashlib.sha256(raw).hexdigest() == source['export']['SHA256']
    skin, ref = metadata['lod0_skin'], metadata['reference']
    v = np.array([(*struct.unpack_from('<3f', raw, skin['vertex_offset_in_export']+i*64), 1.) for i in range(skin['vertex_count'])])
    bones = np.zeros((len(v), 4), int)
    weights = np.zeros((len(v), 4))
    for chunk in skin['chunks']:
        for i in range(chunk['first_vertex'], chunk['last_vertex_exclusive']):
            at = skin['vertex_offset_in_export']+i*64
            bones[i] = [chunk['bone_map'][j] if w else 0 for j, w in zip(raw[at+56:at+60], raw[at+60:at+64])]
            weights[i] = np.frombuffer(raw[at+60:at+64], dtype=np.uint8)/255
    faces = np.array(struct.unpack_from('<'+'H'*skin['index_count'], raw, skin['index_offset_in_export'])).reshape(-1, 3)
    bind = []
    for i, r in enumerate(ref):
        m = np.eye(4)
        m[:3, :3] = rotation(r['orientation'])
        m[:3, 3] = r['position']
        bind.append(bind[r['parent']]@m if i else m)
    result = dict(schema=1, signature='3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193', hands=[])
    for side, wrist in enumerate((11, 35)):
        allowed = set(range(wrist, wrist+16))
        selected = np.array([sum(w for b, w in zip(bb, ww) if b in allowed) > .999 for bb, ww in zip(bones, weights)])
        tri = faces[np.all(selected[faces], axis=1)]
        ids = np.unique(tri)
        remap = {int(j): i for i, j in enumerate(ids)}
        positions = (np.linalg.inv(bind[wrist])@v[ids].T).T[:, :3]*[-.01,.01,.01]
        locals_ = [dict(parent=-1, position=[0, 0, 0], orientation=[0, 0, 0, 1])]
        for i in range(wrist+1, wrist+16):
            r = ref[i]
            locals_.append(dict(parent=r['parent']-wrist, position=(np.array(r['position'])*[-.01,.01,.01]).tolist(), orientation=reflect_joint(r['orientation'])))
        result['hands'].append(dict(side='right' if side else 'left', reference=locals_,
                                  vertices=positions.tolist(), bones=(np.where(weights[ids] > 0, bones[ids]-wrist, 0)).tolist(),
                                  weights=weights[ids].tolist(), triangles=[[remap[int(j)] for j in t[[0,2,1]]] for t in tri]))
    result['joint_basis'] = 'ue_wrist_reflect_x'
    (output/'marcus.json').write_text(json.dumps(result, separators=(',', ':')), 'utf-8')
    # Physical interaction dimensions come from existing shared headers; these are authored defaults.
    def xr(p): return [-p[0]*.01, p[2]*.01, -p[1]*.01]
    parts = []
    points = [(-4.1310674991, -.7928067235, -1.1633055962), (3.2114882469, 46.8190498352, 7.5803027153),
              (.00796632598, 28.14602802187, -3.6039356221), (4.52, 58.35, 13.25), (4.994400476, 25.0155129328, 10.1799370098)]
    for name, point, radius in zip(('main_grip', 'support', 'magazine', 'charging', 'starter'), points, (.1, .14, .12, .18, .14)):
        parts.append(dict(id=name, shape='capsule' if name=='magazine' else 'sphere',
                          local=dict(position_metres=xr(point), rotation_xyzw=[0, 0, 0, 1]),
                          radius_metres=radius, half_length_metres=.08, snap_distance_metres=.14,
                          snap_angle_radians=85*math.pi/180, release_distance_metres=.18, half_extents_metres=[.06]*3))
    (output/'Lancer.parts.json').write_text(json.dumps(dict(schema=1, parts=parts,
        starter_pivot_metres=xr([4.685962200164795,32.844940185546875,10.670499801635742]),
        charging_travel_metres=.23041412353515625, component_triangles=counts), indent=2), 'utf-8')
    return dict(gun_asset_sha256=asset['sha256_actual'], body_asset_sha256=source['export']['SHA256'],
                gun_vertices=len(vertices), gun_triangles=len(triangles),
                hand_vertices=[len(h['vertices']) for h in result['hands']],
                hand_triangles=[len(h['triangles']) for h in result['hands']],
                model_scale_metres_per_original_unit=.01, texture_policy='untextured authoring clay',
                source_to_xr='(-X,Z,-Y)*0.01; reflected winding', component_triangles=counts)


def atlas(output):
    # Ship an OFL-derived glyph atlas, never Windows proprietary fonts.
    source = '\n'.join(p.read_text('utf-8') for p in HERE.iterdir() if p.suffix in ('.cpp','.h','.inl'))
    labels = ''.join(re.findall(r'"([^"\n]*)"', source))
    for p in output.glob('*.json'):
        if p.name=='weapon-catalog.json' or p.name.endswith('.parts.json'):labels+=p.read_text('utf-8')
    alphabet = ''.join(chr(i) for i in range(32, 127))+labels+'交互工作台左右手模型录制回放暂停保存读取校准展开双手固定拿起主握护木弹匣拉栓电锯姿势区域尺寸吸附距离角度释放形状球胶囊盒中心半径轴关节前后当前未连接追踪推测无效原始游戏重置选择最近五秒逐帧速度秒状态成功失败完成返回编辑平移旋转轨迹双手已校准先校准采集部件导入导出项目模式请更新'
    chars = sorted(set(alphabet)-set('\n\r\t'))
    font = ImageFont.truetype(str(HERE/'third_party/NotoSansSC.ttf'), 48)
    try:
        font.set_variation_by_name('Medium')
    except (ValueError, OSError):
        pass
    tile, width = 64, 2048
    height = 2**math.ceil(math.log2(math.ceil(len(chars)/(width//tile))*tile))
    image = Image.new('L', (width, height))
    draw = ImageDraw.Draw(image)
    glyphs = {}
    for i, char in enumerate(chars):
        x, y = i % (width//tile)*tile, i//(width//tile)*tile
        draw.text((x+2, y+2), char, font=font, fill=255, anchor='lt')
        glyphs[str(ord(char))] = [x/width, y/height, tile/width, tile/height, font.getlength(char)/tile]
    (output/'font.raw').write_bytes(image.tobytes())
    (output/'font.json').write_text(json.dumps(dict(width=width, height=height, glyphs=glyphs)), 'utf-8')
    (output/'font-OFL.txt').write_bytes((HERE/'third_party/NotoSansSC-OFL.txt').read_bytes())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=HERE/'assets')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    evidence = original_assets(args.output)
    atlas(args.output)
    evidence['files'] = {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in args.output.iterdir() if p.is_file() and p.name!='provenance.json'}
    (args.output/'provenance.json').write_text(json.dumps(evidence, indent=2), 'utf-8')
    print(json.dumps(evidence))


if __name__ == '__main__': main()
