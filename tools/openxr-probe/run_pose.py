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
    parser.add_argument('--seconds', type=int, default=12, choices=range(3, 181), metavar='3..180')
    parser.add_argument('--render', action='store_true')
    parser.add_argument('--runtime', type=pathlib.Path)
    parser.add_argument('--bridge-pid', type=int, help='Publish poses to the bounded native-camera test')
    parser.add_argument('--game-screen',action='store_true',help='Display only the game HWND as a mono diagnostic panel')
    parser.add_argument('--gpu-game',action='store_true',help='Display native shared GPU frames as a mono diagnostic panel')
    parser.add_argument('--gpu-snapshot',action='store_true',help='Save one XR GPU image after 10 seconds; requires --gpu-game')
    args = parser.parse_args()
    if args.bridge_pid and (args.bridge_pid<=0 or not args.render): parser.error('Bridge requires a positive target PID and --render')
    if not args.bridge_pid and args.seconds>60: parser.error('Standalone diagnostics are limited to 60 seconds')
    if args.game_screen and (args.render or args.bridge_pid): parser.error('Game screen is a separate display test')
    if args.gpu_game and (args.game_screen or args.render or args.bridge_pid): parser.error('GPU game is a separate display test')
    if args.gpu_snapshot and (not args.gpu_game or args.seconds<12): parser.error('Snapshot requires GPU game and at least 12 seconds')
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
    snapshot=pathlib.Path(str(output)+'-gpu.ppm') if args.gpu_snapshot else None
    if snapshot: command.extend(['--gpu-snapshot',str(snapshot)])
    if args.bridge_pid: command.extend(['--bridge-pid',str(args.bridge_pid)])
    game_target=None
    if args.game_screen or args.gpu_game:
        sys.path.insert(0,str(ROOT/'tools/game-io'))
        from game_io import game_window
        pid,hwnd=game_window()
        game_target={'pid':pid,'hwnd':hwnd}
        if args.gpu_game: command.extend(['--gpu-game-pid',str(pid)])
        else: command.extend(['--game-window',str(hwnd),str(pid)])
    timeout = args.seconds + 20
    report = {'csv': str(output), 'stdout': str(stdout), 'stderr': str(stderr),
              'runtime_override': env.get('XR_RUNTIME_JSON'), 'render_pattern': args.render,
              'timeout_seconds': timeout}
    report['bridge_pid']=args.bridge_pid
    report['game_screen_target']=game_target
    report['native_gpu_game']=args.gpu_game
    report['gpu_snapshot_requested']=args.gpu_snapshot
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
    if snapshot: report['gpu_snapshot_saved']=snapshot.is_file();report['gpu_snapshot_path']=str(snapshot)
    if stdout.is_file():
        with stdout.open('rb') as stream:
            stream.seek(max(0,stdout.stat().st_size-32768))
            for line in stream.read().decode('utf-8',errors='replace').splitlines():
                try: counters=json.loads(line)
                except ValueError: continue
                if isinstance(counters,dict) and 'game_quad_layers' in counters: report['game_screen_summary']=counters
                if isinstance(counters,dict) and 'gpu_game_quad_layers' in counters: report['gpu_game_summary']=counters
                if isinstance(counters,dict) and 'swapchain_formats' in counters: report['swapchain_formats']=counters['swapchain_formats']
    stem.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    return 0 if report.get('exit_code') == 0 and not report.get('csv_error') and (not snapshot or report['gpu_snapshot_saved']) else 1


if __name__ == '__main__': sys.exit(main())
