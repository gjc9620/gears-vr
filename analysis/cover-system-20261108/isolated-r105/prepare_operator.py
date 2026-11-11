"""Freeze the existing ordinary cover action for the exact r105 private root."""
from pathlib import Path
import ast
import argparse
import json
import sys
import subprocess
from prepare import ROOT, HERE, OLD, R104, sha, load, replace_once, write_new


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', required=True)
    name = parser.parse_args().name
    if not name.startswith('cover-') or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789_-' for c in name):
        raise ValueError('Exact cover instance name required')
    selected = ROOT / 'analysis/instances' / name
    receipt = load(HERE / selected.name / 'prepared.json')
    if (selected / 'launcher-session.json').exists():
        raise RuntimeError('Do not change an attempt after launch')
    for filename in ('manifest.json', 'config.json'):
        if sha(selected / filename) != receipt[filename.removesuffix('.json') + '_sha256']:
            raise RuntimeError('Private launch inputs changed')
    output = selected / 'cover-output'
    adapter = output / 'cover_private_playground_observation.py'
    adapter_source = ROOT / 'tools/game-playtest/cover_playground_observation.py'
    with adapter.open('x', encoding='utf-8') as stream:
        stream.write(adapter_source.read_text(encoding='utf-8'))
    sources = {}
    for filename in ('cover_boundary_chain.py', 'equipment_controller_chain.py', 'run_menu_firstperson_check.py'):
        sources[filename] = (OLD / filename).read_text(encoding='utf-8')
    boundary = sources['cover_boundary_chain.py']
    sources['cover_boundary_chain.py'] = replace_once(boundary,
        'from cover_playground_observation import sample as sample_playground',
        'from cover_private_playground_observation import sample as sample_playground')
    entry_observer = HERE.parent / 'query-age-cause-r149/entry_observation.py'
    import importlib.util
    spec = importlib.util.spec_from_file_location('cover_entry_observation', entry_observer)
    entry_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(entry_module)
    sources['cover_boundary_chain.py'] = entry_module.instrument(sources['cover_boundary_chain.py'])
    closing_observer = HERE.parent / 'query-close-detail-r180/instrument.py'
    spec = importlib.util.spec_from_file_location('cover_candidate_closing', closing_observer)
    closing_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(closing_module)
    sources['cover_boundary_chain.py'] = closing_module.instrument(sources['cover_boundary_chain.py'])
    equipment = sources['equipment_controller_chain.py']
    menu = sources['run_menu_firstperson_check.py']
    # All cover/weapon observer selections refer to this exact namespace;
    # the ordinary approach, original button actions and five phases stay.
    for filename in ('cover_boundary_chain.py', 'equipment_controller_chain.py'):
        sources[filename] = sources[filename].replace(
            "g.ROOT / 'analysis/instances/solo-m1-1028d/", f"Path({str(selected)!r}) / '")
        sources[filename] = sources[filename].replace(
            "g.ROOT/'analysis/instances/solo-m1-1028d/", f"Path({str(selected)!r})/'")
    menu = replace_once(menu, 'OUT=Path(__file__).resolve().parent;ROOT=OUT.parents[1]',
        f'OUT=Path({str(output)!r});ROOT=Path({str(ROOT)!r})')
    menu = menu.replace("ROOT/'analysis/instances/solo-m1-1028d/", f"Path({str(selected)!r})/'")
    menu = replace_once(menu, 'def live():return control.identity(game[\'pid\'])==game', '''session_bound=json.loads(Path(state['session_report']).read_text(encoding='utf-8-sig'))
assert session_bound['game_identity']==game and session_bound['owner_identity']==state['owner_identity']
assert Path(state['instance_root']).resolve()==OUT.parent.resolve()
identities_bound=[game,state['owner_identity'],session_bound['xr_identity']]
def live():
    current=json.loads((OUT.parent/'launcher-session.json').read_text(encoding='utf-8-sig'))
    report=json.loads(Path(state['session_report']).read_text(encoding='utf-8-sig'))
    return (current.get('instance_root')==state['instance_root']
            and current.get('session_report')==state['session_report']
            and current.get('game_identity')==game
            and current.get('owner_identity')==state['owner_identity']
            and report.get('session_id')==session_bound['session_id']
            and report.get('phase')=='running'
            and all(report.get(k)==session_bound[k] for k in ('owner_identity','game_identity','xr_identity'))
            and all(control.identity(i['pid'])==i for i in identities_bound))''')
    menu = menu.replace("(OUT/'launcher.log')", "(OUT.parent/'isolated-launcher.log')")
    # No pause/resume expansion here; r104 already retains that known failure.
    sources['run_menu_firstperson_check.py'] = menu
    wrapper = (R104 / 'run_cover_eye_approach_operator.py').read_text(encoding='utf-8')
    approach_source = ROOT / 'tools/game-playtest/cover_ordinary_approach.py'
    approach_text = approach_source.read_text(encoding='utf-8')
    ast.parse(approach_text, filename=str(approach_source))
    wrapper = replace_once(wrapper, 'import sys, types', 'import sys, types\n' +
        "cover_approach_module = types.ModuleType('cover_ordinary_approach')\n" +
        f'cover_approach_module.__file__ = {str(approach_source)!r}\n' +
        f'exec(compile({approach_text!r}, cover_approach_module.__file__, "exec"), cover_approach_module.__dict__)\n' +
        "sys.modules['cover_ordinary_approach'] = cover_approach_module")
    wrapper = replace_once(wrapper,
        "O = Path(__file__).resolve().parents[3] / 'analysis/solo-m1-20261028'",
        f'O = Path({str(OLD)!r})')
    wrapper = replace_once(wrapper,
        "for name in ('package-inspector', 'game-io', 'native-capture'):",
        "for name in ('package-inspector', 'game-io', 'native-capture', 'game-playtest'):")
    for filename in sources:
        expression = f"s = (O / '{filename}').read_text(encoding='utf-8')"
        wrapper = replace_once(wrapper, expression, 's = ' + repr(sources[filename]))
    wrapper = replace_once(wrapper,
        "exec(compile(s, str(O / 'run_menu_firstperson_check.py'), 'exec'),",
        """if '--prepare-import-check' in sys.argv:
    import ast
    import cover_ordinary_approach
    import cover_private_playground_observation
    imports = [node for node in ast.parse(s).body if isinstance(node, (ast.Import, ast.ImportFrom))]
    exec(compile(ast.fix_missing_locations(ast.Module(body=imports,type_ignores=[])),
        str(O / 'run_menu_firstperson_check.py'), 'exec'),
        {'__file__':str(O / 'run_menu_firstperson_check.py'),'__name__':'cover_import_check'})
    print('Cover action imports and transformations passed; no game/input access')
    raise SystemExit(0)
exec(compile(s, str(O / 'run_menu_firstperson_check.py'), 'exec'),""")
    # The generated action has its own read-only input helper and result paths.
    # Existing recording modules consume its supplied own directory/identities.
    if any('solo-m1-1028d' in source for source in sources.values()):
        raise RuntimeError('Shared instance selection remains in a frozen action module')
    for filename, source in sources.items():
        ast.parse(source, filename=filename)
    ast.parse(wrapper, filename='cover_action.py')
    path = output / 'cover_action.py'
    with path.open('x', encoding='utf-8') as stream:
        stream.write(wrapper)
    # Reproduce the interpreter's initial imports from this actual directory,
    # before any game/input execution. operator.py previously shadowed stdlib.
    subprocess.run([sys.executable,'-E','-s','-B','-X','utf8','-c',
        'from pathlib import Path; import operator; assert operator.or_(1,2)==3'],
        cwd=output,check=True,capture_output=True,timeout=10,creationflags=subprocess.CREATE_NO_WINDOW)
    imports = subprocess.run([sys.executable,'-E','-s','-B','-X','utf8',str(path),
        '--cover-eye-observation','--equipment-chain','--playground-ready','--dual-output','--prepare-import-check'],
        cwd=ROOT,capture_output=True,timeout=20,creationflags=subprocess.CREATE_NO_WINDOW)
    (output/'import-check.log').write_bytes(imports.stdout+imports.stderr)
    imports.check_returncode()
    write_new(HERE / selected.name / 'operator-prepared.json', dict(
        instance=str(selected), operator=str(path), operator_sha256=sha(path), bootstrap_stdlib_passed=True,
        source_sha256={str(OLD / filename):sha(OLD / filename) for filename in sources},
        wrapper_source_sha256=sha(R104 / 'run_cover_eye_approach_operator.py'),
        generator_sha256=sha(__file__),
        entry_observation_source_sha256=sha(entry_observer),
        candidate_closing_source_sha256=sha(closing_observer),
        ordinary_approach_source_sha256=sha(approach_source), ordinary_approach_frozen_in_operator=True,
        syntax_checked=True, shared_instance_references=0,
        input_reader=str(output / 'read_private_input.py'), input_reader_sha256=receipt['input_reader_sha256'],
        observation_adapter=str(adapter), observation_adapter_sha256=sha(adapter),
        observation_adapter_source=str(adapter_source), observation_adapter_source_sha256=sha(adapter_source),
        actual_input_or_recording_started=False, gameplay_accepted=False,
        flags=['--cover-eye-observation','--equipment-chain','--playground-ready','--dual-output'],
        remaining='Launch/stop owner and one-shot source-stage environment still require explicit r105 runner.'))
    print(json.dumps(load(HERE / selected.name / 'operator-prepared.json')))


if __name__ == '__main__':
    main()
