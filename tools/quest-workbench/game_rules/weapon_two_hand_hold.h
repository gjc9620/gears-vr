#pragma once
#include "pose_math.h"

// Geometry/pose only. A caller must own the current support lease and weapon.
// Both grips remain authenticated observations; no input or bone is written.
namespace gears_weapon_two_hand {
namespace vr=gears_vr;
struct Solution {vr::Pose right{},left{};};
inline double length(vr::Vector v) noexcept {return std::hypot(std::hypot(v.x,v.y),v.z);}
inline vr::Vector sub(vr::Vector a,vr::Vector b) noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline bool geometry(vr::Pose const& foregrip) noexcept {
    const double n=length(foregrip.position);
    const auto q=foregrip.orientation;const double norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return vr::finite(foregrip.position)&&std::isfinite(n)&&n>.15&&n<.9&&std::isfinite(norm)&&norm>.999&&norm<1.001;
}
inline bool solve(vr::Pose const& right,vr::Pose const& left,vr::Pose const& foregrip,
    double units,Solution& out) noexcept try {
    if(!geometry(foregrip)||!vr::finite(right.position)||!vr::finite(left.position)||
       !std::isfinite(units)||units<10||units>500)return false;
    const auto expected=vr::transform(vr::rotation_matrix(right.orientation),foregrip.position);
    auto actual=sub(left.position,right.position);actual={actual.x/units,actual.y/units,actual.z/units};
    const double e=length(expected),a=length(actual);
    if(!std::isfinite(a)||a<.10||std::abs(a-e)>.25)return false;
    const vr::Vector from{expected.x/e,expected.y/e,expected.z/e},to{actual.x/a,actual.y/a,actual.z/a};
    const double dot=std::clamp(from.x*to.x+from.y*to.y+from.z*to.z,-1.,1.);
    if(dot<std::cos(.8))return false;
    const auto correction=vr::normalized(vr::Quaternion{
        from.y*to.z-from.z*to.y,from.z*to.x-from.x*to.z,from.x*to.y-from.y*to.x,1+dot});
    Solution s{};s.right={right.position,vr::normalized(vr::multiply(correction,right.orientation))};
    const auto offset=vr::transform(vr::rotation_matrix(s.right.orientation),foregrip.position);
    s.left={{right.position.x+offset.x*units,right.position.y+offset.y*units,right.position.z+offset.z*units},
        vr::normalized(vr::multiply(s.right.orientation,foregrip.orientation))};
    if(length(sub(s.left.position,left.position))>.25*units)return false;
    out=s;return true;
}catch(...){return false;}
}
