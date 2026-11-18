#pragma once
#include "weapon_hand_participant.h"
#include <array>

namespace gears_boltok_completed_grasp {
struct Identity {
    std::uint64_t reference{},world_generation{},lifecycle{};
    gears_weapon_hand::ObjectIdentity weapon{},component{};
};
inline bool exact(gears_weapon_hand::Frame const& f,gears_weapon_hand::Lease const& lease,
    Identity const& identity,gears_weapon_hand::ObjectIdentity const& component) noexcept {
    return lease.kind==gears_weapon_hand::LeaseKind::magazine&&lease.id&&
        lease.id==f.press_token&&gears_weapon_hand::lease_valid(lease,f.key.lifecycle)&&
        identity.reference==f.reference_generation&&identity.world_generation==f.key.world_generation&&
        identity.lifecycle==f.key.lifecycle&&gears_weapon_hand::same(identity.weapon,f.weapon)&&
        gears_weapon_hand::identity_valid(component)&&gears_weapon_hand::same(identity.component,component);
}
// Return true only when the sole retained incumbent was projected. A foreign
// incumbent or a missing/duplicate exact slot is never overwritten.
inline bool project(std::array<gears_weapon_hand::Lease,2>& slots,
    gears_weapon_hand::Lease const& lease) noexcept {
    unsigned matches{};
    for(auto const& slot:slots){
        if(!gears_weapon_hand::lease_valid(slot,lease.lifecycle))return false;
        if(slot.kind==gears_weapon_hand::LeaseKind::none)continue;
        if(!gears_weapon_hand::same(slot,lease)||++matches>1)return false;
    }
    if(matches)return true;
    slots[0]=lease;return true;
}
}
