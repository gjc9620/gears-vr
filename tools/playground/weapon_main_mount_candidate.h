#pragma once
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/weapon_lancer_wrist_mount.h"

// PG-local preparation, NOT a Native wire contract, authorization or pose
// workspace. Native supplies/certifies BodyBinding only at its current C0
// binding; a Rig V1 world_grip alone does not prove that geometry or scale.
namespace gears_weapon_main_mount {
namespace hand=gears_weapon_hand;
namespace lancer=gears_weapon_lancer_wrist_mount;
inline constexpr std::uint64_t provider_revision=1;
// Semantic identifier, not a Native ABI enum or an extra rotation to apply.
enum class Basis:std::uint32_t {unknown,marcus_mesh_face_forward_y=2};
inline constexpr std::uint64_t supported_basis_revision=2;
inline constexpr char supported_basis_profile[]="marcus115-ref-eyes-face-forward-y-authored-v2";
struct BodyBinding {
    bool known{};hand::FrameKey key{};hand::ObjectIdentity mesh{};
    lancer::Signature geometry{};double model_scale{},units_per_metre{};
    std::uint64_t revision{},basis_revision{};Basis basis{};
};
struct AssetBinding {
    bool certified{},right_attached{};hand::FrameKey key{};
    hand::ObjectIdentity weapon{},component{},asset{};
};
struct Candidate {
    hand::FrameKey key{};
    hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{},component{},asset{};
    std::uint64_t hold_generation{},provider_revision{},body_revision{},basis_revision{};
    Basis basis{};
    lancer::Signature geometry{};double model_scale{},units_per_metre{};
    gears_hand_wrist::Calibration right{};
};
enum class Status:std::uint32_t {ready,inactive,no_weapon,unheld,animation,busy,not_firearm,
    unsupported_asset,not_attached,body_unavailable,body_mismatch,unsupported_geometry};
inline Status eligibility(hand::Frame const& f,hand::GripGameState const& game,
    std::uint64_t hold_generation,std::int64_t now) noexcept {
    if(!hand::frame_active(f,now)||f.recentered||f.heading_deferred||!game.known||
       !gears_vr::controller_pose_usable(f.controllers.hands[1].grip,f.controllers.hands[1].grip_flags,
           f.controllers.hands[1].grip_active))return Status::inactive;
    if(!f.weapon.address)return Status::no_weapon;
    if(!game.main_hand_held||!hold_generation)return Status::unheld;
    if(game.animation_owns_hands)return Status::animation;
    if(game.busy)return Status::busy;
    if(game.kind!=hand::WeaponKind::firearm)return Status::not_firearm;
    return Status::ready;
}
inline Status prepare(hand::Frame const& f,hand::GripGameState const& game,
    std::uint64_t hold_generation,AssetBinding const& asset,BodyBinding const& body,
    std::int64_t now,Candidate& out) noexcept {
    out={};const auto allowed=eligibility(f,game,hold_generation,now);
    if(allowed!=Status::ready)return allowed;
    if(!asset.certified||!hand::same(asset.key,f.key)||!hand::same(asset.weapon,f.weapon)||!hand::identity_valid(asset.component)||
       !hand::identity_valid(asset.asset))return Status::unsupported_asset;
    if(!asset.right_attached)return Status::not_attached;
    if(!body.known||!body.revision||!body.basis_revision)return Status::body_unavailable;
    if(body.basis!=Basis::marcus_mesh_face_forward_y||body.basis_revision!=supported_basis_revision||!hand::same(body.key,f.key)||!hand::same(body.mesh,f.mesh))return Status::body_mismatch;
    lancer::Profile profile{};
    if(!lancer::marcus115(body.geometry,body.model_scale,body.units_per_metre,profile))return Status::unsupported_geometry;
    Candidate value{};value.key=f.key;value.world=f.world;value.controller=f.controller;value.pawn=f.pawn;
    value.mesh=f.mesh;value.weapon=f.weapon;value.component=asset.component;value.asset=asset.asset;
    value.hold_generation=hold_generation;value.provider_revision=provider_revision;value.body_revision=body.revision;
    value.basis=body.basis;value.basis_revision=body.basis_revision;
    value.geometry=body.geometry;value.model_scale=body.model_scale;value.units_per_metre=body.units_per_metre;
    value.right=profile.mount.calibration;out=value;return Status::ready;
}
} // namespace gears_weapon_main_mount
