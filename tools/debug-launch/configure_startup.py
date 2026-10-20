"""Reversible, narrow edits to Reloaded's serialized config string array."""
import argparse
import hashlib
import json
import pathlib
import re
import struct

ROOT = pathlib.Path(__file__).resolve().parents[2]
GAME = ROOT / 'game-installation/Gears of War Reloaded'
CONFIG = GAME / 'WarGame/Config/Steam/Cooked/Coalesced.ini'
STATE = ROOT / 'analysis/startup-config'
EXPECTED_ORIGINAL = '6cf7a0c3a59d85ae6fedf5fc49d8bb406edda9adda7429448d27510e729a8f16'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def decode(data):
    count = struct.unpack_from('<i', data)[0]
    offset = 4
    records = []
    for _ in range(count):
        start = offset
        length = struct.unpack_from('<i', data, offset)[0]
        offset += 4
        size = abs(length) * (2 if length < 0 else 1)
        if not length or size > len(data) - offset:
            raise ValueError('Invalid serialized config string')
        encoding = 'utf-16-le' if length < 0 else 'latin1'
        value = data[offset:offset + size].decode(encoding)
        if not value.endswith('\0'):
            raise ValueError('Missing string terminator')
        offset += size
        records.append((value[:-1], data[start:offset], encoding))
    if offset != len(data) or count % 3:
        raise ValueError('Unexpected config format')
    return records


def encode(records, changes):
    output = [struct.pack('<i', len(records))]
    for index, (value, raw, encoding) in enumerate(records):
        if index not in changes:
            output.append(raw)
            continue
        value = changes[index] + '\0'
        payload = value.encode(encoding)
        length = -(len(payload) // 2) if encoding == 'utf-16-le' else len(payload)
        output.extend((struct.pack('<i', length), payload))
    return b''.join(output)


def patch(data):
    records = decode(data)
    changes = {}
    report = []
    for index in range(0, len(records), 3):
        name, content = records[index][0], records[index + 1][0]
        if name.endswith('\\SteamWarEngine.ini'):
            # Keep the Startup loading movie, attract mode and story cinematics.
            content, count = re.subn(
                r'(?m)^StartupMovies=(?:SplashScreen|MGSLogo|TheCoalitionLogo|UE3AnimatedLogo)\r?\n',
                '', content)
            if count != 4:
                raise ValueError(f'Expected four logo entries, found {count}')
            changes[index + 1] = content
            report.append('Removed four startup logo entries; retained Startup loading movie')
        elif name.endswith('\\SteamWarGame.ini'):
            content, count = re.subn(r'(?m)^bSkipDownloadingTuneables=false(?=\r?$)',
                                    'bSkipDownloadingTuneables=true', content)
            if count != 1:
                raise ValueError(f'Expected one tuneables switch, found {count}')
            changes[index + 1] = content
            report.append('Enabled built-in skip for online tuneables download')
    if len(changes) != 2:
        raise ValueError('Expected Steam engine/game config entries')
    result = encode(records, changes)
    decoded = decode(result)
    for index, record in enumerate(records):
        assert decoded[index][0] == changes.get(index, record[0])
        if index not in changes:
            assert decoded[index][1] == record[1]
    return result, report


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('action', choices=['apply', 'restore', 'check'])
    args = parser.parse_args()
    current = CONFIG.read_bytes()
    backup = STATE / 'Coalesced.original.bin'
    manifest = STATE / 'manifest.json'
    if manifest.exists():
        info = json.loads(manifest.read_text(encoding='utf-8'))
        original = backup.read_bytes()
        if digest(original) != info['original_sha256']:
            raise ValueError('Backup hash mismatch')
        if digest(current) not in (info['original_sha256'], info['patched_sha256']):
            raise ValueError('Config changed externally; refusing to overwrite')
    else:
        original = current
        if digest(original) != EXPECTED_ORIGINAL:
            raise ValueError('Unsupported original config version; investigate before patching')
    patched, report = patch(original)
    if args.action == 'check':
        print(json.dumps({'changes': report, 'current_sha256': digest(current),
                          'original_sha256': digest(original),
                          'patched_sha256': digest(patched)}, indent=2))
        return
    if args.action == 'restore' and not manifest.exists():
        raise ValueError('No saved backup')
    if not manifest.exists():
        STATE.mkdir(parents=True, exist_ok=True)
        backup.write_bytes(original)
        manifest.write_text(json.dumps({'original_sha256': digest(original),
                                       'patched_sha256': digest(patched),
                                       'changes': report}, indent=2), encoding='utf-8')
    target = original if args.action == 'restore' else patched
    temporary = CONFIG.with_suffix('.gearsvr-tmp')
    temporary.write_bytes(target)
    temporary.replace(CONFIG)
    assert CONFIG.read_bytes() == target
    print(args.action, 'OK:', *report, sep='\n')
    print('Takes effect on next game launch.')


if __name__ == '__main__':
    main()
