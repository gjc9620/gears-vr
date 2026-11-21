#pragma once
#include "../vr-common/controller_continuity_frame.h"
#include "../vr-common/weapon_gnasher_mechanics.h"

// Motion facts ONLY. This returns original producer rows, never a Hand/Rig
// Frame, world transform, current pose, grant, edge, animation seek or ACK.
// A future consumer must separately close the current Native cohort, object/
// reference/hold identities, same support lease and original <=100ms source.
namespace gears_gnasher_producer_segment {
namespace c=gears_hand_continuity;namespace vr=gears_vr;
enum class Status {ready,invalid_configuration,invalid_anchor,publisher_changed,
    invalid_history,span_unavailable,missing_sample,changed_controls,excessive_motion};
struct Segment {
    std::array<c::TrackedInput,c::motion_history_capacity+1> points{};
    c::Publisher publisher{};unsigned count{};
};
inline double angle(vr::Quaternion const& a,vr::Quaternion const& b) noexcept {
    const auto x=vr::normalized(a),y=vr::normalized(b);
    const auto dot=std::abs(x.x*y.x+x.y*y.y+x.z*y.z+x.w*y.w);
    return 2*std::acos((std::clamp)(dot,0.,1.));
}
inline bool same_controls(c::TrackedInput const& a,c::TrackedInput const& b) noexcept {
    if(a.input.display_mode!=b.input.display_mode)return false;
    for(unsigned hand=0;hand<2;++hand){
        auto const& x=a.input.hands[hand];auto const& y=b.input.hands[hand];
        if(x.buttons!=y.buttons||x.buttons_active!=y.buttons_active||x.trigger_active!=y.trigger_active||
           x.squeeze_active!=y.squeeze_active)return false;
        for(unsigned i=0;i<vr::control_count;++i)
            if(x.pressed[i]!=y.pressed[i]||x.released[i]!=y.released[i])return false;
    }
    auto const& left=b.input.hands[0];
    return left.squeeze_active==1&&left.squeeze>vr::controller_analog_release_threshold;
}
inline Status verify(c::TrackedInput const& previous,c::Publisher const& previous_publisher,
    c::TrackedInput const& current,c::Publisher const& current_publisher,
    c::MotionHistory const& history,gears_weapon::GnasherMechanicalConfig const& config,
    Segment& out) noexcept {
    out={};
    // The 200ms span bounds retrospective DATA inspection. It is NOT a source
    // lifetime and is never passed to frame_active/rig match or a game call.
    constexpr std::int64_t maximum_inspected_span_ns=200000000;
    if(!config.valid()||config.travel.maximum_sample_gap_ns!=100000000)return Status::invalid_configuration;
    if(!c::tracked_input_shape_valid(previous)||!c::tracked_input_shape_valid(current)||
       !previous_publisher.pid||!previous_publisher.start||previous.input.focused!=1||current.input.focused!=1||
       previous.input.sequence>=current.input.sequence||previous.input.sync_ns<=0||
       current.input.sync_ns<=previous.input.sync_ns)return Status::invalid_anchor;
    if(previous_publisher.pid!=current_publisher.pid||previous_publisher.start!=current_publisher.start)
        return Status::publisher_changed;
    const auto span=current.input.sync_ns-previous.input.sync_ns;
    if(span<=config.travel.maximum_sample_gap_ns||span>maximum_inspected_span_ns||
       current.input.sequence-previous.input.sequence>c::motion_history_capacity)return Status::span_unavailable;
    if(!c::motion_history_valid(history,current,0,config.travel.maximum_sample_gap_ns)||
       !c::motion_history_valid(history,current,1,config.travel.maximum_sample_gap_ns))return Status::invalid_history;
    Segment result{};result.publisher=current_publisher;result.points[result.count++]=previous;
    if(!same_controls(previous,previous))return Status::changed_controls;
    for(unsigned i=0;i<history.count;++i){
        auto const& next=history.frames[i];
        if(next.input.sequence<previous.input.sequence)continue;
        if(next.input.sequence==previous.input.sequence){
            if(!c::same_tracking(previous,next))return Status::invalid_anchor;
            continue;
        }
        auto const& last=result.points[result.count-1];
        if(result.count==result.points.size()||next.input.sequence!=last.input.sequence+1||
           !c::motion_pair(last,next,0,config.travel.maximum_sample_gap_ns)||
           !c::motion_pair(last,next,1,config.travel.maximum_sample_gap_ns))return Status::missing_sample;
        if(!same_controls(previous,next))return Status::changed_controls;
        const double seconds=double(next.input.sync_ns-last.input.sync_ns)/1e9;
        for(unsigned hand=0;hand<2;++hand){
            auto const& a=last.input.hands[hand].grip;auto const& b=next.input.hands[hand].grip;
            if(gears_weapon::length(gears_weapon::difference(a.position,b.position))>
                   config.maximum_hand_speed_metres_per_second*seconds||
               angle(a.orientation,b.orientation)>config.maximum_hand_rotation_radians_per_second*seconds)
                return Status::excessive_motion;
        }
        result.points[result.count++]=next;
    }
    if(result.count<2||!c::same_tracking(result.points[result.count-1],current)||
       result.count-1!=current.input.sequence-previous.input.sequence)return Status::missing_sample;
    out=result;return Status::ready;
}
}
