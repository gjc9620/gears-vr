"""Prepare/start/stop an independently owned existing Source1 simulator instance.

Uses the established launcher, game profile and production features. Never stages
or restores the shared solo instance. Each caller owns its resulting session.
"""
from pathlib import Path
import argparse,ctypes,hashlib,importlib.util,json,os,re,subprocess,sys
sys.path.insert(0,'J:\\codexWorktree\\gearsVr\\tools\\multi-instance')
from runtime_receipts import read_json,GameReceipt,wait_launcher,require_disk,ResourceUnavailable,first_admission,confirmed_prelaunch_rejection

ROOT=Path('J:\\codexWorktree\\gearsVr')
BASE=ROOT/'analysis/solo-m1-20261028'
QUIET=ROOT/'tools/quiet-startup/build-instance-cloak1110'
DEFAULT_SEED='multi-1110-1794304230-a'
spec=importlib.util.spec_from_file_location('gears_multi_prepare',Path('J:\\codexWorktree\\gearsVr\\tools\\multi-instance\\start.py'))
original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)

def write(path,value):path.write_text(json.dumps(value,indent=2),encoding='utf-8')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def owner_resumed(selected,pause):
    """Exact-instance owner resume; preserve the global stop for other callers."""
    receipt=selected/'owner-runtime-resume.json'
    if not receipt.is_file():return False
    try:
        value=json.loads(receipt.read_text(encoding='utf-8'))
        if not isinstance(value,dict):return False
        return (value.get('owner_explicit_resume') is True and value.get('scope')=='longshot-controlled-validation' and
            value.get('instance')==selected.name and selected.name.startswith('longshot-') and
            value.get('global_pause_sha256')==sha(pause) and
            value.get('manifest_sha256')==sha(selected/'manifest.json') and
            value.get('config_sha256')==sha(selected/'config.json'))
    except (OSError,ValueError,TypeError):return False
def instance(name):
    if not re.fullmatch(r'[a-z0-9_-]{1,40}',name):raise ValueError('Use an exact unique 1..40 character instance name')
    return ROOT/'analysis/instances'/name

def prepare(name,*,seed=DEFAULT_SEED,manifest=None,config=None):
    require_disk(ROOT)
    seed_root=instance(seed)
    selected,proof=original.prepare(name,Path(manifest) if manifest else seed_root/'manifest.json',
                                    Path(config) if config else seed_root/'config.json',background_test=True)
    sys.path.insert(0,str(ROOT/'tools/debug-launch'))
    from background_launch import verify_test_behind_build
    verify_test_behind_build(QUIET)
    proof['quiet_build']=str(QUIET)
    proof['isolation_revision']='guardian-source-build-20261110'
    for sub in ('Config','SaveData'):
        for p in (seed_root/'LocalAppData/Coalition/WarGame'/sub).iterdir():
            if p.is_file() and (p.suffix.lower()=='.ini' if sub=='Config' else '.ue3profile' in p.name):
                raw=p.read_bytes()
                if p.read_bytes()!=raw:raise RuntimeError('Seed changed while copying: '+str(p))
                destination=selected/'LocalAppData/Coalition/WarGame'/sub/p.name
                destination.write_bytes(raw)
                proof.setdefault('seed_copies',{})[str(destination.relative_to(selected))]={'source':str(p),'sha256':sha(destination)}
    m=json.loads((selected/'manifest.json').read_text())
    pg=ROOT/m['playground_build']/m['playground_dll']
    if sha(pg)!=m['playground_sha256']:raise RuntimeError('Playground source identity differs')
    pg_build=ROOT/'tools/playground'/('build-'+name)
    original.copy_stable(pg,pg_build/pg.name)
    m['playground_build']=str(pg_build.relative_to(ROOT))
    xr_source=ROOT/m['xr_build'];xr_build=ROOT/'tools/openxr-probe'/('build-'+name)
    if sha(xr_source/'OpenXRPoseProbe.exe')!=m['xr_sha256']:raise RuntimeError('XR source identity differs')
    for p in xr_source.iterdir():
        if p.is_file() and p.suffix.lower() in ('.exe','.dll'):original.copy_stable(p,xr_build/p.name)
    m['xr_build']=str(xr_build.relative_to(ROOT))
    write(selected/'manifest.json',m)
    cfg=json.loads((selected/'config.json').read_text());cfg['render_scale']=.3
    game=Path(proof['game_path'])
    settings=selected/'LocalAppData/Coalition/WarGame/Config/76561197960271872EngineUserSettings.ini'
    if not settings.is_file():raise RuntimeError('Owner-selected game settings missing')
    cfg.update(game_path=str(game),game_settings_paths={str(game):str(settings)})
    write(selected/'config.json',cfg)
    folder=selected/'xr-input/LocalAppData/OpenXR-Simulator';folder.mkdir(parents=True)
    settings=json.loads((seed_root/'xr-input/LocalAppData/OpenXR-Simulator/settings.json').read_text())
    settings.update(render_width=4096,render_height=3288,window_width=960,window_height=540,preview_fps=30)
    write(folder/'settings.json',settings)
    write(folder/'head_pose_command.json',dict(x=0,y=1.7,z=0,yaw=0,pitch=0,roll=0,tracking=1))
    proof['owned_input_directory']=str(folder)
    proof['frozen_playground_sha256']=sha(pg_build/pg.name)
    proof['frozen_xr_sha256']=sha(xr_build/'OpenXRPoseProbe.exe')
    write(selected/'instance.json',proof)
    return selected

def environment(selected):
    proof=json.loads((selected/'instance.json').read_text())
    if selected!=instance(proof['name']) or str(selected)!=proof['root']:raise RuntimeError('Private identity differs')
    slot=json.loads((Path('J:\\codexWorktree\\gearsVr\\tools\\multi-instance\\background-runtime.json')).read_text())['sim_runtime']
    for key in ('manifest','dll'):
        if sha(slot[key])!=slot[key+'_sha256']:raise RuntimeError('Frozen simulator changed')
    env=os.environ.copy()
    env.update(GEARSVR_INSTANCE_ID=proof['name'],GEARSVR_INSTANCE_ROOT=str(selected),
        GEARSVR_INSTANCE_ORIGINAL_LOCALAPPDATA=proof['original_localappdata'],GEARSVR_INSTANCE_GAME_LOGS=proof['game_logs'],
        GEARSVR_QUIET_STARTUP_BUILD=proof['quiet_build'],GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY='1',
        GEARSVR_TEST_WINDOWS_BEHIND='1',GEARSVR_TEST_MUTE_GAME='1',GEARSVR_PYTHON=sys.executable,
        GEARSVR_SIM_DATA_PATH=str(selected/'simdata'),GEARSVR_SIM_REFRESH_HZ='30',
        GEARSVR_QUIET_NATIVE_FOREGROUND='1',
        GEARSVR_TEST_AUDIO_EVIDENCE='1',
        XR_SIMULATOR_NO_ACTIVATE='1',GEARSVR_SOLO_EQUIPMENT_FIXTURE='1',PYTHONIOENCODING='utf-8')
    return env,slot

def require_memory():
    sys.path.insert(0,str(ROOT/'tools/native-capture'))
    from run_stereo_continuous import WindowsControl
    control=WindowsControl();active=[]
    for receipt in (ROOT/'analysis/instances').glob('*/isolated-worker.json'):
        try:
            value=json.loads(receipt.read_text());identity=value.get('worker_identity')
            if identity and control.identity(identity['pid'])==identity:active.append(receipt.parent.name)
        except (OSError,ValueError,KeyError):continue
    if len(active)>=2:
        raise ResourceUnavailable('Two automatic instances already own this 32GiB machine; continue offline work and retry when resources are released: '+', '.join(active))
    class Status(ctypes.Structure):
        _fields_=[('length',ctypes.c_uint32),('load',ctypes.c_uint32)]+[(n,ctypes.c_uint64) for n in
            ('total_physical','available_physical','total_commit','available_commit','total_virtual','available_virtual','extended')]
    value=Status();value.length=ctypes.sizeof(value)
    if not ctypes.WinDLL('kernel32',use_last_error=True).GlobalMemoryStatusEx(ctypes.byref(value)):raise ctypes.WinError(ctypes.get_last_error())
    if value.available_commit<6*1024**3 or value.available_physical<1024**3:
        raise ResourceUnavailable('Insufficient currently available memory for another game; keep existing instances running normally and retry after resources are released')
    return {'available_commit':value.available_commit,'available_physical':value.available_physical,'active_instances':active}

_game_receipts={}

def owned_windows(selected,*,close=False,known=None):
    """Only exact owned game windows; same original startup recovery response."""
    from ctypes import wintypes as w
    sys.path.insert(0,str(ROOT/'tools/native-capture'))
    from run_stereo_continuous import WindowsControl
    control=WindowsControl()
    known=known or _game_receipts.setdefault(str(selected),GameReceipt())
    g=known.observe(selected,control);value=known.index or {}
    if not g:return None
    if control.identity(g['pid'])!=g:return True
    allowed={g['pid']}
    if not close:
        try:
            report=read_json(Path(value['session_report'])) or {}
            xr=report.get('xr_identity')
            if report.get('game_identity')==g and xr and control.identity(xr['pid'])==xr:allowed.add(xr['pid'])
        except (OSError,ValueError,KeyError,UnboundLocalError):pass
    user=ctypes.WinDLL('user32',use_last_error=True)
    user.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
    user.GetWindowTextW.argtypes=[w.HWND,w.LPWSTR,ctypes.c_int]
    user.GetClassNameW.argtypes=[w.HWND,w.LPWSTR,ctypes.c_int]
    user.GetDlgCtrlID.argtypes=[w.HWND];user.GetDlgCtrlID.restype=ctypes.c_int
    user.SendMessageTimeoutW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM,w.UINT,w.UINT,ctypes.POINTER(ctypes.c_size_t)]
    callback=ctypes.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
    @callback
    def top(hwnd,_):
        pid=w.DWORD();user.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
        if pid.value not in allowed or control.identity(g['pid'])!=g:return True
        if close:user.PostMessageW(hwnd,0x10,0,0);return True
        # Native creation protection is primary. Keep only these authenticated
        # own windows cloaked through later recreation/layout changes as well.
        dwm=ctypes.WinDLL('dwmapi',use_last_error=True)
        dwm.DwmSetWindowAttribute.argtypes=[w.HWND,w.DWORD,ctypes.c_void_p,w.DWORD]
        hidden=w.BOOL(True);dwm.DwmSetWindowAttribute(hwnd,13,ctypes.byref(hidden),ctypes.sizeof(hidden))
        cls=ctypes.create_unicode_buffer(128);user.GetClassNameW(hwnd,cls,128)
        if cls.value!='#32770':return True
        rows=[]
        @callback
        def child(h,_):
            text=ctypes.create_unicode_buffer(3000);user.GetWindowTextW(h,text,3000)
            rows.append((h,user.GetDlgCtrlID(h),text.value));return True
        user.EnumChildWindows(hwnd,child,0)
        if any(text=='Last launch did not complete successfully. Revert to default settings?' for _,_,text in rows):
            buttons=[h for h,ident,_ in rows if ident==7]
            if len(buttons)==1 and control.identity(g['pid'])==g:
                result=ctypes.c_size_t();user.SendMessageTimeoutW(buttons[0],0xf5,0,0,2,1000,ctypes.byref(result))
        return True
    user.EnumWindows(top,0)
    return False

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('command',choices=('prepare','start','stop','status','worker'))
    p.add_argument('--name',required=True);p.add_argument('--seed',default=DEFAULT_SEED)
    p.add_argument('--manifest',type=Path);p.add_argument('--config',type=Path)
    p.add_argument('--wait-resources-seconds',type=float,default=60.,help='Bounded first admission only, 0..600 seconds; never retries an admitted worker')
    args=p.parse_args();selected=instance(args.name)
    if args.command=='prepare':
        selected=prepare(args.name,seed=args.seed,manifest=args.manifest,config=args.config)
        print(json.dumps({'root':str(selected),'input_directory':str(selected/'xr-input/LocalAppData/OpenXR-Simulator'),'started':False}));return
    env,slot=environment(selected)
    if args.command=='status':
        print((selected/'launcher-session.json').read_text());return
    cmd=[sys.executable,'-X','utf8','-B',str(Path('J:\\codexWorktree\\gearsVr\\analysis\\instances\\cover-1112-r270-cloak-atom\\cover-output\\cover_body_entry.py')),'launcher','stop' if args.command=='stop' else 'start',
         '--manifest',str(selected/'manifest.json'),'--config',str(selected/'config.json')]
    if args.command=='stop':raise SystemExit(subprocess.call(cmd,cwd=ROOT,env=env))
    pause=Path('J:\\codexWorktree\\gearsVr\\tools\\multi-instance\\automatic-runtime-pause.json')
    if pause.exists() and not owner_resumed(selected,pause):
        raise RuntimeError('Automatic game/XR startup is suspended after repeated foreground interference. Continue offline work; do not bypass this stop or restart until the owner explicitly resumes runtime testing.')
    if Path(json.loads((selected/'instance.json').read_text())['quiet_build']).resolve()!=QUIET.resolve():
        raise RuntimeError('This prepared instance uses the superseded background policy; prepare a fresh name before starting')
    sys.path.insert(0,str(ROOT/'tools/native-capture'))
    from run_stereo_continuous import WindowsControl
    index=selected/'launcher-session.json'
    if index.exists():
        state=json.loads(index.read_text());owner=state.get('owner_identity')
        if owner and WindowsControl().identity(owner['pid'])==owner:raise RuntimeError('This exact private instance already has an owner')
        raise RuntimeError('Use a fresh prepared name for the next run; retain this session evidence')
    cmd+=['--runtime',slot['manifest'],'--auto-continue']
    if args.command=='worker':
        import time
        deadline=time.monotonic()+2
        while not (selected/'isolated-worker.json').exists() and time.monotonic()<deadline:time.sleep(.01)
        admission=read_json(selected/'isolated-worker.json') or {}
        control=WindowsControl();own=control.identity(os.getpid())
        if admission.get('worker_identity')!=own:raise RuntimeError('Own resource reservation identity differs')
        known=GameReceipt();child=None;child_identity=None;failure=None
        try:
            require_disk(ROOT)
            with (selected/'isolated-launcher.log').open('xb') as log:
                child=subprocess.Popen(cmd,cwd=ROOT,env=env,stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
                child_identity=control.identity(child.pid)
                while child.poll() is None:
                    owned_windows(selected,known=known);time.sleep(.25)
        except BaseException as error:
            failure=repr(error)
        finally:
            if child is not None:
                # Always finish Native/entry/audio restoration before asking
                # the game to close. Never kill or abandon an owned launcher.
                wait_launcher(child,control,child_identity)
                from hidden_children import finish
                while True:
                    try:
                        absent=owned_windows(selected,close=True,known=known)
                        no_game=(known.game is None and confirmed_prelaunch_rejection(known.index,child_identity,selected))
                        children=finish(selected,control,known.game) if known.game else {'recorded':None,'remaining':None,'unknown':True}
                        if no_game:children={'not_started':True,'recorded':0,'remaining':[]}
                        result={'launcher_exit':child.returncode,'owned_game_absent':absent,'owned_children':children,
                                'forced_termination':False,'worker_error':failure,'no_game_started':no_game}
                        if (no_game or absent is True) and not children.get('remaining') and not children.get('unknown'):
                            write(selected/'isolated-done.json',result);break
                        write(selected/'isolated-cleanup-pending.json',result)
                    except Exception as error:
                        # Concurrent publishers may transiently hold Windows
                        # sharing locks. Retain the worker/resource reservation.
                        failure=repr(error)
                    time.sleep(.25)
        if failure:raise RuntimeError(failure)
        return
    # Only reserve resources/spawn atomically. The running games do not share a
    # simulator lock and never wait for peer-chat approval or offline encoding.
    from runtime_stage_guard import RuntimeGuard,RuntimeBusy
    request=selected/'isolated-start-request.json'
    with request.open('x',encoding='utf-8') as stream:
        json.dump({'pid':os.getpid(),'wait_seconds':args.wait_resources_seconds,'kind':'first_admission_only'},stream)
    def check_fresh():
        if any((selected/name).exists() for name in ('isolated-worker.json','isolated-worker.log','launcher-session.json','isolated-done.json')):
            raise RuntimeError('Instance already attempted registration; use a fresh prepared name, never retry an admitted/failed worker')
        if pause.exists() and not owner_resumed(selected,pause):
            raise RuntimeError('Automatic runtime paused during admission; no startup allowed')
    def probe():
        disk=require_disk(ROOT)
        memory=require_memory()
        return disk,memory
    def commit(admitted):
        disk,memory=admitted
        with (selected/'isolated-worker.log').open('xb') as log:
            child=subprocess.Popen([sys.executable,'-X','utf8','-B',str(Path(__file__).resolve()),'worker','--name',args.name],cwd=ROOT,env=env,stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
        receipt={'worker_pid':child.pid,'worker_identity':WindowsControl().identity(child.pid),'root':str(selected),'command':cmd,'memory_before_launch':memory,'disk_before_launch':disk,
                 'input_directory':str(selected/'xr-input/LocalAppData/OpenXR-Simulator'),
                 'stop_command':[sys.executable,str(Path(__file__).resolve()),'stop','--name',args.name]}
        write(selected/'isolated-worker.json',receipt)
        return receipt
    receipt=first_admission(lambda:RuntimeGuard('IndependentMemoryAdmission',timeout_ms=1000),
                            check_fresh,probe,commit,busy_errors=(RuntimeBusy,),wait_seconds=args.wait_resources_seconds)
    print(json.dumps(receipt))

if __name__=='__main__':main()
