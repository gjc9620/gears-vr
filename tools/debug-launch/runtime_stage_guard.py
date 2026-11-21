"""One OS mutex across a private runtime's stage, launch and restoration.

Independent launchers keep their existing inner RepositoryLauncher.Owner lock.
The outer caller holds this guard until shared restoration is complete, then
releases it before independent encoding/review. No files, queues or tickets.
"""
import ctypes
import re


class RuntimeBusy(RuntimeError):
    pass


class RuntimeGuard:
    def __init__(self, instance='solo-m1-1028d', *, timeout_ms=0):
        if not isinstance(instance, str) or not re.fullmatch(r'[A-Za-z0-9_.-]{1,80}', instance):
            raise ValueError('Exact private instance name required')
        if type(timeout_ms) is not int or not 0<=timeout_ms<=45000:
            raise ValueError('Mutex wait must be bounded to 0..45000 ms')
        self.timeout_ms=timeout_ms
        self.name='Local\\GearsVR.SharedRuntime.StageToRestore.'+instance
        self.handle=None
        self.api=ctypes.WinDLL('kernel32',use_last_error=True)
        self.api.CreateMutexW.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_wchar_p]
        self.api.CreateMutexW.restype=ctypes.c_void_p
        self.api.WaitForSingleObject.argtypes=[ctypes.c_void_p,ctypes.c_uint32]
        self.api.WaitForSingleObject.restype=ctypes.c_uint32
        self.api.ReleaseMutex.argtypes=[ctypes.c_void_p]
        self.api.CloseHandle.argtypes=[ctypes.c_void_p]

    def __enter__(self):
        if self.handle is not None:raise RuntimeError('Guard already entered')
        handle=self.api.CreateMutexW(None,False,self.name)
        if not handle:raise ctypes.WinError(ctypes.get_last_error())
        outcome=self.api.WaitForSingleObject(handle,self.timeout_ms)
        if outcome==0:
            self.handle=handle;return self
        if outcome==0x80:
            self.api.ReleaseMutex(handle);self.api.CloseHandle(handle)
            raise RuntimeError('Abandoned runtime guard: verify prior shared restoration before startup')
        self.api.CloseHandle(handle)
        if outcome==0x102:raise RuntimeBusy('Shared stage/launch/restoration occupied: '+self.name)
        raise ctypes.WinError(ctypes.get_last_error())

    def __exit__(self,kind,value,traceback):
        handle=self.handle
        if handle is None:return False
        if not self.api.ReleaseMutex(handle):raise ctypes.WinError(ctypes.get_last_error())
        self.handle=None;self.api.CloseHandle(handle)
        return False
