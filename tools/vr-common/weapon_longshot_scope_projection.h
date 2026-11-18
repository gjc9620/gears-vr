#pragma once
#include "stereo_frame.h"

namespace gears_weapon::longshot_scope_projection {
// Full-screen near-eye fallback, not the physical lens/PiP renderer. A live
// owner must supply the current original targeting FOV and current real bore.
// The normal body/cover/HMD/rig basis, stereo views and depth stay untouched.
// Only this frame's projection is narrowed and centred on the real bore ray.
inline bool select(gears_vr::StereoMatrices const& normal,gears_vr::Vector bore_world,
                   double target_horizontal_fov_degrees,gears_vr::StereoMatrices& out) noexcept {
    if(!gears_vr::finite(bore_world)||!std::isfinite(target_horizontal_fov_degrees)||
       target_horizontal_fov_degrees<2||target_horizontal_fov_degrees>90)return false;
    const double length=std::hypot(std::hypot(bore_world.x,bore_world.y),bore_world.z);
    if(!std::isfinite(length)||std::abs(length-1.)>1e-5)return false;
    constexpr double radians=3.14159265358979323846/180;
    const double horizontal=1/std::tan(target_horizontal_fov_degrees*radians/2);
    auto result=normal;
    for(unsigned eye=0;eye<2;++eye){
        const auto& view=normal.view[eye];const auto& projection=normal.projection[eye];
        if(!gears_vr::stereo_rigid(view)||!gears_vr::stereo_projection_supported(projection))return false;
        const gears_vr::Vector ray{bore_world.x*view[0]+bore_world.y*view[4]+bore_world.z*view[8],
            bore_world.x*view[1]+bore_world.y*view[5]+bore_world.z*view[9],
            bore_world.x*view[2]+bore_world.y*view[6]+bore_world.z*view[10]};
        // Same maximum angular keep bound as the scope eye box. This is a
        // geometric guard, not a substitute for live ocular/eye admission.
        if(ray.z<std::cos(22*radians))return false;
        const double magnification=horizontal/projection[0];
        if(!std::isfinite(magnification)||magnification<1||magnification>64)return false;
        auto& p=result.projection[eye];
        p[0]=static_cast<float>(horizontal);
        p[5]=static_cast<float>(double(projection[5])*magnification);
        p[8]=static_cast<float>(-ray.x/ray.z*p[0]);
        p[9]=static_cast<float>(-ray.y/ray.z*p[5]);
        if(!gears_vr::stereo_projection_supported(p))return false;
    }
    out=result;return true;
}
}
