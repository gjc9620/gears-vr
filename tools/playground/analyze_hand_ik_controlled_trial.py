"""Recompute bounded right-hand IK bone response from a private trial log.

This analyzes GetBoneMatrix output in one owned game instance. It cannot
establish rendered, headset-visible, rotating or continuously driven hands.
The source log and derived receipt belong under ignored analysis/.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path

from hand_ik_controlled_trial_run import parse_end


SEQUENCE = ('S', '+X', '-X', '-X', '+X', 'S')
PHASES = ('before', 'written', 'pre_restore', 'restored')


def fields(line, prefix):
    if not line.startswith(prefix):
        raise ValueError('Unexpected trial log row')
    result = {}
    for part in line[len(prefix):].split():
        name, value = part.split('=', 1)
        if name == 'mesh' and name in result and 'mesh_object' not in result:
            result['mesh_object'] = result['mesh']
        elif name in result:
            raise ValueError('Duplicate trial log field')
        result[name] = value
    return result


def position(row, hand):
    matrix = tuple(float(value) for value in row[hand].split(','))
    if len(matrix) != 16 or not all(math.isfinite(value) for value in matrix):
        raise ValueError('Expected a 4x4 bone matrix')
    return matrix[12:15]


def unique_ticks(samples, summary):
    first, last = int(summary['first_sample']), int(summary['last_sample'])
    if first > last:
        raise ValueError('Reversed natural interval')
    values = []
    for index in range(first, last + 1):
        row = samples.get(index)
        if row is None or row['phase'] not in ('pre_natural', 'post_natural', 'restored'):
            raise ValueError('Missing natural sample')
        if row['mesh_tick_known'] != '1':
            raise ValueError('Natural mesh Tick unknown')
        tick = int(row['mesh_tick'])
        if values and tick == int(values[-1]['mesh_tick']):
            continue
        elif any(tick == int(value['mesh_tick']) for value in values):
            raise ValueError('Natural mesh Tick moved backwards')
        else:
            values.append(row)
    if len(values) < 8 or int(summary['distinct_mesh_ticks']) < 8 or summary['complete'] != '1':
        raise ValueError('Incomplete natural Tick interval')
    return values


def interpolate_x(earlier, later, at_qpc, hand='right'):
    t0, t1 = int(earlier['qpc']), int(later['qpc'])
    if not t0 < at_qpc < t1:
        raise ValueError('No natural Tick samples bracketing the QPC target')
    x0, x1 = position(earlier, hand)[0], position(later, hand)[0]
    return x0 + (x1 - x0) * (at_qpc - t0) / (t1 - t0)


def natural_x_at(ticks, at_qpc):
    for earlier, later in zip(ticks, ticks[1:]):
        if int(earlier['qpc']) <= at_qpc <= int(later['qpc']):
            if at_qpc == int(earlier['qpc']):
                return position(earlier, 'right')[0]
            if at_qpc == int(later['qpc']):
                return position(later, 'right')[0]
            return interpolate_x(earlier, later, at_qpc)
    return None


def matched_span_controls(ticks, before_gap, after_gap):
    """Leave out each natural center and interpolate the same pulse span.

    Both endpoints come from their own side of the center. This avoids using
    the tested sample to manufacture a zero residual. Every source Tick is
    from one continuous, no-write interval.
    """
    residuals = []
    for index in range(2, len(ticks) - 2):
        center = ticks[index]
        at = int(center['qpc'])
        left_x = natural_x_at(ticks[:index], at - before_gap)
        right_x = natural_x_at(ticks[index + 1:], at + after_gap)
        if left_x is None or right_x is None:
            continue
        estimated = left_x + (right_x - left_x) * before_gap / (before_gap + after_gap)
        residuals.append(position(center, 'right')[0] - estimated)
    return residuals


def analyze(raw):
    log = raw.decode('utf-8-sig')
    terminal = parse_end(log)
    samples, phases, natural, windows = {}, {}, {}, {}
    identity = None
    last_qpc = -1
    for line in log.splitlines():
        if line.startswith('hand_ik_controlled_trial_sample '):
            row = fields(line, 'hand_ik_controlled_trial_sample ')
            index, window, qpc = int(row['sample']), int(row['window']), int(row['qpc'])
            if index in samples or window not in range(6) or qpc <= last_qpc:
                raise ValueError('Duplicate, invalid or non-monotonic trial sample')
            last_qpc = qpc
            keys = ('pc', 'pawn', 'mesh_object', 'tree', 'weapon', 'control')
            current_identity = tuple(row[key] for key in keys)
            if identity is None:
                identity = current_identity
            elif current_identity != identity:
                raise ValueError('Game/control identity changed during trial')
            samples[index] = row
            if row['phase'] in PHASES:
                key = (window, row['phase'])
                if key in phases:
                    raise ValueError('Duplicate pulse phase')
                phases[key] = row
        elif line.startswith('hand_ik_controlled_trial_natural '):
            row = fields(line, 'hand_ik_controlled_trial_natural ')
            key = (int(row['window']), row['side'])
            if key in natural:
                raise ValueError('Duplicate natural summary')
            natural[key] = row
        elif line.startswith('hand_ik_controlled_trial_window '):
            row = fields(line, 'hand_ik_controlled_trial_window ')
            window = int(row['window'])
            if window in windows:
                raise ValueError('Duplicate trial window')
            windows[window] = row
    if len(windows) != 6 or len(phases) != 24 or len(natural) != 12 or not samples:
        raise ValueError('Six complete pulse windows are required')

    if (terminal['status'] != 5 or terminal['completed_windows'] != 6 or
            terminal['evidence_complete'] != 1 or terminal['restored'] != 1 or
            terminal['setter_calls'] != 12 or terminal['effector_writes'] != 12 or
            terminal['effect_evidence'] != 0 or terminal['inconclusive'] != 1 or
            terminal['safe_to_unload'] != 1):
        raise ValueError('Incomplete or non-restored controlled trial')
    results = []
    matched_control_residuals = []
    matched_control_counts = []
    natural_segments = [unique_ticks(samples, natural[0, 'before'])]
    for index in range(6):
        post = unique_ticks(samples, natural[index, 'after'])
        pulse_tick = int(phases[index, 'pre_restore']['mesh_tick'])
        fresh = [row for row in post if int(row['mesh_tick']) > pulse_tick]
        if len(fresh) < 8:
            raise ValueError('Insufficient fresh no-write recovery Ticks')
        natural_segments.append(fresh)
    for window, label in enumerate(SEQUENCE):
        marker = windows[window]
        if marker['label'] != label or marker['evidence_complete'] != '1':
            raise ValueError('Window order or evidence changed')
        before, written, pulse, restored = (phases[window, phase] for phase in PHASES)
        if any(row['label'] != label for row in (before, written, pulse, restored)):
            raise ValueError('Sample label differs from fixed sequence')
        if not (int(before['qpc']) < int(written['qpc']) < int(pulse['qpc']) <
                int(restored['qpc'])):
            raise ValueError('Pulse phase time order changed')
        if (before['mesh'] != pulse['mesh'] or before['mesh'] != restored['mesh'] or
                before['effector'] != restored['effector'] or
                before['strength'] != '0' or restored['strength'] != '0'):
            raise ValueError('Mesh or owned right-hand fields did not restore')
        if not (0.099 <= float(written['strength']) <= 0.101 and
                0.099 <= float(pulse['strength']) <= 0.101 and
                int(before['mesh_tick']) != int(pulse['mesh_tick'])):
            raise ValueError('Pulse strength or mesh update missing')
        baseline = unique_ticks(samples, natural[window, 'before'])
        recovery = unique_ticks(samples, natural[window, 'after'])
        if any(row['mesh_tick_known'] != '1' or row['action_idle'] != '1'
               for row in (before, written, pulse, restored)):
            raise ValueError('Pulse mesh Tick or idle action was not proved')
        pulse_qpc = int(pulse['qpc'])
        next_tick = next((row for row in recovery
                          if int(row['mesh_tick']) != int(pulse['mesh_tick'])), None)
        if next_tick is None:
            raise ValueError('No fresh mesh Tick after restoring the pulse')
        later_ticks = [row for row in recovery
                       if int(row['mesh_tick']) > int(pulse['mesh_tick'])]
        if len(later_ticks) < 3 or later_ticks[0] is not next_tick:
            raise ValueError('Missing independently observed recovery Ticks')
        second_tick, third_tick = later_ticks[1:3]
        before_gap = pulse_qpc - int(before['qpc'])
        after_gap = int(next_tick['qpc']) - pulse_qpc
        if before_gap <= 0 or after_gap <= 0:
            raise ValueError('Pulse was not bracketed by natural mesh Ticks')
        predicted = interpolate_x(before, next_tick, pulse_qpc)
        left_predicted = interpolate_x(before, next_tick, pulse_qpc, 'left')
        first_recovery_prediction = interpolate_x(before, second_tick,
                                                   int(next_tick['qpc']))
        second_recovery_prediction = interpolate_x(before, third_tick,
                                                    int(second_tick['qpc']))
        controls = [residual for segment in natural_segments
                    for residual in matched_span_controls(segment, before_gap, after_gap)]
        if not controls:
            raise ValueError('No same-span natural control samples')
        matched_control_residuals.extend(controls)
        matched_control_counts.append(len(controls))
        recovery_ms = (float(second_tick['world_time']) -
                       float(restored['world_time'])) * 1000
        if not math.isfinite(recovery_ms) or not 0 < recovery_ms < 100:
            raise ValueError('Second recovery Tick exceeded the bounded trial')
        results.append({
            'window': window, 'label': label,
            'before_mesh_tick': int(before['mesh_tick']),
            'pulse_mesh_tick': int(pulse['mesh_tick']),
            'next_mesh_tick': int(next_tick['mesh_tick']),
            'raw_right_x_delta': position(pulse, 'right')[0] - position(before, 'right')[0],
            'right_x_residual': position(pulse, 'right')[0] - predicted,
            'left_x_residual': position(pulse, 'left')[0] - left_predicted,
            'first_recovery_right_x_residual':
                position(next_tick, 'right')[0] - first_recovery_prediction,
            'second_recovery_right_x_residual':
                position(second_tick, 'right')[0] - second_recovery_prediction,
            'second_recovery_after_rollback_ms': recovery_ms,
            'baseline_distinct_mesh_ticks': int(natural[window, 'before']['distinct_mesh_ticks']),
            'recovery_distinct_mesh_ticks': int(natural[window, 'after']['distinct_mesh_ticks']),
            'matched_control_count': len(controls),
        })
    sham_noise = max(abs(results[index]['right_x_residual']) for index in (0, 5))
    natural_noise = max(abs(value) for value in matched_control_residuals)
    threshold = max(0.03, 5 * max(natural_noise, sham_noise))
    directional = (all(results[index]['right_x_residual'] > threshold for index in (1, 4)) and
                   all(results[index]['right_x_residual'] < -threshold for index in (2, 3)))
    first_recovery_within_noise = all(abs(row['first_recovery_right_x_residual']) <=
                                      max(natural_noise, sham_noise) for row in results)
    second_recovery_within_noise = all(abs(row['second_recovery_right_x_residual']) <=
                                       max(natural_noise, sham_noise) for row in results)
    return {
        'schema': 1, 'source_sha256': hashlib.sha256(raw).hexdigest(),
        'method': 'before_and_first_fresh_post_restore_tick_linear_qpc_interpolation',
        'natural_control_method': 'matched_span_leave_one_out_within_no_write_intervals',
        'recovery_method': 'first_tick_bracketed_by_before_and_second;_second_tick_bracketed_by_before_and_third',
        'natural_sample_count': sum(map(len, natural_segments)),
        'natural_noise': natural_noise, 'sham_noise': sham_noise,
        'matched_control_count': sum(matched_control_counts),
        'threshold': threshold, 'windows': results, 'terminal': terminal,
        'directional_response': directional,
        'first_recovery_within_control_noise': first_recovery_within_noise,
        'second_recovery_within_control_noise': second_recovery_within_noise,
        'current_instance_right_hand_bone_response_candidate': directional and second_recovery_within_noise,
        'xr_continuous_hands_proved': False, 'headset_visibility_proved': False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_text(json.dumps(analyze(args.log.read_bytes()), indent=2) + '\n',
                           encoding='utf-8')


if __name__ == '__main__':
    main()
