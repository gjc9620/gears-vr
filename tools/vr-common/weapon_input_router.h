#pragma once
#include "body_slots.h"
#include <array>

namespace gears_weapon {
enum class WeaponIntent : std::uint8_t {
    grab_ammo,weapon_2,weapon_1,pistol,grenade,release_left_grip,
    recenter,reload,aim_down,aim_up,fire_down,fire_up,
    xbox_a,xbox_b,xbox_x,xbox_y
};
inline BodySlot equipment_body_slot(WeaponIntent intent) noexcept {
    switch(intent){
        case WeaponIntent::weapon_2:return BodySlot::weapon_2;
        case WeaponIntent::weapon_1:return BodySlot::weapon_1;
        case WeaponIntent::pistol:return BodySlot::pistol;
        case WeaponIntent::grenade:return BodySlot::grenade;
        default:return BodySlot::none;
    }
}
enum class RouterStatus : std::uint8_t { rejected,baseline,accepted,ambiguous };
struct RoutedInput {
    RouterStatus status{RouterStatus::rejected};
    std::array<WeaponIntent,16> intents{};
    unsigned count{};
    void push(WeaponIntent value) noexcept {if(count<intents.size())intents[count++]=value;}
};

// Produces intents only. Inventory identity, animation ownership, weapon poses,
// ammo commit and game input forwarding remain the live adapter's authority.
class WeaponInputRouter {
    std::uint64_t session_{},sequence_{},space_epoch_{},profile_generation_{},input_epoch_{};
    std::uint32_t pressed_[2][gears_vr::control_count]{},released_[2][gears_vr::control_count]{};
    void baseline(gears_vr::ControllerInputFrame const& input) noexcept {
        session_=input.session;sequence_=input.sequence;space_epoch_=input.space_epoch;
        profile_generation_=input.profile_generation;input_epoch_=input.input_epoch;
        for(unsigned hand=0;hand<2;++hand)for(unsigned control=0;control<gears_vr::control_count;++control){
            pressed_[hand][control]=input.hands[hand].pressed[control];
            released_[hand][control]=input.hands[hand].released[control];
        }
    }
public:
    void reset() noexcept {*this=WeaponInputRouter{};}
    RoutedInput advance(gears_vr::ControllerInputFrame const& input,gears_vr::StereoSample const& stereo,
                        ArmspanCalibration const& calibration,Pose const& body,std::int64_t now_ns,
                        bool allow_sim=false) noexcept {
        RoutedInput result{};
        if(!gears_vr::controller_frame_matches(input,stereo,now_ns,allow_sim)){
            reset();return result;
        }
        if(!session_||session_!=input.session||space_epoch_!=input.space_epoch||
           profile_generation_!=input.profile_generation||input_epoch_!=input.input_epoch){
            baseline(input);result.status=RouterStatus::baseline;return result;
        }
        if(input.sequence<=sequence_)return result;
        for(unsigned hand=0;hand<2;++hand)for(unsigned control=0;control<gears_vr::control_count;++control){
            const auto p=input.hands[hand].pressed[control],r=input.hands[hand].released[control];
            if(p<pressed_[hand][control]||r<released_[hand][control]||
               p-pressed_[hand][control]>1||r-released_[hand][control]>1){
                baseline(input);result.status=RouterStatus::ambiguous;return result;
            }
        }
        auto new_press=[&](unsigned hand,unsigned control){return input.hands[hand].pressed[control]>pressed_[hand][control];};
        auto new_release=[&](unsigned hand,unsigned control){return input.hands[hand].released[control]>released_[hand][control];};
        if(new_press(0,gears_vr::control_stick))result.push(WeaponIntent::recenter);
        if(new_press(1,gears_vr::control_stick))result.push(WeaponIntent::reload);
        if(new_press(0,gears_vr::control_squeeze)||new_press(1,gears_vr::control_squeeze)){
            for(unsigned hand=0;hand<2;++hand){
                if(!new_press(hand,gears_vr::control_squeeze))continue;
                const auto& h=input.hands[hand];
                if(h.last_squeeze_press_sequence<=sequence_||
                   !gears_vr::controller_pose_usable(h.last_squeeze_press_grip,h.last_squeeze_press_flags,1))continue;
                switch(body_slot(calibration,body,h.last_squeeze_press_grip.position,hand)){
                    case BodySlot::shoulder_ammo:result.push(WeaponIntent::grab_ammo);break;
                    case BodySlot::weapon_2:result.push(WeaponIntent::weapon_2);break;
                    case BodySlot::weapon_1:result.push(WeaponIntent::weapon_1);break;
                    case BodySlot::pistol:result.push(WeaponIntent::pistol);break;
                    case BodySlot::grenade:result.push(WeaponIntent::grenade);break;
                    default:break;
                }
            }
        }
        if(new_release(0,gears_vr::control_squeeze))result.push(WeaponIntent::release_left_grip);
        if(new_press(0,gears_vr::control_trigger))result.push(WeaponIntent::aim_down);
        if(new_release(0,gears_vr::control_trigger))result.push(WeaponIntent::aim_up);
        if(new_press(1,gears_vr::control_trigger))result.push(WeaponIntent::fire_down);
        if(new_release(1,gears_vr::control_trigger))result.push(WeaponIntent::fire_up);
        if(new_press(1,gears_vr::control_a))result.push(WeaponIntent::xbox_a);
        if(new_press(1,gears_vr::control_b))result.push(WeaponIntent::xbox_b);
        if(new_press(0,gears_vr::control_x))result.push(WeaponIntent::xbox_x);
        if(new_press(0,gears_vr::control_y))result.push(WeaponIntent::xbox_y);
        baseline(input);result.status=RouterStatus::accepted;return result;
    }
};
}
