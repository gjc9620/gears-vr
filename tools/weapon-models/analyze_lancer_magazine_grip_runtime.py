"""Measure original detached Lancer magazine relative to its animated left hand.

Only original retail, same-tick matrices are accepted. This yields a reference
hand-bone attachment, not a measured VR controller grip or insertion stroke.
Raw captures and derived summaries remain under ignored analysis/.
"""
import argparse
import json
import math
from pathlib import Path
import statistics

from analyze_lancer_support_runtime import gun_local_point, origin


def relative_matrix(parent, child):
    position = gun_local_point(parent, child)
    rows = [parent[index:index + 3] for index in (0, 4, 8)]
    lengths = [sum(value * value for value in row) for row in rows]
    axes = []
    for index in (0, 4, 8):
        vector = child[index:index + 3]
        axes.append([sum(vector[world] * rows[axis][world] for world in range(3)) /
                     lengths[axis] for axis in range(3)])
    return axes, position


def median_vector(rows):
    return [statistics.median(row[index] for row in rows) for index in range(len(rows[0]))]


def summarize(capture):
    timeline = capture.get('reload_visual_timeline')
    poses = capture.get('held_magazine_poses')
    if (capture.get('passed') is not True or capture.get('mode') != 'reload_visual' or
            capture.get('weapon_mesh_asset', {}).get('name') != 'COGAssaultRifle' or
            not isinstance(timeline, list) or not isinstance(poses, list) or
            len(poses) < 2 or
            [row.get('sample') for row in timeline] != list(range(len(timeline))) or
            any(timeline[index]['qpc'] <= timeline[index - 1]['qpc']
                for index in range(1, len(timeline))) or
            len({row.get('sample') for row in poses}) != len(poses) or
            [pose['sample'] for pose in poses] != sorted(pose['sample'] for pose in poses) or
            any(pose['sample'] < 0 or pose['sample'] >= len(timeline) or
                not timeline[pose['sample']]['mag_pawn'] or
                pose['qpc'] != timeline[pose['sample']]['qpc'] for pose in poses)):
        raise ValueError('Expected complete original held-Lancer-magazine capture')
    relatives = [relative_matrix(row['left'], row['magazine']) for row in poses]
    origins = [entry[1] for entry in relatives]
    center = median_vector(origins)
    axes = [[entry[0][axis] for entry in relatives] for axis in range(3)]
    axis_centers = [median_vector(axis) for axis in axes]
    origin_drift = max(math.dist(point, center) for point in origins)
    basis_drift = max(math.sqrt(sum((entry[0][axis][component] -
                                     axis_centers[axis][component]) ** 2
                                    for axis in range(3) for component in range(3)))
                      for entry in relatives)
    # This exploratory threshold selects a near-rigid interval of the original
    # animation. It is not a VR grab or insertion tolerance.
    near_hold = [math.dist(point, center) <= .005 and
                 math.sqrt(sum((entry[0][axis][component] -
                                axis_centers[axis][component]) ** 2
                               for axis in range(3) for component in range(3))) <= .003
                 for point, entry in zip(origins, relatives)]
    runs = []
    start = None
    for index in range(len(poses) + 1):
        qualifies = index < len(poses) and near_hold[index] and (
            start is None or poses[index]['sample'] == poses[index - 1]['sample'] + 1)
        if not qualifies and start is not None:
            runs.append((start, index))
            start = None
        if index < len(poses) and near_hold[index] and start is None:
            start = index
    stable = max(runs, key=lambda run: run[1] - run[0], default=(0, 0))
    if stable[1] - stable[0] < 8:
        raise ValueError('Original Lancer magazine has no bounded near-rigid hand interval')
    held = relatives[stable[0]:stable[1]]
    held_origins = [entry[1] for entry in held]
    held_center = median_vector(held_origins)
    held_axes = [median_vector([entry[0][axis] for entry in held])
                 for axis in range(3)]
    lengths = [math.sqrt(sum(value * value for value in axis)) for axis in held_axes]
    dots = [sum(held_axes[i][k] * held_axes[j][k] for k in range(3))
            for i, j in ((0, 1), (0, 2), (1, 2))]
    determinant = (held_axes[0][0] *
                   (held_axes[1][1] * held_axes[2][2] - held_axes[1][2] * held_axes[2][1]) -
                   held_axes[0][1] *
                   (held_axes[1][0] * held_axes[2][2] - held_axes[1][2] * held_axes[2][0]) +
                   held_axes[0][2] *
                   (held_axes[1][0] * held_axes[2][1] - held_axes[1][1] * held_axes[2][0]))
    if (any(abs(length - 1) > .02 for length in lengths) or
            any(abs(dot) > .02 for dot in dots) or abs(determinant - 1) > .03):
        raise ValueError('Original magazine/left-hand basis is not a proper rotation')
    held_origin_drift = max(math.dist(point, held_center) for point in held_origins)
    held_basis_drift = max(math.sqrt(sum((entry[0][axis][component] -
                                        held_axes[axis][component]) ** 2
                                       for axis in range(3) for component in range(3)))
                           for entry in held)
    mag_in_gun = [gun_local_point(row['gun'], row['magazine']) for row in poses]
    hand_in_gun = [gun_local_point(row['gun'], row['left']) for row in poses]
    first, last = poses[0]['sample'], poses[-1]['sample']
    held_first = poses[stable[0]]['sample']
    held_last = poses[stable[1] - 1]['sample']
    return {'schema': 1,
            'source': 'owned original retail Lancer hand bone and independent magazine component',
            'capture_pid': capture.get('game_pid'), 'attached_samples': len(poses),
            'first_attached_elapsed_seconds': timeline[first]['world'] - timeline[first]['reload_start'],
            'last_attached_elapsed_seconds': timeline[last]['world'] - timeline[last]['reload_start'],
            'stable_hand_hold_samples': stable[1] - stable[0],
            'stable_hand_hold_first_sample': held_first,
            'stable_hand_hold_last_sample': held_last,
            'stable_hand_hold_first_elapsed_seconds':
                timeline[held_first]['world'] - timeline[held_first]['reload_start'],
            'stable_hand_hold_last_elapsed_seconds':
                timeline[held_last]['world'] - timeline[held_last]['reload_start'],
            'magazine_origin_in_left_hand_during_hold_raw': held_center,
            'magazine_axes_in_left_hand_during_hold': held_axes,
            'magazine_axis_lengths_during_hold': lengths,
            'magazine_axis_dots_during_hold': dots,
            'magazine_basis_determinant_during_hold': determinant,
            'maximum_origin_drift_during_hold_raw': held_origin_drift,
            'maximum_basis_drift_during_hold': held_basis_drift,
            'maximum_origin_drift_all_attached_raw': origin_drift,
            'maximum_basis_drift_all_attached': basis_drift,
            'first_magazine_origin_in_gun_raw': mag_in_gun[0],
            'last_magazine_origin_in_gun_raw': mag_in_gun[-1],
            'magazine_origin_travel_in_gun_raw': math.dist(mag_in_gun[0], mag_in_gun[-1]),
            'first_left_hand_origin_in_gun_raw': hand_in_gun[0],
            'last_left_hand_origin_in_gun_raw': hand_in_gun[-1],
            'first_magazine_world_origin_raw': origin(poses[0]['magazine'])}


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
