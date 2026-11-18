#pragma once
#include "controller_continuity_frame.h"

// Right-hand latches for the ONE already-selected mounted Native input.
// No bridge read, left-hand decoder/arbiter, game call, pose or firing permit.
namespace gears_troika_right {
namespace vr=gears_vr;
struct Result {
    bool grip_known{},grip_down{},grip_owned{},trigger_known{},trigger_down{},both_owned{},fire_requested{};
    std::uint64_t grip_press_sequence{},fire_edge_sequence{};
    std::int64_t grip_window_begin_ns{},fire_window_begin_ns{};
};
class Controls {
    struct Channel {
        std::uint32_t pressed{},released{};bool known{},down{};
        bool advance(vr::ControllerHand const& hand,unsigned control,bool baseline) noexcept {
            const auto p=hand.pressed[control],r=hand.released[control];
            const auto value=control==vr::control_squeeze?hand.squeeze:hand.trigger;
            const auto active=control==vr::control_squeeze?hand.squeeze_active:hand.trigger_active;
            const auto old_p=pressed,old_r=released;pressed=p;released=r;
            if(active!=1){known=false;down=true;return false;}
            if(baseline){known=value<=vr::controller_analog_release_threshold;down=!known;return false;}
            if(p<old_p||r<old_r||p-old_p>1||r-old_r>1||(p!=old_p&&r!=old_r)){
                known=false;down=true;return false;
            }
            const bool press=p!=old_p,release=r!=old_r;
            if(release){
                if(value>=vr::controller_analog_press_threshold||(known&&!down)){known=false;down=true;return false;}
                known=true;down=false;return false;
            }
            if(press){
                if(!known||down||value<=vr::controller_analog_release_threshold){known=false;down=true;return false;}
                down=true;return true;
            }
            if(!known){if(value<=vr::controller_analog_release_threshold){known=true;down=false;}return false;}
            if((down&&value<=vr::controller_analog_release_threshold)||(!down&&value>=vr::controller_analog_press_threshold)){
                known=false;down=true;
            }
            return false;
        }
    } grip_{},trigger_{};
    vr::ControllerInputFrame previous_{};bool ready_{},owned_{},fire_armed_{},fire_requested_{};
    std::uint64_t grip_sequence_{},fire_sequence_{};std::int64_t grip_begin_{},fire_begin_{};
    static bool key(vr::ControllerInputFrame const& a,vr::ControllerInputFrame const& b) noexcept {
        return a.source==b.source&&a.session==b.session&&a.space_epoch==b.space_epoch&&
            a.profile_generation==b.profile_generation&&a.input_epoch==b.input_epoch;
    }
public:
    void lost() noexcept {ready_=owned_=fire_armed_=fire_requested_=false;grip_={};trigger_={};grip_sequence_=fire_sequence_=0;grip_begin_=fire_begin_=0;}
    Result advance(vr::ControllerInputFrame const& f,vr::StereoSample const& stereo,std::int64_t now,
        bool left_owned,bool current_right_contact,bool original_press_contact,bool allow_sim=false) noexcept {
        Result out{};auto const& hand=f.hands[1];
        if(!vr::controller_frame_matches(f,stereo,now,allow_sim)||now<f.sync_ns||now-f.sync_ns>100000000||
           !vr::stereo_render_active(stereo)||!vr::stereo_tracking_valid(stereo,allow_sim)||
           !vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)){lost();return out;}
        const bool same=ready_&&key(f,previous_);
        const bool duplicate=same&&f.sequence==previous_.sequence;
        if(same&&(f.sequence<previous_.sequence||f.sync_ns<previous_.sync_ns||
           (duplicate&&!gears_hand_continuity::same_input(f,previous_)))){lost();return out;}
        const bool baseline=!same||f.sync_ns-previous_.sync_ns>100000000;
        const auto lower=baseline?std::int64_t(0):previous_.sync_ns;
        bool grip_press{},trigger_press{};
        if(baseline){owned_=fire_requested_=fire_armed_=false;grip_sequence_=fire_sequence_=0;grip_begin_=fire_begin_=0;}
        if(!duplicate||baseline){
            grip_press=grip_.advance(hand,vr::control_squeeze,baseline);
            trigger_press=trigger_.advance(hand,vr::control_trigger,baseline);
            if(grip_press&&(hand.last_squeeze_press_sequence<=previous_.sequence||hand.last_squeeze_press_sequence>f.sequence||
               !vr::controller_pose_usable(hand.last_squeeze_press_grip,hand.last_squeeze_press_flags,1)||
               lower<=0||now<lower||now-lower>100000000)){
                grip_.known=false;grip_press=false;
            }
        }
        if(!grip_.known||!grip_.down)owned_=false;
        else if(grip_press){owned_=current_right_contact&&original_press_contact;
            if(owned_){grip_sequence_=hand.last_squeeze_press_sequence;grip_begin_=lower;}}
        const bool both=left_owned&&owned_&&grip_.known&&grip_.down;
        if(!both||!trigger_.known){fire_armed_=fire_requested_=false;}
        else if(!trigger_.down){fire_requested_=false;fire_armed_=true;}
        else if(trigger_press&&fire_armed_){
            fire_requested_=lower>0&&now>=lower&&now-lower<=100000000;fire_armed_=false;
            if(fire_requested_){fire_sequence_=f.sequence;fire_begin_=lower;}
        }
        if(!duplicate||baseline){previous_=f;ready_=true;}
        out={grip_.known,grip_.known&&grip_.down,owned_,trigger_.known,trigger_.known&&trigger_.down,both,fire_requested_,
            owned_?grip_sequence_:0,fire_requested_?fire_sequence_:0,owned_?grip_begin_:0,fire_requested_?fire_begin_:0};return out;
    }
};
} // namespace gears_troika_right
