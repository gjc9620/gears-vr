#pragma once
#include "weapon_lancer_contact_dimensions.h"
#include "hand_authored_mount_profile.h"

namespace gears_weapon_lancer_foregrip {
// Lower positive-X ribbed fore-end on original df356ff3 LOD0 (vertex362),
// ahead of the receiver and below E. This is a designer-authored grasp on
// original geometry, not an original animation's support-wrist transform.
inline constexpr gears_vr::Vector surface_raw{4.00343132019043,41.054927825927734,5.872967720031738};
// Marcus115 palm pad, paired with its broad support finger profile. The
// wrist's long axis follows the barrel; do not turn it90degrees upward just
// to align the finger row. Palm faces inward and upward around the lower rib.
inline constexpr gears_vr::Vector palm_surface_raw{8.,-1.6,0};
inline gears_vr::Quaternion wrist_in_mesh() noexcept {
    // Z90*X180 followed by a20degree local-X roll. Keep the wrist long axis
    // on the barrel, with less imposed pronation than the former45degree
    // socket. The same measured palm point still touches the same lower rib;
    // neither the physical controller nor any arm bone length is moved.
    return {.696364240320019,.696364240320019,-.122787803968973,-.122787803968973};
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
