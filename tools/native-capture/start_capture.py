"""Version-gated launcher for our capture DLL; temporary live instrumentation.

Unlike package-inspector tools, this allocates/writes loader memory and loads a
DLL into the selected game. The capture DLL patches ordinary-code entries.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import pathlib
import subprocess
import sys
import time
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/package-inspector'))
sys.path.insert(0,str(ROOT/'tools/package-inspector/python-libs'))
import pefile
from resolve_camera import Resolver,psapi

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


def modules(handle):
    slots=(w.HMODULE*2048)();size=w.DWORD()
    if not psapi.EnumProcessModulesEx(handle,slots,c.sizeof(slots),c.byref(size),3):
        raise c.WinError(c.get_last_error())
    if size.value>c.sizeof(slots):raise RuntimeError('Module list exceeds bounded buffer')
    result={}
    for address in slots[:size.value//c.sizeof(w.HMODULE)]:
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


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--seconds',type=int,default=90,choices=range(10,181))
    parser.add_argument('--build-dir',type=pathlib.Path,default=pathlib.Path(__file__).with_name('build'))
    parser.add_argument('--dll-name',default='GearsCapture.dll',help='Name of an independently built diagnostic DLL')
    parser.add_argument('--use-reserved',action='store_true',help='Activate the same DLL whose unenabled trampolines were reserved during fresh startup')
    mode=parser.add_mutually_exclusive_group()
    mode.add_argument('--wobble',action='store_true',help='Enable the gated 1.5-degree render-only yaw test')
    mode.add_argument('--sweep',action='store_true',help='Run pitch/yaw/roll and three tiny render-only translations')
    mode.add_argument('--live-head',action='store_true',help='Bounded real-headset test (requires verified pose producer)')
    mode.add_argument('--gpu-diagnostics',action='store_true',help='Observe swapchain metadata and ExecuteCommandLists, without GPU copies')
    mode.add_argument('--family-diagnostics',action='store_true',help='Observe bounded view arrays at the candidate render-family submission')
    args=parser.parse_args()
    if pathlib.Path(args.dll_name).name!=args.dll_name or not args.dll_name.lower().endswith('.dll'):
        raise RuntimeError('DLL name must be a basename ending in .dll')
    dll=args.build_dir.resolve()/args.dll_name
    test=dll.with_name('CaptureSelfTest.exe')
    if not dll.is_file() or not test.is_file():raise RuntimeError('Build capture DLL and self-test first')
    subprocess.run([str(test),str(dll)],check=True)
    if args.gpu_diagnostics: subprocess.run([str(test),str(dll),'--gpu'],check=True)
    if args.family_diagnostics: subprocess.run([str(test),str(dll),'--family'],check=True)
    if args.family_diagnostics: subprocess.run([str(test),str(dll),'--clone'],check=True)
    if args.family_diagnostics: subprocess.run([str(test),str(dll),'--renderer'],check=True)
    profile=json.loads((ROOT/'tools/package-inspector/reloaded-profile.json').read_text())
    r=Resolver(args.pid,profile)
    try:camera=r.resolve()
    finally:r.p.close()
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
        elif dll.name.lower() in loaded:raise RuntimeError('Capture DLL already loaded; restart game before a new capture build/run')
        active_event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.Stop.{args.pid}')
        if active_event:
            k.CloseHandle(active_event);raise RuntimeError('Another capture is active; overlapping instrumentation rejected')
        # Verify exact target prologues before any remote allocation or load.
        signatures={0x15cf08:bytes.fromhex('488bc45356574154415541564157'),
                    0x15c968:bytes.fromhex('488bc4488958084889681048897018')}
        if args.family_diagnostics: signatures[0x3a148c]=bytes.fromhex('48895c2408488974241855')
        if args.family_diagnostics: signatures[0x193d50]=bytes.fromhex('488bc4488958104889681848897020')
        if args.family_diagnostics: signatures[0x116250]=bytes.fromhex('488bc44889581848897020574154')
        r=Resolver(args.pid,profile)
        try:
            refreshed=r.resolve()
            if refreshed['local_player_address']!=camera['local_player_address']:
                raise RuntimeError('Local player changed during preparation')
            for offset,signature in signatures.items():
                if r.read(r.base+offset,len(signature))!=signature:raise RuntimeError('Target prologue differs; instrumentation rejected')
        finally:r.p.close()
        config='[Capture]\n'+f'pid={args.pid}\nseconds={args.seconds}\n'+f"local_player={camera['local_player_address']}\ncontroller={camera['controller_address']}\ntimestamp={camera['timestamp_address']}\n"
        yaw_address=int(camera['pov_address'],16)+int(camera['offsets']['Rotation'],16)+4
        config+=f"camera={camera['camera_address']}\ncamera_yaw={yaw_address:#x}\nwobble={int(args.wobble)}\nsweep={int(args.sweep)}\nlive_head={int(args.live_head)}\n"
        config+=f'gpu_diagnostics={int(args.gpu_diagnostics)}\n'
        config+=f'family_diagnostics={int(args.family_diagnostics)}\n'
        dll.with_name('capture.ini').write_text(config,encoding='utf-16')
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
        code=remote_call(handle,module_base+export.address,None)
        if code:raise RuntimeError(f'Capture start failed: {code}')
        deadline=time.monotonic()+15
        while time.monotonic()<deadline:
            event=k.OpenEventW(0x100000,False,f'Local\\GearsVR.Capture.Stop.{args.pid}')
            if event:
                k.CloseHandle(event)
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
                    print(f'Capture active for PID {args.pid}; status file is readable after capture ends.')
                    return
                value=''
            if f'capturing pid={args.pid} ' in value:print(value.strip());return
            if 'rejected:' in value or 'failed' in value:raise RuntimeError(value.strip())
            time.sleep(.1)
        raise RuntimeError('Capture start status not confirmed; inspect capture-status.txt before continuing')
    finally:
        if remote_memory and loader_finished:k.VirtualFreeEx(handle,remote_memory,0,0x8000)
        k.CloseHandle(handle)


if __name__=='__main__':main()
