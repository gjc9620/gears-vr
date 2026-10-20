"""Observe attested real swapchain output and attempt one nonactivating resize.

The launcher monitor covers reservation through session exit. An attempt is
diagnostic evidence; fresh subsequent output observations establish its effect.
"""
import ctypes
from ctypes import wintypes
import json
import pathlib
import threading
import time

SWP_FLAGS = 0x0002 | 0x0004 | 0x0010 | 0x0200  # NOMOVE/NOZORDER/NOACTIVATE/NOOWNERZORDER


def _phase(emit, phase, expected, hwnd):
    if emit is not None:
        try:
            emit(dict(event='recovery_stage', phase=phase, target_identity=dict(expected), hwnd=hwnd))
        except Exception:
            # Stage storage is diagnostic only; never gate an existing action.
            pass


def read_swapchain_status(path):
    """Read one bounded atomic native snapshot, never the XR source dimensions."""
    with pathlib.Path(path).open('rb') as stream:
        raw = stream.read(4097)
    if len(raw) > 4096:
        raise ValueError('Swapchain status exceeds bound')
    fields = {}
    for token in raw.decode('ascii').split():
        key, value = token.split('=', 1)
        if key in fields:
            raise ValueError('Duplicate swapchain status field')
        fields[key] = value if key == 'phase' else int(value)
    return fields


class SwapchainTinyOutputPolicy:
    """One attempt per launch, using distinct fresh real-output observations."""
    def __init__(self, expected):
        self.expected = dict(expected)
        self.consumed = False
        self.reset()

    def reset(self):
        self.key = self.first_ns = self.last_ns = None

    def observe(self, fields, now_ns):
        def reject(reason):
            self.reset()
            return dict(ready=False, reason=reason)
        if not isinstance(fields, dict) or fields.get('schema') != 1 or fields.get('complete') != 1:
            return reject('invalid_status')
        if (fields.get('pid') != self.expected['pid'] or
                fields.get('creation_time') != self.expected['creation_time']):
            return reject('status_identity_mismatch')
        if fields.get('phase') not in ('prime', 'running') or fields.get('valid') != 1 or fields.get('candidates') != 1:
            return reject('output_unavailable')
        if type(fields.get('windowed')) is not int or fields['windowed'] != 1:
            return reject('dxgi_not_attested_windowed')
        numeric = ('hwnd', 'chain', 'width', 'height', 'qpc', 'qpc_frequency')
        if any(type(fields.get(key)) is not int or fields[key] <= 0 for key in numeric):
            return reject('invalid_output_fields')
        sampled_ns = fields['qpc'] * 1_000_000_000 // fields['qpc_frequency']
        if not 0 <= now_ns - sampled_ns <= 2_000_000_000:
            return reject('status_stale_or_future')
        dimensions = [fields['width'], fields['height']]
        if self.consumed:
            return dict(ready=False, reason='output_after_attempt', dimensions=dimensions,
                        hwnd=fields['hwnd'], sample_qpc=fields['qpc'])
        if any(value > 8 for value in dimensions):
            self.reset()
            return dict(ready=False, reason='output_not_tiny', dimensions=dimensions)
        key = (fields['hwnd'], fields['chain'], fields['qpc_frequency'])
        if (key != self.key or self.last_ns is None or sampled_ns < self.last_ns or
                sampled_ns - self.last_ns > 2_000_000_000):
            self.key, self.first_ns, self.last_ns = key, sampled_ns, sampled_ns
        elif sampled_ns == self.last_ns:
            return dict(ready=False, reason='duplicate_sample')
        else:
            self.last_ns = sampled_ns
        duration = sampled_ns - self.first_ns
        # Normal startup can briefly publish 8x8 output before natural recovery.
        return dict(ready=duration >= 5_000_000_000, reason='persistent_tiny' if duration >= 5_000_000_000 else 'observing_tiny',
                    observed_tiny_seconds=duration / 1_000_000_000, dimensions=dimensions,
                    hwnd=fields['hwnd'], chain=fields['chain'], sample_qpc=fields['qpc'])


class TinyOutputMonitor:
    """Independent best-effort launcher observer, including the pre-level wait."""
    def __init__(self, status_path, diagnostic_path, expected, control_factory=None,
                 attempt=None, interval=.2, *, allow_recovery=True):
        self.status_path, self.diagnostic_path = pathlib.Path(status_path), pathlib.Path(diagnostic_path)
        self.expected, self.policy = dict(expected), SwapchainTinyOutputPolicy(expected)
        self.control_factory, self.attempt = control_factory, attempt
        self.interval = interval
        self.allow_recovery = allow_recovery
        self.stop_event = threading.Event()
        self.thread = None

    def poll_once(self, control, now_ns, emit=None):
        if control.identity(self.expected['pid']) != self.expected:
            self.policy.reset()
            return dict(event='identity_changed', stop=True)
        try:
            fields = read_swapchain_status(self.status_path)
        except OSError as error:
            # Atomic replacement can briefly deny a read on Windows. Retain
            # only the previous verified sequence within its existing 2s gap
            # bound. This poll never authorizes action; observe() still needs
            # a distinct fresh sample before reaching the recovery branch.
            retained = self.policy.last_ns is not None and 0 <= now_ns - self.policy.last_ns <= 2_000_000_000
            if not retained:
                self.policy.reset()
            return dict(event='status_unavailable', error=repr(error), continuity_retained=retained)
        except (ValueError, UnicodeError) as error:
            self.policy.reset()
            return dict(event='status_unavailable', error=repr(error), continuity_retained=False)
        observation = self.policy.observe(fields, now_ns)
        if not observation['ready']:
            return dict(event='observation', **observation)
        if self.stop_event.is_set():
            return dict(event='monitor_cancelled', stop=True)
        if not self.allow_recovery:
            return dict(event='observation', ready=False, reason='recovery_disabled',
                        dimensions=observation['dimensions'])
        # A hidden/minimized/unusable startup HWND is not an attempt. Once it
        # becomes actionable require a fresh full tiny interval before acting.
        _phase(emit, 'window_snapshot_begin', self.expected, observation['hwnd'])
        state = control.snapshot(self.expected['pid'], observation['hwnd'])
        _phase(emit, 'window_snapshot_end', self.expected, observation['hwnd'])
        reason = skip_reason(observation['dimensions'], self.expected, state)
        if reason:
            self.policy.reset()
            return dict(event='recovery_deferred', reason=reason, observation=observation, window=state)
        # Consume before action, including skipped or failed attempts. A fresh
        # identity/window check also happens inside the nonactivating helper.
        self.policy.consumed = True
        _phase(emit, 'recovery_action_begin', self.expected, observation['hwnd'])
        try:
            if self.attempt:
                result = self.attempt(control, self.expected, observation['hwnd'], observation['dimensions'])
            else:
                result = attempt_recovery(control, self.expected, observation['hwnd'], observation['dimensions'], emit=emit)
        except Exception as error:
            result = dict(attempted=False, result='diagnostic_error', error=repr(error))
        _phase(emit, 'recovery_action_complete', self.expected, observation['hwnd'])
        return dict(event='recovery_attempt', observation=observation, result=result)

    def _run(self):
        try:
            self.diagnostic_path.parent.mkdir(parents=True, exist_ok=True)
            with self.diagnostic_path.open('a', encoding='utf-8') as stream:
                def record(value):
                    stream.write(json.dumps(dict(observed_ns=time.perf_counter_ns(), identity=self.expected, **value)) + '\n')
                    stream.flush()
                record(dict(event='monitor_started', status_path=str(self.status_path)))
                try:
                    control = (self.control_factory or WindowsControl)()
                    previous = None
                    while not self.stop_event.is_set():
                        try:
                            event = self.poll_once(control, time.perf_counter_ns(), emit=record)
                        except Exception as error:
                            self.policy.reset()
                            event = dict(event='observer_error', error=repr(error))
                        signature = (event['event'], event.get('reason'), event.get('error'), tuple(event.get('dimensions', [])))
                        if event.get('reason') == 'duplicate_sample':
                            pass
                        elif signature != previous or event['event'] == 'recovery_attempt':
                            record(event)
                            previous = signature
                        if event.get('stop'):
                            break
                        self.stop_event.wait(self.interval)
                except Exception as error:
                    record(dict(event='monitor_error', error=repr(error)))
                finally:
                    record(dict(event='monitor_stopped', consumed=self.policy.consumed))
        except Exception:
            # Diagnostics/storage failures cannot interrupt the owning launcher.
            pass

    def start(self):
        self.thread = threading.Thread(target=self._run, name='GearsVR tiny output observer', daemon=True)
        self.thread.start()
        return self

    def close(self, timeout=2.):
        self.stop_event.set()
        if self.thread:
            self.thread.join(timeout)
        return self.thread is None or not self.thread.is_alive()


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


def attempt_recovery(control, expected, hwnd, observed_output, *, emit=None):
    """Return evidence only; caller owns once-per-session policy and DXGI sampling."""
    result = {'hwnd': hwnd, 'observed_output': observed_output, 'attempted': False,
              'resized': False, 'restored': False, 'foreground_before': None,
              'foreground_after': None, 'result': 'skipped'}
    try:
        result['foreground_before'] = control.foreground()
        _phase(emit, 'attempt_snapshot_before_begin', expected, hwnd)
        state = control.snapshot(expected['pid'], hwnd)
        _phase(emit, 'attempt_snapshot_before_end', expected, hwnd)
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
        _phase(emit, 'resize_begin', expected, hwnd)
        result['resized'] = bool(control.resize(hwnd, width - 1, height))
        _phase(emit, 'resize_end', expected, hwnd)
        _phase(emit, 'attempt_snapshot_after_resize_begin', expected, hwnd)
        after = control.snapshot(expected['pid'], hwnd)
        _phase(emit, 'attempt_snapshot_after_resize_end', expected, hwnd)
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
            _phase(emit, 'restore_begin', expected, hwnd)
            result['restored'] = bool(control.resize(hwnd, width, height))
            _phase(emit, 'restore_end', expected, hwnd)
            result['result'] = 'resize_restore_attempted' if result['restored'] else 'restore_failed'
            _phase(emit, 'attempt_snapshot_after_restore_begin', expected, hwnd)
            result['after_restore'] = control.snapshot(expected['pid'], hwnd)
            _phase(emit, 'attempt_snapshot_after_restore_end', expected, hwnd)
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
        # SetWindowPos does not require WS_THICKFRAME. The observed game window
        # has caption+system menu; native DXGI Windowed separately excludes
        # exclusive fullscreen. Outer borders may extend beyond monitor bounds.
        windowed = (style & 0x00C80000) == 0x00C80000
        return {'identity': self.identity(pid), 'window_pid': owner.value,
                'visible': bool(self.u.IsWindowVisible(hwnd)), 'minimized': bool(self.u.IsIconic(hwnd)),
                'maximized': bool(self.u.IsZoomed(hwnd)), 'windowed': windowed,
                'rect': [rect.left, rect.top, rect.right, rect.bottom],
                'client_size': [client.right - client.left, client.bottom - client.top]}

    def resize(self, hwnd, width, height):
        return bool(self.u.SetWindowPos(hwnd, None, 0, 0, width, height, SWP_FLAGS))
