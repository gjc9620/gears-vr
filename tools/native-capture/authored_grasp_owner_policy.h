#pragma once
#include "../vr-common/hand_authored_grasp.h"

// Value-only classification from the existing admitted/closed Scene binding.
// No registry read/cache, action grant or physical hand override.
namespace gears_authored_grasp_owner {
namespace hand=gears_weapon_hand;namespace api=gears_hand_authored_grasp;
enum class Family:unsigned {unknown,other,lancer};
enum class Need:unsigned {reject,none,lancer_loading,lancer_starter};
struct Actor {
    hand::ObjectIdentity world{},pawn{},mesh{},weapon{};
    std::uint64_t world_generation{};Family family{};bool context_valid{},weapon_known{};
};
inline Need required(api::Request const& q,Actor const& actor) noexcept {
    if(!actor.context_valid||!actor.weapon_known||actor.family==Family::unknown||unsigned(actor.family)>unsigned(Family::lancer)||
       !actor.weapon.address||actor.world_generation!=q.source.key.world_generation||
       !hand::same(actor.world,q.source.world)||!hand::same(actor.pawn,q.source.pawn)||
       !hand::same(actor.mesh,q.source.mesh)||!hand::same(actor.weapon,q.source.weapon))return Need::reject;
    if(actor.family!=Family::lancer||q.mode==api::Mode::right_main_fingers)return Need::none;
    if(q.kind==api::object::Kind::loading)return Need::lancer_loading;
    if(q.kind==api::object::Kind::starter)return Need::lancer_starter;
    return Need::none;
}
}
