"""Fresh startup: reserve camera/family trampolines without enabling native hooks.

Optional --prime-targets observes D3D12 descriptors and, in frame-link builds,
DXGI swapchain/queue bindings for at most 180 seconds before capture handoff.

After level loading, start_capture.py --use-reserved re-resolves the camera and
performs the ordinary signature/PID/lifecycle checks before actual activation.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import pathlib
import subprocess
import time
import sys
from prime_owner import owner_argument, pending_prime, graphics_prime_count
from start_capture import k, modules, remote_call, pefile, ROOT, run_hidden, preparation_live_guard
from runtime_probe import Process
sys.path.insert(0,str(ROOT/'tools/debug-launch'))
from game_install import validate_game, debug_command
import instance_policy
from run_stereo_continuous import WindowsControl
k.SetEvent.argtypes=[w.HANDLE];k.SetEvent.restype=w.BOOL


def mark_launch_stage(path,identity,stage):
    if path is None:return
    receipt=json.loads(path.read_text(encoding='utf-8'))
    if (receipt.get('kind')!='reservation_birth' or receipt.get('schema')!=1 or
            receipt.get('game_identity')!=identity or stage not in ('injection_attempted','prime_attempted')):
        raise RuntimeError('Reservation birth receipt identity differs')
    receipt[stage]=True
    temporary=path.with_name(path.name+'.tmp')
    temporary.write_text(json.dumps(receipt),encoding='utf-8');temporary.replace(path)


def wait_loader_dependencies(exe,born,deadline,*,owner_pid=None,owner_creation_time=None,
                             clock=time.monotonic,sleep=time.sleep):
    """Read-only polling of one fresh identity; no loader/injection retries."""
    control=WindowsControl();identity=None;handle=None;check=None;next_discovery=0
    required={'kernelbase.dll','gowde-steam.exe','d3d12.dll','dxgi.dll'}
    missing=required;attempts=0;last_loaded_names=[]
    if born is not None:
        if (born.get('schema')!=1 or born.get('kind')!='reservation_birth' or
                born.get('injection_attempted') is not False or born.get('prime_attempted') is not False):
            raise RuntimeError('Fresh birth receipt state rejected')
        identity=born.get('game_identity')
        if (not isinstance(identity,dict) or type(identity.get('pid')) is not int or identity['pid']<=0 or
                type(identity.get('creation_time')) is not int or identity['creation_time']<=0 or
                not isinstance(identity.get('image'),str) or pathlib.Path(identity['image']).resolve()!=exe.resolve()):
            raise RuntimeError('Fresh birth receipt identity rejected')
    try:
        while clock()<deadline:
            if owner_pid is not None:
                owner=control.identity(owner_pid)
                if not owner or owner['creation_time']!=owner_creation_time:
                    raise RuntimeError('Owner exited or changed during loader preparation')
                c.set_last_error(0)
                event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.RepositoryLauncher.Stop.{owner_pid}')
                if not event and c.get_last_error() not in (0,2):raise c.WinError(c.get_last_error())
                if event:
                    try:
                        status=k.WaitForSingleObject(event,0)
                        if status==0:raise RuntimeError('Owner cancelled loader preparation')
                        if status!=0x102:raise RuntimeError('Owner cancellation event wait failed')
                    finally:k.CloseHandle(event)
            if clock()>=deadline:break
            if identity is None:
                # Compatibility for direct CLI callers without a birth receipt.
                # Once selected, never enumerate or accept a replacement PID.
                if clock()<next_discovery:sleep(min(.1,max(0,deadline-clock())));continue
                next_discovery=clock()+1
                if clock()>=deadline:break
                result=run_hidden(['powershell','-NoProfile','-NonInteractive','-Command',
                    "Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id"],
                    capture_output=True,text=True,timeout=max(.001,min(2,deadline-clock())))
                pids=[int(x) for x in result.stdout.splitlines() if x.strip().isdigit()]
                if len(pids)>1:raise RuntimeError('Ambiguous fresh process')
                if not pids:continue
                identity=control.identity(pids[0])
                if not identity:raise RuntimeError('Fresh process exited during discovery')
                if pathlib.Path(identity['image']).resolve()!=exe.resolve():raise RuntimeError('Fresh executable path differs')
            if control.identity(identity['pid'])!=identity:raise RuntimeError('Fresh process differs from born identity')
            if check is None:check=preparation_live_guard(identity['pid'],owner_pid,owner_creation_time)
            check()
            if clock()>=deadline:break
            if handle is None:
                handle=k.OpenProcess(0x43a,False,identity['pid'])
                if not handle:raise c.WinError(c.get_last_error())
            check()
            if clock()>=deadline:break
            attempts+=1;loaded=modules(handle)
            check()
            if control.identity(identity['pid'])!=identity:raise RuntimeError('Fresh process differs from born identity')
            last_loaded_names=sorted(loaded)
            missing=required-set(loaded)
            if not missing and clock()<deadline:
                if loaded['gowde-steam.exe'][1].resolve()!=exe.resolve():raise RuntimeError('Fresh executable path differs')
                result=(identity['pid'],handle,loaded,identity,check)
                handle=None # Transfer the one owned handle to the caller.
                return result
            sleep(min(.1,max(0,deadline-clock())))
        print(json.dumps({'reservation_loader_wait_failed':True,'missing_modules':sorted(missing),
                          'module_polls':attempts,'game_identity':identity,
                          'last_loaded_modules':last_loaded_names[:256],
                          'loaded_module_count':len(last_loaded_names),
                          'loaded_modules_truncated':len(last_loaded_names)>256}),flush=True)
        raise RuntimeError('Fresh loader dependencies not ready before bounded deadline')
    finally:
        if handle:k.CloseHandle(handle)


def main():
    command_started=time.monotonic()
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=pathlib.Path,default=ROOT/'tools/native-capture/build-prepare')
    parser.add_argument('--game-path',type=pathlib.Path,help='Supported installation EXE; defaults to repository installation')
    parser.add_argument('--foreground',action='store_true',help='Use the ordinary foreground game startup policy')
    parser.add_argument('--experimental-preserve-upscaler',action='store_true',help='Read-only preflight and preserve the fixed experimental DLSS SR startup profile')
    parser.add_argument('--dll-name',default='GearsCapturePrepare.dll')
    parser.add_argument('--launch-receipt',type=pathlib.Path,help=argparse.SUPPRESS)
    parser.add_argument('--startup-preflight-nonce',help=argparse.SUPPRESS)
    parser.add_argument('--check-shadow-copy',action='store_true',help='Require the bounded source-view copy self-test before startup; does not enable the experiment')
    parser.add_argument('--check-shadow-state',action='store_true',help='Require the independent state lifecycle self-test before startup')
    parser.add_argument('--prime-targets',action='store_true',help='Observe RTV creation/copies during loading, bounded to 180 seconds or capture handoff')
    parser.add_argument('--prime-owner-pid',type=int)
    parser.add_argument('--prime-owner-creation-time',type=int)
    parser.add_argument('--graphics-startup-lock',action='store_true',help='Require three certified graphics settings hooks during owned priming')
    parser.add_argument('--graphics-menu-lock',action='store_true',help='Also require the certified AA selection hook')
    parser.add_argument('--check-mask-control',action='store_true',help='Self-test and reserve the optional first-view mask trampoline without enabling it; requires a DLL with that export')
    parser.add_argument('--check-frame-link',action='store_true',help='Require frame-association self-tests and startup target/binding priming')
    args=parser.parse_args()
    if args.experimental_preserve_upscaler and (args.graphics_startup_lock or args.graphics_menu_lock):
        parser.error('Experimental preserve upscaler conflicts with graphics locks')
    owned=args.prime_owner_pid is not None or args.prime_owner_creation_time is not None
    if owned and (not args.prime_targets or args.prime_owner_pid is None or
                  not 0<args.prime_owner_pid<=0xffffffff or args.prime_owner_creation_time is None or
                  not 0<args.prime_owner_creation_time<=0xffffffffffffffff):
        parser.error('Owned priming requires --prime-targets and paired valid PID/FILETIME identity')
    if args.check_frame_link and not args.prime_targets:parser.error('Frame association needs --prime-targets')
    if args.graphics_startup_lock and not owned:parser.error('Startup graphics lock requires owned priming')
    if args.graphics_menu_lock and not args.graphics_startup_lock:parser.error('Menu lock requires startup graphics protection')
    if pathlib.Path(args.dll_name).name!=args.dll_name:parser.error('DLL basename required')
    exe=validate_game(args.game_path) # Before self-tests, launch, or startup policy writes.
    profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    build=args.build_dir.resolve();dll=build/args.dll_name
    startup_proof=None
    scheduled=[]
    def schedule(command,**kwargs):scheduled.append((command,kwargs))
    if args.graphics_menu_lock:schedule([str(build/'GraphicsMenuLockTest.exe')],check=True)
    for mode in ([],['--family'],['--clone'],['--renderer'],['--lifetime'],['--prepare']):schedule([str(build/'CaptureSelfTest.exe'),str(dll),*mode],check=True)
    schedule([str(build/'CaptureSelfTest.exe'),str(dll),'--geometry'],check=True)
    if args.check_shadow_copy:schedule([str(build/'SourceViewShadowTest.exe')],check=True)
    if args.check_shadow_state:schedule([str(build/'CaptureSelfTest.exe'),str(dll),'--state'],check=True)
    if args.check_mask_control:schedule([str(build/'CaptureSelfTest.exe'),str(dll),'--first-view-mask'],check=True)
    if args.check_frame_link:
        schedule([str(build/'CaptureSelfTest.exe'),str(dll),'--frame-link'],check=True)
        schedule([str(build/'FrameLinkTest.exe')],check=True)
    if args.prime_targets:
        schedule([str(build/'CaptureSelfTest.exe'),str(dll),'--quad'],check=True)
        schedule([str(build/'GpuViewportStatsTest.exe')],check=True)
        schedule([str(build/'CaptureSelfTest.exe'),str(dll),'--viewport'],check=True)
        schedule([str(build/'CaptureSelfTest.exe'),str(dll),'--prime'],check=True)
        schedule([str(build/'CaptureSelfTest.exe'),str(dll),'--prime-handoff'],check=True)
        if owned:
            for mode in ('--prime-owned-cancel','--prime-owned-exit','--prime-owned-handoff'):
                schedule([str(build/'CaptureSelfTest.exe'),str(dll),mode],check=True)
    if args.startup_preflight_nonce is not None:
        if not(owned and args.check_shadow_state and args.check_mask_control and args.check_frame_link):
            raise RuntimeError('Owned complete reservation required for startup self-test proof')
        if len(args.startup_preflight_nonce)!=32 or any(ch not in '0123456789abcdef' for ch in args.startup_preflight_nonce):
            raise RuntimeError('Invalid startup self-test nonce')
        def check_preflight_owner():
            current=WindowsControl().identity(args.prime_owner_pid)
            if not current or current['creation_time']!=args.prime_owner_creation_time:
                raise RuntimeError('Startup self-test owner exited or changed')
        check_preflight_owner()
        from startup_selftests import continuous_suite,run_suite,make_proof
        suite=continuous_suite();started=time.perf_counter_ns()
        hashes=run_suite(dll,suite,run_hidden)
        completed={(row[0],tuple(row[1])) for row in suite}
        for command,options in scheduled:
            key=(pathlib.Path(command[0]).name,tuple('@dll' if value==str(dll) else value for value in command[1:]))
            if key not in completed:run_hidden(command,**options)
        check_preflight_owner()
        startup_proof=make_proof(dll,suite,hashes,dict(pid=args.prime_owner_pid,creation_time=args.prime_owner_creation_time),args.startup_preflight_nonce)
        print(json.dumps({'reservation_selftests_prelaunch_ms':(time.perf_counter_ns()-started)/1e6,'capture_suite_tests':len(suite)}),flush=True)
    else:
        for command,options in scheduled:run_hidden(command,**options)
    isolated=instance_policy.context()
    if isolated and pathlib.Path(args.build_dir).resolve()!=(isolated/'native').resolve():
        raise RuntimeError('Concurrent reservation requires private native status/build files')
    if not isolated and instance_policy.shared_game_pids(instance_policy.running_game_pids()):
        raise RuntimeError('Existing game must exit normally first')
    pe=pefile.PE(str(dll));export=next(x for x in pe.DIRECTORY_ENTRY_EXPORT.symbols if x.name==b'GearsCaptureReserve')
    frame_link_capable=any(x.name==b'GearsCaptureFrameLinkCheck' for x in pe.DIRECTORY_ENTRY_EXPORT.symbols)
    if export.forwarder:raise RuntimeError('Forwarded reserve export rejected')
    rva=export.address;prime_rva=None;mask_rva=None
    if args.check_mask_control:
        mask_export=next(x for x in pe.DIRECTORY_ENTRY_EXPORT.symbols if x.name==b'GearsCaptureReserveFirstViewMask')
        if mask_export.forwarder:raise RuntimeError('Forwarded mask reserve export rejected')
        mask_rva=mask_export.address
    if args.prime_targets:
        prime_name=b'GearsCaptureTargetPrimeOwned' if owned else b'GearsCaptureTargetPrime'
        prime_export=next(x for x in pe.DIRECTORY_ENTRY_EXPORT.symbols if x.name==prime_name)
        if prime_export.forwarder:raise RuntimeError('Forwarded prime export rejected')
        prime_rva=prime_export.address
    pe.close()
    launch_command=debug_command(exe,foreground=args.foreground,experimental_preserve_upscaler=args.experimental_preserve_upscaler)
    if args.launch_receipt is not None:launch_command+=['-LaunchReceipt',str(args.launch_receipt)]
    run_hidden(launch_command,check=True)
    born=None
    if args.launch_receipt is not None:
        born=json.loads(args.launch_receipt.read_text(encoding='utf-8'))
        print(json.dumps({'reservation_birth':born}),flush=True)
    # Four possible remote calls retain their existing15s limits. Keep70s of
    # the owner's120s subprocess bound for those calls and final verification.
    deadline=min(time.monotonic()+25,command_started+50)
    pid,handle,loaded,game_identity,check_live=wait_loader_dependencies(exe,born,deadline,
        owner_pid=args.prime_owner_pid,owner_creation_time=args.prime_owner_creation_time,
        clock=time.monotonic,sleep=time.sleep)
    memory=None;finished=False;prime_attempted=False;reported=False
    try:
        if loaded['gowde-steam.exe'][1].resolve()!=exe.resolve():raise RuntimeError('Fresh executable path differs')
        control=WindowsControl()
        if control.identity(pid)!=game_identity or pathlib.Path(game_identity['image']).resolve()!=exe:
            raise RuntimeError('Fresh process identity differs from selected installation')
        if startup_proof is not None:
            from startup_selftests import artifacts
            if artifacts(dll,startup_proof['suite'])!=startup_proof['artifacts']:
                raise RuntimeError('Prelaunch self-test artifacts changed before reservation')
        validate_game(exe) # Recheck selected bytes before any injection.
        check_live()
        if time.monotonic()>=deadline:raise RuntimeError('Reservation readiness deadline before injection')
        if dll.name.lower() in loaded:raise RuntimeError('Reservation DLL already loaded')
        loader=k.GetProcAddress(k.GetModuleHandleW('kernel32.dll'),b'LoadLibraryW');owner=w.HMODULE()
        if not k.GetModuleHandleExW(6,loader,c.byref(owner)):raise c.WinError(c.get_last_error())
        path=c.create_unicode_buffer(32768);k.GetModuleFileNameW(owner,path,len(path))
        remote=loaded[pathlib.Path(path.value).name.lower()][0]+loader-owner.value
        # Persist before any capture allocation/write/remote execution. A caller
        # must treat an interrupted injection as unknown, never as safe to retry.
        check_live()
        if time.monotonic()>=deadline:raise RuntimeError('Reservation readiness deadline before injection')
        mark_launch_stage(args.launch_receipt,game_identity,'injection_attempted')
        check_live()
        if time.monotonic()>=deadline:raise RuntimeError('Reservation readiness deadline before injection')
        encoded=(str(dll)+'\0').encode('utf-16-le');memory=k.VirtualAllocEx(handle,None,len(encoded),0x3000,0x04)
        if not memory:raise c.WinError(c.get_last_error())
        count=c.c_size_t();buffer=c.create_string_buffer(encoded)
        if not k.WriteProcessMemory(handle,memory,buffer,len(encoded),c.byref(count)) or count.value!=len(encoded):raise c.WinError(c.get_last_error())
        remote_call(handle,remote,memory);finished=True;loaded=modules(handle)
        resident=loaded.get(dll.name.lower())
        if not resident or resident[1].resolve()!=dll:raise RuntimeError('Reserve DLL load not confirmed; do not retry')
        code=remote_call(handle,resident[0]+rva,None)
        if code:raise RuntimeError(f'Reservation failed ({code}); no activation requested')
        event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.Reserved.{pid}')
        if not event:raise RuntimeError('Reservation event absent')
        k.CloseHandle(event)
        if args.check_mask_control:
            code=remote_call(handle,resident[0]+mask_rva,None)
            if code:raise RuntimeError(f'Mask reservation failed ({code}); no activation requested')
            event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.FirstViewMaskReserved.{pid}')
            if not event:raise RuntimeError('Mask reservation event absent')
            k.CloseHandle(event)
        signatures={0x15cf08:bytes.fromhex('488bc45356574154415541564157'),0x15c968:bytes.fromhex('488bc4488958084889681048897018'),0x3a148c:bytes.fromhex('48895c2408488974241855'),0x193d50:bytes.fromhex('488bc4488958104889681848897020')}
        signatures[0x116250]=bytes.fromhex('488bc44889581848897020574154')
        signatures[0x15a80c]=bytes.fromhex('48895c241055488bec4883ec70')
        signatures[0x15a974]=bytes.fromhex('40534883ec20488bd9')
        signatures[0x1fc060]=bytes.fromhex('488bc44889581048897018574154')
        signatures[0x13cfcc]=bytes.fromhex('44894c242048894c240855535657')
        signatures[0x1af42c]=bytes.fromhex('488bc4488958084889701048897818')
        if args.check_mask_control:
            signatures[0x2a7e48]=bytes.fromhex('488bc448895808488970104889781855')
            signatures[0x119bdd]=bytes.fromhex('e866e21800')
        if args.check_shadow_state:signatures[0x33bc70]=bytes.fromhex('48895c2408574883ec208bda488bf9')
        read_only=Process(pid)
        try:
            entries=[{'rva':hex(offset),'matches':read_only.read(loaded['gowde-steam.exe'][0]+offset,len(value))==value} for offset,value in signatures.items()]
        finally:read_only.close()
        if not all(x['matches'] for x in entries):raise RuntimeError('Reservation changed an entry; do not activate')
        graphics_hooks=0
        if args.prime_targets:
            prime_memory=None;prime_finished=False
            try:
                mark_launch_stage(args.launch_receipt,game_identity,'prime_attempted')
                if owned:
                    argument=owner_argument(args.prime_owner_pid,args.prime_owner_creation_time)
                    prime_memory=k.VirtualAllocEx(handle,None,len(argument),0x3000,0x04)
                    if not prime_memory:raise c.WinError(c.get_last_error())
                    written=c.c_size_t();argument_buffer=c.create_string_buffer(argument)
                    if not k.WriteProcessMemory(handle,prime_memory,argument_buffer,len(argument),c.byref(written)) or written.value!=len(argument):
                        raise c.WinError(c.get_last_error())
                prime_attempted=True
                code=remote_call(handle,resident[0]+prime_rva,prime_memory)
                prime_finished=True
            finally:
                if prime_memory and prime_finished:k.VirtualFreeEx(handle,prime_memory,0,0x8000)
            if code:raise RuntimeError(f'Target priming failed ({code}); do not activate')
            prime_event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.TargetPrime.{pid}')
            if not prime_event:raise RuntimeError('Target priming readiness absent')
            try:
                if k.WaitForSingleObject(prime_event,0)!=0x102:raise RuntimeError('Target priming already closed')
                prime_status=dll.with_name('target-prime-status.txt').read_text()
                if not pending_prime(prime_status,pid,args.prime_owner_pid,args.prime_owner_creation_time,require_owned=owned):
                    raise RuntimeError('Target priming identity/readiness not confirmed')
                graphics_hooks=graphics_prime_count(prime_status,args.graphics_startup_lock,args.graphics_menu_lock)
            finally:k.CloseHandle(prime_event)
        if control.identity(pid)!=game_identity:raise RuntimeError('Reserved process identity changed')
        report={'game_identity':game_identity,'game_sha256':profile['sha256'],'pid':pid,'dll':str(dll),'trampolines_reserved':True,'hooks_enabled':args.prime_targets,
                'first_view_mask_reserved':args.check_mask_control,
                'native_hooks_enabled':bool(graphics_hooks),'graphics_hooks_enabled':graphics_hooks,'scene_hooks_enabled':False,
                'descriptor_hooks_enabled':3 if args.prime_targets else 0,'independent_entry_checks':entries}
        report['factory_hooks_enabled']=3 if args.prime_targets and frame_link_capable else 0
        if startup_proof is not None:
            startup_proof['game_identity']=game_identity
            report['startup_selftest_proof']=startup_proof
        if args.prime_targets:
            report['prime_lifetime']='owner' if owned else '180_seconds'
            if owned:report.update(prime_owner_pid=args.prime_owner_pid,prime_owner_creation_time=args.prime_owner_creation_time)
        folder=ROOT/'analysis/runtime';folder.mkdir(parents=True,exist_ok=True)
        (folder/f'reserve-{pid}.json').write_text(json.dumps(report,indent=2))
        print(json.dumps(report))
        reported=True
    finally:
        if owned and prime_attempted and not reported:
            cancel=k.OpenEventW(0x2,False,f'Local\\GearsVR.Capture.TargetPrime.{pid}')
            if cancel:
                try:k.SetEvent(cancel)
                finally:k.CloseHandle(cancel)
        if memory and finished:k.VirtualFreeEx(handle,memory,0,0x8000)
        k.CloseHandle(handle)


if __name__=='__main__':main()
