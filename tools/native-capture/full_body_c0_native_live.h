#pragma once
#include <climits>
#include "continuous_hand_owner_entry.h"
#include "full_body_c0_planner.h"
#include "body_head_authored_mount.h"
#include "../vr-common/body_main_mount_provider.h"
#include "../vr-common/committed_hand_endpoint.h"
#include "../vr-common/committed_hand_interaction.h"
#include "../vr-common/weapon_lancer_held_contact.h"
namespace gears_full_body_c0_native {struct ConsumedPose;}
namespace full_body_c0_native_live {
// Passive correlation for the existing opt-in GPU palette journal. No source,
// pose, lease or writer permission can be obtained from this observation.
struct ComposeObservation {
    int64_t begin_ns{},end_ns{};uintptr_t mesh{};
    uint64_t tick{},compose{};unsigned selected{},begin_result{UINT_MAX},apply_gate{UINT_MAX},
        commit_result{UINT_MAX},solved_mask{},returned{},original_called{};
    unsigned mount_kind{UINT_MAX},mount_prequery{UINT_MAX},mount_query_code{UINT_MAX},
        mount_query_branch{UINT_MAX},mount_callback_status{UINT_MAX},mount_admission{UINT_MAX},hand_status{UINT_MAX};
    uint64_t mount_query{},caller{};
    unsigned source_slot_active{};
    unsigned consumer_result{UINT_MAX},finish_result{UINT_MAX},finish_failure{UINT_MAX},
        endpoint_publish_stage{UINT_MAX},endpoint_publish_flags{},retained_failure{UINT_MAX};
    gears_vr::Vector right_target{}; // Original physical tracking target.
    gears_vr::Vector right_solved_target{}; // Final fixed-length/cosmetic solve target.
};
bool observe_last_compose(uintptr_t,ComposeObservation&) noexcept;
gears_rig_tracking::Reply read_endpoint(gears_committed_hand::Request const*,gears_committed_hand::Frame*) noexcept;
gears_rig_tracking::Reply read_interaction(gears_committed_interaction::Request const*,gears_committed_interaction::Frame*) noexcept;
// Retrospective CPU pose data from the exact original C0 callback. The reader
// runs after Kernel finish and before Backend invalidate; this API only copies
// that result and never reacquires a write/geometry lease. Call synchronously
// with the original Entry and unmodified Core; do not retain callback pointers.
bool read_completed_cover(gears_continuous_hand_entry::Entry const&,
    gears_native_rig_tracking::CoreSnapshot const&,gears_full_body_c0_native::ConsumedPose&) noexcept;
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
