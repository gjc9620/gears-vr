#pragma once
#include "stereo_frame.h"

// Geometry comparison only. Neither pose gets a new timestamp or permission.
// The caller separately authenticates a current tracking publication and a
// current committed render result before choosing its action source.
namespace gears_hand_contact_equivalence {
inline bool same_contact(gears_vr::Pose const& tracking,gears_vr::Pose const& rendered,
                         double units_per_metre) noexcept {
    if(!std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500||
       !gears_vr::stereo_pose_valid(tracking)||!gears_vr::stereo_pose_valid(rendered))return false;
    const double tolerance=.00002*units_per_metre; //20 micrometres, float pose roundoff only.
    if(std::abs(tracking.position.x-rendered.position.x)>tolerance||
       std::abs(tracking.position.y-rendered.position.y)>tolerance||
       std::abs(tracking.position.z-rendered.position.z)>tolerance)return false;
    const auto a=gears_vr::normalized(tracking.orientation),b=gears_vr::normalized(rendered.orientation);
    const double sign=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0?-1.:1.;
    return std::abs(a.x-sign*b.x)<=1e-6&&std::abs(a.y-sign*b.y)<=1e-6&&
        std::abs(a.z-sign*b.z)<=1e-6&&std::abs(a.w-sign*b.w)<=1e-6;
}
}
