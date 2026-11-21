"""Hand off an existing hidden worker without stopping its game or XR host."""
import argparse
import ctypes
from ctypes import wintypes as w
import json
from pathlib import Path
import subprocess
import sys
import time

import isolated_sim as isolated
from runtime_receipts import GameReceipt, read_json, wait_launcher


def show_windows(selected, known, control):
    game = known.observe(selected, control)
    if not game or control.identity(game['pid']) != game:
        return
    allowed = {game['pid']}
    state = known.index or {}
    report_path = state.get('session_report')
    report = read_json(Path(report_path)) if report_path else {}
    xr = (report or {}).get('xr_identity')
    if xr and control.identity(xr['pid']) == xr:
        allowed.add(xr['pid'])
    user = ctypes.WinDLL('user32', use_last_error=True)
    dwm = ctypes.WinDLL('dwmapi', use_last_error=True)
    user.GetWindowThreadProcessId.argtypes = [w.HWND, ctypes.POINTER(w.DWORD)]
    user.GetWindowTextW.argtypes = [w.HWND, w.LPWSTR, ctypes.c_int]
    user.GetWindowLongPtrW.argtypes = [w.HWND, ctypes.c_int]
    user.GetWindowLongPtrW.restype = ctypes.c_ssize_t
    user.SetWindowLongPtrW.argtypes = [w.HWND, ctypes.c_int, ctypes.c_ssize_t]
    user.SetWindowLongPtrW.restype = ctypes.c_ssize_t
    user.ShowWindow.argtypes = [w.HWND, ctypes.c_int]
    user.IsWindowVisible.argtypes = [w.HWND]
    user.IsIconic.argtypes = [w.HWND]
    user.GetWindowRect.argtypes = [w.HWND, ctypes.POINTER(w.RECT)]
    user.SetWindowPos.argtypes = [w.HWND, w.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, w.UINT]
    dwm.DwmGetWindowAttribute.argtypes = [w.HWND, w.DWORD, ctypes.c_void_p, w.DWORD]
    dwm.DwmSetWindowAttribute.argtypes = [w.HWND, w.DWORD, ctypes.c_void_p, w.DWORD]
    callback = ctypes.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    observations = []

    @callback
    def visit(hwnd, unused):
        pid = w.DWORD()
        user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value not in allowed:
            return True
        title = ctypes.create_unicode_buffer(1024)
        user.GetWindowTextW(hwnd, title, len(title))
        if title.value != 'Gears of War: Reloaded' and not title.value.startswith('OpenXR Simulator'):
            return True
        style = user.GetWindowLongPtrW(hwnd, -20)
        desired = (style & ~0x80) | 0x40000  # normal taskbar button, not tool window
        if desired != style:
            user.SetWindowLongPtrW(hwnd, -20, desired)
        cloaked = w.DWORD()
        dwm.DwmGetWindowAttribute(hwnd, 14, ctypes.byref(cloaked), ctypes.sizeof(cloaked))
        if cloaked.value:
            visible = w.BOOL(False)
            result = dwm.DwmSetWindowAttribute(hwnd, 13, ctypes.byref(visible), ctypes.sizeof(visible))
        else:
            result = 0
        if not user.IsWindowVisible(hwnd):
            user.ShowWindow(hwnd, 4)  # show without taking keyboard focus
        if (selected / 'taskbar-only').is_file():
            # A minimized startup window has no usable client size for the
            # real VR startup panel. Keep the actual window off all monitors
            # with its normal taskbar button and valid render dimensions.
            # No activation or periodic z-order changes are needed.
            off_x=user.GetSystemMetrics(76)+user.GetSystemMetrics(78)+128
            off_y=user.GetSystemMetrics(77)
            rect=w.RECT();user.GetWindowRect(hwnd,ctypes.byref(rect))
            if rect.left!=off_x or rect.top!=off_y or user.IsIconic(hwnd):
                user.SetWindowPos(hwnd,None,off_x,off_y,0,0,0x15)
                if user.IsIconic(hwnd):user.ShowWindow(hwnd,4)
        after = w.DWORD()
        dwm.DwmGetWindowAttribute(hwnd, 14, ctypes.byref(after), ctypes.sizeof(after))
        final_rect=w.RECT();user.GetWindowRect(hwnd,ctypes.byref(final_rect))
        observations.append(dict(pid=pid.value, hwnd=int(hwnd), title=title.value,
                                 cloaked_before=cloaked.value, cloaked_after=after.value,
                                 uncloak_result=result, visible=bool(user.IsWindowVisible(hwnd)),
                                 minimized=bool(user.IsIconic(hwnd)),
                                 rect=[final_rect.left,final_rect.top,final_rect.right,final_rect.bottom],
                                 taskbar_button=bool(desired&0x40000) and not bool(desired&0x80)))
        return True

    user.EnumWindows(visit, 0)
    isolated.write(selected / 'visible-windows.json', observations)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', required=True)
    parser.add_argument('--worker', action='store_true')
    parser.add_argument('--once', action='store_true')
    args = parser.parse_args()
    selected = isolated.instance(args.name)
    sys.path.insert(0, str(isolated.ROOT / 'tools/native-capture'))
    from run_stereo_continuous import WindowsControl
    control = WindowsControl()
    if args.once:
        show_windows(selected, GameReceipt(), control)
        print((selected / 'visible-windows.json').read_text())
        return
    receipt_path = selected / 'isolated-worker.json'
    old = read_json(receipt_path)
    index = read_json(selected / 'launcher-session.json')
    owner = (index or {}).get('owner_identity')
    if not owner or control.identity(owner['pid']) != owner:
        raise RuntimeError('Exact existing launcher is not running')
    ready = selected / 'visible-worker-ready.json'
    if args.worker:
        own = control.identity(__import__('os').getpid())
        isolated.write(ready, {'worker_identity': own, 'launcher_identity': owner})
        # Wait until the parent transfers ownership and retires only the old
        # Python watchdog. Its game and launcher processes remain untouched.
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            current = read_json(receipt_path) or {}
            previous = (current.get('visibility_handoff') or {}).get('previous_worker_identity')
            if current.get('worker_identity') == own and previous and control.identity(previous['pid']) != previous:
                break
            time.sleep(.1)
        else:
            raise RuntimeError('Visible worker handoff was not completed')
        known = GameReceipt()
        while control.identity(owner['pid']) == owner:
            show_windows(selected, known, control)
            time.sleep(.25)
        # Preserve the original owned cleanup sequence after normal launcher stop.
        from hidden_children import finish
        while True:
            absent = isolated.owned_windows(selected, close=True, known=known)
            children = finish(selected, control, known.game) if known.game else {'unknown': True}
            result = dict(owned_game_absent=absent, owned_children=children,
                          forced_termination=False, visible_worker=True)
            if absent is True and not children.get('remaining') and not children.get('unknown'):
                isolated.write(selected / 'isolated-done.json', result)
                return
            isolated.write(selected / 'isolated-cleanup-pending.json', result)
            time.sleep(.25)
    previous = (old or {}).get('worker_identity')
    if not previous or control.identity(previous['pid']) != previous:
        raise RuntimeError('Exact previous watchdog is not running')
    with (selected / 'visible-worker.log').open('x', encoding='utf-8') as stream:
        child = subprocess.Popen([sys.executable, '-B', str(Path(__file__).resolve()),
                                  '--name', args.name, '--worker'], cwd=isolated.ROOT,
                                 stdin=subprocess.DEVNULL, stdout=stream, stderr=subprocess.STDOUT,
                                 creationflags=subprocess.CREATE_NO_WINDOW)
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        response = read_json(ready) or {}
        identity = control.identity(child.pid)
        if identity and response.get('worker_identity') == identity and response.get('launcher_identity') == owner:
            break
        if child.poll() is not None:
            raise RuntimeError('Visible watchdog failed; original watchdog remains running')
        time.sleep(.1)
    else:
        raise RuntimeError('Visible watchdog not ready; original remains running')
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    kernel.OpenProcess.restype = w.HANDLE
    kernel.TerminateProcess.argtypes = [w.HANDLE, w.UINT]
    kernel.CloseHandle.argtypes = [w.HANDLE]
    handle = kernel.OpenProcess(0x100001, False, previous['pid'])
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        if control.identity(previous['pid']) != previous or control.identity(owner['pid']) != owner:
            raise RuntimeError('Identity changed; handoff refused')
        updated = dict(old, worker_pid=child.pid, worker_identity=identity,
                       visibility_handoff=dict(previous_worker_identity=previous,
                                               launcher_identity=owner, mode='desktop-and-taskbar'))
        isolated.write(receipt_path, updated)
        # This retires only the old visibility/cleanup wrapper, never the
        # launcher, Native owner, game or XR process. The new wrapper owns cleanup.
        if not kernel.TerminateProcess(handle, 0):
            isolated.write(receipt_path, old)
            raise ctypes.WinError(ctypes.get_last_error())
    finally:
        kernel.CloseHandle(handle)
    print(json.dumps({'instance': args.name, 'visible_worker_pid': child.pid,
                      'launcher_kept_running': owner, 'game_and_xr_not_restarted': True}))


if __name__ == '__main__':
    main()
