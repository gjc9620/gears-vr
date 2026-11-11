"""Verify query-scoped physics rechecks, complete closure and frozen provenance."""
from pathlib import Path
import difflib,hashlib,json,re
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base=H.parent/'actor-backlink-r173/native-source';actual=R/'tools/native-capture'
build=load(H/'native-build-result.json');manifest=load(H/'native-source-manifest.json')['files']
baseline=load(H.parent/'actor-backlink-r173/native-source-manifest.json')['files']
assert build['build_only'] and not build['staged'] and not build['game_started'] and build['flags_unchanged']
assert not build['external_worktree_dependencies'] and len(build['project_dependencies'])==413
names=('cover_physics_roles_live.inl','cover_physics_query_live.inl')
assert set(build['changed'])=={'tools/native-capture/'+name for name in names}
for p,digest in manifest.items():
    assert sha(H/'native-source'/p)==digest
    if p not in build['changed']:assert baseline[p]==digest
for name in names:
    assert sha(actual/name)==sha(H/'native-source/tools/native-capture'/name)==sha(H/'checks-v4/after-src/tools/native-capture'/name)
query=(actual/names[1]).read_text();assert query.count('roles::recheck(')==2 # physics and collider qualified names
assert query.replace('return roles::recheck(actor,metadata,owner,','return roles::sample(actor,metadata,owner,')==(base/'tools/native-capture'/names[1]).read_text()
source=(actual/names[0]).read_text();start=source.index('template<class Guard> Code recheck');end=source.index('#else',start);repeat=source[start:end]
assert repeat.count('values(work,owner,expected,work.last,true)')==2 and repeat.count('same(work.first,work.last)')==2
assert repeat.count('if(!guard())return Code::guard;')==2 and '||!layouts_current(work)' in repeat
assert 'work.recheck_ready=false' in repeat and 'owner.scene_invocation!=work.sample_owner.scene_invocation' in repeat
assert 'pose::metadata_equal(expected,work.sample_metadata)' in repeat and 'output=work.last;work.recheck_ready=true' in repeat
assert not any(x in repeat for x in ('clock_ns','clock_ms','Sleep','retry','metadata_generation='))
values=source[source.index('inline Code values'):source.index('template<class Guard> Code sample')]
assert values.count('binding::source_field(')==12
assert 'for(unsigned n=0;n<w.layout_count;++n)if(!binding::current(w.layouts[n].proof))return Code::changed;' in values
assert 'if(repeat){if(!binding::same(next.owner,w.first.owner))return Code::changed;}' in values
assert 'l->mesh_role_class!=next.mesh.cls' in values and 'l->mesh_role_reference!=l->fields[1].reference.address' in values
sample=source[source.index('template<class Guard> Code sample'):start]
assert 'work.recheck_ready=false' in sample and 'work.members.clear();work.mesh={};work.instance={};' in sample
assert '!same(work.first,work.last)||!layouts_current(work)' in sample and 'output=work.first;work.sample_owner=owner;' in sample
rows=load(H/'checks-v4/results.json');assert all(not x['build_exit'] and not x['test_exit'] and not x['outside_worktree_dependencies'] for x in rows)
before=next(x for x in rows if x['variant']=='before');after=next(x for x in rows if x['name']=='cover_physics_recheck' and x['variant']=='after')
assert before['output'].count('baseline_unclosed_dynamic=')==12 and 'baseline_unclosed_dynamic=' not in after['output']
comparison=load(H/'checks-v4/read-comparison.json');assert all(x['after']<x['before'] for x in comparison.values())
result=dict(native_sha256=build['dll_sha256'],runtime_delta=build['changed'],project_dependencies=413,outside_worktree_dependencies=0,
    flags_unchanged=True,other_frozen_sources_unchanged=True,source_times_or_limits_changed=False,
    scope='Only SourceGuard physics rechecks inside the same synchronous execute; initial sample always invalidates and rebuilds',
    dynamic_passes=2,initial_owner_scene_full_metadata_bound=True,failed_recheck_latches_false=True,
    prior_proofs_before_offsets=True,original_complete_closing_preserved=True,
    dynamic_source_fields='7 mesh/instance/header/root values plus 5 per Body including actual array member, index, owner, BodyData and BoneSpring',
    dynamic_closing_races_reproduced_before=12,dynamic_closing_races_rejected_after=12,
    checks_before=int(re.search(r'CoverPhysicsRecheck checks=(\d+)',before['output'])[1]),
    checks_after=int(re.search(r'CoverPhysicsRecheck checks=(\d+)',after['output'])[1]),includes_existing_physics_checks=282,
    read_comparison=comparison,related_checks=[219,207,23,105,391,53,51],
    earlier_failures='v1 all correctness checks passed but 64-body read count increased1.10%; v2 reused already revalidated owner/mesh class paths. v3 expanded tests misclassified BoneSpring mutation as late: read hook runs before copy, so original already rejects that case; v4 retains that rejection. Final source also closes every Bodies array slot.',
    build_only=True,game_started=False,source_age_repaired=False,owner_links_game_repaired=False,
    movement_authorized=False,world_coverage_verified=False,public_changed=False)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
patch=''
for name in names:patch+=''.join(difflib.unified_diff((base/'tools/native-capture'/name).read_text().splitlines(True),(actual/name).read_text().splitlines(True),fromfile='r173/'+name,tofile='r175/'+name))
(H/'delta.patch').write_text(patch)
print(json.dumps(result))
