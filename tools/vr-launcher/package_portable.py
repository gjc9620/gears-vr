"""Create a verified local ZIP from a staged inventory, without collecting runtime files."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import tempfile
import zipfile

import stage_portable as staging

MAX_INVENTORY_BYTES = 16 * 1024 * 1024
MAX_FILES = 20000
MAX_FILE_BYTES = 2 * 1024 ** 3
MAX_TOTAL_BYTES = 8 * 1024 ** 3


def relative_path(value):
    if not isinstance(value, str) or not value or len(value) > 240:
        raise ValueError('Invalid inventory path')
    parts = value.split('/')
    if any(part in ('', '.', '..') or part[-1:] in (' ', '.') for part in parts):
        raise ValueError('Unsafe inventory path: ' + value)
    if any(ord(char) < 32 or char in '\\:*?"<>|' for char in value):
        raise ValueError('Unsafe inventory path: ' + value)
    if any(re.fullmatch(r'(con|prn|aux|nul|com[0-9]|lpt[0-9])(?:\..*)?', part, re.I) for part in parts):
        raise ValueError('Reserved inventory path: ' + value)
    return value


def read_bounded(path, limit):
    staging.no_links(path)
    with path.open('rb') as stream:
        if not stat.S_ISREG(os.fstat(stream.fileno()).st_mode):
            raise ValueError('Expected regular file: ' + str(path))
        data = stream.read(limit + 1)
    if len(data) > limit:
        raise ValueError('File exceeds size bound: ' + str(path))
    return data


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Duplicate JSON key: ' + key)
        result[key] = value
    return result


def allowed_paths(root):
    manifest = json.loads(read_bounded(staging.contained(root, 'tools/vr-launcher/manifest.json'),
                                      1024 * 1024), object_pairs_hook=unique_object)
    for key, folder in (('native_build', 'native-capture'), ('xr_build', 'openxr-probe')):
        if not re.fullmatch(r'tools/' + folder + r'/build-[A-Za-z0-9_-]+', manifest.get(key, '')):
            raise ValueError('Unexpected staged build path')
    if not re.fullmatch(r'GearsCapture[A-Za-z0-9_-]*\.dll', manifest.get('native_dll', '')):
        raise ValueError('Unexpected staged native DLL')
    result = set(staging.payload_paths(manifest)) | set(staging.LICENSES.values()) | {
        'tools/vr-launcher/manifest.json', 'tools/vr-launcher/config.json', 'README.txt', 'portable-candidate.json'}
    metadata = json.loads(read_bounded(staging.contained(root, 'portable-candidate.json'), 1024 * 1024),
                          object_pairs_hook=unique_object)
    if not isinstance(metadata, dict): raise ValueError('Invalid portable candidate metadata')
    if metadata.get('kind') == 'public-test-release':
        if metadata.get('version') != '0.0.1': raise ValueError('Unsupported release version')
        result.add('RELEASE_NOTES.txt')
    alias = metadata.get('candidate_manifest')
    if alias is not None:
        if not isinstance(alias, str) or not re.fullmatch(r'tools/vr-launcher/manifest-[A-Za-z0-9_-]+\.json', alias):
            raise ValueError('Unexpected candidate manifest alias')
        active = read_bounded(staging.contained(root, 'tools/vr-launcher/manifest.json'), 1024 * 1024)
        if read_bounded(staging.contained(root, alias), 1024 * 1024) != active:
            raise ValueError('Candidate manifest alias differs from active manifest')
        result.add(alias)
    return result


def permitted(name, fixed):
    forbidden = {'analysis', 'game', 'games', 'derivatives', 'pseudocode', 'snapshots', 're-kb'}
    if any(part.casefold() in forbidden for part in name.split('/')):
        return False
    if name in fixed:
        return True
    prefix = 'runtime/python/'
    if not name.startswith(prefix):
        return False
    tail = name[len(prefix):]
    if tail in staging.PYTHON_FILES:
        return True
    parts = tail.split('/')
    if parts[0] == 'Lib' and len(parts) > 1 and tail.endswith('.py'):
        return not any(part.casefold() in staging.LIB_EXCLUDE for part in parts[1:-1])
    return (len(parts) == 2 and parts[0] == 'DLLs' and Path(tail).suffix.lower() in ('.dll', '.pyd')
            and not parts[1].lower().startswith(('_tkinter', 'tcl', 'tk')))


def load_inventory(root):
    data = read_bounded(staging.contained(root, 'inventory.json'), MAX_INVENTORY_BYTES)
    inventory = json.loads(data, object_pairs_hook=unique_object)
    if not isinstance(inventory, dict) or inventory.get('schema') != 1 or inventory.get('inventory_self_excluded') is not True:
        raise ValueError('Unsupported inventory')
    rows = inventory.get('files')
    if not isinstance(rows, list) or not 0 < len(rows) <= MAX_FILES:
        raise ValueError('Invalid inventory file count')
    fixed = allowed_paths(root)
    seen = set(); total = 0
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError('Invalid inventory row')
        name = relative_path(row.get('path'))
        if name.casefold() in seen:
            raise ValueError('Duplicate inventory path: ' + name)
        seen.add(name.casefold())
        if not permitted(name, fixed):
            raise ValueError('Path outside portable allowlist: ' + name)
        size = row.get('size'); sha = row.get('sha256')
        if type(size) is not int or not 0 <= size <= MAX_FILE_BYTES or not isinstance(sha, str) or not re.fullmatch('[0-9a-f]{64}', sha):
            raise ValueError('Invalid inventory size/hash: ' + name)
        total += size
    required = fixed | {'runtime/python/' + name for name in staging.PYTHON_FILES}
    missing = required - {row['path'] for row in rows}
    if missing:
        raise ValueError('Missing required inventory members: ' + ', '.join(sorted(missing)))
    if total > MAX_TOTAL_BYTES or type(inventory.get('bytes')) is not int or total != inventory['bytes']:
        raise ValueError('Invalid inventory byte total')
    return data, rows


def transfer(source, destination, expected_size, expected_hash):
    count = 0; digest = hashlib.sha256()
    for block in iter(lambda: source.read(1024 * 1024), b''):
        count += len(block)
        if count > expected_size:
            raise ValueError('Inventory verification failed: size')
        digest.update(block)
        if destination is not None:
            destination.write(block)
    if count != expected_size or digest.hexdigest() != expected_hash:
        raise ValueError('Inventory verification failed: size/hash')


def verify_zip(path, inventory_bytes, rows):
    with zipfile.ZipFile(path, 'r') as archive:
        names = [row['path'] for row in rows] + ['inventory.json']
        if archive.namelist() != names:
            raise ValueError('Archive member list mismatch')
        if archive.read('inventory.json') != inventory_bytes:
            raise ValueError('Archive inventory mismatch')
        for row in rows:
            info = archive.getinfo(row['path'])
            if info.file_size != row['size']:
                raise ValueError('Archive member size mismatch')
            with archive.open(info) as source:
                transfer(source, None, row['size'], row['sha256'])


def package(stage, output):
    root = staging.no_links(stage).resolve()
    output = staging.no_links(output)
    if output.exists():
        raise FileExistsError('Output already exists: ' + str(output))
    if output.suffix.lower() != '.zip':
        raise ValueError('Output must have .zip extension')
    if not output.parent.is_dir():
        raise ValueError('Output parent directory must exist')
    inventory_bytes, rows = load_inventory(root)
    fd, name = tempfile.mkstemp(prefix='.' + output.name + '.', suffix='.tmp', dir=output.parent)
    temp = Path(name)
    try:
        with os.fdopen(fd, 'w+b') as stream:
            with zipfile.ZipFile(stream, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
                for row in rows:
                    source_path = staging.contained(root, row['path'])
                    with source_path.open('rb') as source:
                        if not stat.S_ISREG(os.fstat(source.fileno()).st_mode):
                            raise ValueError('Expected regular payload file')
                        with archive.open(row['path'], 'w', force_zip64=True) as target:
                            transfer(source, target, row['size'], row['sha256'])
                    staging.no_links(source_path)
                archive.writestr('inventory.json', inventory_bytes)
            stream.flush(); os.fsync(stream.fileno())
        verify_zip(temp, inventory_bytes, rows)
        result = {'output': str(output.resolve()), 'sha256': staging.digest(temp),
                  'files': len(rows) + 1, 'payload_bytes': sum(row['size'] for row in rows),
                  'archive_bytes': temp.stat().st_size}
        staging.no_links(output)
        # A hard link publishes the completed file atomically and fails if the
        # destination appeared meanwhile. No replace/rename-overwrite fallback.
        os.link(temp, output)
        return result
    finally:
        temp.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(package(args.stage, args.output)))


if __name__ == '__main__':
    main()
