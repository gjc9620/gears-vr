"""Launch with the frozen process-local quiet policy; background input is opt-in."""
import argparse
import ctypes
from ctypes import wintypes
import datetime
import hashlib
import json
import os
import pathlib
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]
GAME_IO = ROOT / 'tools/game-io/background_input.py'
RUNTIME = ROOT / 'analysis/runtime'
SW_SHOWNOACTIVATE = 4
DEFAULT_QUIET_BUILD = ROOT / 'tools/quiet-startup/build-quiet11'
DEFAULT_QUIET_SHA256 = {
    'GearsEarlyLaunch.exe': '4eb27120bb8d1c5abf0d282a5280f65bf69d6f798edaa003e34673537bec0bf8',
    'GearsQuietStartup.dll': '58b81c8340d14c7083444e6031f050fc4d1d50a4481301fee76c8269f875ea5b',
}


def quiet_build_for_start(foreground=False, explicit=None):
    if foreground:
        if explicit is not None:raise ValueError('foreground and quiet_build cannot be combined')
        return None
    selected = explicit or os.environ.get('GEARSVR_QUIET_STARTUP_BUILD')
    if test_windows_behind_requested() and not selected:
        raise RuntimeError('Test behind-windows policy requires an explicitly built quiet startup directory')
    return pathlib.Path(selected or DEFAULT_QUIET_BUILD)


def test_windows_behind_requested(foreground=False):
    return not foreground and os.environ.get('GEARSVR_TEST_WINDOWS_BEHIND') == '1'


def verify_test_behind_build(build):
    proof = pathlib.Path(build) / 'test-background-policy.json'
    if not proof.is_file():raise RuntimeError('Missing built test behind-windows policy proof: ' + str(proof))
    value = json.loads(proof.read_text(encoding='utf-8-sig'))
    if value.get('schema') != 1 or value.get('policy') != 'hidden_bottom_showwindow_v2':
        raise RuntimeError('Unsupported test behind-windows policy proof')
    hashes = value.get('sha256')
    if not isinstance(hashes,dict) or set(hashes) != {'GearsEarlyLaunch.exe','GearsQuietStartup.dll'}:
        raise RuntimeError('Incomplete test behind-windows build identity')
    for name,expected in hashes.items():
        file = pathlib.Path(build) / name
        if not file.is_file() or hashlib.sha256(file.read_bytes()).hexdigest() != expected:
            raise RuntimeError('Test behind-windows artifact missing or changed: ' + str(file))


def verify_default_quiet_build(build):
    if pathlib.Path(build).resolve() != DEFAULT_QUIET_BUILD.resolve():return
    for name, expected in DEFAULT_QUIET_SHA256.items():
        path = DEFAULT_QUIET_BUILD / name
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise RuntimeError('Frozen quiet startup artifact missing or changed: ' + str(path))


def nonactivating_startupinfo():
    info = subprocess.STARTUPINFO()
    info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    info.wShowWindow = SW_SHOWNOACTIVATE
    return info


def process_creation_time(process):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    kernel.GetProcessTimes.restype = wintypes.BOOL
    created, exited, system, user = (wintypes.FILETIME() for _ in range(4))
    if not kernel.GetProcessTimes(int(process._handle), ctypes.byref(created), ctypes.byref(exited),
                                  ctypes.byref(system), ctypes.byref(user)):
        raise ctypes.WinError(ctypes.get_last_error())
    return (created.dwHighDateTime << 32) | created.dwLowDateTime


def launch(game_path, game_args=(), *, foreground=False, disable_background_input=True,
           game_io=GAME_IO, runtime=RUNTIME, popen=subprocess.Popen, quiet_build=None, launch_receipt=None):
    quiet_build = quiet_build_for_start(foreground, quiet_build)
    game_path = pathlib.Path(game_path).resolve()
    if not game_path.is_file():
        raise FileNotFoundError(game_path)
    if not disable_background_input and not pathlib.Path(game_io).is_file():
        raise FileNotFoundError(game_io)
    if launch_receipt is not None and pathlib.Path(launch_receipt).exists():
        raise RuntimeError('Launch receipt must be fresh')

    quiet_report = None
    if quiet_build is not None:
        verify_default_quiet_build(quiet_build)
        if test_windows_behind_requested(foreground):verify_test_behind_build(quiet_build)
        from quiet_launch import launch_quiet
        quiet_report = launch_quiet(game_path, game_args, quiet_build, popen=popen, main_noactivate=True)
        game_pid, creation_time = quiet_report['pid'], quiet_report['creation_time']
    else:
        creationflags = 0 if foreground else subprocess.CREATE_NO_WINDOW
        startupinfo = None if foreground else nonactivating_startupinfo()
        process = popen([str(game_path), *map(str, game_args)], cwd=str(game_path.parent),
                        startupinfo=startupinfo, creationflags=creationflags)
        game_pid, creation_time = process.pid, process_creation_time(process)
    if launch_receipt is not None:
        # The reservation owner supplies a fresh, per-invocation path. Publish
        # birth before background helper startup or any capture injection.
        receipt = pathlib.Path(launch_receipt)
        value = dict(schema=1,kind='reservation_birth',game_identity=dict(pid=game_pid,
            creation_time=creation_time,image=str(game_path).casefold()),
            injection_attempted=False,prime_attempted=False)
        receipt.parent.mkdir(parents=True,exist_ok=True)
        temporary=receipt.with_name(receipt.name+'.tmp')
        with temporary.open('x',encoding='utf-8') as stream:json.dump(value,stream)
        temporary.replace(receipt)
    record = {
        'timestamp': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'pid': game_pid,
        'creation_time': creation_time,
        'game_path': str(game_path),
        'game_args': list(map(str, game_args)),
        'window_startup_policy': 'quiet_startup' if quiet_report else 'foreground' if foreground else 'shown_no_activate',
        'background_input_requested': not disable_background_input,
        'test_windows_behind_requested': test_windows_behind_requested(foreground),
    }
    runtime = pathlib.Path(runtime)
    runtime.mkdir(parents=True, exist_ok=True)
    if quiet_report is not None:
        record['quiet_startup'] = quiet_report
    record_path = runtime / ('background-launch-%s.json' % game_pid)
    record['record_path'] = str(record_path)
    record_path.write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')

    helper_pid = None
    if record['background_input_requested']:
        stdout_path = runtime / ('background-input-%s.stdout.log' % game_pid)
        stderr_path = runtime / ('background-input-%s.stderr.log' % game_pid)
        command = [sys.executable, str(game_io), 'enable', '--creation-time', str(record['creation_time']), '--pid', str(game_pid),
                   '--wait-seconds', '60']
        with stdout_path.open('ab') as stdout, stderr_path.open('ab') as stderr:
            helper_info = subprocess.STARTUPINFO()
            helper_info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            helper_info.wShowWindow = 0
            helper = popen(command, cwd=str(ROOT), startupinfo=helper_info,
                           creationflags=subprocess.CREATE_NO_WINDOW,
                           stdout=stdout, stderr=stderr, stdin=subprocess.DEVNULL,
                           close_fds=True)
        helper_pid = helper.pid
        record.update(background_input_helper_pid=helper_pid,
                      background_input_stdout=str(stdout_path),
                      background_input_stderr=str(stderr_path))
    record_path.write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    return record


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-path', type=pathlib.Path, required=True)
    parser.add_argument('--game-arg', action='append', default=[])
    parser.add_argument('--foreground', action='store_true', help='Use the ordinary foreground startup policy')
    inputs = parser.add_mutually_exclusive_group()
    inputs.add_argument('--enable-background-input', dest='disable_background_input', action='store_false',
                        help='Explicitly enable physical controller input while the game is unfocused')
    inputs.add_argument('--disable-background-input', dest='disable_background_input', action='store_true',
                        help='Keep background controller input disabled (default; legacy option)')
    parser.set_defaults(disable_background_input=True)
    parser.add_argument('--quiet-build', type=pathlib.Path, help='Override frozen quiet11 startup build (or GEARSVR_QUIET_STARTUP_BUILD)')
    parser.add_argument('--launch-receipt',type=pathlib.Path,help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    try:
        record = launch(args.game_path, args.game_arg, foreground=args.foreground,
                        disable_background_input=args.disable_background_input,
                        launch_receipt=args.launch_receipt,
                        quiet_build=quiet_build_for_start(args.foreground,args.quiet_build))
        print(json.dumps(record))
        return 0
    except Exception as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
