"""Version-gated launcher for our capture DLL; temporary live instrumentation.

Unlike package-inspector tools, this allocates/writes loader memory and loads a
DLL into the selected game. The capture DLL patches ordinary-code entries.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import math
import pathlib
import subprocess
import sys
import time
from prime_owner import pending_prime
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/package-inspector'))
sys.path.insert(0,str(ROOT/'tools/package-inspector/python-libs'))
import pefile
from resolve_camera import Resolver,psapi,DiscoveryRegistryChanged,DiscoveryHintChanged

k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.CloseHandle.argtypes=[w.HANDLE]
k.GetModuleHandleW.argtypes=[w.LPCWSTR];k.GetModuleHandleW.restype=w.HMODULE
k.GetProcAddress.argtypes=[w.HMODULE,c.c_char_p];k.GetProcAddress.restype=c.c_void_p
k.GetModuleHandleExW.argtypes=[w.DWORD,c.c_void_p,c.POINTER(w.HMODULE)];k.GetModuleHandleExW.restype=w.BOOL
k.GetModuleFileNameW.argtypes=[w.HMODULE,w.LPWSTR,w.DWORD]
k.VirtualAllocEx.argtypes=[w.HANDLE,c.c_void_p,c.c_size_t,w.DWORD,w.DWORD];k.VirtualAllocEx.restype=c.c_void_p
k.VirtualFreeEx.argtypes=[w.HANDLE,c.c_void_p,c.c_size_t,w.DWORD]
k.WriteProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)];k.WriteProcessMemory.restype=w.BOOL
k.CreateRemoteThread.argtypes=[w.HANDLE,c.c_void_p,c.c_size_t,c.c_void_p,c.c_void_p,w.DWORD,c.POINTER(w.DWORD)];k.CreateRemoteThread.restype=w.HANDLE
k.WaitForSingleObject.argtypes=[w.HANDLE,w.DWORD];k.WaitForSingleObject.restype=w.DWORD
k.GetExitCodeThread.argtypes=[w.HANDLE,c.POINTER(w.DWORD)];k.GetExitCodeThread.restype=w.BOOL
k.OpenEventW.argtypes=[w.DWORD,w.BOOL,w.LPCWSTR];k.OpenEventW.restype=w.HANDLE
psapi.GetModuleFileNameExW.argtypes=[w.HANDLE,w.HMODULE,w.LPWSTR,w.DWORD]


def run_hidden(*args,**kwargs):
    """Hide helper consoles; explicit game/simulator GUI windows are unaffected."""
    kwargs['creationflags']=kwargs.get('creationflags',0)|getattr(subprocess,'CREATE_NO_WINDOW',0)
    return subprocess.run(*args,**kwargs)


def modules(handle,*,bases=None):
    selected=None if bases is None else frozenset(bases)
    if selected is not None and (not selected or len(selected)>2048 or
            any(type(value)is not int or not 0<value<1<<64 for value in selected)):
        raise ValueError('Invalid module base selection')
    slots=(w.HMODULE*2048)();size=w.DWORD()
    if not psapi.EnumProcessModulesEx(handle,slots,c.sizeof(slots),c.byref(size),3):
        raise c.WinError(c.get_last_error())
    if size.value>c.sizeof(slots):raise RuntimeError('Module list exceeds bounded buffer')
    result={}
    for address in slots[:size.value//c.sizeof(w.HMODULE)]:
        if selected is not None and address not in selected:continue
        name=c.create_unicode_buffer(32768)
        if psapi.GetModuleFileNameExW(handle,address,name,len(name)):
            result[pathlib.Path(name.value).name.lower()]=(address,pathlib.Path(name.value))
    return result


def remote_call(handle,address,parameter):
    thread=k.CreateRemoteThread(handle,None,0,address,parameter,0,None)
    if not thread:raise c.WinError(c.get_last_error())
    try:
        status=k.WaitForSingleObject(thread,15000)
        if status!=0:raise RuntimeError('Remote loader call did not finish; do not repeat injection blindly')
        code=w.DWORD()
        if not k.GetExitCodeThread(thread,c.byref(code)):raise c.WinError(c.get_last_error())
        return code.value
    finally:k.CloseHandle(thread)


def observer_preparation_churn(error):
    """Known invalidation reports only; a retry must reacquire every proof."""
    from anchor_config import AnchorRegistryChanged,AnchorControlledPawnUnavailable,AnchorControlledLinkChanged
    from ui_observer_config import UIConfigRejected,UIRegistryChanged,UIControlledPawnUnavailable
    from crosshair_assignment_config import CrosshairRegistryChanged,CrosshairControlledLinkChanged
    if type(error) is CrosshairControlledLinkChanged:return True
    if isinstance(error,(AnchorRegistryChanged,UIRegistryChanged,CrosshairRegistryChanged,DiscoveryRegistryChanged)):return True
    if type(error) is UIControlledPawnUnavailable:return True
    if type(error)in(AnchorControlledPawnUnavailable,AnchorControlledLinkChanged):return True
    return type(error) is UIConfigRejected and str(error)=='UI certificate read changed'


def camera_preparation_transient(error):
    # Exact known transition failures only. Schema, code, certificate identity,
    # process identity and unknown memory errors remain hard rejections.
    from anchor_config import AnchorSamplingRejected
    if observer_preparation_churn(error):return True
    reason=str(error)
    if reason in {
        'Camera view target is not controlled Pawn',
        'Controlled Pawn changed; re-resolve required',
        'Pawn.Controller reciprocal link mismatch',
        'Camera/controller/local-player relationship changed',
        'Pawn/controller pointer changed at bracket boundary',
        'Camera/controller/player pointer changed at bracket boundary',
        'View target changed at bracket boundary',
        'Cinematic target changed during identity checks',
        'Camera relationship changed during discovery; retry',
        'Registry reallocated during discovery; retry',
        'Expected one local camera, found 0; no address accepted',
        'Cross-frame camera timestamp during identity checks',
    }:return True
    if reason.startswith('Anchor camera changed during startup: '):return True
    if reason in {'Cross-frame or identity/pointer change during measurement','Anchor sample acquisition deadline exceeded'}:
        # Retry discovery only when every exhausted sample was independently
        # classified as timestamp/registry growth with stable full identities.
        # The message alone also covers real identity changes and is not enough.
        from anchor_config import _temporary_sample_rejection
        if not isinstance(error,AnchorSamplingRejected):return False
        evidence=getattr(error,'evidence',{})
        # A nested timestamp guard can exhaust, then the outer sample loop can
        # continue and exhaust on measurement churn. Its receipt retains the
        # nested stage; classify every raw attempt, rather than losing the
        # existing bounded whole-acquisition retry solely because of that stage.
        attempts=evidence.get('attempts',[]) if evidence.get('stage') in ('sample','sample_before','sample_after') else []
        return bool(attempts) and all(isinstance(a.get('raw_rejection'),dict) and
            _temporary_sample_rejection(a['raw_rejection']) is not None for a in attempts)
    # The strict sampler's initial construction reads Controller.Pawn before
    # it can build a certificate; a transient unpossessed Pawn is null here.
    return reason=='Null object pointer' and getattr(error,'evidence',{}).get('stage')=='initial'


def wait_camera_preparation(acquire,check_live,deadline,*,clock=time.monotonic,sleep=time.sleep,failure_receipt=None):
    """Retry read-only discovery only; caller must not include activation."""
    previous_reason=None;attempts=0;failures=0;last_error=None
    try:
        while True:
            check_live()
            if clock()>=deadline:raise TimeoutError('Camera preparation deadline before activation: '+str(previous_reason))
            attempts+=1
            try:value=acquire()
            except RuntimeError as error:
                failures+=1;last_error=error
                if failures==1 and failure_receipt:failure_receipt(error,failures,'first')
                if not camera_preparation_transient(error):raise
                check_live()
                reason=str(error)
                if reason!=previous_reason:
                    print(json.dumps({'native_preparation_waiting_for_camera':reason}),flush=True)
                    previous_reason=reason
                remaining=deadline-clock()
                if remaining<=0:raise TimeoutError('Camera preparation deadline before activation: '+reason) from error
                sleep(min(.5,remaining));continue
            check_live()
            if clock()>=deadline:raise TimeoutError('Camera preparation completed after deadline; native remains inactive')
            if attempts>1:print(json.dumps({'native_preparation_camera_ready':True,'attempts':attempts}),flush=True)
            return value
    finally:
        if failures>1 and failure_receipt:failure_receipt(last_error,failures,'last')


def preparation_live_guard(pid,owner_pid=None,owner_creation_time=None):
    from run_stereo_continuous import WindowsControl
    control=WindowsControl();game=control.identity(pid)
    if not game:raise RuntimeError('Game exited before camera preparation')
    def check():
        if control.identity(pid)!=game:raise RuntimeError('Game process identity changed during camera preparation')
        if owner_pid is not None:
            owner=control.identity(owner_pid)
            if not owner or owner['creation_time']!=owner_creation_time:
                raise RuntimeError('Owner exited or changed during camera preparation')
            c.set_last_error(0)
            event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.RepositoryLauncher.Stop.{owner_pid}')
            if not event and c.get_last_error() not in (0,2):raise c.WinError(c.get_last_error())
            if event:
                try:
                    status=k.WaitForSingleObject(event,0)
                    if status==0:raise RuntimeError('Owner cancelled camera preparation before activation')
                    if status!=0x102:raise RuntimeError('Owner cancellation event wait failed')
                finally:k.CloseHandle(event)
    check();return check


def observer_signatures(args):
    """Exact original entry signatures shared by startup and live rebind."""
    signatures={0x15cf08:bytes.fromhex('488bc45356574154415541564157'),
                0x15c968:bytes.fromhex('488bc4488958084889681048897018')}
    if args.family_diagnostics: signatures[0x3a148c]=bytes.fromhex('48895c2408488974241855')
    if args.family_diagnostics: signatures[0x193d50]=bytes.fromhex('488bc4488958104889681848897020')
    if args.family_diagnostics: signatures[0x116250]=bytes.fromhex('488bc44889581848897020574154')
    if args.family_diagnostics: signatures[0x15a80c]=bytes.fromhex('48895c241055488bec4883ec70')
    if args.family_diagnostics: signatures[0x15a974]=bytes.fromhex('40534883ec20488bd9')
    if args.family_diagnostics: signatures[0x1fc060]=bytes.fromhex('488bc44889581048897018574154')
    if args.viewport_diagnostics: signatures[0x13cfcc]=bytes.fromhex('44894c242048894c240855535657')
    if args.viewport_diagnostics: signatures[0x1af42c]=bytes.fromhex('488bc4488958084889701048897818')
    if args.first_view_mask_control or args.view_union_mask_fix:
        signatures[0x2a7e48]=bytes.fromhex('488bc448895808488970104889781855')
        signatures[0x119bdd]=bytes.fromhex('e866e21800')
    if args.shadow_view:
        signatures.update({0x1c03c8:bytes.fromhex('48895c240848896c2410488974241857'),
            0x194454:bytes.fromhex('48895c241848894c2408565741564883'),
            0x15ca7c:bytes.fromhex('48895c2408574883ec20488bf9488b89'),
            0x53ea50:bytes.fromhex('e9abe8b9ff')})
    if args.shadow_state:
        signatures.update({0x33bdd4:bytes.fromhex('4883ec28ba08000000b980020000'),
            0x4ce80c:bytes.fromhex('40534883ec20488bc1488d511c488bd9'),
            0x33bc70:bytes.fromhex('48895c2408574883ec208bda488bf9')})
    if args.shadow_constructor:signatures[0x15e428]=bytes.fromhex('488bc4488958104889701848897820')
    return signatures


def resolve_camera_once(pid,profile):
    resolver=Resolver(pid,profile)
    try:return resolver.resolve()
    finally:resolver.p.close()


class _InitialPreparationCamera:
    """One startup's comparison inputs, never a camera/certificate cache."""
    def __init__(self):
        self.reference=None

    def expected(self,refreshed):
        # Latch before any builder can request an internal or outer retry.
        # Only these comparison inputs survive; every result/certificate is fresh.
        if self.reference is None:
            self.reference={key:refreshed[key] for key in ('local_player_address','module_base')}
        return self.reference


def acquire_observer_preparation(pid,profile,camera,signatures,dll,*,anchor_observer=False,ui_observer=False,cinematic_camera=False,
                                 record_anchor_failure=True,preparation_deadline=None,check_live=None,entry_reader=None,
                                 crosshair_assignment_observer=False,camera_discovery=None,anchor_sample_pose=True):
    """Read-only, pre-activation acquisition; never retries identity failures.

    A new Resolver.resolve() prepares fresh registries/caches and roles on every
    attempt. Anchor, UI and the eventual Capture INI all use that same camera.
    Standalone acquisition has a ten-second deadline. Continuous preparation
    uses its shared command deadline, including any outer read-only retries.
    Synchronous reads cannot be preempted; late success is always rejected.
    """
    from ui_observer_config import build_ui_observer_config,ui_observer_ini,UIConfigRejected,UIRegistryChanged,UIControlledPawnUnavailable
    from crosshair_assignment_config import CrosshairRegistryChanged,CrosshairControlledLinkChanged
    from anchor_config import AnchorRegistryChanged,AnchorControlledPawnUnavailable,AnchorControlledLinkChanged
    if type(anchor_sample_pose)is not bool:raise ValueError('anchor_sample_pose must be boolean')
    if not anchor_sample_pose and (not anchor_observer or entry_reader is None):
        raise ValueError('Static-only anchor assembly requires an attested anchor observer')
    if crosshair_assignment_observer and not (anchor_observer and ui_observer):
        raise ValueError('Crosshair assignment observer requires UI/anchor observers')
    deadline=time.monotonic()+10 if preparation_deadline is None else preparation_deadline
    registry_retry=anchor_observer or ui_observer;attempts=3 if registry_retry else 1
    for attempt in range(attempts):
        if check_live:check_live()
        if registry_retry and time.monotonic()>=deadline:
            raise UIConfigRejected('UI acquisition retry deadline exceeded before activation') if ui_observer else TimeoutError('Anchor acquisition retry deadline exceeded before activation')
        try:
            r=Resolver(pid,profile)
            try:
                refreshed=camera_discovery(r) if camera_discovery is not None else r.resolve()
                expected=camera.expected(refreshed) if isinstance(camera,_InitialPreparationCamera) else camera
                if refreshed['local_player_address']!=expected['local_player_address']:
                    raise RuntimeError('Local player changed during preparation')
                if int(refreshed['module_base'],0)!=int(expected['module_base'],0):
                    raise RuntimeError('Module base changed during preparation')
                for offset,signature in signatures.items():
                    raw=entry_reader(r.base+offset,len(signature),r.read) if entry_reader is not None else r.read(r.base+offset,len(signature))
                    if raw!=signature:
                        raise RuntimeError('Target prologue differs; instrumentation rejected')
                anchor_metadata=None;anchor_section='';ui_metadata=None;ui_section=''
                if anchor_observer:
                    from anchor_config import build_anchor_config,anchor_ini,AnchorSamplingRejected
                    try:certificate,anchor_metadata=build_anchor_config(r,refreshed,prepared_camera=refreshed,
                        **({'sample_pose':False} if not anchor_sample_pose else {}),
                        **({'cinematic_camera':True} if cinematic_camera else {}))
                    except AnchorSamplingRejected as error:
                        if record_anchor_failure:save_anchor_sampling_failure(dll,pid,error)
                        raise
                    anchor_section=anchor_ini(certificate);anchor_metadata['pid']=pid
                if ui_observer:
                    ui_certificate,ui_metadata=build_ui_observer_config(r,refreshed,
                        **({'mounted_anchor':certificate} if anchor_metadata is not None and certificate.get('schema')==3 else {}),
                        **({'entry_reader':entry_reader} if entry_reader is not None else {}))
                    ui_section=ui_observer_ini(ui_certificate)
                    if crosshair_assignment_observer and certificate.get('schema')==3:
                        from mounted_context import mounted_crosshair_suspension,mounted_crosshair_ini
                        suspension=mounted_crosshair_suspension(certificate,pid,r.base,profile['sha256'])
                        ui_metadata['crosshair_assignment_suspended']=suspension
                        ui_metadata['crosshair_assignment_observer_enabled']=True
                        ui_section+=mounted_crosshair_ini(suspension)
                    elif crosshair_assignment_observer:
                        from crosshair_assignment_config import build_crosshair_assignment_config,crosshair_assignment_ini
                        assignment=build_crosshair_assignment_config(r,refreshed,include_tick_roots=True,include_writer_view=True,include_aim_state=True,
                            **({'entry_reader':entry_reader} if entry_reader is not None else {}))
                        ui_section+=crosshair_assignment_ini(assignment)
                        ui_metadata['crosshair_assignment']=assignment
                finish=getattr(r,'finish_discovery_field_hints',None)
                if callable(finish):finish()
                if registry_retry and time.monotonic()>=deadline:
                    raise UIConfigRejected('UI acquisition exceeded retry deadline before activation') if ui_observer else TimeoutError('Anchor acquisition exceeded retry deadline before activation')
                if check_live:check_live()
                return refreshed,anchor_metadata,anchor_section,ui_metadata,ui_section
            finally:r.p.close()
        except (UIConfigRejected,CrosshairRegistryChanged,CrosshairControlledLinkChanged,AnchorRegistryChanged,
                AnchorControlledPawnUnavailable,AnchorControlledLinkChanged,DiscoveryRegistryChanged) as error:
            # Only known read churn, typed unpossession and registry mismatches join
            # this budget. Identity/code/unknown failures remain hard; every
            # retry discards the Resolver and all certificates.
            if not observer_preparation_churn(error):raise
            if attempt+1==attempts or time.monotonic()>=deadline:raise
            print(f'Observer preparation changed ({error}); reacquiring all certificates ({attempt+2}/{attempts})',file=sys.stderr)
            # An invalid optional location is discarded once. Its fresh-reader
            # fallback needs no settling pause; the same attempt/deadline and
            # full proof requirements still apply. Other churn keeps its wait.
            if type(error)is not DiscoveryHintChanged:
                time.sleep((min)(.05,(max)(0,deadline-time.monotonic())))


def save_anchor_sampling_failure(dll,pid,error,*,preparation_failures=None,preparation_receipt=None):
    # A failure receipt is unique and never replaces the successful certificate.
    stamp=time.perf_counter_ns()
    path=dll.with_name(f'anchor-observer-config-failed-{pid}-{stamp}.json')
    report={'schema':1,'pid':pid,'created_unix_ns':time.time_ns(),
            'created_perf_counter_ns':stamp,'accepted':False,'native_activation_attempted':False,
            'read_only':True,'sample_acquisition':error.evidence}
    report['failure_stage']=error.evidence.get('stage','sample')
    report['guard_acquisitions']=error.evidence.get('guard_acquisitions',[])
    report['strict_guard_attempts']=error.evidence.get('strict_guard_attempts',0)
    if preparation_failures is not None:report.update(preparation_failures=preparation_failures,preparation_receipt=preparation_receipt)
    with path.open('x',encoding='utf-8') as output:json.dump(report,output,indent=2,allow_nan=False)
    print(f'Anchor observer acquisition failure receipt: {path}',file=sys.stderr)
    return path


def hud_size_ini(args):
    from camera_closer_options import camera_closer_metres
    preview=getattr(args,'first_person_preview',False)
    if type(preview) is not bool:raise ValueError('First person preview must be boolean')
    if preview and not all(getattr(args,key,False) for key in ('anchor_observer','cinematic_camera','crosshair_assignment_observer','continuous_session','stereo_xr')):
        raise ValueError('First person preview requires continuous stereo and certified gameplay/weapon observers')
    result='first_person_preview=1\n' if preview else ''
    closer=camera_closer_metres(args)
    if closer>0:
        if not getattr(args,'continuous_session',False) or not getattr(args,'stereo_xr',False):
            raise ValueError('Camera closer requires continuous stereo XR')
        result+=f'camera_closer_metres={closer:.17g}\n'
    value=getattr(args,'hud_horizontal_fov_degrees',None)
    if value is not None:
        if not getattr(args,'ui_hud_plane',False):raise ValueError('HUD size requires --ui-hud-plane')
        if isinstance(value,bool) or not isinstance(value,(int,float)) or not 45<=value<=100 or not math.isfinite(value):
            raise ValueError('hud_horizontal_fov_degrees must be finite and between 45 and 100')
        result+=f'ui_hud_horizontal_fov_degrees={value:.17g}\n'
    for key,minimum,maximum in (('hud_element_scale',1,2),('hud_center_scale',0,1)):
        value=getattr(args,key,None)
        if value is None:continue
        if not getattr(args,'ui_hud_plane',False):raise ValueError('HUD layout requires --ui-hud-plane')
        if isinstance(value,bool) or not isinstance(value,(int,float)) or not minimum<=value<=maximum or not math.isfinite(value):
            raise ValueError(f'{key} must be finite and between {minimum} and {maximum}')
        result+=f'ui_{key}={value:.17g}\n'
    return result


def configure_anchor_options(args,parser):
    if getattr(args,'ui_reticle_late_sample',False) and not all(
            getattr(args,key,False) for key in ('ui_observer','ui_hud_plane','continuous_session')):
        parser.error('Late reticle sampling requires --ui-observer, --ui-hud-plane and --continuous-session')
    if getattr(args,'crosshair_assignment_observer',False) and (not all(
            getattr(args,key,False) for key in ('ui_observer','anchor_observer','continuous_session')) or getattr(args,'origin_trial',False)):
        parser.error('Crosshair assignment observer requires continuous UI/anchor observers and cannot use origin trial')
    try:hud_size_ini(args)
    except ValueError as error:parser.error(str(error))
    if getattr(args,'cinematic_camera',False) and (not args.anchor_observer or getattr(args,'origin_trial',False)):
        parser.error('Cinematic camera requires --anchor-observer and cannot use the Pawn origin trial')
    if getattr(args,'ui_hud_plane',False) and not (
            getattr(args,'ui_observer',False) and getattr(args,'anchor_observer',False)
            and getattr(args,'continuous_session',False)):
        parser.error('HUD plane requires --ui-observer, --anchor-observer and --continuous-session')
    if getattr(args,'origin_trial',False) and not (
            getattr(args,'anchor_observer',False) and getattr(args,'continuous_session',False)
            and getattr(args,'stereo_gpu_publish',False)):
        parser.error('Origin trial requires --anchor-observer, --continuous-session and --stereo-gpu-publish')
    if args.anchor_observer:
        if not(args.stereo_xr and args.frame_link_diagnostics):
            parser.error('Anchor observer requires --stereo-xr and --frame-link-diagnostics')
        args.family_diagnostics=True # Use the already-existing family observer.


def main():
    command_started=time.monotonic()
    command_started_ns=time.perf_counter_ns()
    def preparation_timing(stage,started_ns):
        finished=time.perf_counter_ns()
        print(json.dumps({'native_prepare_stage':stage,'elapsed_ms':(finished-started_ns)/1000000,
            'since_command_start_ms':(finished-command_started_ns)/1000000,'clock':'perf_counter_ns'}),flush=True)
    parser=argparse.ArgumentParser()
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--seconds',type=int,default=90,choices=range(10,181))
    parser.add_argument('--build-dir',type=pathlib.Path,default=pathlib.Path(__file__).with_name('build'))
    parser.add_argument('--dll-name',default='GearsCapture.dll',help='Name of an independently built diagnostic DLL')
    parser.add_argument('--startup-selftest-proof',type=pathlib.Path,help=argparse.SUPPRESS)
    parser.add_argument('--startup-selftest-digest',help=argparse.SUPPRESS)
    parser.add_argument('--startup-selftest-nonce',help=argparse.SUPPRESS)
    parser.add_argument('--use-reserved',action='store_true',help='Activate the same DLL whose unenabled trampolines were reserved during fresh startup')
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--wobble',action='store_true',help='Enable the gated 1.5-degree render-only yaw test')
    mode.add_argument('--sweep',action='store_true',help='Run pitch/yaw/roll and three tiny render-only translations')
    mode.add_argument('--live-head',action='store_true',help='Bounded real-headset test (requires verified pose producer)')
    mode.add_argument('--gpu-diagnostics',action='store_true',help='Observe swapchain metadata and ExecuteCommandLists, without GPU copies')
    mode.add_argument('--family-diagnostics',action='store_true',help='Observe bounded view arrays at the candidate render-family submission')
    mode.add_argument('--shadow-view',action='store_true',help='Once, construct and destroy an unsubmitted native source-view copy on the game thread')
    mode.add_argument('--shadow-state',action='store_true',help='Once, create independent view state and observe its ordinary deferred deleting wrapper')
    mode.add_argument('--shadow-constructor',action='store_true',help='Once, reconstruct an unsubmitted source view from candidate full-constructor arguments, retaining the original state')
    mode.add_argument('--constructor-state',action='store_true',help='Once, reconstruct an unsubmitted view with independent native state, destroying the view before ordinary state release')
    mode.add_argument('--constructor-eye',action='store_true',help='Once, reconstruct an unsubmitted independent-state view shifted one synthetic game unit right')
    mode.add_argument('--renderer-pair',action='store_true',help='Once, construct/destroy an unsubmitted two-view native renderer; never consume or enqueue it')
    mode.add_argument('--submit-pair',action='store_true',help='Once, submit two half-width synthetic eye views through the ordinary renderer path')
    parser.add_argument('--trial-seconds',type=int,default=30,choices=(30,120),help='Validated view-union trial duration; defaults to thirty seconds')
    parser.add_argument('--submit-pair-seconds',type=int,default=0,choices=range(0,121),help='Sustain pairs for up to sixty seconds, or explicitly selected 120-second validated union trial')
    parser.add_argument('--submit-background',action='store_true',help='Allow bounded viewport pair without game focus: at most ten seconds, or the validated union trial duration; F8 stops the trial')
    parser.add_argument('--submit-output-bounds',action='store_true',help='Use original integer output bounds instead of already scaled source floats for the diagnostic split')
    quad_source=parser.add_mutually_exclusive_group()
    quad_source.add_argument('--quad-left-source-control',action='store_true',help='Ten-second A/B/A right-final-quad source-u control: preserve for 0-3s, sample left for 3-6s, preserve after 6s')
    quad_source.add_argument('--quad-right-source-control',action='store_true',help='Ten-second A/B/A left-final-quad source-u control: preserve for 0-3s, sample right for 3-6s, preserve after 6s')
    parser.add_argument('--submit-mono-control',action='store_true',help='Submit only one reconstructed original view, with no shift or resize, to isolate constructor effects')
    parser.add_argument('--submit-mono-independent-state',action='store_true',help='With --submit-mono-control, submit the view using the newly created state instead of the game state')
    parser.add_argument('--submit-duplicate-control',action='store_true',help='Submit two full original rectangles with no eye shift, to isolate multi-view processing from split geometry')
    parser.add_argument('--viewport-diagnostics',action='store_true',help='Observe D3D12 viewport/scissor/direct draws and first RTV metadata; no ExecuteIndirect coverage')
    parser.add_argument('--color-copy-diagnostics',action='store_true',help='Observe bounded CopyTextureRegion calls involving main color/selected final input; requires viewport pair and unchanged quad sampling')
    parser.add_argument('--first-view-mask-control',action='store_true',help='Ten-second A/B/A native first-view mask t=0 to t=1 geometry control during seconds 3-6; requires exclusive split pair')
    parser.add_argument('--projection-control',action='store_true',help='Thirty-second A/B/A/C/A projection control; requires view union mask fix')
    parser.add_argument('--stereo-xr',action='store_true',help='Bounded native stereo using a verified same-PID OpenXR paired-view stream')
    parser.add_argument('--stereo-gpu-publish',action='store_true',help='Publish ticketed GPU images from validated native stereo frame links')
    parser.add_argument('--simulator-operator',action='store_true',help='Explicitly accept operator-simulated stereo sources; requires the complete stereo GPU publication path')
    parser.add_argument('--continuous-session',action='store_true',help='Explicit persistent strict stereo session; stop through the named event or F8')
    parser.add_argument('--continuous-owner-pid',type=int)
    parser.add_argument('--continuous-owner-creation-time',type=int,help='Owner process FILETIME creation stamp; required only for continuous sessions')
    parser.add_argument('--stereo-units-per-metre',type=float,default=100.,help='Configurable provisional world scale for stereo XR; not automatically calibrated')
    parser.add_argument('--view-union-mask-fix',action='store_true',help='Scoped same-family mask correction from validated adjacent view union; exclusive bounded split pair')
    parser.add_argument('--require-target-prime',action='store_true',help='Require same-PID bounded descriptor observation before activation')
    parser.add_argument('--frame-link-diagnostics',action='store_true',help='Read-only paired final-draw/list/queue/backbuffer association; requires reserved target priming')
    parser.add_argument('--anchor-observer','--anchor-diagnostics',dest='anchor_observer',action='store_true',help='Read-only reflected Pawn/eye anchor diagnostics; requires stereo XR frame association')
    parser.add_argument('--origin-trial',action='store_true',help='Explicit experimental origin A/B/A; requires anchor observer, continuous session and stereo GPU publication')
    parser.add_argument('--cinematic-camera',action='store_true',help='Accept certified cinematic CameraActor targets sharing the controlled WorldInfo while preserving Pawn possession')
    parser.add_argument('--ui-observer',action='store_true',help='Observe registered reticle/HUD Canvas calls without changing coordinates')
    parser.add_argument('--ui-hud-plane',action='store_true',help='Experimental HUD plane; requires UI observer, anchor observer and continuous session')
    parser.add_argument('--ui-reticle-late-sample',action='store_true',help=argparse.SUPPRESS)
    parser.add_argument('--crosshair-assignment-observer',action='store_true',help='Observe certified crosshair assignments; no reticle correction')
    parser.add_argument('--hud-horizontal-fov-degrees',type=float)
    parser.add_argument('--camera-closer-metres',type=float,default=0.)
    parser.add_argument('--first-person-preview',action='store_true')
    parser.add_argument('--hud-element-scale',type=float,help='Experimental certified HUD group size, 1..2; identity default')
    parser.add_argument('--hud-center-scale',type=float,help='Experimental HUD group center displacement, 0..1; identity default')
    parser.add_argument('--outage-diagnostics',action='store_true',help='Record a bounded missing-image window in the existing stereo publication path; disabled by default')
    args=parser.parse_args()
    configure_anchor_options(args,parser)
    if args.ui_observer and not (args.anchor_observer and args.continuous_session):
        parser.error('UI observer requires continuous stereo with anchor observer')
    args.gpu_diagnostics=args.gpu_diagnostics or args.viewport_diagnostics
    if args.continuous_session:
        if (args.continuous_owner_pid is None or not 0<args.continuous_owner_pid<=0xffffffff
                or args.continuous_owner_creation_time is None or not 0<args.continuous_owner_creation_time<=0xffffffffffffffff):
            parser.error('Continuous session requires an owner PID and FILETIME creation identity')
        required=(args.submit_pair and args.submit_output_bounds and args.submit_background and args.viewport_diagnostics
                  and args.stereo_xr and args.stereo_gpu_publish and args.frame_link_diagnostics
                  and args.view_union_mask_fix and args.require_target_prime and args.use_reserved)
        controls=(args.wobble or args.sweep or args.live_head or args.submit_mono_control or args.submit_mono_independent_state
                  or args.submit_duplicate_control or args.quad_left_source_control or args.quad_right_source_control
                  or args.first_view_mask_control or args.projection_control or args.simulator_operator)
        if not required or controls or args.submit_pair_seconds or args.trial_seconds!=30:
            parser.error('Continuous session requires the complete reserved strict stereo path, zero pair duration, and no diagnostic controls')
    elif args.continuous_owner_pid is not None or args.continuous_owner_creation_time is not None:
        parser.error('Owner identity is only valid for continuous sessions')
    if args.require_target_prime and not(args.use_reserved and args.viewport_diagnostics):parser.error('Target priming requires reserved viewport diagnostics')
    if args.frame_link_diagnostics and not(args.require_target_prime and args.view_union_mask_fix):parser.error('Frame association requires target priming and the validated view-union pair')
    if args.stereo_gpu_publish and not(args.stereo_xr and args.frame_link_diagnostics):parser.error('Stereo GPU publication requires stereo XR and frame-link diagnostics')
    if args.outage_diagnostics and not args.stereo_gpu_publish:parser.error('Outage diagnostics require stereo GPU publication')
    if args.simulator_operator and not(args.stereo_xr and args.stereo_gpu_publish and args.frame_link_diagnostics and args.view_union_mask_fix):parser.error('Simulator operator requires stereo XR, GPU publication, frame-link diagnostics and validated union')
    if args.submit_pair_seconds and not args.submit_pair:parser.error('--submit-pair-seconds requires --submit-pair')
    if args.trial_seconds==120 and not args.view_union_mask_fix:parser.error('120-second trials require validated view union mask fix')
    if args.submit_pair_seconds>60 and not(args.trial_seconds==120 and args.view_union_mask_fix and args.submit_pair_seconds==120):parser.error('Durations above sixty seconds require the explicit 120-second validated union trial')
    if args.submit_background and not(args.submit_pair and args.viewport_diagnostics and (args.continuous_session or 1<=args.submit_pair_seconds<=10 or (args.view_union_mask_fix and args.submit_pair_seconds==args.trial_seconds))):parser.error('Background trials require viewport pair diagnostics for one to ten seconds, or the validated union trial duration')
    if args.submit_output_bounds and not args.submit_pair:parser.error('--submit-output-bounds requires --submit-pair')
    if args.submit_mono_control and (not args.submit_pair or args.submit_output_bounds):parser.error('Mono control requires --submit-pair and unchanged source bounds')
    if args.submit_mono_independent_state and not args.submit_mono_control:parser.error('Independent mono state requires --submit-mono-control')
    if args.submit_duplicate_control and (not args.submit_pair or args.submit_output_bounds or args.submit_mono_control):parser.error('Duplicate control requires exclusive unchanged two-view mode')
    if (args.quad_left_source_control or args.quad_right_source_control) and not(args.submit_pair and args.submit_output_bounds and args.viewport_diagnostics and args.submit_pair_seconds==10 and not(args.submit_mono_control or args.submit_mono_independent_state or args.submit_duplicate_control)):
        parser.error('Quad source control requires exclusive ten-second split pair, output bounds, and viewport diagnostics')
    if args.color_copy_diagnostics and not(args.submit_pair and args.viewport_diagnostics and not(args.quad_left_source_control or args.quad_right_source_control)):
        parser.error('Color copy diagnostics require viewport pair without quad source control')
    if args.first_view_mask_control and not(args.submit_pair and args.submit_output_bounds and args.viewport_diagnostics and args.submit_pair_seconds==10 and not(args.submit_mono_control or args.submit_mono_independent_state or args.submit_duplicate_control or args.quad_left_source_control or args.quad_right_source_control)):
        parser.error('First view mask control requires exclusive ten-second split pair, output bounds, and viewport diagnostics without quad source control')
    if args.projection_control and (not args.view_union_mask_fix or args.trial_seconds!=30):parser.error('Projection control requires thirty-second --view-union-mask-fix')
    if args.stereo_xr and (not args.view_union_mask_fix or args.projection_control or args.live_head):parser.error('Stereo XR requires view union mask fix and excludes projection controls and live-head mode')
    if not math.isfinite(args.stereo_units_per_metre) or not 10<=args.stereo_units_per_metre<=500:parser.error('Stereo scale must be finite and between 10 and 500')
    if args.view_union_mask_fix and not(args.submit_pair and args.submit_output_bounds and args.viewport_diagnostics and (args.continuous_session or args.submit_pair_seconds==args.trial_seconds) and not(args.submit_mono_control or args.submit_mono_independent_state or args.submit_duplicate_control or args.quad_left_source_control or args.quad_right_source_control or args.first_view_mask_control)):
        parser.error('View union mask fix requires exclusive bounded split pair matching --trial-seconds, output bounds, and viewport diagnostics without other mask/quad controls')
    if args.submit_pair_seconds and args.seconds<args.submit_pair_seconds+10:parser.error('Capture duration must leave ten seconds beyond the sustained trial for startup and cleanup')
    args.constructor_eye=args.constructor_eye or args.renderer_pair or args.submit_pair
    args.constructor_state=args.constructor_state or args.constructor_eye
    args.shadow_constructor=args.shadow_constructor or args.constructor_state
    args.shadow_state=args.shadow_state or args.constructor_state
    args.shadow_view=args.shadow_view or args.shadow_constructor
    args.family_diagnostics=args.family_diagnostics or args.shadow_view or args.shadow_state
    if pathlib.Path(args.dll_name).name!=args.dll_name or not args.dll_name.lower().endswith('.dll'):
        raise RuntimeError('DLL name must be a basename ending in .dll')
    dll=args.build_dir.resolve()/args.dll_name
    test=dll.with_name('CaptureSelfTest.exe')
    if not dll.is_file() or not test.is_file():raise RuntimeError('Build capture DLL and self-test first')
    selftests_started=time.perf_counter_ns()
    from startup_selftests import capture_suite,continuous_suite,run_suite,consume_proof
    startup_proof=None
    proof_options=(args.startup_selftest_proof,args.startup_selftest_digest,args.startup_selftest_nonce)
    if any(proof_options):
        if not all(proof_options) or not(args.continuous_session and args.use_reserved and args.require_target_prime):
            raise RuntimeError('Complete owned startup self-test handshake required')
        if capture_suite(args)!=continuous_suite():raise RuntimeError('Startup self-test proof does not cover requested flags')
        # The expected digest/nonce arrived through the current successful
        # reservation report, not from the self-declared proof document.
        check=preparation_live_guard(args.pid,args.continuous_owner_pid,args.continuous_owner_creation_time);check()
        from run_stereo_continuous import WindowsControl
        game=WindowsControl().identity(args.pid)
        startup_proof=consume_proof(args.startup_selftest_proof,args.startup_selftest_digest,dll=dll,
            owner=dict(pid=args.continuous_owner_pid,creation_time=args.continuous_owner_creation_time),
            game=game,nonce=args.startup_selftest_nonce)
        check()
        preparation_timing('self_tests_proof_verification',selftests_started)
    else:run_suite(dll,capture_suite(args),run_hidden)
    preparation_timing('self_tests_total',selftests_started)
    profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    check_preparation=preparation_live_guard(args.pid,args.continuous_owner_pid,args.continuous_owner_creation_time)
    # One shared read-only budget. Reserve time within the runner's 90 seconds
    # for existing remote-call/status limits; self-tests are never rerun/skipped.
    preparation_deadline=min(time.monotonic()+60,command_started+(55 if args.use_reserved else 40))
    def failed_preparation(error,failures,position):
        if hasattr(error,'evidence'):
            save_anchor_sampling_failure(dll,args.pid,error,preparation_failures=failures,preparation_receipt=position)
    def prepare(acquire):
        if args.continuous_session and args.anchor_observer:
            return wait_camera_preparation(acquire,check_preparation,preparation_deadline,failure_receipt=failed_preparation)
        check_preparation();value=acquire();check_preparation();return value
    # Initial acquisition's first successful resolve supplies its own reference.
    # Keep it outside prepare() so outer retries cannot silently rebase identity.
    # This consolidates discovery; the former preliminary observation is gone.
    camera=_InitialPreparationCamera()
    certificates_started=time.perf_counter_ns()
    handle=k.OpenProcess(0x43a,False,args.pid)
    if not handle:raise c.WinError(c.get_last_error())
    remote_memory=None;loader_finished=False
    try:
        loaded=modules(handle)
        if args.use_reserved:
            resident=loaded.get(dll.name.lower())
            if not resident or resident[1].resolve()!=dll.resolve():raise RuntimeError('Exact reserved DLL is not resident')
            event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.Reserved.{args.pid}')
            if not event:raise RuntimeError('Reservation readiness not confirmed')
            k.CloseHandle(event)
            if args.first_view_mask_control or args.view_union_mask_fix:
                event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.FirstViewMaskReserved.{args.pid}')
                if not event:raise RuntimeError('First view mask trampoline was not reserved; fresh startup with --check-mask-control is required')
                k.CloseHandle(event)
            if args.require_target_prime:
                prime_status=dll.with_name('target-prime-status.txt').read_text()
                if not pending_prime(prime_status,args.pid,args.continuous_owner_pid,args.continuous_owner_creation_time):
                    raise RuntimeError('Matching pending target priming required; restart for another observation')
                prime_event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.TargetPrime.{args.pid}')
                if not prime_event:raise RuntimeError('Target priming event absent')
                try:
                    if k.WaitForSingleObject(prime_event,0)!=0x102:raise RuntimeError('Target priming already signaled')
                finally:k.CloseHandle(prime_event)
        elif dll.name.lower() in loaded:raise RuntimeError('Capture DLL already loaded; restart game before a new capture build/run')
        active_event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.Stop.{args.pid}')
        if active_event:
            k.CloseHandle(active_event);raise RuntimeError('Another capture is active; overlapping instrumentation rejected')
        # Verify exact target prologues before any remote allocation or load.
        signatures=observer_signatures(args)
        camera,anchor_metadata,anchor_section,ui_metadata,ui_section=prepare(lambda:acquire_observer_preparation(
            args.pid,profile,camera,signatures,dll,anchor_observer=args.anchor_observer,ui_observer=args.ui_observer,
            cinematic_camera=args.cinematic_camera,
            record_anchor_failure=not(args.continuous_session and args.anchor_observer),
            preparation_deadline=preparation_deadline,check_live=check_preparation,
            **({'crosshair_assignment_observer':True} if args.crosshair_assignment_observer else {})))
        check_preparation()
        preparation_timing('certificate_acquisition_including_retry',certificates_started)
        activation_started=time.perf_counter_ns()
        config='[Capture]\n'+f'pid={args.pid}\nseconds={args.seconds}\n'+f"local_player={camera['local_player_address']}\ncontroller={camera['controller_address']}\ntimestamp={camera['timestamp_address']}\n"
        yaw_address=int(camera['pov_address'],16)+int(camera['offsets']['Rotation'],16)+4
        config+=f"camera={camera['camera_address']}\ncamera_yaw={yaw_address:#x}\nwobble={int(args.wobble)}\nsweep={int(args.sweep)}\nlive_head={int(args.live_head)}\n"
        config+=f'gpu_diagnostics={int(args.gpu_diagnostics)}\n'
        config+=f'viewport_diagnostics={int(args.viewport_diagnostics)}\n'
        config+=f'frame_link_diagnostics={int(args.frame_link_diagnostics)}\n'
        config+=f'outage_diagnostics={int(args.outage_diagnostics)}\n'
        config+=f'anchor_observer={int(args.anchor_observer)}\n'
        config+=f'ui_observer={int(args.ui_observer)}\n'
        config+=f'crosshair_assignment_observer={int(args.crosshair_assignment_observer)}\n'
        config+=f'ui_hud_plane={int(getattr(args,"ui_hud_plane",False))}\n'
        config+=f'ui_reticle_late_sample={int(getattr(args,"ui_reticle_late_sample",False))}\n'
        config+=hud_size_ini(args)
        config+=f'origin_trial={int(getattr(args,"origin_trial",False))}\n'
        config+=f'stereo_gpu_publish={int(args.stereo_gpu_publish)}\n'
        config+=f'simulator_operator={int(args.simulator_operator)}\n'
        config+=f'continuous_session={int(args.continuous_session)}\n'
        if args.continuous_session:
            config+=f'continuous_owner_pid={args.continuous_owner_pid}\ncontinuous_owner_creation_time={args.continuous_owner_creation_time}\n'
        config+=f'color_copy_diagnostics={int(args.color_copy_diagnostics)}\n'
        config+=f'first_view_mask_control={int(args.first_view_mask_control)}\n'
        config+=f'projection_control={int(args.projection_control)}\n'
        config+=f'stereo_xr={int(args.stereo_xr)}\nstereo_units_per_metre={args.stereo_units_per_metre:.9g}\n'
        config+=f'view_union_mask_fix={int(args.view_union_mask_fix)}\n'
        config+=f'family_diagnostics={int(args.family_diagnostics)}\n'
        config+=f'shadow_view={int(args.shadow_view)}\n'
        config+=f'shadow_state={int(args.shadow_state)}\n'
        config+=f'shadow_constructor={int(args.shadow_constructor)}\n'
        config+=f'constructor_state={int(args.constructor_state)}\n'
        config+=f'constructor_eye={int(args.constructor_eye)}\n'
        config+=f'renderer_pair={int(args.renderer_pair)}\n'
        config+=f'submit_pair={int(args.submit_pair)}\n'
        config+=f'submit_pair_seconds={args.submit_pair_seconds}\n'
        config+=f'trial_seconds={args.trial_seconds}\n'
        config+=f'submit_background={int(args.submit_background)}\n'
        config+=f'submit_output_bounds={int(args.submit_output_bounds)}\n'
        config+=f'quad_left_source_control={int(args.quad_left_source_control)}\n'
        config+=f'quad_right_source_control={int(args.quad_right_source_control)}\n'
        config+=f'submit_mono_control={int(args.submit_mono_control)}\n'
        config+=f'submit_mono_independent_state={int(args.submit_mono_independent_state)}\n'
        config+=f'submit_duplicate_control={int(args.submit_duplicate_control)}\n'
        config+=anchor_section
        config+=ui_section
        if ui_metadata is not None:
            dll.with_name('ui-observer-config.json').write_text(json.dumps(ui_metadata,indent=2,allow_nan=False)+'\n',encoding='utf-8')
        if anchor_metadata is not None:
            dll.with_name('anchor-observer-config.json').write_text(json.dumps(anchor_metadata,indent=2,allow_nan=False)+'\n',encoding='utf-8')
        dll.with_name('capture.ini').write_text(config,encoding='utf-16')
        check_preparation()
        # GetProcAddress may forward LoadLibraryW into KernelBase. Identify its
        # actual containing module, then relocate by that module's remote base.
        if not args.use_reserved:
            local_loader=k.GetProcAddress(k.GetModuleHandleW('kernel32.dll'),b'LoadLibraryW')
            owner=w.HMODULE()
            if not local_loader or not k.GetModuleHandleExW(6,local_loader,c.byref(owner)):
                raise c.WinError(c.get_last_error())
            owner_path=c.create_unicode_buffer(32768);k.GetModuleFileNameW(owner,owner_path,len(owner_path))
            owner_name=pathlib.Path(owner_path.value).name.lower()
            if owner_name not in loaded:raise RuntimeError('Remote loader owner module not found')
            remote_loader=loaded[owner_name][0]+local_loader-owner.value
            encoded=(str(dll.resolve())+'\0').encode('utf-16-le')
            remote_memory=k.VirtualAllocEx(handle,None,len(encoded),0x3000,0x04)
            if not remote_memory:raise c.WinError(c.get_last_error())
            copied=c.c_size_t();buffer=c.create_string_buffer(encoded)
            if not k.WriteProcessMemory(handle,remote_memory,buffer,len(encoded),c.byref(copied)) or copied.value!=len(encoded):
                raise c.WinError(c.get_last_error())
            remote_call(handle,remote_loader,remote_memory);loader_finished=True
        loaded=modules(handle)
        if dll.name.lower() not in loaded:raise RuntimeError('Capture DLL did not load')
        module_base,module_path=loaded[dll.name.lower()]
        if module_path.resolve()!=dll.resolve():raise RuntimeError('Loaded capture DLL path differs')
        pe=pefile.PE(str(dll))
        export=next(e for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'GearsCaptureStart')
        if export.forwarder:raise RuntimeError('Unexpected forwarded capture export')
        status=dll.with_name('capture-status.txt')
        previous_status_mtime=status.stat().st_mtime_ns if status.exists() else None
        check_preparation()
        if startup_proof is not None:
            from startup_selftests import validate_proof
            validate_proof(startup_proof,dll,dict(pid=args.continuous_owner_pid,creation_time=args.continuous_owner_creation_time),game,args.startup_selftest_nonce)
        check_preparation()
        code=remote_call(handle,module_base+export.address,None)
        if code:raise RuntimeError(f'Capture start failed: {code}')
        deadline=time.monotonic()+15
        while time.monotonic()<deadline:
            event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.Stop.{args.pid}')
            if event:
                k.CloseHandle(event)
                preparation_timing('activation_until_confirmed',activation_started)
                print(f'Capture active for PID {args.pid}; status file is readable after capture ends.')
                return
            try:
                fresh=status.exists() and status.stat().st_mtime_ns!=previous_status_mtime
                value=status.read_text(errors='replace') if fresh else ''
            except PermissionError:
                # The CRT status file stays locked until capture ends. This
                # named event is created only after queued hooks are enabled.
                event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.Stop.{args.pid}')
                if event:
                    k.CloseHandle(event)
                    preparation_timing('activation_until_confirmed',activation_started)
                    print(f'Capture active for PID {args.pid}; status file is readable after capture ends.')
                    return
                value=''
            if f'capturing pid={args.pid} ' in value:
                preparation_timing('activation_until_confirmed',activation_started)
                print(value.strip());return
            if 'rejected:' in value or 'failed' in value:raise RuntimeError(value.strip())
            time.sleep(.1)
        raise RuntimeError('Capture start status not confirmed; inspect capture-status.txt before continuing')
    finally:
        if remote_memory and loader_finished:k.VirtualFreeEx(handle,remote_memory,0,0x8000)
        k.CloseHandle(handle)


if __name__=='__main__':main()
