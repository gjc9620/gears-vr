#pragma once
#include "pose_math.h"
#include "hand_authored_mount_profile.h"

namespace gears_weapon_lancer_contact {
// Visual authoring data for the supported COGAssaultRifle, not a tracking
// calibration or action permit. The settled original wrist origin/basis was
// already used by weapon_mesh_follow_test. Add the SAME authored palm marker
// used by the Native hand target, rather than treating the wrist as the palm.
inline bool palm_marker_in_wrist(gears_vr::Vector& out) noexcept {
    namespace a=gears_hand_authored_mount;
    a::Mount hand{};
    if(!a::marcus115(a::marcus115_signature,a::Side::right,1,100,hand))return false;
    const auto marker=gears_vr::transform(gears_vr::transpose(
        gears_vr::rotation_matrix(hand.calibration.grip_to_wrist_rotation)),
        {-100*hand.calibration.grip_to_wrist_metres.x,
         -100*hand.calibration.grip_to_wrist_metres.y,
         -100*hand.calibration.grip_to_wrist_metres.z});
    if(!gears_vr::finite(marker))return false;out=marker;return true;
}
inline bool authored(gears_vr::Vector& palm_in_mesh,gears_vr::Matrix& wrist_in_mesh) noexcept {
    gears_vr::Vector marker{};if(!palm_marker_in_wrist(marker))return false;
    constexpr double half_pi=1.5707963267948966;
    const auto basis=gears_vr::ue_matrix({0,half_pi,0});
    const auto shift=gears_vr::transform(basis,marker);
    // Lancer-only GripPoint authoring: clear the fixed palm and wrap the
    // original hand skin around this grip. The same mount positions mesh,
    // authoritative bore and muzzle; controller/wrist calibration is unchanged.
    const gears_vr::Vector contact{-4.131067499110079+shift.x,
        -.7928067234513434+shift.y,-1.1633055962145933+shift.z};
    if(!gears_vr::finite(contact))return false;
    palm_in_mesh=contact;wrist_in_mesh=basis;return true;
}
}
