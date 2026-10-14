"""Own one persistent strict stereo/XR session, or request its verified stop.

The game must already have the matching DLL reserved and target priming active.
No simulation commands are sent. Bounded diagnostic runners remain separate.
"""
import argparse
import ctypes
from ctypes import wintypes
import datetime
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import time
import uuid

from run_stereo_probe import ROOT, complete_csv_rows, collect_xr_evidence


class WindowsControl:
    def __init__(self):
        self.k=ctypes.WinDLL('kernel32',use_last_error=True)
        for name,args,result in (
            ('OpenProcess',[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD],wintypes.HANDLE),
            ('CloseHandle',[wintypes.HANDLE],wintypes.BOOL),
            ('GetProcessTimes',[wintypes.HANDLE,*([ctypes.POINTER(wintypes.FILETIME)]*4)],wintypes.BOOL),
            ('QueryFullProcessImageNameW',[wintypes.HANDLE,wintypes.DWORD,wintypes.LPWSTR,ctypes.POINTER(wintypes.DWORD)],wintypes.BOOL),
            ('GetExitCodeProcess',[wintypes.HANDLE,ctypes.POINTER(wintypes.DWORD)],wintypes.BOOL),
            ('CreateMutexW',[ctypes.c_void_p,wintypes.BOOL,wintypes.LPCWSTR],wintypes.HANDLE),
            ('OpenEventW',[wintypes.DWORD,wintypes.BOOL,wintypes.LPCWSTR],wintypes.HANDLE),
            ('SetEvent',[wintypes.HANDLE],wintypes.BOOL)):
            fn=getattr(self.k,name);fn.argtypes=args;fn.restype=result

    def identity(self,pid):
        handle=self.k.OpenProcess(0x1000,False,pid)
        if not handle:return None
        try:
            exit_code=wintypes.DWORD()
            if not self.k.GetExitCodeProcess(handle,ctypes.byref(exit_code)) or exit_code.value!=259:return None
            times=[wintypes.FILETIME() for _ in range(4)]
            if not self.k.GetProcessTimes(handle,*(ctypes.byref(t) for t in times)):
                raise ctypes.WinError(ctypes.get_last_error())
            path=ctypes.create_unicode_buffer(32768);size=wintypes.DWORD(len(path))
            if not self.k.QueryFullProcessImageNameW(handle,0,path,ctypes.byref(size)):
                raise ctypes.WinError(ctypes.get_last_error())
            return {'pid':int(pid),'creation_time':(times[0].dwHighDateTime<<32)|times[0].dwLowDateTime,
                    'image':str(pathlib.Path(path.value).resolve()).casefold()}
        finally:self.k.CloseHandle(handle)

    def owner_lock(self,pid):
        handle=self.k.CreateMutexW(None,False,f'Local\\GearsVR.Continuous.Owner.{pid}')
        error=ctypes.get_last_error()
        if not handle:raise ctypes.WinError(error)
        if error==183:
            self.k.CloseHandle(handle)
            raise RuntimeError('A continuous owner already exists for this game PID')
        return handle

    def close_lock(self,handle):self.k.CloseHandle(handle)

    def signal(self,name):
        handle=self.k.OpenEventW(2,False,name)
        if not handle:return False
        try:
            if not self.k.SetEvent(handle):raise ctypes.WinError(ctypes.get_last_error())
            return True
        finally:self.k.CloseHandle(handle)


def write_state(path,state):
    temporary=path.with_name(path.name+'.tmp')
    temporary.write_text(json.dumps(state,indent=2),encoding='utf-8')
    # A concurrent stop reader can briefly hold a Windows sharing lock.
    for attempt in range(6):
        try:
            os.replace(temporary,path)
            return
        except PermissionError:
            if attempt==5:raise
            time.sleep(.02)


def checked_identity(control,expected,allow_exited=False):
    actual=control.identity(expected['pid'])
    if actual is None and allow_exited:return False
    if actual!=expected:raise RuntimeError('Session process identity changed or exited: '+str(expected['pid']))
    return True


def native_command(args,owner):
    command=[sys.executable,str(ROOT/'tools/native-capture/start_capture.py'),'--pid',str(args.pid),
            '--build-dir',str(args.build_dir.resolve()),'--dll-name',args.dll_name,'--use-reserved',
            '--continuous-session','--continuous-owner-pid',str(owner['pid']),
            '--continuous-owner-creation-time',str(owner['creation_time']),
            '--submit-pair','--submit-background','--submit-output-bounds',
            '--viewport-diagnostics','--color-copy-diagnostics','--view-union-mask-fix',
            '--stereo-xr','--stereo-units-per-metre',str(args.stereo_units_per_metre),
            '--frame-link-diagnostics','--require-target-prime','--stereo-gpu-publish']
    if getattr(args,'outage_diagnostics',False):command.append('--outage-diagnostics')
    if getattr(args,'anchor_observer',False):command.append('--anchor-observer')
    if getattr(args,'origin_trial',False):command.append('--origin-trial')
    if getattr(args,'ui_observer',False):command.append('--ui-observer')
    return command


def xr_command(args,csv_path,owner):
    command=[str(args.xr_build_dir.resolve()/'OpenXRPoseProbe.exe'),'0',str(csv_path),
            '--stereo-game-pid',str(args.pid),'--continuous-session',
            '--owner-pid',str(owner['pid']),'--owner-creation-time',str(owner['creation_time'])]
    if getattr(args,'startup_menu_window',0):
        command+=['--startup-menu-window',str(args.startup_menu_window),str(args.pid)]
    for name in ('outage_diagnostics','shutdown_diagnostics'):
        if getattr(args,name,False):command.append('--'+name.replace('_','-'))
    if getattr(args,'origin_trial',False) or getattr(args,'image_ticket_diagnostics',False):command.append('--image-ticket-diagnostics')
    return command


def native_owner_matches(status,owner):
    return bool(status and owner and status.get('owner_pid')==str(owner['pid'])
                and status.get('owner_creation_time')==str(owner['creation_time']))


def startup_panel_submitted(path):
    for line in path.read_text(encoding='utf-8',errors='replace').splitlines():
        try:value=json.loads(line)
        except ValueError:continue
        if (isinstance(value,dict) and value.get('startup_menu_first_frame') is True
                and value.get('end_frame_succeeded') is True and value.get('panel_counts_as_stereo') is False):return True
    return False


def read_session_status(path,pid,floor):
    try:
        if path.stat().st_mtime_ns<=floor:return None
        text=path.read_text(encoding='utf-8',errors='replace')
        fields=dict(re.findall(r'(\w+)=([^\s]+)',text))
        if (not text.endswith('\n') or fields.get('complete')!='1' or int(fields.get('started_qpc','0'))<=0
                or fields.get('pid')!=str(pid) or fields.get('continuous_session')!='1'):return None
        return fields
    except (OSError,UnicodeError,ValueError):return None


def native_stopped_clean(status):
    """Exact native terminal contract; incomplete is never a clean stop."""
    state_ok=bool(status and (status.get('state_created')=='0' or
                  (status.get('state_created')=='1' and status.get('release_returned')=='1'
                   and status.get('state_destroyed')=='1' and int(status.get('state_flags','0'))&1)))
    return bool(state_ok and status.get('phase')=='stopped' and status.get('closed')=='1'
                and status.get('pending')=='0' and status.get('active')=='0'
                and status.get('publisher_stopped')=='1' and status.get('drain_ok')=='1'
                and status.get('close_ok')=='1' and status.get('cleanup_timeout')=='0'
                and status.get('resources_retained')=='0' and status.get('retained')=='0'
                and status.get('entries_restored')=='1' and status.get('simulator_operator')=='0')


def continuous_passed(state):
    evidence=state.get('continuous_session_evidence',{})
    owner=state.get('owner_identity',{})
    watched=state.get('continuous_owner_evidence',{})
    native=state.get('native_status',{})
    menu=state.get('startup_menu_evidence',{})
    handoff=state.get('startup_menu_handoff',{})
    startup_ok=not state.get('startup_menu_enabled') or (
        state.get('startup_panel_submitted') is True and menu.get('stereo_handoff') is True
        and menu.get('startup_quad_layers',0)>0 and handoff.get('end_frame_succeeded') is True
        and handoff.get('ticketed_projection_rendered') is True and handoff.get('one_way') is True)
    return bool(state.get('native_cleanup_verified') and state.get('entry_restore_verified')
                and startup_ok
                and owner and watched.get('owner_pid')==owner.get('pid')
                and watched.get('owner_creation_time')==owner.get('creation_time') and watched.get('owner_lost') is False
                and native.get('owner_exit_observed')=='0' and native.get('owner_error_observed')=='0'
                and state.get('xr_exit_code')==0 and evidence.get('orderly_stop') is True
                and evidence.get('shutdown_completed') is True and evidence.get('structural_failure') is False
                and evidence.get('submitted_frames',0)>0 and evidence.get('scene_copies',0)>0
                and not state.get('error') and not state.get('unsafe_xr_teardown')
                and not state.get('native_cleanup_error') and not state.get('xr_cleanup_error'))


def request_stop(path,timeout,control=None):
    control=control or WindowsControl()
    state=json.loads(path.read_text(encoding='utf-8'))
    if state.get('schema')!=1 or state.get('kind')!='continuous_strict_stereo' or not state.get('session_id'):
        raise RuntimeError('Not a continuous owner session report')
    if state.get('phase') in ('stopped','incomplete','cancelled'):
        return 0 if state.get('phase')=='cancelled' or state.get('phase')=='stopped' and state.get('passed') else 1
    checked_identity(control,state['owner_identity'])
    checked_identity(control,state['game_identity'])
    if state.get('xr_identity'):checked_identity(control,state['xr_identity'],allow_exited=True)
    if not control.signal(f"Local\\GearsVR.Capture.Stop.{state['game_identity']['pid']}") and state.get('phase')!='stopping':
        raise RuntimeError('Native stop event absent; owner has not activated capture or is already cleaning up')
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        current=json.loads(path.read_text(encoding='utf-8'))
        if current.get('session_id')!=state['session_id']:raise RuntimeError('Session report identity changed')
        if current.get('phase') in ('stopped','incomplete','cancelled'):
            return 0 if current.get('phase')=='cancelled' or current.get('phase')=='stopped' and current.get('passed') else 1
        checked_identity(control,state['owner_identity'])
        time.sleep(.2)
    raise TimeoutError('Stop requested but joint native/XR cleanup not confirmed within deadline')


class StartupCancelled(Exception):
    pass


def request_recenter(path,control=None):
    control=control or WindowsControl()
    state=json.loads(path.read_text(encoding='utf-8'))
    if state.get('schema')!=1 or state.get('kind')!='continuous_strict_stereo' or state.get('phase')!='running':
        raise RuntimeError('Recenter requires a running continuous stereo session')
    for key in ('owner_identity','game_identity','xr_identity'):checked_identity(control,state[key])
    if not control.signal(f"Local\\GearsVR.Stereo.Recenter.{state['game_identity']['pid']}"):
        raise RuntimeError('Stereo recenter control is unavailable in this native build')
    print('Recenter requested; the first eligible view pair with a reliable horizontal heading can commit the new heading and position.',flush=True)
    return 0


def cleanup_owned_xr(child,owned_pid,game_pid,timeout,control,state):
    """Use the retained Popen handle; PID lookup races with orderly exit."""
    state['xr_cleanup_actions']=[]
    state['xr_forced_cleanup_reason']=None
    if child.pid!=owned_pid:
        state['xr_cleanup_error']='Owned XR Popen PID invariant changed'
        return
    try:
        if child.poll() is None:
            try:
                state['xr_stop_signaled']=control.signal(f'Local\\GearsVR.XR.Stop.{game_pid}')
                state['xr_cleanup_actions'].append('stop_signaled' if state['xr_stop_signaled'] else 'stop_event_absent')
            except Exception as error:
                state['xr_stop_signal_error']=repr(error)
                state['xr_cleanup_error']=repr(error)
                state['xr_cleanup_actions'].append('stop_signal_error')
        state['xr_cleanup_actions'].append('wait')
        state['xr_exit_code']=child.wait(timeout=timeout)
    except subprocess.TimeoutExpired as error:
        state['xr_cleanup_error']=repr(error)
        state['xr_forced_cleanup_reason']='cleanup_wait_timeout'
        # Only a real wait timeout permits escalation, against our own handle.
        try:
            if child.poll() is None:
                state['xr_cleanup_actions'].append('terminate_owned_child')
                child.terminate()
            try:
                state['xr_exit_code']=child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                state['xr_cleanup_actions'].append('kill_owned_child')
                child.kill()
                state['xr_exit_code']=child.wait(timeout=5)
        except Exception as force_error:
            state['xr_force_cleanup_error']=repr(force_error)
            state['xr_exit_code']=child.returncode
    except Exception as error:
        state['xr_cleanup_error']=repr(error)
        state['xr_exit_code']=child.returncode


def run_session(args,control=None,on_report=None,cancel_requested=None,before_native_activation=None):
    control=control or WindowsControl()
    lock=control.owner_lock(args.pid)
    token=uuid.uuid4().hex
    prefix=ROOT/'analysis/runtime'/('continuous-stereo-'+str(args.pid)+'-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'-'+token[:8])
    prefix.parent.mkdir(parents=True,exist_ok=True)
    state_path=prefix.with_suffix('.json')
    state={'schema':1,'kind':'continuous_strict_stereo','session_id':token,'phase':'starting','passed':False,
           'source':0,'operator_motion_requested':False,'simulation_not_physical_tracking':False,
           'physical_tracking_verified':False,
           'outage_diagnostics_enabled':bool(getattr(args,'outage_diagnostics',False)),
           'anchor_observer_enabled':bool(getattr(args,'anchor_observer',False)),
           'origin_trial_enabled':bool(getattr(args,'origin_trial',False)),
           'image_ticket_diagnostics_enabled':bool(getattr(args,'origin_trial',False) or getattr(args,'image_ticket_diagnostics',False)),
           'shutdown_diagnostics_enabled':bool(getattr(args,'shutdown_diagnostics',False)),
           'startup_menu_enabled':bool(getattr(args,'startup_menu_window',0)),
           'ui_observer_enabled':bool(getattr(args,'ui_observer',False)),
           'owner_identity':None,'game_identity':None,
           'session_report':str(state_path),'native_started':False,
           'limits':'Explicit persistent session lifecycle; exit0 does not assert uninterrupted frame rate or physical headset acceptance.'}
    child=None;activation_attempted=False
    native_status=args.build_dir.resolve()/'session-status.txt'
    floor=0
    out_path=prefix.with_suffix('.stdout.txt');err_path=prefix.with_suffix('.stderr.txt')
    try:
        state['owner_identity']=control.identity(os.getpid());state['game_identity']=control.identity(args.pid)
        floor=native_status.stat().st_mtime_ns if native_status.exists() else 0
        if not state['game_identity'] or not state['owner_identity']:raise RuntimeError('Live owner/game identity required')
        state['xr_command']=xr_command(args,prefix.with_suffix('.csv'),state['owner_identity'])
        state['native_command']=native_command(args,state['owner_identity'])
        write_state(state_path,state)
        if on_report:on_report(state_path,state['owner_identity'],state['game_identity'])
        print(json.dumps({'session_report':str(state_path),'stop_command':[sys.executable,str(pathlib.Path(__file__).resolve()),'stop','--session',str(state_path)]}),flush=True)
        with out_path.open('w') as out,err_path.open('w') as err:
            try:
                if cancel_requested and cancel_requested():raise StartupCancelled()
                # No Operator layer or pose calls are added to this strict mode.
                child=subprocess.Popen(state['xr_command'],stdout=out,stderr=err)
                state['xr_owned_pid']=child.pid
                state['xr_identity']=control.identity(child.pid)
                if not state['xr_identity']:raise RuntimeError('Owned XR identity could not be established')
                write_state(state_path,state)
                deadline=time.monotonic()+args.startup_timeout
                while time.monotonic()<deadline and child.poll() is None:
                    if cancel_requested and cancel_requested():raise StartupCancelled()
                    path=prefix.with_suffix('.csv')
                    rows=complete_csv_rows(path) if path.exists() else []
                    panel_ready=not state['startup_menu_enabled'] or startup_panel_submitted(out_path)
                    if panel_ready and len(rows)>=30 and all(r.get('stereo_published')=='1' and r.get('session_state')=='5'
                        and r.get('should_render')=='1' and int(r.get('head_flags') or 0)&15==15
                        and int(r.get('view_flags') or 0)&15==15 and r.get('stereo_source')=='0' for r in rows[-20:]):
                        state['preflight_frames']=len(rows)
                        if state['startup_menu_enabled']:state['startup_panel_submitted']=True
                        break
                    checked_identity(control,state['game_identity'])
                    time.sleep(.1)
                else:raise RuntimeError('Strict focused XR source0 or startup panel preflight failed; native remains inactive')
                if before_native_activation:
                    state['phase']='awaiting_level';write_state(state_path,state)
                    def ensure_startup_live():
                        if cancel_requested and cancel_requested():raise StartupCancelled()
                        checked_identity(control,state['game_identity'])
                        if child.poll() is not None:raise RuntimeError('XR exited while waiting for the level')
                    ensure_startup_live()
                    before_native_activation(ensure_startup_live)
                    ensure_startup_live()
                    state['phase']='starting';write_state(state_path,state)
                checked_identity(control,state['game_identity'])
                if cancel_requested and cancel_requested():raise StartupCancelled()
                activation_attempted=True
                with prefix.with_suffix('.native-start.log').open('w') as log:
                    result=subprocess.run(state['native_command'],stdout=log,stderr=subprocess.STDOUT,timeout=90)
                if result.returncode:raise RuntimeError('Native activation failed; seeking cleanup without retry')
                state['native_started']=True
                deadline=time.monotonic()+15
                while time.monotonic()<deadline:
                    status=read_session_status(native_status,args.pid,floor)
                    if status and status.get('phase')=='running':
                        if status.get('simulator_operator')!='0':raise RuntimeError('Continuous native source profile must be strict')
                        if not native_owner_matches(status,state['owner_identity']):raise RuntimeError('Native owner identity does not match this launcher')
                        break
                    if status and status.get('phase') in ('incomplete','stopped'):raise RuntimeError('Native session stopped before running')
                    if child.poll() is not None:raise RuntimeError('XR exited during native startup')
                    time.sleep(.1)
                else:raise TimeoutError('Fresh native running status absent')
                state['phase']='running';state['native_status']=status;state['native_started_qpc']=status['started_qpc'];write_state(state_path,state)
                print(json.dumps({'continuous_running':True,'session_report':str(state_path)}),flush=True)
                while child.poll() is None:
                    if cancel_requested and cancel_requested():
                        state['stop_requested_by']='launcher_cancel';break
                    status=read_session_status(native_status,args.pid,floor)
                    if status and status.get('phase')!='running':break
                    checked_identity(control,state['game_identity'])
                    time.sleep(.25)
            except StartupCancelled:
                state['stop_requested_by']='startup_cancel'
            except KeyboardInterrupt:
                state['stop_requested_by']='keyboard_interrupt'
            except Exception as error:
                state['error']=repr(error)
            finally:
                state['phase']='stopping';write_state(state_path,state)
                if activation_attempted:
                    try:
                        if checked_identity(control,state['game_identity'],allow_exited=True):
                            state['native_stop_signaled']=control.signal(f'Local\\GearsVR.Capture.Stop.{args.pid}')
                        deadline=time.monotonic()+args.cleanup_timeout
                        while time.monotonic()<deadline:
                            status=read_session_status(native_status,args.pid,floor)
                            if status and status.get('phase') in ('stopped','incomplete'):break
                            time.sleep(.2)
                        state['native_status']=status
                        if state.get('native_started_qpc') and status and status.get('started_qpc')!=state['native_started_qpc']:
                            raise RuntimeError('Native session started_qpc identity changed during cleanup')
                        state['native_cleanup_verified']=native_stopped_clean(status) and native_owner_matches(status,state['owner_identity'])
                        if not state['native_cleanup_verified']:raise RuntimeError('Native cleanup incomplete or deadline expired')
                        shutil.copyfile(native_status,prefix.with_suffix('.session-status.txt'))
                        capture=args.build_dir.resolve()/'capture-status.txt'
                        shutil.copyfile(capture,prefix.with_suffix('.native-status.txt'))
                        restored=prefix.with_suffix('.restored.json')
                        with prefix.with_suffix('.verify.log').open('w') as log:
                            verified=subprocess.run([sys.executable,str(ROOT/'tools/native-capture/verify_stopped.py'),
                                '--pid',str(args.pid),'--status',str(prefix.with_suffix('.native-status.txt')),
                                '--output',str(restored)],stdout=log,stderr=subprocess.STDOUT,timeout=20)
                        state['entry_restore_verified']=verified.returncode==0 and json.loads(restored.read_text())['all_match']
                    except Exception as error:
                        state['native_cleanup_error']=repr(error)
                        state['native_cleanup_verified']=False
                if child:
                    cleanup_owned_xr(child,state['xr_owned_pid'],args.pid,args.cleanup_timeout,control,state)
        collect_xr_evidence(out_path,state)
        for line in out_path.read_text(encoding='utf-8',errors='replace').splitlines():
            try:value=json.loads(line)
            except ValueError:continue
            if isinstance(value,dict) and value.get('continuous_session_evidence'):
                state['continuous_session_evidence']=value
                state['simulation_not_physical_tracking']=value.get('simulation_not_physical_tracking') is True
            if isinstance(value,dict) and value.get('session_exit_evidence'):state['session_exit_evidence']=value
            if isinstance(value,dict) and value.get('continuous_owner_evidence'):state['continuous_owner_evidence']=value
            if isinstance(value,dict) and value.get('startup_menu_evidence'):state['startup_menu_evidence']=value
            if isinstance(value,dict) and value.get('startup_menu_handoff'):state['startup_menu_handoff']=value
        state['passed']=continuous_passed(state)
        # Cancellation before native activation is a control result, not proof
        # of stereo output. The launcher still owns startup-prime verification.
        xr_shutdown=state.get('continuous_session_evidence',{}).get('shutdown_completed') is True
        state['startup_cancelled']=bool(state.get('stop_requested_by')=='startup_cancel' and not activation_attempted
            and (child is None or xr_shutdown) and not state.get('error') and not state.get('xr_cleanup_error'))
        state['phase']='cancelled' if state['startup_cancelled'] else 'stopped' if state['passed'] else 'incomplete'
    except Exception as error:
        state['error']=repr(error);state['phase']='incomplete'
    finally:
        write_state(state_path,state);control.close_lock(lock)
    print(json.dumps(state),flush=True)
    return 0 if state['passed'] or state.get('startup_cancelled') else 1


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    commands=parser.add_subparsers(dest='command',required=True)
    start=commands.add_parser('start')
    start.add_argument('--pid',type=int,required=True)
    start.add_argument('--build-dir',type=pathlib.Path,required=True)
    start.add_argument('--dll-name',required=True)
    start.add_argument('--xr-build-dir',type=pathlib.Path,required=True)
    start.add_argument('--stereo-units-per-metre',type=float,default=100.)
    start.add_argument('--startup-timeout',type=float,default=45.)
    start.add_argument('--cleanup-timeout',type=float,default=35.)
    start.add_argument('--anchor-observer','--anchor-diagnostics',dest='anchor_observer',action='store_true')
    start.add_argument('--origin-trial',action='store_true')
    start.add_argument('--image-ticket-diagnostics',action='store_true')
    start.add_argument('--outage-diagnostics',action='store_true')
    start.add_argument('--shutdown-diagnostics',action='store_true')
    stop=commands.add_parser('stop')
    stop.add_argument('--session',type=pathlib.Path,required=True)
    stop.add_argument('--timeout',type=float,default=95.)
    recenter=commands.add_parser('recenter')
    recenter.add_argument('--session',type=pathlib.Path,required=True)
    args=parser.parse_args(argv)
    if args.command=='stop':
        if args.timeout<=0:parser.error('Stop deadline must be positive')
        return request_stop(args.session.resolve(),args.timeout)
    if args.command=='recenter':return request_recenter(args.session.resolve())
    if args.pid<=0 or args.startup_timeout<=0 or args.cleanup_timeout<=0:parser.error('PID and deadlines must be positive')
    if args.origin_trial and not args.anchor_observer:parser.error('Origin trial requires --anchor-observer')
    if pathlib.Path(args.dll_name).name!=args.dll_name:parser.error('DLL basename required')
    if not 10<=args.stereo_units_per_metre<=500:parser.error('Provisional scale must be between 10 and 500')
    return run_session(args)


if __name__=='__main__':
    sys.exit(main())
