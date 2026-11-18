#pragma once
#include "controller_continuity_frame.h"

// Original producer motion only. No world pose, Hand/Rig Frame, lease,
// timestamp renewal, action or native-bone acknowledgement is produced here.
namespace gears_controller_relative_motion {
using V=gears_vr::Vector;using P=gears_vr::Pose;
struct Point {std::uint64_t sequence{};std::int64_t sync_ns{};V position{};};
struct Segment {
    std::array<Point,gears_hand_continuity::motion_history_capacity+1> points{};
    unsigned count{};
};
inline bool held(gears_vr::ControllerHand const& h,unsigned control) noexcept {
    const bool active=control==gears_vr::control_squeeze?h.squeeze_active==1:
        control==gears_vr::control_trigger&&h.trigger_active==1;
    const float value=control==gears_vr::control_squeeze?h.squeeze:h.trigger;
    return active&&value>gears_vr::controller_analog_release_threshold&&h.pressed[control]>h.released[control];
}
inline bool project(gears_hand_continuity::TrackedInput const& raw,P const& part_in_right_m,
                    double model_scale,V& out) noexcept {
    if(!gears_hand_continuity::tracked_input_shape_valid(raw)||
       !gears_vr::stereo_pose_valid(part_in_right_m)||!std::isfinite(model_scale)||model_scale<=.1||model_scale>=10)return false;
    for(auto const& h:raw.input.hands)
        if(!gears_vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active))return false;
    const auto& left=raw.input.hands[0].grip;const auto& right=raw.input.hands[1].grip;
    const V delta{left.position.x-right.position.x,left.position.y-right.position.y,left.position.z-right.position.z};
    const auto relative=gears_vr::xr_to_ue(gears_vr::transform(gears_vr::rotation_matrix(gears_vr::inverse(right.orientation)),delta));
    const V local{relative.x-part_in_right_m.position.x,relative.y-part_in_right_m.position.y,relative.z-part_in_right_m.position.z};
    const auto value=gears_vr::transform(gears_vr::rotation_matrix(gears_vr::inverse(part_in_right_m.orientation)),local);
    const V result{value.x/model_scale,value.y/model_scale,value.z/model_scale};
    if(!gears_vr::finite(result))return false;out=result;return true;
}
inline bool build(gears_hand_continuity::TrackedInput const& previous,
    gears_hand_continuity::TrackedInput const& current,gears_hand_continuity::MotionHistory const& history,
    unsigned control,P const& fixed_part_in_right_m,double scale,Segment& out) noexcept {
    namespace motion=gears_hand_continuity;
    if((control!=gears_vr::control_squeeze&&control!=gears_vr::control_trigger)||history.reserved||
       !history.count||history.count>motion::motion_history_capacity||
       !motion::tracked_input_shape_valid(previous)||!motion::tracked_input_shape_valid(current)||
       !motion::same_tracking(history.frames[history.count-1],current)||
       current.input.sequence<=previous.input.sequence||
       current.input.sequence-previous.input.sequence>motion::motion_history_capacity||
       previous.input.focused!=1||!held(previous.input.hands[0],control))return false;
    Segment result{};V position{};
    if(!project(previous,fixed_part_in_right_m,scale,position))return false;
    result.points[result.count++]={previous.input.sequence,previous.input.sync_ns,position};
    auto last=previous;std::uint64_t ordered{};
    for(unsigned i=0;i<history.count;++i){const auto& next=history.frames[i];
        if(!motion::tracked_input_shape_valid(next)||(ordered&&next.input.sequence<=ordered))return false;
        ordered=next.input.sequence;
        if(next.input.sequence<previous.input.sequence)continue;
        if(next.input.sequence==previous.input.sequence){if(!motion::same_tracking(next,previous))return false;continue;}
        if(!motion::motion_pair(last,next,0,100000000)||!motion::motion_pair(last,next,1,100000000)||
           next.input.display_mode!=previous.input.display_mode||!held(next.input.hands[0],control)||
           next.input.hands[0].pressed[control]!=previous.input.hands[0].pressed[control]||
           next.input.hands[0].released[control]!=previous.input.hands[0].released[control]||
           result.count>=result.points.size()||!project(next,fixed_part_in_right_m,scale,position))return false;
        result.points[result.count++]={next.input.sequence,next.input.sync_ns,position};last=next;
    }
    if(result.count<2||!motion::same_tracking(last,current))return false;
    out=result;return true;
}
// A held object can have its contact tip away from the raw grip centre.
// The mount is expressed in the SAME UE-local metres as the real item mount.
// Preserve the authenticated chain and include original left-hand rotation.
inline bool project_attached(gears_hand_continuity::TrackedInput const& raw,P const& part_in_right_m,
    double scale,V point_in_left_m,V& out) noexcept {
    V grip{};if(!gears_vr::finite(point_in_left_m)||!project(raw,part_in_right_m,scale,grip))return false;
    const auto left_in_right=gears_vr::multiply(gears_vr::inverse(raw.input.hands[1].grip.orientation),raw.input.hands[0].grip.orientation);
    const auto relative_basis=gears_vr::xr_to_ue(gears_vr::rotation_matrix(left_in_right));
    const auto offset=gears_vr::transform(gears_vr::rotation_matrix(gears_vr::inverse(part_in_right_m.orientation)),
        gears_vr::transform(relative_basis,point_in_left_m));
    const V result{grip.x+offset.x/scale,grip.y+offset.y/scale,grip.z+offset.z/scale};
    if(!gears_vr::finite(result))return false;out=result;return true;
}
inline bool build_attached(gears_hand_continuity::TrackedInput const& previous,
    gears_hand_continuity::TrackedInput const& current,gears_hand_continuity::MotionHistory const& history,
    unsigned control,P const& part_in_right_m,double scale,V point_in_left_m,Segment& out) noexcept {
    Segment result{};
    if(!build(previous,current,history,control,part_in_right_m,scale,result))return false;
    for(unsigned i=0;i<result.count;++i){const auto sequence=result.points[i].sequence;
        auto const* raw=sequence==previous.input.sequence?&previous:nullptr;
        for(unsigned n=0;!raw&&n<history.count;++n)if(history.frames[n].input.sequence==sequence)raw=&history.frames[n];
        if(!raw||!project_attached(*raw,part_in_right_m,scale,point_in_left_m,result.points[i].position))return false;
    }
    out=result;return true;
}
}
