"""Run bounded copy, mask, sustained view-union, or projection controls.

Requires a fresh, reserved diagnostic DLL and a visually verified loaded level.
Window images are timing-checked observations, not intermediate GPU readbacks.
"""
import argparse
import csv
import json
import math
import pathlib
import re
import shutil
import subprocess
import sys
import time

from run_quad_source_control import ROOT, game_window, snapshot, qpc, k
from run_stereo_probe import validate_native_source_records


def projection_residuals(by_stage):
    """Compare changed phases with observed A-phase CPU jitter, not a guessed bound."""
    checks = []
    try:
        for eye in ('l', 'r'):
            for axis in ('p8', 'p9'):
                minimum, maximum = f'residual_{eye}_{axis}_min', f'residual_{eye}_{axis}_max'
                baseline = [by_stage[(phase, 'prepare_after')] for phase in ('A1', 'A2', 'A3')]
                ranges = [(float(row[minimum]), float(row[maximum])) for row in baseline]
                if not all(math.isfinite(lo) and math.isfinite(hi) and lo <= hi for lo, hi in ranges):
                    return {'passed': False, 'error': 'Invalid baseline residual range', 'checks': checks}
                low = min(lo for lo, _ in ranges)
                high = max(hi for _, hi in ranges)
                for phase in ('B', 'C'):
                    row = by_stage[(phase, 'prepare_after')]
                    changed_low, changed_high = float(row[minimum]), float(row[maximum])
                    # Float32 subtraction/printing allowance; this is not an
                    # engine jitter limit. The engine range comes from A only.
                    allowance = 4 * 2**-23 * max(1., abs(float(row[f'expected_{eye}_{axis}'])))
                    passed = (all(math.isfinite(v) for v in (low, high, changed_low, changed_high, allowance))
                              and low <= high and low-allowance <= changed_low <= changed_high <= high+allowance)
                    checks.append({'phase': phase, 'eye': eye, 'axis': axis, 'baseline': [low, high],
                                   'changed': [changed_low, changed_high], 'rounding_allowance': allowance, 'passed': passed})
    except (KeyError, ValueError, TypeError):
        return {'passed': False, 'error': 'Missing or invalid post-prepare residuals', 'checks': checks}
    return {'passed': len(checks) == 8 and all(check['passed'] for check in checks), 'checks': checks,
            'limits': 'CPU residual containment within measured baseline; visual projection response is verified separately.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--attempt', type=int, default=1)
    parser.add_argument('--build-dir', type=pathlib.Path, required=True)
    parser.add_argument('--dll-name', required=True)
    parser.add_argument('--mode', choices=('copy', 'mask', 'union', 'projection', 'stereo'), required=True)
    parser.add_argument('--trial-seconds', type=int, choices=(30, 120), default=30,
                        help='Bounded stereo duration; 120 requires ticketed GPU stereo')
    parser.add_argument('--stereo-units-per-metre',type=float,default=100.)
    parser.add_argument('--frame-link-diagnostics',action='store_true')
    parser.add_argument('--stereo-gpu-publish',action='store_true')
    parser.add_argument('--simulator-operator',action='store_true',
                        help='Explicit simulated source forwarded by the operator-motion stereo-game runner')
    parser.add_argument('--finalize-only',action='store_true',help='Finish analysis of existing same-PID evidence without activating hooks')
    args = parser.parse_args()
    if args.attempt < 1:parser.error('Attempt must be positive')
    sustained = args.mode in ('union', 'projection', 'stereo')
    if args.frame_link_diagnostics and not sustained:parser.error('Frame association requires a sustained validated pair')
    if args.stereo_gpu_publish and not(args.mode == 'stereo' and args.frame_link_diagnostics):parser.error('Stereo GPU publication requires stereo mode and frame-link diagnostics')
    if args.simulator_operator and not(args.mode == 'stereo' and args.stereo_gpu_publish and args.frame_link_diagnostics):
        parser.error('Simulator operator requires stereo mode, GPU publication and frame-link diagnostics')
    if args.trial_seconds == 120 and not (args.mode == 'stereo' and args.stereo_gpu_publish):
        parser.error('120-second trial requires ticketed GPU stereo')
    target = game_window()
    if target[0] != args.pid:
        raise RuntimeError('Game PID does not match experiment')
    build = args.build_dir.resolve()
    stem = ROOT / 'analysis/runtime' / f'color-{args.mode}-probe-{args.pid}'
    if args.attempt != 1:stem = stem.with_name(stem.name + f'-attempt{args.attempt}')
    report_path = stem.with_name(stem.name + '-shots.json')
    if report_path.exists() and not args.finalize_only:
        raise RuntimeError('Do not overwrite an existing experiment')
    report = {'pid': args.pid, 'mode': args.mode, 'trial_seconds': args.trial_seconds, 'shots': [], 'error': None,
              'simulator_operator': args.simulator_operator, 'simulation_not_physical_tracking': args.simulator_operator,
              'limits': 'Window snapshots and CPU time bounds do not prove intermediate texture pixels.'}
    if args.finalize_only:
        report = json.loads(report_path.read_text(encoding='utf-8'))
        if report.get('pid') != args.pid or report.get('mode') != args.mode:
            raise RuntimeError('Existing experiment identity differs')
        if bool(report.get('simulator_operator', False)) != args.simulator_operator:
            raise RuntimeError('Existing experiment source mode differs')
    else:
        handles = {}
        start_attempted = False
        try:
            if sustained:
                # Verify the screenshot path before activating a one-shot native
                # experiment, so a helper/size error does not consume the trial.
                baseline = snapshot(stem.name + '-baseline', 1280)
                if baseline['pid'] != args.pid or game_window() != target:
                    raise RuntimeError('Game changed during screenshot preflight')
                report['baseline'] = baseline
            command = [sys.executable, str(ROOT / 'tools/native-capture/start_capture.py'),
                       '--pid', str(args.pid), '--seconds', str(args.trial_seconds+15) if sustained else '25', '--build-dir', str(build),
                       '--dll-name', args.dll_name, '--use-reserved', '--submit-pair',
                       '--submit-pair-seconds', str(args.trial_seconds) if sustained else '10', '--submit-background',
                       '--submit-output-bounds', '--viewport-diagnostics', '--color-copy-diagnostics']
            if sustained:
                command.extend(['--trial-seconds', str(args.trial_seconds)])
            if args.mode == 'mask':
                command.append('--first-view-mask-control')
            elif sustained:
                command.append('--view-union-mask-fix')
                if args.mode == 'projection':
                    command.append('--projection-control')
                if args.mode == 'stereo':
                    command.extend(['--stereo-xr','--stereo-units-per-metre',str(args.stereo_units_per_metre)])
            if args.frame_link_diagnostics:
                command.extend(['--frame-link-diagnostics','--require-target-prime'])
            if args.stereo_gpu_publish:
                command.append('--stereo-gpu-publish')
            if args.simulator_operator:
                command.append('--simulator-operator')
            with stem.with_name(stem.name + '-start.log').open('w') as log:
                # A nonzero exit can occur after remote activation. Preserve the
                # original error and still seek the matching native stop record.
                start_attempted = True
                subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
            started = time.monotonic()
            if args.mode == 'stereo':
                # Native production starts after its three-second mono baseline.
                # Audible cues remain useful while the user is wearing the HMD.
                import winsound
                time.sleep(3)
                winsound.Beep(880, 250)
                winsound.Beep(880, 250)
                print(json.dumps({'motion_cue':'start'}),flush=True)
            deadline = started + 18
            if args.mode == 'mask':
                for phase in ('A1', 'B', 'A2'):
                    handle = k.OpenEventW(0x100000, False,
                        f'Local\\GearsVR.Capture.FirstViewMask.{args.pid}.{phase}')
                    if not handle:
                        raise RuntimeError(f'Missing phase event {phase}')
                    handles[phase] = handle

            def phases():
                return [name for name, handle in handles.items() if k.WaitForSingleObject(handle, 0) == 0]

            schedule = (('A1', None), ('B', None), ('A2', None)) if args.mode == 'mask' else (
                (('early', 5), ('middle', args.trial_seconds/2), ('late', args.trial_seconds-4), ('restored', args.trial_seconds+5))
                if args.mode in ('union','stereo') else (('copy', 5),))
            if args.mode == 'projection':
                schedule = (('A1', 5), ('B', 10), ('A2', 15), ('C', 20), ('A3', 26), ('restored', 35))
            for phase, elapsed in schedule:
                if args.mode == 'mask':
                    while phases() != [phase]:
                        if time.monotonic() > deadline or game_window() != target:
                            raise RuntimeError(f'Phase {phase} not reached in same game')
                        time.sleep(.02)
                    time.sleep(.6)
                else:
                    # Capture enables native pairs after a three-second baseline.
                    while time.monotonic() < started + elapsed:
                        if game_window() != target:
                            raise RuntimeError('Game changed during scheduled observation')
                        time.sleep(min(.2, max(0., started + elapsed - time.monotonic())))
                if args.mode == 'stereo' and phase == 'restored':
                    winsound.Beep(440, 500)
                    print(json.dumps({'motion_cue':'stop'}),flush=True)
                before = phases()
                first = qpc()
                # The existing WGC helper accepts at most 1280 pixels.
                shot = snapshot(stem.name + f'-{phase}', 1280 if sustained else 960)
                last = qpc()
                after = phases()
                valid = shot['pid'] == args.pid and game_window() == target
                if args.mode == 'mask':
                    valid = valid and before == after == [phase]
                report['shots'].append({'phase': phase, 'qpc_start': first, 'qpc_end': last,
                    'phase_before': before, 'phase_after': after, 'phase_consistent': valid, 'frame': shot})
                report_path.write_text(json.dumps(report, indent=2), encoding='utf-8')
                print(json.dumps({'phase': phase, 'phase_consistent': valid, 'image': shot['image']}), flush=True)
                if not valid:
                    raise RuntimeError('Game changed or screenshot crossed phase boundary')
        except Exception as error:
            report['error'] = str(error)
            if not start_attempted:
                raise
            print(json.dumps({'capture_error': str(error), 'collecting_cleanup': True}), flush=True)
        finally:
            for handle in handles.values():
                k.CloseHandle(handle)
            report_path.write_text(json.dumps(report, indent=2), encoding='utf-8')

    deadline = time.monotonic() + (55 if sustained else 35)
    while True:
        try:
            status = (build / 'capture-status.txt').read_text()
        except (OSError, UnicodeError):
            status = ''
        if f'capturing pid={args.pid} ' in status and 'stopped disable_status=0 active=0' in status:
            break
        if time.monotonic() > deadline:
            raise RuntimeError('Clean stop not confirmed; inspect current process before proceeding')
        time.sleep(.2)
    shutil.copyfile(build / 'capture-status.txt', stem.with_suffix('.status.txt'))
    shutil.copyfile(build / 'capture.csv', stem.with_suffix('.csv'))
    restored = stem.with_name(stem.name + '-restored.json')
    lifecycle = stem.with_name(stem.name + '-lifecycle.json')
    with stem.with_name(stem.name + '-cleanup.log').open('w') as log:
        subprocess.run([sys.executable, str(ROOT / 'tools/native-capture/verify_stopped.py'),
            '--pid', str(args.pid), '--status', str(stem.with_suffix('.status.txt')),
            '--output', str(restored)], stdout=log, stderr=subprocess.STDOUT, check=True)
        subprocess.run([sys.executable, str(ROOT / 'tools/native-capture/analyze_submit_duration.py'),
            '--capture', str(stem.with_suffix('.json')), '--restored', str(restored),
            '--output', str(lifecycle)], stdout=log, stderr=subprocess.STDOUT, check=True)

    if args.mode == 'mask':
        rows = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
                if line.startswith('first_view_mask_phase ')]
        bounds = {row['phase']: (int(row['first_qpc']), int(row['last_qpc'])) for row in rows}
    else:
        first = None
        last = 0
        last_pair_event = 0
        first_single_after_pair = None
        last_single_after_pair = 0
        with stem.with_suffix('.csv').open(newline='') as stream:
            for row in csv.DictReader(stream):
                tick = int(row['qpc'])
                if row['family_count'] == '2' and row['test_state'] == '77' and row['kind'] in ('9', '10', '11', '12'):
                    last_pair_event = max(last_pair_event, tick)
                    if row['kind'] == '9':
                        first = tick if first is None else min(first, tick)
                        last = max(last, tick)
        bounds = {'copy': (first, last)}
        if sustained:
            # The CSV combines multiple producer threads; do not infer time
            # order from row order when proving that single-view has resumed.
            with stem.with_suffix('.csv').open(newline='') as stream:
                for row in csv.DictReader(stream):
                    tick = int(row['qpc'])
                    if row['kind'] == '9' and row['family_count'] == '1' and row['test_state'] != '77' and tick > last_pair_event > 0:
                        first_single_after_pair = tick if first_single_after_pair is None else min(first_single_after_pair, tick)
                        last_single_after_pair = max(last_single_after_pair, tick)
            records = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
                       if line.startswith('view_union_mask_fix ')]
            if len(records) != 1:
                raise RuntimeError('Expected exactly one view-union summary')
            correction = records[0]
            report['correction'] = correction
            accepted = int(correction.get('accepted', 0))
            report['correction_valid'] = (correction.get('enabled') == '1' and accepted >= 10
                and int(correction.get('modified', 0)) == accepted
                and int(correction.get('rejected', -1)) == 0)
            corrected_first = int(correction['first_qpc'])
            corrected_last = int(correction['last_qpc'])
            active_bounds = (max(first, corrected_first), min(last, corrected_last)) if first is not None else (None, 0)
            bounds = dict.fromkeys(('early', 'middle', 'late'), active_bounds)
            bounds['restored'] = (first_single_after_pair, last_single_after_pair)
            if args.mode == 'projection':
                phases = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
                          if line.startswith('projection_phase ')]
                consumed = {row['phase']: row for row in phases if row.get('stage') == 'consume_before'}
                report['projection_phases'] = phases
                summaries = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
                             if line.startswith('projection_control ')]
                report['projection_control'] = summaries
                by_stage = {(row['phase'], row['stage']): row for row in phases}
                report['projection_trace_valid'] = (len(summaries) == 1 and summaries[0].get('enabled') == '1'
                    and int(summaries[0].get('rejected', -1)) == 0
                    and all(int(by_stage.get((phase, stage), {}).get('count', 0)) > 0
                            and int(by_stage.get((phase, stage), {}).get('mismatch', -1)) == 0
                            for phase in ('A1', 'B', 'A2', 'C', 'A3')
                            for stage in ('built', 'consume_before', 'prepare_before', 'prepare_after')))
                report['projection_residuals'] = projection_residuals(by_stage)
                report['projection_valid'] = report['projection_trace_valid'] and report['projection_residuals']['passed']
                for phase in ('A1', 'B', 'A2', 'C', 'A3'):
                    row = consumed.get(phase)
                    bounds[phase] = (max(active_bounds[0], int(row['first_qpc'])),
                                     min(active_bounds[1], int(row['last_qpc']))) if row and active_bounds[0] is not None else (None, 0)
    for shot in report['shots']:
        first, last = bounds.get(shot['phase'], (None, 0))
        shot['qpc_covered'] = first is not None and first <= shot['qpc_start'] <= shot['qpc_end'] <= last
    if args.mode == 'stereo':
        try:
            with stem.with_suffix('.csv').open(newline='') as stream:
                report['native_source_evidence'] = validate_native_source_records(status, list(csv.DictReader(stream)), args.simulator_operator)
            report['native_source_valid'] = True
        except (ValueError, TypeError, KeyError) as error:
            report['native_source_valid'] = False
            report['native_source_error'] = str(error)
        summaries = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines() if line.startswith('stereo_xr ')]
        stages = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines() if line.startswith('stereo_stage ')]
        rejections = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines() if line.startswith('stereo_rejection ')]
        report['stereo_summary'], report['stereo_stages'], report['stereo_rejections'] = summaries, stages, rejections
        accepted = int(summaries[0].get('accepted', 0)) if len(summaries) == 1 else 0
        report['stereo_valid'] = (len(summaries) == 1 and summaries[0].get('enabled') == '1' and accepted >= 10
            and {row.get('stage') for row in stages} == {'built','consume_before','prepare_before','prepare_after'}
            and len(stages) == 4 and all(int(row.get('count',0)) == accepted and int(row.get('mismatch',-1)) == 0
                and float(row.get('view_error_max','nan')) == 0 for row in stages))
        report['stereo_limits'] = ('Native ticketed GPU publication; XR projection coverage is verified by run_stereo_probe separately; physical scale remains uncalibrated.'
                                 if args.stereo_gpu_publish else 'Native matrix and lifecycle validation; no GPU ticket publication, physical scale calibration or game projection layer.')
    if args.frame_link_diagnostics:
        for key, prefix in (('frame_link','frame_link '),('frame_link_binding','frame_link_binding '),
                            ('frame_link_backbuffers','frame_link_backbuffer '),('frame_link_rejections','frame_link_rejection ')):
            report[key] = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines() if line.startswith(prefix)]
        report['frame_link_limits'] = 'Conditional CPU draw/list/queue association. Inspect rejection counters and unobserved write/lifetime paths before enabling GPU publication.'
    if args.stereo_gpu_publish:
        rows = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
                if line.startswith('stereo_gpu_publish ')]
        report['stereo_gpu_publish'] = rows
        publisher = rows[0] if len(rows) == 1 else {}
        report['stereo_gpu_publish_valid'] = (publisher.get('enabled') == '1'
            and publisher.get('state') == '4' and publisher.get('stopped') == '1'
            and publisher.get('drain_ok') == '1' and publisher.get('close_ok') == '1'
            and int(publisher.get('published', 0)) > 0
            and int(publisher.get('prepare_ok', 0)) > 0
            and int(publisher.get('prepare_failed', -1)) == 0)
    expected_shots = {'mask': 3, 'copy': 1, 'union': 4, 'projection': 6, 'stereo': 4}[args.mode]
    report['timing_valid'] = report['error'] is None and len(report['shots']) == expected_shots and all(
        s['phase_consistent'] and s['qpc_covered'] for s in report['shots'])
    report['cleanup_valid'] = json.loads(restored.read_text())['all_match'] and json.loads(lifecycle.read_text())['passed']
    report_path.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'timing_valid': report['timing_valid'], 'cleanup_valid': report['cleanup_valid'],
                      'report': str(report_path)}), flush=True)
    if report['error'] or not report['timing_valid'] or not report['cleanup_valid'] or report.get('correction_valid') is False or report.get('projection_valid') is False or report.get('stereo_valid') is False or report.get('stereo_gpu_publish_valid') is False or report.get('native_source_valid') is False:
        raise RuntimeError(report['error'] or 'Screenshot timing, correction coverage, or cleanup not verified')


if __name__ == '__main__':
    main()
