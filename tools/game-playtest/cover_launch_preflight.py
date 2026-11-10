"""Read-only startup check for the existing private cover observation entry.

The launcher still creates and owns its existing owner mutex. This check does
not reserve a future window, grant gameplay permission, stage files or stop any
process. Call immediately before staging and again before launching.
"""
import ctypes
from ctypes import wintypes
import json
import subprocess


OWNER_LOCKS = (
    r'Local\GearsVR.RepositoryLauncher.Owner',
    r'Local\GearsVR.RepositoryLauncher.Owner.solo-m1-1028d',
)
RUNTIME_NAMES = frozenset((
    'gowde-steam.exe', 'openxrposeprobe.exe', 'gearspairedwindowrecord.exe',
))
# These are existing shared-instance runners, not offline encoders/reviewers.
RUNNERS = (
    'run_basic.py', 'run_v63_startup_repro.py', 'sim_entry.py',
    'run_menu_firstperson_check.py', 'run_cover_eye_observation_operator.py',
)


class Occupied(RuntimeError):
    pass


def blocking_processes(rows, own_pid):
    if not isinstance(rows, list):
        raise ValueError('Process inventory must be a complete list')
    result = []
    seen = set()
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError('Malformed process row')
        pid, name = row.get('ProcessId'), row.get('Name')
        if type(pid) is not int or pid <= 0 or not isinstance(name, str) or not name:
            raise ValueError('Missing process identity')
        if pid in seen:
            raise ValueError('Duplicate process identity')
        seen.add(pid)
        if pid == own_pid:
            continue
        name = name.casefold()
        command = row.get('CommandLine')
        if command is not None and not isinstance(command, str):
            raise ValueError('Malformed command line')
        if name in RUNTIME_NAMES:
            result.append(row)
        elif name == 'python.exe':
            # The sealed observation/stage/start interpreters are python.exe;
            # unrelated pythonw background apps are not this runner chain.
            # An inaccessible Python command could be a shared runner. Unknown
            # is not an idle certificate; ordinary known offline jobs are fine.
            if command is None or any(script in command.casefold() for script in RUNNERS):
                result.append(row)
    return result


def process_inventory():
    script = (
        "$ErrorActionPreference='Stop'; "
        "[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false); "
        # PID 0 is Windows' idle accounting row, not an executable owner.
        "$p=@(Get-CimInstance Win32_Process | Where-Object {$_.ProcessId -gt 0} | "
        "Select-Object Name,ProcessId,CommandLine); "
        "ConvertTo-Json -InputObject $p -Compress -Depth 3"
    )
    completed = subprocess.run(
        ['powershell', '-NoProfile', '-NonInteractive', '-Command', script],
        check=True, capture_output=True, encoding='utf-8', timeout=15,
        creationflags=subprocess.CREATE_NO_WINDOW,
    )
    return json.loads(completed.stdout.lstrip('\ufeff'))


def owner_mutex_present(name):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenMutexW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
    kernel.OpenMutexW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    ctypes.set_last_error(0)
    handle = kernel.OpenMutexW(0x100000, False, name)
    if not handle:
        error = ctypes.get_last_error()
        if error == 2:  # ERROR_FILE_NOT_FOUND, never access denied or query error
            return False
        raise ctypes.WinError(error)
    try:
        return True
    finally:
        if not kernel.CloseHandle(handle):
            raise ctypes.WinError(ctypes.get_last_error())


def check(own_pid, *, inventory=process_inventory, mutex=owner_mutex_present):
    if type(own_pid) is not int or own_pid <= 0:
        raise ValueError('Exact current caller PID required')
    before = [name for name in OWNER_LOCKS if mutex(name)]
    if before:
        raise Occupied('Existing launcher owner mutex is present: ' + ', '.join(before))
    blockers = blocking_processes(inventory(), own_pid)
    after = [name for name in OWNER_LOCKS if mutex(name)]
    if blockers or after:
        raise Occupied(json.dumps(dict(processes=blockers, owner_mutexes=after)))
    return dict(
        runtime_idle_observed=True,
        existing_owner_mutexes_absent=True,
        reservation_created=False,
        runtime_window_owned=False,
        stage_or_launch_performed=False,
        atomic_exclusion_proven=False,
        next='Recheck at stage/start; launcher must acquire its existing owner mutex',
    )
