#pragma once
#include "stereo_frame.h"
#include <cstdint>
#include <cmath>

namespace gears_vr {
enum ControllerButton : std::uint32_t {
    controller_a=1u<<0, controller_b=1u<<1,
    controller_x=1u<<2, controller_y=1u<<3,
    controller_stick_click=1u<<4,controller_menu=1u<<5
};
constexpr std::uint32_t controller_known_buttons=controller_a|controller_b|controller_x|controller_y|controller_stick_click|controller_menu;
enum ControllerControl : unsigned { control_a,control_b,control_x,control_y,control_stick,control_trigger,control_squeeze,control_menu,control_count };
constexpr float controller_analog_press_threshold=.65f,controller_analog_release_threshold=.35f;
constexpr std::uint64_t controller_pose_required_flags=15; // valid position/orientation and tracked position/orientation

// Wire data contains no OpenXR handles, pointers, bools or runtime-owned memory.
// Buttons and analog values are observed at xrSyncActions, while poses are
// located at predicted_time. The two times must not be described as identical.
struct ControllerHand {
    Pose grip{},aim{};
    std::uint64_t grip_flags{},aim_flags{};
    std::uint32_t grip_active{},aim_active{};
    std::uint32_t buttons{},buttons_active{};
    float trigger{},squeeze{},stick_x{},stick_y{};
    std::uint32_t trigger_active{},squeeze_active{},stick_active{};
    // Cumulative within input_epoch. A skipped XR frame cannot erase a brief
    // press/release; the game may compare these counters at its own tick rate.
    std::uint32_t pressed[control_count]{},released[control_count]{};
    Pose last_squeeze_press_grip{};
    std::uint64_t last_squeeze_press_flags{},last_squeeze_press_sequence{};
};
struct ControllerInputFrame {
    StereoSource source{StereoSource::RuntimeTracked};
    std::uint64_t session{},sequence{},space_epoch{},profile_generation{1},input_epoch{1};
    std::int64_t predicted_time{},received_ns{},sync_ns{};
    std::uint32_t focused{};
    ControllerHand hands[2]{}; // 0 left, 1 right
    // Display-owner routing hint only. Never pose, scene or action authority.
    std::uint32_t display_mode{};
    std::int64_t display_observed_ns{};
};
inline bool controller_pose_usable(Pose const& pose,std::uint64_t flags,std::uint32_t active) {
    return active==1&&(flags&controller_pose_required_flags)==controller_pose_required_flags&&stereo_pose_valid(pose);
}
inline bool controller_hand_shape_valid(ControllerHand const& hand,unsigned index) {
    if(index>1||hand.grip_active>1||hand.aim_active>1||hand.trigger_active>1||
       hand.squeeze_active>1||hand.stick_active>1)return false;
    constexpr std::uint32_t left=controller_x|controller_y|controller_stick_click|controller_menu;
    constexpr std::uint32_t right=controller_a|controller_b|controller_stick_click;
    const auto allowed=index==0?left:right;
    if((hand.buttons|hand.buttons_active)&~allowed)return false;
    if(hand.buttons&~hand.buttons_active)return false;
    if(!std::isfinite(hand.trigger)||!std::isfinite(hand.squeeze)||
       !std::isfinite(hand.stick_x)||!std::isfinite(hand.stick_y))return false;
    if(hand.trigger<0||hand.trigger>1||hand.squeeze<0||hand.squeeze>1||
       std::abs(hand.stick_x)>1||std::abs(hand.stick_y)>1)return false;
    if(!hand.trigger_active&&hand.trigger!=0)return false;
    if(!hand.squeeze_active&&hand.squeeze!=0)return false;
    if(!hand.stick_active&&(hand.stick_x!=0||hand.stick_y!=0))return false;
    if(index==0&&(hand.pressed[control_a]||hand.released[control_a]||
                  hand.pressed[control_b]||hand.released[control_b]))return false;
    if(index==1&&(hand.pressed[control_x]||hand.released[control_x]||
                  hand.pressed[control_y]||hand.released[control_y]||
                  hand.pressed[control_menu]||hand.released[control_menu]))return false;
    if(hand.grip_active&&(hand.grip_flags&3)==3&&!stereo_pose_valid(hand.grip))return false;
    if(hand.aim_active&&(hand.aim_flags&3)==3&&!stereo_pose_valid(hand.aim))return false;
    if(hand.last_squeeze_press_sequence&&(hand.last_squeeze_press_flags&3)==3&&
       !stereo_pose_valid(hand.last_squeeze_press_grip))return false;
    return true;
}
inline bool controller_frame_shape_valid(ControllerInputFrame const& frame) {
    if(!stereo_source_known(frame.source)||!frame.session||!frame.sequence||!frame.space_epoch||
       !frame.profile_generation||!frame.input_epoch||frame.predicted_time<=0||frame.received_ns<=0||frame.sync_ns<=0||
       frame.sync_ns>frame.received_ns||frame.focused>1||frame.display_mode>7||
       frame.display_observed_ns<0||frame.display_observed_ns>frame.received_ns||
       ((frame.display_mode==0)!=(frame.display_observed_ns==0)))return false;
    for(unsigned i=0;i<2;++i){
        if(!controller_hand_shape_valid(frame.hands[i],i))return false;
        if(frame.hands[i].last_squeeze_press_sequence>frame.sequence)return false;
        for(unsigned control=0;control<control_count;++control){
            const auto presses=frame.hands[i].pressed[control],releases=frame.hands[i].released[control];
            if(presses>frame.sequence||releases>frame.sequence||
               (presses>releases?presses-releases:releases-presses)>1)return false;
        }
    }
    return true;
}
inline bool controller_frame_matches(ControllerInputFrame const& frame,StereoSample const& stereo,
                                      std::int64_t now_ns,bool allow_sim=false) {
    return controller_frame_shape_valid(frame)&&stereo_source_allowed(frame.source,allow_sim)&&
        frame.source==stereo.source&&frame.session==stereo.session&&frame.sequence==stereo.sequence&&
        frame.space_epoch==stereo.space_epoch&&frame.predicted_time==stereo.predicted_time&&
        frame.focused==1&&stereo.focused==1&&now_ns>=frame.received_ns&&
        now_ns-frame.received_ns<=100000000;
}

// Publisher-side edge history. The first focused frame after a boundary is a
// baseline, not an action; counters reset whenever focus/profile changes.
class ControllerEdgeCounter {
    std::uint64_t epoch_{1};
    std::uint64_t session_{},space_epoch_{};
    bool focused_{},down_[2][control_count]{};
    std::uint32_t pressed_[2][control_count]{},released_[2][control_count]{};
    Pose last_squeeze_press_grip_[2]{};
    std::uint64_t last_squeeze_press_flags_[2]{},last_squeeze_press_sequence_[2]{};
    static bool down(ControllerHand const& hand,unsigned control,bool previous) noexcept {
        constexpr std::uint32_t bits[]={controller_a,controller_b,controller_x,controller_y,controller_stick_click};
        if(control<5)return (hand.buttons_active&bits[control])&&(hand.buttons&bits[control]);
        if(control==control_menu)return (hand.buttons_active&controller_menu)&&(hand.buttons&controller_menu);
        const bool trigger=control==control_trigger;
        const float value=trigger?hand.trigger:hand.squeeze;
        const bool active=trigger?hand.trigger_active!=0:hand.squeeze_active!=0;
        return active&&(previous?value>controller_analog_release_threshold:value>=controller_analog_press_threshold);
    }
    void boundary() noexcept {
        ++epoch_;if(!epoch_)epoch_=1;
        focused_=false;
        for(unsigned hand=0;hand<2;++hand)for(unsigned control=0;control<control_count;++control){
            down_[hand][control]=false;pressed_[hand][control]=released_[hand][control]=0;
        }
        for(unsigned hand=0;hand<2;++hand){
            last_squeeze_press_grip_[hand]={};last_squeeze_press_flags_[hand]=0;
            last_squeeze_press_sequence_[hand]=0;
        }
    }
public:
    void profile_changed() noexcept {boundary();}
    void update(ControllerInputFrame& frame) noexcept {
        if(session_&&(session_!=frame.session||space_epoch_!=frame.space_epoch))boundary();
        session_=frame.session;space_epoch_=frame.space_epoch;
        if(!frame.focused){
            if(focused_)boundary();
            for(auto& hand:frame.hands){
                for(unsigned control=0;control<control_count;++control)
                    hand.pressed[control]=hand.released[control]=0;
                hand.last_squeeze_press_grip={};hand.last_squeeze_press_flags=0;
                hand.last_squeeze_press_sequence=0;
            }
            frame.input_epoch=epoch_;return;
        }
        const bool baseline=!focused_;
        for(unsigned hand=0;hand<2;++hand)for(unsigned control=0;control<control_count;++control){
            const bool current=down(frame.hands[hand],control,down_[hand][control]);
            if(!baseline&&current!=down_[hand][control]){
                if(current){
                    ++pressed_[hand][control];
                    if(control==control_squeeze){
                        last_squeeze_press_grip_[hand]=frame.hands[hand].grip;
                        last_squeeze_press_flags_[hand]=frame.hands[hand].grip_flags;
                        last_squeeze_press_sequence_[hand]=frame.sequence;
                    }
                }else ++released_[hand][control];
            }
            down_[hand][control]=current;
            frame.hands[hand].pressed[control]=pressed_[hand][control];
            frame.hands[hand].released[control]=released_[hand][control];
        }
        for(unsigned hand=0;hand<2;++hand){
            frame.hands[hand].last_squeeze_press_grip=last_squeeze_press_grip_[hand];
            frame.hands[hand].last_squeeze_press_flags=last_squeeze_press_flags_[hand];
            frame.hands[hand].last_squeeze_press_sequence=last_squeeze_press_sequence_[hand];
        }
        focused_=true;frame.input_epoch=epoch_;
    }
};
}
