"""One nonactivating resize attempt for an explicitly observed tiny DXGI output.

The session owner must bound calls to once per session. This helper does not
observe DXGI or establish recovery: callers must collect subsequent GPU evidence.
"""
import ctypes
from ctypes import wintypes
import json
import pathlib

SWP_FLAGS = 0x0002 | 0x0004 | 0x0010 | 0x0200  # NOMOVE/NOZORDER/NOACTIVATE/NOOWNERZORDER


class MonitorInfo(ctypes.Structure):
    _fields_ = [('cbSize', wintypes.DWORD), ('rcMonitor', wintypes.RECT),
                ('rcWork', wintypes.RECT), ('dwFlags', wintypes.DWORD)]


class StartupTinyOutputPolicy:
    """Bounded incremental owned-XR log reader and once-per-session decision."""
    def __init__(self):
        self.offset = 0
        self.pending = b''
        self.dimensions = None
        self.tiny_since = None
        self.stereo_submitted = False
        self.consumed = False

    def update(self, event, now):
        if not isinstance(event, dict):
            return
        if event.get('reference_epoch_first_projection') is True and event.get('end_frame_succeeded') is True:
            self.stereo_submitted = True
        if event.get('stereo_source_retired') is True:
            self.dimensions = None
            self.tiny_since = None
        if 'gpu_source_size' in event:
            size = event['gpu_source_size']
            tiny = (isinstance(size, list) and len(size) == 2
                    and all(type(v) is int and 1 <= v <= 8 for v in size))
            self.dimensions = list(size) if tiny else None
            self.tiny_since = (now if self.tiny_since is None else self.tiny_since) if tiny else None

    def read_new(self, path, now):
        limit = 65536
        with path.open('rb') as stream:
            size = stream.seek(0, 2)
            if size < self.offset or size - self.offset > limit:
                # Missing events may have retired/replaced the source. Fail closed.
                self.dimensions = None
                self.tiny_since = None
                self.pending = b''
                self.offset = size
                self.stereo_submitted = True  # Unknown lost history: disallow recovery.
                return
            stream.seek(self.offset)
            data = stream.read(limit)
            self.offset = stream.tell()
        lines = (self.pending + data).split(b'\n')
        self.pending = lines.pop()
        if len(self.pending) > limit:
            self.pending = b''
            self.stereo_submitted = True
            self.tiny_since = None
        for line in lines:
            try:
                event = json.loads(line)
            except (ValueError, UnicodeError):
                continue
            self.update(event, now)

    def ready(self, now, stereo_submitted=False):
        return (not self.consumed and not self.stereo_submitted and not stereo_submitted
                and self.tiny_since is not None and now - self.tiny_since >= 2.0)


def skip_reason(observed_output, expected, state):
    if (not isinstance(observed_output, (list, tuple)) or len(observed_output) != 2
            or any(type(v) is not int or not 1 <= v <= 8 for v in observed_output)):
        return 'output_not_explicitly_tiny'
    if (not isinstance(expected, dict) or type(expected.get('pid')) is not int
            or expected['pid'] <= 0 or type(expected.get('creation_time')) is not int
            or expected['creation_time'] <= 0 or not expected.get('image')):
        return 'invalid_expected_identity'
    if not state or state.get('identity') != expected or state.get('window_pid') != expected['pid']:
        return 'identity_or_window_changed'
    if not state.get('visible') or state.get('minimized') or state.get('maximized') or not state.get('windowed'):
        return 'window_not_visible_normal_windowed'
    client = state.get('client_size', (0, 0))
    if client[0] < 640 or client[1] < 360:
        return 'client_too_small'
    rect = state.get('rect')
    if not rect or rect[2] - rect[0] < 2 or rect[3] - rect[1] < 1:
        return 'invalid_window_rect'
    return None


def attempt_recovery(control, expected, hwnd, observed_output):
    """Return evidence only; caller owns once-per-session policy and DXGI sampling."""
    result = {'hwnd': hwnd, 'observed_output': observed_output, 'attempted': False,
              'resized': False, 'restored': False, 'foreground_before': None,
              'foreground_after': None, 'result': 'skipped'}
    try:
        result['foreground_before'] = control.foreground()
        state = control.snapshot(expected['pid'], hwnd)
        result['before'] = state
        reason = skip_reason(observed_output, expected, state)
        if reason:
            result['reason'] = reason
            return result
        rect = state['rect']
        width, height = rect[2] - rect[0], rect[3] - rect[1]
        result['original_size'] = [width, height]
        result['intermediate_size'] = [width - 1, height]
        result['attempted'] = True
        result['resized'] = bool(control.resize(hwnd, width - 1, height))
        after = control.snapshot(expected['pid'], hwnd)
        result['after_resize'] = after
        # Preserve a concurrent move/resize, mode change, or replacement HWND.
        reason = skip_reason(observed_output, expected, after)
        intermediate_rect = (rect[0], rect[1], rect[2] - 1, rect[3])
        if reason or tuple(after['rect']) != intermediate_rect:
            result['result'] = 'restore_skipped'
            result['reason'] = reason or 'window_rect_changed'
        elif not result['resized']:
            result['result'] = 'resize_failed'
        else:
            result['restored'] = bool(control.resize(hwnd, width, height))
            result['result'] = 'resize_restore_attempted' if result['restored'] else 'restore_failed'
            result['after_restore'] = control.snapshot(expected['pid'], hwnd)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        result['result'] = 'error'
        result['error'] = str(exc)
    finally:
        try:
            result['foreground_after'] = control.foreground()
        except OSError:
            pass
    return result


class WindowsControl:
    """Small ctypes adapter; no foreground activation or game-control operations."""
    def __init__(self):
        self.k = ctypes.WinDLL('kernel32', use_last_error=True)
        self.u = ctypes.WinDLL('user32', use_last_error=True)
        pointer = ctypes.POINTER
        for dll, name, args, restype in (
            (self.k, 'OpenProcess', [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD], wintypes.HANDLE),
            (self.k, 'CloseHandle', [wintypes.HANDLE], wintypes.BOOL),
            (self.k, 'GetProcessTimes', [wintypes.HANDLE] + [pointer(wintypes.FILETIME)] * 4, wintypes.BOOL),
            (self.k, 'GetExitCodeProcess', [wintypes.HANDLE, pointer(wintypes.DWORD)], wintypes.BOOL),
            (self.k, 'QueryFullProcessImageNameW', [wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR, pointer(wintypes.DWORD)], wintypes.BOOL),
            (self.u, 'GetForegroundWindow', [], wintypes.HWND),
            (self.u, 'IsWindow', [wintypes.HWND], wintypes.BOOL),
            (self.u, 'IsWindowVisible', [wintypes.HWND], wintypes.BOOL),
            (self.u, 'IsIconic', [wintypes.HWND], wintypes.BOOL),
            (self.u, 'IsZoomed', [wintypes.HWND], wintypes.BOOL),
            (self.u, 'GetWindowThreadProcessId', [wintypes.HWND, pointer(wintypes.DWORD)], wintypes.DWORD),
            (self.u, 'GetWindowRect', [wintypes.HWND, pointer(wintypes.RECT)], wintypes.BOOL),
            (self.u, 'GetClientRect', [wintypes.HWND, pointer(wintypes.RECT)], wintypes.BOOL),
            (self.u, 'GetWindowLongW', [wintypes.HWND, ctypes.c_int], wintypes.LONG),
            (self.u, 'MonitorFromWindow', [wintypes.HWND, wintypes.DWORD], wintypes.HANDLE),
            (self.u, 'GetMonitorInfoW', [wintypes.HANDLE, pointer(MonitorInfo)], wintypes.BOOL),
            (self.u, 'SetWindowPos', [wintypes.HWND, wintypes.HWND] + [ctypes.c_int] * 4 + [wintypes.UINT], wintypes.BOOL),
        ):
            fn = getattr(dll, name)
            fn.argtypes, fn.restype = args, restype

    def identity(self, pid):
        handle = self.k.OpenProcess(0x1000, False, pid)
        if not handle:
            return None
        try:
            code = wintypes.DWORD()
            if not self.k.GetExitCodeProcess(handle, ctypes.byref(code)) or code.value != 259:
                return None
            times = [wintypes.FILETIME() for _ in range(4)]
            path = ctypes.create_unicode_buffer(32768)
            length = wintypes.DWORD(len(path))
            if not self.k.GetProcessTimes(handle, *(ctypes.byref(t) for t in times)):
                raise ctypes.WinError(ctypes.get_last_error())
            if not self.k.QueryFullProcessImageNameW(handle, 0, path, ctypes.byref(length)):
                raise ctypes.WinError(ctypes.get_last_error())
            return {'pid': pid, 'creation_time': times[0].dwHighDateTime << 32 | times[0].dwLowDateTime,
                    'image': str(pathlib.Path(path.value).resolve()).casefold()}
        finally:
            self.k.CloseHandle(handle)

    def foreground(self):
        return int(self.u.GetForegroundWindow() or 0)

    def snapshot(self, pid, hwnd):
        if not hwnd or not self.u.IsWindow(hwnd):
            return None
        owner = wintypes.DWORD()
        self.u.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        rect, client = wintypes.RECT(), wintypes.RECT()
        if not self.u.GetWindowRect(hwnd, ctypes.byref(rect)) or not self.u.GetClientRect(hwnd, ctypes.byref(client)):
            raise ctypes.WinError(ctypes.get_last_error())
        style = self.u.GetWindowLongW(hwnd, -16)
        # Require a decorated resizable window. Borderless/fullscreen is excluded.
        windowed = (style & 0x00C40000) == 0x00C40000
        monitor = MonitorInfo()
        monitor.cbSize = ctypes.sizeof(monitor)
        if not self.u.GetMonitorInfoW(self.u.MonitorFromWindow(hwnd, 2), ctypes.byref(monitor)):
            raise ctypes.WinError(ctypes.get_last_error())
        bounds = monitor.rcMonitor
        if (rect.left <= bounds.left and rect.top <= bounds.top
                and rect.right >= bounds.right and rect.bottom >= bounds.bottom):
            windowed = False
        return {'identity': self.identity(pid), 'window_pid': owner.value,
                'visible': bool(self.u.IsWindowVisible(hwnd)), 'minimized': bool(self.u.IsIconic(hwnd)),
                'maximized': bool(self.u.IsZoomed(hwnd)), 'windowed': windowed,
                'rect': [rect.left, rect.top, rect.right, rect.bottom],
                'client_size': [client.right - client.left, client.bottom - client.top]}

    def resize(self, hwnd, width, height):
        return bool(self.u.SetWindowPos(hwnd, None, 0, 0, width, height, SWP_FLAGS))
