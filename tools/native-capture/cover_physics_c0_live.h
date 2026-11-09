#pragma once
#include <cstdio>
#include <cwchar>
#include "continuous_hand_owner_entry.h"
#include "../vr-common/native_rig_tracking_frame.h"
namespace gears_cover_physx_query {enum class Code;struct Box;struct Capsule;struct Vector;struct Overlap;struct Sweep;}
namespace cover_physics_c0_live {
bool configure(wchar_t const*,FILE*);
void request_drain()noexcept;
bool poll_stop()noexcept;
unsigned pending_count()noexcept;
void summary(FILE*);
// Raw query observation inside the current captured C0, before a body commit.
// No miss is movement/occupancy permission. Caller must supply PhysX geometry.
gears_cover_physx_query::Code overlap(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Box const&,gears_cover_physx_query::Overlap&)noexcept;
gears_cover_physx_query::Code overlap(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::Overlap&)noexcept;
gears_cover_physx_query::Code sweep(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Box const&,gears_cover_physx_query::Vector const&,gears_cover_physx_query::Sweep&)noexcept;
gears_cover_physx_query::Code sweep(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Capsule const&,gears_cover_physx_query::Vector const&,gears_cover_physx_query::Sweep&)noexcept;
}
