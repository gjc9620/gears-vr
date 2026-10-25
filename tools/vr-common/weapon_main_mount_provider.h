#pragma once
#include "rig_tracking_frame.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

// Supplemental data callback on an existing Hand V1 registration. These PODs
// grant no bone writer, equipment call, aim, muzzle or support-grip permission.
// Rig/Hand V1 layouts and their actual committed Scene keys remain unchanged.
namespace gears_weapon_main_mount_provider {
namespace hand=gears_weapon_hand;
namespace rig=gears_rig_tracking;
inline constexpr std::uint32_t version=1;
inline constexpr std::uint64_t maximum_source_age_ns=50000000;
inline constexpr std::uint64_t provider_revision=1;
inline constexpr std::uint64_t supported_basis_revision=2;
enum class Basis:std::uint32_t {unknown,marcus_mesh_face_forward_y=2};
using Signature=std::array<std::uint8_t,32>;

struct BodyGeometry {
    hand::ObjectIdentity mesh{};
    Signature geometry{};
    double model_scale{},units_per_metre{};
    std::uint64_t revision{},basis_revision{};
    Basis basis{};std::uint32_t reserved{};
};
struct Request {
    std::uint32_t size{sizeof(Request)},abi_version{version};
    // Assigned by the admitted broker, not by a C0/Kernel counter. The query id
    // correlates this synchronous reply; it is never a writer/ownership token.
    std::uint64_t query_id{},hand_lifecycle{},maximum_age_ns{maximum_source_age_ns};
    rig::Frame retained{};
    BodyGeometry body{};
};
enum class Status:std::uint32_t {
    ready,inactive,no_weapon,unheld,animation,busy,not_firearm,unsupported_asset,
    not_attached,body_unavailable,body_mismatch,unsupported_geometry,source_mismatch
};
inline bool status_valid(std::uint32_t value) noexcept {
    return value<=static_cast<std::uint32_t>(Status::source_mismatch);
}
struct Response {
    std::uint32_t size{sizeof(Response)},abi_version{version};
    std::uint64_t query_id{};
    hand::FrameKey key{};
    hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{},component{},asset{};
    std::uint64_t hold_generation{},provider_revision{},body_revision{},basis_revision{};
    Basis basis{};std::uint32_t reserved{};
    Signature geometry{};double model_scale{},units_per_metre{};
    std::uint32_t calibration_known{},calibration_reserved{};
    gears_vr::Vector grip_to_wrist_metres{};
    gears_vr::Quaternion grip_to_wrist_rotation{0,0,0,0};
};

#if defined(_WIN32)
#define GEARS_MAIN_MOUNT_CALL __cdecl
#else
#define GEARS_MAIN_MOUNT_CALL
#endif
using Prepare=std::uint32_t(GEARS_MAIN_MOUNT_CALL*)(void*,Request const*,Response*) noexcept;
struct Registration {
    std::uint32_t size{sizeof(Registration)},abi_version{version};
    std::uint32_t request_bytes{sizeof(Request)},response_bytes{sizeof(Response)};
    std::uint64_t hand_cookie{},native_started_qpc{};
    void* context{};Prepare prepare{};
};
inline bool registration_shape_valid(Registration const& r) noexcept {
    return r.size==sizeof(Registration)&&r.abi_version==version&&
        r.request_bytes==sizeof(Request)&&r.response_bytes==sizeof(Response)&&
        r.hand_cookie&&r.native_started_qpc&&r.context&&r.prepare;
}
inline bool same(Registration const& a,Registration const& b) noexcept {
    return a.size==b.size&&a.abi_version==b.abi_version&&a.request_bytes==b.request_bytes&&
        a.response_bytes==b.response_bytes&&a.hand_cookie==b.hand_cookie&&
        a.native_started_qpc==b.native_started_qpc&&a.context==b.context&&a.prepare==b.prepare;
}
using Attach=hand::Reply(GEARS_MAIN_MOUNT_CALL*)(Registration const*,hand::RegistrationResult*) noexcept;
inline constexpr char attach_export[]="GearsVrAttachWeaponMainMountProviderV1";
#undef GEARS_MAIN_MOUNT_CALL

// Attach once to the exact active parent cookie/native incarnation with the
// same context and callback code in that parent's already-pinned image. An
// identical retry is idempotent. There is no independent detach/pin/cookie.
// Parent unregister/Native stop stops admission and includes this callback in
// existing inflight/drain/quiescence. Neither timeout nor module pin alone
// permits PG context reset; only the existing parent worker unpins.
//
// Native calls synchronously at certified C0 GT after the body plan, before the
// single shared hand composer. The admitted broker fills a unique query id and
// the actual committed Hand lifecycle. retained is the same immutable Core
// projected for that admitted parent, never a fabricated C0-to-Scene Frame.
// Body revision is that retained Core binding generation; current C0 geometry,
// mesh, uniform scale, units and basis are separately certified by Native.
//
// The callback is engine-free/read-only: no game call/setter, Native reentry,
// wait, publication, pointer retention or asynchronous work. PG independently
// holds its active callback lifetime, checks GT/running/cookie and maps to its
// existing prepare_retained routine. The actual cached committed Hand Frame
// must agree with retained including key.frame==retained.scene_invocation.
//
// Non-ready returns an empty Response payload (size/version retained). Ready
// echoes query_id and supplies the full current candidate. Native revalidates
// the exact parent/lifecycle/Scene/objects/geometry/revisions and reconstructs
// the fixed held-Lancer calibration before choosing it for this C0 only.
// Malformed reply, request mutation, reentry or callback fault follows the
// parent fault/invalidation path. Ordinary unheld/busy/stale/unsupported status
// selects stock free-hand calibration and grants no extra ownership.
//
// Admission mutex is released around PG. Exact end accounting also covers
// stop/SEH; parent collect cannot unpin while this callback is in flight. No
// engine-capable restore context is created. Large request/response/snapshots
// use persistent caller/broker storage rather than an assembly hook stack.
static_assert(sizeof(void*)==8&&sizeof(BodyGeometry)==112&&sizeof(Registration)==48);
static_assert(offsetof(Registration,hand_cookie)==16&&offsetof(Registration,context)==32&&offsetof(Registration,prepare)==40);
static_assert(offsetof(Request,query_id)==8&&offsetof(Request,retained)==32);
static_assert(offsetof(Response,query_id)==8&&offsetof(Response,key)==16);
static_assert(std::is_standard_layout_v<BodyGeometry>&&std::is_trivially_copyable_v<BodyGeometry>);
static_assert(std::is_standard_layout_v<Request>&&std::is_trivially_copyable_v<Request>);
static_assert(std::is_standard_layout_v<Response>&&std::is_trivially_copyable_v<Response>);
static_assert(std::is_standard_layout_v<Registration>&&std::is_trivially_copyable_v<Registration>);
} // namespace gears_weapon_main_mount_provider
