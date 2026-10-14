"""Run the bounded A/B/A quad control with phase-synchronized window snapshots.

Requires a fresh game with this exact DLL reserved, already in a verified level.
Phase events describe CPU work; QPC bounds and visible contents require review.
No input is sent. The existing capture worker restores hooks automatically.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import pathlib
import shutil
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/game-io'))
from game_io import game_window, snapshot

k = c.WinDLL('kernel32', use_last_error=True)
k.OpenEventW.argtypes = [w.DWORD, w.BOOL, w.LPCWSTR]
k.OpenEventW.restype = w.HANDLE
k.WaitForSingleObject.argtypes = [w.HANDLE, w.DWORD]
k.WaitForSingleObject.restype = w.DWORD
k.CloseHandle.argtypes = [w.HANDLE]
k.QueryPerformanceCounter.argtypes = [c.POINTER(c.c_longlong)]


def qpc():
    value = c.c_longlong()
    if not k.QueryPerformanceCounter(c.byref(value)):
        raise c.WinError(c.get_last_error())
    return value.value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', required=True, type=int)
    parser.add_argument('--build-dir', required=True, type=pathlib.Path)
    parser.add_argument('--dll-name', default='GearsCaptureQuadControl.dll')
    parser.add_argument('--source', choices=('left', 'right'), default='left',
                        help='Source half to sample in B; modifies only the opposite eye quad')
    args = parser.parse_args()
    target = game_window()
    if target[0] != args.pid:
        raise RuntimeError('Game PID does not match the requested experiment')
    build = args.build_dir.resolve()
    prefix = 'quad-source-control' if args.source == 'left' else 'quad-right-source-control'
    stem = ROOT / 'analysis/runtime' / f'{prefix}-{args.pid}'
    report_path = stem.with_name(stem.name + '-shots.json')
    if report_path.exists():
        raise RuntimeError('This PID already has an experiment record; do not overwrite it')
    report = {'pid': args.pid, 'source': args.source, 'modified_eye': 1 if args.source == 'left' else 0,
              'shots': [], 'error': None,
              'limits': 'CPU phase events are not proof of displayed GPU content.'}
    handles = {}
    activated = False
    try:
        command = [sys.executable, str(ROOT / 'tools/native-capture/start_capture.py'),
                   '--pid', str(args.pid), '--seconds', '25', '--build-dir', str(build),
                   '--dll-name', args.dll_name, '--use-reserved', '--submit-pair',
                   '--submit-pair-seconds', '10', '--submit-background',
                   '--submit-output-bounds', '--viewport-diagnostics', f'--quad-{args.source}-source-control']
        with stem.with_name(stem.name + '-start.log').open('w') as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        activated = True
        deadline = time.monotonic() + 18
        for phase in ('A1', 'B', 'A2'):
            handle = k.OpenEventW(0x100000, False,
                                  f'Local\\GearsVR.Capture.QuadSource.{args.pid}.{phase}')
            if not handle:
                raise c.WinError(c.get_last_error())
            handles[phase] = handle

        def phases():
            return [name for name, handle in handles.items() if k.WaitForSingleObject(handle, 0) == 0]

        for phase in ('A1', 'B', 'A2'):
            while phases() != [phase]:
                if time.monotonic() > deadline or game_window() != target:
                    raise RuntimeError(f'Phase {phase} not reached on the same game window')
                time.sleep(.02)
            time.sleep(.6)
            before = phases()
            started = qpc()
            shot = snapshot(f'{prefix}-{args.pid}-{phase}', 960)
            ended = qpc()
            after = phases()
            valid = before == after == [phase] and shot['pid'] == args.pid and game_window() == target
            report['shots'].append({'phase': phase, 'qpc_start': started, 'qpc_end': ended,
                                    'phase_before': before, 'phase_after': after,
                                    'phase_consistent': valid, 'frame': shot})
            report_path.write_text(json.dumps(report, indent=2), encoding='utf-8')
            print(json.dumps({'phase': phase, 'phase_consistent': valid, 'image': shot['image']}), flush=True)
            if not valid:
                raise RuntimeError('Screenshot crossed a phase boundary; experiment needs review')
    except Exception as error:
        report['error'] = str(error)
        if not activated:
            raise
        print(json.dumps({'capture_error': str(error), 'collecting_cleanup': True}), flush=True)
    finally:
        for handle in handles.values():
            k.CloseHandle(handle)
        report_path.write_text(json.dumps(report, indent=2), encoding='utf-8')

    # The CRT status file remains locked until capture and cleanup are complete.
    deadline = time.monotonic() + 35
    while True:
        try:
            status = (build / 'capture-status.txt').read_text()
        except (OSError, UnicodeError):
            status = ''
        if f'capturing pid={args.pid} ' in status and 'stopped disable_status=0 active=0' in status:
            break
        if time.monotonic() > deadline:
            raise RuntimeError('Clean stop not confirmed; inspect capture status before further work')
        time.sleep(.2)
    shutil.copyfile(build / 'capture-status.txt', stem.with_suffix('.status.txt'))
    shutil.copyfile(build / 'capture.csv', stem.with_suffix('.csv'))
    restored = stem.with_name(stem.name + '-restored.json')
    subprocess.run([sys.executable, str(ROOT / 'tools/native-capture/verify_stopped.py'),
                    '--pid', str(args.pid), '--status', str(stem.with_suffix('.status.txt')),
                    '--output', str(restored)], check=True)
    subprocess.run([sys.executable, str(ROOT / 'tools/native-capture/analyze_submit_duration.py'),
                    '--capture', str(stem.with_suffix('.json')), '--restored', str(restored),
                    '--output', str(stem.with_name(stem.name + '-lifecycle.json'))], check=True)
    if report['error']:
        raise RuntimeError(report['error'])


if __name__ == '__main__':
    main()
