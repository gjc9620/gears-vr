"""Add one immutable witness to the existing first measurement rejection."""
from pathlib import Path
R=Path(__file__).resolve().parents[3]

def change_once(source,old,new):
    assert source.count(old)==1,old
    return source.replace(old,new)

def instrument(source):
    assert 'cover_measurement_core_observation.h' not in source
    source=change_once(source,'#include "first_person_body_visibility.h"',
        '#include "first_person_body_visibility.h"\n#include "cover_measurement_core_observation.h"')
    source=change_once(source,'unsigned detail{~0u};};',
        'unsigned detail{~0u};gears_cover_measurement_core::Witness core{};};')
    source=change_once(source,'void note_cover_probe(CoverProbeStep step,unsigned detail=~0u)noexcept{',
        'void note_cover_probe(CoverProbeStep step,unsigned detail=~0u,a::Policy const* policy=nullptr)noexcept{')
    selected=('body_tick_source_live::enabled?\n'
        '            gears_cover_measurement_core::observe(s.context.body_source,policy->now_ns,a::maximum_age_ns,policy->allow_sim):\n'
        '            gears_cover_measurement_core::observe(s.context.source,policy->now_ns,a::maximum_age_ns,policy->allow_sim)'
        if 'a::measure_cover_pose(s.context.body_source,' in source else
        'gears_cover_measurement_core::observe(s.context.source,policy->now_ns,a::maximum_age_ns,policy->allow_sim)')
    old='''        s.context.source.stereo.sequence,s.context.source.world.address,s.context.source.pawn.address,detail};
    slot.state.store(2,std::memory_order_release);'''
    new='''        s.context.source.stereo.sequence,s.context.source.world.address,s.context.source.pawn.address,detail};
    if(policy&&step==CoverProbeStep::measurement&&detail==unsigned(a::Status::core))
        slot.row.core='''+selected+''';
    slot.state.store(2,std::memory_order_release);'''
    source=change_once(source,old,new)
    source=change_once(source,
        'auto check=[](CoverProbeStep stage,bool okay,unsigned detail=~0u){if(!okay)note_cover_probe(stage,detail);return okay;};',
        'auto check=[&policy](CoverProbeStep stage,bool okay,unsigned detail=~0u){if(!okay)note_cover_probe(stage,detail,&policy);return okay;};')
    old='''            fprintf(f,"cover_pose_probe_first_%u=%u,%llu,%llu,%llu,%llu,%llu ",i,r.detail,r.scene,r.compose,r.sequence,r.world,r.pawn);}}'''
    new='''            fprintf(f,"cover_pose_probe_first_%u=%u,%llu,%llu,%llu,%llu,%llu ",i,r.detail,r.scene,r.compose,r.sequence,r.world,r.pawn);
            auto const& w=r.core;if(w.mode){
                fprintf(f,"cover_pose_probe_core_%u=%u,%u,%u,%u,%u,%u,%u,",i,w.mode,w.valid,w.non_age_valid,w.expired,w.controllers_match,w.verdict,w.hard_detail);
                fprintf(f,"%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,",(long long)w.now,(long long)w.stereo_received,(long long)w.controller_received,(long long)w.controller_sync,
                    (long long)w.metadata_begin,(long long)w.metadata_end,(long long)w.tick_begin,(long long)w.tick_end);
                fprintf(f,"%llu,%llu,%llu,%llu,%u,%u,%u,%u ",(unsigned long long)w.limit,(unsigned long long)w.published,
                    (unsigned long long)w.ticket,(unsigned long long)w.reference_revision,w.size,w.abi,w.flags,w.status);}}}'''
    return change_once(source,old,new)

if __name__=='__main__':
    path=R/'tools/native-capture/body_c0_goal_policy_live.inl'
    path.write_text(instrument(path.read_text(encoding='utf-8-sig')),encoding='utf-8')
