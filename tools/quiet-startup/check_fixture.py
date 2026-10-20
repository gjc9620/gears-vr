"""Exercise the owned early launcher; observe OS foreground events without input."""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
from pathlib import Path
import re
import subprocess
import time


def inspect_failed_child(pid):
    """Read-only exit verification; denied access is never proof of cleanup."""
    kernel = c.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    kernel.OpenProcess.restype = w.HANDLE
    kernel.WaitForSingleObject.argtypes = [w.HANDLE, w.DWORD]
    kernel.WaitForSingleObject.restype = w.DWORD
    kernel.GetProcessTimes.argtypes = [w.HANDLE] + [c.POINTER(w.FILETIME)] * 4
    kernel.GetProcessTimes.restype = w.BOOL
    kernel.GetExitCodeProcess.argtypes = [w.HANDLE, c.POINTER(w.DWORD)]
    kernel.GetExitCodeProcess.restype = w.BOOL
    kernel.CloseHandle.argtypes = [w.HANDLE]
    handle = kernel.OpenProcess(0x100000 | 0x1000, False, pid)
    if not handle:
        error = c.get_last_error()
        return dict(pid=pid, exited=error == 87, state='pid_absent' if error == 87 else 'unverified', error=error)
    try:
        created, exited, system, user = (w.FILETIME() for _ in range(4))
        if not kernel.GetProcessTimes(handle, c.byref(created), c.byref(exited), c.byref(system), c.byref(user)):
            return dict(pid=pid, exited=False, state='unverified', error=c.get_last_error())
        wait = kernel.WaitForSingleObject(handle, 0)
        code = w.DWORD()
        queried = kernel.GetExitCodeProcess(handle, c.byref(code))
        return dict(pid=pid, creation_time=(created.dwHighDateTime << 32) | created.dwLowDateTime,
                    exited=wait == 0, state='signaled' if wait == 0 else 'still_present',
                    exit_code=code.value if queried else None)
    finally:
        kernel.CloseHandle(handle)


def check(build, output, *, missing_policy=False, invalid_policy=False):
    if missing_policy and invalid_policy:
        raise ValueError('Select one policy failure mode')
    failure_mode = missing_policy or invalid_policy
    user = c.WinDLL('user32', use_last_error=True)
    user.GetForegroundWindow.restype = w.HWND
    user.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
    user.GetClassNameW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
    user.GetClassNameW.restype = c.c_int
    kernel = c.WinDLL('kernel32', use_last_error=True)
    kernel.GetTickCount64.restype = c.c_ulonglong
    callback_type = c.WINFUNCTYPE(None, w.HANDLE, w.DWORD, w.HWND, c.c_long, c.c_long, w.DWORD, w.DWORD)
    user.SetWinEventHook.argtypes = [w.DWORD, w.DWORD, w.HMODULE, callback_type, w.DWORD, w.DWORD, w.DWORD]
    user.SetWinEventHook.restype = w.HANDLE
    user.UnhookWinEvent.argtypes = [w.HANDLE]
    user.PeekMessageW.argtypes = [c.POINTER(w.MSG), w.HWND, w.UINT, w.UINT, w.UINT]
    events = []
    @callback_type
    def callback(_hook, _event, window, _object, _child, _thread, stamp):
        pid = w.DWORD()
        user.GetWindowThreadProcessId(window, c.byref(pid))
        foreground = user.GetForegroundWindow()
        foreground_pid = w.DWORD()
        user.GetWindowThreadProcessId(foreground, c.byref(foreground_pid))
        event_class = c.create_unicode_buffer(256)
        user.GetClassNameW(window, event_class, len(event_class))
        # Rich fields are persisted only for fixture-owned/known fixture HWNDs.
        # Global events below retain just PID, HWND and the original event time.
        events.append(dict(pid=pid.value, hwnd=int(window or 0), time_ms=stamp,
                           callback_tick_ms=kernel.GetTickCount64(),
                           callback_foreground_hwnd=int(foreground or 0),
                           callback_foreground_pid=foreground_pid.value, event_class=event_class.value))
    hook = user.SetWinEventHook(3, 3, None, callback, 0, 0, 0)
    if not hook:
        raise c.WinError(c.get_last_error())
    fixture = output.with_suffix('.fixture.json')
    if fixture.exists() or output.exists():
        user.UnhookWinEvent(hook)
        raise RuntimeError('Output already exists; preserve previous evidence')
    output.parent.mkdir(parents=True, exist_ok=True)
    policy = build/('MissingQuietStartupPolicy.dll' if missing_policy else
                    'QuietStartupFixture.exe' if invalid_policy else 'GearsQuietStartup.dll')
    if missing_policy and policy.exists():
        user.UnhookWinEvent(hook)
        raise RuntimeError('Missing-policy fixture path unexpectedly exists')
    command = [str(build/'GearsEarlyLaunch.exe'), str(build/'QuietStartupFixture.exe'), str(policy), str(fixture)]
    initial = int(user.GetForegroundWindow() or 0)
    start = time.monotonic()
    child = None
    try:
        child = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 creationflags=subprocess.CREATE_NO_WINDOW, text=True)
        # The launcher exits only after detaching and releasing the fixture.
        deadline = start + 30
        def pump():
            message = w.MSG()
            while user.PeekMessageW(c.byref(message), None, 0, 0, 1):
                user.TranslateMessage(c.byref(message)); user.DispatchMessageW(c.byref(message))
        while time.monotonic() < deadline:
            pump()
            if child.poll() is not None and (failure_mode or fixture.exists()):
                break
            time.sleep(.01)
        if child.poll() is None:
            # Do not kill a debugger mid-transaction or free remote arguments.
            raise TimeoutError('Early-launch helper still running; inspect the live owned handle')
        stdout, stderr = child.communicate()
        for _ in range(10):
            pump(); time.sleep(.01)
        parsed = json.loads(stdout.strip()) if child.returncode == 0 else None
        fixture_result = json.loads(fixture.read_text()) if fixture.exists() else None
        cleanup_lines = re.findall(r'owned_child_cleanup pid=(\d+)([^\r\n]*)', stderr)
        cleanup = [dict(inspect_failed_child(int(pid)), reported_exited=bool(re.search(r'\bexited=1\b', tail)))
                   for pid, tail in cleanup_lines]
        owned_pid = parsed['pid'] if parsed else cleanup[0]['pid'] if len(cleanup) == 1 else None
        owned_events = [e for e in events if e['pid'] == owned_pid] if owned_pid else []
        fixture_handles = set((fixture_result or {}).get('windows', {}).values()) - {0}
        handle_events = [e for e in events if e['hwnd'] in fixture_handles]
        result = dict(launcher_pid=child.pid, exit_code=child.returncode,
                      elapsed_seconds=time.monotonic()-start, launch=parsed,
                      fixture=fixture_result, stderr=stderr, owned_foreground_events=owned_events,
                      fixture_handle_foreground_events=handle_events,
                      foreground_events=[{key:e[key] for key in ('pid','hwnd','time_ms')} for e in events],
                      initial_foreground=initial, final_foreground=int(user.GetForegroundWindow() or 0),
                      manual_activation_tested=False, game_started=False)
        result.update(policy_path=str(policy), failure_mode='missing_file' if missing_policy else
                      'invalid_policy_executable' if invalid_policy else None, failed_child_checks=cleanup)
        if failure_mode:
            # A missing file currently fails canonicalization before child creation.
            # An existing EXE is a distinct post-create invalid-policy injection.
            cleanup_ok = (len(cleanup) == 1 and all(x['reported_exited'] and x['exited'] for x in cleanup))
            missing_rejected = missing_policy and not cleanup and policy.name.casefold() in stderr.casefold()
            result['passed'] = bool(child.returncode != 0 and stderr.strip() and not fixture_result
                                    and not owned_events and (cleanup_ok or missing_rejected))
        else:
            result['passed'] = (child.returncode == 0 and fixture_result is not None
                                and fixture_result['visible'] and fixture_result['foreground_preserved']
                                and fixture_result['temporary_style_removed'] and not fixture_result['topmost']
                                and all(fixture_result.get(key) for key in (
                                    'ansi_visible', 'ansi_temporary_style_removed', 'parent_child_preserved',
                                    'owned_popup_preserved', 'parent_hide_preserved', 'child_visibility_restored',
                                    'explicit_noactivate_preserved', 'hide_preserved',
                                    'show_minimized_preserved', 'restore_preserved', 'minimize_preserved',
                                    'restore_again_preserved', 'status_queried'))
                                and len(fixture_result['suppressed']) == 8
                                and all(value > 0 for value in fixture_result['suppressed'])
                                and not owned_events)
        output.write_text(json.dumps(result, indent=2), encoding='utf-8')
        return result
    finally:
        user.UnhookWinEvent(hook)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    failures = parser.add_mutually_exclusive_group()
    failures.add_argument('--missing-policy', action='store_true', help='Absent DLL path, rejected before child creation')
    failures.add_argument('--invalid-policy', action='store_true', help='Existing EXE as invalid DLL; verify failed child cleanup')
    args = parser.parse_args()
    result = check(args.build.resolve(), args.output.resolve(), missing_policy=args.missing_policy,
                   invalid_policy=args.invalid_policy)
    print(json.dumps(result))
    raise SystemExit(0 if result['passed'] else 1)
