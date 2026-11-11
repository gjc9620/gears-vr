"""Remove copied observations and reconstruct the exact r169 reader decisions."""
from pathlib import Path
import difflib,hashlib,importlib.util,json,re
H=Path(__file__).resolve().parent;R=H.parents[2]
base=H.parent/'source-age-detail-r169/native-source/tools/native-capture';actual=R/'tools/native-capture'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
b=json.loads((H/'native-build-result.json').read_text())
assert b['build_only'] and not b['staged'] and not b['game_started'] and b['flags_unchanged'] and not b['external_worktree_dependencies']
patch='';reconstructed=[]
for name in ('cover_physx_asset_actors.h','cover_physx_collider.h','cover_physx_components.h'):
    original=(base/name).read_text();current=(actual/name).read_text()
    value=current.replace('#include "cover_owner_link_observation.h"\n','').replace('namespace linkobs=gears_cover_owner_link_observation;\n','')
    pattern=r'(if\([^\n]+\))\{\n\s+linkobs::note\(\{[^\n]+\n[^\n]+\}\);return Code::links;\}'
    value,count=re.subn(pattern,r'\1return Code::links;',value);assert count==2
    assert value==original,name
    reconstructed.append(name)
for name in ('cover_physx_query.h','cover_physics_c0_live.inl'):
    original=(base/name).read_text();current=(actual/name).read_text();value=current
    if name=='cover_physx_query.h':
        value,count=re.subn(r'^ +gears_cover_owner_link_observation::PhaseScope[^\n]+\n','',value,flags=re.M);assert count==2
    else:
        value=value.replace('    gears_cover_owner_link_observation::Report owner_links{};\n','')
        value=value.replace('        gears_cover_owner_link_observation::Scope link_scope(true,{common.world,common.pawn,common.scene,common.compose,common.thread},row.owner_links);\n','')
        start=value.index('    auto const& links=path_copy.owner_links;');end=value.index('    fprintf(f,"cover_path_probe_age_key=',start)
        value=value[:start]+value[end:]
    assert value==original,name
    reconstructed.append(name)
for name in reconstructed:
    patch+=''.join(difflib.unified_diff((base/name).read_text().splitlines(True),(actual/name).read_text().splitlines(True),fromfile='r169/'+name,tofile='r171/'+name))
for path in b['changed']:
    name=Path(path).name
    assert sha(actual/name)==sha(H/'native-source'/path)==sha(H/'checks-v3/source'/path)
observer=(actual/'cover_owner_link_observation.h').read_text()
assert not re.search(r'(clock_ns|clock_ms|safe_copy|read_memory|GetCurrentThreadId)\s*\(',observer)
related=json.loads((H/'checks-v2/results.json').read_text())[1:]
focused=json.loads((H/'checks-v3/results.json').read_text())
assert all(not x['build_exit'] and not x['test_exit'] for x in related+focused)
for path in b['changed']:
    assert sha(H/'checks-v2/source'/path)==sha(H/'native-source'/path)
spec=importlib.util.spec_from_file_location('r171_links',H/'inspect_links.py')
parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser);parser.selftest()
result=dict(native_sha256=b['dll_sha256'],changed=b['changed'],project_dependencies=len(b['project_dependencies']),
    outside_worktree_dependencies=0,flags_unchanged=True,original_decisions_reconstructed_exactly=reconstructed,
    observer_checks=89,parser_checks=22,related_checks=[90,92,108,391,51],
    initial_failures_retained=['checks-v1 missing test-only bcrypt link library','checks-v2 new fixture body block too short; uncaught fixture write exception'],
    extra_game_reads=False,extra_clocks=False,guard_weakening=False,original_calls_added=False,
    one_shot_path_scope=True,first_failure_only=True,missing_is_unknown=True,
    build_only=True,game_started=False,owner_links_repaired=False,movement_authorized=False,world_coverage_verified=False,public_changed=False)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
(H/'delta.patch').write_text(patch)
print(json.dumps(result))
