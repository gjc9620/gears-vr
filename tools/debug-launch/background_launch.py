"""Launch the game without initially activating its window, then arm background input."""
import argparse
import ctypes
from ctypes import wintypes
import datetime
import json
import os
import pathlib
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]
GAME_IO = ROOT / 'tools/game-io/background_input.py'
RUNTIME = ROOT / 'analysis/runtime'
SW_SHOWNOACTIVATE = 4


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


def launch(game_path, game_args=(), *, foreground=False, disable_background_input=False,
           game_io=GAME_IO, runtime=RUNTIME, popen=subprocess.Popen, quiet_build=None):
    if foreground and quiet_build is not None:
        raise ValueError('foreground and quiet_build cannot be combined')
    game_path = pathlib.Path(game_path).resolve()
    if not game_path.is_file():
        raise FileNotFoundError(game_path)
    if not disable_background_input and not pathlib.Path(game_io).is_file():
        raise FileNotFoundError(game_io)

    quiet_report = None
    if quiet_build is not None:
        from quiet_launch import launch_quiet
        quiet_report = launch_quiet(game_path, game_args, quiet_build, popen=popen)
        game_pid, creation_time = quiet_report['pid'], quiet_report['creation_time']
    else:
        creationflags = 0 if foreground else subprocess.CREATE_NO_WINDOW
        startupinfo = None if foreground else nonactivating_startupinfo()
        process = popen([str(game_path), *map(str, game_args)], cwd=str(game_path.parent),
                        startupinfo=startupinfo, creationflags=creationflags)
        game_pid, creation_time = process.pid, process_creation_time(process)
    record = {
        'timestamp': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'pid': game_pid,
        'creation_time': creation_time,
        'game_path': str(game_path),
        'game_args': list(map(str, game_args)),
        'window_startup_policy': 'quiet_startup' if quiet_report else 'foreground' if foreground else 'shown_no_activate',
        'background_input_requested': not disable_background_input,
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
    parser.add_argument('--disable-background-input', action='store_true')
    parser.add_argument('--quiet-build', type=pathlib.Path, default=os.environ.get('GEARSVR_QUIET_STARTUP_BUILD') or None)
    args = parser.parse_args(argv)
    try:
        record = launch(args.game_path, args.game_arg, foreground=args.foreground,
                        disable_background_input=args.disable_background_input, quiet_build=args.quiet_build)
        print(json.dumps(record))
        return 0
    except Exception as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
