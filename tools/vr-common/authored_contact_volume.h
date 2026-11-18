#pragma once
#include "pose_math.h"
#include <algorithm>
#include <cmath>

// Static workbench authoring geometry only. These predicates neither retain a
// grab nor grant input, animation, inventory or Native write authority. Callers
// still authenticate the current weapon/rig, current part transform and hands.
namespace gears_authored_contact_volume {
namespace vr=gears_vr;
enum class Shape : unsigned { sphere,box,capsule };
struct Volume {
    Shape shape{Shape::sphere};
    vr::Pose local{}; // workbench object coordinates, metres; NOT raw UE axes
    vr::Vector half{.06,.06,.06};
    double radius{.08},half_length{.06}; // capsule centre segment lies on local Y
    double snap_distance{.1},snap_angle{1.48},release_distance{.18};
    bool operator==(Volume const& b) const noexcept {
        return shape==b.shape&&local.position.x==b.local.position.x&&local.position.y==b.local.position.y&&
            local.position.z==b.local.position.z&&local.orientation.x==b.local.orientation.x&&
            local.orientation.y==b.local.orientation.y&&local.orientation.z==b.local.orientation.z&&
            local.orientation.w==b.local.orientation.w&&half.x==b.half.x&&half.y==b.half.y&&half.z==b.half.z&&
            radius==b.radius&&half_length==b.half_length&&snap_distance==b.snap_distance&&
            snap_angle==b.snap_angle&&release_distance==b.release_distance;
    }
};
inline bool pose_valid(vr::Pose const& p) noexcept {
    const auto q=p.orientation;
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return vr::finite(p.position)&&std::isfinite(n)&&std::abs(n-1.)<.01;
}
inline bool valid(Volume const& v) noexcept {
    if(!pose_valid(v.local)||!vr::finite(v.half)||v.half.x<0||v.half.y<0||v.half.z<0||
       !std::isfinite(v.radius)||v.radius<0||!std::isfinite(v.half_length)||v.half_length<0||
       !std::isfinite(v.snap_distance)||v.snap_distance<0||!std::isfinite(v.release_distance)||v.release_distance<0||
       !std::isfinite(v.snap_angle)||v.snap_angle<0||v.snap_angle>3.14159265358979323846)return false;
    switch(v.shape){
    case Shape::sphere:case Shape::capsule:return v.radius>0;
    case Shape::box:return v.half.x>0&&v.half.y>0&&v.half.z>0;
    default:return false;
    }
}
inline bool contains(Volume const& v,vr::Vector point_in_object) noexcept {
    if(!valid(v)||!vr::finite(point_in_object))return false;
    const auto p=vr::transform(vr::transpose(vr::rotation_matrix(v.local.orientation)),
        {point_in_object.x-v.local.position.x,point_in_object.y-v.local.position.y,point_in_object.z-v.local.position.z});
    if(v.shape==Shape::box)return std::abs(p.x)<=v.half.x&&std::abs(p.y)<=v.half.y&&std::abs(p.z)<=v.half.z;
    const double y=v.shape==Shape::capsule?p.y-std::clamp(p.y,-v.half_length,v.half_length):p.y;
    return p.x*p.x+y*y+p.z*p.z<=v.radius*v.radius;
}
inline bool acquire(Volume const& v,vr::Vector palm_in_object,vr::Pose wrist,vr::Pose target) noexcept {
    if(!contains(v,palm_in_object)||!pose_valid(wrist)||!pose_valid(target))return false;
    const double x=wrist.position.x-target.position.x,y=wrist.position.y-target.position.y,z=wrist.position.z-target.position.z;
    if(x*x+y*y+z*z>v.snap_distance*v.snap_distance)return false;
    const auto a=vr::normalized(wrist.orientation),b=vr::normalized(target.orientation);
    const double angle=2*std::acos(std::clamp(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w),0.,1.));
    return angle<=v.snap_angle;
}
} // namespace gears_authored_contact_volume
