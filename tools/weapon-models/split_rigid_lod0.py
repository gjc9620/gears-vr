"""Split a verified, rigid-skinned LOD0 inspection mesh by original bone.

This is an ignored-analysis geometry proof, not a replacement game asset.
Mixed weights or triangles crossing bone groups fail closed. UV, normals,
materials, skeleton and animation are intentionally not exported.
"""
import argparse
from collections import defaultdict
import hashlib
import json
import pathlib
import re
import struct

from audit_skeletal_bones import audit, name_table, vertex_bone_map


def split(raw, geometry, audited):
    skin = audited['lod0_skin']
    vertex_count = skin['vertex_count']
    vertex_start = skin['vertex_offset_in_export']
    bone_by_vertex = []
    for vertex in range(vertex_count):
        offset = vertex_start + vertex * 64
        indices = raw[offset + 56:offset + 60]
        weights = raw[offset + 60:offset + 64]
        bone_map = vertex_bone_map(skin, vertex)
        nonzero = [i for i, weight in enumerate(weights) if weight]
        if len(nonzero) != 1 or weights[nonzero[0]] != 255 or indices[nonzero[0]] >= len(bone_map):
            raise ValueError('Non-rigid or invalid bone influence; manual review required')
        bone_by_vertex.append(bone_map[indices[nonzero[0]]])
    index_count = skin['index_count']
    indices = struct.unpack_from('<' + 'H' * index_count, raw, skin['index_offset_in_export'])
    grouped = defaultdict(list)
    for triangle in range(0, index_count, 3):
        face = indices[triangle:triangle + 3]
        if any(vertex >= vertex_count for vertex in face):
            raise ValueError('Triangle index outside vertex buffer')
        bones = {bone_by_vertex[vertex] for vertex in face}
        if len(bones) != 1:
            raise ValueError('Triangle crosses bone groups; cannot rigidly split')
        grouped[bones.pop()].append(face)
    if sum(map(len, grouped.values())) * 3 != index_count:
        raise ValueError('Triangle count changed while splitting')
    return grouped


def export_parts(raw, geometry, audited, grouped, output_dir):
    written = []
    for bone_index, faces in sorted(grouped.items()):
        bone = audited['bone_names'][bone_index]
        if not re.fullmatch(r'[A-Za-z0-9_]+', bone):
            raise ValueError('Unsafe bone name')
        used = sorted({vertex for face in faces for vertex in face})
        remap = {vertex: index + 1 for index, vertex in enumerate(used)}
        path = output_dir / (bone + '.obj')
        with path.open('w', encoding='ascii', newline='\n') as target:
            target.write('# Original LOD0 rigid-bone geometry inspection only\n')
            target.write('# source_sha256 ' + audited['source_sha256'] + '\n')
            target.write('# bone ' + bone + '\n')
            for vertex in used:
                x, y, z = struct.unpack_from('<3f', raw,
                                             audited['lod0_skin']['vertex_offset_in_export'] + vertex * 64)
                target.write(f'v {x:.9g} {y:.9g} {z:.9g}\n')
            for face in faces:
                target.write('f ' + ' '.join(str(remap[index]) for index in face) + '\n')
        written.append({'bone': bone, 'skeleton_index': bone_index,
                        'vertices': len(used), 'triangles': len(faces),
                        'obj': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    return written


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-manifest', type=pathlib.Path, required=True)
    parser.add_argument('--geometry-manifest', type=pathlib.Path, required=True)
    parser.add_argument('--names', type=pathlib.Path, required=True)
    parser.add_argument('--mesh-name', required=True)
    parser.add_argument('--output-dir', type=pathlib.Path, required=True)
    parser.add_argument('--package', type=pathlib.Path)
    args = parser.parse_args()
    ignored_root = (pathlib.Path.cwd() / 'analysis').resolve()
    output_dir = args.output_dir.resolve()
    if not output_dir.is_relative_to(ignored_root):
        parser.error('Split game-derived geometry must stay under this checkout analysis/')
    source = json.loads(args.source_manifest.read_text(encoding='utf-8'))
    geometry = json.loads(args.geometry_manifest.read_text(encoding='utf-8'))
    selected = [item for item in geometry if item['name'] == args.mesh_name]
    if len(selected) != 1:
        parser.error('Select one exact geometry inventory name')
    model = selected[0]
    assets = [item for item in source['assets'] if item['path'] == model['path']]
    if len(assets) != 1:
        parser.error('Geometry source missing or ambiguous')
    package = args.package or pathlib.Path(source['source_package'])
    report = audit(source, name_table(args.names), package, geometry)
    audited = next(item for item in report['assets'] if item['path'] == model['path'])
    asset = assets[0]
    with package.open('rb') as stream:
        stream.seek(asset['package_offset'])
        raw = stream.read(asset['serialized_size'])
    if hashlib.sha256(raw).hexdigest() != audited['source_sha256']:
        raise ValueError('Source changed between audit and split')
    grouped = split(raw, model, audited)
    output_dir.mkdir(parents=True, exist_ok=True)
    parts = export_parts(raw, model, audited, grouped, output_dir)
    manifest = {'schema': 1, 'source_path': audited['path'],
                'source_sha256': audited['source_sha256'],
                'geometry_only': True, 'parts': parts}
    (output_dir / 'parts.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(' '.join(f"{item['bone']}:{item['triangles']}" for item in parts))


if __name__ == '__main__':
    main()
