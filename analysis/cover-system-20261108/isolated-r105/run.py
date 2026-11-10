"""One r105 private source-stage observation, with ordinary owned shutdown."""
from pathlib import Path
import importlib.util
import ctypes
import argparse
import json
import os
import subprocess
import sys
import time
import traceback
from prepare import ROOT, HERE, OLD, sha, load, write_new

NAME = 'cover-1110-r105-source-stage'
SELECTED = ROOT / 'analysis/instances' / NAME
OUTPUT = SELECTED / 'cover-output'
TOOLS = ROOT / 'tools/multi-instance/isolated_sim.py'


def main():
    global NAME, SELECTED, OUTPUT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', required=True)
    parser.add_argument('--run', action='store_true', required=True)
    name = parser.parse_args().name
    if not name.startswith('cover-') or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789_-' for c in name):
        raise ValueError('Exact cover instance name required')
    NAME = name
    SELECTED = ROOT / 'analysis/instances' / NAME
    OUTPUT = SELECTED / 'cover-output'
    if Path.cwd().resolve() != ROOT or sys.flags.optimize:
        raise RuntimeError('Use --run in the canonical worktree without optimization')
    prepared = load(HERE / NAME / 'prepared.json')
    action = load(HERE / NAME / 'operator-prepared.json')
    for path, expected in ((SELECTED/'manifest.json', prepared['manifest_sha256']),
                           (SELECTED/'config.json', prepared['config_sha256']),
                           (Path(action['operator']), action['operator_sha256']),
                           (Path(action['input_reader']), action['input_reader_sha256']),
                           (TOOLS, prepared['isolation_source_sha256'])):
        if sha(path) != expected:
            raise RuntimeError('Prepared input changed: ' + str(path))
    manifest = load(SELECTED/'manifest.json')
    if 'openxr_loader_sha256' not in prepared or sha(ROOT/manifest['xr_build']/'openxr_loader.dll') != prepared['openxr_loader_sha256']:
        raise RuntimeError('Private XR loader missing or changed')
    for prefix, filename in (('native', manifest['native_dll']), ('playground', manifest['playground_dll']), ('xr', 'OpenXRPoseProbe.exe')):
        if sha(ROOT/manifest[prefix+'_build']/filename) != manifest[prefix+'_sha256']:
            raise RuntimeError('Private artifact changed: ' + prefix)
    if (SELECTED/'launcher-session.json').exists():
        raise RuntimeError('Attempt already launched; inspect it, never repeat')
    spec = importlib.util.spec_from_file_location('r105_isolated_sim', TOOLS)
    isolated = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(isolated)
    memory = isolated.require_memory()
    sys.path[:0] = [str(ROOT/'tools/native-capture'), str(OLD)]
    from run_stereo_continuous import WindowsControl
    from record_paired_windows import PairedWindowRecording
    control = WindowsControl()
    env = os.environ.copy()
    for key in ('GEARSVR_LANCER_INTEGRATED_PREPARED', 'GEARSVR_LANCER_INTEGRATED_CHECK', 'GEARSVR_STARTUP_REPLAY_ACTIONS'):
        env.pop(key, None)
    env.update(GEARSVR_COVER_SPHERE_PROBE='1', GEARSVR_AUTHORED_EYE_TRACE='1', GEARSVR_LANCER_REVIEW_VOLUMES='0')
    result = dict(name=NAME, memory_before=memory, started_at_ns=time.time_ns(),
                  source=1, forced_termination=False, gameplay_accepted=False, headset_accepted=False)
    result['foreground_observations'] = dict(samples=0, owned_activation=False)
    user = ctypes.WinDLL('user32', use_last_error=True)
    user.GetForegroundWindow.restype = ctypes.c_void_p
    user.GetWindowThreadProcessId.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    write_new(OUTPUT/'claim.json', dict(state='consumed', at_ns=time.time_ns(), pid=os.getpid()))
    paired = operator = None
    log = None
    def save():
        (OUTPUT/'run-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    def current():
        try:
            state = load(SELECTED/'launcher-session.json')
            session = load(state['session_report']) if state.get('session_report') else {}
            return state, session
        except (FileNotFoundError, PermissionError, json.JSONDecodeError):
            return {}, {}
    def check_background(state, session):
        pid = ctypes.c_uint32()
        user.GetWindowThreadProcessId(user.GetForegroundWindow(), ctypes.byref(pid))
        own = [state.get('game_identity'), state.get('owner_identity'), session.get('xr_identity'), result.get('operator_identity')]
        result['foreground_observations']['samples'] += 1
        if any(identity and identity['pid']==pid.value and control.identity(pid.value)==identity for identity in own):
            result['foreground_observations'].update(owned_activation=True, foreground_pid=pid.value)
            raise RuntimeError('Own private process activated; stop our action and clean up normally')
    try:
        start = subprocess.run([sys.executable, '-E', '-s', '-B', '-X', 'utf8', str(TOOLS), 'start', '--name', NAME],
            cwd=ROOT, env=env, capture_output=True, timeout=30, creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(start_exit=start.returncode, start_stdout=start.stdout.decode('utf-8', errors='replace'),
                      start_stderr=start.stderr.decode('utf-8', errors='replace'))
        save()
        if start.returncode:
            raise RuntimeError('Private launcher start refused; retain original output')
        worker = load(SELECTED/'isolated-worker.json')
        result['worker_identity'] = control.identity(worker['worker_pid'])
        if not result['worker_identity']:
            raise RuntimeError('Private worker already absent before readiness')
        deadline = time.monotonic()+180
        while time.monotonic()<deadline:
            state, session = current()
            check_background(state, session)
            if (SELECTED/'isolated-done.json').exists():
                raise RuntimeError('Private launcher ended before observation: '+str(load(SELECTED/'isolated-done.json')))
            if session.get('error'):
                raise RuntimeError('Private session error: '+str(session['error']))
            if paired is None and session.get('xr_identity') and session.get('xr_command'):
                directory = OUTPUT/'startup'
                directory.mkdir(exist_ok=True)
                candidate = PairedWindowRecording(state, directory, control, max_seconds=600)
                try:
                    candidate.start()
                except AssertionError as error:
                    detail = error.args[0] if error.args else None
                    if not (candidate.process is None and isinstance(detail, tuple) and detail == ('Expected exactly one owned simulator preview', [])):
                        raise
                else:
                    paired = candidate
                    result['paired_directory'] = str(directory)
                    save()
            if session.get('phase')=='running' and session.get('native_started') is True:
                if paired is None:
                    raise RuntimeError('No exact-owned paired recording before Native readiness')
                if state.get('vr_gamepad', {}).get('source') != 1:
                    raise RuntimeError('No own Source1 input')
                for key in ('game_identity', 'owner_identity', 'xr_identity'):
                    identity = session[key]
                    if control.identity(identity['pid']) != identity:
                        raise RuntimeError('Readiness identity changed: '+key)
                result['running_identities'] = {key:session[key] for key in ('game_identity','owner_identity','xr_identity')}
                save()
                break
            time.sleep(.2)
        else:
            raise TimeoutError('Private startup did not reach Native within 180s')
        log = (OUTPUT/'operator.log').open('xb')
        command = [sys.executable, '-E', '-s', '-B', '-X', 'utf8', action['operator'], *action['flags'], '--external-paired-recording']
        operator = subprocess.Popen(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT,
            stdin=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS)
        result['operator_identity'] = control.identity(operator.pid)
        save()
        deadline = time.monotonic()+150
        while operator.poll() is None and time.monotonic()<deadline:
            check_background(*current())
            if (OUTPUT/'request-stop').exists():
                raise RuntimeError('Own bounded stop requested')
            time.sleep(.2)
        result['operator_exit'] = operator.poll()
        if operator.poll() is None:
            raise TimeoutError('Own operator exceeded bounded action time; stop owner normally')
        if operator.returncode:
            raise RuntimeError('Own ordinary cover observation failed; preserve operator log')
    except BaseException as error:
        result['error'] = repr(error)
        result['traceback'] = traceback.format_exc()
    finally:
        # The isolated tool selects the exact private owner. No global stop,
        # shared restore or process termination is used on any exit path.
        state, session = current()
        owner = state.get('owner_identity')
        if owner and control.identity(owner['pid']) == owner:
            try:
                stop = subprocess.run([sys.executable, '-E', '-s', '-B', '-X', 'utf8', str(TOOLS), 'stop', '--name', NAME],
                    cwd=ROOT, env=env, capture_output=True, timeout=105, creationflags=subprocess.CREATE_NO_WINDOW)
                result.update(stop_exit=stop.returncode, stop_stdout=stop.stdout.decode('utf-8', errors='replace'), stop_stderr=stop.stderr.decode('utf-8', errors='replace'))
            except BaseException as error:
                result['stop_error'] = repr(error)
        if paired:
            try:
                paired.stop()
                result['paired_exit'] = paired.process.returncode
            except BaseException as error:
                result['paired_error'] = repr(error)
        if operator and operator.poll() is None:
            try:
                operator.wait(timeout=20)
            except subprocess.TimeoutExpired:
                result['operator_still_live'] = control.identity(operator.pid)
        if operator:
            result['operator_exit'] = operator.poll()
        if log:
            log.close()
        deadline = time.monotonic()+100
        while not (SELECTED/'isolated-done.json').exists() and time.monotonic()<deadline:
            time.sleep(.25)
        state, session = current()
        result['terminal_state'] = state
        result['terminal_session'] = session
        result['isolated_done'] = load(SELECTED/'isolated-done.json') if (SELECTED/'isolated-done.json').exists() else None
        result['remaining_owned'] = {key:control.identity(identity['pid']) for key in ('game_identity','owner_identity','xr_identity')
                                     if (identity:=session.get(key)) and control.identity(identity['pid'])==identity}
        result['normal_cleanup_verified'] = (session.get('native_cleanup_verified') is True
            and session.get('entry_restore_verified') is True and session.get('vr_gamepad_cleanup',{}).get('restored') is True
            and session.get('unified_weapons_cleanup',{}).get('drained') is True and session.get('xr_exit_code')==0
            and not result['remaining_owned'] and result['isolated_done'] is not None
            and result['isolated_done']['launcher_exit']==0 and result['isolated_done']['owned_game_absent'] is True)
        save()
    print(json.dumps({key:result.get(key) for key in ('error','operator_exit','paired_exit','normal_cleanup_verified','remaining_owned')}))
    return 0 if not result.get('error') and result['normal_cleanup_verified'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
