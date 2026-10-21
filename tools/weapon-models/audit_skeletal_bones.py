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


def lod0_chunks(raw, cursor, vertex_count, vertex_start, bones):
    """Recover GoWU LOD0 per-chunk BoneMaps from a bounded verified export.

    UsedBones/ActiveBoneIndices is *not* the palette of every GPU vertex.
    Each chunk has a separate BoneMap; the section names its chunk. The
    serialized CPU vertices are cross-checked against every GPU vertex.
    """
    chunk_count = struct.unpack_from('<i', raw, cursor)[0]
    if not 0 < chunk_count <= 32:
        raise ValueError('Invalid LOD0 chunk count')
    cursor += 4
    chunks = []
    for number in range(chunk_count):
        if cursor + 8 > vertex_start:
            raise ValueError('Truncated LOD0 chunk header')
        first_vertex, rigid_vertices = struct.unpack_from('<ii', raw, cursor)
        if (first_vertex != (chunks[-1]['last_vertex_exclusive'] if chunks else 0) or
                rigid_vertices < 0 or first_vertex + rigid_vertices > vertex_count):
            raise ValueError('Noncontiguous LOD0 chunk vertices')
        # In the verified GoWU package the serialized rigid vertex is 57 bytes:
        # position(12), packed tangent/normal(8), four float UVs(32),
        # vertex color(4), bone index(1). The GPU vertex then has four bone
        # indices and four weights instead of the single rigid index.
        rigid_start = cursor + 8
        rigid_end = rigid_start + rigid_vertices * 57
        if rigid_end > vertex_start:
            raise ValueError('Rigid vertices exceed LOD0 source')
        if rigid_end + 12 > vertex_start:
            raise ValueError('Truncated LOD0 soft vertex header')
        element_size, soft_vertices, reported_stride = struct.unpack_from('<iii', raw, rigid_end)
        if (element_size, reported_stride) not in ((0, 0), (4, 68)) or \
                (element_size == 0 and soft_vertices != 0) or soft_vertices < 0:
            raise ValueError('Unsupported LOD0 soft vertex layout')
        last_vertex = first_vertex + rigid_vertices + soft_vertices
        soft_start = rigid_end + 12
        map_count_start = soft_start + soft_vertices * 64
        if last_vertex > vertex_count or map_count_start + 4 > vertex_start:
            raise ValueError('Soft vertices exceed LOD0 source')
        for local in range(rigid_vertices):
            cpu_at = rigid_start + local * 57
            gpu_at = vertex_start + (first_vertex + local) * 64
            if (raw[cpu_at:cpu_at + 56] != raw[gpu_at:gpu_at + 56] or
                    raw[cpu_at + 56] != raw[gpu_at + 56] or
                    raw[gpu_at + 60:gpu_at + 64] != bytes((255, 0, 0, 0))):
                raise ValueError('Rigid CPU/GPU LOD0 vertex mismatch')
        for local in range(soft_vertices):
            cpu_at = soft_start + local * 64
            gpu_at = vertex_start + (first_vertex + rigid_vertices + local) * 64
            if raw[cpu_at:cpu_at + 64] != raw[gpu_at:gpu_at + 64]:
                raise ValueError('Soft CPU/GPU LOD0 vertex mismatch')
        count = struct.unpack_from('<i', raw, map_count_start)[0]
        if not 0 < count <= len(bones):
            raise ValueError('Invalid LOD0 chunk BoneMap count')
        map_start = map_count_start + 4
        footer = map_start + count * 2
        if footer + 12 > vertex_start:
            raise ValueError('Truncated LOD0 chunk BoneMap')
        mapping = struct.unpack_from('<' + 'h' * count, raw, map_start)
        if any(index < 0 or index >= len(bones) for index in mapping) or len(set(mapping)) != count:
            raise ValueError('Invalid LOD0 chunk BoneMap')
        footer_rigid, footer_soft, maximum_influences = struct.unpack_from('<iii', raw, footer)
        if (footer_rigid != rigid_vertices or footer_soft != soft_vertices or
                not 1 <= maximum_influences <= 4):
            raise ValueError('LOD0 chunk footer disagrees with vertex header')
        for vertex in range(first_vertex, last_vertex):
            at = vertex_start + vertex * 64
            if any(index >= count for index, weight in zip(raw[at + 56:at + 60],
                                                            raw[at + 60:at + 64]) if weight):
                raise ValueError('LOD0 skin index outside chunk BoneMap')
        if number + 1 < chunk_count:
            if struct.unpack_from('<i', raw, footer + 12)[0] != last_vertex:
                raise ValueError('Next LOD0 chunk is not contiguous')
        elif last_vertex != vertex_count or struct.unpack_from('<i', raw, footer + 16)[0] != vertex_count:
            raise ValueError('LOD0 chunk vertices differ from GPU header')
        chunks.append({'first_vertex': first_vertex,
                       'last_vertex_exclusive': first_vertex + rigid_vertices + soft_vertices,
                       'rigid_vertices': rigid_vertices, 'soft_vertices': soft_vertices,
                       'maximum_influences': maximum_influences,
                       'bone_map': list(mapping),
                       'bone_names': [bones[index] for index in mapping],
                       'footer_offset_in_export': footer})
        cursor = footer + 12
    return chunks


def vertex_bone_map(skin, vertex):
    for chunk in skin['chunks']:
        if chunk['first_vertex'] <= vertex < chunk['last_vertex_exclusive']:
            return chunk['bone_map']
    raise ValueError('Vertex outside all LOD0 chunks')


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
    section_rows = []
    for _ in range(sections):
        material, chunk, first_index, count = struct.unpack_from('<HHIH', raw, cursor)
        section_rows.append({'material_index': material, 'chunk_index': chunk,
                             'first_index': first_index, 'triangles': count})
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
    cursor += used_count * 2
    vertices = geometry['vertex_count']
    start = lod0_vertex_offset(raw, geometry)
    if vertices <= 0 or start < 0 or start + vertices * 64 > len(raw):
        raise ValueError('Invalid LOD0 vertex range')
    chunks = lod0_chunks(raw, cursor, vertices, start, bones)
    if {index for chunk in chunks for index in chunk['bone_map']} != set(palette):
        raise ValueError('LOD0 active bones differ from chunk BoneMaps')
    indices = struct.unpack_from('<' + 'H' * index_count, raw, index_offset)
    covered = 0
    for section in section_rows:
        first, count = section['first_index'], section['triangles'] * 3
        chunk_index = section['chunk_index']
        if first != covered or first + count > index_count or chunk_index >= len(chunks):
            raise ValueError('Invalid LOD0 section topology')
        chunk = chunks[chunk_index]
        if any(not chunk['first_vertex'] <= vertex < chunk['last_vertex_exclusive']
               for vertex in indices[first:first + count]):
            raise ValueError('LOD0 section indexes outside its chunk')
        covered += count
    if covered != index_count:
        raise ValueError('Uncovered LOD0 index buffer')
    chunk_by_vertex = [None] * vertices
    for chunk in chunks:
        for vertex in range(chunk['first_vertex'], chunk['last_vertex_exclusive']):
            chunk_by_vertex[vertex] = chunk['bone_map']
    if any(mapping is None for mapping in chunk_by_vertex):
        raise ValueError('LOD0 chunk vertex gap')
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
                mapping = chunk_by_vertex[vertex]
                if index >= len(mapping):
                    raise ValueError('LOD0 skin index outside chunk BoneMap')
                active.add(mapping[index])
        weighted.update(active)
        dominant[chunk_by_vertex[vertex][indices[max(range(4), key=lambda i: weights[i])]]] += 1
    if sum(dominant.values()) != vertices:
        raise ValueError('LOD0 vertex attribution incomplete')
    return {'vertex_count': vertices, 'index_count': index_count,
            'vertex_offset_in_export': start,
            'index_offset_in_export': index_offset,
            'influence_layout': 'verified GoWU LOD0 chunks; indices 56..59 map through per-chunk BoneMap, weights 60..63',
            'sections': section_rows, 'chunks': chunks,
            'active_bone_indices': list(palette),
            'active_bones': [{'bone': bones[index], 'skeleton_index': index,
                              'weighted_vertices': weighted[index], 'dominant_vertices': dominant[index]}
                             for index in palette]}


def rigid_partition_summary(raw, skin):
    """Count why an inspection-only rigid split is or is not lossless."""
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
            mapping = vertex_bone_map(skin, vertex)
            if indices[active[0]] >= len(mapping):
                raise ValueError('Rigid influence outside chunk BoneMap')
            by_vertex.append(mapping[indices[active[0]]])
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
