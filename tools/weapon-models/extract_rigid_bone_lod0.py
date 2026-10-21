"""Export one verified rigid LOD0 bone group with its original primary UVs.

This is inspection geometry derived from a purchased game package. Outputs are
restricted to the checkout's ignored analysis/ directory. The OBJ has no
textures, skinned hierarchy, collision, animation or runtime attachment.
"""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re
import struct

from audit_skeletal_bones import audit, name_table, vertex_bone_map


def extract(raw, audited, bone):
    bones = audited['bone_names']
    if bone not in bones:
        raise ValueError('Target bone absent from verified skeleton')
    skin = audited['lod0_skin']
    target = bones.index(bone)
    vertices = skin['vertex_count']
    first = skin['vertex_offset_in_export']
    belonging = [False] * vertices
    positions, uvs = [], []
    for vertex in range(vertices):
        at = first + vertex * 64
        position = struct.unpack_from('<3f', raw, at)
        uv = struct.unpack_from('<2f', raw, at + 24)
        if not all(math.isfinite(value) and abs(value) < 10000 for value in (*position, *uv)):
            raise ValueError('Invalid original LOD0 position or primary UV')
        positions.append(position)
        uvs.append(uv)
        indices = raw[at + 56:at + 60]
        weights = raw[at + 60:at + 64]
        mapping = vertex_bone_map(skin, vertex)
        target_weight = 0
        for local, weight in zip(indices, weights):
            if weight:
                if local >= len(mapping):
                    raise ValueError('Vertex influence exceeds chunk BoneMap')
                if mapping[local] == target:
                    target_weight += weight
        if target_weight not in (0, 255):
            raise ValueError('Target bone has mixed-weight vertices; preserve skinning')
        belonging[vertex] = target_weight == 255
    indices = struct.unpack_from('<' + 'H' * skin['index_count'], raw,
                                 skin['index_offset_in_export'])
    faces = []
    materials = Counter()
    for section_number, section in enumerate(skin['sections']):
        begin = section['first_index']
        end = begin + section['triangles'] * 3
        for start in range(begin, end, 3):
            face = indices[start:start + 3]
            if any(vertex >= vertices for vertex in face):
                raise ValueError('Section face exceeds vertex buffer')
            members = [belonging[vertex] for vertex in face]
            if any(members):
                if not all(members):
                    raise ValueError('Target bone crosses a triangle; preserve skinning')
                faces.append((face, section_number, section['material_index']))
                materials[section['material_index']] += 1
    if not faces:
        raise ValueError('Target bone has no complete LOD0 triangles')
    used = sorted({vertex for face, _, _ in faces for vertex in face})
    if len(used) != sum(belonging):
        raise ValueError('Target bone has unreferenced LOD0 vertices')
    return {'positions': positions, 'uvs': uvs, 'used_vertices': used,
            'faces': faces, 'material_face_counts': dict(sorted(materials.items()))}


def write_obj(path, extracted, digest, bone):
    used = extracted['used_vertices']
    remap = {vertex: index + 1 for index, vertex in enumerate(used)}
    with path.open('w', encoding='ascii', newline='\n') as output:
        output.write('# Verified source-derived inspection geometry; not a game asset\n')
        output.write(f'# source_sha256 {digest}\n# bone {bone}\n')
        for vertex in used:
            output.write('v ' + ' '.join(f'{value:.9g}' for value in extracted['positions'][vertex]) + '\n')
        for vertex in used:
            output.write('vt ' + ' '.join(f'{value:.9g}' for value in extracted['uvs'][vertex]) + '\n')
        active = None
        for face, section, material in extracted['faces']:
            if active != (section, material):
                output.write(f'g section_{section}_material_{material}\n')
                active = (section, material)
            output.write('f ' + ' '.join(f'{remap[vertex]}/{remap[vertex]}' for vertex in face) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-manifest', required=True, type=Path)
    parser.add_argument('--geometry-manifest', required=True, type=Path)
    parser.add_argument('--names', required=True, type=Path)
    parser.add_argument('--mesh-name', required=True)
    parser.add_argument('--target-bone', required=True)
    parser.add_argument('--output-dir', required=True, type=Path)
    parser.add_argument('--package', type=Path)
    args = parser.parse_args()
    root = (Path.cwd() / 'analysis').resolve()
    output_dir = args.output_dir.resolve()
    if not output_dir.is_relative_to(root):
        parser.error('Game-derived geometry must stay under this checkout analysis/')
    if not re.fullmatch(r'[A-Za-z0-9_]+', args.target_bone):
        parser.error('Unsafe target bone name')
    manifest = json.loads(args.source_manifest.read_text(encoding='utf-8'))
    geometry = json.loads(args.geometry_manifest.read_text(encoding='utf-8'))
    selected = [item for item in geometry if item['name'] == args.mesh_name]
    if len(selected) != 1:
        parser.error('Select one exact geometry inventory name')
    model = selected[0]
    sources = [asset for asset in manifest['assets'] if asset['path'] == model['path']]
    if len(sources) != 1:
        parser.error('Verified source mesh missing or ambiguous')
    package = args.package or Path(manifest['source_package'])
    report = audit(manifest, name_table(args.names), package, geometry)
    audited = next(asset for asset in report['assets'] if asset['path'] == model['path'])
    source = sources[0]
    with package.open('rb') as stream:
        stream.seek(source['package_offset'])
        raw = stream.read(source['serialized_size'])
    if hashlib.sha256(raw).hexdigest() != audited['source_sha256']:
        raise ValueError('Source changed after audit')
    extracted = extract(raw, audited, args.target_bone)
    output_dir.mkdir(parents=True, exist_ok=True)
    path = output_dir / (args.target_bone + '.obj')
    write_obj(path, extracted, audited['source_sha256'], args.target_bone)
    bounds = audited['bounds_offset_in_export']
    count = struct.unpack_from('<i', raw, bounds + 28)[0]
    references = struct.unpack_from('<' + 'i' * count, raw, bounds + 32)
    if any(section['material_index'] >= count for section in audited['lod0_skin']['sections']):
        raise ValueError('LOD0 section material index outside source material slots')
    metadata = {'schema': 1, 'source_path': audited['path'],
                'source_sha256': audited['source_sha256'],
                'bone': args.target_bone, 'lod': 0,
                'original_vertices': len(extracted['used_vertices']),
                'original_triangles': len(extracted['faces']),
                'primary_uv_preserved': True, 'normals_preserved': False,
                'material_face_counts': extracted['material_face_counts'],
                'source_material_object_refs': list(references),
                'geometry_only': True, 'runtime_ready': False,
                'obj': path.name, 'obj_sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    (output_dir / 'part.json').write_text(json.dumps(metadata, indent=2) + '\n',
                                          encoding='utf-8')
    print(json.dumps({key: metadata[key] for key in
                      ('bone', 'original_vertices', 'original_triangles',
                       'material_face_counts')}, sort_keys=True))


if __name__ == '__main__':
    main()
