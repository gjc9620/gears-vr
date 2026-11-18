#pragma once
#include "pose_math.h"
#include <cstdint>
namespace gears_boltok_close_support {
namespace vr=gears_vr;
inline constexpr std::uint64_t profile=0x424f4c544f4b0011ULL;
inline bool geometry(vr::Pose const& p) noexcept {
 const auto q=p.orientation;const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
 const double d=std::hypot(std::hypot(p.position.x,p.position.y),p.position.z);
 return vr::finite(p.position)&&std::isfinite(d)&&d>.005&&d<.35&&std::isfinite(n)&&std::abs(n-1.)<=.0001;
}
inline bool world(vr::Pose const& right,vr::Pose const& physical_left,vr::Pose const& local,double units,vr::Pose& out) noexcept {
 if(!geometry(local)||!vr::stereo_pose_valid(right)||!vr::stereo_pose_valid(physical_left)||!std::isfinite(units)||units<10||units>500)return false;
 const auto delta=vr::transform(vr::rotation_matrix(right.orientation),local.position);
 vr::Pose goal{{right.position.x+delta.x*units,right.position.y+delta.y*units,right.position.z+delta.z*units},vr::normalized(vr::multiply(right.orientation,local.orientation))};
 const auto p=physical_left.position;
 if(std::hypot(std::hypot(goal.position.x-p.x,goal.position.y-p.y),goal.position.z-p.z)>.25*units)return false;
 out=goal;return true;
}
}
