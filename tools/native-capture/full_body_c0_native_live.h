#pragma once
#include "continuous_hand_owner_entry.h"
#include "full_body_c0_planner.h"
#include "body_head_authored_mount.h"
#include "../vr-common/body_main_mount_provider.h"
namespace full_body_c0_native_live {
bool main_mount_policy_enabled() noexcept;
bool body_mount_policy_enabled() noexcept;
bool body_mount_source_current(gears_body_main_mount_provider::Source const&) noexcept;
bool configure(wchar_t const*,FILE*);
void pose_begin(gears_continuous_hand_entry::Entry const&) noexcept;
// Synchronous policy seam: call AFTER pose_begin and BEFORE before_original,
// with the actual same-latch source used to construct the supplied Goal.
bool apply(gears_continuous_hand_entry::Entry const&,gears_full_body_c0::Goal const&,
           gears_native_rig_tracking::CoreSnapshot const&) noexcept;
bool apply_with_view(gears_continuous_hand_entry::Entry const&,gears_full_body_c0::Goal const&,
           gears_native_rig_tracking::CoreSnapshot const&,gears_vr::StereoMatrix const&) noexcept;
bool apply_with_view_marker(gears_continuous_hand_entry::Entry const&,gears_full_body_c0::Goal const&,
           gears_native_rig_tracking::CoreSnapshot const&,gears_vr::StereoMatrix const&,
           gears_body_head_authored::Marker const&,gears_vr::Pose const* eye=nullptr) noexcept;
void before_original(gears_continuous_hand_entry::Entry const&) noexcept;
void after_original(gears_continuous_hand_entry::Entry const&) noexcept;
void pose_end(gears_continuous_hand_entry::Entry const&,bool,bool) noexcept;
void request_drain();unsigned pending_count();void summary(FILE*);
}
