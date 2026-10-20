"""Bounded, read-only desktop window snapshots for autonomous test diagnostics.

Does not activate/show/hide/reorder windows, install hooks, or read their titles.
Sampling is not atomic; invalid samples must never be counted as passing.
"""
import ctypes as c
from ctypes import wintypes as w
import time


class WindowSnapshot:
    def __init__(self):
        self.u = c.WinDLL('user32', use_last_error=True)
        self.d = c.WinDLL('dwmapi')
        signatures = {
            'GetTopWindow': ([w.HWND], w.HWND),
            'GetWindow': ([w.HWND, w.UINT], w.HWND),
            'GetWindowThreadProcessId': ([w.HWND, c.POINTER(w.DWORD)], w.DWORD),
            'IsWindowVisible': ([w.HWND], w.BOOL),
            'IsIconic': ([w.HWND], w.BOOL),
            'GetShellWindow': ([], w.HWND),
            'GetDesktopWindow': ([], w.HWND),
        }
        for name, (args, result) in signatures.items():
            function = getattr(self.u, name)
            function.argtypes, function.restype = args, result
        self.d.DwmGetWindowAttribute.argtypes = [w.HWND, w.DWORD, c.c_void_p, w.DWORD]
        self.d.DwmGetWindowAttribute.restype = c.c_long

    def _order(self, limit):
        handles, seen = [], set()
        hwnd = self.u.GetTopWindow(None)
        while hwnd:
            if hwnd in seen:
                raise RuntimeError('Window order cycle detected')
            if len(handles) >= limit:
                raise RuntimeError('Top-level traversal exceeded snapshot limit')
            seen.add(hwnd)
            handles.append(hwnd)
            hwnd = self.u.GetWindow(hwnd, 2)  # GW_HWNDNEXT
        return handles

    def capture(self, limit=4096, visible_limit=256):
        start = time.monotonic()
        order = self._order(limit)
        excluded = {self.u.GetShellWindow(), self.u.GetDesktopWindow()} - {None, 0}
        rows = []
        for hwnd in order:
            pid = w.DWORD()
            thread = self.u.GetWindowThreadProcessId(hwnd, c.byref(pid))
            if not thread or not pid.value:
                raise RuntimeError('Window identity disappeared during snapshot')
            visible, minimized = bool(self.u.IsWindowVisible(hwnd)), bool(self.u.IsIconic(hwnd))
            if not visible or minimized or hwnd in excluded:
                continue
            cloaked = w.DWORD()
            if self.d.DwmGetWindowAttribute(hwnd, 14, c.byref(cloaked), c.sizeof(cloaked)) != 0:
                raise RuntimeError('Unable to determine DWM cloaking')
            if cloaked.value:
                continue
            if len(rows) >= visible_limit:
                raise RuntimeError('Visible windows exceeded snapshot limit')
            rect = w.RECT()
            # DWMWA_EXTENDED_FRAME_BOUNDS uses screen coordinates, without
            # changing the caller's DPI awareness or moving any window.
            if self.d.DwmGetWindowAttribute(hwnd, 9, c.byref(rect), c.sizeof(rect)) != 0:
                raise RuntimeError('Unable to read DWM window bounds')
            after = w.DWORD()
            if self.u.GetWindowThreadProcessId(hwnd, c.byref(after)) != thread or after.value != pid.value:
                raise RuntimeError('Window identity changed during snapshot')
            rows.append(dict(hwnd=hwnd, pid=pid.value, visible=True, minimized=False,
                             cloaked=False, rect=(rect.left, rect.top, rect.right, rect.bottom)))
        if order != self._order(limit):
            raise RuntimeError('Window order changed during snapshot')
        return dict(windows=rows, excluded_hwnds=sorted(excluded),
                    traversed_windows=len(order),
                    elapsed_seconds=time.monotonic() - start, atomic=False)
