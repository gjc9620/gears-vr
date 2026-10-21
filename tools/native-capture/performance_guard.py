"""Read-only exclusion checks for comparable game performance measurements.

Isolated game instances deliberately have different launcher mutexes. This
guard sees the raw process list and registered live workers across instances.
It never stops a process, changes a window, or writes a profile.
"""
import ctypes
from ctypes import wintypes as w
import json
from pathlib import Path

WINDOWS_EPOCH_100NS=116444736000000000
GAME_NAMES={'gowde-steam.exe','goweday-steam.exe'}

def process_snapshot():
    class Entry(ctypes.Structure):
        _fields_=[('dwSize',w.DWORD),('cntUsage',w.DWORD),('pid',w.DWORD),
            ('heap',ctypes.c_size_t),('module',w.DWORD),('threads',w.DWORD),
            ('parent',w.DWORD),('priority',w.LONG),('flags',w.DWORD),('exe',w.WCHAR*260)]
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD]
    kernel.CreateToolhelp32Snapshot.restype=w.HANDLE
    kernel.Process32FirstW.argtypes=[w.HANDLE,ctypes.POINTER(Entry)]
    kernel.Process32NextW.argtypes=[w.HANDLE,ctypes.POINTER(Entry)]
    kernel.CloseHandle.argtypes=[w.HANDLE]
    handle=kernel.CreateToolhelp32Snapshot(2,0)
    if handle==ctypes.c_void_p(-1).value:raise ctypes.WinError(ctypes.get_last_error())
    result={};entry=Entry();entry.dwSize=ctypes.sizeof(entry)
    try:
        valid=kernel.Process32FirstW(handle,ctypes.byref(entry))
        while valid:
            result[entry.pid]={'name':entry.exe.casefold(),'parent':entry.parent}
            valid=kernel.Process32NextW(handle,ctypes.byref(entry))
        if ctypes.get_last_error()!=18:raise ctypes.WinError(ctypes.get_last_error())
    finally:kernel.CloseHandle(handle)
    return result

def blockers(root,processes,identity,*,exclude=()):
    """Identity is the existing creation-time/image process reader."""
    root=Path(root);excluded=set(exclude);found=[]
    for pid,process in processes.items():
        if pid in excluded:continue
        if process['name'] in GAME_NAMES:found.append({'kind':'game','pid':pid})
        if process['name']=='cl.exe':found.append({'kind':'compiler','pid':pid})
    base=root/'analysis/instances'
    workers=list(base.glob('*/worker.json'))
    indexes=[root/'analysis/runtime/vr-launcher-session.json',*base.glob('*/launcher-session.json')]
    if len(workers)>256 or len(indexes)>257:
        return found+[{'kind':'registration_limit','pid':None}]
    for path in workers:
        try:
            row=json.loads(path.read_text(encoding='utf-8'));pid=row.get('worker_pid')
            if pid in excluded or pid not in processes:continue
            current=identity(pid)
            if not current or not processes[pid]['name'].startswith('python'):continue
            born_ns=(current['creation_time']-WINDOWS_EPOCH_100NS)*100
            # A reused PID born after this registration does not own that worker.
            if born_ns>path.stat().st_mtime_ns+1_000_000_000:continue
            found.append({'kind':'worker','pid':pid,'path':str(path),'identity':current})
        except (OSError,ValueError,KeyError,TypeError):continue
    for path in indexes:
        try:
            row=json.loads(path.read_text(encoding='utf-8'));owner=row.get('owner_identity')or{}
            pid=owner.get('pid')
            if pid in excluded or pid not in processes:continue
            if owner and identity(pid)==owner:
                found.append({'kind':'launcher','pid':pid,'path':str(path),'identity':owner})
        except (OSError,ValueError,KeyError,TypeError):continue
    return found
