#pragma once
#include "hand_authored_mount_profile.h"

// Designer-authored held pose only. The caller must certify the ACTUAL current
// COGAssaultRifle asset, rig geometry, model scale and units. This profile grants
// no writer/lease, physical calibration or authoritative bore/muzzle/support
// grip certification. It must never mark a WeaponMount valid by itself.
namespace gears_weapon_lancer_wrist_mount {
using Signature=gears_hand_authored_mount::Signature;
inline constexpr char profile_id[]="marcus115-lancer-held-grip-v1";
inline constexpr std::uint32_t revision=1;
inline constexpr char supported_mesh[]="COGAssaultRifle";
struct Profile {
    std::uint32_t revision{};
    gears_hand_authored_mount::Provenance provenance{gears_hand_authored_mount::Provenance::unknown};
    gears_hand_authored_mount::Mount mount{};
    // Static orientation relation only, not an attachment translation/pose.
    // Mesh +Y forward/+Z up maps to Wrist +X forward/+Z up by -90deg yaw.
    gears_vr::Quaternion gun_in_wrist_rotation{};
};

// Reuse the authored free-hand palm marker, not a captured animation matrix.
// Its Wrist-local marker is -RF^T*tF. Held RH=identity, tH=RH*RF^T*tF,
// so the same marker remains at tracked Grip after the wrist is reoriented.
// Original Native weapon-bone animation increments remain Native-owned.
// All rejecting paths leave out byte-for-byte unchanged.
inline bool marcus115(Signature const& actualGeometry,double modelScale,double units,Profile& out) noexcept {
    namespace authored=gears_hand_authored_mount;
    authored::Mount free{};
    if(!authored::marcus115(actualGeometry,authored::Side::right,modelScale,units,free))return false;
    const auto held_translation=gears_vr::transform(gears_vr::transpose(
        gears_vr::rotation_matrix(free.calibration.grip_to_wrist_rotation)),free.calibration.grip_to_wrist_metres);
    if(!gears_vr::finite(held_translation))return false;
    Profile candidate{};candidate.revision=revision;candidate.provenance=authored::Provenance::designer_authored;
    candidate.mount=free;candidate.mount.calibration={true,held_translation,{0,0,0,1}};
    const double h=std::sqrt(.5);candidate.gun_in_wrist_rotation={0,0,-h,h};
    out=candidate;return true;
}
} // namespace gears_weapon_lancer_wrist_mount
