"""Session-owned calibration display data; it never accepts a measurement."""
import ctypes as C
from ctypes import wintypes as W
import json
import os
from pathlib import Path
import re
import struct
import time
import uuid

WIRE = struct.Struct('<8sII6Q16sQqIIII')
assert WIRE.size == 112
PHASES = {'waiting': 1, 'holding': 2, 'saved': 3, 'cancelled': 4, 'failed': 5}
LANGUAGES = {'en': 0, 'zh-CN': 1}
TOKEN = re.compile(r'[0-9a-f]{32}\Z')


def token_bytes(token):
    if not isinstance(token, str) or not TOKEN.fullmatch(token) or token == '0' * 32:
        raise ValueError('Invalid calibration display token')
    return bytes.fromhex(token)


def object_names(token):
    token_bytes(token)
    name = 'Local\\GearsVR.ArmspanUI.v1.' + token
    return name, name + '.Lock'


def _integer(value, maximum):
    return type(value) is int and 0 < value <= maximum


def packet(game, owner, publisher, token, sequence, updated_ns, phase, progress, language):
    for identity in (game, owner, publisher):
        if (not _integer(identity.get('pid'), 0xffffffff) or
                not _integer(identity.get('creation_time'), 0xffffffffffffffff)):
            raise ValueError('Invalid calibration display identity')
    if not _integer(sequence, 0xffffffffffffffff) or not _integer(updated_ns, 0x7fffffffffffffff):
        raise ValueError('Invalid calibration display publication')
    if phase not in PHASES or language not in LANGUAGES or type(progress) is not int:
        raise ValueError('Invalid calibration display content')
    if ((phase == 'holding' and not 0 <= progress < 1000) or
            (phase == 'saved' and progress != 1000) or
            (phase not in ('holding', 'saved') and progress != 0)):
        raise ValueError('Invalid calibration display progress')
    return WIRE.pack(b'GCALUI1', 1, WIRE.size, game['pid'], game['creation_time'],
                     owner['pid'], owner['creation_time'], publisher['pid'], publisher['creation_time'],
                     token_bytes(token), sequence, updated_ns, PHASES[phase], progress, LANGUAGES[language], 0)


class Publisher:
    """One fresh mapping/mutex, held until the owned XR child has exited."""
    def __init__(self, game, owner, language='en', *, token=None):
        self.token = token or uuid.uuid4().hex
        self.game, self.owner, self.language = dict(game), dict(owner), language
        self.mapping = self.mutex = self.view = None
        self.sequence = self.updated_ns = 0
        self.api = C.WinDLL('kernel32', use_last_error=True)
        for name, restype, arguments in (
            ('CreateMutexW', W.HANDLE, (C.c_void_p, W.BOOL, W.LPCWSTR)),
            ('CreateFileMappingW', W.HANDLE, (W.HANDLE, C.c_void_p, W.DWORD, W.DWORD, W.DWORD, W.LPCWSTR)),
            ('MapViewOfFile', C.c_void_p, (W.HANDLE, W.DWORD, W.DWORD, W.DWORD, C.c_size_t)),
            ('UnmapViewOfFile', W.BOOL, (C.c_void_p,)),
            ('WaitForSingleObject', W.DWORD, (W.HANDLE, W.DWORD)),
            ('ReleaseMutex', W.BOOL, (W.HANDLE,)), ('CloseHandle', W.BOOL, (W.HANDLE,)),
            ('GetCurrentProcess', W.HANDLE, ()),
            ('GetProcessTimes', W.BOOL, (W.HANDLE, C.POINTER(W.FILETIME), C.POINTER(W.FILETIME), C.POINTER(W.FILETIME), C.POINTER(W.FILETIME))),
            ('QueryPerformanceCounter', W.BOOL, (C.POINTER(C.c_int64),)),
            ('QueryPerformanceFrequency', W.BOOL, (C.POINTER(C.c_int64),))):
            method = getattr(self.api, name)
            method.restype, method.argtypes = restype, arguments
        created, exited, kernel, user = (W.FILETIME() for _ in range(4))
        if not self.api.GetProcessTimes(self.api.GetCurrentProcess(), C.byref(created), C.byref(exited), C.byref(kernel), C.byref(user)):
            raise OSError('Calibration display publisher identity unavailable')
        self.publisher = {'pid': os.getpid(), 'creation_time': (created.dwHighDateTime << 32) | created.dwLowDateTime}
        frequency = C.c_int64()
        if not self.api.QueryPerformanceFrequency(C.byref(frequency)) or frequency.value <= 0:
            raise OSError('Calibration display clock unavailable')
        self.frequency = frequency.value
        name, mutex_name = object_names(self.token)
        # Validate all fields before creating any kernel object.
        packet(self.game, self.owner, self.publisher, self.token, 1, self.now(), 'waiting', 0, language)
        try:
            C.set_last_error(0)
            self.mutex = self.api.CreateMutexW(None, False, mutex_name)
            collision = C.get_last_error() == 183
            if not self.mutex or collision:
                raise OSError('Calibration display mutex already exists or is unavailable')
            C.set_last_error(0)
            self.mapping = self.api.CreateFileMappingW(W.HANDLE(-1), None, 4, 0, WIRE.size, name)
            collision = C.get_last_error() == 183
            if not self.mapping or collision:
                raise OSError('Calibration display mapping already exists or is unavailable')
            self.view = self.api.MapViewOfFile(self.mapping, 2, 0, 0, WIRE.size)
            if not self.view:
                raise OSError('Calibration display mapping view unavailable')
            self.publish('waiting', 0)
        except BaseException:
            self.close()
            raise

    def now(self):
        counter = C.c_int64()
        if not self.api.QueryPerformanceCounter(C.byref(counter)):
            raise OSError('Calibration display clock unavailable')
        return counter.value * 1_000_000_000 // self.frequency

    def publish(self, phase, progress, now_ns=None):
        if not self.view or not self.mutex:
            raise RuntimeError('Calibration display publisher is closed')
        now_ns = self.now() if now_ns is None else now_ns
        if now_ns < self.updated_ns:
            raise RuntimeError('Calibration display clock moved backwards')
        sequence = self.sequence + 1
        data = packet(self.game, self.owner, self.publisher, self.token, sequence, now_ns, phase, progress, self.language)
        waited = self.api.WaitForSingleObject(self.mutex, 100)
        if waited != 0:
            if waited == 0x80:
                self.api.ReleaseMutex(self.mutex)
            raise RuntimeError('Calibration display update lock unavailable')
        try:
            C.memmove(self.view, data, len(data))
        finally:
            if not self.api.ReleaseMutex(self.mutex):
                raise OSError('Calibration display lock release failed')
        self.sequence, self.updated_ns = sequence, now_ns
        return sequence

    def close(self):
        failures = []
        if self.view:
            if not self.api.UnmapViewOfFile(self.view):
                failures.append('view')
            self.view = None
        for name in ('mapping', 'mutex'):
            handle = getattr(self, name)
            if handle and not self.api.CloseHandle(handle):
                failures.append(name)
            setattr(self, name, None)
        if failures:
            raise OSError('Calibration display cleanup failed: ' + ', '.join(failures))


def wait_saved(path, token, minimum_sequence, child, owner_alive, cancel, publisher,
               *, clock=time.monotonic, sleep=time.sleep, timeout=2.5, visible_seconds=.6):
    """Require one successful XR saved-card submission, then keep it readable."""
    token_bytes(token)
    deadline = clock() + timeout
    offset = 0
    pending = b''
    seen_at = None
    last_pulse = clock()
    evidence = None
    while clock() < deadline:
        if cancel():
            return None
        if child.poll() is not None or not owner_alive():
            raise RuntimeError('Calibration display child/owner exited before saved feedback')
        try:
            with Path(path).open('rb') as stream:
                stream.seek(offset)
                data = stream.read(65536)
                offset += len(data)
        except FileNotFoundError:
            data = b''
        pending += data
        lines = pending.split(b'\n')
        pending = lines.pop()
        if len(pending) > 65536 or any(len(line) > 65536 for line in lines):
            raise RuntimeError('Calibration display feedback line exceeds its bound')
        for line in lines:
            try:
                value = json.loads(line)
            except (ValueError, UnicodeError):
                continue
            if (isinstance(value, dict) and value.get('armspan_ui_presented') is True and
                    value.get('token') == token and value.get('phase') == 'saved' and
                    _integer(value.get('sequence'), 0xffffffffffffffff) and
                    minimum_sequence <= value['sequence'] <= publisher.sequence and
                    value.get('end_frame_succeeded') is True):
                if seen_at is None:
                    seen_at, evidence = clock(), value
        if seen_at is not None and clock() - seen_at >= visible_seconds:
            return evidence
        if clock() - last_pulse >= .2:
            publisher.publish('saved', 1000)
            last_pulse = clock()
        sleep(.01)
    raise TimeoutError('Calibration was saved but its headset feedback was not confirmed')
