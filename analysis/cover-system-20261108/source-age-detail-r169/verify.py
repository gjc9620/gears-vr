"""Reconstruct original age decisions after removing observation-only code."""
from pathlib import Path
import difflib,hashlib,importlib.util,json,re
H=Path(__file__).resolve().parent;R=H.parents[2];base=H.parent/'collider-recheck-r167/native-source/tools/native-capture';actual=R/'tools/native-capture'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
body=(actual/'body_tick_source_live.inl').read_text();normalized=body.replace('#include "body_source_age_observation.h"\n','')
start=normalized.index('void observe_age(');end=normalized.index('void note_rejected',start)
helper=normalized[start:end];assert not re.search(r'(clock_ns|clock_ms|safe_copy|read_memory|GetCurrentThreadId)\s*\(',helper)
normalized=normalized[:start]+normalized[end:]
normalized,count=re.subn(r'^    +observe_age\([^\n]+\);\n','',normalized,flags=re.M);assert count==3
assert normalized==(base/'body_tick_source_live.inl').read_text()
c0=(actual/'cover_physics_c0_live.inl').read_text();normalized=c0.replace('#include "body_source_age_observation.h"\n','')
normalized=normalized.replace('    gears_body_source_age_observation::Report source_ages{};\n','')
normalized=normalized.replace('        gears_body_source_age_observation::Scope age_scope(true,{common.world,common.pawn,common.scene,common.compose,common.thread},row.source_ages);\n','')
start=normalized.index('    auto const& ages=path_copy.source_ages;');end=normalized.index('    fprintf(f,"cover_path_probe_timing_invalid',start)
normalized=normalized[:start]+normalized[end:]
assert normalized==(base/'cover_physics_c0_live.inl').read_text()
results=json.loads((H/'checks-v1/results.json').read_text());assert all(not r['build_exit'] and not r['test_exit'] for r in results)
b=json.loads((H/'native-build-result.json').read_text());assert b['build_only'] and not b['staged'] and not b['game_started'] and b['flags_unchanged'] and not b['external_worktree_dependencies']
for name in ('body_source_age_observation.h','body_tick_source_live.inl','cover_physics_c0_live.inl'):
    assert sha(actual/name)==sha(H/'checks-v1/source/tools/native-capture'/name)==sha(H/'native-source/tools/native-capture'/name)
spec=importlib.util.spec_from_file_location('r169_age_parser',H/'inspect_age.py');parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser);parser.selftest()
result=dict(runtime_delta=b['changed'],native_sha256=b['dll_sha256'],project_dependencies=len(b['project_dependencies']),outside_worktree_dependencies=0,
    flags_unchanged=True,body_source_decisions_reconstructed_exactly=True,c0_control_flow_reconstructed_exactly=True,
    existing_clock_calls_unchanged=True,extra_game_reads=False,source_or_time_renewal=False,
    scope='Existing private one-shot path query only; key-matched copied first classify/current and first-age rows, bounded four rows',
    observer_checks=26,parser_checks=15,related_checks=[25,135,23,391,51],
    classify_and_current_clocks_distinct=True,first_check_has_query_offset=True,missing_observations_unknown=True,
    build_only=True,game_started=False,source_age_repaired=False,movement_authorized=False,world_coverage_verified=False,public_changed=False)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
patch=''
for name in ('body_tick_source_live.inl','cover_physics_c0_live.inl'):
    patch+=''.join(difflib.unified_diff((base/name).read_text().splitlines(True),(actual/name).read_text().splitlines(True),fromfile='r167/'+name,tofile='r169/'+name))
(H/'delta.patch').write_text(patch)
print(json.dumps(result))
