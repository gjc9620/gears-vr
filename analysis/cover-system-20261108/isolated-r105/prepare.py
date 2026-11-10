"""Prepare the cover candidate in a new private namespace; never starts a game."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
R104 = HERE.parent / 'source-stage-r104'
OLD = ROOT / 'analysis/solo-m1-20261028'
FROZEN = OLD / 'equipment-controls-prepared-20261110-cover-source-stage-r104'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def write_new(path, value):
    with Path(path).open('x', encoding='utf-8') as stream:
        stream.write(json.dumps(value, indent=2))


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise RuntimeError('Frozen source boundary changed: ' + old[:100])
    return source.replace(old, new)


def input_reader(selected):
    # Reuse the actual Source1 shared-memory reader, but select the publisher
    # from this owned session. Never enumerate a machine-wide unique XR.
    source = (OLD / 'read_private_input.py').read_text(encoding='utf-8')
    source = replace_once(source,
        "ROOT=Path(__file__).resolve().parents[2];OUT=Path(__file__).resolve().parent",
        f"ROOT=Path({str(ROOT)!r});OUT=Path(__file__).resolve().parent")
    begin = source.index('control=WindowsControl();cfg=')
    end = source.index('try:\n    pid,start=', begin)
    source = source[:begin] + f'''control=WindowsControl()
OUT=Path({str(selected / 'cover-output')!r})
state=json.loads(Path({str(selected / 'launcher-session.json')!r}).read_text())
assert Path(state['instance_root']).resolve()==Path({str(selected)!r}).resolve()
session=json.loads(Path(state['session_report']).read_text())
assert state['phase'] in ('session_owned','waiting_for_level') and session['phase']=='running', ('indexed/session phase',state.get('phase'),session.get('phase'))
assert state['session_id']==session['session_id']
assert state['owner_identity']==session['owner_identity']
assert state['game_identity']==session['game_identity']
assert state.get('vr_gamepad',{{}}).get('source')==1
game=state['game_identity'];publisher=session['xr_identity']
manifest=json.loads(Path({str(selected / 'manifest.json')!r}).read_text())
expected=(ROOT/manifest['xr_build']/'OpenXRPoseProbe.exe').resolve()
assert Path(publisher['image']).resolve()==expected
for identity in (game,publisher,state['owner_identity']):
    assert control.identity(identity['pid'])==identity
reader=w.Reader(game,publisher,control.identity)
''' + source[end:]
    compile(source, 'read_private_input.py', 'exec')
    if 'Get-CimInstance' in source or 'solo-m1-1028d' in source:
        raise RuntimeError('Global runtime selection remains in input reader')
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', required=True)
    parser.add_argument('--native-r106', action='store_true')
    parser.add_argument('--native-r111', action='store_true')
    parser.add_argument('--native-r117', action='store_true')
    parser.add_argument('--native-r122', action='store_true')
    parser.add_argument('--native-r124', action='store_true')
    parser.add_argument('--native-r126', action='store_true')
    args = parser.parse_args()
    if sum((args.native_r106,args.native_r111,args.native_r117,args.native_r122,args.native_r124,args.native_r126))>1:
        raise ValueError('Select one frozen Native')
    if not args.name.startswith('cover-'):
        raise ValueError('Cover owns only its cover- prefixed new instance')
    if Path.cwd().resolve() != ROOT or sys.flags.optimize:
        raise RuntimeError('Use the canonical worktree without optimization')
    spec = importlib.util.spec_from_file_location('cover_isolated_sim', ROOT / 'tools/multi-instance/isolated_sim.py')
    isolated = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(isolated)
    selected = isolated.instance(args.name)
    if selected.exists():
        raise RuntimeError('Fresh name required; never replace an old attempt')
    prepared = load(FROZEN / 'prepared.json')
    expected = load(R104 / 'plan.json')['build_sha256']
    if args.native_r106:
        current = HERE.parent / 'sweep-research-r106'
        receipt = load(current / 'native-build-result.json')
        if (receipt['changed'] != ['tools/native-capture/cover_physx_query_code.h']
                or receipt['external_worktree_dependencies'] or not receipt['flags_unchanged']):
            raise RuntimeError('r106 frozen delta differs')
        expected['native'] = receipt['dll_sha256']
        prepared['builds']['native'] = dict(saved=str(current/'native-build/GearsCapture.dll'), sha256=expected['native'])
    if args.native_r111 or args.native_r117 or args.native_r122 or args.native_r124 or args.native_r126:
        current = HERE.parent / ('pose-probe-r126' if args.native_r126 else 'layout-witness-r124' if args.native_r124 else 'overlap-query-r122' if args.native_r122 else 'overlap-collector-r117' if args.native_r117 else 'self-evidence-r111')
        receipt = load(current / 'native-build-result.json')
        overlays = ('cover_physx_overlap_stream.h','cover_physx_query.h','cover_physx_stream.h') if args.native_r117 else (
            'cover_physx_self_evidence.h','cover_physx_query.h','cover_physics_query_live.inl')
        if args.native_r122:
            overlays=('cover_physx_overlap_code.h','cover_physx_shape_owner.h','cover_physx_query.h','cover_physics_query_live.inl','cover_physics_c0_live.h','cover_physics_c0_live.inl')
        if args.native_r124:
            overlays=('cover_physics_roles_live.inl','cover_physics_query_live.inl','cover_physics_c0_live.inl')
        if args.native_r126:
            overlays=('body_c0_goal_policy_live.inl',)
        changed = {'tools/native-capture/'+name for name in overlays}
        if (set(receipt['changed']) != changed or receipt['external_worktree_dependencies']
                or not receipt['flags_unchanged'] or not receipt['build_only'] or receipt['staged']):
            raise RuntimeError('Selected frozen Native delta differs')
        expected['native'] = receipt['dll_sha256']
        prepared['builds']['native'] = dict(saved=str(current/'native-build/GearsCapture.dll'), sha256=expected['native'])
    for key in ('native', 'playground', 'sim_xr'):
        item = prepared['builds'][key]
        if item['sha256'] != expected[key] or sha(item['saved']) != expected[key]:
            raise RuntimeError('Frozen cover artifact differs: ' + key)
    previous = load(HERE.parent / 'sphere-call-r102/runtime/result.json')
    if not (previous['normal_cleanup_verified'] and previous['baseline_restored'] and previous['runtime_released']):
        raise RuntimeError('Own previous restoration is not closed')
    baseline_path = Path(previous['baseline_path'])
    if sha(baseline_path) != previous['baseline_sha256']:
        raise RuntimeError('Own original baseline changed')
    baseline = load(baseline_path)
    manifest_row, config_row = baseline['files'][:2]
    for row in (manifest_row, config_row):
        if sha(row['saved']) != row['sha256']:
            raise RuntimeError('Original profile snapshot changed')
    # Only the saved manifest schema/settings are reused; all three executable
    # artifacts below are this cover candidate's immutable compiled copies.
    manifest = load(manifest_row['saved'])
    for key, prefix in (('native', 'native'), ('playground', 'playground'), ('sim_xr', 'xr')):
        artifact = Path(prepared['builds'][key]['saved'])
        manifest[prefix + '_build'] = str(artifact.parent)
        manifest[prefix + '_sha256'] = expected[key]
        if prefix != 'xr':
            manifest[prefix + '_dll'] = artifact.name
    manifest['candidate_validation'] = 'Cover isolated source-stage observation; selected frozen Native, no gameplay acceptance'
    manifest['candidate_id'] = args.name
    manifest['private_main_grip_ab'] = False
    manifest['lancer_interaction_ranges'] = False
    inputs = HERE / args.name
    inputs.mkdir()
    write_new(inputs / 'manifest.json', manifest)
    write_new(inputs / 'config.json', load(config_row['saved']))
    # The sealed r104 executable archive contains only the EXE. Include its
    # actual deployed OpenXR loader dependency in a new, immutable input dir.
    xr_input = inputs / 'xr'
    xr_input.mkdir()
    xr_exe = Path(prepared['builds']['sim_xr']['saved'])
    isolated.original.copy_stable(xr_exe, xr_input / xr_exe.name)
    loader = ROOT / 'tools/openxr-probe/build-solo-m1-sim/openxr_loader.dll'
    loader_sha = '66946ea6e9cc2649e299ec412521c5eafe56c635d05055097fe4db81e11932df'
    if sha(loader) != loader_sha:
        raise RuntimeError('Deployed OpenXR loader identity differs')
    isolated.original.copy_stable(loader, xr_input / loader.name)
    manifest['xr_build'] = str(xr_input)
    (inputs / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    selected = isolated.prepare(args.name, manifest=inputs / 'manifest.json', config=inputs / 'config.json')
    # Preserve the cover test's explicitly authorized 0.4 profile. This writes
    # only our newly prepared config; the isolation tool's common default stays.
    config = load(selected / 'config.json')
    config['render_scale'] = .4
    (selected / 'config.json').write_text(json.dumps(config, indent=2), encoding='utf-8')
    output = selected / 'cover-output'
    output.mkdir()
    reader = input_reader(selected)
    (output / 'read_private_input.py').write_text(reader, encoding='utf-8')
    resulting = load(selected / 'manifest.json')
    for prefix, filename in (('native', resulting['native_dll']), ('playground', resulting['playground_dll']), ('xr', 'OpenXRPoseProbe.exe')):
        path = (ROOT / resulting[prefix + '_build'] / filename).resolve()
        if sha(path) != resulting[prefix + '_sha256']:
            raise RuntimeError('Prepared artifact differs: ' + prefix)
        if prefix == 'native' and not path.is_relative_to(selected):
            raise RuntimeError('Native escaped private root')
        if prefix != 'native' and path.parent.name != 'build-' + args.name:
            raise RuntimeError('Shared build output selected')
    write_new(inputs / 'prepared.json', dict(
        instance=str(selected), output=str(output), started=False,
        native_sha256=expected['native'], playground_sha256=expected['playground'], xr_sha256=expected['sim_xr'],
        manifest_sha256=sha(selected / 'manifest.json'), config_sha256=sha(selected / 'config.json'),
        input_reader_sha256=sha(output / 'read_private_input.py'),
        preparation_source_sha256=sha(__file__), isolation_source_sha256=sha(ROOT / 'tools/multi-instance/isolated_sim.py'),
        input_reader_source_sha256=sha(OLD / 'read_private_input.py'),
        openxr_loader_sha256=loader_sha,
        render_scale=.4, prepared_at_ns=time.time_ns(),
        operator_migration_complete=False, runtime_admission_consumed=False,
        collision_verified=False, movement_authorized=False, gameplay_accepted=False,
        remaining='Migrate existing ordinary cover operator and exact-owned paired recording, then one actual source-stage query; normal private stop required.'))
    print(json.dumps(load(inputs / 'prepared.json')))


if __name__ == '__main__':
    main()
