"""Run the pose/pattern diagnostic with a total deadline and compact evidence."""
import argparse
import csv
import datetime
import json
import math
import os
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]


def summarize(path):
    rows = list(csv.DictReader(path.open(newline='', encoding='utf-8')))
    keys = ('head_x', 'head_y', 'head_z', 'qx', 'qy', 'qz', 'qw')
    poses = [tuple(float(r[k]) for k in keys) for r in rows]
    finite = [p for p in poses if all(math.isfinite(v) for v in p)]
    tracked = [(int(r['head_flags']) & 15) == 15 for r in rows]
    non_default_tracked = sum(ok and p != (0, 0, 0, 0, 0, 0, 1)
                              and all(math.isfinite(v) for v in p)
                              for ok, p in zip(tracked, poses))
    longest_untracked = 0
    run_start = None
    for row, ok in zip(rows, tracked):
        stamp = int(row['predicted_time'])
        if not ok:
            if run_start is None: run_start = stamp
            longest_untracked = max(longest_untracked, stamp - run_start)
        else: run_start = None
    return {
        'frames': len(rows), 'finite_poses': len(finite),
        'fully_valid_and_tracked': sum(tracked),
        'non_default_tracked_frames': non_default_tracked,
        'longest_untracked_seconds': round(longest_untracked / 1e9, 3),
        'unique_head_poses': len(set(finite)),
        'head_ranges': {k: [min(p[i] for p in finite), max(p[i] for p in finite)]
                        for i, k in enumerate(keys)} if finite else {},
        'live_tracking_verified': False,
        'interpretation': 'Motion must be compared with deliberate headset movement; flags alone do not prove live tracking.',
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=int, default=12, choices=range(3, 61), metavar='3..60')
    parser.add_argument('--render', action='store_true')
    parser.add_argument('--runtime', type=pathlib.Path)
    args = parser.parse_args()
    env = os.environ.copy()
    if args.runtime:
        if not args.runtime.is_file(): parser.error('Runtime manifest not found')
        env['XR_RUNTIME_JSON'] = str(args.runtime.resolve())
    folder = ROOT / 'analysis/openxr'
    folder.mkdir(parents=True, exist_ok=True)
    stem = folder / ('pose-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    output = stem.with_suffix('.csv')
    stdout, stderr = stem.with_suffix('.stdout.txt'), stem.with_suffix('.stderr.txt')
    command = [str(pathlib.Path(__file__).parent / 'build/OpenXRPoseProbe.exe'), str(args.seconds), str(output)]
    if args.render: command.append('--render')
    timeout = args.seconds + 20
    report = {'csv': str(output), 'stdout': str(stdout), 'stderr': str(stderr),
              'runtime_override': env.get('XR_RUNTIME_JSON'), 'render_pattern': args.render,
              'timeout_seconds': timeout}
    # Files prevent unbounded runtime messages consuming model context or RAM.
    with stdout.open('wb') as out, stderr.open('wb') as err:
        try:
            result = subprocess.run(command, env=env, stdout=out, stderr=err, timeout=timeout)
            report['exit_code'] = result.returncode
        except subprocess.TimeoutExpired:
            report['error'] = 'Total deadline exceeded; diagnostic child terminated'
        except OSError as error:
            report['error'] = str(error)
    if output.is_file():
        try: report['pose_summary'] = summarize(output)
        except (ValueError, KeyError) as error: report['csv_error'] = str(error)
    stem.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    return 0 if report.get('exit_code') == 0 and not report.get('csv_error') else 1


if __name__ == '__main__': sys.exit(main())
