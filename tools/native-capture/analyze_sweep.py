"""Validate a completed six-axis capture without touching the game."""
import argparse
import collections
import csv
import json
import math
import pathlib
import sys


def analyze(path):
    groups = collections.defaultdict(list)
    unexpected_edits = 0
    with path.open(newline='', encoding='utf-8') as stream:
        for row in csv.DictReader(stream):
            modified = bool(int(row['valid']) & 64)
            if modified and (row['focused'] != '1' or
                             (row['kind'] == '2' and int(row['caller_rva'], 16) != 0x15D0CA)):
                unexpected_edits += 1
            if row['kind'] == '1': groups[int(row['pose_phase'])].append(row)
    report = {'unexpected_edits': unexpected_edits, 'phases': {}}
    axes = ('x', 'y', 'z', 'pitch', 'rot_yaw', 'roll')
    all_rows = [r for group in groups.values() for r in group]
    if not all_rows: raise ValueError('No scene-view rows found')
    report['source_ranges'] = {
        a: [min(float(r['source_'+a]) for r in all_rows),
            max(float(r['source_'+a]) for r in all_rows)] for a in axes}
    for phase, rows in sorted(groups.items()):
        errors = [abs(float(r['yaw' if a == 'rot_yaw' else a]) -
                      float(r['source_'+a]) - float(r['delta_'+a])) for r in rows for a in axes]
        finite_matrices = all(math.isfinite(float(r['matrix'+str(i)])) for r in rows for i in range(32))
        report['phases'][phase] = {
            'samples': len(rows), 'modified': sum(bool(int(r['valid']) & 64) for r in rows),
            'view_matrices': len({tuple(r['matrix'+str(i)] for i in range(16)) for r in rows}),
            'projection_matrices': len({tuple(r['matrix'+str(i)] for i in range(16, 32)) for r in rows}),
            'finite_matrices': finite_matrices, 'max_arithmetic_error': max(errors),
            'delta_ranges': {a: [min(float(r['delta_'+a]) for r in rows),
                                 max(float(r['delta_'+a]) for r in rows)] for a in axes},
        }
    def matrices(phase):
        return {tuple(r['matrix'+str(i)] for i in range(32)) for r in groups.get(phase, [])}
    checks = {
        'all_phases_present': set(groups) == set(range(8)),
        'no_unfocused_or_other_event_caller_edits': unexpected_edits == 0,
        'finite_matrices': all(p['finite_matrices'] for p in report['phases'].values()),
        'offset_arithmetic': all(p['max_arithmetic_error'] < 0.0001 for p in report['phases'].values()),
        'all_axes_changed_view': all(report['phases'].get(p, {}).get('view_matrices', 0) > 1 and
                                     report['phases'].get(p, {}).get('modified', 0) > 0 for p in range(1, 7)),
        'projection_unchanged': len({tuple(r['matrix'+str(i)] for i in range(16, 32)) for r in all_rows}) == 1,
        'baseline_and_recovery_unmodified': all(not (int(r['valid']) & 64) for p in (0, 7) for r in groups[p]),
        'recovery_matches_baseline': len(matrices(0)) == 1 and matrices(0) == matrices(7),
    }
    report['checks'] = checks
    report['passed'] = all(checks.values())
    report['limits'] = 'This stationary sweep checks logged view changes; it does not prove live head tracking, scene geometry, stereo or all camera paths.'
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', type=pathlib.Path)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    args = parser.parse_args()
    report = analyze(args.csv)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'passed': report['passed'], 'checks': report['checks'], 'output': str(args.output)}, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__': sys.exit(main())
