"""Make an analysis-only cut of a verified skinned LOD0 mesh by one bone.

Vertices blended between the chosen bone and the frame are cut at 50% bone
weight. Both sides share the cut line and preserve the original surface area.
The resulting OBJ files lack UVs, materials, closed seams and animation; they
must never be substituted for the purchased game's mesh.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from audit_skeletal_bones import audit, name_table


def area(triangle):
    a, b, c = triangle
    u = tuple(b[i] - a[i] for i in range(3))
    v = tuple(c[i] - a[i] for i in range(3))
    cross = (u[1] * v[2] - u[2] * v[1],
             u[2] * v[0] - u[0] * v[2],
             u[0] * v[1] - u[1] * v[0])
    return math.sqrt(sum(value * value for value in cross)) * .5


def clip(face, target):
    """Clip a (position, target weight) triangle at the half-weight seam."""
    def inside(vertex):
        return vertex[1] >= .5 if target else vertex[1] <= .5

    def intersection(a, b):
        fraction = (.5 - a[1]) / (b[1] - a[1])
        return (tuple(a[0][i] + fraction * (b[0][i] - a[0][i])
                      for i in range(3)), .5)

    polygon = []
    for index, current in enumerate(face):
        previous = face[index - 1]
        was_inside, is_inside = inside(previous), inside(current)
        if was_inside != is_inside and previous[1] != current[1]:
            polygon.append(intersection(previous, current))
        if is_inside:
            polygon.append(current)
    if len(polygon) < 3:
        return []
    triangles = []
    for index in range(1, len(polygon) - 1):
        triangle = (polygon[0][0], polygon[index][0], polygon[index + 1][0])
        if area(triangle) > 1e-12:
            triangles.append(triangle)
    return triangles


def partition(positions, weights, indices):
    if len(positions) != len(weights) or len(indices) % 3:
        raise ValueError('Invalid LOD0 vertex or index count')
    parts = {'detachable': [], 'frame': []}
    source_area = output_area = 0.0
    seam_faces = 0
    for start in range(0, len(indices), 3):
        face_indices = indices[start:start + 3]
        if any(index >= len(weights) for index in face_indices):
            raise ValueError('LOD0 triangle exceeds vertex buffer')
        face = [(positions[index], weights[index]) for index in face_indices]
        original_area = area(tuple(vertex[0] for vertex in face))
        source_area += original_area
        if min(vertex[1] for vertex in face) < .5 < max(vertex[1] for vertex in face):
            seam_faces += 1
        for name, is_target in (('detachable', True), ('frame', False)):
            triangles = clip(face, is_target)
            parts[name].extend(triangles)
            output_area += sum(area(triangle) for triangle in triangles)
    if source_area <= 0 or abs(output_area - source_area) > source_area * 1e-6:
        raise ValueError('Weighted partition changed LOD0 surface area')
    if not parts['detachable'] or not parts['frame']:
        raise ValueError('Expected both a detachable part and a frame')
    return parts, {'source_triangles': len(indices) // 3,
                   'source_surface_area_raw_squared_units': source_area,
                   'split_surface_area_raw_squared_units': output_area,
                   'seam_triangles': seam_faces,
                   'detachable_triangles': len(parts['detachable']),
                   'frame_triangles': len(parts['frame'])}


def read_lod0(raw, audited, target_bone):
    skin = audited['lod0_skin']
    bones = audited['bone_names']
    if target_bone not in bones:
        raise ValueError('Target bone absent from original skeleton')
    target_index = bones.index(target_bone)
    palette = [item['skeleton_index'] for item in skin['active_bones']]
    if target_index not in palette:
        raise ValueError('Target bone has no active LOD0 skin influence')
    positions, weights = [], []
    for vertex in range(skin['vertex_count']):
        offset = skin['vertex_offset_in_export'] + vertex * 64
        position = struct.unpack_from('<3f', raw, offset)
        indices = raw[offset + 56:offset + 60]
        influences = raw[offset + 60:offset + 64]
        if not all(math.isfinite(value) for value in position) or sum(influences) != 255:
            raise ValueError('Invalid LOD0 skinned vertex')
        weight = 0
        for index, influence in zip(indices, influences):
            if influence:
                if index >= len(palette):
                    raise ValueError('LOD0 active bone index outside palette')
                if palette[index] == target_index:
                    weight += influence
        positions.append(position)
        weights.append(weight / 255)
    indices = struct.unpack_from('<' + 'H' * skin['index_count'], raw,
                                 skin['index_offset_in_export'])
    return positions, weights, indices


def write_obj(path, triangles, source_sha256, bone):
    with path.open('w', encoding='ascii', newline='\n') as target:
        target.write('# Analysis-only weighted LOD0 cut; open seam, no UV/material/animation\n')
        target.write(f'# source_sha256 {source_sha256}\n# bone {bone}\n')
        for triangle in triangles:
            for vertex in triangle:
                target.write('v ' + ' '.join(f'{value:.9g}' for value in vertex) + '\n')
        for index in range(len(triangles)):
            start = index * 3 + 1
            target.write(f'f {start} {start + 1} {start + 2}\n')


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
    ignored_root = (Path.cwd() / 'analysis').resolve()
    output_dir = args.output_dir.resolve()
    if not output_dir.is_relative_to(ignored_root):
        parser.error('Derived geometry must stay under this checkout analysis/')
    source = json.loads(args.source_manifest.read_text(encoding='utf-8'))
    geometry = json.loads(args.geometry_manifest.read_text(encoding='utf-8'))
    selected = [item for item in geometry if item['name'] == args.mesh_name]
    if len(selected) != 1:
        parser.error('Select one exact geometry inventory name')
    model = selected[0]
    matches = [item for item in source['assets'] if item['path'] == model['path']]
    if len(matches) != 1:
        parser.error('Source mesh missing or ambiguous')
    package = args.package or Path(source['source_package'])
    report = audit(source, name_table(args.names), package, geometry)
    audited = next(item for item in report['assets'] if item['path'] == model['path'])
    asset = matches[0]
    with package.open('rb') as stream:
        stream.seek(asset['package_offset'])
        raw = stream.read(asset['serialized_size'])
    if hashlib.sha256(raw).hexdigest() != audited['source_sha256']:
        raise ValueError('Source changed between audit and partition')
    parts, counts = partition(*read_lod0(raw, audited, args.target_bone))
    output_dir.mkdir(parents=True, exist_ok=True)
    exported = {}
    for name, triangles in parts.items():
        path = output_dir / f'{name}.obj'
        write_obj(path, triangles, audited['source_sha256'], args.target_bone)
        exported[name] = {'triangles': len(triangles), 'sha256':
                          hashlib.sha256(path.read_bytes()).hexdigest()}
    manifest = {'schema': 1, 'source_path': audited['path'],
                'source_sha256': audited['source_sha256'], 'target_bone': args.target_bone,
                'weight_threshold': .5, 'geometry_only': True, 'closed_seams': False,
                'requires_materials_and_runtime_fit': True,
                'counts': counts, 'parts': exported}
    (output_dir / 'parts.json').write_text(json.dumps(manifest, indent=2) + '\n',
                                           encoding='utf-8')
    print(json.dumps(counts, sort_keys=True))


if __name__ == '__main__':
    main()
