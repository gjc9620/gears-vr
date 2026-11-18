#pragma once
#include "weapon_hammerburst_owner_magazine.h"

namespace gears_weapon_hammerburst_handoff {
namespace vr=gears_vr;
namespace m=gears_weapon_hammerburst_owner_magazine;
// Positions are weapon METRES with gun scale removed. Ratios convert asset
// offsets and the Marcus marker into that same frame. No input is changed.
inline bool magazine_contact_in_weapon(vr::Pose bone_metres,double units,
    double static_over_gun,double body_over_gun,vr::Pose& grip_metres) noexcept {
    if(!m::valid_pose(bone_metres)||!std::isfinite(units)||units<10||units>500||
       !std::isfinite(body_over_gun)||body_over_gun<=0)return false;
    bone_metres.position={bone_metres.position.x*units,bone_metres.position.y*units,bone_metres.position.z*units};
    vr::Pose display{},wrist{},grip{};
    if(!m::static_world_from_bone(bone_metres,static_over_gun,display)||
       !m::authored_wrist_world_from_static(display,static_over_gun,wrist)||
       !m::grip_from_wrist(wrist,body_over_gun,units,grip))return false;
    grip.position={grip.position.x/units,grip.position.y/units,grip.position.z/units};
    if(!m::valid_pose(grip))return false;
    grip_metres=grip;return true;
}
inline bool in_contact(vr::Pose const& physical_metres,vr::Pose const& contact_metres,double gun_scale) noexcept {
    if(!m::valid_pose(physical_metres)||!m::valid_pose(contact_metres)||
       !std::isfinite(gun_scale)||gun_scale<=0)return false;
    const auto a=physical_metres.position,b=contact_metres.position;
    const double distance=std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)*gun_scale;
    return std::isfinite(distance)&&distance<=.06;
}
// A part-transition witness, never an input edge or action permit. The live
// caller separately validates the current Native decision on every tick.
struct State {
    bool active{},saw_up{};
    std::uint64_t latch_token{},magazine_token{};
    void reset() noexcept {*this={};}
    void opened(std::uint64_t token) noexcept {reset();active=true;latch_token=token;}
    void observe_up(bool known,bool down) noexcept {
        if(active&&known&&!down){saw_up=true;magazine_token=0;}
    }
    bool can_propose() const noexcept {return active&&(saw_up||magazine_token);}
    bool admit(bool known,bool down,std::uint64_t token,bool fresh_grant,bool contact) noexcept {
        if(!active||!known||!down||!token||!contact)return false;
        if(magazine_token)return token==magazine_token;
        if(!saw_up||token==latch_token||!fresh_grant)return false;
        magazine_token=token;saw_up=false;return true;
    }
};
} // namespace gears_weapon_hammerburst_handoff
