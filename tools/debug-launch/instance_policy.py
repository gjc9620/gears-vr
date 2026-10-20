"""Validate a private profile/build namespace before permitting concurrent games."""
import argparse
import json
import os
from pathlib import Path
import re
import ctypes
from ctypes import wintypes
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]


def context(environ=None, root=ROOT):
    env = os.environ if environ is None else environ
    name, selected = env.get('GEARSVR_INSTANCE_ID'), env.get('GEARSVR_INSTANCE_ROOT')
    if not name and not selected:
        return None
    if not name or not re.fullmatch(r'[a-z0-9_-]{1,40}', name) or not selected:
        raise RuntimeError('Incomplete or invalid isolated-instance selection')
    path = Path(selected).resolve()
    base = Path(root).resolve() / 'analysis/instances'
    if path != (base / name).resolve() or not path.is_relative_to(base):
        raise RuntimeError('Instance directory must be its named private workspace directory')
    proof = json.loads((path / 'instance.json').read_text(encoding='utf-8'))
    if proof.get('schema') != 1 or proof.get('name') != name or proof.get('root') != str(path):
        raise RuntimeError('Instance identity proof differs')
    for folder in ('LocalAppData/Coalition/WarGame/Config', 'LocalAppData/Coalition/WarGame/SaveData',
                   'RoamingAppData', 'Documents', 'GameLogs', 'native'):
        if not (path / folder).is_dir():
            raise RuntimeError('Private instance directory missing: ' + folder)
    if env.get('GEARSVR_INSTANCE_ORIGINAL_LOCALAPPDATA') != proof.get('original_localappdata'):
        raise RuntimeError('Original profile identity differs')
    return path


def owner_lock_name():
    selected = context()
    return 'Local\\GearsVR.RepositoryLauncher.Owner' + ('.' + selected.name if selected else '')


def local_appdata():
    selected = context()
    return selected / 'LocalAppData' if selected else Path.home() / 'AppData/Local'


def index_path():
    selected = context()
    return selected / 'launcher-session.json' if selected else ROOT / 'analysis/runtime/vr-launcher-session.json'


def require_private_manifest(manifest):
    selected = context()
    if selected:
        build = (ROOT / manifest['native_build']).resolve()
        if build != (selected / 'native').resolve():
            raise RuntimeError('Concurrent instance requires its private native build/status directory')


def process_identity(pid):
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD]
    kernel.OpenProcess.restype=wintypes.HANDLE
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    kernel.GetProcessTimes.argtypes=[wintypes.HANDLE]+[ctypes.POINTER(wintypes.FILETIME)]*4
    kernel.QueryFullProcessImageNameW.argtypes=[wintypes.HANDLE,wintypes.DWORD,wintypes.LPWSTR,ctypes.POINTER(wintypes.DWORD)]
    handle=kernel.OpenProcess(0x1000,False,pid)
    if not handle:return None
    try:
        created,exited,system,user=(wintypes.FILETIME() for _ in range(4))
        buffer=ctypes.create_unicode_buffer(32768);length=wintypes.DWORD(len(buffer))
        if not kernel.GetProcessTimes(handle,ctypes.byref(created),ctypes.byref(exited),ctypes.byref(system),ctypes.byref(user)):
            return None
        if not kernel.QueryFullProcessImageNameW(handle,0,buffer,ctypes.byref(length)):return None
        return dict(pid=pid,creation_time=(created.dwHighDateTime<<32)|created.dwLowDateTime,image=buffer.value.casefold())
    finally:kernel.CloseHandle(handle)


def shared_game_pids(pids, identity=process_identity, root=ROOT):
    """Normal launches can coexist with attested private instances, never with an unknown game."""
    remaining=set(pids)
    base=Path(root).resolve()/'analysis/instances'
    paths=list(base.glob('*/launcher-session.json')) if base.exists() else []
    if len(paths)>128:return list(pids)
    for path in paths:
        try:
            selected=path.parent.resolve()
            if selected.parent!=base or not re.fullmatch(r'[a-z0-9_-]{1,40}',selected.name):continue
            proof=json.loads((selected/'instance.json').read_text(encoding='utf-8'))
            state=json.loads(path.read_text(encoding='utf-8'))
            candidate=state.get('game_identity') or {}
            if (proof.get('schema')==1 and proof.get('name')==selected.name and proof.get('root')==str(selected)
                    and state.get('kind')=='repository_vr_launcher' and candidate.get('pid') in remaining
                    and candidate.get('image')==str(Path(proof['game_path']).resolve()).casefold()
                    and identity(candidate['pid'])==candidate):
                remaining.remove(candidate['pid'])
        except (OSError,ValueError,KeyError,TypeError):
            continue
    return sorted(remaining)


def running_game_pids():
    result=subprocess.run(['powershell','-NoProfile','-NonInteractive','-Command',
        "Get-CimInstance Win32_Process -Filter 'Name=''GOWDE-Steam.exe''' | Select-Object -ExpandProperty ProcessId"],
        capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW)
    return [int(line) for line in result.stdout.splitlines() if line.strip().isdigit()]


_background_cache = {}


def background_process_pids(identity=process_identity, root=ROOT):
    """Group only registered automatic tests for occlusion, with fresh PID checks.

    A private player instance is still a user application. Metadata is cached
    briefly, but process creation identities are checked on every observation.
    """
    root=Path(root).resolve();key=str(root);now=time.monotonic()
    cached=_background_cache.get(key)
    if cached is None or now-cached[0]>=1:
        candidates=[];base=root/'analysis/instances'
        paths=list(base.glob('*/worker.json')) if base.exists() else []
        for path in paths[:128]:
            try:
                selected=path.parent.resolve()
                if selected.parent!=base:continue
                worker=json.loads(path.read_text(encoding='utf-8'))
                proof=json.loads((selected/'instance.json').read_text(encoding='utf-8'))
                state=json.loads((selected/'launcher-session.json').read_text(encoding='utf-8'))
                game=state.get('game_identity') or {}
                if (worker.get('background_test') is not True or worker.get('root')!=str(selected)
                        or proof.get('root')!=str(selected) or proof.get('name')!=selected.name
                        or proof.get('schema')!=1 or state.get('kind')!='repository_vr_launcher'
                        or game.get('image')!=str(Path(proof['game_path']).resolve()).casefold()):continue
                candidates.append(game)
                session_path=Path(state.get('session_report','')).resolve()
                if session_path.parent==root/'analysis/runtime' and session_path.is_file():
                    session=json.loads(session_path.read_text(encoding='utf-8'))
                    if (session.get('game_identity')==game and session.get('owner_identity')==state.get('owner_identity')
                            and session.get('kind')=='continuous_strict_stereo' and session.get('xr_identity')):
                        candidates.append(session['xr_identity'])
            except (OSError,ValueError,KeyError,TypeError):continue
        _background_cache[key]=(now,candidates)
    else:candidates=cached[1]
    return {item['pid'] for item in candidates if type(item.get('pid')) is int and identity(item['pid'])==item}


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check-startup',action='store_true')
    args=parser.parse_args()
    value = context()
    if args.check_startup and value is None:
        if shared_game_pids(running_game_pids()):raise SystemExit('Existing shared-profile game must exit or use the multi-instance launcher')
        print(json.dumps(dict(shared_profile_available=True)))
    elif value is None:
        raise SystemExit('No isolated instance selected')
    else:print(json.dumps(dict(instance=value.name, root=str(value))))
