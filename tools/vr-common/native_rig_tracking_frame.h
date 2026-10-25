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
// Native-only diagnostic shape policy: detail::payload_valid with only
// elapsed-age ceilings removed. Invalid/future clocks and all payload/cohort
// checks remain hard. No source field or timestamp is changed. Validating the
// controller cohort at its own received time checks shape, never freshness.
inline bool core_snapshot_non_age_valid(CoreSnapshot const& f,std::int64_t now,std::uint64_t age,
    bool allow_sim=false) noexcept {
    if(f.size!=sizeof(f)||f.abi_version!=version||!f.source_ticket||
       !f.native_reference_revision||(f.native_reference_revision&1))return false;
    using namespace gears_vr;
    using namespace gears_rig_tracking;
    if(f.status!=Status::ready||
       (f.flags&~(controllers_matched|producer_continuity_available|cover_key_known))||
       (f.view_policy&~(first_person|cover|run_stabilized|camera_closer))||f.reserved||
       !f.native.game_pid||!f.native.game_thread||!f.native.game_start||!f.native.native_started_qpc||
       !f.publication_sequence||!f.scene_invocation||!f.world_generation||!f.binding_generation||!f.reference_generation||
       !f.published_ns||f.scene_qpc<=0||f.qpc_frequency<=0||!observed_valid(f.observed)||
       !f.native_reference.epoch||f.native_reference.flags!=reference_stable||f.native_reference.reserved||
       !gears_weapon_hand::identity_valid(f.world)||!gears_weapon_hand::identity_valid(f.controller)||
       !gears_weapon_hand::identity_valid(f.pawn)||!gears_weapon_hand::identity_valid(f.mesh)||
       !gears_weapon_hand::identity_valid(f.weapon,true)||!stereo_rigid(f.source_view)||!stereo_rigid(f.base_view)||
       !stereo_pose_valid(f.reference)||!std::isfinite(f.units_per_metre)||f.units_per_metre<10||f.units_per_metre>500||
       !stereo_tracking_valid(f.stereo,allow_sim)||!f.stereo.space_epoch||!age||age>rig::maximum_age_ns||
       now<=0||f.published_ns>std::uint64_t(now)||
       f.stereo.received_ns<=0||std::uint64_t(f.stereo.received_ns)>f.published_ns||
       now<f.stereo.received_ns)return false;
    if(f.flags&producer_continuity_available){if(!(f.flags&controllers_matched)||!f.producer.stream||!f.producer.tracking[0]||!f.producer.tracking[1])return false;}
    else if(!producer_empty(f.producer))return false;
    if(f.flags&cover_key_known){if(!f.cover_type||!gears_weapon_hand::identity_valid(f.cover_link)||f.cover_slot<0)return false;}
    else if(f.cover_type||f.cover_slot!=-1||!gears_weapon_hand::identity_valid(f.cover_link,true)||f.cover_link.address)return false;
    if((f.world_pose_mask&~(head_pose|left_grip_pose|right_grip_pose))||!(f.world_pose_mask&head_pose)||!stereo_pose_valid(f.world_head))return false;
    if(f.flags&controllers_matched){
        if(!f.controller_publisher_pid||!f.controller_publisher_start||(!controller_frame_matches(f.controllers,f.stereo,f.controllers.received_ns,allow_sim)||now<f.controllers.received_ns)||
           f.controllers.received_ns<=0||f.controllers.sync_ns<=0||std::uint64_t(f.controllers.received_ns)>f.published_ns||
           std::uint64_t(f.controllers.sync_ns)>f.published_ns)return false;
    }else if((f.world_pose_mask&(left_grip_pose|right_grip_pose))||f.controller_publisher_pid||f.controller_publisher_start)return false;
    for(unsigned i=0;i<2;++i)if(f.world_pose_mask&(left_grip_pose<<i)){
        const auto& h=f.controllers.hands[i];
        if(!(f.flags&controllers_matched)||!controller_pose_usable(h.grip,h.grip_flags,h.grip_active)||!stereo_pose_valid(f.world_grip[i]))return false;
    }
    return true;
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
