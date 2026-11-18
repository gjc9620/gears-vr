#pragma once
#include "weapon_lancer_contact_dimensions.h"
#include "hand_authored_mount_profile.h"
#include "weapon_lancer_owner_grabposes.h"

namespace gears_weapon_lancer_foregrip {
// Original df356ff3 LOD0 Root10252: lower near-side edge of the ribbed
// HANDGUARD PANEL, above the chainsaw/guard. Verified original skin binding
// is100% b_AR_Root. Former3096 was also Root-bound, but sits on the separate
// low strip by the chainsaw; that semantic mistake put the hand in the gap.
// Former362 is Receiver-bound and cannot be used as a fixed gun socket.
// Whole-hand mount is paired with weapon_lancer_natural_hand.h. Offline skin
// authoring is separate from sleeve, runtime and natural-grasp acceptance.
inline constexpr unsigned surface_vertex=10252;
inline constexpr gears_vr::Vector surface_raw{3.2114882469177246,46.81904983520508,7.580302715301514};
// Registration vector from the authored wrist to the real socket. Historical
// name retained for source compatibility; it is NOT a measured skin vertex.
// Palm/15 finger branches were fitted together using actual skin triangles.
// Owner-captured wrist paired with its15 joints; keep the original surface
// marker as a semantic reference rather than treating it as the wrist itself.
inline const gears_vr::Vector palm_surface_raw=[](){
    const auto& p=gears_lancer_owner_grabposes::support_wrist_in_gun;
    return gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(p.orientation)),
        {surface_raw.x-p.position.x,surface_raw.y-p.position.y,surface_raw.z-p.position.z});
}();
inline gears_vr::Quaternion wrist_in_mesh() noexcept {
    // Forward near-side support: thumb forward, separate finger wraps
    // authored on the original fore-end skin. Proper rotation, never a mirrored skeleton or
    // controller correction. Full arm/sleeve appearance still needs review.
    return gears_lancer_owner_grabposes::support_wrist_in_gun.orientation;
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
