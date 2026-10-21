"""Audit original weapon mesh bone names without exporting game geometry.

The inventory and name table are local, ignored analysis artifacts. This tool
only reads the verified package bytes and writes a small metadata report under
the caller's ignored analysis directory.
"""
import argparse
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


def audit(manifest, names, package):
    results = []
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
            results.append({'path': asset['path'], 'source_sha256': digest,
                            'bounds_offset_in_export': offset, 'bone_count': len(bones),
                            'bone_names': bones, 'depth': depth, 'lod_count': lods,
                            'confidence': 'bone hierarchy only; skin influence and animation unverified'})
    return {'schema': 1, 'source_package': str(package), 'assets': results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-manifest', required=True, type=pathlib.Path)
    parser.add_argument('--names', required=True, type=pathlib.Path)
    parser.add_argument('--package', type=pathlib.Path)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    args = parser.parse_args()
    manifest = json.loads(args.source_manifest.read_text(encoding='utf-8'))
    package = args.package or pathlib.Path(manifest['source_package'])
    report = audit(manifest, name_table(args.names), package)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
