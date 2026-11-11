#pragma once
#include <cstdio>
#include <cwchar>
#include "continuous_hand_owner_entry.h"
#include "../vr-common/native_rig_tracking_frame.h"
namespace gears_cover_physx_query {enum class Code;struct Sphere;struct Box;struct Capsule;struct Vector;struct Overlap;struct Sweep;struct StreamLimits;struct AllSweep;struct AllSweepOwners;struct AllOverlap;struct AllOverlapOwners;struct AssetShapeWorkspace;}
namespace gears_cover_physx_asset_shapes {struct Snapshot;}
namespace gears_cover_capsule_path {struct Policy;struct RawPath;}
namespace cover_physics_c0_live {
// Read-only complete supplied PhysicsAsset inventory in the same admitted C0.
// Scratch/result must be caller-owned heap/TLS. No movement permission follows.
gears_cover_physx_query::Code observe_asset_shapes(gears_continuous_hand_entry::Entry const&,
    gears_native_rig_tracking::CoreSnapshot const&,gears_cover_physx_query::AssetShapeWorkspace&,
    gears_cover_physx_asset_shapes::Snapshot&)noexcept;
gears_cover_physx_query::Code overlap_all(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::StreamLimits const&,gears_cover_physx_query::AllOverlap&)noexcept;
gears_cover_physx_query::Code overlap_all(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::StreamLimits const&,gears_cover_physx_query::AllOverlapOwners&)noexcept;
gears_cover_physx_query::Code sweep_capsule_path(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::Vector const&,gears_cover_capsule_path::Policy const&,
    gears_cover_physx_query::StreamLimits const&,gears_cover_capsule_path::RawPath&)noexcept;
bool configure(wchar_t const*,FILE*);
void request_drain()noexcept;
bool poll_stop()noexcept;
unsigned pending_count()noexcept;
void summary(FILE*);
// Explicit mutually-exclusive one-shot sphere/path/shape observation per load.
// Eye input is in GAME units; sphere/path convert to PhysX before dispatch.
// Called after current C0 eye measurement; never grants movement or occupancy.
void observe_sphere_once(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Vector const&)noexcept;
// Raw query observation inside the current captured C0, before a body commit.
// No miss is movement/occupancy permission. Caller must supply PhysX geometry.
gears_cover_physx_query::Code overlap(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Sphere const&,gears_cover_physx_query::Overlap&)noexcept;
gears_cover_physx_query::Code overlap(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Box const&,gears_cover_physx_query::Overlap&)noexcept;
gears_cover_physx_query::Code overlap(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::Overlap&)noexcept;
gears_cover_physx_query::Code sweep(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Box const&,gears_cover_physx_query::Vector const&,gears_cover_physx_query::Sweep&)noexcept;
gears_cover_physx_query::Code sweep(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::Vector const&,gears_cover_physx_query::Sweep&)noexcept;
gears_cover_physx_query::Code sweep_all(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Box const&,gears_cover_physx_query::Vector const&,gears_cover_physx_query::StreamLimits const&,gears_cover_physx_query::AllSweep&)noexcept;
gears_cover_physx_query::Code sweep_all(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::Vector const&,gears_cover_physx_query::StreamLimits const&,gears_cover_physx_query::AllSweep&)noexcept;
gears_cover_physx_query::Code sweep_all(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Box const&,gears_cover_physx_query::Vector const&,gears_cover_physx_query::StreamLimits const&,gears_cover_physx_query::AllSweepOwners&)noexcept;
gears_cover_physx_query::Code sweep_all(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::Vector const&,gears_cover_physx_query::StreamLimits const&,gears_cover_physx_query::AllSweepOwners&)noexcept;
}
