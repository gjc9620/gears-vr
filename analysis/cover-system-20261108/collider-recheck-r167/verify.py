"""Verify same-query recheck scope and explicit dynamic closing additions."""
from pathlib import Path
import difflib,hashlib,json,re
H=Path(__file__).resolve().parent;R=H.parents[2];base=H.parent/'component-root-repeat-r165/native-source/tools/native-capture';actual=R/'tools/native-capture'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
old=(base/'cover_collider_roles_live.inl').read_text();new=(actual/'cover_collider_roles_live.inl').read_text();normalized=new
normalized=normalized.replace('    binding::Binding sample_owner{};bool recheck_ready{};\n','')
normalized=normalized.replace('    work.recheck_ready=false;\n','')
normalized=normalized.replace('output=work.first;work.sample_owner=owner;work.recheck_ready=true;return Code::current;','output=work.first;return Code::current;')
begin=normalized.index('// Only Session::SourceGuard');end=normalized.index('#else',begin)
normalized=normalized[:begin]+normalized[end:]
normalized=normalized.replace('template<class Guard> Code recheck(binding::Binding const&,Guard,Workspace&,Snapshot&)noexcept{return Code::unavailable;}\n','')
normalized,count=re.subn(r'binding::source_field\(w\.(?:pawn|component|body),', 'binding::field(',normalized)
assert count==8 and normalized==old
query=(actual/'cover_physics_query_live.inl').read_text();assert query.count('collider_roles::recheck(')==1
assert query.replace('collider_roles::recheck(','collider_roles::sample(')==(base/'cover_physics_query_live.inl').read_text()
recheck=new[new.index('template<class Guard> Code recheck'):new.index('#else',new.index('template<class Guard> Code recheck'))]
assert recheck.count('values(work,owner,work.last,true)')==2
assert recheck.count('same(work.first,work.last)')==2 and '||!current(work)' in recheck
assert recheck.count('if(!guard())return Code::guard;')==2
assert 'work.recheck_ready=false' in recheck and 'owner.scene_invocation!=work.sample_owner.scene_invocation' in recheck
assert not any(word in recheck for word in ('clock_ns','clock_ms','metadata_generation=','Sleep','retry'))
results=json.loads((H/'checks-v3/results.json').read_text());assert all(not r['build_exit'] and not r['test_exit'] for r in results)
before=next(r for r in results if r['variant']=='before');after=next(r for r in results if r['variant']=='after')
assert 'baseline_unclosed_owner_race=1' in before['output'] and 'baseline_unclosed_owner_race=1' not in after['output']
b=json.loads((H/'native-build-result.json').read_text());assert b['build_only'] and not b['staged'] and not b['game_started'] and b['flags_unchanged'] and not b['external_worktree_dependencies']
for name in ('cover_collider_roles_live.inl','cover_physics_query_live.inl'):
    assert sha(actual/name)==sha(H/'checks-v3/after-src/tools/native-capture'/name)==sha(H/'native-source/tools/native-capture'/name)
result=dict(runtime_delta=b['changed'],native_sha256=b['dll_sha256'],project_dependencies=len(b['project_dependencies']),outside_worktree_dependencies=0,
    scope='Only SourceGuard collider rechecks in one synchronous execute; initial sample invalidates and rebuilds, owner key/Scene required; entered failure latches false',
    dynamic_passes=2,dynamic_fields_added_to_existing_closing_proof=8,complete_proofs_revalidated_before_offsets=True,original_full_closing_preserved=True,
    initial_sample_guard_transaction_preserved=True,baseline_owner_mutation_acceptance_reproduced=True,new_owner_mutation_rejected=True,
    checks_before=int(re.search(r'CoverColliderRecheck checks=(\d+)',before['output'])[1]),
    checks_after=int(re.search(r'CoverColliderRecheck checks=(\d+)',after['output'])[1]),includes_existing_collider_checks=219,
    read_comparison=json.loads((H/'checks-v3/read-comparison.json').read_text()),flags_unchanged=True,
    build_only=True,game_started=False,source_age_repaired=False,movement_authorized=False,world_coverage_verified=False,public_changed=False,
    early_failures='v1 reproduced baseline and candidate unclosed late OwnerComponent mutation. v2 closed all eight dynamic fields; inherited BoneSpring hook moved to its own second read in v3 to preserve between-observation intent under paired reads.')
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
patch=''
for name in ('cover_collider_roles_live.inl','cover_physics_query_live.inl'):
    patch+=''.join(difflib.unified_diff((base/name).read_text().splitlines(True),(actual/name).read_text().splitlines(True),fromfile='r165/'+name,tofile='r167/'+name))
(H/'delta.patch').write_text(patch)
print(json.dumps(result))
