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
static_assert(std::is_standard_layout_v<TrackedInput> &&
              std::is_trivially_copyable_v<TrackedInput>);

inline bool same_input(gears_vr::ControllerInputFrame const& a,
                       gears_vr::ControllerInputFrame const& b) noexcept {
    if(a.source!=b.source||a.session!=b.session||a.sequence!=b.sequence||
       a.space_epoch!=b.space_epoch||a.profile_generation!=b.profile_generation||
       a.input_epoch!=b.input_epoch||a.predicted_time!=b.predicted_time||
       a.received_ns!=b.received_ns||a.sync_ns!=b.sync_ns||a.focused!=b.focused)return false;
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
                  input.sequence-previous_.sequence!=1){
            if(stream==(std::numeric_limits<std::uint64_t>::max)()){failed_=true;return false;}
            // The new stream invalidates both hands; retain their cumulative
            // loss counts and establish usable baselines without adding loss.
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
