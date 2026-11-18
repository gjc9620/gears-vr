#pragma once
#include "hand_authored_mount_profile.h"
#include "weapon_support_volume.h"

namespace gears_weapon_longshot_foregrip {
// Original Longshot LOD0 vertex 6423 (zero-based), source export SHA256
// 741d96530f076bf7d3a98d31c9813e2e65f566eda193eaa5f5ea6710c5421e9f.
// Provisional fore-end contact selected from the reviewed side/top model.
// Live skin/contact verification remains required.
inline constexpr unsigned surface_vertex=6423;
inline constexpr gears_vr::Vector surface_raw{.7677327,43.251,3.37458};
inline constexpr gears_vr::Vector palm_surface_raw{8.,-1.6,0};
inline gears_weapon::SupportBox contact_box(gears_vr::Matrix const& mesh_axes_in_right_ue) noexcept {
    gears_weapon::SupportBox box{};box.enabled=true;
    // Original mesh +Y is the barrel direction. Full acquire W/L/H is
    // 12/16/12cm and keep W/L/H is16/20/16cm. UE vector -> XR vector.
    constexpr gears_vr::Matrix conversion{{{0,1,0},{0,0,1},{-1,0,0}}};
    box.axes=gears_vr::multiply(conversion,mesh_axes_in_right_ue);
    box.acquire_half={.06,.08,.06};box.retain_half={.08,.10,.08};return box;
}
inline gears_vr::Quaternion wrist_in_mesh() noexcept {
    return {.18301270189221933,.6830127018922193,-.6830127018922193,-.18301270189221933};
}
inline bool grip_in_mesh(gears_vr::Vector& out) noexcept {
    gears_hand_authored_mount::Mount left{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::left,1,100,left))return false;
    const auto marker=gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(left.calibration.grip_to_wrist_rotation)),
        {-100*left.calibration.grip_to_wrist_metres.x,-100*left.calibration.grip_to_wrist_metres.y,-100*left.calibration.grip_to_wrist_metres.z});
    const auto axes=gears_vr::rotation_matrix(wrist_in_mesh());
    const auto palm=gears_vr::transform(axes,palm_surface_raw);
    const auto shift=gears_vr::transform(axes,marker);
    out={surface_raw.x-palm.x+shift.x,surface_raw.y-palm.y+shift.y,surface_raw.z-palm.z+shift.z};
    return gears_vr::finite(out);
}
}
