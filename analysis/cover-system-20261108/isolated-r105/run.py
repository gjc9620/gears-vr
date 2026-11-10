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


def require_start_allowed():
    pause = ROOT / 'tools/multi-instance/automatic-runtime-pause.json'
    if pause.exists() and load(pause).get('automatic_start_disabled') is True:
        raise RuntimeError('Owner paused automatic game/XR activity; no new input or start, stop our active owner normally')


def original_stereo_submitted(session):
    timing=session.get('startup_timings',{}).get('first_stereo_submission')
    return (isinstance(timing,dict) and timing.get('timing_is_runner_observation') is False
            and type(timing.get('xr_monotonic_ns')) is int and timing['xr_monotonic_ns']>0)


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
    require_start_allowed()
    prepared = load(HERE / NAME / 'prepared.json')
    action = load(HERE / NAME / 'operator-prepared.json')
    for path, expected in ((SELECTED/'manifest.json', prepared['manifest_sha256']),
                           (SELECTED/'config.json', prepared['config_sha256']),
                           (Path(action['operator']), action['operator_sha256']),
                           (Path(action['input_reader']), action['input_reader_sha256']),
                           (Path(action['observation_adapter']), action['observation_adapter_sha256']),
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
    from cover_projection_recording import ProjectionRecording
    control = WindowsControl()
    env = os.environ.copy()
    for key in ('GEARSVR_LANCER_INTEGRATED_PREPARED', 'GEARSVR_LANCER_INTEGRATED_CHECK', 'GEARSVR_STARTUP_REPLAY_ACTIONS'):
        env.pop(key, None)
    env.update(GEARSVR_COVER_SPHERE_PROBE='1', GEARSVR_AUTHORED_EYE_TRACE='1', GEARSVR_LANCER_REVIEW_VOLUMES='0',
               GEARSVR_QUIET_NATIVE_FOREGROUND='1', GEARSVR_TEST_WINDOWS_BEHIND='1', GEARSVR_TEST_MUTE_GAME='1',
               GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY='1', XR_SIMULATOR_NO_ACTIVATE='1')
    result = dict(name=NAME, memory_before=memory, started_at_ns=time.time_ns(),
                  source=1, forced_termination=False, gameplay_accepted=False, headset_accepted=False)
    result['foreground_observations'] = dict(samples=0, owned_activation=False)
    result['cloak_observations'] = dict(samples=0, failures=[])
    result['runner_sha256'] = sha(__file__)
    result['projection_recorder_sha256'] = sha(HERE/'cover_projection_recording.py')
    user = ctypes.WinDLL('user32', use_last_error=True)
    user.GetForegroundWindow.restype = ctypes.c_void_p
    user.GetWindowThreadProcessId.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    user.IsWindowVisible.argtypes = [ctypes.c_void_p]
    user.EnumWindows.argtypes = [ctypes.c_void_p, ctypes.c_ssize_t]
    dwm = ctypes.WinDLL('dwmapi')
    dwm.DwmGetWindowAttribute.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32]
    enum_callback = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_ssize_t)
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
        require_start_allowed()
        pid = ctypes.c_uint32()
        user.GetWindowThreadProcessId(user.GetForegroundWindow(), ctypes.byref(pid))
        own = [state.get('game_identity'), state.get('owner_identity'), session.get('xr_identity'), result.get('operator_identity')]
        result['foreground_observations']['samples'] += 1
        if any(identity and identity['pid']==pid.value and control.identity(pid.value)==identity for identity in own):
            result['foreground_observations'].update(owned_activation=True, foreground_pid=pid.value)
            raise RuntimeError('Own private process activated; stop our action and clean up normally')
        identities = {i['pid']:i for i in (state.get('game_identity'), session.get('xr_identity')) if i}
        @enum_callback
        def visit(hwnd, _):
            target = ctypes.c_uint32()
            user.GetWindowThreadProcessId(hwnd, ctypes.byref(target))
            identity = identities.get(target.value)
            if identity and control.identity(target.value)==identity and user.IsWindowVisible(hwnd):
                value = ctypes.c_uint32()
                hr = dwm.DwmGetWindowAttribute(hwnd,14,ctypes.byref(value),ctypes.sizeof(value))
                result['cloak_observations']['samples'] += 1
                if hr or not value.value:
                    result['cloak_observations']['failures'].append(dict(hwnd=int(hwnd),identity=identity,hr=hr,cloak=value.value))
            return 1
        if not user.EnumWindows(visit,0):
            raise RuntimeError('Own cloak window enumeration failed')
        if result['cloak_observations']['failures']:
            raise RuntimeError('Own visible window is not DWM cloaked; normal stop required')
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
            if (session.get('phase')=='running' and session.get('native_started') is True
                    and original_stereo_submitted(session)):
                if state.get('vr_gamepad', {}).get('source') != 1:
                    raise RuntimeError('No own Source1 input')
                for key in ('game_identity', 'owner_identity', 'xr_identity'):
                    identity = session[key]
                    if control.identity(identity['pid']) != identity:
                        raise RuntimeError('Readiness identity changed: '+key)
                result['running_identities'] = {key:session[key] for key in ('game_identity','owner_identity','xr_identity')}
                directory = OUTPUT/'projection'
                game_identity,xr_identity=session['game_identity'],session['xr_identity']
                def projection_live():
                    return control.identity(game_identity['pid'])==game_identity and control.identity(xr_identity['pid'])==xr_identity
                paired = ProjectionRecording(SELECTED/'xr-input/LocalAppData/OpenXR-Simulator',directory,projection_live)
                paired.start()
                result['projection_directory']=str(directory)
                # Two sequential frames each have the existing five-second
                # freshness deadline. Start after actual stereo, not merely
                # Native activation; allow those two bounded frame waits.
                ready_until=time.monotonic()+12
                while len(paired.frames)<2 and not paired.errors and time.monotonic()<ready_until:
                    check_background(*current());time.sleep(.05)
                if paired.errors or len(paired.frames)<2:
                    result['projection_stage_at_readiness_failure']=dict(paired.capture_stage)
                    raise RuntimeError('No two fresh actual projection frames before cover input: '+repr(paired.errors))
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
        # Finish GPU capture while the exact game/XR identities still exist.
        # Never reveal a window to record and never substitute desktop pixels.
        if paired:
            result['projection_stage_before_stop']=dict(paired.capture_stage)
            try:
                paired.stop()
                result['projection_frames']=len(paired.frames)
                result['projection_errors']=paired.errors
            except BaseException as error:
                result['projection_error']=repr(error)
            finally:
                result['projection_frames']=len(paired.frames)
                result['projection_errors']=list(paired.errors)
                result['projection_stages']=list(paired.capture_stages)
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
        if paired and paired.thread.is_alive():
            # Rejoin the SAME stopped capture after normal owner shutdown.
            # Preserve the initial error even if its pending I/O now finishes.
            try:paired.stop()
            except BaseException as error:result['projection_final_stop_error']=repr(error)
            result['projection_frames']=len(paired.frames)
            result['projection_errors']=list(paired.errors)
            result['projection_stages']=list(paired.capture_stages)
        if paired:result['projection_thread_joined']=not paired.thread.is_alive()
        result['normal_cleanup_verified'] = (session.get('native_cleanup_verified') is True
            and session.get('entry_restore_verified') is True and session.get('vr_gamepad_cleanup',{}).get('restored') is True
            and session.get('unified_weapons_cleanup',{}).get('drained') is True and session.get('xr_exit_code')==0
            and not result['remaining_owned'] and result['isolated_done'] is not None
            and result['isolated_done']['launcher_exit']==0 and result['isolated_done']['owned_game_absent'] is True)
        save()
    print(json.dumps({key:result.get(key) for key in ('error','operator_exit','paired_exit','normal_cleanup_verified','remaining_owned')}))
    return 0 if (not result.get('error') and not result.get('projection_error')
        and result.get('projection_frames',0)>1 and result['cloak_observations']['samples']>0
        and not result['cloak_observations']['failures'] and result['normal_cleanup_verified']) else 1


if __name__ == '__main__':
    raise SystemExit(main())
