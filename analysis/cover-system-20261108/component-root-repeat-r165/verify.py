"""Verify unchanged dynamic operations, closing proofs and per-sample clearing."""
from pathlib import Path
import difflib,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
rel=Path('tools/native-capture/cover_component_roles_live.inl')
base=(H.parent/'query-timing-r163/native-source'/rel).read_text(encoding='utf-8-sig')
new=(R/rel).read_text(encoding='utf-8-sig')
normalized=new
block='''    // Only the second values pass may reuse this root's selected schema.
    // Re-read its complete first-pass proof before consuming any offset.
    // A new/reordered actor node cannot consume another root's cached fields.
    if(repeat){
        if(index>=w.first.root_count||!binding::same(next.actor,w.first.roots[index].actor)||
           !binding::metadata_current()||w.metadata!=binding::metadata_bytes||w.generation!=binding::metadata_generation||
           !binding::current(p))return Code::changed;
    }else if'''
assert normalized.count(block)==1;normalized=normalized.replace(block,'    if')
for old,replacement in (('Root& output,bool repeat=false)','Root& output)'),('Snapshot& output,bool repeat=false)','Snapshot& output)'),
    ('if(!repeat&&!array_schema','if(!array_schema'),('next.roots[i],repeat)','next.roots[i])'),('values(work,owner,work.last,true)','values(work,owner,work.last)')):
    assert normalized.count(old)==1;normalized=normalized.replace(old,replacement)
assert normalized==base
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for tree in ('checks-v1/after-src','checks-v4/after-src','native-source'):assert sha(R/rel)==sha(H/tree/rel)
def segment(text,begin,end):return text[text.index(begin):text.index(end,text.index(begin))]
assert segment(base,'inline bool current(Workspace const&','template<class Guard>')==segment(new,'inline bool current(Workspace const&','template<class Guard>')
assert segment(base,'    std::uintptr_t world{},base_actor{};','inline bool attachment_schema').replace('if(!array_schema','if(!repeat&&!array_schema')==segment(new,'    std::uintptr_t world{},base_actor{};','inline bool attachment_schema')
v1=json.loads((H/'checks-v1/results.json').read_text());v4=json.loads((H/'checks-v4/results.json').read_text())
assert all(not r['build_exit'] and not r['test_exit'] for r in v1[2:])
assert all(not r['build_exit'] and not r['test_exit'] for r in v4)
b=json.loads((H/'native-build-result.json').read_text());assert b['build_only'] and not b['staged'] and not b['game_started'] and b['flags_unchanged'] and not b['external_worktree_dependencies']
result=dict(runtime_delta=b['changed'],native_sha256=b['dll_sha256'],project_dependencies=len(b['project_dependencies']),
    outside_worktree_dependencies=0,flags_unchanged=True,control_flow_reconstructed_exactly=True,
    dynamic_root_reads_unchanged=True,complete_closing_proofs_and_clear_unchanged=True,
    reuse_scope='Second values pass, same actor at same root index; complete first root proof current before any offset; no cross-sample cache',
    focused_checks=346,focused_includes_existing_component_checks=207,existing_consumer_checks=[207,23,105,391,53,51],
    before_after_read_comparison=json.loads((H/'checks-v4/read-comparison.json').read_text()),
    initial_failures='v1-v3 focused fixture names for index704 overwrote original body at arena+0x4c000; mutation nth2 was still first dynamic double-read. v4 uses indices1000+ and occurrence3 for the second phase. Runtime delta unchanged.',
    build_only=True,game_started=False,source_age_repaired=False,world_coverage_verified=False,movement_authorized=False,public_changed=False)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
(H/'delta.patch').write_text(''.join(difflib.unified_diff(base.splitlines(True),new.splitlines(True),fromfile='r163/'+str(rel),tofile='r165/'+str(rel))))
print(json.dumps(result))
