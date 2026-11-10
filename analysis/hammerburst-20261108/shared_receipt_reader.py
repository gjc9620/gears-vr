"""FAILED EXPERIMENT, not imported by runtime: Windows shared receipt reader.

Actual four-test probe:2 passed/2 failed. Even FILE_SHARE_DELETE did not make
Path.replace succeed while this handle stayed open on this host. Do not use
this as a publisher repair or assert it resolves the observed WinError5.

This changes file sharing only. It never retries a launch, changes a receipt,
retains a value across reads, or supplies identity/action authority.
"""
import ctypes
from ctypes import wintypes as w
import json,msvcrt,os

def open_shared_text(path):
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateFileW.argtypes=[w.LPCWSTR,w.DWORD,w.DWORD,w.LPVOID,w.DWORD,w.DWORD,w.HANDLE]
    kernel.CreateFileW.restype=w.HANDLE
    kernel.CloseHandle.argtypes=[w.HANDLE];kernel.CloseHandle.restype=w.BOOL
    # Existing file only, GENERIC_READ, SHARE_READ|WRITE|DELETE. In particular,
    # do not deny the publisher's rename while this reader consumes old bytes.
    handle=kernel.CreateFileW(str(path),0x80000000,7,None,3,0x80,None)
    if handle==ctypes.c_void_p(-1).value:raise ctypes.WinError(ctypes.get_last_error())
    try:fd=msvcrt.open_osfhandle(handle,os.O_RDONLY|os.O_BINARY)
    except BaseException:
        kernel.CloseHandle(handle);raise
    try:return os.fdopen(fd,'r',encoding='utf-8-sig')
    except BaseException:
        os.close(fd);raise

def read_shared_json(path):
    with open_shared_text(path) as source:return json.load(source)
