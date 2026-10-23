"""Local file-queue service for sparse HWND screenshots and bounded virtual-pad input.

No desktop capture, keyboard injection, network listener, or game-memory writes.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import datetime
import json
import math
import os
import pathlib
import subprocess
import sys
import time
import uuid

ROOT = pathlib.Path(__file__).resolve().parents[2]
HERE = pathlib.Path(__file__).resolve().parent
STATE = ROOT / 'analysis/game-io'
GAME = ROOT / 'Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe'
user = c.WinDLL('user32', use_last_error=True)
kernel = c.WinDLL('kernel32', use_last_error=True)
user.GetForegroundWindow.restype = w.HWND
user.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
user.IsWindowVisible.argtypes = [w.HWND]
user.IsIconic.argtypes = [w.HWND]
user.ShowWindow.argtypes = [w.HWND, c.c_int]
user.SetForegroundWindow.argtypes = [w.HWND]
user.GetAsyncKeyState.argtypes = [c.c_int]
user.GetAsyncKeyState.restype = c.c_short
kernel.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
kernel.OpenProcess.restype = w.HANDLE
kernel.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)]
kernel.CloseHandle.argtypes = [w.HANDLE]
CALLBACK = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
user.EnumWindows.argtypes = [CALLBACK, w.LPARAM]


def game_window():
    windows = []
    @CALLBACK
    def visit(hwnd, _):
        if not user.IsWindowVisible(hwnd):
            return True
        pid = w.DWORD()
        user.GetWindowThreadProcessId(hwnd, c.byref(pid))
        handle = kernel.OpenProcess(0x1000, False, pid.value)
        if handle:
            try:
                text = c.create_unicode_buffer(32768)
                length = w.DWORD(len(text))
                if kernel.QueryFullProcessImageNameW(handle, 0, text, c.byref(length)):
                    if os.path.normcase(text.value) == os.path.normcase(str(GAME)):
                        windows.append((pid.value, int(hwnd)))
            finally:
                kernel.CloseHandle(handle)
        return True
    user.EnumWindows(visit, 0)
    if not windows:
        raise RuntimeError('Game window not found')
    # Prefer the actual large client area over secondary helper windows.
    user.GetClientRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
    def area(item):
        rect = w.RECT(); user.GetClientRect(item[1], c.byref(rect))
        return rect.right * rect.bottom
    return max(windows, key=area)


def focused(hwnd):
    return int(user.GetForegroundWindow() or 0) == hwnd and not user.IsIconic(hwnd)


def interrupted():
    return (STATE / 'stop').exists() or bool(user.GetAsyncKeyState(0x77) & 0x8000)  # F8


def focus_game():
    pid, hwnd = game_window()
    if not focused(hwnd):
        if user.IsIconic(hwnd):
            user.ShowWindow(hwnd, 9)  # SW_RESTORE
        user.SetForegroundWindow(hwnd)
        time.sleep(0.2)
    if not focused(hwnd):
        raise RuntimeError('Windows refused foreground activation; click the game once')
    return {'pid': pid, 'focused': True}


def atomic_json(path, data):
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
    temporary.replace(path)


def snapshot(request_id, width=640):
    from PIL import Image, ImageStat
    pid, hwnd = game_window()
    image = STATE / 'frames' / f'{request_id}.jpg'
    image.parent.mkdir(parents=True, exist_ok=True)
    result = subprocess.run([str(HERE / 'build/WindowCapture.exe'), str(hwnd), str(image), str(width)],
                            capture_output=True, text=True, timeout=9)
    if result.returncode:
        raise RuntimeError(result.stderr.strip())
    details = json.loads(result.stdout)
    with Image.open(image) as frame:
        tiny = frame.convert('L').resize((32, 18))
        values = list(tiny.get_flattened_data())
        stats = ImageStat.Stat(tiny)
        details['mean_luma'] = round(stats.mean[0], 2)
        details['luma_stddev'] = round(stats.stddev[0], 2)
        details['possibly_blank'] = stats.stddev[0] < 1.0
    previous = STATE / 'previous-luma.json'
    if previous.exists():
        old = json.loads(previous.read_text())
        # Compare only frames from the same game process and source dimensions.
        if old['pid'] == pid and old['size'] == [details['source_width'], details['source_height']]:
            details['change_score'] = round(sum(abs(a-b) for a,b in zip(values, old['pixels'])) / len(values), 2)
    atomic_json(previous, {'pid': pid, 'size': [details['source_width'], details['source_height']], 'pixels': values})
    details.update(image=str(image), pid=pid, hwnd=hwnd, focused=focused(hwnd),
                   bytes=image.stat().st_size, captured_at=datetime.datetime.now().isoformat())
    atomic_json(image.with_suffix('.json'), details)
    return details


BUTTON_NAMES = {
    'a': 'A', 'b': 'B', 'x': 'X', 'y': 'Y', 'start': 'START', 'back': 'BACK',
    'up': 'DPAD_UP', 'down': 'DPAD_DOWN', 'left': 'DPAD_LEFT', 'right': 'DPAD_RIGHT',
    'lb': 'LEFT_SHOULDER', 'rb': 'RIGHT_SHOULDER',
    'ls': 'LEFT_THUMB', 'rs': 'RIGHT_THUMB',
}


def telemetry():
    """Read existing camera locator data instead of paying for another image."""
    import struct
    sys.path.insert(0, str(ROOT / 'tools/package-inspector'))
    from resolve_camera import Resolver
    pid, hwnd = game_window()
    resolver = Resolver(pid, json.loads((ROOT / 'tools/package-inspector/reloaded-profile.json').read_text()))
    try:
        camera = resolver.resolve()
        address = int(camera['pov_address'], 16)
        timestamp = int(camera['timestamp_address'], 16)
        first = struct.unpack('<f', resolver.read(timestamp, 4))[0]
        time.sleep(0.2)
        raw = struct.unpack('<3f3if', resolver.read(address, 28))
        second = struct.unpack('<f', resolver.read(timestamp, 4))[0]
        return {'pid': pid, 'focused': focused(hwnd), 'camera_time': second,
                'camera_time_advancing': second != first,
                'location': [round(x, 3) for x in raw[:3]],
                'rotation_ru': list(raw[3:6]), 'fov': round(raw[6], 3)}
    finally:
        resolver.p.close()


def validate_action(data):
    seconds = float(data.get('seconds', 0.15))
    if not math.isfinite(seconds) or not 0.02 <= seconds <= 3:
        raise ValueError('Each action must last 0.02..3 seconds')
    buttons = data.get('buttons', [])
    if any(x not in BUTTON_NAMES for x in buttons):
        raise ValueError('Unknown button')
    axes = {}
    for name in ('left', 'right'):
        values = data.get(name, [0, 0])
        if len(values) != 2:
            raise ValueError('Stick needs two axes')
        axes[name] = [float(x) for x in values]
        if any(not math.isfinite(x) or abs(x) > 1 for x in axes[name]):
            raise ValueError('Stick axes must be -1..1')
    triggers = [float(data.get(name, 0)) for name in ('lt', 'rt')]
    if any(not math.isfinite(x) or not 0 <= x <= 1 for x in triggers):
        raise ValueError('Trigger values must be 0..1')
    return seconds, buttons, axes, triggers


def perform(pad, vg, data):
    seconds, buttons, axes, triggers = validate_action(data)
    background = data.get('background', False)
    wait = float(data.get('wait_focus', 0))
    if not math.isfinite(wait) or not 0 <= wait <= 60:
        raise ValueError('Focus wait must be 0..60 seconds')
    staged = False
    try:
        pid, hwnd = game_window()
        if background:
            expected_pid, creation = data.get('pid'), data.get('creation_time')
            if (type(expected_pid) is not int or expected_pid <= 0 or
                    type(creation) is not int or creation <= 0):
                raise ValueError('Background action requires exact --pid and --creation-time')
            if wait:
                raise ValueError('Background action cannot use --wait-focus')
            if pid != expected_pid:
                raise RuntimeError('Background action PID does not match game window')
            if interrupted():
                raise RuntimeError('Input stopped on stop request')
            import background_input
            # Status verifies the complete certified function and executable;
            # this path never enables or restores a patch.
            status = background_input.change(pid, 'status', expected_creation=creation)
            identity = status['identity']
            if (status['state'] != 'enabled' or identity['pid'] != pid or
                    identity['creation_time'] != creation):
                raise RuntimeError('Background input is not verified enabled for this process')
            control = background_input.WindowsControl()

        def guard():
            if interrupted():
                raise RuntimeError('Input stopped on stop request')
            if background:
                if game_window() != (pid, hwnd) or control.identity(pid) != identity:
                    raise RuntimeError('Game identity changed during background action')
            elif not focused(hwnd):
                raise RuntimeError('Input stopped on focus loss or stop request')

        if not background:
            deadline = time.monotonic() + wait
            while not focused(hwnd):
                if time.monotonic() >= deadline or interrupted():
                    raise RuntimeError('Game lacks focus; no input sent')
                time.sleep(0.05)
                if game_window() != (pid, hwnd):
                    raise RuntimeError('Game restarted while waiting')
        staged = True
        pad.reset()
        for button in buttons:
            pad.press_button(getattr(vg.XUSB_BUTTON, 'XUSB_GAMEPAD_' + BUTTON_NAMES[button]))
        pad.left_joystick_float(*axes['left'])
        pad.right_joystick_float(*axes['right'])
        pad.left_trigger_float(triggers[0]); pad.right_trigger_float(triggers[1])
        guard()
        began = time.monotonic()
        pad.update()
        while time.monotonic() - began < seconds:
            guard()
            time.sleep(0.01)
        guard()
    finally:
        if background or staged:
            pad.reset(); pad.update()
    result = {'pid': pid, 'input_seconds': round(time.monotonic() - began, 3), 'neutral_after': True}
    if background:
        result.update(background=True, creation_time=creation)
    return result


def serve():
    # Persistent controller avoids reconnecting on every short input command.
    mutex = kernel.CreateMutexW
    mutex.argtypes = [w.LPVOID, w.BOOL, w.LPCWSTR]; mutex.restype = w.HANDLE
    lock = mutex(None, False, 'Local\\GearsVR.GameIO.Server')
    if not lock or c.get_last_error() == 183:
        raise RuntimeError('Game I/O server already running')
    STATE.mkdir(parents=True, exist_ok=True)
    (STATE / 'requests').mkdir(exist_ok=True); (STATE / 'results').mkdir(exist_ok=True)
    # Commands belong to the service that accepted them; never replay on restart.
    for old in (STATE / 'requests').glob('*.json'):
        atomic_json(STATE / 'results' / old.name, {'ok': False, 'error': 'Service restarted; command canceled'})
        old.unlink()
    (STATE / 'stop').unlink(missing_ok=True)
    sys.path.insert(0, str(ROOT / 'tools/third-party/vgamepad'))
    import vgamepad as vg
    pad = vg.VX360Gamepad()
    pad.reset(); pad.update()
    from vgamepad.win import vigem_client
    index = c.c_ulong()
    code = vigem_client.vigem_target_x360_get_user_index(pad._busp, pad._devicep, c.byref(index))
    controller_slot = index.value if code == 0x20000000 else None
    atomic_json(STATE / 'server.json', {'pid': os.getpid(), 'started_at': datetime.datetime.now().isoformat(),
                                      'controller': 'virtual Xbox 360', 'xinput_slot': controller_slot,
                                      'capabilities': ['background_action_v1'],
                                      'state': 'ready'})
    try:
        while not interrupted():
            for file in sorted((STATE / 'requests').glob('*.json')):
                request_id = file.stem
                request = {}
                try:
                    request = json.loads(file.read_text(encoding='utf-8'))
                    if time.time() - float(request['created']) > 90:
                        raise RuntimeError('Expired command; input refused')
                    command = request['command']
                    if command == 'status':
                        try:
                            pid, hwnd = game_window()
                            result = {'pid': pid, 'hwnd': hwnd, 'focused': focused(hwnd), 'game_found': True}
                        except RuntimeError:
                            result = {'game_found': False}
                        result['controller'] = 'connected'
                        result['xinput_slot'] = controller_slot
                    elif command == 'telemetry':
                        result = telemetry()
                    elif command == 'focus':
                        result = focus_game()
                    elif command == 'capture':
                        result = snapshot(request_id, request.get('width', 640))
                    elif command == 'action':
                        result = perform(pad, vg, request)
                        time.sleep(0.2)
                        if request.get('capture', True):
                            result['frame'] = snapshot(request_id, request.get('width', 640))
                    else:
                        raise ValueError('Unknown command')
                    response = {'ok': True, **result}
                except Exception as error:
                    pad.reset(); pad.update()
                    response = {'ok': False, 'error': str(error)}
                response['request_id'] = request_id
                atomic_json(STATE / 'results' / file.name, response)
                with (STATE / 'history.jsonl').open('a', encoding='utf-8') as history:
                    history.write(json.dumps({'request': request, 'result': response}) + '\n')
                file.unlink()
            time.sleep(0.05)
    finally:
        pad.reset(); pad.update()
        atomic_json(STATE / 'server.json', {'pid': os.getpid(), 'state': 'stopped'})
        kernel.CloseHandle(lock)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=['serve', 'status', 'telemetry', 'focus', 'capture', 'action', 'stop'])
    parser.add_argument('--width', type=int, default=640)
    parser.add_argument('--seconds', type=float, default=0.15)
    parser.add_argument('--buttons', nargs='*', default=[])
    parser.add_argument('--left', nargs=2, type=float, default=[0, 0])
    parser.add_argument('--right', nargs=2, type=float, default=[0, 0])
    parser.add_argument('--lt', type=float, default=0)
    parser.add_argument('--rt', type=float, default=0)
    parser.add_argument('--wait-focus', type=float, default=0)
    parser.add_argument('--background', action='store_true', help='Opt in to verified background controller action')
    parser.add_argument('--pid', type=int, help='Exact game PID for background action')
    parser.add_argument('--creation-time', type=int, help='Exact game creation FILETIME for background action')
    parser.add_argument('--no-capture', action='store_true')
    args = parser.parse_args()
    if args.background and (args.command != 'action' or not args.pid or not args.creation_time or args.wait_focus):
        parser.error('--background requires action, --pid and --creation-time, without --wait-focus')
    if args.command == 'serve':
        serve(); return
    if args.command == 'stop':
        STATE.mkdir(parents=True, exist_ok=True); (STATE / 'stop').touch(); print('stop requested'); return
    if not (STATE / 'server.json').exists():
        raise RuntimeError('Start Start-GameIO.ps1 first')
    service = json.loads((STATE / 'server.json').read_text())
    if args.background and 'background_action_v1' not in service.get('capabilities', []):
        raise RuntimeError('Service does not support background action; stop and restart Start-GameIO.ps1')
    handle = kernel.OpenProcess(0x1000, False, service['pid'])
    if not handle:
        raise RuntimeError('Service is not running; start Start-GameIO.ps1')
    try:
        exit_code = w.DWORD()
        kernel.GetExitCodeProcess.argtypes = [w.HANDLE, c.POINTER(w.DWORD)]
        if not kernel.GetExitCodeProcess(handle, c.byref(exit_code)) or exit_code.value != 259 or service['state'] != 'ready':
            raise RuntimeError('Service is not ready; start Start-GameIO.ps1')
    finally:
        kernel.CloseHandle(handle)
    STATE.mkdir(parents=True, exist_ok=True)
    (STATE / 'requests').mkdir(exist_ok=True); (STATE / 'results').mkdir(exist_ok=True)
    request_id = datetime.datetime.now().strftime('%Y%m%d-%H%M%S-') + uuid.uuid4().hex[:8]
    request = vars(args).copy()
    request['capture'] = not request.pop('no_capture')
    request['created'] = time.time()
    path = STATE / 'requests' / f'{request_id}.json'
    atomic_json(path, request)
    result = STATE / 'results' / path.name
    deadline = time.monotonic() + 80
    while not result.exists():
        if time.monotonic() >= deadline:
            path.unlink(missing_ok=True)
            raise RuntimeError('Server timeout; pending command canceled')
        time.sleep(0.1)
    response = json.loads(result.read_text(encoding='utf-8'))
    print(json.dumps(response, ensure_ascii=False, indent=2))
    if not response['ok']:
        sys.exit(1)


if __name__ == '__main__':
    main()
