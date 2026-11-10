"""Same bounded diagnostic transform on canonical and frozen measurement paths.
No predicate, ordering, source choice, or freshness policy is changed.
"""
from pathlib import Path
R=Path(__file__).resolve().parents[3]
DECL='''enum class CoverProbeStep:unsigned {stopping,idle,current_before,key_before,type,slot,link,marker,measurement,key_after,same_key_after,current_after,stopping_after,revision_after,entry,complete,exception,count};
struct CoverProbeRow {uint64_t scene{},compose{},sequence{},world{},pawn{};unsigned detail{~0u};};
struct CoverProbeSlot {std::atomic<unsigned> state{};std::atomic<uint64_t> count{};CoverProbeRow row{};};
std::array<CoverProbeSlot,unsigned(CoverProbeStep::count)> cover_probe_rows;
void note_cover_probe(CoverProbeStep step,unsigned detail=~0u)noexcept{
    auto& slot=cover_probe_rows[unsigned(step)];++slot.count;
    unsigned expected=0;if(!slot.state.compare_exchange_strong(expected,1))return;
    auto const& s=n::state;slot.row={s.context.source.scene_invocation,s.context.copy.invocation,
        s.context.source.stereo.sequence,s.context.source.world.address,s.context.source.pawn.address,detail};
    slot.state.store(2,std::memory_order_release);
}
'''
SUMMARY='''    // First immutable witness per gate; no lock or extra game read.
    for(unsigned i=0;i<cover_probe_rows.size();++i){auto const& slot=cover_probe_rows[i];
        fprintf(f,"cover_pose_probe_count_%u=%llu ",i,slot.count.load());
        if(slot.state.load(std::memory_order_acquire)==2){auto const& r=slot.row;
            fprintf(f,"cover_pose_probe_first_%u=%u,%llu,%llu,%llu,%llu,%llu ",i,r.detail,r.scene,r.compose,r.sequence,r.world,r.pawn);}}
'''
def instrument(s):
    assert 'enum class CoverProbeStep' not in s
    a=s.index('void sample_current_cover_pose() noexcept {');b=s.index('\nvoid pose_begin(',a)
    fn=s[a:b]
    fn=fn.replace('    if(stopping.load()||!observed_rebind_idle())return;',
        '    if(stopping.load()){note_cover_probe(CoverProbeStep::stopping);return;}\n    if(!observed_rebind_idle()){note_cover_probe(CoverProbeStep::idle);return;}')
    pos=fn.index('        if(!s.context.current(')
    fn=fn[:pos]+'''        auto check=[](CoverProbeStep stage,bool okay,unsigned detail=~0u){if(!okay)note_cover_probe(stage,detail);return okay;};
        a::marker::Status marker_status{};a::Status measurement_status{};
'''+fn[pos:]
    # Restrict substitutions to pre-publication admission. Post-probe closure
    # stays byte-identical; it still invalidates the measured publication.
    c=fn.index('        current_cover_pose=candidate;')
    pre=fn[:c];post=fn[c:]
    replacements=[
        ('!s.context.current(n::tx::Stage::prepare)', '!check(CoverProbeStep::current_before,s.context.current(n::tx::Stage::prepare))',1),
        ('!cover_context_live::sample_current(s.context.actor,before)', '!check(CoverProbeStep::key_before,cover_context_live::sample_current(s.context.actor,before))',1),
        ('before.type!=s.context.source.cover_type','!check(CoverProbeStep::type,before.type==s.context.source.cover_type)',1),
        ('before.slot!=s.context.source.cover_slot','!check(CoverProbeStep::slot,before.slot==s.context.source.cover_slot)',1),
        ('!gears_weapon_hand::same(rig::object(before.link),s.context.source.cover_link)', '!check(CoverProbeStep::link,gears_weapon_hand::same(rig::object(before.link),s.context.source.cover_link))',1),
        ('a::marker::prepare(s.model,s.source,a::marker::Basis::marcus_mesh_face_forward_y,origin.work,marker)!=a::marker::Status::okay',
         '!check(CoverProbeStep::marker,(marker_status=a::marker::prepare(s.model,s.source,a::marker::Basis::marcus_mesh_face_forward_y,origin.work,marker))==a::marker::Status::okay,unsigned(marker_status))',1),
    ]
    for old,new,n in replacements:
        assert pre.count(old)>=n,old;pre=pre.replace(old,new,n)
    # Assign status before passing it to the observer: C++ argument evaluation
    # order must not pair a new result with an old detail value.
    pre=pre.replace('!check(CoverProbeStep::marker,(marker_status=a::marker::prepare(s.model,s.source,a::marker::Basis::marcus_mesh_face_forward_y,origin.work,marker))==a::marker::Status::okay,unsigned(marker_status))',
        '!([&](){marker_status=a::marker::prepare(s.model,s.source,a::marker::Basis::marcus_mesh_face_forward_y,origin.work,marker);return check(CoverProbeStep::marker,marker_status==a::marker::Status::okay,unsigned(marker_status));})()')
    if 'const auto measured=' in pre:
        pre=pre.replace('measured!=a::Status::okay','!check(CoverProbeStep::measurement,measured==a::Status::okay,unsigned(measured))')
        pre=pre.replace('a::marker::Status marker_status{};a::Status measurement_status{};','a::marker::Status marker_status{};')
    else:
        old='a::measure_cover_pose(s.context.source,s.model,s.source,marker,policy,s.workspace,candidate)!=a::Status::okay'
        assert old in pre
        pre=pre.replace(old,'!([&](){measurement_status=a::measure_cover_pose(s.context.source,s.model,s.source,marker,policy,s.workspace,candidate);return check(CoverProbeStep::measurement,measurement_status==a::Status::okay,unsigned(measurement_status));})()')
    # Frozen and canonical order differs; replace each operand in place.
    pre=pre.replace('!cover_context_live::sample_current(s.context.actor,after)','!check(CoverProbeStep::key_after,cover_context_live::sample_current(s.context.actor,after))')
    pre=pre.replace('!cover_context_live::same(before,after)','!check(CoverProbeStep::same_key_after,cover_context_live::same(before,after))')
    pre=pre.replace('!s.context.current(n::tx::Stage::prepare)','!check(CoverProbeStep::current_after,s.context.current(n::tx::Stage::prepare))')
    pre=pre.replace('stopping.load()||revision.load()!=expected_revision',
        '!check(CoverProbeStep::stopping_after,!stopping.load())||!check(CoverProbeStep::revision_after,revision.load()==expected_revision)')
    post=post.replace('        if(s.context.entry)cover_physics',
        '        if(!s.context.entry)note_cover_probe(CoverProbeStep::entry);\n        else {note_cover_probe(CoverProbeStep::complete,0);}\n        if(s.context.entry)cover_physics',1)
    post=post.replace('catch(...){current_cover_pose_valid=false;}','catch(...){note_cover_probe(CoverProbeStep::exception);current_cover_pose_valid=false;}')
    s=s[:a]+DECL+pre+post+s[b:]
    marker='void summary(FILE* f){if(!f)return;'
    assert s.count(marker)==1;s=s.replace(marker,marker+'\n'+SUMMARY)
    return s
if __name__=='__main__':
    p=R/'tools/native-capture/body_c0_goal_policy_live.inl';p.write_text(instrument(p.read_text()))
