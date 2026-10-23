"""Default startup policy: DLSS OFF; explicit fixed-profile SR preflight is read-only."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import instance_policy
import steam_account

ROOT = pathlib.Path(__file__).resolve().parents[2]
STATE = (instance_policy.context() / 'graphics-policy') if instance_policy.context() else ROOT / 'analysis/graphics-policy'


def active_steam_id():
    """Prefer the online account, then a verified offline client selection."""
    import winreg
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam\ActiveProcess') as key:
            account, kind = winreg.QueryValueEx(key, 'ActiveUser')
        if kind == winreg.REG_DWORD and type(account) is int and 0 < account <= 0xffffffff:
            return str(0x0110000100000000 | account)
        if kind == winreg.REG_DWORD and type(account) is int and account == 0:
            return steam_account.offline_steam_id()
    except OSError:
        pass
    return None


def locate(game_path=None):
    folder = (instance_policy.local_appdata() / 'Coalition/WarGame/Config').resolve()
    # An explicit per-install binding supports installations using a different
    # local profile. A game-path change cannot silently reuse another binding.
    if game_path is not None and not instance_policy.context():
        config_path = ROOT / 'tools/vr-launcher/config.json'
        config = json.loads(config_path.read_text(encoding='utf-8')) if config_path.exists() else {}
        bindings = config.get('game_settings_paths', {})
        if (not isinstance(bindings, dict) or any(
                not isinstance(key, str) or not pathlib.Path(key).is_absolute()
                or not isinstance(value, str) or not pathlib.Path(value).is_absolute()
                for key, value in bindings.items())):
            raise RuntimeError('game_settings_paths must map absolute game paths to settings files')
        selected = str(pathlib.Path(game_path).resolve()).casefold()
        matches = [value for key, value in bindings.items()
                   if pathlib.Path(key).is_absolute() and str(pathlib.Path(key).resolve()).casefold() == selected]
        if len(matches) > 1:
            raise RuntimeError('Multiple settings bindings for the selected installation')
        if matches:
            target = pathlib.Path(matches[0]).resolve()
            if (target.parent != folder or not target.is_file()
                    or not re.fullmatch(r'(?:[0-9]{17})?EngineUserSettings\.ini', target.name)):
                raise RuntimeError('Settings binding must select an existing game settings file in the local Config directory')
            return target
    files = list(folder.glob('*EngineUserSettings.ini'))
    if len(files) > 1:
        active = active_steam_id()
        matches = [file for file in files if file.name == str(active) + 'EngineUserSettings.ini']
        if len(matches) == 1:
            return matches[0].resolve()
        raise RuntimeError('Multiple user-settings files; cannot identify the online/offline Steam profile. '
                           'Select a per-install profile with game_settings_paths in the launcher config')
    if len(files) != 1:
        raise RuntimeError('Expected exactly one user-settings file')
    return files[0].resolve()


def decode(raw):
    if raw.startswith(b'\xff\xfe'):
        return raw.decode('utf-16'), 'utf-16'
    encoding = 'utf-8-sig' if raw.startswith(b'\xef\xbb\xbf') else 'utf-8'
    return raw.decode(encoding), encoding


def dlss_value(text):
    values = re.findall(r'^DLSS=([^\r\n]*)\r?$', text, re.M | re.I)
    if len(values) != 1 or values[0].lower() not in ('true', 'false'):
        raise RuntimeError('Expected one boolean DLSS setting')
    return values[0]


def set_dlss(raw, value):
    text, encoding = decode(raw)
    dlss_value(text)
    patched = re.sub(r'^(DLSS=)[^\r\n]*', lambda m: m[1] + value, text, flags=re.M | re.I)
    return patched.encode(encoding)


def require_game_stopped():
    if instance_policy.context():
        index=instance_policy.index_path()
        if index.exists():
            state=json.loads(index.read_text(encoding='utf-8'))
            if state.get('phase') not in ('finished','incomplete','preflight','cancelled_before_activation') and state.get('game_identity'):
                raise RuntimeError('This private instance is already active')
        return
    if instance_policy.shared_game_pids(instance_policy.running_game_pids()):
        raise RuntimeError('Exit the game normally before applying the startup graphics policy')


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def atomic_write(path, raw):
    temporary = path.with_name(path.name + '.gearsvr-policy-tmp')
    # Exclusive creation avoids destroying an existing temporary or another writer.
    with temporary.open('xb') as output:
        output.write(raw)
    try:
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)
    if path.read_bytes() != raw:
        raise RuntimeError('Graphics policy write verification failed')


def status(target=None):
    target = pathlib.Path(target) if target is not None else locate()
    value = dlss_value(decode(target.read_bytes())[0])
    return {'path': str(target), 'persisted_dlss': value, 'startup_policy': 'off',
            'runtime_lock_verified': False, 'live_dlss': 'unknown'}


def check_experimental_sr(target=None):
    """Read-only preflight for one historical SR profile; no live support claim."""
    target = pathlib.Path(target) if target is not None else locate()
    text, _ = decode(target.read_bytes())
    expected = {'DLSS':'true', 'FSR2':'false', 'FSR3':'false', 'MFSR':'false',
                'AADefaultQualityMode':'3'}
    for key, value in expected.items():
        values = re.findall(r'^' + re.escape(key) + r'=([^\r\n]*)\r?$', text, re.M | re.I)
        if len(values) != 1 or values[0].lower() != value:
            raise RuntimeError('Experimental DLSS SR requires one ' + key + '=' + value)
    return {'path':str(target), 'startup_policy':'experimental_preserve_upscaler',
            'persisted_dlss':'true', 'persisted_aa_quality_mode':3, 'changed':False,
            'runtime_lock_verified':False, 'live_dlss':'unknown'}


def enforce_off_before_launch(target=None, state=STATE, stopped=require_game_stopped):
    stopped()
    target = (pathlib.Path(target) if target is not None else locate()).resolve()
    raw = target.read_bytes()
    value = dlss_value(decode(raw)[0])
    if value.lower() == 'false':
        return dict(status(target), changed=False)
    state = pathlib.Path(state)
    state.mkdir(parents=True, exist_ok=True)
    original_hash = digest(raw)
    backup = state / (original_hash + '.ini')
    manifest = state / (original_hash + '.json')
    if backup.exists() and backup.read_bytes() != raw:
        raise RuntimeError('Existing graphics policy backup differs')
    if not backup.exists():
        with backup.open('xb') as output:
            output.write(raw)
    report = {'path': str(target), 'backup': str(backup.resolve()),
              'original_sha256': original_hash, 'original_dlss': value}
    if manifest.exists() and json.loads(manifest.read_text(encoding='utf-8')) != report:
        raise RuntimeError('Existing graphics policy manifest differs')
    if not manifest.exists():
        manifest.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    # Check again before replacing; never silently discard another settings edit.
    if target.read_bytes() != raw:
        raise RuntimeError('Settings changed during graphics policy preparation')
    atomic_write(target, set_dlss(raw, 'False'))
    return dict(status(target), changed=True, restore_manifest=str(manifest.resolve()))


def restore(manifest, target=None, stopped=require_game_stopped):
    stopped()
    report = json.loads(pathlib.Path(manifest).read_text(encoding='utf-8'))
    target = (pathlib.Path(target) if target is not None else locate()).resolve()
    if target != pathlib.Path(report['path']).resolve():
        raise RuntimeError('Graphics policy backup belongs to a different settings file')
    saved = pathlib.Path(report['backup']).read_bytes()
    if digest(saved) != report['original_sha256'] or dlss_value(decode(saved)[0]) != report['original_dlss']:
        raise RuntimeError('Graphics policy backup verification failed')
    raw = target.read_bytes()
    if dlss_value(decode(raw)[0]).lower() != 'false':
        raise RuntimeError('DLSS changed independently; refusing restore')
    atomic_write(target, set_dlss(raw, report['original_dlss']))
    return dict(status(target), restored=True, other_current_settings_preserved=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('status', 'check', 'apply', 'restore', 'check-experimental-sr'))
    parser.add_argument('--manifest', help='Explicit backup manifest required for restore')
    parser.add_argument('--game-path', type=pathlib.Path, help='Selected installation for an optional settings binding')
    args = parser.parse_args()
    target = locate(args.game_path)
    if args.action == 'check-experimental-sr':
        result = check_experimental_sr(target)
    elif args.action == 'restore':
        if not args.manifest:
            parser.error('restore requires --manifest')
        result = restore(args.manifest, target)
    elif args.action == 'apply':
        result = enforce_off_before_launch(target)
    else:
        result = status(target)
        if args.action == 'check' and result['persisted_dlss'].lower() != 'false':
            raise RuntimeError('Persisted DLSS must be OFF before release startup')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
