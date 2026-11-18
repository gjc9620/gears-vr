#pragma once
#include "hand_authored_mount_profile.h"

// Designer-authored held pose only. The caller must certify the ACTUAL current
// COGAssaultRifle asset, rig geometry, model scale and units. This profile grants
// no writer/lease, physical calibration or authoritative bore/muzzle/support
// grip certification. It must never mark a WeaponMount valid by itself.
namespace gears_weapon_lancer_wrist_mount {
using Signature=gears_hand_authored_mount::Signature;
inline constexpr char profile_id[]="marcus115-lancer-held-grip-v5-owner-down55";
inline constexpr std::uint32_t revision=5;
// Owner confirms residual ~20deg upward bore in ONE-handed neutral holding
// with R2's existing down35 profile. Fit down55 in this one shared mount;
// Native wrist, gun, support geometry and muzzle all consume this same basis.
// This is owner-specific authoring, not universal controller calibration.
inline constexpr gears_vr::Quaternion gun_pitch_down{0,.46174861323503391,0,.88701083317822171};
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
    // Rebuild Native and PG together. Fit the held hand with the SAME55degree
    // orientation as this gun while retaining the original palm marker.
    // Free hands keep their authored20degree pose; pickup blends the existing
    // part-specific wrist fit, never changes the physical controller path.
    const double h=std::sqrt(.5);
    Profile candidate{};candidate.revision=revision;candidate.provenance=authored::Provenance::designer_authored;
    candidate.mount=free;
    const auto free_q=free.calibration.grip_to_wrist_rotation;
    const auto marker=gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(free_q)),
        free.calibration.grip_to_wrist_metres);
    candidate.mount.calibration.grip_to_wrist_metres=gears_vr::transform(gears_vr::rotation_matrix(gun_pitch_down),marker);
    candidate.mount.calibration.grip_to_wrist_rotation=gun_pitch_down;
    const auto q=candidate.mount.calibration.grip_to_wrist_rotation;
    candidate.gun_in_wrist_rotation=gears_vr::multiply(gears_vr::multiply(
        gears_vr::Quaternion{-q.x,-q.y,-q.z,q.w},gun_pitch_down),gears_vr::Quaternion{0,0,-h,h});
    out=candidate;return true;
}
// Native's committed endpoint already places its origin at the actual palm,
// but deliberately retains the evaluated WRIST axes for its exact geometry
// proof. The weapon mount consumes GRIP axes. Remove the authored wrist bend
// once, at that consumer boundary; never turn the gun down with the hand fit.
inline bool grip_from_palm_endpoint(gears_vr::Pose const& endpoint,gears_vr::Pose& out) noexcept {
    const auto q=endpoint.orientation;const double norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(!gears_vr::finite(endpoint.position)||!std::isfinite(norm)||norm<.999||norm>1.001)return false;
    Profile p{};if(!marcus115(gears_hand_authored_mount::marcus115_signature,1,100,p))return false;
    auto grip=endpoint;
    grip.orientation=gears_vr::normalized(gears_vr::multiply(endpoint.orientation,
        gears_vr::inverse(p.mount.calibration.grip_to_wrist_rotation)));
    out=grip;return true;
}
} // namespace gears_weapon_lancer_wrist_mount
