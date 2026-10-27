"""Summarize original held-Gnasher character-hand bones in its main-bone frame.

The purchase-derived matrices belong under ignored analysis/. A hand-bone
origin is a pose reference, not a proven controller grip or lever contact.
"""
import argparse
import json
import math
from pathlib import Path
from statistics import median


def rotation(matrix):
    if len(matrix) != 16 or not all(isinstance(x, (int, float)) and math.isfinite(x)
                                         for x in matrix):
        raise ValueError('Invalid 4x4 bone matrix')
    rows = [matrix[0:3], matrix[4:7], matrix[8:11]]
    for i in range(3):
        for j in range(3):
            dot = sum(rows[i][k] * rows[j][k] for k in range(3))
            if abs(dot - (1.0 if i == j else 0.0)) > .005:
                raise ValueError('Bone rotation is not orthonormal')
    return rows


def point_in_main(main, other):
    axes = rotation(main)
    rotation(other)
    difference = [other[i] - main[i] for i in (12, 13, 14)]
    return [sum(difference[j] * axes[i][j] for j in range(3))
            for i in range(3)]


def basis_in_main(main, other):
    parent, child = rotation(main), rotation(other)
    return [[sum(child[i][k] * parent[j][k] for k in range(3))
             for j in range(3)] for i in range(3)]


def orientation_gap(a, b):
    trace = sum(a[i][j] * b[i][j] for i in range(3) for j in range(3))
    return math.degrees(math.acos(max(-1., min(1., (trace - 1.) * .5))))


def summary(points, bases):
    center = [median(point[i] for point in points) for i in range(3)]
    representative = min(range(len(points)), key=lambda i: math.dist(center, points[i]))
    return {'median_raw': center,
            'maximum_drift_raw': max(math.dist(center, point) for point in points),
            'representative_frame': representative,
            'basis_in_main': bases[representative],
            'maximum_basis_drift_degrees': max(
                orientation_gap(bases[representative], basis) for basis in bases)}


def analyze(capture):
    rows = capture.get('held_grips')
    if capture.get('schema') != 2 or not capture.get('passed') or \
            capture.get('left_hand_ik_on') is not True or \
            capture.get('animation') != 'held idle' or \
            not isinstance(rows, list) or len(rows) != 24 or \
            [row.get('frame') for row in rows] != list(range(24)):
        raise ValueError('Expected complete passed held-Gnasher capture')
    expected = {'main', 'lever', 'hand_right', 'hand_left', 'ik_left'}
    transformed = {key: [] for key in expected - {'main'}}
    bases = {key: [] for key in transformed}
    for row in rows:
        for key in transformed:
            transformed[key].append(point_in_main(row['main'], row[key]))
            bases[key].append(basis_in_main(row['main'], row[key]))
    return {'schema': 1, 'capture_pid': capture.get('game_pid'),
            'frames': len(rows),
            'bones_in_main': {key: summary(points, bases[key])
                              for key, points in sorted(transformed.items())}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = analyze(json.loads(args.capture.read_text(encoding='utf-8')))
    value = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.write_text(value, encoding='utf-8')
    else:
        print(value, end='')


if __name__ == '__main__':
    main()
