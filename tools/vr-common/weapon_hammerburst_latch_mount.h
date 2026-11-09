#pragma once
#include "hand_authored_mount_profile.h"
#include "hand_wrist_calibration.h"

// Owned-value coordinate conversion only. This does not authenticate a weapon,
// approve an authored pose, acquire a lease, alter tracking or publish a hand.
// The caller supplies the SAME observed rigid gun/latch frames and still owns
// every existing identity, freshness, skin-fit and actual-action requirement.
namespace gears_hammerburst_latch_mount {
namespace vr=gears_vr;
namespace mount=gears_hand_authored_mount;
struct Profile {vr::Pose wrist_in_latch_reference_raw{};};
struct Target {
    vr::Pose wrist_world_raw{},grip_world_raw{},grip_in_weapon_metres{};
};
namespace detail {
inline vr::Vector add(vr::Vector a,vr::Vector b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline vr::Vector sub(vr::Vector a,vr::Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline vr::Vector scale(vr::Vector a,double s) noexcept {return {a.x*s,a.y*s,a.z*s};}
inline vr::Vector rotate(vr::Quaternion q,vr::Vector p) noexcept {return vr::transform(vr::rotation_matrix(q),p);}
inline bool pose(vr::Pose const& p) noexcept {
    const auto q=p.orientation;const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return vr::finite(p.position)&&std::isfinite(n)&&std::abs(n-1.)<=.0001;
}
}
// Frame positions are UE world raw units, orientations are rigid/unit quats.
// Part and gun scale are the observed uniform matrix scale. The authored raw
// wrist offset scales with that part once; the existing hand calibration uses
// body model scale separately. Mechanical acquisition and cosmetic publication
// must both consume this one resulting Grip, not two unrelated contact points.
// Failure leaves out unchanged. No radius/timestamp/permission is introduced.
inline bool target(Profile const& profile,vr::Pose const& latch_world_raw,
    double part_scale,vr::Pose const& weapon_world_raw,double weapon_scale,
    mount::Signature const& actual_body_geometry,double body_scale,
    double units_per_metre,Target& out) noexcept {
    if(!detail::pose(profile.wrist_in_latch_reference_raw)||!detail::pose(latch_world_raw)||
       !detail::pose(weapon_world_raw)||!std::isfinite(part_scale)||part_scale<.1||part_scale>10||
       !std::isfinite(weapon_scale)||weapon_scale<.1||weapon_scale>10||
       std::abs(part_scale-weapon_scale)>.001||!std::isfinite(body_scale)||body_scale<=0||body_scale>3)return false;
    mount::Mount hand{};
    if(!mount::marcus115(actual_body_geometry,mount::Side::left,body_scale,units_per_metre,hand))return false;
    Target candidate{};const auto& local=profile.wrist_in_latch_reference_raw;
    candidate.wrist_world_raw={detail::add(latch_world_raw.position,
        detail::rotate(latch_world_raw.orientation,detail::scale(local.position,part_scale))),
        vr::normalized(vr::multiply(latch_world_raw.orientation,local.orientation))};
    const auto grip_q=vr::normalized(vr::multiply(candidate.wrist_world_raw.orientation,
        vr::inverse(hand.calibration.grip_to_wrist_rotation)));
    const auto offset=detail::rotate(grip_q,detail::scale(hand.calibration.grip_to_wrist_metres,units_per_metre));
    candidate.grip_world_raw={detail::sub(candidate.wrist_world_raw.position,offset),grip_q};
    const auto inverse=vr::inverse(weapon_world_raw.orientation);
    candidate.grip_in_weapon_metres={detail::scale(detail::rotate(inverse,
        detail::sub(candidate.grip_world_raw.position,weapon_world_raw.position)),1/(weapon_scale*units_per_metre)),
        vr::normalized(vr::multiply(inverse,grip_q))};
    if(!detail::pose(candidate.wrist_world_raw)||!detail::pose(candidate.grip_world_raw)||
       !detail::pose(candidate.grip_in_weapon_metres))return false;
    out=candidate;return true;
}
}
