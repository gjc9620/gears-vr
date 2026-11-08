#pragma once
#include "weapon_support_grip.h"

namespace gears_weapon_hammerburst_support {
namespace hand=gears_weapon_hand;
enum class Mode { free,snap,grab };
// Read-only view of the existing owner's decision. Snap never grants a lease.
inline Mode mode(hand::Frame const& f,hand::GripGameState const& game,
                 hand::Decision const& d,std::int64_t now) noexcept {
    if(!hand::frame_active(f,now)||!game.known||!game.main_hand_held||game.busy||
       game.animation_owns_hands||game.kind!=hand::WeaponKind::firearm||
       !f.left_tracked||!f.left_grip_known||!hand::same(f.key,d.key)||d.release_required||
       hand::index_pinch(f))return Mode::free;
    const auto& left=f.controllers.hands[0];
    if(left.squeeze_active!=1)return Mode::free;
    const bool down=hand::control_held(left,gears_vr::control_squeeze);
    if(down){
        return f.left_grip_down&&f.press_token&&d.lease.id==f.press_token&&
            hand::lease_valid(d.lease,f.key.lifecycle)&&d.lease.kind==hand::LeaseKind::support&&
            (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)?
            Mode::grab:Mode::free;
    }
    if(f.left_grip_down||d.lease.kind!=hand::LeaseKind::none||
       game.request_magazine||game.request_ammo)return Mode::free;
    for(auto const& lease:game.object_incumbents)if(lease.kind!=hand::LeaseKind::none)return Mode::free;
    return hand::support_contact(f,game,false)?Mode::snap:Mode::free;
}
}
