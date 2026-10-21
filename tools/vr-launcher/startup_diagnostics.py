"""Bounded startup receipts. Observation must never change launch or cleanup."""
import ctypes
from ctypes import wintypes
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys
import time

VERSION = '0.0.2'
REVISION = 'startup-diagnostics-1'
MAX_TRACE_BYTES = 128 * 1024
CONTEXT_FILES = ('GearsVR Launcher.exe', 'tools/vr-launcher/launcher.py',
                 'tools/vr-launcher/startup_diagnostics.py', 'tools/vr-launcher/diagnostics_bundle.py',
                 'tools/native-capture/launch_reserve.py', 'tools/debug-launch/Start-DebugGame.ps1',
                 'tools/debug-launch/game_install.py', 'tools/debug-launch/background_launch.py',
                 'tools/debug-launch/quiet_launch.py')
PHASES = ('launcher_subprocess', 'reservation', 'game_validation', 'selftests', 'launch_script',
          'loader_dependencies', 'dll_load', 'reserve_trampolines', 'reserve_mask',
          'entry_verification', 'target_priming', 'prime_readiness', 'reservation_complete')


def context(root, manifest, *, include_hashes=True):
    root = Path(root)
    hashes = {}
    for name in CONTEXT_FILES if include_hashes else ():
        try:
            path = root / name
            if path.is_file() and path.stat().st_size <= 2 * 1024 * 1024:
                hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
        except OSError:
            pass
    return dict(version=VERSION, diagnostics_revision=REVISION,
                python_version='.'.join(map(str, sys.version_info[:3])), python_bits=struct.calcsize('P') * 8,
                bundled_python=Path(sys.executable).resolve().is_relative_to((root / 'runtime/python').resolve()),
                runtime_selection='override' if os.environ.get('XR_RUNTIME_JSON') else 'system',
                steam_app_id='2523720', steam_game_id='2523720', steam_environment_scope='child_process',
                script_hashes=hashes,
                artifacts={key:manifest[key] for key in ('native_sha256', 'xr_sha256', 'game_sha256') if key in manifest})


class ProcessObservation:
    """Hold a query-only handle to the born process, including after its exit."""
    def __init__(self, identity):
        self.handle = None
        self.pid = identity['pid']
        self.state = 'unavailable'
        self.error = None
        self.kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        k = self.kernel
        k.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        k.OpenProcess.restype = wintypes.HANDLE
        k.CloseHandle.argtypes = [wintypes.HANDLE]
        k.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
        k.GetProcessTimes.restype = wintypes.BOOL
        k.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
        k.WaitForSingleObject.restype = wintypes.DWORD
        k.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
        k.GetExitCodeProcess.restype = wintypes.BOOL
        handle = k.OpenProcess(0x101000, False, self.pid)  # SYNCHRONIZE | QUERY_LIMITED_INFORMATION
        if not handle:
            self.error = ctypes.get_last_error()
            return
        self.handle = handle
        created, exited, system, user = (wintypes.FILETIME() for _ in range(4))
        if not k.GetProcessTimes(handle, ctypes.byref(created), ctypes.byref(exited),
                                 ctypes.byref(system), ctypes.byref(user)):
            self.error = ctypes.get_last_error()
            self.close()
        elif ((created.dwHighDateTime << 32) | created.dwLowDateTime) != identity['creation_time']:
            self.state = 'identity_changed'
            self.close()
        else:
            self.state = 'observed'

    def snapshot(self):
        result = dict(pid=self.pid, state=self.state, same_identity=self.state == 'observed')
        if self.error is not None:
            result['winerror'] = self.error
        if not self.handle:
            return result
        wait = self.kernel.WaitForSingleObject(self.handle, 0)
        if wait not in (0, 258):
            return dict(result, state='unavailable', winerror=ctypes.get_last_error())
        result['state'] = 'exited' if wait == 0 else 'running'
        if wait == 0:
            code = wintypes.DWORD()
            if self.kernel.GetExitCodeProcess(self.handle, ctypes.byref(code)):
                result.update(exit_code=code.value, exit_code_hex=f'0x{code.value:08X}')
            else:
                result['winerror'] = ctypes.get_last_error()
        return result

    def close(self):
        if self.handle:
            self.kernel.CloseHandle(self.handle)
            self.handle = None


def prime_status(path, started_wall_ns, pid):
    """Only typed status; never copy hook addresses, entry bytes or raw lines."""
    try:
        path = Path(path)
        info = path.stat()
        if info.st_mtime_ns < started_wall_ns:
            return {'state':'stale'}
        with path.open('rb') as stream:
            text = stream.read(16384).decode('utf-8', errors='replace')
        match = re.search(r'\bcapturing pid=(\d+)\b', text)
        if match and int(match[1]) != pid:
            return {'state':'pid_mismatch'}
        hooks = [dict(create=a, queue=b) for a, b in re.findall(
            r'\bcreate=(MH_[A-Z_]+) queue=(MH_[A-Z_]+)\b', text)[:16]]
        result = dict(state='captured' if text else 'empty', pid_verified=match is not None,
                      setup_failed='prime_setup_failed' in text, hooks=hooks,
                      truncated=info.st_size > 16384)
        rejected = re.search(r'graphics_prime_admission rejected=1 reason=(\d+) policy_reason=(\d+)', text)
        if rejected:
            result['graphics_rejection'] = dict(reason=int(rejected[1]), policy_reason=int(rejected[2]))
        return result
    except FileNotFoundError:
        return {'state':'missing'}
    except OSError as error:
        return dict(state='unavailable', winerror=getattr(error, 'winerror', None))


class StartupTrace:
    def __init__(self, path, *, append=False, metadata=None, attempt_id=None):
        self.path = Path(path) if path is not None else None
        self.started_ns = time.perf_counter_ns()
        self.started_wall_ns = time.time_ns()
        self.attempt_id = attempt_id
        self.phase = 'reservation'
        self.phase_started_ns = self.started_ns
        self.observation = None
        self.status_path = None
        self.prime_started_wall_ns = None
        if self.path is not None:
            try:
                if append:
                    with self.path.open('r', encoding='utf-8') as stream:
                        first = json.loads(stream.readline(32768))
                    self.started_ns = first['started_ns']
                    self.started_wall_ns = first['started_wall_ns']
                    self.attempt_id = first['attempt_id']
                else:
                    self.path.parent.mkdir(parents=True, exist_ok=True)
                    with self.path.open('x', encoding='utf-8') as stream:
                        row = dict(schema=1, kind='startup_diagnostic', event='context', phase='launcher_subprocess',
                                   attempt_id=attempt_id, started_ns=self.started_ns,
                                   started_wall_ns=self.started_wall_ns, context=metadata or {})
                        stream.write(json.dumps(row) + '\n')
            except Exception:
                self.path = None

    def emit(self, event, **values):
        if self.path is None:
            return
        try:
            row = dict(schema=1, kind='startup_diagnostic', event=event, phase=self.phase,
                       timestamp_utc=dt.datetime.now(dt.timezone.utc).isoformat(),
                       elapsed_ms=round((time.perf_counter_ns() - self.started_ns) / 1e6, 3), **values)
            raw = json.dumps(row, ensure_ascii=True) + '\n'
            if self.path.stat().st_size + len(raw) <= MAX_TRACE_BYTES:
                with self.path.open('a', encoding='utf-8') as stream:
                    stream.write(raw)
        except Exception:
            pass

    def begin(self, phase):
        self.phase = phase
        self.phase_started_ns = time.perf_counter_ns()
        if phase == 'target_priming':
            self.prime_started_wall_ns = time.time_ns()
        self.emit('begin')

    def end(self, **values):
        self.emit('complete', duration_ms=round((time.perf_counter_ns() - self.phase_started_ns) / 1e6, 3), **values)

    def watch(self, identity):
        if self.path is None or not isinstance(identity, dict):
            return
        try:
            self.observation = ProcessObservation(identity)
            self.emit('game_birth', process=self.observation.snapshot())
        except Exception as error:
            self.emit('observation_unavailable', error=repr(error)[:1500])

    def failure(self, error):
        values = dict(error=repr(error)[:4096], exception_type=type(error).__name__,
                      winerror=getattr(error, 'winerror', None))
        try:
            if self.observation:
                values['process'] = self.observation.snapshot()
                if self.status_path and self.prime_started_wall_ns is not None:
                    values['prime_status'] = prime_status(self.status_path, self.prime_started_wall_ns, self.observation.pid)
        except Exception:
            values['process'] = {'state':'unavailable'}
        self.emit('failure', **values)

    def close(self):
        if self.observation:
            try:
                self.observation.close()
            except Exception:
                pass


def read_trace(path):
    """Read complete bounded rows, including when the child was timed out."""
    try:
        with Path(path).open('rb') as stream:
            raw = stream.read(MAX_TRACE_BYTES)
        rows = []
        for line in raw.splitlines(keepends=True):
            if not line.endswith(b'\n') or len(line) > 32768:
                continue
            try:
                row = json.loads(line)
                if isinstance(row, dict) and row.get('schema') == 1 and row.get('kind') == 'startup_diagnostic':
                    rows.append(row)
            except (ValueError, UnicodeError):
                pass
        return rows[-96:]
    except OSError:
        return []
