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
    start = geometry['vertex_offset']
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
            'index_offset_in_export': index_offset,
            'influence_layout': 'inferred 64-byte vertices; indices 56..59, weights 60..63',
            'active_bones': [{'bone': bones[index], 'skeleton_index': index,
                              'weighted_vertices': weighted[index], 'dominant_vertices': dominant[index]}
                             for index in palette]}


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
                      'animation_verified': False}
            if geometry is not None:
                if asset['path'] not in mesh_by_path or mesh_by_path[asset['path']]['sha256'] != digest:
                    raise ValueError('Missing or changed geometry inventory: ' + asset['path'])
                result['lod0_skin'] = lod0_skin(raw, bones, offset, mesh_by_path[asset['path']])
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
