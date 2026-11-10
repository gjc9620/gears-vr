"""Freeze the existing ordinary cover action for the exact r105 private root."""
from pathlib import Path
import ast
import argparse
import json
import sys
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
    sources = {}
    for filename in ('cover_boundary_chain.py', 'equipment_controller_chain.py', 'run_menu_firstperson_check.py'):
        sources[filename] = (OLD / filename).read_text(encoding='utf-8')
    boundary = sources['cover_boundary_chain.py']
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
    for filename in sources:
        expression = f"s = (O / '{filename}').read_text(encoding='utf-8')"
        wrapper = replace_once(wrapper, expression, 's = ' + repr(sources[filename]))
    # The generated action has its own read-only input helper and result paths.
    # Existing recording modules consume its supplied own directory/identities.
    if any('solo-m1-1028d' in source for source in sources.values()):
        raise RuntimeError('Shared instance selection remains in a frozen action module')
    for filename, source in sources.items():
        ast.parse(source, filename=filename)
    ast.parse(wrapper, filename='operator.py')
    path = output / 'operator.py'
    with path.open('x', encoding='utf-8') as stream:
        stream.write(wrapper)
    write_new(HERE / selected.name / 'operator-prepared.json', dict(
        instance=str(selected), operator=str(path), operator_sha256=sha(path),
        source_sha256={str(OLD / filename):sha(OLD / filename) for filename in sources},
        wrapper_source_sha256=sha(R104 / 'run_cover_eye_approach_operator.py'),
        generator_sha256=sha(__file__),
        syntax_checked=True, shared_instance_references=0,
        input_reader=str(output / 'read_private_input.py'), input_reader_sha256=receipt['input_reader_sha256'],
        actual_input_or_recording_started=False, gameplay_accepted=False,
        flags=['--cover-eye-observation','--equipment-chain','--playground-ready','--dual-output'],
        remaining='Launch/stop owner and one-shot source-stage environment still require explicit r105 runner.'))
    print(json.dumps(load(HERE / selected.name / 'operator-prepared.json')))


if __name__ == '__main__':
    main()
