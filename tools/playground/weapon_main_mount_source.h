#pragma once
#include "weapon_main_mount_candidate.h"
#include "weapon_rig_binding.h"

// PG-local data agreement, NOT a wire format or Native authorization. The
// Native owner must certify actual current C0 geometry/scale and revalidate
// its context before/after use. known alone is not proof of that certification.
// No reader, pose, lease, setter, retained pointer or asynchronous cache.
namespace gears_weapon_main_mount_source {
namespace policy=gears_weapon_main_mount;
namespace rig=gears_rig_tracking;
namespace binding=gears_equipment_rig;
struct BodyContext {
    bool known{};rig::NativeKey native{};
    std::uint64_t publication_sequence{},scene_invocation{},world_generation{},binding_generation{},reference_generation{};
    gears_weapon_hand::ObjectIdentity mesh{};
    policy::lancer::Signature geometry{};double model_scale{},units_per_metre{};
    std::uint64_t revision{},basis_revision{};policy::Basis basis{};
};
enum class SourceStatus:std::uint32_t {ready,source_mismatch,body_unavailable,body_mismatch,unsupported_geometry};
inline SourceStatus bind(gears_weapon_hand::Frame const& hand,rig::Frame const& retained,
    BodyContext const& context,std::int64_t now,std::uint32_t expectedSource,
    policy::BodyBinding& out,binding::Status& agreement) noexcept {
    out={};agreement=binding::match(hand,retained,now,expectedSource);
    if(agreement!=binding::Status::matched)return SourceStatus::source_mismatch;
    if(!context.known||!context.revision)return SourceStatus::body_unavailable;
    if(!rig::same(context.native,retained.native)||context.publication_sequence!=retained.publication_sequence||
       context.scene_invocation!=retained.scene_invocation||context.world_generation!=retained.world_generation||
       context.binding_generation!=retained.binding_generation||context.reference_generation!=retained.reference_generation||
       !gears_weapon_hand::same(context.mesh,retained.mesh)||context.units_per_metre!=retained.units_per_metre||
       context.basis!=policy::Basis::marcus_mesh_face_forward_y||context.basis_revision!=policy::supported_basis_revision)
        return SourceStatus::body_mismatch;
    policy::lancer::Profile profile{};
    if(!policy::lancer::marcus115(context.geometry,context.model_scale,context.units_per_metre,profile))
        return SourceStatus::unsupported_geometry;
    policy::BodyBinding value{};value.known=true;
    // Copy only the actual committed hand key after exact retained-Scene
    // agreement. A C0/Kernel stamp must never be substituted for this key.
    value.key=hand.key;value.mesh=context.mesh;value.geometry=context.geometry;
    value.model_scale=context.model_scale;value.units_per_metre=context.units_per_metre;
    value.revision=context.revision;value.basis_revision=context.basis_revision;value.basis=context.basis;
    out=value;return SourceStatus::ready;
}
} // namespace gears_weapon_main_mount_source
