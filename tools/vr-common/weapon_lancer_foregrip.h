#pragma once
#include "weapon_lancer_contact_dimensions.h"
#include "hand_authored_mount_profile.h"

namespace gears_weapon_lancer_foregrip {
// Fixed lower edge of the fore-end on original df356ff3 LOD0, vertex3096
// (100% b_AR_Root), ahead of the receiver and below E. Former vertex362
// belongs to b_AR_Receiver: its reference location is not a fixed gun socket.
// This is authored original geometry, not a stock support-wrist transform.
//10249 was a higher SIDE rib, leaving the palm beside the gun. Pair the
// lower edge with the broad support profile, not a tighter side fist.
//3096/3101 are split-normal vertices at the SAME side/chamfer boundary.
// A low Z or one vertex normal does not certify full-palm surface fitting.
// Actual45596 applies this pose, but its palm/proximal surface crossings
// remain unaccepted; do not infer anatomical fit from marker coincidence.
inline constexpr unsigned surface_vertex=3096;
inline constexpr gears_vr::Vector surface_raw{.6686892509460449,49.655517578125,3.170393943786621};
// Marcus115 palm pad, paired with its broad support finger profile. The
// Wrist approaches from the player's near side in the horizontal plane.
// Marcus LEFT thumb is on local -Z: point it FORWARD along the gun, with
// the index-to-little row toward the rear. Palm faces upward underneath.
inline constexpr gears_vr::Vector palm_surface_raw{8.,-1.6,0};
inline gears_vr::Quaternion wrist_in_mesh() noexcept {
    // Owner5112dfd7: thumb on the near side toward the muzzle, fingers around
    // the other side, forearm naturally behind the hand. The prior upward
    // palm still reversed this signed left-hand orientation (thumb rearward).
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
