"""Summarize original Lancer reload bone motion from one owned retail receipt.

This is offline analysis of saved same-PlayerTick matrices. It never attaches to
the game, drives a mesh, or chooses a VR grasp surface from an animation alone.
"""

import argparse
import hashlib
import json
import math
from pathlib import Path


def unit_rows(matrix):
    if len(matrix) != 16 or not all(math.isfinite(value) for value in matrix):
        raise ValueError('Original component/bone matrix is incomplete')
    rows = [[matrix[4 * row + column] for column in range(3)] for row in range(3)]
    result = []
    for row in rows:
        norm = math.sqrt(sum(value * value for value in row))
        if not .98 <= norm <= 1.02:
            raise ValueError('Original component/bone matrix has unexpected scale')
        result.append([value / norm for value in row])
    for i in range(3):
        for j in range(i + 1, 3):
            if abs(sum(result[i][k] * result[j][k] for k in range(3))) > .01:
                raise ValueError('Original component/bone matrix is sheared')
    determinant = (
        result[0][0] * (result[1][1] * result[2][2] - result[1][2] * result[2][1]) -
        result[0][1] * (result[1][0] * result[2][2] - result[1][2] * result[2][0]) +
        result[0][2] * (result[1][0] * result[2][1] - result[1][1] * result[2][0]))
    if not .98 <= determinant <= 1.02:
        raise ValueError('Original component/bone matrix is mirrored')
    return result


def relative_pose(bone, gun):
    gun_rows, bone_rows = unit_rows(gun), unit_rows(bone)
    rotation = [[sum(bone_rows[i][k] * gun_rows[j][k] for k in range(3))
                 for j in range(3)] for i in range(3)]
    displacement = [bone[12 + k] - gun[12 + k] for k in range(3)]
    position = [sum(displacement[k] * gun_rows[i][k] for k in range(3))
                for i in range(3)]
    return rotation, position


def rotation_change(current, baseline):
    delta = [[sum(current[i][k] * baseline[j][k] for k in range(3))
              for j in range(3)] for i in range(3)]
    angle = math.acos(max(-1., min(1., (sum(delta[i][i] for i in range(3)) - 1.) / 2.)))
    if abs(math.sin(angle)) < .01:
        return math.degrees(angle), None
    axis = [(delta[(i + 2) % 3][(i + 1) % 3] -
             delta[(i + 1) % 3][(i + 2) % 3]) / (2. * math.sin(angle))
            for i in range(3)]
    axis_length = math.sqrt(sum(value * value for value in axis))
    if not .98 <= axis_length <= 1.02:
        raise ValueError('Original bone rotation has an invalid hinge axis')
    return math.degrees(angle), [value / axis_length for value in axis]


def summarize_part(rows, bones, name):
    initial_rotation, initial_position = relative_pose(bones[0][name], bones[0]['gun'])
    motions = []
    for timeline, sample in zip(rows, bones):
        rotation, position = relative_pose(sample[name], sample['gun'])
        degrees, axis = rotation_change(rotation, initial_rotation)
        shift = math.dist(position, initial_position)
        motions.append((degrees, shift, axis, timeline['ms'], timeline['sample']))
    peak = max(motions, key=lambda item: item[0])
    moving = [item for item in motions if item[0] > 5.]
    return {
        'reference_origin_in_gun_raw': initial_position,
        'peak_angle_degrees': peak[0],
        'peak_axis_in_gun': peak[2],
        'peak_sample': peak[4],
        'peak_ms': peak[3],
        'maximum_pivot_shift_raw': max(item[1] for item in motions),
        'first_above_five_degrees_ms': moving[0][3] if moving else None,
        'last_above_five_degrees_ms': moving[-1][3] if moving else None,
    }


def analyze(source):
    data = source.read_bytes()
    receipt = json.loads(data)
    if receipt.get('passed') is not True or receipt.get('mode') != 'reload_visual':
        raise ValueError('Input is not a passed original reload-visual receipt')
    rows, bones = receipt['reload_visual_timeline'], receipt['reload_bone_poses']
    if len(rows) < 10 or len(rows) != len(bones) or any(
        row['sample'] != sample['sample'] or row['qpc'] != sample['qpc']
        for row, sample in zip(rows, bones)
    ):
        raise ValueError('Original reload and bone rows are not paired')
    if not any(row['reloading'] for row in rows) or not any(
        not row['reloading'] for row in rows[next(
            index for index, row in enumerate(rows) if row['reloading']):]
    ):
        raise ValueError('Original reload did not finish in the receipt')
    return {
        'schema': 1,
        'source_receipt': str(source.resolve()),
        'source_sha256': hashlib.sha256(data).hexdigest(),
        'sample_count': len(rows),
        'reloading_samples': sum(row['reloading'] for row in rows),
        'latch': summarize_part(rows, bones, 'latch'),
        'receiver': summarize_part(rows, bones, 'receiver'),
        'source_limit': ('same-tick original animation only; does not identify a safe '
                         'controller grasp surface, physical stroke, or headset appearance'),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    summary = analyze(args.receipt)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
