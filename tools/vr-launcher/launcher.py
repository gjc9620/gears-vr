"""Repository stereo launcher with optional verified menu navigation."""
import argparse
import ctypes
import hashlib
import json
import math
import os
import pathlib
import subprocess
import struct
import sys
import time
import uuid
from types import SimpleNamespace

ROOT=pathlib.Path(__file__).resolve().parents[2]
HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'tools/debug-launch'))
import instance_policy
INDEX=instance_policy.index_path()
SESSION_OPTIONS=('anchor_observer','cinematic_camera','outage_diagnostics','image_ticket_diagnostics','shutdown_diagnostics','ui_observer','ui_hud_plane','ui_reticle_late_sample','gpu_snapshot','menu_recovery','world_recovery','crosshair_assignment_observer','movie_display','path_choice_center')

def atomic_json(path,value):
    path.parent.mkdir(parents=True,exist_ok=True)
    temp=path.with_name(path.name+'.tmp')
    temp.write_text(json.dumps(value,indent=2),encoding='utf-8')
    for attempt in range(6):
        try:os.replace(temp,path);return
        except PermissionError:
            if attempt==5:raise
            time.sleep(.02)

def rooted(value,root=ROOT):
    path=pathlib.Path(value)
    return (root/path).resolve() if not path.is_absolute() else path.resolve()

def hud_horizontal_fov(config,manifest):
    supported=manifest.get('hud_size_control',False)
    if type(supported) is not bool:raise ValueError('Manifest hud_size_control must be boolean')
    explicit='hud_horizontal_fov_degrees' in config
    if explicit and not supported:raise ValueError('Manifest does not support hud_size_control')
    if explicit and not manifest.get('ui_hud_plane',False):raise ValueError('HUD size requires ui_hud_plane')
    if not explicit and not (supported and manifest.get('ui_hud_plane',False)):return None
    value=config.get('hud_horizontal_fov_degrees',65.)
    if isinstance(value,bool) or not isinstance(value,(int,float)) or not 45<=value<=100 or not math.isfinite(value):
        raise ValueError('hud_horizontal_fov_degrees must be finite and between 45 and 100')
    return float(value)



def hud_layout(config,manifest):
    supported=manifest.get('hud_layout_control',False)
    if type(supported) is not bool:raise ValueError('Manifest hud_layout_control must be boolean')
    fields=('hud_element_scale','hud_center_scale')
    explicit=any(key in config for key in fields)
    if explicit and not supported:raise ValueError('Manifest does not support hud_layout_control')
    if (supported or explicit) and not manifest.get('ui_hud_plane',False):
        raise ValueError('HUD layout requires ui_hud_plane')
    if not supported:return {}
    result={}
    for key,minimum,maximum in (('hud_element_scale',1,2),('hud_center_scale',0,1)):
        default=manifest.get(key,1.)
        if isinstance(default,bool) or not isinstance(default,(int,float)) or not minimum<=default<=maximum or not math.isfinite(default):
            raise ValueError(f'Manifest {key} must be finite and between {minimum} and {maximum}')
        value=config.get(key,default)
        if isinstance(value,bool) or not isinstance(value,(int,float)) or not minimum<=value<=maximum or not math.isfinite(value):
            raise ValueError(f'{key} must be finite and between {minimum} and {maximum}')
        result[key]=float(value)
    return result


def camera_closer(config,manifest):
    supported=manifest.get('camera_distance_control',False)
    if type(supported) is not bool:raise ValueError('Manifest camera_distance_control must be boolean')
    value=config.get('camera_closer_metres',0.)
    if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not 0<=value<=.5:
        raise ValueError('camera_closer_metres must be finite and between 0 and 0.5')
    if value>0 and (not supported or not all(manifest.get(key,False) for key in ('anchor_observer','cinematic_camera'))):
        raise ValueError('Camera closer requires supported build and cinematic/anchor guards')
    return float(value)


def render_scale(config,manifest):
    supported=manifest.get('runtime_render_size',False)
    if type(supported) is not bool:raise ValueError('Manifest runtime_render_size must be boolean')
    if not supported:
        if 'render_scale' in config:raise ValueError('This build does not support runtime render size')
        return None
    value=config.get('render_scale',1.)
    if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not .25<=value<=2:
        raise ValueError('render_scale must be finite and between 0.25 and 2.0 (per dimension)')
    return float(value)


def render_size_info(manifest,scale,root=ROOT):
    probe=rooted(manifest['xr_build'],root)/'OpenXRPoseProbe.exe'
    if hashlib.sha256(probe.read_bytes()).hexdigest().lower()!=manifest['xr_sha256'].lower():
        raise RuntimeError('XR render size probe SHA256 mismatch')
    result=subprocess.run([str(probe),'--render-size-info','--render-scale',str(scale)],
        capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=30,
        creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    if result.returncode:raise RuntimeError('VR render size rejected before game launch: '+(result.stderr.strip() or result.stdout.strip()))
    plans=[]
    for line in result.stdout.splitlines():
        try:value=json.loads(line)
        except ValueError:continue
        if isinstance(value,dict) and value.get('runtime_render_size_plan') is True:plans.append(value)
    if len(plans)!=1:raise RuntimeError('Runtime did not return one render size plan')
    plan=plans[0]
    for key in ('recommended','eye','source'):
        if not isinstance(plan.get(key),list) or len(plan[key])!=2 or any(type(v) is not int or v<1 for v in plan[key]):
            raise RuntimeError('Runtime render size plan has invalid dimensions')
    w,h=plan['eye'];rw,rh=plan['recommended']
    if (plan.get('render_scale')!=scale or plan['source']!=[2*w,h] or not 64<=w<=4096 or not 64<=h<=8192 or
        w!=math.ceil(rw*scale-1e-9) or h!=math.ceil(rh*scale-1e-9)):
        raise RuntimeError('Runtime render size plan does not match the requested multiplier and engine limits')
    return plan


def settings(config,manifest,root=ROOT):
    if set(config)-{'game_path','stereo_units_per_metre','hud_horizontal_fov_degrees','launcher_language','hud_element_scale','hud_center_scale','camera_closer_metres','render_scale'}:raise ValueError('Unknown configuration field')
    camera_closer(config,manifest)
    render_scale(config,manifest)
    if 'launcher_language' in config and config['launcher_language'] not in ('zh-CN','en'):
        raise ValueError('launcher_language must be zh-CN or en')
    hud_horizontal_fov(config,manifest)
    hud_layout(config,manifest)
    if manifest.get('schema')!=1:raise ValueError('Unsupported manifest schema')
    if any(type(manifest.get(key,False)) is not bool for key in (*SESSION_OPTIONS,'startup_menu','startup_selftests_prelaunch','graphics_startup_lock','graphics_menu_lock','experimental_preserve_upscaler')):
        raise ValueError('Manifest session options must be boolean')
    if manifest.get('experimental_preserve_upscaler',False) and any(manifest.get(key,False) for key in ('graphics_startup_lock','graphics_menu_lock')):
        raise ValueError('Experimental preserve upscaler conflicts with graphics locks')
    if manifest.get('startup_selftests_prelaunch',False) and not manifest.get('startup_menu',False):
        raise ValueError('Prelaunch self-tests require owned startup')
    if manifest.get('graphics_startup_lock',False) and not manifest.get('startup_menu',False):
        raise ValueError('Startup graphics lock requires owned startup menu lifecycle')
    if manifest.get('menu_recovery',False) and not manifest.get('startup_menu',False):
        raise ValueError('Menu recovery requires startup menu lifecycle')
    if manifest.get('movie_display',False) and not manifest.get('menu_recovery',False):
        raise ValueError('Movie display requires menu recovery and a movie-capable XR build')
    if manifest.get('path_choice_center',False) and not all(manifest.get(k,False) for k in ('menu_recovery','ui_observer','anchor_observer')):
        raise ValueError('PathChoice centering requires menu/UI/anchor observers')
    if manifest.get('graphics_menu_lock',False) and not manifest.get('graphics_startup_lock',False):
        raise ValueError('Menu lock requires startup graphics protection')
    if manifest.get('ui_hud_plane',False) and not (manifest.get('ui_observer',False) and manifest.get('anchor_observer',False)):
        raise ValueError('HUD plane requires UI observer and anchor observer in the continuous launcher')
    if manifest.get('ui_reticle_late_sample',False) and not all(manifest.get(key,False) for key in ('ui_observer','ui_hud_plane')):
        raise ValueError('Late reticle sampling requires UI observer and HUD plane in the continuous launcher')
    if manifest.get('crosshair_assignment_observer',False) and not all(manifest.get(key,False) for key in ('ui_observer','anchor_observer')):
        raise ValueError('Crosshair assignment observer requires UI observer and anchor observer')
    if manifest.get('world_recovery',False) and not all(manifest.get(key,False) for key in ('anchor_observer','ui_observer','cinematic_camera')):
        raise ValueError('World recovery requires anchor/UI observers and cinematic camera')
    if manifest.get('cinematic_camera',False) and not manifest.get('anchor_observer',False):
        raise ValueError('Cinematic camera requires anchor observer')
    scale=config.get('stereo_units_per_metre',100)
    if isinstance(scale,bool) or not isinstance(scale,(int,float)) or not math.isfinite(scale) or not 10<=scale<=500:
        raise ValueError('stereo_units_per_metre must be finite and between 10 and 500')
    if not isinstance(config.get('game_path',manifest['game_path']),str) or not str(config.get('game_path',manifest['game_path'])).strip():
        raise ValueError('game_path must name an executable file')
    game=rooted(config.get('game_path',manifest['game_path']),root)
    return game,float(scale)

def verify_files(manifest,game,root=ROOT):
    sys.path.insert(0,str(ROOT/'tools/debug-launch'))
    from game_install import validate_game
    validate_game(game,root,manifest['game_sha256'])
    checks=((game,manifest['game_sha256']),
        (rooted(manifest['native_build'],root)/manifest['native_dll'],manifest['native_sha256']),
        (rooted(manifest['xr_build'],root)/'OpenXRPoseProbe.exe',manifest['xr_sha256']))
    for path,expected in checks:
        if hashlib.sha256(path.read_bytes()).hexdigest().lower()!=expected.lower():
            raise RuntimeError('Manifest SHA256 mismatch: '+str(path))
    profile=json.loads((root/'tools/package-inspector/reloaded-profile.json').read_text())
    if profile['sha256'].lower()!=manifest['game_sha256'].lower():raise RuntimeError('Resolver profile differs from manifest')
    return profile

def registered(resolver,address):
    obj=resolver.identity(address)
    if not obj or obj['internal_index']>=len(resolver.object_pointers) or resolver.object_pointers[obj['internal_index']]!=address:
        raise RuntimeError('Player object registry mismatch')
    if obj['name'].startswith('Default__'):raise RuntimeError('Default object is not a live player')
    return obj

def possessed_player(resolver,camera):
    """Reflect live possession; a plausible menu camera alone is insufficient."""
    pc=int(camera['controller_address'],16);controller=registered(resolver,pc)
    pawn_prop=resolver.property(controller['cls'],'Pawn',True)
    if pawn_prop['type']!='ObjectProperty':raise RuntimeError('Unsupported Pawn property type')
    pawn=resolver.u64(pc+pawn_prop['offset'])
    if not pawn:raise RuntimeError('Waiting for a possessed level pawn')
    obj=registered(resolver,pawn)
    cls=obj['cls'];seen=set();war_pawn=False
    while cls and cls not in seen and len(seen)<128:
        seen.add(cls)
        if resolver.identity(cls)['name']=='WarPawn':war_pawn=True;break
        cls=resolver.u64(cls+resolver.profile['field_super'])
    if not war_pawn:raise RuntimeError('Possessed pawn is not a WarPawn subclass')
    backlink=resolver.property(obj['cls'],'Controller',True)
    if backlink['type']!='ObjectProperty' or resolver.u64(pawn+backlink['offset'])!=pc:
        raise RuntimeError('Pawn/controller possession mismatch')
    outer=obj['outer'];seen=set();level=0
    while outer and outer not in seen and len(seen)<128:
        seen.add(outer);owner=registered(resolver,outer)
        if resolver.class_name(owner)=='Level':level=outer;break
        outer=owner['outer']
    if not level:raise RuntimeError('Pawn has no registered level owner')
    if resolver.u64(pc+pawn_prop['offset'])!=pawn or resolver.u64(pawn+backlink['offset'])!=pc:
        raise RuntimeError('Possession changed during discovery')
    stamp=struct.unpack('<f',resolver.read(int(camera['timestamp_address'],16),4))[0]
    if not math.isfinite(stamp):raise RuntimeError('Invalid camera timestamp')
    return {'pawn':hex(pawn),'controller':hex(pc),'camera':camera['camera_address'],'level':hex(level),'timestamp':stamp}

def stable_player(current,previous):
    return bool(current and previous and current['timestamp']>previous['timestamp'] and
        all(current[key]==previous[key] for key in ('pawn','controller','camera','level')))

def cancel_name(pid):return 'Local\\GearsVR.RepositoryLauncher.Stop.'+str(pid)

class Backend:
    def __init__(self):
        sys.path.insert(0,str(ROOT/'tools/native-capture'));sys.path.insert(0,str(ROOT/'tools/package-inspector'))
        import run_stereo_continuous
        from resolve_camera import Resolver
        self.runner=run_stereo_continuous;self.Resolver=Resolver;self.control=self.runner.WindowsControl()
        from ctypes import wintypes
        self.control.k.CreateEventW.argtypes=[ctypes.c_void_p,wintypes.BOOL,wintypes.BOOL,wintypes.LPCWSTR]
        self.control.k.CreateEventW.restype=wintypes.HANDLE
        self.control.k.WaitForSingleObject.argtypes=[wintypes.HANDLE,wintypes.DWORD]
        self.control.k.WaitForSingleObject.restype=wintypes.DWORD
    def open_cancel(self,pid):
        ctypes.set_last_error(0)
        handle=self.control.k.CreateEventW(None,True,False,cancel_name(pid))
        error=ctypes.get_last_error()
        if not handle:raise ctypes.WinError(error)
        if error==183:self.control.k.CloseHandle(handle);raise RuntimeError('Startup stop event already exists')
        return handle
    def cancelled(self,handle):
        result=self.control.k.WaitForSingleObject(handle,0)
        if result not in (0,258):raise RuntimeError('Startup stop wait failed')
        return result==0
    def game_pids(self):
        result=subprocess.run(['powershell','-NoProfile','-NonInteractive','-Command',
            "Get-CimInstance Win32_Process -Filter 'Name=''GOWDE-Steam.exe''' | Select-Object -ExpandProperty ProcessId"],capture_output=True,text=True,check=True,
            creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        pids=[int(x) for x in result.stdout.splitlines() if x.strip().isdigit()]
        if not pids:return []
        if instance_policy.context():
            if not INDEX.exists():return []
            current=json.loads(INDEX.read_text(encoding='utf-8')).get('game_identity')
            return [current['pid']] if current and current['pid'] in pids and self.control.identity(current['pid'])==current else []
        return instance_policy.shared_game_pids(pids,identity=self.control.identity)
    def reserve(self,manifest,log,owner=None,game_path=None,foreground=False):
        receipt=log.with_name(log.name+'.'+uuid.uuid4().hex+'.birth.json')
        preflight_nonce=uuid.uuid4().hex if owner and manifest.get('startup_selftests_prelaunch',False) else None
        cmd=[sys.executable,str(ROOT/'tools/native-capture/launch_reserve.py'),'--build-dir',str(rooted(manifest['native_build'])),
             '--dll-name',manifest['native_dll'],'--prime-targets','--check-shadow-state','--check-mask-control','--check-frame-link',
             '--launch-receipt',str(receipt)]
        if game_path is not None:cmd+=['--game-path',str(game_path)]
        if foreground:cmd+=['--foreground']
        if manifest.get('experimental_preserve_upscaler',False):cmd+=['--experimental-preserve-upscaler']
        if owner:
            cmd+=['--prime-owner-pid',str(owner['pid']),'--prime-owner-creation-time',str(owner['creation_time'])]
        if preflight_nonce:cmd+=['--startup-preflight-nonce',preflight_nonce]
        if manifest.get('graphics_startup_lock',False):cmd+=['--graphics-startup-lock']
        if manifest.get('graphics_menu_lock',False):cmd+=['--graphics-menu-lock']
        selected=rooted(game_path if game_path is not None else manifest['game_path'])
        def failed_cleanup():
            try:cleanup=self.close_uninjected_reservation(receipt,selected)
            except Exception as error:cleanup={'status':'cleanup_error','error':repr(error)}
            with log.open('a',encoding='utf-8') as stream:stream.write('\n'+json.dumps({'reservation_failure_cleanup':cleanup})+'\n')
            return cleanup
        try:
            result=subprocess.run(cmd,capture_output=True,text=True,timeout=120,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        except subprocess.TimeoutExpired as error:
            def decoded(value):return value.decode('utf-8',errors='replace') if isinstance(value,bytes) else value or ''
            log.write_text(decoded(error.stdout)+'\n'+decoded(error.stderr),encoding='utf-8')
            error.reservation_failure_cleanup=failed_cleanup();raise
        log.write_text(result.stdout+'\n'+result.stderr,encoding='utf-8')
        if result.returncode:
            error=RuntimeError('Fresh reservation failed; no retry: '+str(log))
            error.reservation_failure_cleanup=failed_cleanup()
            raise error
        reports=[]
        for line in result.stdout.splitlines():
            try:value=json.loads(line)
            except ValueError:continue
            if isinstance(value,dict) and value.get('trampolines_reserved'):reports.append(value)
        if len(reports)!=1:raise RuntimeError('Reservation did not return one accepted PID')
        report=reports[0]
        graphics_required=manifest.get('graphics_startup_lock',False)
        graphics_count=4 if manifest.get('graphics_menu_lock',False) else 3 if graphics_required else 0
        if (not report.get('hooks_enabled') or report.get('native_hooks_enabled')!=graphics_required or
                report.get('graphics_hooks_enabled',0)!=graphics_count or report.get('scene_hooks_enabled',False) or
                not report.get('first_view_mask_reserved') or not all(x['matches'] for x in report['independent_entry_checks'])):
            raise RuntimeError('Reservation proof rejected')
        if owner and (report.get('prime_lifetime')!='owner' or report.get('prime_owner_pid')!=owner['pid'] or
                      report.get('prime_owner_creation_time')!=owner['creation_time']):
            raise RuntimeError('Owned target-prime reservation identity mismatch')
        selected=rooted(game_path if game_path is not None else manifest['game_path'])
        accepted=report.get('game_identity')
        if (not isinstance(accepted,dict) or accepted.get('pid')!=report['pid'] or
                type(accepted.get('creation_time')) is not int or accepted['creation_time']<=0 or
                rooted(accepted.get('image',''))!=selected or
                report.get('game_sha256','').lower()!=manifest['game_sha256'].lower()):
            raise RuntimeError('Reservation executable identity/hash mismatch')
        if preflight_nonce:
            sys.path.insert(0,str(ROOT/'tools/native-capture'))
            from startup_selftests import validate_proof,encoded
            proof=report.get('startup_selftest_proof')
            proof_dll=rooted(manifest['native_build'])/manifest['native_dll']
            validate_proof(proof,proof_dll,owner,accepted,preflight_nonce)
            if proof['artifacts'][str(proof_dll.resolve()).casefold()]!=manifest['native_sha256']:
                raise RuntimeError('Startup self-test DLL differs from manifest')
            path=log.with_name(log.name+'.'+preflight_nonce+'.selftests.json')
            raw=encoded(proof)
            with path.open('xb') as stream:stream.write(raw)
            report['startup_selftest_handshake']=dict(path=str(path),digest=hashlib.sha256(raw).hexdigest(),nonce=preflight_nonce)
        return report
    def close_uninjected_reservation(self,receipt,selected,*,clock=time.monotonic,sleep=time.sleep):
        if not receipt.exists():return {'status':'no_birth_receipt'}
        value=json.loads(receipt.read_text(encoding='utf-8'));identity=value.get('game_identity')
        if (value.get('schema')!=1 or value.get('kind')!='reservation_birth' or
                value.get('injection_attempted') is not False or value.get('prime_attempted') is not False):
            return {'status':'injection_state_unknown_or_attempted'}
        if (not isinstance(identity,dict) or type(identity.get('pid')) is not int or identity['pid']<=0 or
                type(identity.get('creation_time')) is not int or identity['creation_time']<=0 or
                not isinstance(identity.get('image'),str) or rooted(identity['image'])!=selected):
            return {'status':'birth_identity_rejected'}
        deadline=clock()+10;posted=set()
        while clock()<deadline:
            current=self.control.identity(identity['pid'])
            if current is None:return {'status':'exited','game_identity':identity,'wm_close_posted':len(posted)}
            if current!=identity:return {'status':'identity_changed','wm_close_posted':len(posted)}
            for window in self.game_windows(identity['pid']):
                if self.control.identity(identity['pid'])!=identity:
                    return {'status':'identity_changed','wm_close_posted':len(posted)}
                if window not in posted:
                    self.post_normal_close(window,identity);posted.add(window)
            sleep(min(.1,max(0,deadline-clock())))
        return {'status':'close_timeout','game_identity':identity,'wm_close_posted':len(posted)}
    def game_windows(self,pid):
        from ctypes import wintypes
        user=ctypes.WinDLL('user32',use_last_error=True);windows=[]
        callback=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
        user.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
        user.EnumWindows.argtypes=[callback,wintypes.LPARAM]
        def collect(window,_):
            owner=wintypes.DWORD();user.GetWindowThreadProcessId(window,ctypes.byref(owner))
            if owner.value==pid:windows.append(window)
            return True
        if not user.EnumWindows(callback(collect),0):raise ctypes.WinError(ctypes.get_last_error())
        return windows
    def post_normal_close(self,window,identity):
        from ctypes import wintypes
        user=ctypes.WinDLL('user32',use_last_error=True)
        user.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
        user.PostMessageW.argtypes=[wintypes.HWND,wintypes.UINT,wintypes.WPARAM,wintypes.LPARAM]
        owner=wintypes.DWORD();user.GetWindowThreadProcessId(window,ctypes.byref(owner))
        if owner.value!=identity['pid'] or self.control.identity(identity['pid'])!=identity:
            raise RuntimeError('Close window/process ownership changed')
        if not user.PostMessageW(window,0x10,0,0):raise ctypes.WinError(ctypes.get_last_error())
    def game_window(self,pid,*,diagnostic=None):
        from ctypes import wintypes
        user=ctypes.WinDLL('user32',use_last_error=True)
        callback=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
        user.EnumWindows.argtypes=[callback,wintypes.LPARAM];user.EnumWindows.restype=wintypes.BOOL
        user.IsWindowVisible.argtypes=[wintypes.HWND];user.IsWindowVisible.restype=wintypes.BOOL
        user.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
        user.GetClientRect.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.RECT)]
        windows=[];matched=0
        if diagnostic is not None:diagnostic.clear();diagnostic.update(candidates=[],dropped_candidates=0)
        @callback
        def visit(hwnd,_):
            nonlocal matched
            actual=wintypes.DWORD();user.GetWindowThreadProcessId(hwnd,ctypes.byref(actual))
            if actual.value!=pid:return True
            matched+=1
            visible=bool(user.IsWindowVisible(hwnd));rect=wintypes.RECT();rectangle=None
            # Preserve the original short-circuit reads: hidden windows have an
            # unknown client rect, rather than adding an extra native query.
            reason='not_visible'
            if visible:
                if user.GetClientRect(hwnd,ctypes.byref(rect)):
                    rectangle=dict(left=rect.left,top=rect.top,right=rect.right,bottom=rect.bottom)
                    reason=None if rect.right>=640 and rect.bottom>=360 else 'client_too_small'
                else:reason='client_rect_unavailable'
            if reason is None:windows.append(int(hwnd))
            if diagnostic is not None:
                if len(diagnostic['candidates'])<16:
                    diagnostic['candidates'].append(dict(hwnd=int(hwnd),visible=visible,client_rect=rectangle,
                                                        eligible=reason is None,rejection=reason))
                else:diagnostic['dropped_candidates']+=1
            return True
        if not user.EnumWindows(visit,0):
            if diagnostic is not None:diagnostic.update(reason='enum_failed',matched_pid_windows=matched)
            raise ctypes.WinError(ctypes.get_last_error())
        if diagnostic is not None:
            diagnostic.update(matched_pid_windows=matched,eligible_windows=len(windows),
                reason='multiple_eligible_windows' if len(windows)>1 else 'accepted' if windows else
                       'no_eligible_window' if matched else 'no_pid_window')
        if len(windows)>1:raise RuntimeError('Multiple game windows; cannot select startup panel')
        return windows[0] if windows else None

    def finish_owned_prime(self,manifest,identity,owner):
        if self.control.identity(identity['pid']) is None:return {'game_exited':True}
        self.runner.checked_identity(self.control,identity)
        path=rooted(manifest['native_build'])/'target-prime-status.txt'
        text=path.read_text()
        captures=[line for line in text.splitlines() if line.startswith('capturing ')]
        fields=dict(token.split('=',1) for token in captures[0].split()[1:] if '=' in token) if len(captures)==1 else {}
        required={'pid':str(identity['pid']),'lifetime':'owner','owner_pid':str(owner['pid']),
                  'owner_creation_time':str(owner['creation_time'])}
        if any(fields.get(key)!=value for key,value in required.items()):raise RuntimeError('Prime cleanup identity mismatch')
        if any(line.startswith('handed_off ') for line in text.splitlines()):return {'handed_off':True}
        if not self.control.signal(f"Local\\GearsVR.Capture.TargetPrime.{identity['pid']}"):
            raise RuntimeError('Owned prime cancellation event absent')
        deadline=time.monotonic()+15
        while time.monotonic()<deadline:
            self.runner.checked_identity(self.control,identity)
            text=path.read_text()
            stops=[line for line in text.splitlines() if line.startswith('stopped ')]
            if stops:
                fields=dict(token.split('=',1) for token in stops[-1].split()[1:] if '=' in token)
                if any(fields.get(key)!=value for key,value in {'disable_status':'0','active':'0','entries_match':'1'}.items()):
                    raise RuntimeError('Owned prime cleanup did not restore all entries')
                return {'cancelled':True,'entry_restore_verified':True}
            time.sleep(.05)
        raise TimeoutError('Owned prime restoration is still pending')
    def require_prime_active(self,identity):
        self.runner.checked_identity(self.control,identity)
        handle=self.control.k.OpenEventW(0x100000,False,f"Local\\GearsVR.Capture.TargetPrime.{identity['pid']}")
        if not handle:raise RuntimeError('Startup graphics protection event absent')
        try:
            if self.control.k.WaitForSingleObject(handle,0)!=258:
                raise RuntimeError('Startup graphics protection stopped before handoff')
        finally:self.control.k.CloseHandle(handle)
    def player(self,pid,profile):
        from anchor_config import LiveRegistryAnchorSampler
        resolver=self.Resolver(pid,profile)
        try:
            sampler=LiveRegistryAnchorSampler(resolver)
            if sampler.view_target is None:
                raise RuntimeError('Waiting for readable Camera.ViewTarget.Target')
            current=sampler.guard()
            for role in ('pawn','controller','camera','local_player','level','outer_chain','view_target'):
                if current[role]!=sampler.initial[role]:
                    raise RuntimeError('Anchor relationship changed during player discovery: '+role)
            # Use the sampler's one resolved camera chain, never a second resolve
            # whose identities might belong to a different transition frame.
            for role,key in (('controller','controller_address'),('camera','camera_address'),('local_player','local_player_address')):
                if current[role]['address']!=int(sampler.resolved[key],16):
                    raise RuntimeError('Resolved camera relationship changed: '+role)
            if current['view_target']!=current['pawn']['address']:
                raise RuntimeError('Camera view target is not controlled Pawn')
            stamp=struct.unpack('<f',bytes.fromhex(current['timestamp_raw']))[0]
            if not math.isfinite(stamp):raise RuntimeError('Invalid camera timestamp')
            return {**{role:hex(current[role]['address']) for role in ('pawn','controller','camera','level')},'timestamp':stamp}
        finally:resolver.p.close()
    def start_tiny_output_monitor(self,manifest,identity,diagnostic):
        sys.path.insert(0,str(ROOT/'tools/debug-launch'))
        from tiny_output_recovery import TinyOutputMonitor
        return TinyOutputMonitor(rooted(manifest['native_build'])/'swapchain-output-status.txt',
                                 diagnostic,identity,
                                 allow_recovery=os.environ.get('GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY')!='1').start()
    def navigate(self,pid,identity,cancel,deadline,log,check_live=None):
        sys.path.insert(0,str(HERE))
        from menu_navigation import navigate
        return navigate(pid,identity,self,cancel,deadline,log,check_live=check_live)

def start(config,manifest,backend,index=INDEX,clock=time.monotonic,sleep=time.sleep,auto_continue=False,foreground=False):
    instance_policy.require_private_manifest(manifest)
    game,scale=settings(config,manifest)
    profile=verify_files(manifest,game) # Reject unsupported installs before startup side effects.
    selected_render_scale=render_scale(config,manifest)
    size_plan=render_size_info(manifest,selected_render_scale) if selected_render_scale is not None else None
    if size_plan:
        print(json.dumps(size_plan),flush=True)
        print('VR per eye: %d x %d; render scale %.2f per dimension; desktop resolution is independent.' %
              (*size_plan['eye'],selected_render_scale),flush=True)
    owner=backend.control.identity(os.getpid())
    if not owner:raise RuntimeError('Launcher process identity unavailable')
    ctypes.set_last_error(0)
    lock=backend.control.k.CreateMutexW(None,False,instance_policy.owner_lock_name())
    if not lock:raise ctypes.WinError(ctypes.get_last_error())
    if ctypes.get_last_error()==183:
        backend.control.k.CloseHandle(lock);raise RuntimeError('Another repository launcher is active')
    state={'schema':1,'kind':'repository_vr_launcher','owner_identity':owner,'phase':'preflight','passed':False,'native_activated':False}
    if instance_policy.context():state['instance_root']=str(instance_policy.context())
    if size_plan:state['render_size_plan']=size_plan
    audio_state_key='test_audio_guard'
    cancel=None;identity=None;tiny_output_monitor=None;test_audio_guard=None;owned_prime=manifest.get('startup_menu',False)
    try:
        cancel=backend.open_cancel(owner['pid'])
        if backend.game_pids():raise RuntimeError('An existing game process must exit normally before fresh startup')
        atomic_json(index,state)
        started=clock();state['phase']='reserving';atomic_json(index,state)
        reserved=(backend.reserve(manifest,index.with_suffix('.reserve.log'),owner=owner,game_path=game,foreground=foreground) if owned_prime else
                  backend.reserve(manifest,index.with_suffix('.reserve.log'),game_path=game,foreground=foreground))
        pid=reserved['pid'];identity=backend.control.identity(pid)
        if not identity or identity!=reserved.get('game_identity') or pathlib.Path(identity['image']).resolve()!=game:raise RuntimeError('Reserved game identity mismatch')
        if os.environ.get('GEARSVR_TEST_MUTE_GAME')=='1':
            sys.path.insert(0,str(HERE))
            from game_audio_guard import begin
            test_audio_guard=begin(identity,owner,index.with_name(index.stem+'.'+uuid.uuid4().hex+'.audio.jsonl'))
            state['test_audio_guard']={'enabled':True,'log':str(test_audio_guard.log),'muting_confirmed':False}
        elif foreground:
            sys.path.insert(0,str(HERE))
            from game_audio_guard import begin_manual
            test_audio_guard=begin_manual(identity,owner,index.with_name(index.stem+'.'+uuid.uuid4().hex+'.audio.jsonl'))
            if test_audio_guard:
                audio_state_key='manual_game_audio'
                state[audio_state_key]={'enabled':True,'log':str(test_audio_guard.log),'configured':False}
        try:
            tiny_output_monitor=backend.start_tiny_output_monitor(manifest,identity,index.with_suffix('.tiny-output.jsonl'))
        except Exception as error:
            # An unavailable diagnostic observer must not abort startup.
            try:
                with index.with_suffix('.tiny-output.jsonl').open('a',encoding='utf-8') as stream:
                    stream.write(json.dumps(dict(event='monitor_start_error',error=repr(error),identity=identity))+'\n')
            except OSError:pass
        state.update(phase='waiting_for_level',game_identity=identity,reservation=reserved);atomic_json(index,state)
        def prepare_level(check_live=None):
            def check_startup():
                if check_live:check_live()
                if manifest.get('graphics_startup_lock',False):backend.require_prime_active(identity)
            check_startup()
            if auto_continue:
                sys.path.insert(0,str(HERE))
                from menu_navigation import NavigationCancelled
                state.update(phase='navigating_menu');atomic_json(index,state)
                try:
                    navigation_deadline=clock()+60 if owned_prime else started+120
                    navigation_guards={'check_live':check_startup} if check_live or manifest.get('graphics_startup_lock',False) else {}
                    state['menu_navigation']=backend.navigate(pid,identity,cancel,navigation_deadline,index.with_suffix('.menu.jsonl'),**navigation_guards)
                    if not state['menu_navigation'].get('loading_requested'):
                        raise RuntimeError('Menu navigation did not request level loading')
                except NavigationCancelled:return None
                state['phase']='waiting_for_level';atomic_json(index,state)
            previous=None;last_error='No possessed player observed'
            # Legacy reservations retain the 120/180-second boundary. Owned prime
            # is tied to this launcher process, so a player may stay in menus.
            while owned_prime or clock()-started<120:
                check_startup()
                if backend.cancelled(cancel):return None
                if backend.control.identity(pid)!=identity:raise RuntimeError('Reserved game exited or changed')
                try:current=backend.player(pid,profile)
                except (RuntimeError,OSError,ValueError) as error:current=None;last_error=str(error)
                if not owned_prime and clock()-started>=120:
                    raise TimeoutError('Player discovery exceeded the conservative prime deadline; no activation or retry; wait for prime watchdog')
                if stable_player(current,previous):
                    state.pop('waiting_for_level_reason',None)
                    print(json.dumps({'level_player_ready':True}),flush=True)
                    break
                reason=last_error if current is None else 'Waiting for the same guarded player/camera with an advancing timestamp'
                if state.get('waiting_for_level_reason')!=reason:
                    state['waiting_for_level_reason']=reason;atomic_json(index,state)
                    print(json.dumps({'waiting_for_level':reason}),flush=True)
                previous=current;sleep(.5)
            else:raise TimeoutError('No stable possessed level player before prime deadline; no activation/retry; existing prime watchdog must restore: '+last_error)
            state['level_player']=current;atomic_json(index,state);return current
        if not owned_prime and prepare_level() is None:
            state.update(phase='cancelled_before_activation',native_activated=False,prime_watchdog_pending=True)
            atomic_json(index,state);return 0
        args=SimpleNamespace(pid=pid,build_dir=rooted(manifest['native_build']),dll_name=manifest['native_dll'],
            xr_build_dir=rooted(manifest['xr_build']),stereo_units_per_metre=scale,startup_timeout=45.,cleanup_timeout=35.,
            native_expected_sha256=manifest['native_sha256'])
        args.diagnostics_artifacts={key:manifest[key] for key in ('native_sha256','xr_sha256','game_sha256')}
        args.diagnostics_artifacts['native_build']=pathlib.Path(manifest['native_build']).name
        handshake=reserved.get('startup_selftest_handshake')
        if handshake:
            args.startup_selftest_proof=handshake['path'];args.startup_selftest_digest=handshake['digest'];args.startup_selftest_nonce=handshake['nonce']
        for key in SESSION_OPTIONS:setattr(args,key,manifest.get(key,False))
        hud_fov=hud_horizontal_fov(config,manifest)
        if hud_fov is not None:args.hud_horizontal_fov_degrees=hud_fov
        for key,value in hud_layout(config,manifest).items():setattr(args,key,value)
        args.camera_closer_metres=camera_closer(config,manifest)
        args.render_scale=selected_render_scale
        if owned_prime:
            deadline=clock()+30
            last_window_diagnostic={}
            while clock()<deadline:
                if backend.cancelled(cancel):
                    state.update(phase='cancelled_before_activation',native_activated=False)
                    atomic_json(index,state);return 0
                if backend.control.identity(pid)!=identity:raise RuntimeError('Reserved game exited or changed')
                hwnd=backend.game_window(pid,diagnostic=last_window_diagnostic)
                if hwnd:args.startup_menu_window=hwnd;break
                sleep(.1)
            else:
                state['startup_window_last_diagnostic']=last_window_diagnostic
                raise TimeoutError('Game window unavailable for startup VR panel')
        def on_report(path,reported_owner,reported_game):
            report=pathlib.Path(path).resolve()
            if report.parent!=(ROOT/'analysis/runtime').resolve() or reported_owner!=owner or reported_game!=identity:
                raise RuntimeError('Runner report identity mismatch')
            original=json.loads(report.read_text())
            state.pop('native_activated',None) # The owned runner report is authoritative after handoff.
            state.update(session_report=str(report),session_id=original['session_id'],phase='session_owned')
            atomic_json(index,state)
        extra={}
        if owned_prime:
            def before_activation(check_live):
                if prepare_level(check_live) is None:raise backend.runner.StartupCancelled()
            extra['before_native_activation']=before_activation
        code=backend.runner.run_session(args,backend.control,on_report=on_report,cancel_requested=lambda:backend.cancelled(cancel),**extra)
        report=json.loads(pathlib.Path(state['session_report']).read_text()) if state.get('session_report') else {}
        cancelled=report.get('phase')=='cancelled' and report.get('startup_cancelled') is True
        passed=code==0 and report.get('passed') is True
        state.update(phase='cancelled_before_activation' if cancelled else 'finished' if passed else 'incomplete',
            passed=passed,control_exit_code=code,native_activated=report.get('native_started',False))
        if cancelled:state['prime_watchdog_pending']=not owned_prime
        atomic_json(index,state)
        return code
    except Exception as error:
        state.update(phase='incomplete',error=repr(error),no_automatic_retry=True,
            recovery=('Owned prime is cancelled during cleanup; inspect prime_cleanup before normal game exit/retry.' if owned_prime else
                      'Game remains running. Before activation, wait for the existing 180-second prime watchdog; exit the game normally before retry.'))
        cleanup=getattr(error,'reservation_failure_cleanup',None)
        if cleanup is not None:
            state['reservation_failure_cleanup']=cleanup
            state['recovery']=('Exact uninjected startup game exited normally; no retry was attempted.' if cleanup.get('status')=='exited' else
                'Reservation failed; inspect reservation_failure_cleanup. No forced process termination or injection retry was attempted.')
        atomic_json(index,state);raise
    finally:
        if test_audio_guard:
            try:state[audio_state_key]=test_audio_guard.close()
            except Exception as error:state[audio_state_key]={'enabled':True,'cleanup_failed':True,'error':repr(error)}
            # Audio restoration failure must not obstruct existing native/prime cleanup.
            try:atomic_json(index,state)
            except Exception:pass
        if tiny_output_monitor:
            try:
                joined=tiny_output_monitor.close()
                state['tiny_output_monitor_cleanup']={'joined':joined if type(joined) is bool else None}
            except Exception as error:
                state['tiny_output_monitor_cleanup']={'joined':False,'error':repr(error)}
            # Best-effort diagnostics must not prevent native/prime cleanup.
            try:atomic_json(index,state)
            except Exception:pass
        try:
            if owned_prime and identity:
                try:
                    state['prime_cleanup']=backend.finish_owned_prime(manifest,identity,owner)
                    state['prime_watchdog_pending']=False
                except Exception as error:
                    state.update(prime_cleanup_error=repr(error),prime_watchdog_pending=True,passed=False)
                    atomic_json(index,state)
                    raise
                atomic_json(index,state)
        finally:
            if cancel:backend.control.k.CloseHandle(cancel)
            backend.control.k.CloseHandle(lock)

def stop(backend,index=INDEX):
    state=json.loads(index.read_text())
    if state.get('schema')!=1 or state.get('kind')!='repository_vr_launcher':
        raise RuntimeError('No indexed continuous session; inspect launcher status, do not guess a PID')
    if not state.get('session_report'):
        if state.get('phase') not in ('preflight','reserving','navigating_menu','waiting_for_level'):
            raise RuntimeError('No active startup or indexed continuous session')
        backend.runner.checked_identity(backend.control,state['owner_identity'])
        if not backend.control.signal(cancel_name(state['owner_identity']['pid'])):raise RuntimeError('Startup owner stop event absent')
        print('Startup cancellation requested; no game is terminated; priming restores through its watchdog.')
        return 0
    report=pathlib.Path(state['session_report']).resolve()
    original=json.loads(report.read_text())
    if original.get('session_id')!=state.get('session_id') or original.get('owner_identity')!=state.get('owner_identity'):
        raise RuntimeError('Indexed owner session identity mismatch')
    if original.get('phase') in ('stopped','incomplete','cancelled'):
        return backend.runner.request_stop(report,95.,backend.control)
    # Cover XR preflight before the native stop event exists; runner consumes this callback.
    backend.runner.checked_identity(backend.control,state['owner_identity'])
    if not backend.control.signal(cancel_name(state['owner_identity']['pid'])):raise RuntimeError('Launcher cancellation event absent')
    if original.get('phase') in ('starting','awaiting_level'):
        print('Session startup cancellation requested; owner will perform cleanup without activating new native work.')
        return 0
    return backend.runner.request_stop(report,95.,backend.control)

def indexed_session(index):
    state=json.loads(index.read_text())
    if state.get('schema')!=1 or state.get('kind')!='repository_vr_launcher' or not state.get('session_report'):
        raise RuntimeError('No indexed continuous session')
    path=pathlib.Path(state['session_report']).resolve();session=json.loads(path.read_text())
    if (session.get('schema')!=1 or session.get('kind')!='continuous_strict_stereo' or
        session.get('session_id')!=state.get('session_id') or session.get('owner_identity')!=state.get('owner_identity') or
        session.get('game_identity')!=state.get('game_identity')):
        raise RuntimeError('Indexed session owner/game association mismatch')
    return state,path,session

def current_native_status(backend,session):
    command=session.get('native_command',[])
    try:build=pathlib.Path(command[command.index('--build-dir')+1]).resolve()
    except (ValueError,IndexError):raise RuntimeError('Indexed native command has no build directory')
    diagnostic={}
    fields=backend.runner.read_session_status(build/'session-status.txt',session['game_identity']['pid'],0,
                                              diagnostic=diagnostic)
    reason=None
    if not fields:reason='reader_rejected'
    elif not backend.runner.native_owner_matches(fields,session['owner_identity']):reason='owner_mismatch'
    elif fields.get('started_qpc')!=str(session.get('native_started_qpc')):reason='started_qpc_mismatch'
    if reason is not None:
        detail=dict(reason=reason,reader=diagnostic or {'reason':'unknown'})
        if fields:
            detail.update(owner_fields_present=all(k in fields for k in ('owner_pid','owner_creation_time')),
                          observed_started_qpc_present='started_qpc' in fields,
                          expected_started_qpc_present=session.get('native_started_qpc') is not None)
        error=RuntimeError('Current native status belongs to a different or unverified session: '+
                           json.dumps(detail,sort_keys=True,separators=(',',':')))
        error.native_status_diagnostic=detail
        raise error
    return fields

def recenter(backend,index=INDEX):
    state,path,session=indexed_session(index)
    if session.get('phase')!='running':raise RuntimeError('Recenter requires the indexed session to be running')
    # Keep the same owner process and game creation identity, not merely a PID.
    for identity in ('owner_identity','game_identity','xr_identity'):
        backend.runner.checked_identity(backend.control,session[identity])
    current_native_status(backend,session)
    result=backend.runner.request_recenter(path,backend.control)
    print(json.dumps({'recenter':'requested','applied_verified':False}),flush=True)
    return result

def status(backend,index=INDEX):
    state=json.loads(index.read_text());result={'launcher':state,'reference_generation':None,'native_status_matched':False}
    if state.get('session_report'):
        try:
            _,_,session=indexed_session(index)
            result['session_phase']=session.get('phase')
            fields=current_native_status(backend,session)
            result.update(native_status_matched=True,native_phase=fields.get('phase'),
                reference_generation=int(fields['reference_generation']) if 'reference_generation' in fields else None,
                recenter_requests=int(fields['recenter_requests']) if 'recenter_requests' in fields else None)
        except (RuntimeError,OSError,ValueError,KeyError) as error:result['status_error']=str(error)
    print(json.dumps(result,indent=2));return result

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('command',choices=('start','stop','status','recenter','export-logs','render-size'))
    parser.add_argument('--config',type=pathlib.Path,default=HERE/'config.json')
    parser.add_argument('--manifest',type=pathlib.Path,default=HERE/'manifest.json')
    parser.add_argument('--runtime',type=pathlib.Path,help='Child-process OpenXR manifest override; no registry change')
    parser.add_argument('--auto-continue',action='store_true',help='Navigate the verified menu path into the existing campaign')
    parser.add_argument('--foreground',action='store_true',help='Use the ordinary foreground game startup policy')
    parser.add_argument('--output',type=pathlib.Path,help='Destination ZIP for export-logs')
    parser.add_argument('--overwrite',action='store_true',help='Explicitly replace an existing diagnostics ZIP')
    args=parser.parse_args()
    if args.command=='export-logs' and args.output is None:
        parser.error('export-logs requires --output')
    try:
        if args.command=='export-logs':
            # -I omits the script directory; use only our bundled sibling module.
            sys.path.insert(0,str(HERE))
            from diagnostics_bundle import export_bundle
            receipt=export_bundle(ROOT,args.output,overwrite=args.overwrite)
            print(json.dumps(receipt,ensure_ascii=False),flush=True)
            return 0
        backend=Backend()
        if args.command=='status':status(backend);return 0
        if args.command=='stop':return stop(backend)
        if args.command=='recenter':return recenter(backend)
        if args.runtime:
            runtime=args.runtime.resolve()
            if not runtime.is_file():raise RuntimeError('OpenXR runtime manifest does not exist')
            os.environ['XR_RUNTIME_JSON']=str(runtime)
        if args.command=='render-size':
            config=json.loads(args.config.read_text());manifest=json.loads(args.manifest.read_text())
            value=render_scale(config,manifest)
            if value is None:raise ValueError('This build does not support runtime render size')
            print(json.dumps(render_size_info(manifest,value)),flush=True);return 0
        return start(json.loads(args.config.read_text()),json.loads(args.manifest.read_text()),backend,auto_continue=args.auto_continue,foreground=args.foreground)
    except Exception as error:print(str(error),file=sys.stderr);return 1

if __name__=='__main__':raise SystemExit(main())
