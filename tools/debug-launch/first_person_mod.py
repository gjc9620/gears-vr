"""Reversible local installation of the user's downloaded First Person 1.0.

Only the two fixed CookedPC packages are replaced. Backups live outside Git.
Unknown current files are never overwritten by install or restore.
"""
import argparse
import hashlib
import json
import os
import pathlib
import shutil
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
GAME = ROOT / 'Gears of War Reloaded'
COOKED = GAME / 'WarGame/CookedPC'
SOURCE = ROOT / 'First Person-21-1-0-1764109229'
BACKUP = ROOT / 'analysis/backups/first-person-1.0'
MANIFEST = BACKUP / 'manifest.json'
EXPECTED = {'Engine': '983f0d5d6757e2652ce29f27bc8fb8bd7a979ff3d87446e004c771e5188ae8f5',
            'WarfareGame': '98872b93f7a2257e1aeb0a25ae29347dc5950e3ee75af178df12bc091eec219a'}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def ensure_closed():
    import subprocess
    result = subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-Command',
        "if (Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue) {exit 1} else {exit 0}"],
        capture_output=True)
    if result.returncode:
        raise RuntimeError('Close the game before installing or restoring packages')


def replace_verified(source, target, expected):
    # Prepare next to the destination, verify the complete copy, then replace
    # atomically on the same volume. Original backups are never used as staging.
    fd, temporary = tempfile.mkstemp(prefix=target.name + '.gearsvr-', suffix='.tmp', dir=target.parent)
    os.close(fd)
    stage = pathlib.Path(temporary)
    try:
        shutil.copyfile(source, stage)
        if digest(stage) != expected:
            raise RuntimeError('Staging copy hash mismatch')
        os.replace(stage, target)
        if digest(target) != expected:
            raise RuntimeError('Installed file hash mismatch')
    finally:
        if stage.exists():
            stage.unlink()


def manifest():
    data = json.loads(MANIFEST.read_text(encoding='utf-8'))
    if data.get('game') != str(GAME) or set(data.get('files', {})) != set(EXPECTED):
        raise RuntimeError('Backup manifest belongs to another target')
    for name, info in data['files'].items():
        if info.get('mod_sha256') != EXPECTED[name]:
            raise RuntimeError('Unexpected mod identity in backup manifest')
        if digest(BACKUP / (name + '.u')) != info['original_sha256']:
            raise RuntimeError('Original backup hash mismatch')
    return data


def status(data):
    report = {}
    for name, mod_hash in EXPECTED.items():
        current = digest(COOKED / (name + '.u'))
        original = data['files'][name]['original_sha256'] if data else None
        report[name] = dict(sha256=current, state='mod' if current == mod_hash else
                           ('original' if current == original else 'unregistered'))
    print(json.dumps(dict(game=str(GAME), backup=str(BACKUP), files=report), indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('action', choices=['install', 'restore', 'status'])
    args = parser.parse_args()
    data = manifest() if MANIFEST.exists() else None
    if args.action == 'status':
        status(data)
        return
    ensure_closed()
    if args.action == 'install':
        for name, expected in EXPECTED.items():
            source = SOURCE / (name + '.xxx')
            if digest(source) != expected:
                raise RuntimeError('Downloaded mod hash differs from the researched version')
            with source.open('rb') as stream:
                if stream.read(4) != bytes.fromhex('c1832a9e'):
                    raise RuntimeError('Unexpected Unreal package header')
        if data is None:
            BACKUP.mkdir(parents=True, exist_ok=True)
            data = dict(game=str(GAME), files={})
            for name, mod_hash in EXPECTED.items():
                original = COOKED / (name + '.u')
                original_hash = digest(original)
                if original_hash == mod_hash:
                    raise RuntimeError('Mod already present without registered original backup')
                destination = BACKUP / (name + '.u')
                if destination.exists():
                    if digest(destination) != original_hash:
                        raise RuntimeError('Existing backup differs; refusing to overwrite it')
                else:
                    with destination.open('xb') as output, original.open('rb') as input_file:
                        shutil.copyfileobj(input_file, output)
                if digest(destination) != original_hash:
                    raise RuntimeError('Original backup verification failed')
                data['files'][name] = dict(original_sha256=original_hash, mod_sha256=mod_hash)
            # Save both verified originals before modifying either game file.
            with MANIFEST.open('x', encoding='utf-8') as stream:
                json.dump(data, stream, indent=2)
    elif data is None:
        raise RuntimeError('No registered original backup to restore')
    # Validate all target files before changing any; third-party changes require
    # investigation instead of an automatic overwrite.
    previous = {}
    for name, info in data['files'].items():
        current = digest(COOKED / (name + '.u'))
        if current not in (info['original_sha256'], info['mod_sha256']):
            raise RuntimeError('Current package differs from original and mod; refusing overwrite')
        previous[name] = current
    try:
        for name, info in data['files'].items():
            target = COOKED / (name + '.u')
            source = SOURCE / (name + '.xxx') if args.action == 'install' else BACKUP / (name + '.u')
            expected = info['mod_sha256'] if args.action == 'install' else info['original_sha256']
            if previous[name] != expected:
                replace_verified(source, target, expected)
    except Exception:
        # Attempt to return the pair to its pre-operation state; backups remain
        # intact even if rollback itself fails.
        for name, before in previous.items():
            rollback = BACKUP / (name + '.u') if before == data['files'][name]['original_sha256'] else SOURCE / (name + '.xxx')
            if digest(COOKED / (name + '.u')) != before:
                replace_verified(rollback, COOKED / (name + '.u'), before)
        raise
    status(data)


if __name__ == '__main__':
    main()
