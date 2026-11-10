"""One independently admitted observation using the unchanged r80 game slice."""
from pathlib import Path
import hashlib
import importlib
import json
import os
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
OLD = ROOT/'analysis/solo-m1-20261028'
INSTANCE = ROOT/'analysis/instances/solo-m1-1028d'
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
load = lambda p: json.loads(Path(p).read_text(encoding='utf-8-sig'))


def static_check():
    if not (Path.cwd().resolve() == ROOT and sys.flags.optimize == 0
            and sys.flags.ignore_environment and sys.flags.no_user_site
            and sys.flags.dont_write_bytecode and sys.flags.utf8_mode and not sys.flags.isolated):
        raise RuntimeError('Exact worktree and -E -s -B -X utf8 required')
    ready = load(HERE/'ready.json')
    if sha(HERE/'plan.json') != ready['plan_sha256'] or sha(__file__) != ready['entry_sha256']:
        raise RuntimeError('Independent entry identity changed')
    plan = load(HERE/'plan.json')
    if Path(sys.executable).resolve() != Path(plan['interpreter']).resolve() or sha(sys.executable) != plan['interpreter_sha256']:
        raise RuntimeError('Interpreter changed')
    for path, digest in plan['source_sha256'].items():
        if sha(ROOT/path) != digest:
            raise RuntimeError('Observation input changed: ' + path)
    for name, proof in plan['original_five'].items():
        if sha(proof['path']) != proof['sha256']:
            raise RuntimeError('Sealed original changed: ' + name)
    for script in ('inner.py', 'stage.py'):
        compile((HERE/script).read_text(encoding='utf-8'), str(HERE/script), 'exec')
    check = subprocess.run([sys.executable, '-E', '-s', '-B', '-X', 'utf8',
                            str(HERE/'inner.py'), '--prepare-only'],
                           cwd=ROOT, capture_output=True, timeout=60,
                           creationflags=subprocess.CREATE_NO_WINDOW)
    if check.returncode:
        raise RuntimeError('Original preparation rejected: ' + (check.stdout+check.stderr).decode('utf-8', errors='replace'))
    return plan, json.loads(check.stdout)


def prove_closed(started_wall, plan):
    """Separate ordinary cleanup, cancelled pre-Native, and failed prebirth."""
    done_path, worker_path = OLD/'done.json', OLD/'worker.json'
    if min(done_path.stat().st_mtime, worker_path.stat().st_mtime) < started_wall:
        raise RuntimeError('No terminal record for this launch')
    done, worker = load(done_path), load(worker_path)
    if done.get('launcher_pid') != worker.get('launcher_pid') or type(done.get('launcher_exit_code')) is not int:
        raise RuntimeError('Launcher is not terminal or belongs to another attempt')
    sys.path.insert(0, str(ROOT/'tools/native-capture'))
    from run_stereo_continuous import WindowsControl
    control = WindowsControl()
    index = INSTANCE/'launcher-session.json'
    if index.stat().st_mtime < started_wall:
        raise RuntimeError('Launcher identity record is stale')
    state = load(index)
    owner = state.get('owner_identity')
    if not owner or owner['pid'] != worker['launcher_pid'] or control.identity(owner['pid']) is not None:
        raise RuntimeError('Owned launcher identity is unproven or still live')
    if done.get('remaining_owned_game', 'unknown') is not None:
        raise RuntimeError('Owned game not released')
    if not done.get('session_report'):
        if (done['launcher_exit_code'] == 0 or state.get('phase') != 'incomplete'
                or state.get('game_identity') or not state.get('error')
                or state.get('reservation_failure_cleanup', {}).get('status') != 'no_birth_receipt'):
            raise RuntimeError('Game-free startup failure unproven')
        return dict(kind='prebirth_failure', normal_game_cleanup_not_applicable=True,
                    owner=owner, launcher_exit=done['launcher_exit_code'], error=state['error'])
    path = Path(done['session_report'])
    if path.resolve().parent != ROOT/'analysis/runtime' or path.stat().st_mtime < started_wall:
        raise RuntimeError('Runtime report path or time differs')
    report = load(path)
    game = report.get('game_identity')
    if (report.get('owner_identity') != owner or not game
            or Path(game['image']).resolve() != Path(plan['game_path']).resolve()
            or control.identity(game['pid']) is not None):
        raise RuntimeError('Runtime owner/game identity not closed')
    xr = report.get('xr_identity')
    if xr and control.identity(xr['pid']) is not None:
        raise RuntimeError('Owned XR remains live')
    if report.get('vr_gamepad_cleanup', {}).get('restored') is not True:
        raise RuntimeError('Owned input not restored')
    if report.get('xr_forced_cleanup_reason') or type(report.get('xr_exit_code')) is not int:
        raise RuntimeError('XR cleanup is forced or unproven')
    if report.get('native_started') is False and report.get('startup_cancelled') is True and report.get('phase') == 'cancelled':
        if report.get('xr_cleanup_actions') != ['stop_signaled', 'wait']:
            raise RuntimeError('Cancelled XR normal stop not recorded')
        return dict(kind='startup_cancelled_before_native', normal_cleanup_verified=False,
                    startup_cancelled_cleanup_verified=True, game=game, owner=owner,
                    report=str(path), report_sha256=sha(path), xr_exit=report['xr_exit_code'])
    if not (report.get('native_cleanup_verified') is True and report.get('entry_restore_verified') is True
            and report.get('unified_weapons_cleanup', {}).get('drained') is True
            and report.get('xr_exit_code') == 0 and done['launcher_exit_code'] == 0):
        raise RuntimeError('Native/entry/input/XR ordinary closure not proven')
    artifacts = report['diagnostics_artifacts']
    if artifacts['native_sha256'] != plan['build_sha256']['native'] or artifacts['xr_sha256'] != plan['build_sha256']['sim_xr']:
        raise RuntimeError('Closed runtime artifacts differ')
    return dict(kind='normal', normal_cleanup_verified=True, game=game, owner=owner,
                report=str(path), report_sha256=sha(path))


def action_result(inner, started_wall):
    # Cleanup success is independent of action success. In 56276 the outer
    # child returned 0 while the actual operator returned 1 before cover entry.
    result = dict(observation_sequence_completed=False, gameplay_accepted=False)
    try:
        game = inner['capture_identity']
        capture = Path(inner['capture_identity_receipt'])
        prefix = 'menu-firstperson-' + str(game['pid']) + '-interrupted-cover-'
        if capture.parent.parent != OLD or not capture.parent.name.startswith(prefix):
            raise ValueError('Capture identity path differs')
        stamp = capture.parent.name[len(prefix):]
        if len(stamp) != 15 or stamp[8] != '-' or not stamp.replace('-', '').isdigit():
            raise ValueError('Capture stamp differs')
        actions_path = OLD/('lancer-startup-actions-' + stamp + '.json')
        if actions_path.stat().st_mtime < started_wall:
            raise ValueError('Operator result predates launch')
        actions = load(actions_path)
        result.update(operator_exit=actions.get('exit_code'), operator_error=actions.get('error'),
                      actions_path=str(actions_path), actions_sha256=sha(actions_path))
        current = load(OLD/'menu-firstperson-current.json')
        if current['game'] != game:
            raise ValueError('Action result game differs')
        result['action_error'] = current.get('error')
        directory = Path(current['directory'])
        if directory.parent != OLD or not directory.name.startswith('menu-firstperson-' + str(game['pid']) + '-'):
            raise ValueError('Action directory differs')
        boundary_path = directory/'cover-boundary.json'
        boundary = load(boundary_path)
        phases = boundary.get('eye_observation', [])
        result.update(boundary_path=str(boundary_path), boundary_sha256=sha(boundary_path),
                      observation_phases=len(phases))
        result['observation_sequence_completed'] = (
            type(actions.get('exit_code')) is int and actions['exit_code'] == 0
            and not actions.get('error') and not current.get('error')
            and current.get('checks', {}).get('cover_eye_observation_sequence_completed') is True
            and boundary.get('game') == game and boundary.get('passed') is True
            and len(phases) == 5
            and all(type(p.get('begin_qpc')) is int and type(p.get('end_qpc')) is int
                    and 0 < p['begin_qpc'] <= p['end_qpc'] for p in phases))
    except (OSError, ValueError, KeyError, TypeError) as error:
        result['action_evidence_error'] = repr(error)
    return result


def main():
    if sys.argv[1:] not in (['--prepare-only'], ['--run']):
        raise RuntimeError('Use exactly --prepare-only or --run')
    plan, original = static_check()
    if sys.argv[1:] == ['--prepare-only']:
        print(json.dumps(dict(original=original, new_entry_static_passed=True,
                              claim_consumed=False, staged=False, started=False)))
        return 0
    sys.path.insert(0, str(ROOT/'tools/debug-launch'))
    from runtime_stage_guard import RuntimeGuard
    # Same outer guard as the other independent runners; hold from the fresh
    # baseline through restore. The launcher's own owner mutex remains inner.
    with RuntimeGuard('solo-m1-1028d'):
        try:
            return run_owned(plan)
        except BaseException as error:
            result_path=HERE/'runtime/result.json'
            recovered=load(result_path).get('baseline_restored') is True if result_path.exists() else False
            if (HERE/'runtime/claim.json').exists() and not recovered:
                # Keep the actual OS guard alive on unknown cleanup/restore.
                # No force-stop, unlock, retry, or replacement run follows.
                (HERE/'runtime/guard-held-error.json').write_text(json.dumps(dict(error=repr(error),
                    guard_held=True, restoration_proven=False, automatic_restart=False)),encoding='utf8')
                print('Cleanup/restoration uncertain; retaining shared runtime guard for recovery.',file=sys.stderr,flush=True)
                while True:
                    time.sleep(1)
            raise



def run_owned(plan):
    sys.path.insert(0, str(ROOT/'tools/game-playtest'))
    sys.path.insert(0, str(HERE))
    import cover_launch_preflight as gate
    import cover_observation_baseline as backup
    # This is the specifically outstanding GN restoration, not a future
    # predecessor ticket. Do not infer its completion from an idle game/mutex.
    recovery = plan['outstanding_restoration']
    if sha(recovery['baseline']) != recovery['baseline_sha256'] or sha(recovery['claim']) != recovery['claim_sha256']:
        raise RuntimeError('Outstanding original-restoration identity changed')
    original = load(recovery['baseline'])
    resolution = load(recovery['receipt'])
    if (resolution.get('identity') != original['identity']
            or resolution.get('claim_sha256') != recovery['claim_sha256']
            or resolution.get('restored') is not True
            or resolution.get('released_after_restore') is not True
            or resolution.get('public_written') is not False
            or resolution.get('checks') != {row['label']: True for row in original['files']}):
        raise RuntimeError('Known outstanding original restoration is not complete')
    gate.check(os.getpid())
    claim = HERE/'runtime/claim.json'
    if claim.exists():
        raise RuntimeError('This independently identified attempt is consumed; never retry')
    original_plan = load(HERE/'runtime-plan.json')
    specs = [(Path(p), Path(p).resolve() == Path(plan['public_read_only']).resolve())
             for p in original_plan['required_baseline_paths']]
    profile = load(INSTANCE/'instance.json')
    for relative, item in profile['copied'].items():
        if relative.startswith(('LocalAppData\\Coalition\\WarGame\\Config\\',
                                'LocalAppData\\Coalition\\WarGame\\SaveData\\')):
            specs.append((Path(item['source']), True))
    private = INSTANCE/'LocalAppData/Coalition/WarGame'
    proof = backup.capture(specs, [private/'Config', private/'SaveData'],
                           HERE/('runtime/prestage-'+str(time.time_ns())), ROOT)
    proof_path = Path(proof['store'])/'baseline.json'
    proof_sha = sha(proof_path)
    gate.check(os.getpid())
    backup.verify(proof, current=True)
    # Exclusive child claim precedes its own stage. The original r80 claim and
    # its sealed five files are never changed or consumed.
    with (HERE/'runtime/inner.log').open('xb') as log:
        started_wall = time.time()
        process = subprocess.Popen([sys.executable, '-E', '-s', '-B', '-X', 'utf8',
                                    str(HERE/'inner.py'), '--run'], cwd=ROOT,
                                   stdout=log, stderr=subprocess.STDOUT,
                                   creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        # Existing inner run_basic owns the 180-second boundary and normal stop.
        result = process.wait()
    outcome = dict(child_exit=result, normal_cleanup_verified=False, baseline_restored=False,
                   baseline_path=str(proof_path), baseline_sha256=proof_sha,
                   goal_complete=False, gameplay_accepted=False, headset_accepted=False)
    try:
        inner = load(HERE/'runtime/attempt-result.json')
        if inner.get('started'):
            outcome['closure'] = prove_closed(started_wall, plan)
            outcome['normal_cleanup_verified'] = outcome['closure'].get('normal_cleanup_verified', False)
            if inner.get('capture_identity') and inner['capture_identity'] != outcome['closure'].get('game'):
                raise RuntimeError('Capture and terminal game differ')
        elif not inner.get('stage_exit_code') == 0:
            outcome['closure'] = dict(kind='stage_rejected_before_launch', game_started=False)
        else:
            outcome['closure'] = dict(kind='prelaunch_rejected', game_started=False)
        gate.check(os.getpid())
        if sha(proof_path) != proof_sha:
            raise RuntimeError('Restoration baseline changed')
        trace = INSTANCE/'native/authored-eye-geometry.bin'
        trace_before = next(row for row in proof['files'] if Path(row['path']) == trace)
        if trace.exists() and not trace_before['exists']:
            if not outcome['normal_cleanup_verified']:
                raise RuntimeError('Unattributed trace retained; no deletion during restore')
            saved = HERE/'runtime/retained-eye-geometry.bin'
            with saved.open('xb') as stream:
                stream.write(trace.read_bytes())
                stream.flush()
                os.fsync(stream.fileno())
            if sha(saved) != sha(trace):
                raise RuntimeError('Raw trace backup differs; source retained')
            outcome['raw_trace_retained'] = dict(path=str(saved), sha256=sha(saved))
        outcome['restoration'] = backup.restore(proof)
        outcome['baseline_restored'] = True
        outcome['runtime_released'] = True
        outcome['action'] = action_result(inner, started_wall)
        if outcome.get('normal_cleanup_verified'):
            status=load(outcome['closure']['report'])['native_status']
            probe={k:v for k,v in status.items() if k.startswith('cover_sphere_probe_')}
            probe['queried_current_c0']=(probe.get('cover_sphere_probe_requested')=='1'
                and probe.get('cover_sphere_probe_attempted')=='1'
                and probe.get('cover_sphere_probe_finished')=='1'
                and probe.get('cover_sphere_probe_code')=='0')
            outcome['sphere_probe']=probe

    except BaseException as error:
        outcome['cleanup_or_restore_error'] = repr(error)
        raise
    finally:
        backup.save_json(HERE/'runtime/result.json', outcome)
    print(json.dumps(outcome))
    return result if result else (0 if outcome.get('action', {}).get('observation_sequence_completed') and outcome.get('sphere_probe', {}).get('queried_current_c0') else 1)


if __name__ == '__main__':
    raise SystemExit(main())
