"""Use an explicitly selected Windows taskbar shortcut, then verify game focus.

No gameplay input is sent here. The persistent game_io pad service is untouched.
The caller must obtain the slot from the user; never guess it from window order.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import time
from game_io import user, kernel, game_window, focused, interrupted


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--slot',type=int,required=True,choices=range(10))
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--creation-time',type=int,required=True)
    args=parser.parse_args()
    pid,hwnd=game_window()
    if pid!=args.pid:raise RuntimeError('Game PID differs from the owned session')
    handle=kernel.OpenProcess(0x1000,False,pid)
    if not handle:raise c.WinError(c.get_last_error())
    try:
        kernel.GetProcessTimes.argtypes=[w.HANDLE,*([c.POINTER(w.FILETIME)]*4)]
        stamps=[w.FILETIME() for _ in range(4)]
        if not kernel.GetProcessTimes(handle,*(c.byref(x) for x in stamps)):
            raise c.WinError(c.get_last_error())
        creation=(stamps[0].dwHighDateTime<<32)|stamps[0].dwLowDateTime
        if creation!=args.creation_time:raise RuntimeError('Game creation identity changed')
        sent=False
        if not focused(hwnd):
            if interrupted():raise RuntimeError('Focus shortcut interrupted')
            if any(user.GetAsyncKeyState(key)&0x8000 for key in (0x10,0x11,0x12,0x5b,0x5c)):
                raise RuntimeError('A modifier key is held; no shortcut sent')
            user.keybd_event.argtypes=[w.BYTE,w.BYTE,w.DWORD,c.c_size_t]
            user.keybd_event.restype=None
            digit=0x30+args.slot
            try:
                user.keybd_event(0x5b,0,0,0)
                user.keybd_event(digit,0,0,0)
            finally:
                user.keybd_event(digit,0,2,0)
                user.keybd_event(0x5b,0,2,0)
            sent=True
            deadline=time.monotonic()+2
            while not focused(hwnd) and time.monotonic()<deadline:
                if interrupted():raise RuntimeError('Focus shortcut interrupted')
                time.sleep(.05)
        if game_window()!=(pid,hwnd) or not focused(hwnd):
            raise RuntimeError('Taskbar shortcut did not select the expected game; no gameplay input sent')
        print(json.dumps(dict(pid=pid,creation_time=creation,hwnd=hwnd,focused=True,
            shortcut=f'Win+{args.slot}',shortcut_sent=sent,gameplay_input_sent=False)))
    finally:kernel.CloseHandle(handle)


if __name__=='__main__':main()
