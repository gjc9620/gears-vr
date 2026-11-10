"""Verify frozen scope, dynamic reads and retained transaction closures."""
from pathlib import Path
import difflib,hashlib,json,re
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def dynamic_calls(text):
    calls=[]
    for match in re.finditer(r'\b(binding::identity|binding::field|safe_copy|instance)\(',text):
        start=match.end();depth=1;end=start
        while depth:
            if text[end]=='(':depth+=1
            if text[end]==')':depth-=1
            end+=1
        calls.append((match.group(1),re.sub(r'\s+','',text[start:end])))
    return calls
patches=[]
for name in ('cover_physics_roles_live.inl','cover_collider_roles_live.inl'):
    before=(H/('before-'+name)).read_text(encoding='utf-8-sig');after=(R/'tools/native-capture'/name).read_text(encoding='utf-8-sig')
    assert after==(H/'checks-v1/after-src/tools/native-capture'/name).read_text(encoding='utf-8-sig')
    assert dynamic_calls(before)==dynamic_calls(after)
    old_sample=before.split('template<class Guard> Code sample',1)[1].split('#else',1)[0]
    new_sample=after.split('template<class Guard> Code sample',1)[1].split('#else',1)[0]
    assert new_sample.replace('work.last,true)','work.last)')==old_sample
    marker='inline bool layouts_current' if 'physics_roles' in name else 'inline bool current'
    end='inline Code values' if 'physics_roles' in name else 'template<class Guard> Code sample'
    assert before.split(marker,1)[1].split(end,1)[0]==after.split(marker,1)[1].split(end,1)[0]
    patches.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='before/'+name,tofile='after/'+name))
(H/'delta.patch').write_text(''.join(patches),encoding='utf-8')
for result in load(H/'checks-v1/results.json'):
    assert result['build_exit']==result['test_exit']==0 and not result['outside_worktree_dependencies']
b=load(H/'native-build-result.json')
assert set(b['changed'])=={'tools/native-capture/cover_physics_roles_live.inl','tools/native-capture/cover_collider_roles_live.inl'}
assert not b['external_worktree_dependencies'] and b['flags_unchanged'] and len(b['project_dependencies'])==409
result=dict(runtime_delta=b['changed'],dll_sha256=b['dll_sha256'],project_dependencies=409,
    outside_worktree_dependencies=0,flags_unchanged=True,all_dynamic_read_calls_unchanged=True,
    sample_guard_transaction_unchanged_except_second_pass_selection=True,complete_closing_proofs_unchanged=True,
    reuse_scope='Second values pass only; exact first identities and complete Proof::current required before cached offsets',
    null_body_transition_rejected=True,first_and_second_pass_schema_mutations_rejected=True,
    read_comparison=load(H/'checks-v1/read-comparison.json'),build_only=True,staged=False,game_started=False,
    query_latency_verified=False,source_age_repaired=False,world_coverage_verified=False,movement_authorized=False,headset_accepted=False)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
