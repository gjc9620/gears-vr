#pragma once
#include "weapon_lancer_authored_volume.h"
// Geometric reference for an acquired PASSIVE socket. It is never an input
// sample, part observation, lease, clock or permission to move the mechanism.
// Caller must prove the same live hold/epochs and actual current part first.
namespace gears_lancer_neutral_socket {
namespace vr=gears_vr;namespace a=gears_lancer_authored_volume;
struct Reference {bool known{};vr::Pose grip{},target{};std::array<double,16> delta{};};
inline bool contact(a::Volume const& region,vr::Pose const& grip,vr::Pose const& wrist,
    vr::Pose const& target,std::array<double,16> const& delta,double units,bool snap) noexcept {
    return a::pose_valid(grip)&&a::contains_raw_part_point(region,grip.position,delta)&&
        (!snap||a::wrist_snap(region,wrist,target,units));
}
inline bool acquire(Reference& out,a::Volume const& region,vr::Pose const& grip,vr::Pose const& wrist,
    vr::Pose const& target,std::array<double,16> const& delta,double units,bool snap) noexcept {
    if(!contact(region,grip,wrist,target,delta,units,snap))return false;
    out={true,grip,target,delta};return true;
}
inline bool retained(Reference const& ref,a::Volume const& region,vr::Pose const& grip,
    vr::Pose const& wrist,double units,bool snap) noexcept {
    if(!ref.known||!a::pose_valid(grip)||!a::pose_valid(ref.grip)||!std::isfinite(units)||units<=0)return false;
    const auto p=grip.position,q=ref.grip.position;
    // Same original 3cm neutral displacement, now in CURRENT gun-local space:
    // cosmetic unfolding cannot chase the hand, nor can moving the gun away
    // leave an old world-space contact latched. Never re-centre on each tick.
    if(std::hypot(std::hypot(p.x-q.x,p.y-q.y),p.z-q.z)>units*.03)return false;
    return contact(region,grip,wrist,ref.target,ref.delta,units,snap);
}
}
