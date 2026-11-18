#pragma once
#include "hand_authored_mount_profile.h"
#include "weapon_frag_grasp_pose.h"

// Authored visual mount for the separately authenticated original Frag mesh.
// This is an initial skin/handle fit, NOT a recovered stock hold animation or
// throw origin. Original chain animation/physics remain engine-owned. The live
// caller must verify the actual Frag class, mesh/package and attachment; this
// data grants no equipment, projectile or skeleton-write authority.
namespace gears_weapon_frag_grasp {
struct Profile {
    gears_hand_authored_mount::Mount hand{};
    gears_vr::Vector palm_in_mesh{},handle_in_mesh{gears_weapon_frag_grasp_pose::center_in_mesh};
    gears_vr::Matrix wrist_in_mesh{};
};
inline bool marcus115(gears_hand_authored_mount::Signature const& geometry,double scale,double units,Profile& out,double mesh_scale=1) noexcept {
    Profile p{};
    if(!std::isfinite(mesh_scale)||mesh_scale<.1||mesh_scale>10||
       !gears_hand_authored_mount::marcus115(geometry,gears_hand_authored_mount::Side::right,scale,units,p.hand))return false;
    const auto& c=p.hand.calibration;
    const auto marker=gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(c.grip_to_wrist_rotation)),
        {-c.grip_to_wrist_metres.x*units/scale,-c.grip_to_wrist_metres.y*units/scale,-c.grip_to_wrist_metres.z*units/scale});
    // Original root LOD0 shaft spans Z-6.612..8.357 at X8/Y0. The
    // previous Z-15 point lay on the Counter neck. This is an authored
    // root-shaft fit, not a claim about current animated skin or throw origin.
    // Original Counter/chain/end animation and physics remain engine owned.
    p.wrist_in_mesh=gears_vr::rotation_matrix(gears_weapon_frag_grasp_pose::wrist_in_mesh);
    const auto handle_in_wrist=gears_weapon_frag_grasp_pose::center_in_wrist;
    const auto delta=gears_vr::transform(p.wrist_in_mesh,
        {(marker.x-handle_in_wrist.x)*scale/mesh_scale,(marker.y-handle_in_wrist.y)*scale/mesh_scale,
         (marker.z-handle_in_wrist.z)*scale/mesh_scale});
    p.palm_in_mesh={p.handle_in_mesh.x+delta.x,p.handle_in_mesh.y+delta.y,p.handle_in_mesh.z+delta.z};
    if(!gears_vr::finite(p.palm_in_mesh))return false;out=p;return true;
}
inline bool grip_from_palm_endpoint(gears_vr::Pose const& endpoint,gears_vr::Pose& out) noexcept {
    const auto q=endpoint.orientation;const auto n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(!gears_vr::finite(endpoint.position)||!std::isfinite(n)||n<.999||n>1.001)return false;
    Profile p{};if(!marcus115(gears_hand_authored_mount::marcus115_signature,1,100,p))return false;
    auto grip=endpoint;
    grip.orientation=gears_vr::normalized(gears_vr::multiply(endpoint.orientation,
        gears_vr::inverse(p.hand.calibration.grip_to_wrist_rotation)));
    out=grip;return true;
}
}
