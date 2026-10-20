"""Windows ownership for the two bounded PathChoice service request files.

Publication never replaces a final path. Renewal and deletion lock and verify
the original regular file and its immutable bytes before changing metadata.
"""
import ctypes
from ctypes import wintypes as w
import os
from pathlib import Path
import uuid


class OwnershipError(RuntimeError):
    pass


class _Info(ctypes.Structure):
    _fields_ = [('attributes', w.DWORD), ('creation', w.FILETIME),
                ('access', w.FILETIME), ('write', w.FILETIME),
                ('volume', w.DWORD), ('size_high', w.DWORD), ('size_low', w.DWORD),
                ('links', w.DWORD), ('index_high', w.DWORD), ('index_low', w.DWORD)]


def _api():
    if os.name != 'nt':
        raise OSError('PathChoice owned sidecars require Windows file handles')
    k = ctypes.WinDLL('kernel32', use_last_error=True)
    signatures = {
        'CreateFileW': ([w.LPCWSTR, w.DWORD, w.DWORD, ctypes.c_void_p, w.DWORD, w.DWORD, w.HANDLE], w.HANDLE),
        'CloseHandle': ([w.HANDLE], w.BOOL),
        'GetFileInformationByHandle': ([w.HANDLE, ctypes.POINTER(_Info)], w.BOOL),
        'ReadFile': ([w.HANDLE, ctypes.c_void_p, w.DWORD, ctypes.POINTER(w.DWORD), ctypes.c_void_p], w.BOOL),
        'MoveFileW': ([w.LPCWSTR, w.LPCWSTR], w.BOOL),
        'SetFileTime': ([w.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(w.FILETIME)], w.BOOL),
        'SetFileInformationByHandle': ([w.HANDLE, ctypes.c_int, ctypes.c_void_p, w.DWORD], w.BOOL),
        'GetSystemTimeAsFileTime': ([ctypes.POINTER(w.FILETIME)], None),
    }
    for name, (args, result) in signatures.items():
        fn = getattr(k, name); fn.argtypes = args; fn.restype = result
    return k


def _checked(ok):
    if not ok:
        raise ctypes.WinError(ctypes.get_last_error())


class OwnedSidecar:
    def __init__(self, path):
        self.path = Path(path).absolute()
        self.raw = None
        self.file_id = None
        self._k = _api()

    @property
    def owned(self):
        return self.file_id is not None

    def _open(self, path, access=0x80000000):
        # No sharing: locks replacement, byte writes and competing metadata writes.
        h = self._k.CreateFileW(str(path), access, 0, None, 3, 0x00200000, None)
        if h == w.HANDLE(-1).value:
            raise ctypes.WinError(ctypes.get_last_error())
        return h

    def _info(self, handle):
        info = _Info()
        _checked(self._k.GetFileInformationByHandle(handle, ctypes.byref(info)))
        if info.attributes & (0x10 | 0x400):
            raise OwnershipError('Sidecar is a directory or reparse point')
        return info, (info.volume, info.index_high, info.index_low)

    def _verify(self, handle):
        info, identity = self._info(handle)
        if identity != self.file_id or (info.size_high << 32 | info.size_low) != len(self.raw):
            raise OwnershipError('Sidecar identity or size changed')
        buffer = ctypes.create_string_buffer(len(self.raw) + 1); count = w.DWORD()
        _checked(self._k.ReadFile(handle, buffer, len(self.raw) + 1, ctypes.byref(count), None))
        if count.value != len(self.raw) or buffer.raw[:count.value] != self.raw:
            raise OwnershipError('Sidecar bytes changed')
        return info

    def publish(self, raw):
        if self.owned:
            raise OwnershipError('Sidecar already owns an activation')
        if type(raw) is not bytes or not raw or len(raw) > 1024 * 1024:
            raise ValueError('Complete bounded immutable sidecar bytes required')
        temporary = self.path.with_name(self.path.name + '.' + uuid.uuid4().hex + '.tmp')
        moved = False
        try:
            with temporary.open('xb') as stream:
                stream.write(raw); stream.flush(); os.fsync(stream.fileno())
            h = self._open(temporary)
            try:
                _, identity = self._info(h)
            finally:
                self._k.CloseHandle(h)
            _checked(self._k.MoveFileW(str(temporary), str(self.path)))
            moved = True
            # Record the pre-move identity, never adopt a concurrent replacement.
            self.raw, self.file_id = raw, identity
            h = self._open(self.path)
            try:
                self._verify(h)
            finally:
                self._k.CloseHandle(h)
        finally:
            if not moved:
                temporary.unlink(missing_ok=True)

    def renew(self):
        if not self.owned:
            raise OwnershipError('No owned sidecar to renew')
        h = self._open(self.path, 0x80000000 | 0x100)
        try:
            info = self._verify(h)
            now = w.FILETIME(); self._k.GetSystemTimeAsFileTime(ctypes.byref(now))
            old = info.write.dwLowDateTime | info.write.dwHighDateTime << 32
            tick = max(now.dwLowDateTime | now.dwHighDateTime << 32, old + 1)
            stamp = w.FILETIME(tick & 0xffffffff, tick >> 32)
            _checked(self._k.SetFileTime(h, None, None, ctypes.byref(stamp)))
        finally:
            self._k.CloseHandle(h)

    def remove(self):
        if not self.owned:
            return
        try:
            h = self._open(self.path, 0x80000000 | 0x10000)
        except FileNotFoundError:
            self.raw = self.file_id = None
            return
        try:
            self._verify(h)
            flag = ctypes.c_ubyte(1)
            _checked(self._k.SetFileInformationByHandle(h, 4, ctypes.byref(flag), ctypes.sizeof(flag)))
        finally:
            self._k.CloseHandle(h)
        self.raw = self.file_id = None
