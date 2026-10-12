"""Fresh startup: reserve camera/family trampolines without enabling native hooks.

Optional --prime-targets observes three D3D12 descriptor methods for at most
180 seconds before ordinary capture takes over; it changes no camera parameters.

After level loading, start_capture.py --use-reserved re-resolves the camera and
performs the ordinary signature/PID/lifecycle checks before actual activation.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
import pathlib
import subprocess
import time
from start_capture import k, modules, remote_call, pefile, ROOT
from runtime_probe import Process


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=pathlib.Path,default=ROOT/'tools/native-capture/build-prepare')
    parser.add_argument('--dll-name',default='GearsCapturePrepare.dll')
    parser.add_argument('--check-shadow-copy',action='store_true',help='Require the bounded source-view copy self-test before startup; does not enable the experiment')
    parser.add_argument('--check-shadow-state',action='store_true',help='Require the independent state lifecycle self-test before startup')
    parser.add_argument('--prime-targets',action='store_true',help='Observe RTV creation/copies during loading, bounded to 180 seconds or capture handoff')
    args=parser.parse_args()
    if pathlib.Path(args.dll_name).name!=args.dll_name:parser.error('DLL basename required')
    build=args.build_dir.resolve();dll=build/args.dll_name
    for mode in ([],['--family'],['--clone'],['--renderer'],['--lifetime'],['--prepare']):subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),*mode],check=True)
    if args.check_shadow_copy:subprocess.run([str(build/'SourceViewShadowTest.exe')],check=True)
    if args.check_shadow_state:subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--state'],check=True)
    if args.prime_targets:
        subprocess.run([str(build/'GpuViewportStatsTest.exe')],check=True)
        subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--viewport'],check=True)
        subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--prime'],check=True)
        subprocess.run([str(build/'CaptureSelfTest.exe'),str(dll),'--prime-handoff'],check=True)
    result=subprocess.run(['powershell','-NoProfile','-NonInteractive','-Command',"if(Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue){exit 1}else{exit 0}"],capture_output=True)
    if result.returncode:raise RuntimeError('Existing game must exit normally first')
    exe=ROOT/'Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe'
    profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    if hashlib.sha256(exe.read_bytes()).hexdigest()!=profile['sha256']:raise RuntimeError('Unvalidated game build')
    pe=pefile.PE(str(dll));export=next(x for x in pe.DIRECTORY_ENTRY_EXPORT.symbols if x.name==b'GearsCaptureReserve')
    if export.forwarder:raise RuntimeError('Forwarded reserve export rejected')
    rva=export.address;prime_rva=None
    if args.prime_targets:
        prime_export=next(x for x in pe.DIRECTORY_ENTRY_EXPORT.symbols if x.name==b'GearsCaptureTargetPrime')
        if prime_export.forwarder:raise RuntimeError('Forwarded prime export rejected')
        prime_rva=prime_export.address
    pe.close()
    subprocess.run(['powershell','-NoProfile','-NonInteractive','-File',str(ROOT/'tools/debug-launch/Start-DebugGame.ps1')],check=True)
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
    memory=None;finished=False
    try:
        if loaded['gowde-steam.exe'][1].resolve()!=exe.resolve():raise RuntimeError('Fresh executable path differs')
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
        signatures={0x15cf08:bytes.fromhex('488bc45356574154415541564157'),0x15c968:bytes.fromhex('488bc4488958084889681048897018'),0x3a148c:bytes.fromhex('48895c2408488974241855'),0x193d50:bytes.fromhex('488bc4488958104889681848897020')}
        signatures[0x116250]=bytes.fromhex('488bc44889581848897020574154')
        signatures[0x15a80c]=bytes.fromhex('48895c241055488bec4883ec70')
        signatures[0x15a974]=bytes.fromhex('40534883ec20488bd9')
        signatures[0x1fc060]=bytes.fromhex('488bc44889581048897018574154')
        if args.check_shadow_state:signatures[0x33bc70]=bytes.fromhex('48895c2408574883ec208bda488bf9')
        read_only=Process(pid)
        try:
            entries=[{'rva':hex(offset),'matches':read_only.read(loaded['gowde-steam.exe'][0]+offset,len(value))==value} for offset,value in signatures.items()]
        finally:read_only.close()
        if not all(x['matches'] for x in entries):raise RuntimeError('Reservation changed an entry; do not activate')
        if args.prime_targets:
            code=remote_call(handle,resident[0]+prime_rva,None)
            if code:raise RuntimeError(f'Target priming failed ({code}); do not activate')
            prime_event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.TargetPrime.{pid}')
            if not prime_event:raise RuntimeError('Target priming readiness absent')
            k.CloseHandle(prime_event)
        report={'pid':pid,'dll':str(dll),'trampolines_reserved':True,'hooks_enabled':args.prime_targets,
                'native_hooks_enabled':False,'descriptor_hooks_enabled':3 if args.prime_targets else 0,'independent_entry_checks':entries}
        folder=ROOT/'analysis/runtime';folder.mkdir(parents=True,exist_ok=True)
        (folder/f'reserve-{pid}.json').write_text(json.dumps(report,indent=2))
        print(json.dumps(report))
    finally:
        if memory and finished:k.VirtualFreeEx(handle,memory,0,0x8000)
        k.CloseHandle(handle)


if __name__=='__main__':main()
