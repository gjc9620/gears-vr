"""Signal our existing capture worker to disable its hooks and flush logs."""
import argparse
import ctypes as c
from ctypes import wintypes as w
parser=argparse.ArgumentParser()
parser.add_argument('--pid',type=int,required=True)
args=parser.parse_args()
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenEventW.argtypes=[w.DWORD,w.BOOL,w.LPCWSTR];k.OpenEventW.restype=w.HANDLE
k.SetEvent.argtypes=[w.HANDLE];k.SetEvent.restype=w.BOOL
k.CloseHandle.argtypes=[w.HANDLE]
event=k.OpenEventW(2,False,f'Local\\GearsVR.Capture.Stop.{args.pid}')
if not event:raise RuntimeError(f'No active capture stop event for PID {args.pid}; check whether capture already ended')
try:
    if not k.SetEvent(event):raise c.WinError(c.get_last_error())
    print('Stop requested; confirm disable_status in the completed capture before declaring patches restored.')
finally:k.CloseHandle(event)
