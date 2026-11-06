#pragma once
#include "controller_continuity_frame.h"
#include "rig_tracking_frame.h"

namespace gears_current_continuity {
// An omitted Scene controller receipt must really be absent, not a hidden
// conflicting sample. This says nothing about the independently read input.
template<class Scene>
inline bool absent_controllers(Scene const& scene) noexcept {
    const gears_vr::ControllerInputFrame empty{};
    return !(scene.flags&(gears_rig_tracking::controllers_matched|gears_rig_tracking::producer_continuity_available))&&
        !(scene.world_pose_mask&(gears_rig_tracking::left_grip_pose|gears_rig_tracking::right_grip_pose))&&
        !scene.controller_publisher_pid&&!scene.controller_publisher_start&&
        gears_rig_tracking::producer_empty(scene.producer)&&
        gears_hand_continuity::same_input(scene.controllers,empty);
}
// The caller authenticates the current cohort/publisher/reference/age first.
// Scene metadata may have a matched controller packet but no continuity read
// from that earlier render. An explicit current-source consumer may use its
// independently authenticated continuity; legacy tracking consumers may not.
// A present Scene continuity receipt always remains an exact stream baseline.
inline bool metadata_scene(gears_hand_continuity::TrackedInput const& current,
    gears_rig_tracking::ProducerContinuity const& previous,std::uint64_t previous_input,
    bool parent_has_continuity,bool allow_current_epochs,bool allow_absent_parent) noexcept {
    if(!gears_hand_continuity::tracked_input_shape_valid(current))return false;
    if(!parent_has_continuity)return allow_absent_parent&&gears_rig_tracking::producer_empty(previous);
    if(!previous.stream||current.stream_epoch!=previous.stream)return false;
    for(unsigned side=0;side<2;++side){
        const auto value=current.tracking_epoch[side],old=previous.tracking[side];
        if(!old)return false;
        if(value!=old&&(!allow_current_epochs||value<old||current.input.sequence<=previous_input))return false;
    }
    return true;
}
// The Native current-source owner retains its last successfully acquired
// cohort independently of the optional render-Scene receipt. Missing Scene
// continuity cannot erase this monotonic baseline or authorize an old epoch.
inline bool not_regressed(gears_hand_continuity::TrackedInput const& current,
    gears_hand_continuity::TrackedInput const& previous) noexcept {
    if(current.stream_epoch<previous.stream_epoch||
       current.tracking_epoch[0]<previous.tracking_epoch[0]||current.tracking_epoch[1]<previous.tracking_epoch[1])return false;
    if(current.input.input_epoch==previous.input.input_epoch&&current.input.profile_generation==previous.input.profile_generation){
        if(current.input.sequence<previous.input.sequence)return false;
        if(current.input.sequence==previous.input.sequence)return current.stream_epoch==previous.stream_epoch&&
            current.tracking_epoch[0]==previous.tracking_epoch[0]&&current.tracking_epoch[1]==previous.tracking_epoch[1]&&
            gears_hand_continuity::same_input(current.input,previous.input);
    }
    return true;
}
}
