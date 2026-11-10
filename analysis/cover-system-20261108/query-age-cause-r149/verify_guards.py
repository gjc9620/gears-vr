"""Verify observer-only source transforms and unchanged guard/journal text."""
from pathlib import Path
import hashlib,json,subprocess
from instrument import ready,query
H=Path(__file__).resolve().parent;R=H.parents[2]
prefix='tools/native-capture/'
def query_function(s):
    a=s.index('template<class Shape,class Output,class Invoke> q::Code query(')
    b=s.index('\ntemplate<class Shape>',a)
    return s[a:b]
def restore_query(s):
    s=s.replace('    n::ready_wrist::cause::Witness cause_witness{};\n','')
    s=s.replace('''    struct PublishGuard {gears_cover_query_guard::Witness const& value;n::ready_wrist::cause::Witness const& cause;
        ~PublishGuard(){query_owner_guard=value;query_source_cause=cause;}} publish_guard{owner_witness,cause_witness};''',
'''    struct PublishGuard {gears_cover_query_guard::Witness const& value;
        ~PublishGuard(){query_owner_guard=value;}} publish_guard{owner_witness};''')
    s=s.replace('''        {
            n::ready_wrist::cause::Scope observe(path_probe_requested||sphere_probe_requested,
                {reinterpret_cast<uintptr_t>(&entry),entry.invocation,unsigned(entry.thread),unsigned(n::tx::Stage::prepare)},
                unsigned(n::tx::Verdict::source_age_expired),cause_witness);
            if(!s.context.current(n::tx::Stage::prepare))return reject(Gate::context,unsigned(s.context.rejection()),s.context.rejection_detail());
        }''','''        if(!s.context.current(n::tx::Stage::prepare))return reject(Gate::context,unsigned(s.context.rejection()),s.context.rejection_detail());''')
    return s
def journal(s):
    a=s.index('void source_rejected(');b=s.index('\nvoid begin(',a)
    return s[a:b]
rows=[]
for mode in ('canonical','frozen-r146'):
    before={};after={}
    for name,fn in (('ready_wrist_entry_live.inl',ready),('cover_physics_c0_live.inl',query)):
        before[name]=(subprocess.check_output(['git','show','e27ed298:'+prefix+name],cwd=R,text=True,encoding='utf-8')
            if mode=='canonical' else (H.parent/'current-measurement-r146/native-source'/prefix/name).read_text(encoding='utf-8-sig'))
        after[name]=((R if mode=='canonical' else H/'native-source')/prefix/name).read_text(encoding='utf-8-sig')
        assert fn(before[name])==after[name]
    assert restore_query(query_function(after['cover_physics_c0_live.inl']))==query_function(before['cover_physics_c0_live.inl'])
    note='    if(e&&cause::armed())cause::note({reinterpret_cast<uintptr_t>(e),e->invocation,unsigned(e->thread),unsigned(stage)},unsigned(verdict),causes);\n'
    assert journal(after['ready_wrist_entry_live.inl']).replace(note,'')==journal(before['ready_wrist_entry_live.inl'])
    rows.append(dict(mode=mode,query_guard_order_and_conditions_unchanged=True,original_commit_journal_filter_unchanged=True,
        before_sha256={p:hashlib.sha256(v.encode()).hexdigest() for p,v in before.items()},
        after_sha256={p:hashlib.sha256(v.encode()).hexdigest() for p,v in after.items()}))
(H/'guard-verification.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows))
