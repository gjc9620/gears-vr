#pragma once
#include "hand_marcus115_reference.h"
#include "weapon_hammerburst_owner_magazine.h"
#include "weapon_hammerburst_owner_support.h"

// HB part finger presets only. No weapon classification, runtime branch,
// grant, or shared V4 ABI change.
namespace gears_hammerburst_part_hand {
namespace api=gears_hand_authored_grasp;using Kind=api::object::Kind;
inline constexpr std::uint64_t support_profile_id=0x48425550504f5254ull;
inline constexpr std::uint64_t magazine_profile_id=0x48424d41475a4e45ull;

inline bool request(Kind kind,gears_weapon_hand::Frame const& source,
    gears_vr::Pose const& grip,api::Request& out) noexcept {
    const gears_vr::Quaternion* selected=nullptr;
    std::uint64_t profile_id{};
    if(kind==Kind::support){
        selected=gears_weapon_hammerburst_foregrip::captured_finger_locals;
        profile_id=support_profile_id;
    }else if(kind==Kind::magazine){
        selected=gears_weapon_hammerburst_owner_magazine::authored_finger_locals;
        profile_id=magazine_profile_id;
    }else return false;
    api::Request q{};q.source=source;q.grip=grip;q.kind=kind;
    q.geometry_signature=gears_hand_authored_mount::marcus115_signature;
    q.profile_id=profile_id;q.side=api::Side::left;q.mode=api::Mode::left_part;
    q.reference=gears_hand_marcus115_reference::left;
    for(unsigned i=0;i<15;++i)q.closed[i]=selected[i];
    if(!api::structural(q))return false;out=q;return true;
}
}