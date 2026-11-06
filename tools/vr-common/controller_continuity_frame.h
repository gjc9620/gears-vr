#pragma once
#include "controller_input_frame.h"
#include <limits>
#include <type_traits>

namespace gears_hand_continuity {
struct Publisher {
    std::uint64_t pid{},start{};
};
struct TrackedInput {
    gears_vr::ControllerInputFrame input{};
    std::uint64_t stream_epoch{},tracking_epoch[2]{};
};
// Caller-owned copy of the EXISTING producer ring, oldest to newest. This is
// motion data only: it is not a fresh Hand/Rig Frame, a release edge, a world
// mapping or an action permit. Shared-memory packet layout is unchanged.
inline constexpr unsigned motion_history_capacity=8;
struct MotionHistory {
    std::uint32_t count{},reserved{};
    TrackedInput frames[motion_history_capacity]{};
};
static_assert(std::is_standard_layout_v<TrackedInput> &&
              std::is_trivially_copyable_v<TrackedInput>);

inline bool same_input(gears_vr::ControllerInputFrame const& a,
                       gears_vr::ControllerInputFrame const& b) noexcept {
    if(a.source!=b.source||a.session!=b.session||a.sequence!=b.sequence||
       a.space_epoch!=b.space_epoch||a.profile_generation!=b.profile_generation||
       a.input_epoch!=b.input_epoch||a.predicted_time!=b.predicted_time||
       a.received_ns!=b.received_ns||a.sync_ns!=b.sync_ns||a.focused!=b.focused||
       a.display_mode!=b.display_mode||a.display_observed_ns!=b.display_observed_ns)return false;
    for(unsigned i=0;i<2;++i){
        auto const& x=a.hands[i];auto const& y=b.hands[i];
        if(!gears_vr::stereo_same_pose(x.grip,y.grip)||!gears_vr::stereo_same_pose(x.aim,y.aim)||
           x.grip_flags!=y.grip_flags||x.aim_flags!=y.aim_flags||
           x.grip_active!=y.grip_active||x.aim_active!=y.aim_active||
           x.buttons!=y.buttons||x.buttons_active!=y.buttons_active||
           x.trigger!=y.trigger||x.squeeze!=y.squeeze||x.stick_x!=y.stick_x||x.stick_y!=y.stick_y||
           x.trigger_active!=y.trigger_active||x.squeeze_active!=y.squeeze_active||
           x.stick_active!=y.stick_active||
           !gears_vr::stereo_same_pose(x.last_squeeze_press_grip,y.last_squeeze_press_grip)||
           x.last_squeeze_press_flags!=y.last_squeeze_press_flags||
           x.last_squeeze_press_sequence!=y.last_squeeze_press_sequence)return false;
        for(unsigned control=0;control<gears_vr::control_count;++control)
            if(x.pressed[control]!=y.pressed[control]||x.released[control]!=y.released[control])return false;
    }
    return true;
}
inline bool tracked_input_shape_valid(TrackedInput const& frame) noexcept {
    return frame.stream_epoch&&frame.tracking_epoch[0]&&frame.tracking_epoch[1]&&
        gears_vr::controller_frame_shape_valid(frame.input);
}

namespace detail {
inline bool same_origin(gears_vr::ControllerInputFrame const& a,
                        gears_vr::ControllerInputFrame const& b) noexcept {
    return a.source==b.source&&a.session==b.session;
}

inline bool same_key(gears_vr::ControllerInputFrame const& a,
                     gears_vr::ControllerInputFrame const& b) noexcept {
    return same_origin(a,b)&&a.space_epoch==b.space_epoch&&
        a.input_epoch==b.input_epoch&&a.profile_generation==b.profile_generation;
}
inline bool raw_regressed(gears_vr::ControllerInputFrame const& previous,
                          gears_vr::ControllerInputFrame const& next) noexcept {
    return same_origin(previous,next)&&(next.sequence<=previous.sequence||
        next.space_epoch<previous.space_epoch||next.input_epoch<previous.input_epoch||
        next.profile_generation<previous.profile_generation);
}
}

inline bool same_tracking(TrackedInput const& a,TrackedInput const& b) noexcept {
    return a.stream_epoch==b.stream_epoch&&a.tracking_epoch[0]==b.tracking_epoch[0]&&
        a.tracking_epoch[1]==b.tracking_epoch[1]&&same_input(a.input,b.input);
}
inline bool motion_pair(TrackedInput const& previous,TrackedInput const& next,unsigned hand,
    std::int64_t maximum_gap_ns) noexcept {
    if(hand>1||maximum_gap_ns<=0||!tracked_input_shape_valid(previous)||!tracked_input_shape_valid(next)||
       previous.stream_epoch!=next.stream_epoch||previous.tracking_epoch[hand]!=next.tracking_epoch[hand]||
       !detail::same_key(previous.input,next.input)||previous.input.focused!=1||next.input.focused!=1||
       previous.input.sequence==UINT64_MAX||next.input.sequence!=previous.input.sequence+1||
       previous.input.predicted_time>=next.input.predicted_time||previous.input.sync_ns>=next.input.sync_ns||
       std::uint64_t(next.input.sync_ns)-std::uint64_t(previous.input.sync_ns)>std::uint64_t(maximum_gap_ns)||
       std::uint64_t(next.input.predicted_time)-std::uint64_t(previous.input.predicted_time)>std::uint64_t(maximum_gap_ns)||
       previous.input.received_ns>next.input.received_ns)return false;
    for(auto const* p:{&previous,&next}){const auto& h=p->input.hands[hand];
        if(!gears_vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active))return false;}
    const auto& a=previous.input.hands[hand];const auto& b=next.input.hands[hand];
    for(unsigned i=0;i<gears_vr::control_count;++i)
        if(b.pressed[i]<a.pressed[i]||b.released[i]<a.released[i]||
           std::uint64_t(b.pressed[i])-a.pressed[i]+std::uint64_t(b.released[i])-a.released[i]>1)return false;
    return true;
}
inline bool motion_history_valid(MotionHistory const& h,TrackedInput const& current,unsigned hand,
    std::int64_t maximum_gap_ns=100000000) noexcept {
    if(hand>1||h.reserved||!h.count||h.count>motion_history_capacity||
       !tracked_input_shape_valid(current)||!same_tracking(h.frames[h.count-1],current)||
       current.input.focused!=1)return false;
    const auto& grip=current.input.hands[hand];
    if(!gears_vr::controller_pose_usable(grip.grip,grip.grip_flags,grip.grip_active))return false;
    for(unsigned i=1;i<h.count;++i)if(!motion_pair(h.frames[i-1],h.frames[i],hand,maximum_gap_ns))return false;
    return true;
}

// Authenticate the exact latest right squeeze edge against its ORIGINAL
// producer sample, not a consumer poll time or a reconstructed pose. Optional
// ring evidence only: it cannot authorize a game action or survive an epoch.
inline bool recorded_right_squeeze_press(TrackedInput const& current,TrackedInput const& press) noexcept {
    if(!tracked_input_shape_valid(current)||!tracked_input_shape_valid(press)||
       current.stream_epoch!=press.stream_epoch||current.tracking_epoch[1]!=press.tracking_epoch[1]||
       !detail::same_key(current.input,press.input)||current.input.focused!=1||press.input.focused!=1||
       press.input.sequence>current.input.sequence||press.input.sync_ns>current.input.sync_ns||
       press.input.received_ns>current.input.received_ns)return false;
    const auto& latest=current.input.hands[1];const auto& edge=press.input.hands[1];
    return latest.last_squeeze_press_sequence==press.input.sequence&&
        edge.last_squeeze_press_sequence==press.input.sequence&&
        latest.pressed[gears_vr::control_squeeze]==edge.pressed[gears_vr::control_squeeze]&&
        edge.pressed[gears_vr::control_squeeze]>edge.released[gears_vr::control_squeeze]&&
        edge.squeeze_active&&edge.squeeze>=gears_vr::controller_analog_press_threshold&&
        gears_vr::controller_pose_usable(edge.grip,edge.grip_flags,edge.grip_active)&&
        latest.last_squeeze_press_flags==edge.grip_flags&&edge.last_squeeze_press_flags==edge.grip_flags&&
        gears_vr::stereo_same_pose(latest.last_squeeze_press_grip,edge.grip)&&
        gears_vr::stereo_same_pose(edge.last_squeeze_press_grip,edge.grip);
}

// Keep one instance across mapping opens. Call once for every source sample,
// including unpublished samples. Epochs describe source, not receiver history.
class SourceTracker {
    gears_vr::ControllerInputFrame previous_{};
    std::uint64_t stream_epoch_{},tracking_epoch_[2]{};
    bool known_{},usable_[2]{},pending_boundary_{},failed_{};
    friend struct SourceTrackerTestAccess;
public:
    bool update(gears_vr::ControllerInputFrame const& source,TrackedInput& out) noexcept {
        const auto input=source;
        out={};
        if(failed_)return false;
        if(!gears_vr::controller_frame_shape_valid(input)){
            pending_boundary_=true;return false;
        }
        if(known_&&same_input(input,previous_)){
            // A cached repeat cannot erase an interruption already observed.
            if(pending_boundary_)return false;
            out.input=previous_;out.stream_epoch=stream_epoch_;
            out.tracking_epoch[0]=tracking_epoch_[0];out.tracking_epoch[1]=tracking_epoch_[1];
            return true;
        }
        if(known_&&detail::raw_regressed(previous_,input)){pending_boundary_=true;return false;}
        const bool usable[]={
            gears_vr::controller_pose_usable(input.hands[0].grip,input.hands[0].grip_flags,input.hands[0].grip_active),
            gears_vr::controller_pose_usable(input.hands[1].grip,input.hands[1].grip_flags,input.hands[1].grip_active)};
        auto stream=stream_epoch_;
        std::uint64_t tracking[]={tracking_epoch_[0],tracking_epoch_[1]};
        if(!known_){
            stream=tracking[0]=tracking[1]=1;
        }else if(pending_boundary_||!detail::same_key(previous_,input)||
                  input.sequence-previous_.sequence!=1||input.sync_ns<previous_.sync_ns||input.received_ns<previous_.received_ns||
                  input.sync_ns-previous_.sync_ns>100000000){
            if(stream==(std::numeric_limits<std::uint64_t>::max)()){failed_=true;return false;}
            // The new stream invalidates both hands; retain their cumulative
            // loss counts and establish usable baselines without adding loss.
            // Version2 also records actual producer sync interruption, rather
            // than mistaking a slow consumer poll for a missing source sample.
            ++stream;
        }else{
            for(unsigned hand=0;hand<2;++hand)if(usable_[hand]&&!usable[hand]){
                if(tracking[hand]==(std::numeric_limits<std::uint64_t>::max)()){failed_=true;return false;}
                ++tracking[hand];
            }
        }
        previous_=input;stream_epoch_=stream;pending_boundary_=false;known_=true;
        for(unsigned hand=0;hand<2;++hand){tracking_epoch_[hand]=tracking[hand];usable_[hand]=usable[hand];}
        out.input=input;out.stream_epoch=stream;
        out.tracking_epoch[0]=tracking[0];out.tracking_epoch[1]=tracking[1];
        return true;
    }
};
}
