"""Verify observer-only runtime delta against the actual frozen r158 files."""
from pathlib import Path
import hashlib,json,re,difflib
H=Path(__file__).resolve().parent;R=H.parents[2];base=H.parent/'schema-repeat-r158/native-source/tools/native-capture'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
actual=R/'tools/native-capture'
text=(actual/'cover_physics_query_live.inl').read_text(encoding='utf-8-sig')
normalized,n=re.subn(r'timing::measure\(timing::Stage::\w+,\[&\]\(\)\{return ([^\n]+?);\}\)',r'\1',text)
assert n==7
for line in ('#include "cover_query_timing.h"\n','namespace timing=gears_cover_query_timing;\n','        timing::Span closing_timing(timing::Stage::closing);\n'):
    assert normalized.count(line)==1;normalized=normalized.replace(line,'')
assert normalized==(base/'cover_physics_query_live.inl').read_text(encoding='utf-8-sig')
c0=(actual/'cover_physics_c0_live.inl').read_text(encoding='utf-8-sig')
removed=('    gears_cover_query_timing::Report timing{};\n',
    '    fprintf(f,"cover_path_probe_timing_invalid=%llu cover_path_probe_timing_overflow=%llu ",path_copy.timing.invalid,path_copy.timing.overflow);\n',
    '    for(unsigned i=0;i<unsigned(gears_cover_query_timing::Stage::count);++i){auto const& bin=path_copy.timing.bins[i];\n        fprintf(f,"cover_path_probe_timing_%u=%llu,%llu,%llu,%llu ",i,bin.calls,bin.completed,bin.total_ns,bin.max_ns);}\n',
    '        gears_cover_query_timing::Scope timing_scope(true,row.timing,+[]()noexcept{return rig_tracking_live::clock_ns();});\n')
for line in removed:assert c0.count(line)==1;c0=c0.replace(line,'')
assert c0==(base/'cover_physics_c0_live.inl').read_text(encoding='utf-8-sig')
build=json.loads((H/'native-build-result.json').read_text())
assert build['build_only'] and not build['staged'] and not build['game_started'] and not build['external_worktree_dependencies'] and build['flags_unchanged']
for name in ('cover_query_timing.h','cover_physics_query_live.inl','cover_physics_c0_live.inl'):
    assert sha(actual/name)==sha(H/'native-source/tools/native-capture'/name)==sha(H/'checks-v2/source/tools/native-capture'/name)
v1=json.loads((H/'checks-v1/results.json').read_text());v2=json.loads((H/'checks-v2/results.json').read_text())
assert v1[0]['build_exit']==2 and all(not r['build_exit'] and not r['test_exit'] for r in v1[1:])
assert len(v2)==1 and not v2[0]['build_exit'] and not v2[0]['test_exit']
result=dict(runtime_delta=build['changed'],native_sha256=build['dll_sha256'],
    project_dependencies=len(build['project_dependencies']),outside_worktree_dependencies=0,
    original_query_control_flow_reconstructed_exactly=True,original_c0_control_flow_reconstructed_exactly=True,
    flags_unchanged=True,scope='Only the existing private one-shot path probe arms timing; disabled callers do not read a clock',
    timing_bins=8,nested_times_overlap=True,closed_spans_do_not_mean_success=True,
    initial_test_compile_failure='Two test-local identifiers shadowed existing fixture globals under /WX; renamed only those identifiers for v2',
    focused_checks=135,focused_includes_existing_session_checks=105,
    affected_existing_checks=[23,105,391,53,51],build_only=True,game_started=False,source_age_repaired=False,
    world_coverage_verified=False,movement_authorized=False,headset_accepted=False,public_changed=False)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
patch=''
for name in ('cover_physics_query_live.inl','cover_physics_c0_live.inl'):
    patch+=''.join(difflib.unified_diff((base/name).read_text().splitlines(True),(actual/name).read_text().splitlines(True),fromfile='r158/'+name,tofile='r163/'+name))
(H/'delta.patch').write_text(patch)
print(json.dumps(result))
