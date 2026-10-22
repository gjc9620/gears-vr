"""Summarize a bounded SG_Chamber main/lever bone-matrix capture.

The capture contains game-derived data and stays under ignored analysis/. This
tool only emits measurements; it does not alter an asset or infer a VR grip.
UE3 FMatrix uses row-vector transforms, so lever * transpose(main) expresses
the lever orientation in the main bone's reference frame.
"""
import argparse
import json
import math
from pathlib import Path


def rotation(matrix):
    if len(matrix) != 16 or not all(isinstance(x, (int, float)) and math.isfinite(x)
                                         for x in matrix):
        raise ValueError('Invalid 4x4 bone matrix')
    value = [matrix[0:3], matrix[4:7], matrix[8:11]]
    for i in range(3):
        for j in range(3):
            dot = sum(value[i][k] * value[j][k] for k in range(3))
            if abs(dot - (1.0 if i == j else 0.0)) > .005:
                raise ValueError('Bone rotation is not orthonormal')
    return value


def transpose(matrix):
    return [list(row) for row in zip(*matrix)]


def multiply(left, right):
    return [[sum(left[i][k] * right[k][j] for k in range(3))
             for j in range(3)] for i in range(3)]


def angle_axis(matrix):
    radians = math.acos(max(-1.0, min(1.0,
        (sum(matrix[i][i] for i in range(3)) - 1.0) * .5)))
    if radians < .01:
        return math.degrees(radians), None
    axis = [(matrix[2][1] - matrix[1][2]) / (2 * math.sin(radians)),
            (matrix[0][2] - matrix[2][0]) / (2 * math.sin(radians)),
            (matrix[1][0] - matrix[0][1]) / (2 * math.sin(radians))]
    length = math.sqrt(sum(x*x for x in axis))
    if abs(length - 1.0) > .05:
        raise ValueError('Rotation axis is unstable')
    return math.degrees(radians), [x / length for x in axis]


def pivot_in_main(sample, main_rotation):
    difference = [sample['lever'][i] - sample['main'][i] for i in (12, 13, 14)]
    return [sum(difference[j] * main_rotation[i][j] for j in range(3))
            for i in range(3)]


def summarize(capture):
    samples = capture.get('samples')
    if not capture.get('passed') or capture.get('animation') != 'SG_Chamber' or \
            not isinstance(samples, list) or len(samples) != 35 or \
            [row.get('frame') for row in samples] != list(range(35)):
        raise ValueError('Expected one complete 35-frame SG_Chamber capture')
    relative = []
    pivots = []
    for row in samples:
        main = rotation(row['main'])
        lever = rotation(row['lever'])
        relative.append(multiply(lever, transpose(main)))
        pivots.append(pivot_in_main(row, main))
    rotations = [angle_axis(multiply(transpose(relative[0]), current))
                 for current in relative]
    peak = max(range(35), key=lambda index: rotations[index][0])
    baseline = pivots[0]
    return {'schema': 1, 'capture_pid': capture.get('pid'), 'frames': len(samples),
            'peak_frame': peak, 'peak_degrees': rotations[peak][0],
            'peak_axis_in_main': rotations[peak][1],
            'final_degrees_from_home': rotations[-1][0],
            'initial_pivot_in_main_raw': baseline,
            'maximum_pivot_drift_raw': max(math.dist(baseline, point) for point in pivots)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = summarize(json.loads(args.capture.read_text(encoding='utf-8')))
    value = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.write_text(value, encoding='utf-8')
    else:
        print(value, end='')


if __name__ == '__main__':
    main()
