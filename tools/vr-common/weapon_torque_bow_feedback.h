#pragma once
#include "weapon_support_grip.h"
// One Bow feedback intent per authenticated PHYSICAL LGrip press. No ammo,
// reload/fire model, grant, timer, callback or input writer exists here.
namespace gears_torque_bow_feedback {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;
class Intent {
    hand::Frame source_{};std::uint64_t hold_{};bool known_{};
    bool cohort(hand::Frame const& f,std::uint64_t hold) const noexcept {
        auto a=source_.key,b=f.key;a.frame=b.frame=0;
        return hold_==hold&&hand::same(a,b)&&hand::same(source_.world,f.world)&&
            hand::same(source_.controller,f.controller)&&hand::same(source_.pawn,f.pawn)&&hand::same(source_.mesh,f.mesh)&&
            hand::same(source_.weapon,f.weapon)&&source_.reference_generation==f.reference_generation&&
            vr::stereo_same_pose(source_.reference,f.reference)&&source_.controllers.source==f.controllers.source&&
            source_.controllers.session==f.controllers.session&&source_.controllers.space_epoch==f.controllers.space_epoch&&
            source_.controllers.input_epoch==f.controllers.input_epoch&&source_.controllers.profile_generation==f.controllers.profile_generation;
    }
    void save(hand::Frame const& f,std::uint64_t hold) noexcept {source_=f;hold_=hold;known_=true;}
public:
    void lost() noexcept {known_=false;}
    bool advance(hand::Frame const& f,std::uint64_t hold,std::int64_t now,bool eligible) noexcept {
        const auto& h=f.controllers.hands[0];
        if(!hold||!hand::frame_active(f,now)||hand::index_pinch(f)||!f.left_tracked||!f.left_grip_known||h.squeeze_active!=1||
           !vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active)||f.controllers.sync_ns<=0||
           now<f.controllers.sync_ns||now-f.controllers.sync_ns>100000000){lost();return false;}
        const bool same=cohort(f,hold);
        if(same){
            if(f.controllers.sequence==source_.controllers.sequence&&f.controllers.sync_ns==source_.controllers.sync_ns)return false;
            // Keep the consumed-input high water even after lost(). An old
            // released packet cannot rearm a press that already got its cue.
            if(f.controllers.sequence<=source_.controllers.sequence||f.controllers.sync_ns<=source_.controllers.sync_ns){lost();return false;}
        }
        if(!same||!known_){save(f,hold);return false;} // Held startup/recovery is a baseline, never a new request.
        const auto& previous=source_.controllers;const auto& old=previous.hands[0];
        if(f.controllers.sync_ns-previous.sync_ns>100000000||h.pressed[vr::control_squeeze]<old.pressed[vr::control_squeeze]||
           h.released[vr::control_squeeze]<old.released[vr::control_squeeze]){save(f,hold);return false;}
        const auto presses=h.pressed[vr::control_squeeze]-old.pressed[vr::control_squeeze];
        const auto releases=h.released[vr::control_squeeze]-old.released[vr::control_squeeze];
        const bool down=f.left_grip_down!=0,was_down=source_.left_grip_down!=0;
        const bool new_press=down&&presses==1&&releases==(was_down?1u:0u)&&
            h.last_squeeze_press_sequence>previous.sequence&&h.last_squeeze_press_sequence<=f.controllers.sequence&&
            vr::controller_pose_usable(h.last_squeeze_press_grip,h.last_squeeze_press_flags,h.grip_active);
        // Consume even a refused/outside/nonempty intent BEFORE any caller can
        // dispatch its effect. Unknown original returns cannot replay this press.
        save(f,hold);return eligible&&new_press;
    }
};
}
