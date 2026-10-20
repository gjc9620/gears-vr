"""Explicit early-policy launcher adapter; never falls back to ordinary startup."""
import json
import os
from pathlib import Path
import subprocess
import sys

from game_install import validate_game


def windows_control():
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'native-capture'))
    from run_stereo_continuous import WindowsControl
    return WindowsControl()


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Duplicate quiet startup report field: ' + key)
        result[key] = value
    return result


def launch_quiet(game_path, game_args, build, *, popen=subprocess.Popen, main_noactivate=None):
    if main_noactivate is not None and type(main_noactivate) is not bool:
        raise ValueError('main_noactivate must be None or boolean')
    game = Path(validate_game(game_path)).resolve()
    build = Path(build).resolve()
    launcher, policy = build / 'GearsEarlyLaunch.exe', build / 'GearsQuietStartup.dll'
    if not build.is_dir() or not launcher.is_file() or not policy.is_file():
        raise FileNotFoundError('Quiet startup build requires GearsEarlyLaunch.exe and GearsQuietStartup.dll: ' + str(build))
    info = subprocess.STARTUPINFO()
    info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    info.wShowWindow = 0
    child_environment = None
    if main_noactivate is not None:
        child_environment = dict(os.environ)
        child_environment['GEARSVR_QUIET_MAIN_NOACTIVATE'] = '1' if main_noactivate else '0'
    bootstrap = popen([str(launcher), str(game), str(policy), *map(str, game_args)],
                      cwd=str(game.parent), startupinfo=info, creationflags=subprocess.CREATE_NO_WINDOW,
                      stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.DEVNULL,
                      close_fds=True, text=True, env=child_environment)
    # Native bootstrap owns bounded entry/install/cleanup transactions. A Python
    # timeout must not kill its debugger or abandon a suspended child midway.
    stdout, stderr = bootstrap.communicate()
    if bootstrap.returncode != 0:
        raise RuntimeError(f'Quiet startup failed (bootstrap pid={bootstrap.pid}, exit={bootstrap.returncode}): {stderr.strip()}')
    try:
        report = json.loads(stdout, object_pairs_hook=unique_object)
        if not isinstance(report, dict):
            raise ValueError('Expected one JSON object')
        for key in ('entry_restored', 'debugger_detached', 'quiet_policy_installed'):
            if report.get(key) is not True:
                raise ValueError('Missing successful ' + key)
        for key, maximum in (('pid', 0xffffffff), ('creation_time', (1 << 64) - 1)):
            if type(report.get(key)) is not int or not 0 < report[key] <= maximum:
                raise ValueError('Invalid ' + key)
        if report['pid'] == bootstrap.pid:
            raise ValueError('Bootstrap PID is not the game PID')
        if type(report.get('pre_entry_visible_windows')) is not int or report['pre_entry_visible_windows'] != 0:
            raise ValueError('Pre-entry visible windows were not excluded')
    except (ValueError, TypeError) as error:
        raise RuntimeError('Invalid quiet startup report: ' + str(error)) from error
    identity = windows_control().identity(report['pid'])
    if (not identity or identity.get('pid') != report['pid'] or
            identity.get('creation_time') != report['creation_time'] or
            not isinstance(identity.get('image'), str) or
            str(Path(identity['image']).resolve()).casefold() != str(game).casefold()):
        raise RuntimeError('Quiet startup game process identity mismatch or exited')
    return dict(report, bootstrap_pid=bootstrap.pid, build=str(build))
