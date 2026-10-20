"""Bounded real-XR paired-view test; game projection requires a native publisher."""
import argparse
import csv
import io
import importlib.util
import json
import math
import os
import pathlib
import re
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]


def complete_csv_rows(path):
    # The producer flushes while still sampling. A reader may see only part of
    # the newest row; only newline-terminated records are eligible evidence.
    data = path.read_bytes()
    boundary = data.rfind(b'\n')
    if boundary < 0:
        return []
    return list(csv.DictReader(io.StringIO(data[:boundary + 1].decode('utf-8'))))


def operator_module():
    path = ROOT / 'tools/openxr-probe/operator_client.py'
    spec = importlib.util.spec_from_file_location('gears_operator_client', path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def paired_render_ready(path, prior_qpc, evidence):
    if not path.is_file():
        return False
    rows = complete_csv_rows(path)
    for row in reversed(rows):
        if (row.get('kind') in ('9', '10') and row.get('test_state') == '77'
                and row.get('family_count') == '2' and int(row.get('qpc') or 0) > prior_qpc):
            evidence.update({'qpc': int(row['qpc']), 'kind': int(row['kind']),
                             'family_count': 2, 'test_state': 77, 'path': str(path)})
            return True
    return False


def source_flags_valid(source, head_flags, view_flags, simulator_operator=False):
    expected = int(simulator_operator)
    return (int(source) == expected and (int(view_flags) & 15) == 15
            and (int(head_flags) & (3 if simulator_operator else 15)) == (3 if simulator_operator else 15))


def validate_native_source_records(status, native_rows, simulator_operator=False):
    summaries = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
                 if line.startswith('stereo_xr ')]
    if len(summaries) != 1 or int(summaries[0].get('simulator_operator', '0')) != int(simulator_operator):
        raise ValueError('Native simulator_operator mode missing or differs from requested source')
    samples = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
               if line.startswith('stereo_sample ')]
    if simulator_operator and (len(samples) != 2 or {s.get('edge') for s in samples} != {'first', 'last'}):
        raise ValueError('Simulated source requires native first/last sample evidence')
    for sample in samples:
        source = sample.get('source', None if simulator_operator else '0')
        if source is None or not source_flags_valid(source, sample['head_flags'], sample['view_flags'], simulator_operator):
            raise ValueError('Native summary raw source/flags differ from requested mode')
    ticketed = [row for row in native_rows if int(row.get('stereo_ticket') or 0) > 0]
    if simulator_operator and not ticketed:
        raise ValueError('Simulated source requires native ticketed CSV evidence')
    for row in ticketed:
        source = row.get('stereo_source', None if simulator_operator else '0')
        if source is None or not source_flags_valid(source, row['stereo_head_flags'], row['stereo_view_flags'], simulator_operator):
            raise ValueError('Native ticketed CSV source/flags changed or were invalid')
    return {'native_ticketed_rows': len(ticketed), 'native_simulator_operator': int(simulator_operator),
            'simulation_not_physical_tracking': bool(simulator_operator)}


def validate_stereo_sources(xr_csv, native_csv, native_status, simulator_operator=False):
    """Preserve raw flags and require one declared source throughout the window.

    Old strict CSVs may omit source metadata; simulated samples must declare it.
    No tracking bits are synthesized from the independently tracked eye poses.
    """
    result = {'passed': False, 'expected_source': int(simulator_operator),
              'simulation_not_physical_tracking': bool(simulator_operator)}
    try:
        status = native_status.read_text(encoding='utf-8', errors='replace')
        summaries = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines()
                     if line.startswith('stereo_xr ')]
        if len(summaries) != 1:
            raise ValueError('Native stereo source summary missing or ambiguous')
        summary = summaries[0]
        if int(summary.get('simulator_operator', '0')) != int(simulator_operator):
            raise ValueError('Native simulator_operator mode differs from requested source')
        first, last = (int(summary[key]) for key in ('first_sequence', 'last_sequence'))
        if first <= 0 or last < first:
            raise ValueError('Native source sequence window invalid')
        rows = [row for row in complete_csv_rows(xr_csv)
                if first <= int(row.get('stereo_sequence') or 0) <= last]
        if not rows:
            raise ValueError('No XR samples in native source window')
        for row in rows:
            source = row.get('stereo_source', None if simulator_operator else '0')
            if (source is None or not source_flags_valid(source, row['head_flags'], row['view_flags'], simulator_operator)
                    or row.get('stereo_published') != '1' or row.get('session_state') != '5'
                    or row.get('should_render') != '1'):
                raise ValueError('XR source/flags changed or were invalid in accepted window')
        native_rows = complete_csv_rows(native_csv)
        result.update(validate_native_source_records(status, native_rows, simulator_operator))
        result.update(passed=True, native_sequence_window=[first, last], xr_samples=len(rows),
                      legacy_source_assumed_zero=any('stereo_source' not in row for row in rows),
                      xr_raw_flag_counts={str(pair):sum((r['head_flags'],r['view_flags']) == pair for r in rows)
                                          for pair in {(r['head_flags'],r['view_flags']) for r in rows}})
    except (OSError, ValueError, TypeError, KeyError) as error:
        result['error'] = str(error)
    return result


def sixdof_ranges(rows,initial):
    """LOCAL rotation vector of q_current * inverse(q_initial), in degrees."""
    if not initial:raise ValueError('SixDoF motion requires the observed initial pose')
    baseline=initial['orientation'];inverse=[-v for v in baseline[:3]]+[baseline[3]]
    norm=math.sqrt(sum(v*v for v in baseline))
    if not math.isfinite(norm) or abs(norm-1)>.01:raise ValueError('Invalid baseline quaternion')
    vectors=[];positions=[]
    for row in rows:
        q=[float(row[k]) for k in ('qx','qy','qz','qw')]
        if abs(math.sqrt(sum(v*v for v in q))-1)>.01:raise ValueError('Invalid sampled quaternion')
        x,y,z,w=q;u,v,t,s=inverse
        relative=[w*u+x*s+y*t-z*v,w*v-x*t+y*s+z*u,w*t+x*v-y*u+z*s,w*s-x*u-y*v-z*t]
        if relative[3]<0:relative=[-v for v in relative]
        length=math.sqrt(sum(v*v for v in relative[:3]))
        factor=math.degrees(2*math.atan2(length,relative[3]))/length if length>1e-12 else 0
        vectors.append([v*factor for v in relative[:3]])
        positions.append([float(row['head_'+axis])-initial['position'][i] for i,axis in enumerate('xyz')])
    if not all(math.isfinite(v) for group in (vectors,positions) for row in group for v in row):
        raise ValueError('Nonfinite six-axis evidence')
    return {'rotation_span_degrees':dict(zip(('pitch','yaw','roll'),
                (max(v[i] for v in vectors)-min(v[i] for v in vectors) for i in range(3)))),
            'translation_span_metres':dict(zip('xyz',
                (max(v[i] for v in positions)-min(v[i] for v in positions) for i in range(3))))}


def validate_operator_motion(xr_csv, native_csv, native_status, simulator_operator=False,profile='yaw-x',initial=None):
    if profile not in ('yaw-x','sixdof'):raise ValueError('Unknown Operator profile')
    result = {'passed': False, 'limits': 'XR motion within the accepted native sequence window and varying native left-view matrices are indirect evidence; native status does not log every frozen pose.'}
    if not native_status.is_file() or not native_csv.is_file() or not xr_csv.is_file():
        result['error'] = 'Motion evidence files missing'
        return result
    status = native_status.read_text(encoding='utf-8', errors='replace')
    summaries = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines() if line.startswith('stereo_xr ')]
    stages = [dict(re.findall(r'(\w+)=([^\s]+)', line)) for line in status.splitlines() if line.startswith('stereo_stage ')]
    if len(summaries) != 1:
        result['error'] = 'Native stereo sequence window missing or ambiguous'
        return result
    first, last = (int(summaries[0][key]) for key in ('first_sequence', 'last_sequence'))
    rows = [row for row in complete_csv_rows(xr_csv) if first <= int(row.get('stereo_sequence') or 0) <= last]
    if len(rows) < 2:
        result['error'] = 'Not enough XR samples in native window'
        return result
    yaws, xs = [], []
    for row in rows:
        x, y, z, w = (float(row[key]) for key in ('qx', 'qy', 'qz', 'qw'))
        yaws.append(math.degrees(math.atan2(2 * (w*y + x*z), 1 - 2*(y*y + x*x))) % 360)
        xs.append(float(row['head_x']))
    ordered = sorted(yaws)
    gaps = [b-a for a,b in zip(ordered, ordered[1:])] + [ordered[0]+360-ordered[-1]]
    yaw_span = 360-max(gaps)
    x_span = max(xs)-min(xs)
    flags_valid = all(row.get('stereo_published') == '1' and row.get('session_state') == '5'
                      and row.get('should_render') == '1'
                      and source_flags_valid(row.get('stereo_source', '-1' if simulator_operator else '0'),
                                             row['head_flags'], row['view_flags'], simulator_operator) for row in rows)
    native = [r for r in complete_csv_rows(native_csv) if r.get('test_state') == '77'
              and r.get('family_count') == '2' and (int(r.get('valid') or 0) & 512)]
    changes = {}
    for kind, label in (('10', 'consume'), ('12', 'prepare')):
        values = [[float(row['matrix'+str(i)]) for i in range(16)] for row in native if row.get('kind') == kind]
        ranges = [max(v[i] for v in values)-min(v[i] for v in values) for i in range(16)] if values else [0]*16
        changes[label] = {'samples': len(values), 'rotation_element_span': max(ranges[i] for i in (0,1,2,4,5,6,8,9,10)),
                          'translation_element_span': max(ranges[i] for i in (12,13,14))}
    stages_valid = (len(stages) == 4 and {r['stage'] for r in stages} == {'built','consume_before','prepare_before','prepare_after'}
                    and all(int(r.get('count',0)) == int(summaries[0]['accepted']) and int(r.get('mismatch',-1)) == 0
                            and float(r.get('view_error_max','nan')) == 0 for r in stages))
    finite = all(math.isfinite(v) for v in [*xs,*yaws])
    result.update({'native_sequence_window': [first,last], 'xr_samples': len(rows), 'yaw_span_degrees': yaw_span,
                   'head_x_span_metres': x_span, 'required_yaw_span_degrees': 20, 'required_head_x_span_metres': .10,
                   'flags_valid': flags_valid, 'stages_valid': stages_valid, 'native_view_changes': changes,
                   'simulation_not_physical_tracking': bool(simulator_operator)})
    result['passed'] = bool(finite and flags_valid and stages_valid and yaw_span >= 20 and x_span >= .10
                            and all(v['samples'] >= 2 and v['rotation_element_span'] > .01
                                    and v['translation_element_span'] > .01 for v in changes.values()))
    result['operator_profile']=profile
    if profile=='sixdof':
        axes=sixdof_ranges(rows,initial)
        required_rotation={'yaw':20,'pitch':14,'roll':14}
        required_translation={'x':.10,'y':.06,'z':.08}
        axes_valid=all(axes['rotation_span_degrees'][k]>=v for k,v in required_rotation.items()) and all(
            axes['translation_span_metres'][k]>=v for k,v in required_translation.items())
        residual_keys=[f'residual_{eye}_p{p}_{edge}' for eye in ('l','r') for p in (8,9) for edge in ('min','max')]
        residuals_valid=all(all(float(stage.get(k,'nan'))==0 for k in residual_keys) for stage in stages)
        result.update(axes,required_rotation_span_degrees=required_rotation,
                      required_translation_span_metres=required_translation,six_axes_valid=axes_valid,
                      both_eye_residuals_valid=residuals_valid,initial_pose=initial)
        # Relative LOCAL ranges replace absolute yaw when baseline is tilted.
        result['passed']=bool(simulator_operator and finite and flags_valid and stages_valid and residuals_valid and axes_valid
            and all(v['samples']>=2 and v['rotation_element_span']>.01 and v['translation_element_span']>.01 for v in changes.values()))
    return result


def xr_command(args, output):
    mode = ['--stereo-game-pid'] if args.stereo_game else ['--render', '--stereo-bridge-pid']
    command = [str(args.xr_build_dir.resolve() / 'OpenXRPoseProbe.exe'),
               '150' if args.trial_seconds == 120 else '60', str(output), *mode, str(args.pid)]
    if args.trial_seconds == 120:
        command.extend(['--stereo-producer-seconds', '120'])
    if args.gpu_snapshot:
        command.extend(['--gpu-snapshot', str(output) + '-gpu.ppm'])
    if getattr(args, 'simulator_operator', False) is True:
        command.append('--simulator-operator')
    if getattr(args,'shutdown_diagnostics',False) is True:
        command.append('--shutdown-diagnostics')
    return command


def native_command(args):
    command = [sys.executable, str(ROOT / 'tools/native-capture/run_color_probe.py'),
               '--pid', str(args.pid), '--attempt', str(args.attempt),
               '--build-dir', str(args.build_dir.resolve()), '--dll-name', args.dll_name,
               '--mode', 'stereo']
    if args.frame_link_diagnostics or args.stereo_game:
        command.append('--frame-link-diagnostics')
    if args.stereo_game:
        command.append('--stereo-gpu-publish')
    if args.trial_seconds == 120:
        command.extend(['--trial-seconds', '120'])
    if getattr(args, 'simulator_operator', False) is True:
        command.append('--simulator-operator')
    return command


def collect_xr_evidence(path, report):
    if not path.is_file():
        return
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        try:
            counters = json.loads(line)
        except ValueError:
            continue
        if not isinstance(counters, dict):
            continue
        for field, key in (('submitted_image_layers', 'render_pattern_summary'),
                           ('stereo_bridge_session', 'stereo_bridge_summary'),
                           ('stereo_game_projection_layers', 'stereo_game_summary'),
                           ('stereo_submission_evidence', 'stereo_submission_evidence')):
            if field in counters:
                report[key] = counters
        if 'stereo_projection_rejection' in counters:
            report.setdefault('stereo_projection_rejections', []).append(counters)
        if counters.get('unsafe_xr_teardown'):
            report['unsafe_xr_teardown'] = True


def passed(report, stereo_game=False):
    normal = (report.get('native_exit_code') == report.get('xr_exit_code') == 0
              and 'error' not in report and 'xr_error' not in report
              and not report.get('unsafe_xr_teardown'))
    if report.get('operator_motion_requested'):
        normal = normal and report.get('operator_motion_valid', False)
    if 'stereo_source_evidence' in report:
        normal = normal and report['stereo_source_evidence'].get('passed', False)
    if not stereo_game:
        return normal
    evidence = report.get('stereo_submission_evidence', {})
    counters = report.get('stereo_game_summary', {})
    coverage = bool(evidence.get('sustained_submission_passed')
                    and evidence.get('required_span_seconds') == report.get('trial_seconds', 30)-1
                    and not evidence.get('structural_failure')
                    and counters.get('stereo_game_projection_layers', 0) > 0
                    and counters.get('stereo_game_copies', 0) > 0)
    report['stereo_coverage_passed'] = bool(normal and coverage)
    return report['stereo_coverage_passed']


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--attempt',type=int,default=1)
    parser.add_argument('--build-dir', type=pathlib.Path, default=ROOT / 'tools/native-capture/build-stereoxr')
    parser.add_argument('--dll-name', default='GearsCaptureStereoXR.dll')
    parser.add_argument('--xr-build-dir', type=pathlib.Path, default=ROOT / 'tools/openxr-probe/build-stereoxr')
    parser.add_argument('--frame-link-diagnostics', action='store_true')
    parser.add_argument('--stereo-game', action='store_true', help='Native ticketed GPU publication and XR game projection coverage')
    parser.add_argument('--gpu-snapshot', action='store_true', help='Save one XR GPU image; requires stereo-game')
    parser.add_argument('--shutdown-diagnostics',action='store_true',help='Opt-in bounded XR shutdown markers and terminate stack addresses; does not change failure handling')
    parser.add_argument('--operator-motion', action='store_true', help='Run bounded synthetic simulator motion after native paired rendering starts')
    parser.add_argument('--operator-profile',choices=('yaw-x','sixdof'),default='yaw-x',
                        help='SixDoF requires explicit simulator-operator, stereo-game and a 120-second trial')
    parser.add_argument('--simulator-operator', action='store_true',
                        help='Explicit synthetic pose source; requires operator-motion and stereo-game; does not prove physical tracking')
    parser.add_argument('--trial-seconds', type=int, choices=(30,120), default=30)
    args = parser.parse_args(argv)
    if args.pid <= 0 or args.attempt < 1:
        parser.error('PID and attempt must be positive')
    if args.gpu_snapshot and not args.stereo_game:
        parser.error('GPU snapshot requires stereo-game')
    if args.trial_seconds == 120 and not args.stereo_game:
        parser.error('120-second trial requires stereo-game')
    if args.simulator_operator and not (args.operator_motion and args.stereo_game):
        parser.error('Simulator operator source requires operator-motion and stereo-game')
    if args.operator_profile=='sixdof' and not(args.operator_motion and args.simulator_operator and args.stereo_game and args.trial_seconds==120):
        parser.error('SixDoF profile requires operator-motion, simulator-operator, stereo-game and trial-seconds120')
    stem = ROOT / 'analysis/runtime' / f'stereo-xr-{args.pid}-attempt{args.attempt}'
    output = stem.with_suffix('.csv')
    if (any(stem.with_suffix(suffix).exists() for suffix in ('.csv', '.json', '.stdout.txt', '.stderr.txt', '.operator.json', '.operator.mcp.jsonl'))
            or pathlib.Path(str(output) + '-gpu.ppm').exists()):
        raise RuntimeError('Preserve existing experiment')
    stem.parent.mkdir(parents=True, exist_ok=True)
    report = {'pid': args.pid, 'xr_csv': str(output), 'native_started': False,
              'stereo_game': args.stereo_game, 'trial_seconds': args.trial_seconds, 'operator_motion_requested': args.operator_motion, 'xr_command': xr_command(args, output),
              'native_command': native_command(args), 'simulator_operator': args.simulator_operator,
              'simulation_not_physical_tracking': args.simulator_operator,'operator_profile':args.operator_profile,
              'shutdown_diagnostics_requested':args.shutdown_diagnostics}
    operator = None
    child_options = {}
    if args.operator_motion:
        try:
            helper = operator_module()
            helper.assert_operator_port_free()
            child_options['env'] = helper.operator_child_env(os.environ.copy())
            operator = (helper.OperatorMotion(stem,total_timeout=170,profile='sixdof') if args.operator_profile=='sixdof'
                        else helper.OperatorMotion(stem,total_timeout=90))
        except Exception as error:
            report.update({'error': str(error), 'passed': False})
            stem.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
            print(json.dumps(report), flush=True)
            return 1
    with stem.with_suffix('.stdout.txt').open('w') as out, stem.with_suffix('.stderr.txt').open('w') as err:
        xr_started = time.monotonic()
        try:
            child = subprocess.Popen(report['xr_command'], stdout=out, stderr=err, **child_options)
        except Exception as error:
            report.update({'error': str(error), 'passed': False})
            if operator:
                try:
                    operator.close()
                except Exception as cleanup_error:
                    report['operator_cleanup_error'] = str(cleanup_error)
                report['operator_summary'] = operator.report
            stem.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
            print(json.dumps(report), flush=True)
            return 1
        try:
            deadline = time.monotonic() + 20
            ready = False
            while time.monotonic() < deadline and child.poll() is None:
                if output.exists():
                    rows = complete_csv_rows(output)
                    # Published, tracked and focused poses are required before
                    # activating a native experiment. The first flush is 60 rows.
                    ready = len(rows) >= 30 and all(r.get('stereo_published') == '1'
                        and r.get('session_state') == '5' and r.get('should_render') == '1'
                        and (int(r.get('view_flags') or 0) & 15) == 15
                        and (int(r.get('head_flags') or 0) & 15) == 15
                        and int(r.get('stereo_source', '-1' if args.simulator_operator else '0')) == int(args.simulator_operator)
                        for r in rows[-20:])
                    if ready:
                        report['preflight_frames'] = len(rows)
                        break
                time.sleep(.1)
            if not ready:
                raise RuntimeError('Focused paired tracking not ready; native hooks remain inactive')
            if operator:
                operator.connect_and_read_initial()
                if time.monotonic()-xr_started > 22:
                    raise RuntimeError('Too little XR lifetime remains for the native thirty-second trial')
                live_csv = args.build_dir.resolve() / 'capture.csv'
                previous = complete_csv_rows(live_csv) if live_csv.is_file() else []
                floor = max((int(r.get('qpc') or 0) for r in previous), default=0)
                report['operator_pair_start'] = {}
                operator.start_motion(lambda: paired_render_ready(live_csv, floor, report['operator_pair_start']), wait_timeout=20)
            print(json.dumps({'xr_tracking_ready': True, 'native_test_starting': True}), flush=True)
            report['native_started'] = True
            result = subprocess.run(report['native_command'], timeout=args.trial_seconds+150)
            report['native_exit_code'] = result.returncode
            if operator:
                operator.finish(timeout=70 if args.operator_profile=='sixdof' else 25)
        except Exception as error:
            report['error'] = str(error)
        finally:
            if operator:
                try:
                    operator.close()
                except Exception as error:
                    report['error'] = report.get('error', '') + ' Operator cleanup: ' + str(error)
                report['operator_summary'] = operator.report
            try:
                report['xr_exit_code'] = child.wait(timeout=args.trial_seconds+35)
            except subprocess.TimeoutExpired:
                report['xr_error'] = 'XR diagnostic exceeded total deadline'
                child.terminate()
                try:
                    child.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait(timeout=5)
    collect_xr_evidence(stem.with_suffix('.stdout.txt'), report)
    native_stem = stem.with_name(f'color-stereo-probe-{args.pid}' + (f'-attempt{args.attempt}' if args.attempt != 1 else ''))
    if args.stereo_game:
        report['stereo_source_evidence'] = validate_stereo_sources(output, native_stem.with_suffix('.csv'),
            native_stem.with_suffix('.status.txt'), args.simulator_operator)
    if args.operator_motion:
        try:
            report['operator_motion_evidence'] = validate_operator_motion(output, native_stem.with_suffix('.csv'), native_stem.with_suffix('.status.txt'),
                args.simulator_operator,args.operator_profile,report.get('operator_summary',{}).get('initial_pose'))
            report['operator_motion_valid'] = bool(report['operator_motion_evidence']['passed']
                and report.get('operator_pair_start') and report.get('operator_summary', {}).get('success')
                and 'error' not in report)
        except Exception as error:
            report['operator_motion_valid'] = False
            report['operator_motion_validation_error'] = str(error)
    if args.gpu_snapshot:
        snapshot = pathlib.Path(str(output) + '-gpu.ppm')
        report['gpu_snapshot_path'] = str(snapshot)
        report['gpu_snapshot_saved'] = snapshot.is_file()
    success = passed(report, args.stereo_game)
    if args.gpu_snapshot and not report['gpu_snapshot_saved']:
        success = False
    report['passed'] = success
    stem.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report), flush=True)
    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
