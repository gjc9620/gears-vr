#pragma once
#include "weapon_main_mount_provider.h"
#include "controller_continuity_frame.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>

// A distinct callback/data contract for an actual Native Tick/C0 consumer.
// It does not replace or change Rig/Hand/main-mount V1, and never constructs a
// fresh Hand Frame from retained data. No POD/id grants a writer or game call.
namespace gears_body_main_mount_provider {
namespace mount=gears_weapon_main_mount_provider;
namespace hand=gears_weapon_hand;
namespace rig=gears_rig_tracking;
inline constexpr std::uint32_t version=2;
inline constexpr std::uint64_t tracking_age_ns=40000000;
inline constexpr std::uint64_t scene_age_ns=50000000;
inline constexpr std::uint64_t equipment_age_ns=50000000;
using Status=mount::Status;

struct Tracking {
    // These are observations from the one real outer-Tick acquisition. Raw
    // timestamps/poses remain producer data, not a C0 stamp or renewed Scene.
    std::uint64_t tick{},thread{},parent_cookie{};
    std::int64_t begin_ns{},end_ns{};
    gears_vr::StereoSample stereo{};
    gears_vr::ControllerInputFrame controllers{};
    gears_hand_continuity::TrackedInput continuity{};
    rig::NativeReference native_reference{};
    std::uint64_t reference_revision{},publisher_pid{},publisher_start{};
};
struct Basis {
    // Native certifies this exact canonical Compose and current capture.
    // Matrices carry data only. PG does not return or authorize a world pose.
    std::uint64_t compose{};
    std::int64_t capture_begin_ns{};
    mount::BodyGeometry geometry{};
    gears_vr::StereoMatrix component_to_world{},base_view{};
};
struct Source {
    // Historical association is byte-for-byte the genuine original Scene.
    // Its original tracking may be stale for V1, which must still reject it.
    // V2 separately bounds actual GT metadata, tracking/capture and equipment
    // at 50/40/50 ms. Original Scene must be fresh at metadata BEGIN, remains
    // unchanged, and must still match the live Native cache/reference/ticket.
    rig::Frame original_scene{};
    std::uint64_t scene_ticket{},scene_cache_revision{};
    std::int64_t metadata_begin_ns{},metadata_end_ns{};
    Tracking tracking{};
    Basis basis{};
};
struct Request {
    std::uint32_t size{sizeof(Request)},abi_version{version};
    // Assigned/snapshotted by the admitted parent broker. Neither a caller's
    // query/tick id nor this header authenticates a live C0 by itself.
    std::uint64_t query_id{},hand_lifecycle{},action_generation{};
    std::uint64_t maximum_tracking_age_ns{tracking_age_ns};
    std::uint64_t maximum_scene_age_ns{scene_age_ns};
    std::uint64_t maximum_equipment_age_ns{equipment_age_ns};
    Source source{};
};
struct SourceKey {
    // Echo-only association. Native also checks immutable request bytes and
    // the actual live owner/lease/source/basis before/after and every write.
    // This intentionally omits some full-source fields; same(SourceKey) is
    // never a replacement for exact full-source/reference/basis validation.
    std::uint64_t scene_publication{},scene_binding{},scene_ticket{},scene_cache_revision{};
    std::uint64_t tick{},thread{},compose{},sequence{},session{},space_epoch{},source{};
    std::uint64_t reference_revision{},publisher_pid{},publisher_start{};
    std::int64_t begin_ns{},end_ns{},stereo_received_ns{},controllers_received_ns{},sync_ns{},capture_begin_ns{};
    std::int64_t metadata_begin_ns{},metadata_end_ns{};
};
inline SourceKey source_key(Source const& s) noexcept {
    SourceKey k{};auto const& t=s.tracking;
    k.scene_publication=s.original_scene.publication_sequence;k.scene_binding=s.original_scene.binding_generation;
    k.scene_ticket=s.scene_ticket;k.scene_cache_revision=s.scene_cache_revision;
    k.metadata_begin_ns=s.metadata_begin_ns;k.metadata_end_ns=s.metadata_end_ns;
    k.tick=t.tick;k.thread=t.thread;k.compose=s.basis.compose;k.sequence=t.stereo.sequence;
    k.session=t.stereo.session;k.space_epoch=t.stereo.space_epoch;k.source=std::uint64_t(t.stereo.source);
    k.reference_revision=t.reference_revision;k.publisher_pid=t.publisher_pid;k.publisher_start=t.publisher_start;
    k.begin_ns=t.begin_ns;k.end_ns=t.end_ns;k.stereo_received_ns=t.stereo.received_ns;
    k.controllers_received_ns=t.controllers.received_ns;k.sync_ns=t.controllers.sync_ns;k.capture_begin_ns=s.basis.capture_begin_ns;
    return k;
}
inline bool same(SourceKey const& a,SourceKey const& b) noexcept {
    return a.scene_publication==b.scene_publication&&a.scene_binding==b.scene_binding&&
        a.scene_ticket==b.scene_ticket&&a.scene_cache_revision==b.scene_cache_revision&&
        a.metadata_begin_ns==b.metadata_begin_ns&&a.metadata_end_ns==b.metadata_end_ns&&
        a.tick==b.tick&&a.thread==b.thread&&a.compose==b.compose&&a.sequence==b.sequence&&
        a.session==b.session&&a.space_epoch==b.space_epoch&&a.source==b.source&&
        a.reference_revision==b.reference_revision&&a.publisher_pid==b.publisher_pid&&a.publisher_start==b.publisher_start&&
        a.begin_ns==b.begin_ns&&a.end_ns==b.end_ns&&a.stereo_received_ns==b.stereo_received_ns&&
        a.controllers_received_ns==b.controllers_received_ns&&a.sync_ns==b.sync_ns&&a.capture_begin_ns==b.capture_begin_ns;
}
struct Response {
    std::uint32_t size{sizeof(Response)},abi_version{version};
    std::uint64_t query_id{},hand_lifecycle{},action_generation{};
    SourceKey source{};
    // Actual query-local PG observation times, not timestamps copied from the
    // old Hand Frame. begin precedes the first actual Game read; end follows
    // both assets/attachment/final Game/epoch/hold/domain checks. Validate
    // 0<begin<=end<=now and age from BEGIN, never renew earlier data with end.
    std::int64_t equipment_begin_ns{},equipment_end_ns{};
    // Data only: its original FrameKey remains the parent Scene/device key.
    // It is never renamed as a Tick/Compose Frame or used as an action grant.
    // Its query_id must also match the outer query; its own headers/reserved
    // fields/identities/profile still require exact independent validation.
    mount::Response mount{};
};

#if defined(_WIN32)
#define GEARS_BODY_MAIN_MOUNT_CALL __cdecl
#else
#define GEARS_BODY_MAIN_MOUNT_CALL
#endif
using Prepare=std::uint32_t(GEARS_BODY_MAIN_MOUNT_CALL*)(void*,Request const*,Response*) noexcept;
struct Registration {
    std::uint32_t size{sizeof(Registration)},abi_version{version};
    std::uint32_t request_bytes{sizeof(Request)},response_bytes{sizeof(Response)};
    std::uint64_t hand_cookie{},native_started_qpc{};
    void* context{};Prepare prepare{};
};
inline bool registration_shape_valid(Registration const& r) noexcept {
    return r.size==sizeof(Registration)&&r.abi_version==version&&r.request_bytes==sizeof(Request)&&
        r.response_bytes==sizeof(Response)&&r.hand_cookie&&r.native_started_qpc&&r.context&&r.prepare;
}
inline bool same(Registration const& a,Registration const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&a.request_bytes==b.request_bytes&&
        a.response_bytes==b.response_bytes&&a.hand_cookie==b.hand_cookie&&a.native_started_qpc==b.native_started_qpc&&
        a.context==b.context&&a.prepare==b.prepare;
}
using Attach=hand::Reply(GEARS_BODY_MAIN_MOUNT_CALL*)(Registration const*,hand::RegistrationResult*) noexcept;
inline constexpr char attach_export[]="GearsVrAttachBodyMainMountProviderV2";
#undef GEARS_BODY_MAIN_MOUNT_CALL

// Same active Hand parent context, cookie, pinned image, callback counts,
// reentry exclusion, revocation/lifecycle/action generation and drain as V1;
// no independent registrar, pin or detach. Callback is synchronous/read-only,
// engine-free: no publication, setter, native reentry, wait or retained pointer.
// PG obtains a distinct query-bound equipment witness; old Client Frame-fresh
// operations and old world_grip proof remain unchanged for V1 consumers.
// New tracking eligibility uses this genuine raw bundle. Native alone maps it
// through the certified current basis and reconstructs the fixed calibration.
// Non-ready response has no payload beyond the default size/version headers.
// Malformed reply/request mutation/reentry/fault uses the existing parent fault
// path. A newer packet may never replace this source during the same C0.
static_assert(sizeof(void*)==8&&sizeof(Tracking)==1792&&sizeof(Basis)==256&&sizeof(Source)==3904);
static_assert(sizeof(Request)==3960&&sizeof(SourceKey)==176&&sizeof(Response)==728&&sizeof(Registration)==48);
static_assert(alignof(Tracking)==8&&alignof(Basis)==8&&alignof(Source)==8&&alignof(Request)==8&&alignof(Response)==8);
static_assert(offsetof(Registration,hand_cookie)==16&&offsetof(Registration,context)==32&&offsetof(Registration,prepare)==40);
static_assert(offsetof(Request,query_id)==8&&offsetof(Request,source)==56);
static_assert(offsetof(Response,query_id)==8&&offsetof(Response,source)==32);
static_assert(std::is_standard_layout_v<Tracking>&&std::is_trivially_copyable_v<Tracking>);
static_assert(std::is_standard_layout_v<Basis>&&std::is_trivially_copyable_v<Basis>);
static_assert(std::is_standard_layout_v<Source>&&std::is_trivially_copyable_v<Source>);
static_assert(std::is_standard_layout_v<Request>&&std::is_trivially_copyable_v<Request>);
static_assert(std::is_standard_layout_v<Response>&&std::is_trivially_copyable_v<Response>);
static_assert(std::is_standard_layout_v<Registration>&&std::is_trivially_copyable_v<Registration>);
}
