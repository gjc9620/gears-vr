#pragma once
#include "weapon_support_grip.h"
// One cosmetic request per actual NEW LGrip edge. No delayed intent, ammo,
// reload/fire permission, lease, timer or input publisher is created here.
namespace gears_boomshot_feedback {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;
class Intent {
    hand::Frame source_{};std::uint64_t hold_{};bool known_{};
    bool cohort(hand::Frame const& f,std::uint64_t hold)const noexcept {
        auto a=source_.key,b=f.key;a.frame=b.frame=0;
        return hold_==hold&&hand::same(a,b)&&hand::same(source_.world,f.world)&&hand::same(source_.controller,f.controller)&&
            hand::same(source_.pawn,f.pawn)&&hand::same(source_.mesh,f.mesh)&&hand::same(source_.weapon,f.weapon)&&
            source_.reference_generation==f.reference_generation&&vr::stereo_same_pose(source_.reference,f.reference)&&
            source_.stereo.session==f.stereo.session&&source_.stereo.space_epoch==f.stereo.space_epoch&&
            source_.controllers.source==f.controllers.source&&source_.controllers.session==f.controllers.session&&
            source_.controllers.space_epoch==f.controllers.space_epoch&&source_.controllers.input_epoch==f.controllers.input_epoch&&
            source_.controllers.profile_generation==f.controllers.profile_generation;
    }
    void save(hand::Frame const& f,std::uint64_t hold) noexcept {source_=f;hold_=hold;known_=true;}
public:
    void lost() noexcept {known_=false;} // Keep the high-water source; old packets cannot rearm it.
    bool advance(hand::Frame const& f,std::uint64_t hold,std::int64_t observed_ns,bool eligible) noexcept {
        const auto& left=f.controllers.hands[0];const bool down=f.left_grip_down!=0;
        if(!hold||!hand::frame_active(f,observed_ns)||hand::index_pinch(f)||!f.left_tracked||!f.left_grip_known||
           left.squeeze_active!=1||down!=hand::control_held(left,vr::control_squeeze)||
           !vr::controller_pose_usable(left.grip,left.grip_flags,left.grip_active)||f.controllers.sync_ns<=0||
           observed_ns<f.controllers.sync_ns||observed_ns-f.controllers.sync_ns>100000000){lost();return false;}
        const bool continuing=cohort(f,hold);
        if(continuing){
            if(f.controllers.sequence==source_.controllers.sequence&&f.controllers.sync_ns==source_.controllers.sync_ns)return false;
            if(f.key.frame<source_.key.frame||f.controllers.sequence<=source_.controllers.sequence||
               f.controllers.sync_ns<=source_.controllers.sync_ns){lost();return false;}
        }
        if(!continuing||!known_){save(f,hold);return false;}
        const auto& old=source_.controllers.hands[0];
        if(f.controllers.sync_ns-source_.controllers.sync_ns>100000000||
           left.pressed[vr::control_squeeze]<old.pressed[vr::control_squeeze]||
           left.released[vr::control_squeeze]<old.released[vr::control_squeeze]){save(f,hold);return false;}
        const auto presses=left.pressed[vr::control_squeeze]-old.pressed[vr::control_squeeze];
        const auto releases=left.released[vr::control_squeeze]-old.released[vr::control_squeeze];
        const bool new_press=down&&presses==1&&releases==(source_.left_grip_down?1u:0u)&&
            f.press_token>source_.press_token&&left.last_squeeze_press_sequence>source_.controllers.sequence&&
            left.last_squeeze_press_sequence<=f.controllers.sequence&&
            vr::controller_pose_usable(left.last_squeeze_press_grip,left.last_squeeze_press_flags,left.grip_active);
        // Consume even an ineligible/outside/nonempty edge before reflection,
        // so moving to the shoulder or an unknown original return cannot replay it.
        save(f,hold);return eligible&&new_press;
    }
};
}
