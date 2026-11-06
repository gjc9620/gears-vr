#pragma once
#include "rig_tracking_frame.h"
#include "hand_authored_mount_profile.h"
#include <type_traits>

// Separate data-only result of the shared C0 hand transaction. This is NOT a
// Rig V1 frame, a new tracking sample, a skeletal lease or an action permit.
namespace gears_committed_hand {
namespace hand=gears_weapon_hand;namespace rig=gears_rig_tracking;
inline constexpr unsigned version=3;
inline constexpr std::uint64_t maximum_age_ns=40000000;
// The observed variant is the ACTUAL fresh post-Compose palm, after original
// animation (including recoil). It can keep the gun attached visually but
// deliberately never satisfies matches() or an interaction/action grant.
enum class Purpose:unsigned {held_lancer,free_hand_visual,observed_held_visual};
struct Frame {
    unsigned size{sizeof(Frame)},abi_version{version};
    Purpose purpose{Purpose::held_lancer};unsigned reserved{};
    hand::FrameKey parent{};
    hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};
    std::uint64_t serial{},tick{},compose{},hold_generation{},binding_generation{},reference_generation{};
    rig::NativeReference native_reference{};
    rig::ProducerContinuity producer{};
    std::int64_t acquisition_begin_ns{},acquisition_end_ns{},committed_ns{},returned_ns{};
    gears_vr::StereoSample stereo{};
    gears_vr::ControllerInputFrame controllers{};
    gears_hand_authored_mount::Signature geometry{};
    double units_per_metre{};
    gears_vr::Pose right_grip{};
};
struct Request {unsigned size{sizeof(Request)},abi_version{version},response_bytes{sizeof(Frame)},reserved{};
    std::uint64_t cookie{},native_started{};};
inline bool request_valid(Request const& r) noexcept {
    return r.size==sizeof r&&r.abi_version==version&&r.response_bytes==sizeof(Frame)&&!r.reserved&&r.cookie&&r.native_started;
}
inline bool fresh(Frame const& f,std::int64_t now,bool allow_sim) noexcept {
    const auto young=[&](std::int64_t t){return t>0&&now>=t&&std::uint64_t(now-t)<=maximum_age_ns;};
    const bool held=f.purpose==Purpose::held_lancer||f.purpose==Purpose::observed_held_visual;
    const bool visual=f.purpose==Purpose::free_hand_visual;
    return f.size==sizeof f&&f.abi_version==version&&!f.reserved&&(held||visual)&&f.serial&&f.tick&&f.compose&&
        (held?(f.hold_generation&&f.parent.lifecycle):(!f.hold_generation&&!f.parent.lifecycle))&&
        f.binding_generation&&f.reference_generation&&f.parent.registration_cookie&&f.parent.native_started_qpc&&
        f.parent.game_pid&&f.parent.game_start&&f.parent.thread_id&&f.parent.world_generation&&f.parent.frame&&
        hand::identity_valid(f.world)&&hand::identity_valid(f.controller)&&hand::identity_valid(f.pawn)&&
        hand::identity_valid(f.mesh)&&hand::identity_valid(f.weapon)&&
        f.geometry==gears_hand_authored_mount::marcus115_signature&&
        f.native_reference.flags==rig::reference_stable&&f.native_reference.epoch&&!f.native_reference.reserved&&
        f.producer.stream&&f.producer.tracking[0]&&f.producer.tracking[1]&&
        std::isfinite(f.units_per_metre)&&f.units_per_metre>=10&&f.units_per_metre<=500&&
        f.acquisition_end_ns>=f.acquisition_begin_ns&&f.committed_ns>=f.acquisition_end_ns&&f.returned_ns>=f.committed_ns&&
        young(f.acquisition_begin_ns)&&young(f.acquisition_end_ns)&&young(f.committed_ns)&&young(f.returned_ns)&&
        young(f.stereo.received_ns)&&young(f.controllers.received_ns)&&young(f.controllers.sync_ns)&&
        gears_vr::stereo_render_active(f.stereo)&&f.stereo.focused==1&&
        gears_vr::controller_frame_matches(f.controllers,f.stereo,now,allow_sim)&&
        gears_vr::controller_pose_usable(f.controllers.hands[1].grip,f.controllers.hands[1].grip_flags,f.controllers.hands[1].grip_active)&&
        gears_vr::stereo_pose_valid(f.right_grip);
}
// A normally consumed C0 result is render data, not a new tracking ticket.
// Its acquisition must have passed40ms at publication. The existing weapon
// action window is100ms, with ORIGINAL times retained; Native additionally
// revokes the result at the next owning C0 begin (even if that C0 fails).
inline bool current_result(Frame const& f,std::int64_t now,bool allow_sim) noexcept {
    constexpr std::uint64_t action_age_ns=100000000;
    const auto within=[&](std::int64_t t){return t>0&&now>=t&&std::uint64_t(now-t)<=action_age_ns;};
    return fresh(f,f.returned_ns,allow_sim)&&within(f.acquisition_begin_ns)&&within(f.returned_ns)&&
        within(f.stereo.received_ns)&&within(f.controllers.received_ns)&&within(f.controllers.sync_ns);
}
// A later, independently double-read rendered wrist may replace a post-Compose
// observation. Preserve EVERY original source/commit/return timestamp: this is
// visual geometry, never another tracking publication or an action grant.
// A new visual observation still has to fit the original40ms publication age.
inline bool observe_visual(Frame const& prior,gears_vr::Pose const& palm,
    std::uint64_t serial,std::int64_t now,bool allow_sim,Frame& out) noexcept {
    if((prior.purpose!=Purpose::held_lancer&&prior.purpose!=Purpose::observed_held_visual)||
       serial<=prior.serial||!fresh(prior,now,allow_sim)||!gears_vr::stereo_pose_valid(palm))return false;
    auto candidate=prior;candidate.purpose=Purpose::observed_held_visual;
    candidate.serial=serial;candidate.right_grip=palm;out=candidate;return true;
}
inline bool matches(Frame const& f,hand::Frame const& action,rig::Frame const& tracking,
    std::uint64_t held,std::int64_t now,bool allow_sim) noexcept {
    return f.purpose==Purpose::held_lancer&&held&&current_result(f,now,allow_sim)&&f.hold_generation==held&&
        f.parent.registration_cookie==action.key.registration_cookie&&f.parent.native_started_qpc==action.key.native_started_qpc&&
        f.parent.game_pid==action.key.game_pid&&f.parent.game_start==action.key.game_start&&f.parent.thread_id==action.key.thread_id&&
        f.parent.world_generation==action.key.world_generation&&f.parent.lifecycle==action.key.lifecycle&&
        f.parent.frame<=action.key.frame&&f.parent.frame&&
        hand::same(f.world,action.world)&&hand::same(f.controller,action.controller)&&hand::same(f.pawn,action.pawn)&&
        hand::same(f.mesh,action.mesh)&&hand::same(f.weapon,action.weapon)&&
        f.binding_generation==tracking.binding_generation&&f.reference_generation==tracking.reference_generation&&
        f.native_reference.epoch==tracking.native_reference.epoch&&f.units_per_metre==tracking.units_per_metre&&
        f.producer.stream==tracking.producer.stream&&f.producer.tracking[0]==tracking.producer.tracking[0]&&
        f.producer.tracking[1]==tracking.producer.tracking[1]&&
        f.stereo.source==tracking.stereo.source&&f.stereo.session==tracking.stereo.session&&f.stereo.space_epoch==tracking.stereo.space_epoch&&
        f.controllers.input_epoch==tracking.controllers.input_epoch&&f.controllers.profile_generation==tracking.controllers.profile_generation;
}
#if defined(_WIN32)
using Read=rig::Reply(__cdecl*)(Request const*,Frame*) noexcept;
#else
using Read=rig::Reply(*)(Request const*,Frame*) noexcept;
#endif
inline constexpr char read_export[]="GearsVrReadCommittedHandEndpointV3";
static_assert(std::is_standard_layout_v<Request>&&std::is_trivially_copyable_v<Request>);
static_assert(std::is_standard_layout_v<Frame>&&std::is_trivially_copyable_v<Frame>);
}
