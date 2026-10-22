"""Measure original Lancer palm centers in its mesh frame from owned-game data.

The 24-frame capture and derived result stay in ignored analysis/. UE3 FMatrix
uses row-vector transforms; each world hand origin is projected onto the gun's
three world-space basis rows. This is a palm-center candidate, not a measured
Quest controller grip offset or proof of visual contact.
"""
import argparse
import json
import math
from pathlib import Path
import statistics


def origin(matrix):
    if len(matrix) != 16 or not all(math.isfinite(value) for value in matrix):
        raise ValueError('Invalid original 4x4 matrix')
    if any(abs(matrix[index]) > .002 for index in (3, 7, 11)) or abs(matrix[15] - 1) > .002:
        raise ValueError('Original matrix is not affine')
    return matrix[12:15]


def gun_local_point(gun, hand):
    delta = [a - b for a, b in zip(origin(hand), origin(gun))]
    rows = [gun[index:index + 3] for index in (0, 4, 8)]
    lengths = [sum(value * value for value in row) for row in rows]
    if any(length < .01 or length > 100 for length in lengths):
        raise ValueError('Gun basis has invalid scale')
    for i in range(3):
        for j in range(i + 1, 3):
            product = sum(rows[i][k] * rows[j][k] for k in range(3))
            if abs(product) > .002 * math.sqrt(lengths[i] * lengths[j]):
                raise ValueError('Gun basis is sheared')
    if max(lengths) - min(lengths) > .04 * max(lengths):
        raise ValueError('Gun basis has nonuniform scale')
    return [sum(delta[k] * rows[i][k] for k in range(3)) / lengths[i]
            for i in range(3)]


def hand_basis_in_gun(gun, hand):
    """Return each unit hand-bone axis expressed in the original gun frame."""
    origin(gun)
    origin(hand)
    gun_rows = [gun[index:index + 3] for index in (0, 4, 8)]
    hand_rows = [hand[index:index + 3] for index in (0, 4, 8)]
    gun_scale = [math.sqrt(sum(value * value for value in row)) for row in gun_rows]
    hand_scale = [math.sqrt(sum(value * value for value in row)) for row in hand_rows]
    if any(not math.isfinite(value) or value < .1 or value > 10
           for value in gun_scale + hand_scale):
        raise ValueError('Invalid original hand/gun basis scale')
    basis = [[sum(hand_rows[i][k] * gun_rows[j][k] for k in range(3)) /
              (hand_scale[i] * gun_scale[j]) for j in range(3)] for i in range(3)]
    for i in range(3):
        for j in range(i, 3):
            dot = sum(basis[i][k] * basis[j][k] for k in range(3))
            if abs(dot - (1 if i == j else 0)) > .002:
                raise ValueError('Original hand/gun basis is not rigid')
    determinant = (basis[0][0] * (basis[1][1] * basis[2][2] - basis[1][2] * basis[2][1]) -
                   basis[0][1] * (basis[1][0] * basis[2][2] - basis[1][2] * basis[2][0]) +
                   basis[0][2] * (basis[1][0] * basis[2][1] - basis[1][1] * basis[2][0]))
    if abs(determinant - 1) > .002:
        raise ValueError('Original hand/gun basis is reflected')
    return basis


def centered(points):
    center = [statistics.median(point[axis] for point in points) for axis in range(3)]
    return center, max(math.dist(point, center) for point in points)


def summarize(capture):
    rows = capture.get('samples')
    if (capture.get('passed') is not True or capture.get('left_hand_ik_on') is not True or
            capture.get('weapon_mesh_asset', {}).get('name') != 'COGAssaultRifle' or
            not isinstance(rows, list) or len(rows) != 24 or
            [row.get('sample') for row in rows] != list(range(24)) or
            any(rows[index]['qpc'] <= rows[index - 1]['qpc'] for index in range(1, 24))):
        raise ValueError('Expected complete settled original Lancer capture')
    local = {name: [gun_local_point(row['gun'], row[name]) for row in rows]
             for name in ('ik_left', 'hand_left', 'hand_right')}
    centers = {name: centered(points) for name, points in local.items()}
    right_bases = [hand_basis_in_gun(row['gun'], row['hand_right']) for row in rows]
    right_basis = [[statistics.median(basis[axis][component] for basis in right_bases)
                    for component in range(3)] for axis in range(3)]
    right_basis_drift = max(abs(basis[axis][component] - right_basis[axis][component])
                            for basis in right_bases for axis in range(3)
                            for component in range(3))
    maximum_ik_palm_error = max(math.dist(origin(row['ik_left']), origin(row['hand_left']))
                                for row in rows)
    world_travel = math.dist(origin(rows[0]['gun']), origin(rows[-1]['gun']))
    return {'schema': 1, 'source': 'owned original retail Lancer bone/component matrices',
            'capture_pid': capture.get('game_pid'), 'samples': len(rows),
            'left_palm_in_gun_raw': centers['hand_left'][0],
            'left_ik_in_gun_raw': centers['ik_left'][0],
            'right_palm_in_gun_raw': centers['hand_right'][0],
            'right_hand_basis_in_gun_rows': right_basis,
            'maximum_right_hand_basis_component_drift': right_basis_drift,
            'maximum_left_palm_local_drift_raw': centers['hand_left'][1],
            'maximum_left_ik_local_drift_raw': centers['ik_left'][1],
            'maximum_right_palm_local_drift_raw': centers['hand_right'][1],
            'maximum_ik_to_palm_world_error_raw': maximum_ik_palm_error,
            'first_to_last_gun_world_travel_raw': world_travel,
            'palm_separation_raw': math.dist(centers['hand_left'][0],
                                             centers['hand_right'][0])}


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
