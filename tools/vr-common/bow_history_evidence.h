#pragma once
#include "controller_continuity_frame.h"
#include <array>

// Read-only Bow evidence. This receipt verifies a path in an already
// publisher-authenticated ring copy. It grants no game action, pose, release,
// decoder baseline, deadline extension or permission to omit a current check.
namespace bow_history_evidence {
namespace hc=gears_hand_continuity;
namespace vr=gears_vr;
enum class Failure {none,publisher,clock,current,history,baseline,boundary,controls,edge_state};
struct Change {std::uint64_t sequence{};std::int64_t sync_ns{};unsigned control{};bool pressed{};};
struct Receipt {
    Failure failure{Failure::history};
    unsigned samples{},changes{};
    bool all_changes_within_100ms{};
    std::array<Change,hc::motion_history_capacity*2> original_changes{};
};
inline Receipt inspect(hc::TrackedInput const& baseline,hc::Publisher const& baseline_publisher,
    hc::TrackedInput const& current,hc::Publisher const& current_publisher,
    hc::MotionHistory const& history,std::int64_t now,bool baseline_trigger_down) noexcept {
    Receipt out{};
    const auto fail=[](Failure reason) noexcept {Receipt r{};r.failure=reason;return r;};
    if(!baseline_publisher.pid||!baseline_publisher.start||
       baseline_publisher.pid!=current_publisher.pid||baseline_publisher.start!=current_publisher.start)
        return fail(Failure::publisher);
    if(now<=0)return fail(Failure::clock);
    if(!hc::tracked_input_shape_valid(current)||now<current.input.received_ns||
       now<current.input.sync_ns||now-current.input.sync_ns>100000000)
        return fail(Failure::current);
    if(!hc::motion_history_valid(history,current,1))return fail(Failure::history);
    unsigned first=history.count;
    for(unsigned i=0;i<history.count;++i)
        if(hc::same_tracking(history.frames[i],baseline)){first=i;break;}
    if(first==history.count||first+1==history.count)return fail(Failure::baseline);
    // Explicit epochs/publisher must match the saved baseline. No inference
    // from monotonic counters or the latest level can fill a missing sample.
    bool fresh=true;
    bool levels[]={baseline_trigger_down,(baseline.input.hands[1].buttons&vr::controller_stick_click)!=0};
    const auto baseline_value=baseline.input.hands[1].trigger;
    if((levels[0]&&baseline_value<=.35f)||(!levels[0]&&baseline_value>=.65f))return fail(Failure::baseline);
    for(unsigned i=first;i<history.count;++i){
        const auto& sample=history.frames[i];const auto& h=sample.input.hands[1];
        if(sample.stream_epoch!=baseline.stream_epoch||
           sample.tracking_epoch[0]!=baseline.tracking_epoch[0]||
           sample.tracking_epoch[1]!=baseline.tracking_epoch[1])return fail(Failure::boundary);
        if(h.trigger_active!=1||!(h.buttons_active&vr::controller_stick_click))return fail(Failure::controls);
        if(i==first)continue;
        const auto& previous=history.frames[i-1].input.hands[1];
        constexpr unsigned controls[]={vr::control_trigger,vr::control_stick};
        for(unsigned side=0;side<2;++side){
            const auto control=controls[side];const bool before=levels[side];
            const bool after=side==0?(before?h.trigger>.35f:h.trigger>=.65f):
                (h.buttons&vr::controller_stick_click)!=0;
            const auto p=h.pressed[control]-previous.pressed[control];
            const auto r=h.released[control]-previous.released[control];
            if(p!=unsigned(!before&&after)||r!=unsigned(before&&!after))return fail(Failure::edge_state);
            levels[side]=after;
            if(!p&&!r)continue;
            // motion_pair already requires monotonic counters and <=1 total
            // transition per control per raw sample, preserving original order.
            out.original_changes[out.changes++]={sample.input.sequence,sample.input.sync_ns,control,p!=0};
            if(now<sample.input.sync_ns||now-sample.input.sync_ns>100000000)fresh=false;
        }
    }
    out.failure=Failure::none;out.samples=history.count-first;out.all_changes_within_100ms=fresh;return out;
}
}
