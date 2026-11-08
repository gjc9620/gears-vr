#pragma once
#include "hand_authored_mount_profile.h"

namespace gears_weapon_hammerburst_foregrip {
// Provisional fixed fore-end surface, HB LOD0 source 4b6bd784.
// Vertex2719 of hb-latch-part/b_HB_MagLatch-remainder.obj (610823e0).
// Geometry placement only; runtime surface/skin and headset fit unaccepted.
inline constexpr unsigned surface_vertex=2719;
inline constexpr gears_vr::Vector surface_raw{2.90855742,44.0751266,3.81094384};
// Marcus115 palm pad, paired with its broad support finger profile. The
// Wrist approaches from the player's near side in the horizontal plane.
// Marcus LEFT thumb is on local -Z: point it FORWARD along the gun, with
// the index-to-little row toward the rear. Palm faces upward underneath.
inline constexpr gears_vr::Vector palm_surface_raw{8.,-1.6,0};
inline gears_vr::Quaternion wrist_in_mesh() noexcept {
    // Shared Marcus left-palm anatomy; HB position is authored separately.
    // This is a proper rotation, not a mirrored skeleton or controller edit.
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
