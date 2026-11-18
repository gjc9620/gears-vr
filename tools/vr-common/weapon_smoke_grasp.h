#pragma once
#include "weapon_lancer_wrist_mount.h"

// Basic production Smoke mount, not final GrabPose or headset acceptance.
// Original Smoke_Grenade LOD0 b_Bolo_Counter rigid-part bounds:
// (4.0551295,-2.8007438,-20.9654636)..(12.1428699,2.8007438,-5.3141890).
// Source export SHA256 bd94a1935a1d722db9f1e50c88efc4eaecad21410149a31da067cc9ae2fbfa6c.
// Its midpoint is only an initial handle marker. Keep chain/skin/physics owned
// by the engine. Current Smoke class/asset/scale/attachment admission is external.
namespace gears_weapon_smoke_grasp {
struct Profile {
    gears_hand_authored_mount::Mount hand{};
    gears_vr::Vector palm_in_mesh{},handle_in_mesh{8.098999738693237,0,-13.13982629776001};
    gears_vr::Matrix wrist_in_mesh{};
};
inline bool marcus115(gears_hand_authored_mount::Signature const& geometry,double scale,double units,
                     Profile& out,double mesh_scale=1) noexcept {
    Profile p{};gears_weapon_lancer_wrist_mount::Profile held{};
    if(!std::isfinite(mesh_scale)||mesh_scale<.1||mesh_scale>10||
       !gears_weapon_lancer_wrist_mount::marcus115(geometry,scale,units,held))return false;
    // Smoke uses the existing ordinary held-hand calibration, without a new
    // Native hand writer or the private Frag finger/rotation profile.
    p.hand=held.mount;const auto& c=p.hand.calibration;
    const auto marker=gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(c.grip_to_wrist_rotation)),
        {-c.grip_to_wrist_metres.x*units/scale,-c.grip_to_wrist_metres.y*units/scale,-c.grip_to_wrist_metres.z*units/scale});
    p.wrist_in_mesh=gears_vr::rotation_matrix({1,0,0,0});
    const gears_vr::Vector handle_in_wrist{9,-3,0};
    const auto delta=gears_vr::transform(p.wrist_in_mesh,
        {(marker.x-handle_in_wrist.x)*scale/mesh_scale,(marker.y-handle_in_wrist.y)*scale/mesh_scale,
         (marker.z-handle_in_wrist.z)*scale/mesh_scale});
    p.palm_in_mesh={p.handle_in_mesh.x+delta.x,p.handle_in_mesh.y+delta.y,p.handle_in_mesh.z+delta.z};
    if(!gears_vr::finite(p.palm_in_mesh))return false;out=p;return true;
}
inline bool release_centre(Profile const& p,double mesh_scale,double units,gears_vr::Vector& out) noexcept {
    if(!std::isfinite(mesh_scale)||mesh_scale<.1||mesh_scale>10||
       !std::isfinite(units)||units<10||units>500||!gears_vr::finite(p.palm_in_mesh))return false;
    const auto rotation=gears_vr::multiply(gears_vr::rotation_matrix(p.hand.calibration.grip_to_wrist_rotation),
                                        gears_vr::transpose(p.wrist_in_mesh));
    const auto shift=gears_vr::transform(rotation,
        {-p.palm_in_mesh.x*mesh_scale,-p.palm_in_mesh.y*mesh_scale,-p.palm_in_mesh.z*mesh_scale});
    const gears_vr::Vector result{shift.y/units,shift.z/units,-shift.x/units};
    if(!gears_vr::finite(result)||std::hypot(std::hypot(result.x,result.y),result.z)>.3)return false;
    out=result;return true;
}
}
