"""Project same-tick retail Lancer Magazine socket samples into the gun mesh.

The source capture and derived result belong under ignored analysis/. The
socket is an original animation/ejection anchor, not a proven VR feed-port
orientation or Quest controller grip calibration.
"""
import argparse
import json
import math
from pathlib import Path
import statistics

from analyze_lancer_support_runtime import gun_local_point


def local_socket(gun, world):
    if len(world) != 3 or not all(math.isfinite(value) for value in world):
        raise ValueError('Invalid original socket world point')
    point_matrix = [1., 0., 0., 0., 0., 1., 0., 0.,
                    0., 0., 1., 0., *world, 1.]
    return gun_local_point(gun, point_matrix)


def original_socket_fields(begin):
    fields = dict(item.split('=', 1) for item in begin.split()[1:])
    expected = {'pc', 'pawn', 'weapon', 'mesh', 'asset', 'socket', 'name', 'bone',
                'relative', 'rotation', 'count'}
    if (set(fields) not in (expected, expected | {'bone_index'}) or
            fields['name'] != 'Magazine' or
            fields['bone'] != 'b_AR_Magazine' or int(fields['count']) != 2):
        raise ValueError('Original Lancer socket identity changed')
    relative = [float(value) for value in fields['relative'].split(',')]
    if len(relative) != 3 or not all(math.isfinite(value) for value in relative):
        raise ValueError('Original socket relative location invalid')
    return fields, relative


def original_rows(capture, mode, rows_key):
    rows = capture.get(rows_key)
    if (capture.get('passed') is not True or capture.get('mode') != mode or
            capture.get('weapon_mesh_asset', {}).get('name') != 'COGAssaultRifle' or
            not isinstance(rows, list) or len(rows) < 3 or
            [row.get('sample') for row in rows] != list(range(len(rows))) or
            any(rows[index]['qpc'] <= rows[index - 1]['qpc']
                for index in range(1, len(rows)))):
        raise ValueError('Expected complete original Lancer socket capture')
    return rows


def summarize(capture, partition=None):
    rows = original_rows(capture, 'socket', 'socket_samples')
    if len(rows) != 16:
        raise ValueError('Expected 16 settled original Lancer socket samples')
    fields, relative = original_socket_fields(capture.get('socket_begin', ''))
    points = [local_socket(row['gun'], row['world']) for row in rows]
    center = [statistics.median(point[axis] for point in points)
              for axis in range(3)]
    drift = max(math.dist(point, center) for point in points)
    result = {'schema': 1,
              'source': 'owned original retail Lancer Magazine socket and same-tick gun matrices',
              'capture_pid': capture.get('game_pid'), 'samples': len(rows),
              'socket_bone': fields['bone'], 'socket_relative_raw': relative,
              'socket_in_gun_raw': center,
              'maximum_socket_local_drift_raw': drift}
    if partition is not None:
        if (partition.get('source_path') !=
                "SkeletalMesh'COG_AssaultRifle.Mesh.COGAssaultRifle'" or
                partition.get('target_bone', partition.get('bone')) != 'b_AR_Magazine'):
            raise ValueError('Lancer partition identity changed')
        pivot = partition['mesh_space_reference_pivot_candidate_raw']
        if len(pivot) != 3 or not all(math.isfinite(value) for value in pivot):
            raise ValueError('Lancer reference bone pivot invalid')
        predicted = [a + b for a, b in zip(pivot, relative)]
        result['static_reference_plus_socket_raw'] = predicted
        result['runtime_to_static_candidate_distance_raw'] = math.dist(center, predicted)
    with_bone = ['bone' in row for row in rows]
    if any(with_bone):
        if not all(with_bone) or int(fields.get('bone_index', -1)) != 8:
            raise ValueError('Original Lancer magazine bone evidence incomplete')
        pivots = [local_socket(row['gun'], row['bone'][12:15]) for row in rows]
        pivot = [statistics.median(point[axis] for point in pivots)
                 for axis in range(3)]
        delta = [center[axis] - pivot[axis] for axis in range(3)]
        distance = math.dist(center, pivot)
        if not 1 < distance < 100:
            raise ValueError('Original Lancer socket/bone separation invalid')
        result.update(bone_index=8, bone_pivot_in_gun_raw=pivot,
                      maximum_bone_pivot_local_drift_raw=max(
                          math.dist(point, pivot) for point in pivots),
                      socket_from_pivot_in_gun_raw=delta,
                      pivot_to_socket_distance_raw=distance,
                      pivot_to_socket_axis_in_gun=[value / distance for value in delta],
                      socket_offset_length_error_raw=abs(distance - math.dist(relative, [0, 0, 0])))
        if partition is not None:
            result['runtime_to_static_pivot_distance_raw'] = math.dist(
                pivot, partition['mesh_space_reference_pivot_candidate_raw'])
    return result


def summarize_reload(capture):
    rows = original_rows(capture, 'socket_reload', 'socket_reload_samples')
    fields, relative = original_socket_fields(capture.get('socket_reload_begin', ''))
    first_reload = next((i for i, row in enumerate(rows) if row['reloading']), None)
    if (first_reload is None or first_reload < 2 or
            not any(not row['reloading'] for row in rows[first_reload + 1:])):
        raise ValueError('Original Lancer reload sequence not bounded by idle/finished')
    points = [local_socket(row['gun'], row['world']) for row in rows]
    idle = points[:first_reload]
    center = [statistics.median(point[axis] for point in idle)
              for axis in range(3)]
    idle_drift = max(math.dist(point, center) for point in idle)
    maximum = max(range(first_reload, len(rows)),
                  key=lambda i: math.dist(points[i], center))
    # The original AmmoControl hides/scales the skinned magazine near this
    # interval; capture the first large socket departure without treating it
    # as a player insertion direction.
    departure = next((i for i in range(first_reload, len(rows))
                      if math.dist(points[i], center) >= 5), None)
    if departure is None:
        raise ValueError('Original Lancer magazine socket did not depart')
    return {'schema': 1,
            'source': 'owned original retail Lancer Magazine socket during original reload',
            'capture_pid': capture.get('game_pid'), 'samples': len(rows),
            'socket_bone': fields['bone'], 'socket_relative_raw': relative,
            'idle_socket_in_gun_raw': center,
            'maximum_idle_socket_local_drift_raw': idle_drift,
            'first_large_departure_elapsed_seconds': rows[departure]['time'] - rows[departure]['start'],
            'first_large_departure_delta_raw': [points[departure][i] - center[i] for i in range(3)],
            'maximum_departure_elapsed_seconds': rows[maximum]['time'] - rows[maximum]['start'],
            'maximum_departure_delta_raw': [points[maximum][i] - center[i] for i in range(3)],
            'axis_ranges_raw': [[min(point[i] for point in points),
                                 max(point[i] for point in points)] for i in range(3)],
            'returned_socket_in_gun_raw': points[-1],
            'socket_not_fixed_during_original_reload': True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--partition', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    partition = json.loads(args.partition.read_text(encoding='utf-8')) if args.partition else None
    capture = json.loads(args.capture.read_text(encoding='utf-8'))
    result = (summarize_reload(capture) if capture.get('mode') == 'socket_reload'
              else summarize(capture, partition))
    value = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.write_text(value, encoding='utf-8')
    else:
        print(value, end='')


if __name__ == '__main__':
    main()
