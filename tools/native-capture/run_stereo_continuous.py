"""Own one persistent strict stereo/XR session, or request its verified stop.

The game must already have the matching DLL reserved and target priming active.
No simulation commands are sent. Bounded diagnostic runners remain separate.
"""
import argparse
import ctypes
import contextlib
from ctypes import wintypes
import datetime
import json
import math
import os
import pathlib
import re
import shutil
import subprocess
import sys
import time
import uuid

from run_stereo_probe import ROOT, complete_csv_rows, collect_xr_evidence
from camera_closer_options import camera_closer_metres
from session_telemetry import SessionTelemetry

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

    def game_absent(self,pid):
        """A failed identity lookup alone cannot prove the game has exited."""
        handle=self.k.OpenProcess(0x1000,False,pid)
        if not handle:return ctypes.get_last_error()==87  # ERROR_INVALID_PARAMETER: no such PID
        try:
            code=wintypes.DWORD()
            return bool(self.k.GetExitCodeProcess(handle,ctypes.byref(code)) and code.value!=259)
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


def preserve_native_diagnostics(build,prefix,state):
    """Raw failure evidence only; saving a file never verifies native cleanup."""
    receipts={}
    for name,suffix in (('session-status.txt','.session-status.txt'),('capture-status.txt','.native-status.txt')):
        source=build/name;destination=prefix.with_suffix(suffix)
        receipt={'source':str(source),'path':str(destination),'saved':False,
                 'identity_verified':False,'acceptance_evidence':False}
        try:
            shutil.copyfile(source,destination)
            receipt['saved']=True
        except OSError as error:
            receipt['error']=repr(error)
        receipts[name]=receipt
    state['native_diagnostics']=receipts
    return all(receipt['saved'] for receipt in receipts.values())


def checked_identity(control,expected,allow_exited=False):
    actual=control.identity(expected['pid'])
    if actual is None and allow_exited:return False
    if actual!=expected:raise RuntimeError('Session process identity changed or exited: '+str(expected['pid']))
    return True


def start_path_choice_position(args,state,native_status,floor,control):
    if not getattr(args,'path_choice_center',False):return None
    try:
        # Native99 support is explicit; never fall back to FCanvas or the old trial.
        if str(state['native_status'].get('path_position_service_compiled'))!='1':
            raise ValueError('Native99 PathChoice position service required')
        from path_choice_position_monitor import PathChoicePositionMonitor
        def read_path_status():
            diagnostic={}
            status=read_session_status(native_status,args.pid,floor,diagnostic=diagnostic)
            if status is None:
                error=ValueError('Missing native status: '+json.dumps(diagnostic or {'reason':'unknown'},
                                                                     sort_keys=True,separators=(',',':')))
                error.native_status_diagnostic=diagnostic or {'reason':'unknown'}
                raise error
            return status
        return PathChoicePositionMonitor(args.build_dir.resolve()/'path-choice-position-trial.ini',
            args.build_dir.resolve()/'path-choice-close-observer.ini',state,
            read_path_status,control.identity)
    except Exception as error:
        state['path_choice_center_start_error']=repr(error)
        return None


def close_path_choice_position(monitor,state):
    if monitor is None:return
    try:monitor.close()
    except Exception as error:state['path_choice_center_close_error']=repr(error)
    if monitor.cleanup_error:state['path_choice_center_cleanup_error']=monitor.cleanup_error
    state['path_choice_center_worker_joined']=monitor.worker_joined


def finish_path_choice_position(monitor,state,control):
    if monitor is None:return
    try:
        # Independently proven process absence; missing native status is insufficient.
        verified=(state.get('native_cleanup_verified') is True and state.get('entry_restore_verified') is True)
        absent=False
        if not verified:
            absent=getattr(control,'game_absent',lambda pid:False)(state['game_identity']['pid']) is True
        state['path_choice_center_game_absent_verified']=absent
        state['path_choice_center_sidecars_cleaned']=monitor.finish_cleanup(
            native_cleanup_verified=state.get('native_cleanup_verified') is True,
            entry_restore_verified=state.get('entry_restore_verified') is True,game_absent=absent)
    except Exception as error:state['path_choice_center_cleanup_error']=repr(error)
    if monitor.cleanup_error:state['path_choice_center_cleanup_error']=monitor.cleanup_error
    state['path_choice_center_worker_joined']=monitor.worker_joined


def validate_render_scale(args):
    value=getattr(args,'render_scale',None)
    if value is not None and (isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not .25<=value<=2):
        raise ValueError('render_scale must be finite and between 0.25 and 2.0')
    return value


def validate_ui_hud_plane(args):
    validate_render_scale(args)
    camera_closer_metres(args)
    if getattr(args,'ui_reticle_late_sample',False) and not all(getattr(args,key,False) for key in ('ui_observer','ui_hud_plane')):
        raise ValueError('Late reticle sampling requires UI observer and HUD plane in a continuous session')
    if getattr(args,'path_choice_center',False) and not all(getattr(args,k,False) for k in ('menu_recovery','ui_observer','anchor_observer')):
        raise ValueError('PathChoice centering requires menu/UI/anchor observers')
    if getattr(args,'crosshair_assignment_observer',False) and (not all(
            getattr(args,key,False) for key in ('ui_observer','anchor_observer')) or getattr(args,'origin_trial',False)):
        raise ValueError('Crosshair assignment observer requires continuous UI/anchor observers and cannot use origin trial')
    value=getattr(args,'hud_horizontal_fov_degrees',None)
    if value is not None:
        if not getattr(args,'ui_hud_plane',False):raise ValueError('HUD size requires --ui-hud-plane')
        if isinstance(value,bool) or not isinstance(value,(int,float)) or not 45<=value<=100 or not math.isfinite(value):
            raise ValueError('hud_horizontal_fov_degrees must be finite and between 45 and 100')
    for key,minimum,maximum in (('hud_element_scale',1,2),('hud_center_scale',0,1)):
        value=getattr(args,key,None)
        if value is not None:
            if not getattr(args,'ui_hud_plane',False):raise ValueError('HUD layout requires --ui-hud-plane')
            if isinstance(value,bool) or not isinstance(value,(int,float)) or not minimum<=value<=maximum or not math.isfinite(value):
                raise ValueError(f'{key} must be finite and between {minimum} and {maximum}')
    if getattr(args,'cinematic_camera',False) and (not getattr(args,'anchor_observer',False) or getattr(args,'origin_trial',False)):
        raise ValueError('Cinematic camera requires anchor observer and cannot use the Pawn origin trial')
    if getattr(args,'world_recovery',False):
        if not all(getattr(args,key,False) for key in ('anchor_observer','ui_observer','cinematic_camera')):
            raise ValueError('World recovery requires anchor/UI observers and cinematic camera')
        digest=getattr(args,'native_expected_sha256',None)
        if not isinstance(digest,str) or len(digest)!=64 or any(ch not in '0123456789abcdef' for ch in digest):
            raise ValueError('World recovery requires trusted native manifest SHA256')
    # Every run_session path is continuous; direct callers must validate before
    # creating the owner lock, writing reports, or launching either process.
    if getattr(args,'ui_hud_plane',False) and not (
            getattr(args,'ui_observer',False) and getattr(args,'anchor_observer',False)):
        raise ValueError('HUD plane requires --ui-observer and --anchor-observer in a continuous session')


def native_command(args,owner):
    validate_ui_hud_plane(args)
    command=[sys.executable,str(ROOT/'tools/native-capture/start_capture.py'),'--pid',str(args.pid),
            '--build-dir',str(args.build_dir.resolve()),'--dll-name',args.dll_name,'--use-reserved',
            '--continuous-session','--continuous-owner-pid',str(owner['pid']),
            '--continuous-owner-creation-time',str(owner['creation_time']),
            '--submit-pair','--submit-background','--submit-output-bounds',
            '--viewport-diagnostics','--color-copy-diagnostics','--view-union-mask-fix',
            '--stereo-xr','--stereo-units-per-metre',str(args.stereo_units_per_metre),
            '--frame-link-diagnostics','--require-target-prime','--stereo-gpu-publish']
    if camera_closer_metres(args)>0:command+=['--camera-closer-metres',str(camera_closer_metres(args))]
    if getattr(args,'first_person_preview',False):command.append('--first-person-preview')
    if getattr(args,'outage_diagnostics',False):command.append('--outage-diagnostics')
    for key in ('startup_selftest_proof','startup_selftest_digest','startup_selftest_nonce'):
        if getattr(args,key,None) is not None:command+=['--'+key.replace('_','-'),str(getattr(args,key))]
    if getattr(args,'anchor_observer',False):command.append('--anchor-observer')
    if getattr(args,'cinematic_camera',False):command.append('--cinematic-camera')
    if getattr(args,'origin_trial',False):command.append('--origin-trial')
    if getattr(args,'ui_observer',False):command.append('--ui-observer')
    if getattr(args,'ui_hud_plane',False):command.append('--ui-hud-plane')
    if getattr(args,'ui_reticle_late_sample',False):command.append('--ui-reticle-late-sample')
    if getattr(args,'crosshair_assignment_observer',False):command.append('--crosshair-assignment-observer')
    if getattr(args,'hud_horizontal_fov_degrees',None) is not None:
        command+=['--hud-horizontal-fov-degrees',str(args.hud_horizontal_fov_degrees)]
    for key in ('hud_element_scale','hud_center_scale'):
        value=getattr(args,key,None)
        if value is not None:command+=['--'+key.replace('_','-'),str(value)]
    return command


def xr_environment(parent):
    env=dict(parent)
    env['XR_SIMULATOR_NO_ACTIVATE']='1'
    return env


def xr_command(args,csv_path,owner):
    command=[str(args.xr_build_dir.resolve()/'OpenXRPoseProbe.exe'),'0',str(csv_path),
            '--stereo-game-pid',str(args.pid),'--continuous-session',
            '--owner-pid',str(owner['pid']),'--owner-creation-time',str(owner['creation_time'])]
    if validate_render_scale(args) is not None:command+=['--render-scale',str(args.render_scale)]
    if getattr(args,'startup_menu_window',0):
        command+=['--startup-menu-window',str(args.startup_menu_window),str(args.pid)]
    if getattr(args,'menu_recovery',False):
        if not getattr(args,'startup_menu_window',0):raise ValueError('Menu recovery requires startup menu window')
        command+=['--menu-state-file',str(args.menu_state_file),'--menu-game-creation',str(args.menu_game_creation)]
    if getattr(args,'movie_display',False):
        if not getattr(args,'menu_recovery',False):raise ValueError('Movie display requires menu recovery')
        command.append('--movie-display')
    for name in ('outage_diagnostics','shutdown_diagnostics'):
        if getattr(args,name,False):command.append('--'+name.replace('_','-'))
    if getattr(args,'origin_trial',False) or getattr(args,'image_ticket_diagnostics',False):command.append('--image-ticket-diagnostics')
    if getattr(args,'gpu_snapshot',False):command+=['--gpu-snapshot',str(csv_path.with_suffix('.gpu.ppm'))]
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


def first_stereo_submitted(path):
    """Existing XR marker is emitted after successful xrEndFrame, not running."""
    try:lines=path.read_text(encoding='utf-8',errors='replace').splitlines()
    except OSError:return None # Diagnostic acquisition must not stop VR.
    for line in lines:
        try:value=json.loads(line)
        except ValueError:continue
        if (isinstance(value,dict) and value.get('reference_epoch_first_projection') is True
                and type(value.get('space_epoch')) is int and value['space_epoch']==1
                and value.get('end_frame_succeeded') is True
                and type(value.get('monotonic_ns')) is int and value['monotonic_ns']>0):return value
    return None


@contextlib.contextmanager
def open_session_status(path):
    """One immutable published generation; Windows permits atomic replacement."""
    if os.name != 'nt':
        with path.open('r',encoding='utf-8',errors='replace') as stream:yield stream
        return
    import msvcrt
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,ctypes.c_void_p,
                                wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]
    kernel.CreateFileW.restype=wintypes.HANDLE
    kernel.CloseHandle.argtypes=[wintypes.HANDLE];kernel.CloseHandle.restype=wintypes.BOOL
    handle=kernel.CreateFileW(str(path),0x80000000,1|2|4,None,3,0x80,None)
    if handle==wintypes.HANDLE(-1).value:raise ctypes.WinError(ctypes.get_last_error())
    try:fd=msvcrt.open_osfhandle(handle,os.O_RDONLY|os.O_BINARY)
    except BaseException:
        kernel.CloseHandle(handle);raise
    # Ownership moved to the CRT descriptor; each failure path closes once.
    try:stream=os.fdopen(fd,'r',encoding='utf-8',errors='replace')
    except BaseException:
        os.close(fd);raise
    with stream:yield stream


def read_session_status(path,pid,floor,*,diagnostic=None):
    """Same admission as before; optional bounded reasons from this exact read.

    No second read, retry, raw contents, paths, owner IDs or timestamp values
    are retained. Missing diagnostic data remains explicitly unknown.
    """
    if diagnostic is not None:diagnostic.clear()
    def rejected(reason,**details):
        if diagnostic is not None:diagnostic.update(reason=reason,**details)
        return None
    stage='open'
    try:
        with open_session_status(path) as stream:
            stage='stat'
            if os.fstat(stream.fileno()).st_mtime_ns<=floor:return rejected('stale_file')
            stage='read'
            text=stream.read()
        if diagnostic is not None:diagnostic.update(text_length=len(text),ends_with_newline=text.endswith('\n'))
        fields=dict(re.findall(r'(\w+)=([^\s]+)',text))
        if not text:return rejected('empty_file')
        if not text.endswith('\n'):return rejected('missing_final_newline')
        if fields.get('complete')!='1':return rejected('complete_marker_missing_or_invalid')
        stage='started_qpc'
        if int(fields.get('started_qpc','0'))<=0:return rejected('started_qpc_missing_or_nonpositive')
        if fields.get('pid')!=str(pid):return rejected('pid_mismatch',field_present='pid' in fields)
        if fields.get('continuous_session')!='1':return rejected('continuous_session_mismatch',field_present='continuous_session' in fields)
        if diagnostic is not None:diagnostic.update(reason='accepted')
        return fields
    except OSError as error:
        return rejected('io_error',stage=stage,exception_type=type(error).__name__,
                        errno=error.errno,winerror=getattr(error,'winerror',None))
    except UnicodeError as error:
        return rejected('unicode_error',stage=stage,exception_type=type(error).__name__)
    except ValueError as error:
        return rejected('invalid_started_qpc' if stage=='started_qpc' else 'invalid_value',
                        stage=stage,exception_type=type(error).__name__)


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
    recovery=state.get('menu_recovery_enabled',False)
    recovery_evidence=state.get('menu_recovery_evidence',{})
    handoff_mode_ok=(recovery is False and handoff.get('one_way') is True) or (
        recovery is True and handoff.get('one_way') is False
        and recovery_evidence.get('menu_recovery_evidence') is True
        and recovery_evidence.get('panel_counts_as_stereo') is False)
    startup_ok=(not state.get('startup_menu_enabled') and recovery is False) or (
        state.get('startup_menu_enabled') is True
        and state.get('startup_panel_submitted') is True and menu.get('stereo_handoff') is True
        and menu.get('startup_quad_layers',0)>0 and handoff.get('end_frame_succeeded') is True
        and handoff.get('ticketed_projection_rendered') is True and handoff_mode_ok)
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


def request_snapshot(path,control=None):
    """One diagnostic GPU readback from an explicitly enabled, live XR owner."""
    control=control or WindowsControl()
    state=json.loads(path.read_text(encoding='utf-8'))
    if (state.get('schema')!=1 or state.get('kind')!='continuous_strict_stereo' or
            state.get('phase')!='running' or state.get('gpu_snapshot_requested') is not True):
        raise RuntimeError('Snapshot requires a running session launched with --gpu-snapshot')
    for key in ('owner_identity','game_identity','xr_identity'):checked_identity(control,state[key])
    xr_pid=state['xr_identity']['pid']
    if not control.signal(f'Local\\GearsVR.XR.Snapshot.{xr_pid}'):
        raise RuntimeError('Snapshot control unavailable in this XR build')
    print(json.dumps({'gpu_snapshot':'requested','xr_pid':xr_pid,'saved_verified':False}),flush=True)
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
    timing_origin=time.perf_counter_ns()
    validate_ui_hud_plane(args)
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
           'cinematic_camera_enabled':bool(getattr(args,'cinematic_camera',False)),
           'origin_trial_enabled':bool(getattr(args,'origin_trial',False)),
           'image_ticket_diagnostics_enabled':bool(getattr(args,'origin_trial',False) or getattr(args,'image_ticket_diagnostics',False)),
           'shutdown_diagnostics_enabled':bool(getattr(args,'shutdown_diagnostics',False)),
           'startup_menu_enabled':bool(getattr(args,'startup_menu_window',0)),
           'world_recovery_enabled':bool(getattr(args,'world_recovery',False)),
           'menu_recovery_enabled':bool(getattr(args,'menu_recovery',False)),
           'movie_display_enabled':bool(getattr(args,'movie_display',False)),
           'ui_observer_enabled':bool(getattr(args,'ui_observer',False)),
           'ui_hud_plane_enabled':bool(getattr(args,'ui_hud_plane',False)),
           'ui_reticle_late_sample_enabled':bool(getattr(args,'ui_reticle_late_sample',False)),
           'crosshair_assignment_observer_enabled':bool(getattr(args,'crosshair_assignment_observer',False)),
           'hud_horizontal_fov_degrees':getattr(args,'hud_horizontal_fov_degrees',None),
           'camera_closer_metres':camera_closer_metres(args),
           'gpu_snapshot_requested':bool(getattr(args,'gpu_snapshot',False)),
           'owner_identity':None,'game_identity':None,
           'session_report':str(state_path),'native_started':False,
           'limits':'Explicit persistent session lifecycle; exit0 does not assert uninterrupted frame rate or physical headset acceptance.'}
    state.update({key:getattr(args,key) for key in ('hud_element_scale','hud_center_scale') if getattr(args,key,None) is not None})
    state['diagnostics_artifacts']=getattr(args,'diagnostics_artifacts',{})
    child=None;activation_attempted=False;menu_monitor=None;world_monitor=None;world_log=None;telemetry=None;path_center=None;right_stick_policy=None
    state['path_choice_center_enabled']=bool(getattr(args,'path_choice_center',False))
    native_status=args.build_dir.resolve()/'session-status.txt'
    floor=0
    out_path=prefix.with_suffix('.stdout.txt');err_path=prefix.with_suffix('.stderr.txt')
    state['startup_timings']={}
    def stage_completed(name,started,evidence,actual_ns=None):
        if name in state['startup_timings']:return
        observed=time.perf_counter_ns()
        measured=observed if actual_ns is None else actual_ns
        entry={'elapsed_ms':(measured-started)/1000000,'since_runner_start_ms':(measured-timing_origin)/1000000,
               'evidence':evidence,'clock':'Windows QPC / perf_counter_ns','timing_is_runner_observation':actual_ns is None}
        if actual_ns is not None:entry.update(xr_monotonic_ns=actual_ns,runner_observation_delay_ms=(observed-actual_ns)/1000000)
        state['startup_timings'][name]=entry;write_state(state_path,state)
        if telemetry:telemetry.event('startup_stage',stage=name,elapsed_ms=entry['elapsed_ms'],since_runner_start_ms=entry['since_runner_start_ms'])
        print(json.dumps({'startup_stage':name,**entry}),flush=True)
    def observe_first_stereo(started):
        if 'first_stereo_submission' in state['startup_timings']:return
        marker=first_stereo_submitted(out_path)
        if marker and started<=marker['monotonic_ns']<=time.perf_counter_ns():
            stage_completed('first_stereo_submission',started,'XR epoch1 actual projection successful xrEndFrame marker',marker['monotonic_ns'])
    try:
        state['owner_identity']=control.identity(os.getpid());state['game_identity']=control.identity(args.pid)
        floor=native_status.stat().st_mtime_ns if native_status.exists() else 0
        if not state['game_identity'] or not state['owner_identity']:raise RuntimeError('Live owner/game identity required')
        telemetry=SessionTelemetry(prefix.with_suffix('.telemetry.jsonl'),prefix.with_suffix('.csv'),
            args.build_dir.resolve()/'swapchain-output-status.txt',state['game_identity'],state['owner_identity'])
        state['diagnostics']=telemetry.summary()
        if getattr(args,'menu_recovery',False):
            args.menu_state_file=prefix.with_suffix('.menu.bin');args.menu_game_creation=state['game_identity']['creation_time']
            state['menu_state_file']=str(args.menu_state_file)
        state['xr_command']=xr_command(args,prefix.with_suffix('.csv'),state['owner_identity'])
        state['native_command']=native_command(args,state['owner_identity'])
        write_state(state_path,state)
        if on_report:on_report(state_path,state['owner_identity'],state['game_identity'])
        print(json.dumps({'session_report':str(state_path),'stop_command':[sys.executable,str(pathlib.Path(__file__).resolve()),'stop','--session',str(state_path)]}),flush=True)
        with out_path.open('w') as out,err_path.open('w') as err:
            try:
                if cancel_requested and cancel_requested():raise StartupCancelled()
                # No Operator layer or pose calls are added to this strict mode.
                xr_started=time.perf_counter_ns()
                child=subprocess.Popen(state['xr_command'],stdout=out,stderr=err,
                    env=xr_environment(os.environ),creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
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
                stage_completed('xr_preflight',xr_started,'strict focused source0 CSV and required startup panel')
                if before_native_activation:
                    state['phase']='awaiting_level';write_state(state_path,state)
                    camera_started=time.perf_counter_ns()
                    def ensure_startup_live():
                        if cancel_requested and cancel_requested():raise StartupCancelled()
                        checked_identity(control,state['game_identity'])
                        if child.poll() is not None:raise RuntimeError('XR exited while waiting for the level')
                    ensure_startup_live()
                    before_native_activation(ensure_startup_live)
                    ensure_startup_live()
                    stage_completed('camera_ready',camera_started,'before_native_activation callback returned and startup remained live')
                    state['phase']='starting';write_state(state_path,state)
                checked_identity(control,state['game_identity'])
                if cancel_requested and cancel_requested():raise StartupCancelled()
                native_started=time.perf_counter_ns()
                activation_attempted=True
                with prefix.with_suffix('.native-start.log').open('w') as log:
                    result=subprocess.run(state['native_command'],stdout=log,stderr=subprocess.STDOUT,timeout=90,
                        creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
                if result.returncode:raise RuntimeError('Native activation failed; seeking cleanup without retry')
                stage_completed('native_prepare',native_started,'native preparation command exited 0; does not prove a stereo frame')
                state['native_started']=True
                running_wait_started=time.perf_counter_ns()
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
                stage_completed('native_running',running_wait_started,'fresh running status with matching owner; does not prove a stereo frame')
                state['phase']='running';state['native_status']=status;state['native_started_qpc']=status['started_qpc'];write_state(state_path,state)
                telemetry.bind_native(status['started_qpc'])
                telemetry.sample(status,state['phase'])
                print(json.dumps({'continuous_running':True,'session_report':str(state_path)}),flush=True)
                observe_first_stereo(native_started)
                if getattr(args,'menu_recovery',False):
                    from menu_display_monitor import MenuDisplayMonitor,BackgroundMenuDisplayMonitor
                    menu_monitor=BackgroundMenuDisplayMonitor(MenuDisplayMonitor(args.menu_state_file,state['game_identity'],state['owner_identity'],control.identity,
                        movie_display=getattr(args,'movie_display',False)))
                if getattr(args,'world_recovery',False):
                    from world_rebind_runtime import RuntimePreparation
                    from world_rebind_monitor import WorldRebindMonitor
                    adapter=RuntimePreparation(args.build_dir,args.dll_name,args.native_expected_sha256,
                        state['game_identity'],state['owner_identity'])
                    world_log=prefix.with_suffix('.world.jsonl').open('w',encoding='utf-8')
                    world_monitor=WorldRebindMonitor(game=state['game_identity'],owner=state['owner_identity'],
                        started_qpc=state['native_started_qpc'],baseline=adapter.baseline,observe=adapter.observe,
                        prepare=adapter.prepare,signal=control.signal,identity=control.identity,enabled=True,
                        probe_interval=2.0,include_pawn_changes=getattr(args,'crosshair_assignment_observer',False),
                        changed_hint=adapter.changed_hint,hint_interval=.05,prepare_changed=adapter.prepare_changed)
                path_center=start_path_choice_position(args,state,native_status,floor,control)
                if getattr(args,'right_stick_deadzone_percent',None) is not None:
                    from right_stick_policy import Policy
                    right_stick_policy=Policy(args.pid,args.right_stick_deadzone_percent,control.identity)
                    right_stick_policy.tick()
                    state['right_stick_policy']=dict(right_stick_policy.report)
                    if not right_stick_policy.report['applied']:
                        raise RuntimeError('Right-stick dead zone was not applied: '+repr(right_stick_policy.report['errors']))
                while child.poll() is None:
                    if right_stick_policy:right_stick_policy.tick()
                    observe_first_stereo(native_started)
                    if cancel_requested and cancel_requested():
                        state['stop_requested_by']='launcher_cancel';break
                    status=read_session_status(native_status,args.pid,floor)
                    if status and status.get('phase')!='running':break
                    checked_identity(control,state['game_identity'])
                    if menu_monitor:menu_monitor.poll()
                    if path_center:
                        try:
                            for event in path_center.poll():
                                telemetry.event(event['event'],**{key:value for key,value in event.items() if key!='event'})
                        except Exception as error:
                            state['path_choice_center_poll_error']=repr(error)
                            close_path_choice_position(path_center,state)
                    telemetry.sample(status,state['phase'],menu_monitor.last_menu_mode if menu_monitor else None)
                    if world_monitor:
                        for event in world_monitor.poll(status,menu_monitor.last_menu_mode if menu_monitor and menu_monitor.last else None):
                            world_log.write(json.dumps(dict(observed_ns=time.perf_counter_ns(),**event))+'\n');world_log.flush()
                    ordinary_interval=.1 if getattr(args,'movie_display',False) else .25
                    if world_monitor:world_monitor.wait(ordinary_interval)
                    else:time.sleep(ordinary_interval)
            except StartupCancelled:
                state['stop_requested_by']='startup_cancel'
            except KeyboardInterrupt:
                state['stop_requested_by']='keyboard_interrupt'
            except Exception as error:
                state['error']=repr(error)
            finally:
                if right_stick_policy:
                    try:state['right_stick_policy']=right_stick_policy.close()
                    except Exception as error:state['right_stick_policy_close_error']=repr(error)
                close_path_choice_position(path_center,state)
                if world_monitor:
                    try:world_monitor.close()
                    except Exception as error:state['world_monitor_close_error']=repr(error)
                if world_log:world_log.close()
                if menu_monitor:
                    try:menu_monitor.close()
                    except Exception as error:state['menu_monitor_close_error']=repr(error)
                    state['menu_observation_worker']=menu_monitor.summary()
                state['phase']='stopping';write_state(state_path,state)
                if activation_attempted:
                    diagnostics_attempted=False
                    try:
                        if checked_identity(control,state['game_identity'],allow_exited=True):
                            state['native_stop_signaled']=control.signal(f'Local\\GearsVR.Capture.Stop.{args.pid}')
                        deadline=time.monotonic()+args.cleanup_timeout
                        while time.monotonic()<deadline:
                            status=read_session_status(native_status,args.pid,floor)
                            if status and status.get('phase') in ('stopped','incomplete'):break
                            time.sleep(.2)
                        state['native_status']=status
                        diagnostics_saved=preserve_native_diagnostics(args.build_dir.resolve(),prefix,state)
                        diagnostics_attempted=True
                        if state.get('native_started_qpc') and status and status.get('started_qpc')!=state['native_started_qpc']:
                            raise RuntimeError('Native session started_qpc identity changed during cleanup')
                        state['native_cleanup_verified']=native_stopped_clean(status) and native_owner_matches(status,state['owner_identity'])
                        if not state['native_cleanup_verified']:raise RuntimeError('Native cleanup incomplete or deadline expired')
                        if not diagnostics_saved:raise RuntimeError('Native diagnostic preservation failed')
                        restored=prefix.with_suffix('.restored.json')
                        prime_source=args.build_dir.resolve()/'target-prime-status.txt'
                        prime_args=[]
                        if prime_source.is_file():
                            prime_copy=prefix.with_suffix('.prime-status.txt')
                            shutil.copyfile(prime_source,prime_copy)
                            state['prime_status_saved']=str(prime_copy)
                            prime_args=['--prime-status',str(prime_copy)]
                        manifest_source=args.build_dir.resolve()/'hook-entry-manifest.json'
                        if manifest_source.is_file():
                            manifest_copy=prefix.with_suffix('.hook-entry-manifest.json')
                            shutil.copyfile(manifest_source,manifest_copy)
                            state['hook_manifest_saved']=str(manifest_copy)
                            prime_args += ['--hook-manifest',str(manifest_copy),
                                           '--game-creation-time',str(state['game_identity']['creation_time'])]
                        with prefix.with_suffix('.verify.log').open('w') as log:
                            verified=subprocess.run([sys.executable,str(ROOT/'tools/native-capture/verify_stopped.py'),
                                '--pid',str(args.pid),'--status',str(prefix.with_suffix('.native-status.txt')),
                                '--output',str(restored),*prime_args],stdout=log,stderr=subprocess.STDOUT,timeout=20,
                                creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
                        state['entry_restore_verified']=verified.returncode==0 and json.loads(restored.read_text())['all_match']
                    except Exception as error:
                        state['native_cleanup_error']=repr(error)
                        state['native_cleanup_verified']=False
                    finally:
                        if not diagnostics_attempted:
                            preserve_native_diagnostics(args.build_dir.resolve(),prefix,state)
                if child:
                    cleanup_owned_xr(child,state['xr_owned_pid'],args.pid,args.cleanup_timeout,control,state)
                finish_path_choice_position(path_center,state,control)
        if activation_attempted:observe_first_stereo(native_started)
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
            if isinstance(value,dict) and value.get('menu_recovery_evidence'):state['menu_recovery_evidence']=value
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
        if telemetry:
            telemetry.close(state['phase'],passed=state.get('passed',False),native_cleanup_verified=state.get('native_cleanup_verified',False),
                xr_exit_code=state.get('xr_exit_code'),error=state.get('error'))
            state['diagnostics']=telemetry.summary()
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
    start.add_argument('--render-scale',type=float,help='Runtime recommended eye size multiplier per dimension; requires matching native build')
    start.add_argument('--camera-closer-metres',type=float,default=0.)
    start.add_argument('--first-person-preview',action='store_true')
    start.add_argument('--right-stick-deadzone-percent',type=float)
    start.add_argument('--startup-timeout',type=float,default=45.)
    start.add_argument('--cleanup-timeout',type=float,default=35.)
    start.add_argument('--anchor-observer','--anchor-diagnostics',dest='anchor_observer',action='store_true')
    start.add_argument('--world-recovery',action='store_true')
    start.add_argument('--native-expected-sha256',help='Expected DLL SHA256 from trusted launch manifest; required for world recovery')
    start.add_argument('--origin-trial',action='store_true')
    start.add_argument('--cinematic-camera',action='store_true')
    start.add_argument('--movie-display',action='store_true',help='Opt-in attested movie progress display; requires owned menu recovery')
    start.add_argument('--ui-observer',action='store_true')
    start.add_argument('--path-choice-center',action='store_true',help='Opt-in authenticated PathChoice position service; requires Native99 support')
    start.add_argument('--ui-hud-plane',action='store_true')
    start.add_argument('--ui-reticle-late-sample',action='store_true',help=argparse.SUPPRESS)
    start.add_argument('--crosshair-assignment-observer',action='store_true')
    start.add_argument('--hud-horizontal-fov-degrees',type=float)
    start.add_argument('--hud-element-scale',type=float)
    start.add_argument('--hud-center-scale',type=float)
    start.add_argument('--image-ticket-diagnostics',action='store_true')
    start.add_argument('--gpu-snapshot',action='store_true',help='Enable bounded original-resolution GPU snapshots: one automatic, additional captures via snapshot command')
    start.add_argument('--outage-diagnostics',action='store_true')
    start.add_argument('--shutdown-diagnostics',action='store_true')
    stop=commands.add_parser('stop')
    stop.add_argument('--session',type=pathlib.Path,required=True)
    stop.add_argument('--timeout',type=float,default=95.)
    recenter=commands.add_parser('recenter')
    recenter.add_argument('--session',type=pathlib.Path,required=True)
    snapshot=commands.add_parser('snapshot')
    snapshot.add_argument('--session',type=pathlib.Path,required=True)
    args=parser.parse_args(argv)
    if args.command=='stop':
        if args.timeout<=0:parser.error('Stop deadline must be positive')
        return request_stop(args.session.resolve(),args.timeout)
    if args.command=='recenter':return request_recenter(args.session.resolve())
    if args.command=='snapshot':return request_snapshot(args.session.resolve())
    if args.pid<=0 or args.startup_timeout<=0 or args.cleanup_timeout<=0:parser.error('PID and deadlines must be positive')
    if args.origin_trial and not args.anchor_observer:parser.error('Origin trial requires --anchor-observer')
    try:validate_ui_hud_plane(args)
    except ValueError as error:parser.error(str(error))
    if pathlib.Path(args.dll_name).name!=args.dll_name:parser.error('DLL basename required')
    if not 10<=args.stereo_units_per_metre<=500:parser.error('Provisional scale must be between 10 and 500')
    return run_session(args)


if __name__=='__main__':
    sys.exit(main())
