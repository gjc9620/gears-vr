"""Start a separate game/VR namespace with private profiles and native status files."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/debug-launch'))
from background_launch import verify_test_behind_build
from game_install import validate_game


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def copy_stable(source, target):
    raw = source.read_bytes()
    if source.read_bytes() != raw:
        raise RuntimeError('Source changed while copying: ' + str(source))
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open('xb') as stream:
        stream.write(raw)
    if target.read_bytes() != raw:
        raise RuntimeError('Private copy verification failed')
    return digest(raw)


def prepare(name, manifest_path, config_path):
    if not re.fullmatch(r'[a-z0-9_-]{1,40}', name):
        raise ValueError('Instance name must contain 1-40 lowercase letters, digits, - or _')
    selected = ROOT / 'analysis/instances' / name
    if selected.exists():
        raise RuntimeError('Instance directory already exists; choose a fresh name: ' + name)
    manifest = json.loads(manifest_path.read_text(encoding='utf-8-sig'))
    config = json.loads(config_path.read_text(encoding='utf-8-sig'))
    game = validate_game(manifest['game_path'], expected=manifest['game_sha256'])
    quiet = ROOT / 'tools/quiet-startup/build-instance4'
    verify_test_behind_build(quiet)
    original = Path(os.environ.get('LOCALAPPDATA', str(Path.home() / 'AppData/Local'))).resolve()
    source_data = original / 'Coalition/WarGame'
    native = (ROOT / manifest['native_build']).resolve()
    if digest((native / manifest['native_dll']).read_bytes()) != manifest['native_sha256']:
        raise RuntimeError('Default native build identity differs')
    selected.mkdir(parents=True)
    copied = {}
    for folder in ('Config', 'SaveData'):
        source = source_data / folder
        paths = [p for p in source.iterdir() if p.is_file() and not p.is_symlink()
                 and (p.suffix.lower() == '.ini' if folder == 'Config' else '.ue3profile' in p.name)]
        if not paths or len(paths) > 64:
            raise RuntimeError('Expected a bounded set of existing settings/profiles')
        for p in paths:
            if p.stat().st_size > 8 * 1024 * 1024:
                raise RuntimeError('Profile/config exceeds copy bound')
            target = selected / 'LocalAppData/Coalition/WarGame' / folder / p.name
            copied[str(target.relative_to(selected))] = dict(source=str(p), sha256=copy_stable(p, target))
    for folder in ('RoamingAppData', 'Documents', 'GameLogs', 'native'):
        (selected / folder).mkdir()
    cache=source_data/'PSOCache'
    cache_files=[p for p in cache.rglob('*') if p.is_file() and not p.is_symlink()]
    if len(cache_files)>512 or sum(p.stat().st_size for p in cache_files)>2*1024**3:
        raise RuntimeError('Game shader cache exceeds private copy bound')
    for p in cache_files:
        target=selected/'LocalAppData/Coalition/WarGame/PSOCache'/p.relative_to(cache)
        copied[str(target.relative_to(selected))]=dict(source=str(p),sha256=copy_stable(p,target))
    for p in native.iterdir():
        if p.is_file() and p.suffix.lower() in ('.exe', '.dll'):
            copied['native/' + p.name] = dict(source=str(p), sha256=copy_stable(p, selected / 'native' / p.name))
    manifest.update(native_build=str(selected / 'native'), gpu_snapshot=False)
    # All automatic checks use the existing test-only simulator; normal player
    # runtime selection is left to the ordinary launcher.
    for filename, value in [('manifest.json', manifest), ('config.json', config)]:
        (selected / filename).write_text(json.dumps(value, indent=2), encoding='utf-8')
    proof = dict(schema=1, name=name, root=str(selected), original_localappdata=str(original),
                 game_path=str(game), game_logs=str(game.parent.parent / 'WarGame/Logs'),
                 quiet_build=str(quiet), copied=copied, game_exe_modified=False)
    (selected / 'instance.json').write_text(json.dumps(proof, indent=2), encoding='utf-8')
    return selected, proof


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', default='background-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    parser.add_argument('--manifest', type=Path, default=ROOT / 'tools/vr-launcher/manifest.json')
    parser.add_argument('--config', type=Path, default=ROOT / 'tools/vr-launcher/config.json')
    parser.add_argument('--playtest', action='store_true', help='Background zero-image autonomous Mod regression')
    parser.add_argument('--seconds', type=int, default=21600)
    parser.add_argument('--fixture', type=Path)
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    if not 1 <= args.seconds <= 21600:
        parser.error('seconds must be between 1 and 21600')
    selected, proof = prepare(args.name, args.manifest, args.config)
    if args.prepare_only:
        print(json.dumps(dict(root=str(selected), launched=False)))
        return
    env = os.environ.copy()
    env.update(GEARSVR_INSTANCE_ID=args.name, GEARSVR_INSTANCE_ROOT=str(selected),
               GEARSVR_INSTANCE_ORIGINAL_LOCALAPPDATA=proof['original_localappdata'],
               GEARSVR_INSTANCE_GAME_LOGS=proof['game_logs'],
               GEARSVR_QUIET_STARTUP_BUILD=proof['quiet_build'])
    command = [sys.executable, str(ROOT / 'tools/vr-launcher/launcher.py'), 'start',
               '--manifest', str(selected / 'manifest.json'), '--config', str(selected / 'config.json')]
    output = None
    if args.playtest:
        env.update(GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY='1', GEARSVR_TEST_WINDOWS_BEHIND='1', GEARSVR_TEST_MUTE_GAME='1')
        output = ROOT / 'analysis/playtest' / (datetime.datetime.now().strftime('%Y%m%d-') + 'instance-' + args.name)
        if output.exists():
            raise RuntimeError('Playtest evidence directory already exists')
        command = [sys.executable, str(ROOT / 'tools/game-playtest/run.py'), 'run', '--flows',
                   '--hold-on-stall', '--test-health-assist', '--forward-input-check', '--look-input-check',
                   '--navigation', '--world-state', '--interact', '--combat', '--interaction-goals',
                   '--branch-side', 'right', '--seconds', str(args.seconds), '--wait-for-idle', '300',
                   '--manifest', str(selected / 'manifest.json'), '--config', str(selected / 'config.json'),
                   '--output', str(output)]
        if args.fixture:
            command += ['--fixture', str(args.fixture.resolve())]
    with (selected / 'worker.log').open('xb') as stream:
        child = subprocess.Popen(command, cwd=ROOT, env=env, stdin=subprocess.DEVNULL, stdout=stream,
                                 stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW | subprocess.CREATE_NEW_PROCESS_GROUP)
    receipt = dict(schema=1, worker_pid=child.pid, instance=args.name, root=str(selected),
                   output=str(output) if output else None, private_launcher_index=str(selected / 'launcher-session.json'),
                   command=command, background_test=args.playtest)
    (selected / 'worker.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
