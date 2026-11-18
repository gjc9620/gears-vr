#pragma once
#include "weapon_longshot_scope_control.h"

namespace gears_weapon::longshot_eye_contact {
// Original neutral LOD0 rear glass disk, 18 ring vertices and18 triangles.
// OBJ SHA256 bc0191628b53166e1cd19bc1802bf277f9ec60975ffd125a2e4783f54f0795c7.
// Geometry fit is not a measured exit pupil or a live-render calibration.
inline constexpr gears_vr::Vector ocular_center_raw{-2.5813411041628322,20.38986,18.036765533341583};
inline constexpr double glass_radius_raw=1.853067492876909;
inline bool ocular_world_m(gears_vr::Pose const& mesh_world_units,double scale,double units,
                           gears_vr::Pose& out) noexcept {
    if(!gears_vr::stereo_pose_valid(mesh_world_units)||!std::isfinite(scale)||scale<.1||scale>10||
       !std::isfinite(units)||units<10||units>500)return false;
    const auto delta=gears_vr::transform(gears_vr::rotation_matrix(mesh_world_units.orientation),
        {ocular_center_raw.x*scale,ocular_center_raw.y*scale,ocular_center_raw.z*scale});
    // UE forward/right/up basis: barrel +Y, right -X, up +Z in the mesh.
    const gears_vr::Quaternion in_mesh{0,0,.7071067811865475,.7071067811865475};
    const auto orientation=gears_vr::multiply(mesh_world_units.orientation,in_mesh);
    const gears_vr::Pose candidate{{(mesh_world_units.position.x+delta.x)/units,
        (mesh_world_units.position.y+delta.y)/units,(mesh_world_units.position.z+delta.z)/units},orientation};
    if(!gears_vr::stereo_pose_valid(candidate))return false;out=candidate;return true;
}
// Both poses use UE forward/right/up basis, with positions in world metres.
// Do not feed them to the XR -Z-forward contact helper without conversion.
inline longshot_scope_control::EyeContact contact_ue(gears_vr::Pose const& ocular,
    gears_vr::Pose const& eye,bool tracked) noexcept {
    if(!tracked||!gears_vr::stereo_pose_valid(ocular)||!gears_vr::stereo_pose_valid(eye))return {};
    const auto inverse=gears_vr::transpose(gears_vr::rotation_matrix(ocular.orientation));
    const auto p=gears_vr::transform(inverse,{eye.position.x-ocular.position.x,
        eye.position.y-ocular.position.y,eye.position.z-ocular.position.z});
    const auto forward=gears_vr::transform(inverse,gears_vr::transform(gears_vr::rotation_matrix(eye.orientation),{1,0,0}));
    return {-p.x,std::hypot(p.y,p.z),std::acos((std::clamp)(forward.x,-1.,1.))*180/3.14159265358979323846,true};
}
}
