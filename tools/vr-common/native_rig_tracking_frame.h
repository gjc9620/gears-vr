#pragma once
#include "rig_tracking_frame.h"

// Native-owned data only. No store, bridge read, clock query, setter, scope,
// identity authentication or live ticket/reference/stop check is supplied here.
namespace gears_native_rig_tracking {
namespace rig=gears_rig_tracking;
constexpr std::uint32_t version=1;
struct NativeSessionKey {
    std::uint32_t game_pid{},game_thread{};
    std::uint64_t game_start{},native_started_qpc{};
};
inline NativeSessionKey session_key(rig::NativeKey const& key) noexcept {
    return {key.game_pid,key.game_thread,key.game_start,key.native_started_qpc};
}
inline bool same(NativeSessionKey const& a,NativeSessionKey const& b) noexcept {
    return a.game_pid==b.game_pid&&a.game_thread==b.game_thread&&a.game_start==b.game_start&&a.native_started_qpc==b.native_started_qpc;
}
// Independent typed storage. In particular this contains neither a public
// V1 Frame nor a fake external registration cookie. All timestamps/epochs name
// the original Scene/source, never an invented meshTick or local observation.
struct CoreSnapshot {
    std::uint32_t size{sizeof(CoreSnapshot)},abi_version{version};
    rig::Status status{rig::Status::unavailable};std::uint32_t flags{};
    NativeSessionKey native{};
    std::uint64_t publication_sequence{},scene_invocation{},world_generation{},binding_generation{};
    std::uint64_t reference_generation{},published_ns{};
    std::int64_t scene_qpc{},qpc_frequency{};
    rig::ObservedContinuity observed{};rig::ProducerContinuity producer{};rig::NativeReference native_reference{};
    std::uint32_t controller_publisher_pid{},view_policy{};
    std::uint64_t controller_publisher_start{};
    gears_weapon_hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{},cover_link{};
    std::int32_t cover_slot{-1};std::uint32_t cover_type{};
    double units_per_metre{};
    gears_vr::StereoMatrix source_view{},base_view{};
    gears_vr::Pose reference{};
    gears_vr::StereoSample stereo{};
    gears_vr::ControllerInputFrame controllers{};
    std::uint32_t world_pose_mask{},reserved{};
    gears_vr::Pose world_head{},world_grip[2]{};
    // Internal metadata shape only. The Native caller must separately compare
    // these to its CURRENT selected-input ticket and reference mirror revision.
    std::uint64_t source_ticket{},native_reference_revision{};
};
inline bool core_snapshot_valid(CoreSnapshot const& core,std::int64_t now,std::uint64_t max_age,
    bool allow_sim=false) noexcept {
    return core.size==sizeof(core)&&core.abi_version==version&&core.source_ticket&&
        core.native_reference_revision&&!(core.native_reference_revision&1)&&rig::detail::payload_valid(core,now,max_age,allow_sim);
}
// PRECONDITION: the caller separately authenticated the actual current broker
// NativeKey/receipt. This pure projection can check only its nonzero cookie and
// exact NativeSessionKey equality, not whether that cookie is currently owned.
// It creates no permission, sequence, epoch, timestamp or source sample. The
// original public V1 validator still guards the projected value. Failure leaves
// out byte-for-byte unchanged. Core metadata remains for Native-only checking.
inline bool project_for_authenticated_registration(CoreSnapshot const& core,rig::NativeKey const& authenticated,
    std::int64_t now,std::uint64_t max_age,rig::Frame& out,bool allow_sim=false) noexcept {
    if(!authenticated.registration_cookie||!same(core.native,session_key(authenticated))||
       !core_snapshot_valid(core,now,max_age,allow_sim))return false;
    rig::Frame candidate{};rig::detail::copy_payload(core,candidate);candidate.native=authenticated;
    if(!rig::frame_valid(candidate,now,max_age,allow_sim))return false;
    std::memcpy(&out,&candidate,sizeof(out));return true;
}
static_assert(sizeof(NativeSessionKey)==24&&alignof(NativeSessionKey)==8);
static_assert(std::is_standard_layout_v<CoreSnapshot>&&std::is_trivially_copyable_v<CoreSnapshot>);
static_assert(std::is_standard_layout_v<NativeSessionKey>&&std::is_trivially_copyable_v<NativeSessionKey>);
} // namespace gears_native_rig_tracking
