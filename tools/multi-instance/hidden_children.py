"""Normal cleanup of exact same-image children isolated by the startup DLL."""
from pathlib import Path
import ctypes,json,time
from ctypes import wintypes as w

def finish(selected,control,game):
    path=Path(selected)/'background-children.jsonl'
    if not path.exists():return {'recorded':0,'remaining':[]}
    if path.stat().st_size>256*1024:raise RuntimeError('Owned child receipt exceeds bound')
    records=[json.loads(line) for line in path.read_text().splitlines() if line.strip()]
    if len(records)>128:raise RuntimeError('Owned child receipt count exceeds bound')
    unique={}
    for record in records:
        if record.get('parent_pid')!=game['pid'] or record.get('same_image') is not True:raise RuntimeError('Child parent/image proof differs')
        if record.get('desktop')!='GearsVR.HiddenChildren.'+str(game['pid']):raise RuntimeError('Child private desktop differs')
        unique[(record['pid'],record['creation_time'])]=record
    identities={pid:{'pid':pid,'creation_time':stamp,'image':game['image']} for pid,stamp in unique}
    user=ctypes.WinDLL('user32',use_last_error=True)
    user.OpenDesktopW.argtypes=[w.LPCWSTR,w.DWORD,w.BOOL,w.DWORD];user.OpenDesktopW.restype=w.HANDLE
    user.CloseDesktop.argtypes=[w.HANDLE]
    user.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM]
    callback=ctypes.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
    user.EnumDesktopWindows.argtypes=[w.HANDLE,callback,w.LPARAM]
    deadline=time.monotonic()+8
    while time.monotonic()<deadline:
        remaining=[identity for identity in identities.values() if control.identity(identity['pid'])==identity]
        if not remaining:break
        desktop=user.OpenDesktopW('GearsVR.HiddenChildren.'+str(game['pid']),0,False,0x41)
        if not desktop:break
        @callback
        def close(hwnd,_):
            pid=w.DWORD();user.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
            identity=identities.get(pid.value)
            if identity and control.identity(pid.value)==identity:user.PostMessageW(hwnd,0x10,0,0)
            return True
        try:user.EnumDesktopWindows(desktop,close,0)
        finally:user.CloseDesktop(desktop)
        time.sleep(.1)
    return {'recorded':len(unique),'remaining':[identity for identity in identities.values() if control.identity(identity['pid'])==identity],'forced_termination':False}
