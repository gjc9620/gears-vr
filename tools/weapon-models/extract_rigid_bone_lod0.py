"""Export a verified rigid LOD0 bone group and complementary body preview.

This is inspection geometry derived from a purchased game package. Outputs are
restricted to the checkout's ignored analysis/ directory. The complementary
OBJ can still contain originally skinned vertices, so neither OBJ has its
source hierarchy, collision, animation or runtime attachment.
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
    positions, uvs, normals = [], [], []
    for vertex in range(vertices):
        at = first + vertex * 64
        position = struct.unpack_from('<3f', raw, at)
        # This game's verified 64-byte GPU vertex places two packed tangent
        # vectors at 12..19, then UV0 at 20..27. Offset 24 alone is UV0.v;
        # the former 24..31 read wrongly paired UV0.v with UV1.u.
        packed_normal = raw[at + 16:at + 19]
        normal = tuple((value - 127.5) / 127.5 for value in packed_normal)
        normal_length = math.sqrt(sum(value * value for value in normal))
        if not .75 <= normal_length <= 1.25:
            raise ValueError('Invalid packed LOD0 normal')
        normal = tuple(value / normal_length for value in normal)
        uv = struct.unpack_from('<2f', raw, at + 20)
        if not all(math.isfinite(value) and abs(value) < 10000 for value in (*position, *uv)):
            raise ValueError('Invalid original LOD0 position or primary UV')
        positions.append(position)
        uvs.append(uv)
        normals.append(normal)
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
    faces, remainder_faces = [], []
    materials, remainder_materials = Counter(), Counter()
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
            else:
                remainder_faces.append((face, section_number, section['material_index']))
                remainder_materials[section['material_index']] += 1
    if not faces:
        raise ValueError('Target bone has no complete LOD0 triangles')
    if 3 * (len(faces) + len(remainder_faces)) != skin['index_count']:
        raise ValueError('Target and remainder do not cover original LOD0 indices')
    used = sorted({vertex for face, _, _ in faces for vertex in face})
    if len(used) != sum(belonging):
        raise ValueError('Target bone has unreferenced LOD0 vertices')
    remainder_used = sorted({vertex for face, _, _ in remainder_faces for vertex in face})
    if set(used) & set(remainder_used):
        raise ValueError('Target and remainder share a vertex')
    return {'positions': positions, 'uvs': uvs, 'normals': normals, 'used_vertices': used,
            'faces': faces, 'material_face_counts': dict(sorted(materials.items())),
            'remainder_used_vertices': remainder_used, 'remainder_faces': remainder_faces,
            'remainder_material_face_counts': dict(sorted(remainder_materials.items()))}


def write_obj(path, extracted, digest, bone, *, remainder=False):
    used = extracted['remainder_used_vertices'] if remainder else extracted['used_vertices']
    faces = extracted['remainder_faces'] if remainder else extracted['faces']
    remap = {vertex: index + 1 for index, vertex in enumerate(used)}
    with path.open('w', encoding='ascii', newline='\n') as output:
        output.write('# Verified source-derived inspection geometry; not a game asset\n')
        output.write(f'# source_sha256 {digest}\n# bone {bone}\n')
        output.write(f'# part {"remainder" if remainder else "target"}\n')
        for vertex in used:
            output.write('v ' + ' '.join(f'{value:.9g}' for value in extracted['positions'][vertex]) + '\n')
        for vertex in used:
            output.write('vt ' + ' '.join(f'{value:.9g}' for value in extracted['uvs'][vertex]) + '\n')
        for vertex in used:
            output.write('vn ' + ' '.join(f'{value:.9g}' for value in extracted['normals'][vertex]) + '\n')
        active = None
        for face, section, material in faces:
            if active != (section, material):
                output.write(f'g section_{section}_material_{material}\n')
                active = (section, material)
            # In these original arrays the face winding is opposite the
            # stored normal. Reverse it for OBJ's front-face convention.
            output.write('f ' + ' '.join(f'{remap[vertex]}/{remap[vertex]}/{remap[vertex]}'
                                         for vertex in reversed(face)) + '\n')


def raw_bone_reference(audited, bone):
    """Retain the parent-local source pose, without claiming a mesh-space hinge."""
    index = audited['bone_names'].index(bone)
    record = audited['reference_bones'][index]
    parent = record['parent_index']
    if record['bone'] != bone or not 0 <= parent < len(audited['bone_names']):
        raise ValueError('Reference bone hierarchy differs from audited names')
    return {'skeleton_index': index, 'parent_bone': audited['bone_names'][parent],
            'parent_index': parent,
            'position_parent_local_raw': record['position_parent_local_raw'],
            'orientation_xyzw_raw': record['orientation_xyzw'],
            'mesh_space_pivot_verified': False}


def _quaternion_multiply(a, b):
    x, y, z, w = a
    u, v, t, s = b
    return (w*u+x*s+y*t-z*v, w*v-x*t+y*s+z*u,
            w*t+x*v-y*u+z*s, w*s-x*u-y*v-z*t)


def _quaternion_rotate(q, vector):
    inverse = (-q[0], -q[1], -q[2], q[3])
    product = _quaternion_multiply(_quaternion_multiply(q, (*vector, 0.)), inverse)
    return product[:3]


def mesh_space_reference_pivot(audited, bone):
    """Compose audited parent-local reference poses into a static pivot candidate.

    The original game component's live bone matrix and lever axis have not been
    sampled, so the result must not be treated as a calibrated runtime hinge.
    """
    index = audited['bone_names'].index(bone)
    poses = []
    for number, reference in enumerate(audited['reference_bones'][:index + 1]):
        if reference['bone'] != audited['bone_names'][number]:
            raise ValueError('Reference bone ordering differs from audited names')
        position = reference['position_parent_local_raw']
        orientation = reference['orientation_xyzw']
        magnitude = math.sqrt(sum(value*value for value in orientation))
        if not .98 <= magnitude <= 1.02:
            raise ValueError('Invalid reference bone orientation')
        orientation = tuple(value/magnitude for value in orientation)
        if number:
            parent = reference['parent_index']
            if not 0 <= parent < number:
                raise ValueError('Invalid reference bone parent')
            parent_position, parent_orientation = poses[parent]
            translated = _quaternion_rotate(parent_orientation, position)
            position = tuple(parent_position[axis] + translated[axis]
                             for axis in range(3))
            orientation = _quaternion_multiply(parent_orientation, orientation)
        poses.append((position, orientation))
    return list(poses[index][0])


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
    remainder_path = output_dir / (args.target_bone + '-remainder.obj')
    write_obj(remainder_path, extracted, audited['source_sha256'], args.target_bone,
              remainder=True)
    bounds = audited['bounds_offset_in_export']
    count = struct.unpack_from('<i', raw, bounds + 28)[0]
    references = struct.unpack_from('<' + 'i' * count, raw, bounds + 32)
    if any(section['material_index'] >= count for section in audited['lod0_skin']['sections']):
        raise ValueError('LOD0 section material index outside source material slots')
    metadata = {'schema': 3, 'source_path': audited['path'],
                'source_sha256': audited['source_sha256'],
                'bone': args.target_bone, 'lod': 0,
                'original_vertices': len(extracted['used_vertices']),
                'original_triangles': len(extracted['faces']),
                'primary_uv_preserved': True, 'uv0_vertex_offset': 20,
                'normals_preserved': True, 'packed_normal_vertex_offset': 16,
                'obj_face_winding_reversed': True,
                'material_face_counts': extracted['material_face_counts'],
                'remainder_vertices': len(extracted['remainder_used_vertices']),
                'remainder_triangles': len(extracted['remainder_faces']),
                'remainder_material_face_counts':
                    extracted['remainder_material_face_counts'],
                'original_lod0_triangles': audited['lod0_skin']['index_count'] // 3,
                'target_plus_remainder_exactly_cover_lod0': True,
                'remainder_original_skinning_preserved': False,
                'source_material_object_refs': list(references),
                'raw_reference_bone': raw_bone_reference(audited, args.target_bone),
                'mesh_space_reference_pivot_candidate_raw':
                    mesh_space_reference_pivot(audited, args.target_bone),
                'runtime_pivot_and_axis_verified': False,
                'geometry_only': True, 'runtime_ready': False,
                'obj': path.name, 'obj_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                'remainder_obj': remainder_path.name,
                'remainder_obj_sha256': hashlib.sha256(remainder_path.read_bytes()).hexdigest()}
    (output_dir / 'part.json').write_text(json.dumps(metadata, indent=2) + '\n',
                                          encoding='utf-8')
    print(json.dumps({key: metadata[key] for key in
                      ('bone', 'original_vertices', 'original_triangles',
                       'material_face_counts')}, sort_keys=True))


if __name__ == '__main__':
    main()
