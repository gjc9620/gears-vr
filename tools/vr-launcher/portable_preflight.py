"""Strict staged-only dependency preflight. No process inspection, input or game launch."""
import argparse
import importlib
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/vr-launcher'))
from stage_portable import PROJECT_MODULES, LICENSES, QUIET_BUILD, QUIET_HASHES, AUDIO_HELPER, contained, digest, payload_paths


def check_inventory(root):
    root = Path(root).resolve()
    inventory = json.loads((root / 'inventory.json').read_text())
    names = set()
    for item in inventory['files']:
        name = item['path']
        if name in names: raise ValueError('Duplicate inventory file: ' + name)
        names.add(name); file = contained(root, name)
        if not file.is_file() or file.stat().st_size != item['size'] or digest(file) != item['sha256']:
            raise ValueError('Inventory verification failed: ' + name)
    actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
    if actual != names | {'inventory.json'}: raise ValueError('Uninventoried or missing files: ' + str(actual ^ (names | {'inventory.json'})))
    return inventory


def resolve_pe_dependency(root, binary, name, manifest, system_directory):
    """Model the owned loader contexts, never PATH or an unrelated package DLL.

    The injected capture/quiet DLLs use remote LoadLibraryW: their own directory
    is not automatically added to the game's DLL search. Without a selected
    game's directory proof, only Windows imports are admitted for those DLLs.
    """
    root = Path(root).resolve(); binary = Path(binary).resolve(); name = name.lower()
    if Path(name).name != name or ':' in name or '/' in name or '\\' in name:
        raise ValueError('Invalid PE DLL name')
    if not binary.is_relative_to(root): raise ValueError('Binary escapes stage')
    python = root / 'runtime/python'
    capture = contained(root, Path(manifest['native_build']) / manifest['native_dll'])
    quiet = contained(root, QUIET_BUILD + '/GearsQuietStartup.dll')
    injected = binary in (capture, quiet)
    if binary.is_relative_to(python):
        # CPython extensions are loaded with DLL_LOAD_DIR; the Python EXE
        # directory remains the application directory for VC/python DLLs.
        directories = [binary.parent, python] if binary.parent == python / 'DLLs' else [python]
    elif injected:
        directories = []
    elif binary.suffix.lower() == '.exe':
        directories = [binary.parent]
    elif binary == contained(root, Path(manifest['xr_build']) / 'openxr_loader.dll'):
        # Imported by OpenXRPoseProbe.exe from the same application directory.
        directories = [binary.parent]
    else:
        raise ValueError('Unknown PE loader context: ' + str(binary.relative_to(root)))
    for directory in directories:
        candidate = contained(root, (directory / name).relative_to(root))
        if candidate.is_file(): return {'scope': 'bundled-loader-directory', 'path': candidate.relative_to(root).as_posix()}
    if name.startswith(('api-ms-win-', 'ext-ms-win-')):
        return {'scope': 'windows-api-set', 'path': name}
    system = Path(system_directory) / name
    if system.is_file(): return {'scope': 'windows-system32', 'path': name}
    condition = '; injected DLL requires verified game-directory/system placement; sibling alone is insufficient' if injected else ''
    raise ValueError('Unresolved PE import ' + name + ' from ' + str(binary.relative_to(root)) + condition)


def inspect_pe_imports(root, binary, manifest, pefile, system_directory):
    pe = pefile.PE(str(binary), fast_load=True)
    try:
        pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT'],
                                               pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT']])
        result = []
        for attribute, kind in (('DIRECTORY_ENTRY_IMPORT', 'import'), ('DIRECTORY_ENTRY_DELAY_IMPORT', 'delay-import')):
            for entry in getattr(pe, attribute, []):
                name = entry.dll.decode('ascii').lower()
                result.append(dict(binary=Path(binary).relative_to(root).as_posix(), dll=name, kind=kind,
                    **resolve_pe_dependency(root, binary, name, manifest, system_directory)))
        return result
    finally: pe.close()


def import_project_modules(root, project_modules=PROJECT_MODULES):
    """Import the staged allowlist and reject modules resolved outside it."""
    root = Path(root).resolve()
    for folder in project_modules: sys.path.insert(0, str(root / 'tools' / folder))
    sys.path.insert(0, str(root / 'tools/package-inspector/python-libs'))
    imported = []
    for names in project_modules.values():
        for name in names.split():
            if name in ('portable_preflight', 'stage_portable'): continue
            module = importlib.import_module(name)
            if not Path(module.__file__).resolve().is_relative_to(root): raise ValueError('External project import: ' + name)
            imported.append(name)
    return imported


def preflight(root=ROOT):
    root = Path(root).resolve()
    interpreter = root / 'runtime/python/python.exe'
    if not sys.flags.isolated or not sys.flags.no_user_site or Path(sys.executable).resolve() != interpreter:
        raise ValueError('Use only stage/runtime/python/python.exe -I -B; developer fallback forbidden')
    if not Path(sys.prefix).resolve().is_relative_to(root / 'runtime/python'):
        raise ValueError('Python prefix escapes bundled runtime')
    for path in sys.path:
        if path and not Path(path).resolve().is_relative_to(root): raise ValueError('External sys.path: ' + path)
    inventory = check_inventory(root)
    manifest = json.loads((root / 'tools/vr-launcher/manifest.json').read_text())
    config = json.loads((root / 'tools/vr-launcher/config.json').read_text())
    if manifest['game_path'] != 'SELECT_GAME_EXECUTABLE' or config['game_path'] != '':
        raise ValueError('Expected player game-selection placeholder')
    for relative in [*payload_paths(manifest), *LICENSES.values()]:
        if not contained(root, relative).is_file(): raise ValueError('Missing payload dependency: ' + relative)
    for key, relative in (('native_sha256', Path(manifest['native_build']) / manifest['native_dll']),
                          ('xr_sha256', Path(manifest['xr_build']) / 'OpenXRPoseProbe.exe')):
        if digest(contained(root, relative)) != manifest[key]: raise ValueError('Manifest hash mismatch: ' + key)
    for name, expected in QUIET_HASHES.items():
        if digest(contained(root, f'{QUIET_BUILD}/{name}')) != expected: raise ValueError('Quiet11 hash mismatch')
    imported = import_project_modules(root)
    audio = importlib.import_module('game_audio_guard')
    if audio.MANUAL_HELPER.resolve() != contained(root, AUDIO_HELPER):
        raise ValueError('Manual audio helper escapes stage')
    audio.verify_manual_helper(audio.MANUAL_HELPER)
    pefile = importlib.import_module('pefile')
    if not Path(pefile.__file__).resolve().is_relative_to(root): raise ValueError('External pefile import')
    # Query the actual Windows system directory, never a developer PATH entry.
    import ctypes
    directory = ctypes.create_unicode_buffer(32768)
    length = ctypes.windll.kernel32.GetSystemDirectoryW(directory, len(directory))
    if not length or length >= len(directory): raise ValueError('Windows system directory unavailable')
    system_directory = Path(directory.value)
    resolutions = []
    for item in inventory['files']:
        file = root / item['path']
        if file.suffix.lower() not in ('.exe', '.dll', '.pyd'): continue
        resolutions.extend(inspect_pe_imports(root, file, manifest, pefile, system_directory))
    launcher = importlib.import_module('launcher')
    launcher.settings(dict(config, game_path='SELECT_GAME_EXECUTABLE'), manifest, root)
    background = importlib.import_module('background_launch')
    if background.DEFAULT_QUIET_BUILD.resolve() != contained(root, QUIET_BUILD): raise ValueError('Quiet build escapes stage')
    background.verify_default_quiet_build(background.DEFAULT_QUIET_BUILD)
    # Catch accidental absolute cache paths or registry-dependent stdlib imports.
    for name, module in list(sys.modules.items()):
        file = getattr(module, '__file__', None)
        if file and not file.startswith('<') and not Path(file).resolve().is_relative_to(root):
            raise ValueError('Imported module escapes stage: ' + name + ': ' + file)
    return dict(passed=True, isolated=True, interpreter='runtime/python/python.exe',
                files=len(inventory['files']), payload_bytes=inventory['bytes'],
                inventory_sha256=digest(root / 'inventory.json'), project_imports=sorted(imported),
                pe_imports=sorted({item['dll'] for item in resolutions}), pe_resolutions=resolutions,
                windows_system_dependencies=sorted({item['dll'] for item in resolutions if item['scope'].startswith('windows-')}),
                game_started=False, gui_started=False,
                release_ready=False, limits=json.loads((root / 'portable-candidate.json').read_text())['limits'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.resolve().is_relative_to(ROOT): raise ValueError('Keep preflight evidence outside the inventoried stage')
    result = preflight()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x', encoding='utf-8') as stream: json.dump(result, stream, indent=2)
    print(json.dumps(result))


if __name__ == '__main__': main()
