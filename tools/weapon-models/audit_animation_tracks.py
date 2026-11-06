"""Audit shipped UE3 AnimSequence track counts without exporting animation bytes.

The caller supplies a decompressed analysis copy and its export/name inventory.
This is a bounded reader for the Reloaded package layout already used by
compare_default_properties.py, not a general UE3 animation decoder.
"""
import argparse
import csv
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'package-inspector'))
from compare_default_properties import locate_defaults


def table(path):
    with path.open(encoding='utf-8-sig', newline='') as stream:
        return list(csv.DictReader(stream, delimiter='\t'))


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def decode_interval_fixed32_no_w(stream, offset, count):
    """Decode one verified UE3 interval quaternion track into analysis-only poses.

    Layout follows UEViewer's FQuatIntervalFixed32NoW: 3 float minima,
    3 float ranges, then one packed uint32 per key. See the pinned reference:
    https://github.com/gildor2/UEViewer/blob/a0bfb468d42be831b126632fd8a0ae6b3614f981/Unreal/UnrealMesh/UnMeshTypes.h#L2224
    Values remain in the animation's stored local convention, not mesh/world space.
    """
    if count < 2 or offset < 0 or offset + 24 + 4 * count > len(stream):
        raise ValueError('Interval quaternion track exceeds compressed stream')
    minima = struct.unpack_from('<3f', stream, offset)
    ranges = struct.unpack_from('<3f', stream, offset + 12)
    if not all(math.isfinite(value) for value in (*minima, *ranges)):
        raise ValueError('Nonfinite interval quaternion header')
    samples = []
    for index in range(count):
        packed = struct.unpack_from('<I', stream, offset + 24 + 4 * index)[0]
        xyz = [(((packed >> shift) & mask) / scale - 1.0) * span + low
               for shift, mask, scale, span, low in zip(
                   (21, 10, 0), (2047, 2047, 1023), (1023, 1023, 511),
                   ranges, minima)]
        squared = sum(value * value for value in xyz)
        if squared > 1.0001:
            raise ValueError('Invalid decoded interval quaternion')
        w = math.sqrt(max(0.0, 1.0 - squared))
        sample = {'key_index': index, 'quaternion_xyzw': [*xyz, w]}
        if abs(xyz[1]) < 1e-4 and abs(xyz[2]) < 1e-4:
            sample['signed_local_x_degrees'] = math.degrees(2.0 * math.atan2(xyz[0], w))
        samples.append(sample)
    return samples


def decode_float96_no_w(stream, offset, count):
    """UEViewer pinned UnMeshTypes.h FQuatFloat96NoW: three floats, W>=0.

    Same reference revision as the interval decoder above. Stored animation
    coordinates only; no model/world convention or key timing is inferred.
    """
    if count < 1 or offset < 0 or offset + 12 * count > len(stream):
        raise ValueError('Float96 quaternion track exceeds compressed stream')
    samples = []
    for index in range(count):
        xyz = struct.unpack_from('<3f', stream, offset + index * 12)
        squared = sum(value * value for value in xyz)
        if not all(math.isfinite(value) for value in xyz) or squared > 1.0001:
            raise ValueError('Invalid Float96 quaternion')
        w = math.sqrt(max(0.0, 1.0 - squared))
        sample = {'key_index': index, 'quaternion_xyzw': [*xyz, w]}
        if abs(xyz[1]) < 1e-4 and abs(xyz[2]) < 1e-4:
            sample['signed_local_x_degrees'] = math.degrees(2.0 * math.atan2(xyz[0], w))
        samples.append(sample)
    return samples


def audit(package, inventory_path, names_path, prefix, animset_path=None,
          sample_sequence=None, sample_bone=None, legacy_rotation_ranges=False):
    if (sample_sequence is None) != (sample_bone is None):
        raise ValueError('Sampling requires both a sequence and a bone')
    if sample_bone is not None and animset_path is None:
        raise ValueError('Sampling requires an AnimSet bone-name list')
    inventory = table(inventory_path)
    names = {int(row['Index']): row['Name'] for row in table(names_path)}
    exports = {int(row['Index']): row['Path'] for row in inventory}
    selected = [row for row in inventory
                if row['Class'] == 'AnimSequence' and row['Path'].startswith(prefix)]
    if not selected:
        raise ValueError('No matching AnimSequence exports')
    package_size = package.stat().st_size
    result = {'schema': 2, 'source_package': str(package),
              'source_sha256': file_sha256(package),
              'object_prefix': prefix, 'sequences': []}
    with package.open('rb') as stream:
        bone_names = None
        if animset_path:
            matches = [row for row in inventory if row['Path'] == animset_path]
            if len(matches) != 1 or matches[0]['Class'] != 'AnimSet':
                raise ValueError('Expected exactly one matching AnimSet')
            animset = matches[0]
            stream.seek(int(animset['Offset']))
            data = stream.read(int(animset['Size']))
            digest = hashlib.sha256(data).hexdigest()
            if digest != animset['SHA256']:
                raise ValueError('AnimSet hash mismatch')
            tags, _, _, _ = locate_defaults(data, names, exports)
            encoded_bones = bytes.fromhex(tags['TrackBoneNames']['value']['hex'])
            count = struct.unpack_from('<i', encoded_bones)[0]
            if count <= 0 or count > 512 or len(encoded_bones) != 4 + count * 8:
                raise ValueError('Malformed AnimSet bone-name array')
            bone_names = []
            for index in range(count):
                name_index, number = struct.unpack_from('<ii', encoded_bones, 4 + index * 8)
                if name_index not in names or number < 0:
                    raise ValueError('Invalid AnimSet bone FName')
                bone_names.append(names[name_index] +
                                  ('_' + str(number - 1) if number else ''))
            result['animset_path'] = animset_path
            result['animset_sha256'] = digest
            result['track_bone_names'] = bone_names
        for row in selected:
            offset = int(row['Offset'])
            size = int(row['Size'])
            if offset < 0 or size < 0 or offset + size > package_size:
                raise ValueError('Export outside package: ' + row['Path'])
            stream.seek(offset)
            raw = stream.read(size)
            actual = hashlib.sha256(raw).hexdigest()
            if actual != row['SHA256']:
                raise ValueError('Export hash mismatch: ' + row['Path'])
            properties, end, _, start = locate_defaults(raw, names, exports)
            required = ('SequenceName', 'SequenceLength', 'NumFrames',
                        'CompressedTrackOffsets')
            if not all(key in properties for key in required):
                raise ValueError('Missing animation metadata: ' + row['Path'])
            # A missing compression tag means the class default applies. We
            # cannot name that default from this export alone, but key counts
            # remain available from CompressedTrackOffsets.
            rotation_compression = properties.get('RotationCompressionFormat', {}).get('value')
            encoded = properties['CompressedTrackOffsets']['value']['hex']
            offsets = bytes.fromhex(encoded)
            if len(offsets) < 4 or len(offsets) % 4:
                raise ValueError('Malformed track-offset array: ' + row['Path'])
            words = struct.unpack('<' + 'i' * (len(offsets) // 4), offsets)
            count = words[0]
            if count <= 0 or count % 4 or count != len(words) - 1 or count > 4096:
                raise ValueError('Unexpected track-offset count: ' + row['Path'])
            if bone_names is not None and count // 4 != len(bone_names):
                raise ValueError('Track count differs from AnimSet bone list: ' + row['Path'])
            tail = raw[end:]
            if len(tail) < 4:
                raise ValueError('Missing compressed stream length: ' + row['Path'])
            stream_size = struct.unpack_from('<i', tail)[0]
            if stream_size < 0 or stream_size != len(tail) - 4:
                raise ValueError('Unexpected native tail layout: ' + row['Path'])
            tracks = []
            for bone, begin in enumerate(range(1, len(words), 4)):
                trans_offset, trans_keys, rot_offset, rot_keys = words[begin:begin + 4]
                if min(trans_offset, trans_keys, rot_offset, rot_keys) < 0 or \
                   trans_offset > stream_size or rot_offset > stream_size:
                    raise ValueError('Invalid track metadata: ' + row['Path'])
                tracks.append({'bone_index': bone,
                               'bone_name': bone_names[bone] if bone_names else None,
                               'translation_offset': trans_offset,
                               'translation_keys': trans_keys,
                               'rotation_offset': rot_offset,
                               'rotation_keys': rot_keys})
            sampled_rotation = None
            if sample_sequence == properties['SequenceName']['value']:
                matching = [track for track in tracks if track['bone_name'] == sample_bone]
                if len(matching) != 1:
                    raise ValueError('Expected exactly one matching bone in ' + row['Path'])
                decoders = {'ACF_IntervalFixed32NoW': decode_interval_fixed32_no_w,
                            'ACF_Float96NoW': decode_float96_no_w}
                if rotation_compression not in decoders:
                    raise ValueError('Unsupported sampled rotation compression: ' + row['Path'])
                track = matching[0]
                sample_offset = track['rotation_offset']
                range_header = None
                if legacy_rotation_ranges:
                    if rotation_compression != 'ACF_Float96NoW' or track['rotation_keys'] < 2:
                        raise ValueError('Legacy Float96 ranges require a multi-key Float96 track')
                    boundaries = [value for other in tracks for value in
                                  (other['translation_offset'], other['rotation_offset'])
                                  if value > sample_offset]
                    boundary = min(boundaries, default=stream_size)
                    if sample_offset + 24 + 12 * track['rotation_keys'] != boundary:
                        raise ValueError('Legacy range header/keys do not exactly fill track span')
                    range_header = struct.unpack_from('<6f', tail[4:], sample_offset)
                    if not all(math.isfinite(x) for x in range_header) or any(x < 0 for x in range_header[3:]):
                        raise ValueError('Invalid legacy rotation ranges')
                    sample_offset += 24
                samples = decoders[rotation_compression](tail[4:], sample_offset, track['rotation_keys'])
                if range_header is not None:
                    for sample in samples:
                        for value, low, span in zip(sample['quaternion_xyzw'][:3],range_header[:3],range_header[3:]):
                            if value < low-1e-5 or value > low+span+1e-5:
                                raise ValueError('Quaternion lies outside legacy track ranges')
                sampled_rotation = {
                    'bone_name': sample_bone,
                    'coordinate_convention': 'stored animation-local quaternion; mesh/world conversion unverified',
                    'timing': 'not decoded; game-runtime key times unverified',
                    'legacy_range_header': range_header,
                    'rotation_data_offset': sample_offset,
                    'samples': samples,
                }
            result['sequences'].append({
                'path': row['Path'], 'export_sha256': actual,
                'export_offset': offset, 'export_size': size,
                'tag_start': start, 'tag_end': end,
                'sequence_name': properties['SequenceName']['value'],
                'sequence_length_seconds': properties['SequenceLength']['value'],
                'frames': properties['NumFrames']['value'],
                'rotation_compression': rotation_compression,
                'compressed_stream_bytes': stream_size,
                'tracks': tracks,
                **({'sampled_rotation': sampled_rotation} if sampled_rotation is not None else {}),
            })
    if sample_sequence is not None and sum(
            item['sequence_name'] == sample_sequence for item in result['sequences']) != 1:
        raise ValueError('Expected exactly one sample sequence: ' + sample_sequence)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', required=True, type=Path)
    parser.add_argument('--inventory', required=True, type=Path)
    parser.add_argument('--names', required=True, type=Path)
    parser.add_argument('--object-prefix', required=True)
    parser.add_argument('--animset-path', help='Exact owning AnimSet object path')
    parser.add_argument('--sample-sequence', help='One sequence name for bounded rotation-key decoding')
    parser.add_argument('--sample-bone', help='One bone name from --animset-path')
    parser.add_argument('--legacy-rotation-ranges', action='store_true',
                        help='Explicit old-style Float96 range header; exact span and bounds required')
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    result = audit(args.package, args.inventory, args.names,
                   args.object_prefix, args.animset_path,
                   args.sample_sequence, args.sample_bone, args.legacy_rotation_ranges)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2), encoding='utf8')
    for sequence in result['sequences']:
        rotations = [track['rotation_keys'] for track in sequence['tracks']]
        print(sequence['sequence_name'], sequence['sequence_length_seconds'],
              'seconds; rotation keys per bone:', rotations)


if __name__ == '__main__':
    main()
