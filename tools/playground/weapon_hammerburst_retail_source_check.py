"""Read-only HB frozen source check; never queries processes or starts a game."""
from pathlib import Path
import ast
import hashlib
import json

ROOT = Path(__file__).resolve().parents[2]
EVIDENCE = ROOT / 'analysis/hammerburst-20261108'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def check():
    plan = load(EVIDENCE / 'manual-veto-retail-runtime-plan.json')
    target = Path(plan['candidate'])
    prepared = load(target / 'prepared.json')
    assert sha(target / 'prepared.json') == plan['prepared_sha256']
    assert Path(prepared['source_archive']) == target
    errors = []

    def verify(path, expected, kind):
        path = Path(path)
        if not path.is_file() or sha(path) != expected:
            errors.append({'path': str(path), 'kind': kind})

    for rel, digest in prepared['source_sha256'].items():
        verify(target / 'source' / rel, digest, 'frozen_source')
    for rel, digest in prepared['native_source_sha256'].items():
        verify(target / 'native-source' / rel, digest, 'frozen_native')
    for name, binary in prepared['builds'].items():
        verify(binary['saved'], binary['sha256'], 'binary:' + name)
    dependencies = Path(plan['runtime_inputs_receipt'])
    verify(dependencies, plan['runtime_inputs_sha256'], 'input_manifest')
    inputs = load(dependencies)
    for path, digest in inputs['sha256'].items():
        verify(path, digest, 'current_runtime_input')
        ast.parse(Path(path).read_text(encoding='utf-8-sig'), filename=path)
    boundary = load(EVIDENCE / 'manual-veto-retail-python-boundary.json')
    assert boundary['prepared_sha256'] == plan['prepared_sha256']
    current_python = set((ROOT / 'tools').rglob('*.py')) | set((ROOT / 'analysis/solo-m1-20261028').glob('*.py'))
    inventory = {path.relative_to(ROOT).as_posix() for path in current_python}
    if inventory != set(boundary['files']):
        errors.append(dict(kind='python_inventory_changed',
                           added=sorted(inventory - set(boundary['files'])),
                           removed=sorted(set(boundary['files']) - inventory)))
    for rel, digest in boundary['files'].items():
        verify(Path(boundary['archive']) / rel, digest, 'archived_python')
        verify(ROOT / rel, digest, 'current_python')

    compiled = EVIDENCE / 'manual-veto-isolated/source'
    base = Path(prepared['base_prepared']) / 'source'
    additions = {
        'playground.cpp': 'bool manual_enabled() noexcept;\n',
        'weapon_hammerburst_runtime.inl': 'bool manual_enabled() noexcept {return weapon_part_inputs::original_source_enabled.load(std::memory_order_acquire);}\n',
        'weapon_lancer_manual_reload.inl': '        (hammerburst_runtime::manual_enabled()&&playground::instance(source.weapon.address,L"Weap_LocustAssaultRifle"))||\n',
    }
    for name, line in additions.items():
        rel = Path('tools/playground') / name
        before = (base / rel).read_text()
        after = (compiled / rel).read_text()
        # The declaration string exists in other namespaces; locate its HB scope.
        needle = 'namespace hammerburst_runtime {\n' if name == 'playground.cpp' else None
        if needle:
            assert after.count(needle + line) == 1
            restored = after.replace(needle + line, needle, 1)
        else:
            assert after.count(line) == 1
            restored = after.replace(line, '', 1)
        assert restored == before, 'Unexpected additional compiled source change: ' + name
        verify(target / 'source' / rel, sha(compiled / rel), 'compiled_source')
    build = load(EVIDENCE / 'manual-veto-isolated/result.json')
    assert build['configure_exit'] == build['build_exit'] == 0
    assert build['sha256'] == prepared['builds']['playground']['sha256']
    retail = plan['retail_requirement']
    assert retail['game_path'] == 'J:/SteamLibrary/steamapps/common/Gears of War Reloaded/Binaries_x64/GOWDE-Steam.exe'
    verify(retail['game_path'], retail['game_sha256'], 'purchased_game')
    result = dict(passed=not errors, errors=errors,
                  source_count=len(prepared['source_sha256']),
                  native_source_count=len(prepared['native_source_sha256']),
                  runtime_inputs=len(inputs['sha256']),
                  python_upper_bound_count=len(boundary['files']),
                  source_delta_lines=3, staged=False, game_started=False,
                  runtime_window_owned=False, startup_resolution_pending=True,
                  acceptance='Static integrity only; no callback, reload, skin or Steam gameplay acceptance')
    (EVIDENCE / 'manual-veto-retail-static-check.json').write_text(json.dumps(result, indent=2))
    return result


if __name__ == '__main__':
    result = check()
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result['passed'] else 1)
