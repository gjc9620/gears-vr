"""One manual6DoF launch through the live signed-in/offline Steam client.

An existing game is never adopted. Bind one fresh executable and its live
Steam parent before the usual reservation. No login/license/file changes.
"""
import ctypes as C
from ctypes import wintypes as W
from pathlib import Path
import subprocess,time
APP_ID='2523720'

def processes():
    class Entry(C.Structure):
        _fields_=[('size',W.DWORD),('usage',W.DWORD),('pid',W.DWORD),('heap',C.c_size_t),
            ('module',W.DWORD),('threads',W.DWORD),('parent',W.DWORD),('priority',W.LONG),
            ('flags',W.DWORD),('name',W.WCHAR*260)]
    k=C.WinDLL('kernel32',use_last_error=True)
    k.CreateToolhelp32Snapshot.argtypes=[W.DWORD,W.DWORD];k.CreateToolhelp32Snapshot.restype=W.HANDLE
    k.Process32FirstW.argtypes=[W.HANDLE,C.POINTER(Entry)];k.Process32FirstW.restype=W.BOOL
    k.Process32NextW.argtypes=[W.HANDLE,C.POINTER(Entry)];k.Process32NextW.restype=W.BOOL
    k.CloseHandle.argtypes=[W.HANDLE];k.CloseHandle.restype=W.BOOL
    snapshot=k.CreateToolhelp32Snapshot(2,0)
    if snapshot in (None,C.c_void_p(-1).value):raise C.WinError(C.get_last_error())
    rows=[]
    try:
        item=Entry();item.size=C.sizeof(item);ok=k.Process32FirstW(snapshot,C.byref(item))
        while ok:
            rows.append(dict(pid=item.pid,parent=item.parent,name=item.name));ok=k.Process32NextW(snapshot,C.byref(item))
        if C.get_last_error()!=18:raise C.WinError(C.get_last_error())
    finally:k.CloseHandle(snapshot)
    return rows

def selected_game(rows,game,steam,identity,not_before):
    matches=[x for x in rows if x['name'].casefold()=='gowde-steam.exe']
    if len(matches)>1:raise RuntimeError('Multiple game processes appeared after Steam launch; none adopted')
    if not matches:return None
    row=matches[0];current=identity(row['pid'])
    if not current:return None
    if Path(current['image']).resolve()!=Path(game).resolve() or current['creation_time']<not_before:
        raise RuntimeError('Steam launch did not produce the requested fresh game identity')
    if row['parent']!=steam['pid'] or identity(steam['pid'])!=steam:
        raise RuntimeError('Fresh game is not a child of the same live Steam client; none adopted')
    return current

def launch(game,args,*,control=None,enumerate_processes=None,popen=subprocess.Popen,
           clock=time.monotonic,sleep=time.sleep,steam=None,timeout=30):
    if control is None:
        import sys
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'native-capture'))
        from run_stereo_continuous import WindowsControl
        control=WindowsControl()
    if enumerate_processes is None:enumerate_processes=processes
    if steam is None:
        from steam_account import registry_state
        _,pid,folder=registry_state();steam=control.identity(pid)
        if not steam or Path(steam['image']).resolve()!=(folder/'steam.exe').resolve():
            raise RuntimeError('Steam client identity unavailable; keep Steam open for purchased-game startup')
    if any(x['name'].casefold()=='gowde-steam.exe' for x in enumerate_processes()):
        raise RuntimeError('A game is already running; manual Steam launch will not adopt or interrupt it')
    if control.identity(steam['pid'])!=steam:raise RuntimeError('Steam client changed before launch')
    not_before=time.time_ns()//100+116444736000000000
    child=popen([steam['image'],'-applaunch',APP_ID,*map(str,args)],cwd=str(Path(steam['image']).parent),
        creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0),stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    deadline=clock()+timeout
    while clock()<deadline:
        if control.identity(steam['pid'])!=steam:raise RuntimeError('Steam client exited/changed during game launch; no retry')
        actual=selected_game(enumerate_processes(),game,steam,control.identity,not_before)
        if actual:
            if control.identity(actual['pid'])!=actual:raise RuntimeError('Fresh Steam game exited during identity confirmation')
            return dict(pid=actual['pid'],creation_time=actual['creation_time'],steam_client_identity=steam,
                steam_launch_request_pid=child.pid,steam_app_id=APP_ID,steam_authenticated_launch=True)
        sleep(.1)
    raise RuntimeError('Steam did not start the requested fresh game within30seconds; no direct-EXE fallback or retry')
