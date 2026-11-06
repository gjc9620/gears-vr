#pragma once
#include "committed_hand_interaction.h"
#include "weapon_equipment_observation.h"

// Weapon-side observation helpers. Keep these out of the Native wire header:
// Native includes that wire inside its private implementation namespace.
namespace gears_committed_interaction {
inline hand::EquipmentIdentity equipment_identity(Frame const& f) noexcept {
    const auto& r=f.right;
    return {r.parent,r.world,r.controller,r.pawn,r.mesh,r.weapon,f.reference,r.reference_generation};
}
inline bool matches_retained_item(Frame const& f,hand::RetainedItemInput const& a,
    std::uint64_t held,std::int64_t now,bool sim) noexcept {
    const auto& r=f.right;const auto& c=r.controllers;const auto& left=c.hands[0];const auto& i=a.parent;
    const unsigned control=a.interaction_control==1?gears_vr::control_trigger:gears_vr::control_squeeze;
    // Original event counters exclude release/repress even if its intermediate
    // pose was missed. No new grant, interpolated travel or current game-state
    // claim is made by this observation-only association.
    return f.size==sizeof f&&f.abi_version==version&&held&&r.hold_generation==held&&
        r.purpose==gears_committed_hand::Purpose::held_lancer&&gears_committed_hand::current_result(r,now,sim)&&
        hand::equipment_identity_valid(i)&&hand::same(r.parent,i.key)&&
        hand::same(r.world,i.world)&&hand::same(r.controller,i.controller)&&hand::same(r.pawn,i.pawn)&&
        hand::same(r.mesh,i.mesh)&&hand::same(r.weapon,i.weapon)&&r.reference_generation==i.reference_generation&&
        gears_vr::stereo_same_pose(f.reference,i.reference)&&
        gears_vr::stereo_pose_valid(f.left_grip)&&gears_vr::stereo_pose_valid(f.left_wrist)&&
        std::isfinite(f.model_scale)&&f.model_scale>0&&
        hand::lease_valid(a.lease,i.key.lifecycle)&&a.press_token&&a.lease.id==a.press_token&&
        (a.lease.kind==hand::LeaseKind::magazine||a.lease.kind==hand::LeaseKind::ammo)&&
        c.source==a.source&&c.session==a.session&&c.space_epoch==a.space&&
        c.input_epoch==a.input&&c.profile_generation==a.profile&&c.sequence>=a.controller_sequence&&
        a.original_input_ns>0&&c.sync_ns>=a.original_input_ns&&a.decision_ns<=now&&
        a.interaction_control<=1&&left.pressed[control]==a.pressed&&left.released[control]==a.released&&
        hand::control_held(left,control)&&
        gears_vr::controller_pose_usable(left.grip,left.grip_flags,left.grip_active);
}
}
