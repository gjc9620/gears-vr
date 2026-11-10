"""Check exact source scope, retained closing reads and actual saved evidence."""
from pathlib import Path
import hashlib,json,sys
from instrument import instrument
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before=(H/'before-cover_physics_roles_live.inl').read_text(encoding='utf-8-sig')
after=(R/'tools/native-capture/cover_physics_roles_live.inl').read_text(encoding='utf-8-sig')
assert after==instrument(before)
for start,end in (('inline bool layouts_current','inline Code values'),('template<class Guard> Code sample','#else')):
    assert before.split(start,1)[1].split(end,1)[0]==after.split(start,1)[1].split(end,1)[0]
old_values=before.split('inline Code values',1)[1].split('template<class Guard> Code sample',1)[0]
new_values=after.split('inline Code values',1)[1].split('template<class Guard> Code sample',1)[0]
assert new_values.replace('body_mesh_role(*l,next.mesh.cls,w.witness.pass)',
    'class_role(l->proof,next.mesh.cls,L"SkeletalMeshComponent",l->fields[1].reference.address)')==old_values
build=load(H/'native-build-result.json')
assert build['changed']==['tools/native-capture/cover_physics_roles_live.inl'] and build['flags_unchanged']
assert not build['external_worktree_dependencies'] and len(build['project_dependencies'])==409
for folder in ('checks-v2','checks-v3'):
    for result in load(H/folder/'results.json'):
        assert result['build_exit']==result['test_exit']==0 and not result['outside_worktree_dependencies']
sys.path.insert(0,str(H.parent/'query-age-cause-r149'))
from inspect_cause import inspect
saved=load(H.parent/'query-age-cause-r149/runtime-r155/session-original.json')
assert inspect(saved)['source_age_cause']['causes']==1
try:inspect(saved,build['dll_sha256']);raise RuntimeError('Old Native evidence accepted for new artifact')
except (AssertionError,ValueError):pass
result=dict(runtime_delta=build['changed'],dll_sha256=build['dll_sha256'],project_dependencies=409,
    outside_worktree_dependencies=0,flags_unchanged=True,sample_transaction_unchanged=True,
    closing_layout_proof_unchanged=True,all_dynamic_body_reads_unchanged=True,
    two_pass_mutation_checks=True,read_comparison=load(H/'checks-v3/read-comparison.json'),
    previous_failures='checks-v1 lacked copied CPU fixture sources; retained. No product gate changed for this setup failure.',
    old_evidence_rejected_for_new_native=True,build_only=True,game_started=False,
    query_latency_verified=False,source_age_repaired=False,movement_authorized=False,headset_accepted=False)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
