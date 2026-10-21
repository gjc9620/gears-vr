"""Audit original weapon mesh bone names without exporting game geometry.

The inventory and name table are local, ignored analysis artifacts. This tool
only reads the verified package bytes and writes a small metadata report under
the caller's ignored analysis directory.
"""
import argparse
from collections import Counter
import csv
import hashlib
import json
import math
import pathlib
import struct


def name_table(path):
    with path.open(encoding='utf-8-sig', newline='') as source:
        return {int(row['Index']): row['Name'] for row in csv.DictReader(source, delimiter='\t')}


def skeletal_bones(raw, end, names):
    candidates = []
    for offset in range(max(end - 8, 0), min(end + 500, len(raw) - 36)):
        bounds = struct.unpack_from('<7f', raw, offset)
        if (not all(math.isfinite(value) for value in bounds) or
                max(abs(value) for value in bounds) >= 1000 or
                min(bounds[3:6]) < 0 or not 1 < bounds[6] < 500):
            continue
        materials = struct.unpack_from('<i', raw, offset + 28)[0]
        if not 0 <= materials < 20:
            continue
        start = offset + 32 + materials * 4 + 24
        if start + 4 > len(raw):
            continue
        count = struct.unpack_from('<i', raw, start)[0]
        end_bones = start + 4 + count * 48
        if not 0 < count <= 100 or end_bones + 8 > len(raw):
            continue
        indices = [struct.unpack_from('<i', raw, start + 4 + i * 48)[0]
                   for i in range(count)]
        bones = [names.get(index) for index in indices]
        depth, lods = struct.unpack_from('<ii', raw, end_bones)
        if all(bones) and 0 < depth < 30 and 0 < lods < 10:
            candidates.append((offset, bones, depth, lods))
    if len(candidates) != 1:
        raise ValueError(f'Expected one skeletal hierarchy; found {len(candidates)}')
    return candidates[0]


def reference_bones(raw, bounds_offset, bones):
    """Read the inferred UE3 48-byte reference skeleton records.

    Positions are raw parent-local package units, not calibrated VR metres.
    This does not identify a hinge axis or prove an animation's travel.
    """
    materials = struct.unpack_from('<i', raw, bounds_offset + 28)[0]
    start = bounds_offset + 32 + materials * 4 + 24 + 4
    result = []
    for index, bone in enumerate(bones):
        name_index, name_number, flags, *pose = struct.unpack_from('<iii4f3fii', raw,
                                                                     start + index * 48)
        orientation, position, children, parent = pose[:4], pose[4:7], pose[7], pose[8]
        length_sq = sum(value * value for value in orientation)
        if (name_number != 0 or not 0 <= flags < 1024 or
                not all(math.isfinite(value) and abs(value) < 1000
                        for value in (*orientation, *position)) or
                not .98 <= length_sq <= 1.02 or
                not 0 <= children < len(bones) or
                (index == 0 and parent != 0) or
                (index > 0 and not 0 <= parent < index)):
            raise ValueError(f'Invalid reference skeleton transform: {bone}')
        result.append({'bone': bone, 'parent_index': parent, 'child_count': children,
                       'orientation_xyzw': orientation, 'position_parent_local_raw': position,
                       'flags': flags, 'name_index': name_index})
    return result


def lod0_vertex_offset(raw, geometry):
    """Find the unique validated vertex-buffer header when inventory omitted it."""
    if 'vertex_offset' in geometry:
        return geometry['vertex_offset']
    if geometry['decoded_stride'] != 64 or geometry['reported_stride'] not in (64, 68):
        raise ValueError('Unsupported LOD0 vertex layout')
    vertices = geometry['vertex_count']
    header = struct.pack('<ii', vertices, geometry['reported_stride'])
    matches = []
    cursor = 0
    while (cursor := raw.find(header, cursor)) >= 0:
        start = cursor + len(header)
        cursor += 1
        if start + vertices * 64 > len(raw):
            continue
        sample_indices = {0, vertices // 4, vertices // 2, vertices * 3 // 4, vertices - 1}
        if all(all(math.isfinite(value) and abs(value) < 1000
                   for value in struct.unpack_from('<3f', raw, start + index * 64)) and
               sum(raw[start + index * 64 + 60:start + index * 64 + 64]) == 255
               for index in sample_indices):
            matches.append(start)
    if len(matches) != 1:
        raise ValueError(f'Expected one LOD0 vertex buffer; found {len(matches)}')
    return matches[0]


def lod0_skin(raw, bones, bounds_offset, geometry):
    """Decode the inspected 64-byte LOD0 vertex format and active bone map."""
    if geometry['decoded_stride'] != 64:
        raise ValueError('Unsupported skin vertex stride')
    materials = struct.unpack_from('<i', raw, bounds_offset + 28)[0]
    cursor = bounds_offset + 32 + materials * 4 + 24 + 4 + len(bones) * 48 + 8
    sections = struct.unpack_from('<i', raw, cursor)[0]
    if not 0 < sections <= 32:
        raise ValueError('Invalid LOD0 section count')
    cursor += 4
    triangles = 0
    for _ in range(sections):
        _, _, _, count = struct.unpack_from('<HHIH', raw, cursor)
        triangles += count
        cursor += 10
    element_size, index_count = struct.unpack_from('<ii', raw, cursor)
    if element_size != 2 or index_count != geometry['index_count'] or triangles * 3 != index_count:
        raise ValueError('LOD0 topology differs from geometry inventory')
    index_offset = cursor + 8
    cursor += 8 + index_count * element_size
    used_count = struct.unpack_from('<i', raw, cursor)[0]
    if not 0 < used_count <= len(bones):
        raise ValueError('Invalid LOD0 bone map')
    cursor += 4
    palette = struct.unpack_from('<' + 'H' * used_count, raw, cursor)
    if len(set(palette)) != used_count or any(index >= len(bones) for index in palette):
        raise ValueError('Invalid LOD0 bone palette index')
    vertices = geometry['vertex_count']
    start = lod0_vertex_offset(raw, geometry)
    if vertices <= 0 or start < 0 or start + vertices * 64 > len(raw):
        raise ValueError('Invalid LOD0 vertex range')
    dominant = Counter()
    weighted = Counter()
    for vertex in range(vertices):
        offset = start + vertex * 64
        indices = raw[offset + 56:offset + 60]
        weights = raw[offset + 60:offset + 64]
        if sum(weights) != 255:
            raise ValueError('LOD0 skin weights do not sum to 255')
        active = set()
        for index, weight in zip(indices, weights):
            if weight:
                if index >= used_count:
                    raise ValueError('LOD0 skin index outside bone palette')
                active.add(palette[index])
        weighted.update(active)
        dominant[palette[indices[max(range(4), key=lambda i: weights[i])]]] += 1
    if sum(dominant.values()) != vertices:
        raise ValueError('LOD0 vertex attribution incomplete')
    return {'vertex_count': vertices, 'index_count': index_count,
            'vertex_offset_in_export': start,
            'index_offset_in_export': index_offset,
            'influence_layout': 'inferred 64-byte vertices; indices 56..59, weights 60..63',
            'active_bones': [{'bone': bones[index], 'skeleton_index': index,
                              'weighted_vertices': weighted[index], 'dominant_vertices': dominant[index]}
                             for index in palette]}


def rigid_partition_summary(raw, skin):
    """Count why an inspection-only rigid split is or is not lossless."""
    palette = [item['skeleton_index'] for item in skin['active_bones']]
    by_vertex = []
    mixed = 0
    for vertex in range(skin['vertex_count']):
        offset = skin['vertex_offset_in_export'] + vertex * 64
        indices = raw[offset + 56:offset + 60]
        weights = raw[offset + 60:offset + 64]
        active = [i for i, weight in enumerate(weights) if weight]
        if len(active) != 1 or weights[active[0]] != 255:
            mixed += 1
            by_vertex.append(None)
        else:
            by_vertex.append(palette[indices[active[0]]])
    indices = struct.unpack_from('<' + 'H' * skin['index_count'], raw,
                                 skin['index_offset_in_export'])
    rigid = mixed_faces = crossing = 0
    for start in range(0, len(indices), 3):
        bones = [by_vertex[vertex] for vertex in indices[start:start + 3]]
        if None in bones:
            mixed_faces += 1
        elif len(set(bones)) != 1:
            crossing += 1
        else:
            rigid += 1
    if (rigid + mixed_faces + crossing) * 3 != skin['index_count']:
        raise ValueError('Partition topology count mismatch')
    return {'mixed_weight_vertices': mixed, 'mixed_weight_triangles': mixed_faces,
            'cross_bone_triangles': crossing, 'rigid_triangles': rigid,
            'rigid_split_eligible': mixed == 0 and crossing == 0}


def audit(manifest, names, package, geometry=None):
    results = []
    mesh_by_path = {item['path']: item for item in geometry} if geometry is not None else {}
    with package.open('rb') as source:
        for asset in manifest['assets']:
            source.seek(asset['package_offset'])
            raw = source.read(asset['serialized_size'])
            if len(raw) != asset['serialized_size']:
                raise ValueError('Truncated asset: ' + asset['path'])
            digest = hashlib.sha256(raw).hexdigest()
            if digest != asset['sha256_actual'] or digest != asset['sha256_inventory']:
                raise ValueError('Source asset hash changed: ' + asset['path'])
            offset, bones, depth, lods = skeletal_bones(raw, asset['tag_stream_end_in_export'], names)
            result = {'path': asset['path'], 'source_sha256': digest,
                      'bounds_offset_in_export': offset, 'bone_count': len(bones),
                      'bone_names': bones, 'depth': depth, 'lod_count': lods,
                      'animation_verified': False,
                      'reference_bones': reference_bones(raw, offset, bones),
                      'reference_layout': 'inferred FName/flags/quat/position/children/parent; raw parent-local package units'}
            if geometry is not None:
                if asset['path'] not in mesh_by_path or mesh_by_path[asset['path']]['sha256'] != digest:
                    raise ValueError('Missing or changed geometry inventory: ' + asset['path'])
                result['lod0_skin'] = lod0_skin(raw, bones, offset, mesh_by_path[asset['path']])
                result['rigid_partition'] = rigid_partition_summary(raw, result['lod0_skin'])
            results.append(result)
    return {'schema': 1, 'source_package': str(package), 'assets': results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-manifest', required=True, type=pathlib.Path)
    parser.add_argument('--names', required=True, type=pathlib.Path)
    parser.add_argument('--package', type=pathlib.Path)
    parser.add_argument('--geometry-manifest', type=pathlib.Path,
                        help='Optional verified LOD0 export inventory for skin influence audit')
    parser.add_argument('--output', required=True, type=pathlib.Path)
    args = parser.parse_args()
    manifest = json.loads(args.source_manifest.read_text(encoding='utf-8'))
    package = args.package or pathlib.Path(manifest['source_package'])
    geometry = json.loads(args.geometry_manifest.read_text(encoding='utf-8')) if args.geometry_manifest else None
    report = audit(manifest, name_table(args.names), package, geometry)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
