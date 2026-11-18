// Metres throughout. Pure cosmetic/geometry correction, never a source or action permit.
// Axial Y is preserved exactly; caller authenticates original producer history.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "weapon_boltok_cylinder_grasp.h"

namespace gears_boltok_loading_snap {
namespace owner=gears_boltok_owner_profiles;
namespace cylinder=gears_boltok_cylinder_grasp;
using P=gears_vr::Pose;
using V=gears_vr::Vector;
using Q=gears_vr::Quaternion;

struct Settings {
    bool enabled=true;
    bool insertion_assist=true;
    double seconds=.12;
    double radius_m=owner::ammo.volume_radius_metres;
    double snap_distance_m=owner::ammo.snap_distance_metres;
    double snap_angle_radians=owner::ammo.snap_angle_radians;
    double release_distance_m=owner::ammo.release_distance_metres;
};
inline constexpr Settings settings{};

inline V volume_center_in_feed(double physical_scale) noexcept {
    return gears_weapon::scaled(
        gears_weapon::difference(owner::ammo.volume_center_raw,cylinder::feed_raw),
        physical_scale*.01);
}

struct Correction {
    V lateral_m{};
    Q rotation{};
};

inline bool finite_pose(P const& p) noexcept {
    const auto q=p.orientation;
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return gears_vr::finite(p.position)&&std::isfinite(n)&&std::abs(n-1)<=1e-6;
}

inline bool quaternion_unit(Q const& q) noexcept {
    const double n2=double(q.x)*q.x+double(q.y)*q.y+
        double(q.z)*q.z+double(q.w)*q.w;
    return std::isfinite(n2)&&std::abs(n2-1.0)<=1e-6;
}

inline double quaternion_angle(Q const& a,Q const& b) noexcept {
    const double dot=std::abs(
        double(a.x)*b.x+double(a.y)*b.y+double(a.z)*b.z+double(a.w)*b.w);
    return 2.0*std::acos(std::clamp(dot,0.0,1.0));
}

inline P compose_local(P const& lhs,P const& rhs) noexcept {
    return cylinder::compose(lhs,rhs);
}

inline bool make(P const& raw_item,P const& item_in_grip,
    double physical_scale,Correction& out) noexcept {
    auto const& raw_item_in_face=raw_item.position;
    auto const& raw_item_orientation=raw_item.orientation;
    if(!std::isfinite(physical_scale)||physical_scale<=0||physical_scale>100)
        return false;
    if(!std::isfinite(raw_item_in_face.x)||!std::isfinite(raw_item_in_face.y)||
       !std::isfinite(raw_item_in_face.z)||!quaternion_unit(raw_item_orientation)||
       !finite_pose(item_in_grip))return false;
    const double radius=settings.radius_m*physical_scale;
    const auto from_center=gears_weapon::difference(raw_item_in_face,volume_center_in_feed(physical_scale));
    const double squared=from_center.x*from_center.x+from_center.y*from_center.y+from_center.z*from_center.z;
    if(!std::isfinite(radius)||squared>radius*radius+1e-9)return false;
    const Q face_identity{0,0,0,1};
    if(!std::isfinite(quaternion_angle(raw_item_orientation,face_identity))||
       quaternion_angle(raw_item_orientation,face_identity)>
           settings.snap_angle_radians+1e-6)return false;

    const auto inverse_rotation=gears_vr::inverse(item_in_grip.orientation);
    const P inverse_grip{gears_weapon::rotate(inverse_rotation,
        gears_weapon::scaled(item_in_grip.position,-1)),inverse_rotation};
    const P raw_grip=compose_local(raw_item,inverse_grip);
    const P goal{ {0,raw_item_in_face.y,0},face_identity };
    const P goal_grip=compose_local(goal,inverse_grip);
    const double distance=gears_weapon::length(gears_weapon::difference(
        raw_grip.position,goal_grip.position));
    if(!std::isfinite(distance)||
       distance>settings.snap_distance_m*physical_scale+1e-6)return false;

    Correction candidate{};
    candidate.lateral_m={-raw_item_in_face.x,0,-raw_item_in_face.z};
    candidate.rotation={-raw_item_orientation.x,-raw_item_orientation.y,
        -raw_item_orientation.z,raw_item_orientation.w};
    if(!std::isfinite(candidate.lateral_m.x)||
       !std::isfinite(candidate.lateral_m.z)||!quaternion_unit(candidate.rotation))
        return false;
    if(candidate.rotation.w<0){
        candidate.rotation.x=-candidate.rotation.x;
        candidate.rotation.y=-candidate.rotation.y;
        candidate.rotation.z=-candidate.rotation.z;
        candidate.rotation.w=-candidate.rotation.w;
    }
    out=candidate;
    return true;
}

inline Q normalize(Q const& q) noexcept {
    const double length=std::sqrt(
        double(q.x)*q.x+double(q.y)*q.y+double(q.z)*q.z+double(q.w)*q.w);
    return {q.x/length,q.y/length,q.z/length,q.w/length};
}

inline Q nlerp(Q const& a,Q const& b,double t) noexcept {
    double dot=double(a.x)*b.x+double(a.y)*b.y+double(a.z)*b.z+double(a.w)*b.w;
    Q target=b;
    if(dot<0){target={-b.x,-b.y,-b.z,-b.w};dot=-dot;}
    Q result{
        a.x+(target.x-a.x)*t,a.y+(target.y-a.y)*t,
        a.z+(target.z-a.z)*t,a.w+(target.w-a.w)*t};
    return normalize(result);
}

inline bool apply(P const& raw_pose,Correction const& correction,
    double physical_scale,double amount,P& out) noexcept {
    if(!finite_pose(raw_pose)||!std::isfinite(physical_scale)||
       physical_scale<=0||physical_scale>100||!std::isfinite(amount)||
       amount<0||amount>1||!std::isfinite(correction.lateral_m.y)||
       correction.lateral_m.y!=0||!quaternion_unit(correction.rotation))
        return false;
    const double radius=settings.radius_m*physical_scale;
    const double lateral=std::hypot(
        double(correction.lateral_m.x),double(correction.lateral_m.z));
    if(!std::isfinite(lateral)||lateral>radius+1e-6)return false;
    const double angle=quaternion_angle(correction.rotation,Q{0,0,0,1});
    if(!std::isfinite(angle)||angle>settings.snap_angle_radians+1e-6)return false;

    const double smooth=amount*amount*(3.0-2.0*amount);
    P candidate{};
    candidate.position={
        raw_pose.position.x+correction.lateral_m.x*smooth,
        raw_pose.position.y,
        raw_pose.position.z+correction.lateral_m.z*smooth};
    candidate.orientation=normalize(gears_vr::multiply(
        nlerp(Q{0,0,0,1},correction.rotation,smooth),raw_pose.orientation));
    if(!finite_pose(candidate))return false;
    out=candidate;
    return true;
}

inline bool keep(P const& raw_pose,double physical_scale) noexcept {
    if(!finite_pose(raw_pose)||!std::isfinite(physical_scale)||
       physical_scale<=0||physical_scale>100)return false;
    const double distance=gears_weapon::length(gears_weapon::difference(raw_pose.position,volume_center_in_feed(physical_scale)));
    const double limit=std::max(settings.release_distance_m,
        settings.snap_distance_m)*physical_scale;
    const double angle=quaternion_angle(raw_pose.orientation,Q{0,0,0,1});
    const double angle_limit=std::min(
        3.14159265358979323846,settings.snap_angle_radians+.35);
    return std::isfinite(distance)&&distance<=limit+1e-6&&
        std::isfinite(angle)&&angle<=angle_limit+1e-6;
}

// Constrain each authenticated RAW point to the fixed socket, instead of
// freezing an entry offset which becomes wrong as the user finishes reaching.
// Only XZ and orientation change. Time alone cannot create axial travel.
inline bool constrain(P const& raw,double physical_scale,bool keeping,
    double amount,P& out) noexcept {
    if(!std::isfinite(physical_scale)||physical_scale<=0||
       physical_scale>100||!finite_pose(raw)||!std::isfinite(amount)||
       amount<0||amount>1)return false;
    if(keeping){
        if(!keep(raw,physical_scale))return false;
    }else{
        const double radius=gears_boltok_loading_snap::settings.radius_m*
            physical_scale;
        const auto delta=gears_weapon::difference(raw.position,volume_center_in_feed(physical_scale));
        const double squared=delta.x*delta.x+delta.y*delta.y+delta.z*delta.z;
        if(!std::isfinite(radius)||squared>radius*radius+1e-9)return false;
        const double angle=quaternion_angle(raw.orientation,Q{0,0,0,1});
        if(!std::isfinite(angle)||
           angle>gears_boltok_loading_snap::settings.snap_angle_radians+1e-6)
            return false;
    }

    if(amount==0.0){
        P candidate=raw;
        if(!finite_pose(candidate))return false;
        out=candidate;
        return true;
    }

    const double smooth=amount*amount*(3.0-2.0*amount);
    P candidate{};
    candidate.position={
        raw.position.x*(1.0-smooth),
        raw.position.y,
        raw.position.z*(1.0-smooth)};
    if(amount==1.0){
        candidate.orientation={0,0,0,1};
    }else{
        candidate.orientation=nlerp(raw.orientation,Q{0,0,0,1},smooth);
    }
    if(!finite_pose(candidate)||candidate.position.y!=raw.position.y)
        return false;
    out=candidate;
    return true;
}

} // namespace gears_boltok_loading_snap
