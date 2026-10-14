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
from prime_owner import owner_argument, pending_prime
from start_capture import k, modules, remote_call, pefile, ROOT
from runtime_probe import Process
sys.path.insert(0,str(ROOT/'tools/debug-launch'))
from game_install import validate_game, debug_command
from run_stereo_continuous import WindowsControl
k.SetEvent.argtypes=[w.HANDLE];k.SetEvent.restype=w.BOOL


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=pathlib.Path,default=ROOT/'tools/native-capture/build-prepare')
    parser.add_argument('--game-path',type=pathlib.Path,help='Supported installation EXE; defaults to repository installation')
    parser.add_argument('--dll-name',default='GearsCapturePrepare.dll')
    parser.add_argument('--check-shadow-copy',action='store_true',help='Require the bounded source-view copy self-test before startup; does not enable the experiment')
    parser.add_argument('--check-shadow-state',action='store_true',help='Require the independent state lifecycle self-test before startup')
    parser.add_argument('--prime-targets',action='store_true',help='Observe RTV creation/copies during loading, bounded to 180 seconds or capture handoff')
    parser.add_argument('--prime-owner-pid',type=int)
    parser.add_argument('--prime-owner-creation-time',type=int)
    parser.add_argument('--check-mask-control',action='store_true',help='Self-test and reserve the optional first-view mask trampoline without enabling it; requires a DLL with that export')
    parser.add_argument('--check-frame-link',action='store_true',help='Require frame-association self-tests and startup target/binding priming')
    args=parser.parse_args()
    owned=args.prime_owner_pid is not None or args.prime_owner_creation_time is not None
    if owned and (not args.prime_targets or args.prime_owner_pid is None or
                  not 0<args.prime_owner_pid<=0xffffffff or args.prime_owner_creation_time is None or
                  not 0<args.prime_owner_creation_time<=0xffffffffffffffff):
        parser.error('Owned priming requires --prime-targets and paired valid PID/FILETIME identity')
    if args.check_frame_link and not args.prime_targets:parser.error('Frame association needs --prime-targets')
    if pathlib.Path(args.dll_name).name!=args.dll_name:parser.error('DLL basename required')
    exe=validate_game(args.game_path) # Before self-tests, launch, or startup policy writes.
    profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    build=args.build_dir.resolve();dll=build/args.dll_name
    for mode in ([],['--family'],['--clone'],['--renderer'],['--lifetime'],['--prepare']):subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),*mode],check=True)
    subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--geometry'],check=True)
    if args.check_shadow_copy:subprocess.run([str(build/'SourceViewShadowTest.exe')],check=True)
    if args.check_shadow_state:subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--state'],check=True)
    if args.check_mask_control:subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--first-view-mask'],check=True)
    if args.check_frame_link:
        subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--frame-link'],check=True)
        subprocess.run([str(build/'FrameLinkTest.exe')],check=True)
    if args.prime_targets:
        subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--quad'],check=True)
        subprocess.run([str(build/'GpuViewportStatsTest.exe')],check=True)
        subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--viewport'],check=True)
        subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--prime'],check=True)
        subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--prime-handoff'],check=True)
        if owned:
            for mode in ('--prime-owned-cancel','--prime-owned-exit','--prime-owned-handoff'):
                subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),mode],check=True)
    result=subprocess.run(['powershell','-NoProfile','-NonInteractive','-Command',"if(Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue){exit 1}else{exit 0}"],capture_output=True)
    if result.returncode:raise RuntimeError('Existing game must exit normally first')
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
    subprocess.run(debug_command(exe),check=True)
    handle=None;deadline=time.monotonic()+12
    while time.monotonic()<deadline:
        result=subprocess.run(['powershell','-NoProfile','-NonInteractive','-Command',"Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id"],capture_output=True,text=True)
        pids=[int(x) for x in result.stdout.splitlines() if x.strip().isdigit()]
        if len(pids)>1:raise RuntimeError('Ambiguous fresh process')
        if pids:
            pid=pids[0];handle=k.OpenProcess(0x43a,False,pid)
            if handle:
                loaded=modules(handle)
                if all(x in loaded for x in ('kernelbase.dll','gowde-steam.exe','d3d12.dll','dxgi.dll')):break
                k.CloseHandle(handle);handle=None
        time.sleep(.05)
    if not handle:raise RuntimeError('Fresh loader dependencies not ready')
    memory=None;finished=False;prime_attempted=False;reported=False
    try:
        if loaded['gowde-steam.exe'][1].resolve()!=exe.resolve():raise RuntimeError('Fresh executable path differs')
        control=WindowsControl();game_identity=control.identity(pid)
        if not game_identity or pathlib.Path(game_identity['image']).resolve()!=exe:
            raise RuntimeError('Fresh process identity differs from selected installation')
        validate_game(exe) # Recheck selected bytes before any injection.
        if dll.name.lower() in loaded:raise RuntimeError('Reservation DLL already loaded')
        loader=k.GetProcAddress(k.GetModuleHandleW('kernel32.dll'),b'LoadLibraryW');owner=w.HMODULE()
        if not k.GetModuleHandleExW(6,loader,c.byref(owner)):raise c.WinError(c.get_last_error())
        path=c.create_unicode_buffer(32768);k.GetModuleFileNameW(owner,path,len(path))
        remote=loaded[pathlib.Path(path.value).name.lower()][0]+loader-owner.value
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
        if args.prime_targets:
            prime_memory=None;prime_finished=False
            try:
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
            finally:k.CloseHandle(prime_event)
        if control.identity(pid)!=game_identity:raise RuntimeError('Reserved process identity changed')
        report={'game_identity':game_identity,'game_sha256':profile['sha256'],'pid':pid,'dll':str(dll),'trampolines_reserved':True,'hooks_enabled':args.prime_targets,
                'first_view_mask_reserved':args.check_mask_control,
                'native_hooks_enabled':False,'descriptor_hooks_enabled':3 if args.prime_targets else 0,'independent_entry_checks':entries}
        report['factory_hooks_enabled']=3 if args.prime_targets and frame_link_capable else 0
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
