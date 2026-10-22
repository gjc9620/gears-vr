"""Summarize the bounded read-only hand probe; never launches or controls a game.

Build GEARS_PLAYGROUND_FREE_HANDS_PROBE=ON, explicitly start/stop the three
PlaygroundFreeHandsProbe exports in an owned test, then pass its PID log here.
Original hand matrices stay in ignored analysis/, not in the returned summary.
"""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PHASES = ('disabled', 'invalid_player', 'unproved_animation_state',
          'original_animation', 'missing_tracking', 'waiting_new_pose',
          'controller_targets')


def fields(line):
    items = [item.split('=', 1) for item in line.split()[1:]]
    if any(len(item) != 2 for item in items) or len(dict(items)) != len(items):
        raise ValueError('Malformed or repeated hand probe field')
    return dict(items)


def vector(value, length):
    result = [float(item) for item in value.split(',')]
    if len(result) != length or not all(math.isfinite(item) for item in result):
        raise ValueError('Invalid hand pose/matrix')
    return result


def pose_row(line):
    row = fields(line)
    integers = ('sample', 'qpc', 'ms', 'generation', 'session', 'sequence',
                'space_epoch', 'input_epoch', 'world_generation', 'pose_ms', 'phase', 'tracked_mask',
                'eligible_mask', 'apply_mask', 'state_known', 'fixed_mask',
                'special_move', 'bones_known')
    vectors = ('left', 'left_rotation', 'right', 'right_rotation')
    matrices = ('mesh', 'hand_left', 'hand_right')
    optional = set(matrices) if row.get('bones_known') == '1' else set()
    if set(row) != set(integers + vectors + ('units_per_metre', 'weapon')) | optional:
        raise ValueError('Unexpected hand probe fields')
    for key in integers:
        row[key] = int(row[key])
        if row[key] < 0:
            raise ValueError('Negative hand probe counter')
    for key in vectors:
        row[key] = vector(row[key], 3)
    for key in optional:
        row[key] = vector(row[key], 16)
    row['units_per_metre'] = float(row['units_per_metre'])
    row['weapon'] = int(row['weapon'], 16)
    if (row['phase'] >= len(PHASES) or row['tracked_mask'] > 3 or
            row['eligible_mask'] & ~row['tracked_mask'] or row['apply_mask'] or
            row['state_known'] > 15 or row['fixed_mask'] > 15 or
            row['special_move'] > 255 or row['bones_known'] > 1 or
            not row['generation']):
        raise ValueError('Hand probe violates the read-only/ownership contract')
    if row['tracked_mask'] and not all(row[key] for key in
                                     ('session', 'sequence', 'space_epoch', 'input_epoch',
                                      'world_generation', 'pose_ms')):
        raise ValueError('Tracked hand is missing its XR ticket')
    if row['tracked_mask'] and not (10 <= row['units_per_metre'] <= 500 and
                                   0 <= row['ms'] - row['pose_ms'] <= 100):
        raise ValueError('Tracked hand has a stale/future pose or invalid world scale')
    # The live candidate cannot yet certify full pickup/cancel/end ownership.
    if row['eligible_mask'] or row['phase'] == 6:
        raise ValueError('Unproved live hand control was reported as eligible')
    return row


def summarize(text):
    rows, begin, end = [], None, None
    for line in text.splitlines():
        if line.startswith('free_hand_probe_begin '):
            begin, rows, end = fields(line), [], None
            if any(begin.get(key) != '0' for key in
                   ('ownership_proved', 'apply_mask', 'writes')):
                raise ValueError('Probe did not start read-only')
        elif line.startswith('free_hand_pose '):
            if begin is None or end is not None:
                raise ValueError('Hand sample outside the current probe')
            row = pose_row(line)
            if row['sample'] != len(rows) or len(rows) >= 160:
                raise ValueError('Noncontiguous or excessive hand samples')
            if rows and (row['ms'] < rows[-1]['ms'] or row['qpc'] < rows[-1]['qpc']):
                raise ValueError('Hand sample clock went backwards')
            rows.append(row)
        elif line.startswith('free_hand_probe_end '):
            end = fields(line)
    if begin is None or end is None or not rows or end.get('writes') != '0':
        raise ValueError('Missing complete read-only probe envelope')
    if int(end['samples']) != len(rows) or int(end['status']) not in (3, 4, 5):
        raise ValueError('Probe completion/count mismatch')
    if int(end['status']) == 3 and len(rows) != 160:
        raise ValueError('Sample-limit completion requires all 160 rows')
    return dict(schema=1, source='native_GVE6_world_pose_bridge', samples=len(rows),
                completed=int(end['status']) == 3, stop_reason=end['reason'],
                xr_samples=sum(bool(row['tracked_mask']) for row in rows),
                left_samples=sum(bool(row['tracked_mask'] & 1) for row in rows),
                right_samples=sum(bool(row['tracked_mask'] & 2) for row in rows),
                original_bone_samples=sum(row['bones_known'] for row in rows),
                phases=dict(Counter(PHASES[row['phase']] for row in rows)),
                fixed_masks=sorted({row['fixed_mask'] for row in rows}),
                writes=0, apply_mask=0, ownership_proved=False,
                visible_hands_proved=False, physical_controller_source_proved=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Keep hand probe evidence under this worktree analysis/')
    content = args.log.read_bytes()
    report = summarize(content.decode('utf-8-sig'))
    report.update(log=str(args.log.resolve()), log_sha256=hashlib.sha256(content).hexdigest())
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
