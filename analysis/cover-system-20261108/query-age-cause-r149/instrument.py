"""Observe the existing prepare rejection without changing any guard."""
from pathlib import Path
R=Path(__file__).resolve().parents[3]
def once(s,old,new):
    assert s.count(old)==1,old
    return s.replace(old,new)
def ready(s):
    s=once(s,'namespace ready_wrist {','#include "cover_query_source_cause.h"\nnamespace ready_wrist {\nnamespace cause=gears_cover_query_source_cause;')
    old='void source_rejected(Entry const* e,tx::Stage stage,tx::Verdict verdict,unsigned causes) noexcept {\n'
    return once(s,old,old+'''    if(e&&cause::armed())cause::note({reinterpret_cast<uintptr_t>(e),e->invocation,unsigned(e->thread),unsigned(stage)},unsigned(verdict),causes);
''')
def query(s):
    s=once(s,'thread_local gears_cover_query_guard::Witness query_owner_guard;',
        'thread_local gears_cover_query_guard::Witness query_owner_guard;\nthread_local n::ready_wrist::cause::Witness query_source_cause;')
    s=once(s,'    gears_cover_query_guard::Witness owner_guard{};','    gears_cover_query_guard::Witness owner_guard{};\n    n::ready_wrist::cause::Witness source_cause{};')
    old='''    gears_cover_query_guard::Witness owner_witness{};
    struct PublishGuard {gears_cover_query_guard::Witness const& value;
        ~PublishGuard(){query_owner_guard=value;}} publish_guard{owner_witness};'''
    new='''    gears_cover_query_guard::Witness owner_witness{};
    n::ready_wrist::cause::Witness cause_witness{};
    struct PublishGuard {gears_cover_query_guard::Witness const& value;n::ready_wrist::cause::Witness const& cause;
        ~PublishGuard(){query_owner_guard=value;query_source_cause=cause;}} publish_guard{owner_witness,cause_witness};'''
    s=once(s,old,new)
    old='''        if(!s.context.current(n::tx::Stage::prepare))return reject(Gate::context,unsigned(s.context.rejection()),s.context.rejection_detail());'''
    new='''        {
            n::ready_wrist::cause::Scope observe(path_probe_requested||sphere_probe_requested,
                {reinterpret_cast<uintptr_t>(&entry),entry.invocation,unsigned(entry.thread),unsigned(n::tx::Stage::prepare)},
                unsigned(n::tx::Verdict::source_age_expired),cause_witness);
            if(!s.context.current(n::tx::Stage::prepare))return reject(Gate::context,unsigned(s.context.rejection()),s.context.rejection_detail());
        }'''
    s=once(s,old,new)
    s=once(s,'        common.owner_guard=query_owner_guard;','        common.owner_guard=query_owner_guard;\n        common.source_cause=query_source_cause;')
    old='''    // The finished row is published under the mutex after the packet write.'''
    new='''    auto const& cause=p.source_cause;
    fprintf(f,"cover_path_probe_source_age_cause=%u,%llu,%llu,%u,%u,%u,%u ",unsigned(cause.observed),
        uint64_t(cause.key.entry),cause.key.compose,cause.key.thread,cause.key.stage,cause.verdict,cause.causes);
    // The finished row is published under the mutex after the packet write.'''
    return once(s,old,new)
if __name__=='__main__':
    for name,fn in (('ready_wrist_entry_live.inl',ready),('cover_physics_c0_live.inl',query)):
        path=R/'tools/native-capture'/name;path.write_text(fn(path.read_text(encoding='utf-8-sig')),encoding='utf-8')
