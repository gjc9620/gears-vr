#pragma once
#include "controller_input_frame.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

// Synchronous Native-GT / weapon participant contract. This header creates no
// arbiter, mapping, hook or writer. Native owns the one left-hand arbiter.
namespace gears_weapon_hand {
constexpr std::uint32_t version=1;
enum class LeaseKind:std::uint32_t {none,support,ammo,magazine,cover,other};
enum class DecisionStatus:std::uint32_t {released,retained,granted,needs_release,no_candidate,
    incumbent_conflict,candidate_conflict,inactive,lifecycle_changed,invalid,replayed,ambiguous,lease_lost};
enum class Reply:std::uint32_t {accepted,unavailable,invalid,busy,draining,quiescent,wrong_cookie,reentrant};
enum class InvalidateReason:std::uint32_t {context_lost,input_lost,native_stopping,unregistering,callback_fault};
enum FrameFlags:std::uint32_t {scene_owned=1,input_matched=2,gameplay_interactable=4,allow_sim=8};
struct ObjectIdentity {
    std::uint64_t address{},outer{},klass{};
    std::uint32_t registry_index{},name_index{},name_number{},known{};
};
inline bool same(ObjectIdentity const& a,ObjectIdentity const& b) noexcept {
    return a.address==b.address&&a.outer==b.outer&&a.klass==b.klass&&
        a.registry_index==b.registry_index&&a.name_index==b.name_index&&a.name_number==b.name_number&&a.known==b.known;
}
inline bool identity_valid(ObjectIdentity const& a,bool allow_empty=false) noexcept {
    if(!a.address)return allow_empty&&same(a,ObjectIdentity{});
    return a.known==1&&!(a.address&7)&&a.klass&&!(a.klass&7)&&!(a.outer&7);
}
struct FrameKey {
    std::uint64_t game_start{},native_started_qpc{},registration_cookie{},lifecycle{},frame{},world_generation{};
    std::uint32_t game_pid{},thread_id{};
};
inline bool same(FrameKey const& a,FrameKey const& b) noexcept {
    return a.game_start==b.game_start&&a.native_started_qpc==b.native_started_qpc&&
        a.registration_cookie==b.registration_cookie&&a.lifecycle==b.lifecycle&&a.frame==b.frame&&
        a.world_generation==b.world_generation&&a.game_pid==b.game_pid&&a.thread_id==b.thread_id;
}
struct Frame {
    std::uint32_t size{sizeof(Frame)},abi_version{version},flags{},reserved{};
    FrameKey key{};
    std::int64_t now_ns{};
    ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};
    gears_vr::ControllerInputFrame controllers{};
    gears_vr::StereoSample stereo{};
    gears_vr::Pose reference{};
    std::uint64_t reference_generation{};
    std::uint32_t stereo_status{},recentered{},heading_deferred{},reserved2{};
    // Decoded ONCE by Native's shared logical input adapter before prepare.
    // Token belongs to this owner lifecycle, not an epoch-reset XR counter.
    std::uint64_t press_token{};
    std::uint32_t left_grip_down{},left_grip_known{},left_tracked{},reserved3{};
};
struct Lease {
    LeaseKind kind{};std::uint32_t reserved{};
    std::uint64_t lifecycle{},id{};
};
inline bool same(Lease const& a,Lease const& b) noexcept {
    return a.kind==b.kind&&a.reserved==b.reserved&&a.lifecycle==b.lifecycle&&a.id==b.id;
}
inline bool lease_valid(Lease const& lease,std::uint64_t lifecycle) noexcept {
    if(lease.reserved||unsigned(lease.kind)>unsigned(LeaseKind::other))return false;
    return lease.kind==LeaseKind::none?!lease.lifecycle&&!lease.id:lease.lifecycle==lifecycle&&lease.id;
}
struct Proposal {
    std::uint32_t size{sizeof(Proposal)},abi_version{version};
    FrameKey key{};std::uint64_t nonce{};
    std::array<Lease,2> incumbents{};
    // Only currently certified weapon candidates. Cover requests come from
    // Native's own surface/reach participant, never from these callbacks.
    std::uint32_t support{},ammo{},magazine{},reserved{};
};
struct Decision {
    std::uint32_t size{sizeof(Decision)},abi_version{version};
    FrameKey key{};std::uint64_t proposal_nonce{},press_token{};
    std::int64_t now_ns{};
    Lease lease{};
    DecisionStatus status{DecisionStatus::invalid};
    std::uint32_t new_grant{},release_required{1},reserved{};
};
struct Invalidation {
    std::uint32_t size{sizeof(Invalidation)},abi_version{version};
    std::uint64_t registration_cookie{},native_started_qpc{};
    InvalidateReason reason{};std::uint32_t reserved{};
};
inline bool frame_shape_valid(Frame const& f) noexcept {
    return f.size==sizeof(Frame)&&f.abi_version==version&&!(f.flags&~15u)&&!f.reserved&&!f.reserved2&&!f.reserved3&&
        f.key.game_start&&f.key.native_started_qpc&&f.key.registration_cookie&&f.key.lifecycle&&f.key.frame&&
        f.key.world_generation&&f.key.game_pid&&f.key.thread_id&&f.now_ns>0&&
        identity_valid(f.world)&&identity_valid(f.controller)&&identity_valid(f.pawn)&&identity_valid(f.mesh)&&
        identity_valid(f.weapon,true)&&f.reference_generation&&f.recentered<=1&&f.heading_deferred<=1&&
        gears_vr::stereo_pose_valid(f.reference)&&f.left_grip_down<=1&&f.left_grip_known<=1&&f.left_tracked<=1&&
        (!f.left_grip_down||(f.press_token&&f.left_grip_known&&f.left_tracked));
}
inline bool frame_active(Frame const& f,std::int64_t now_ns) noexcept {
    constexpr std::uint32_t required=scene_owned|input_matched|gameplay_interactable;
    return frame_shape_valid(f)&&(f.flags&required)==required&&now_ns>=f.now_ns&&
        (f.stereo_status==unsigned(gears_vr::StereoStatus::accepted)||f.stereo_status==unsigned(gears_vr::StereoStatus::initial))&&
        gears_vr::stereo_render_active(f.stereo)&&gears_vr::stereo_tracking_valid(f.stereo,(f.flags&allow_sim)!=0)&&
        gears_vr::stereo_geometry_valid(f.stereo)&&f.stereo.received_ns>0&&f.stereo.received_ns<=now_ns&&
        now_ns-f.stereo.received_ns<=100000000&&
        gears_vr::controller_frame_matches(f.controllers,f.stereo,now_ns,(f.flags&allow_sim)!=0);
}

#if defined(_WIN32)
#define GEARS_WEAPON_HAND_CALL __cdecl
#else
#define GEARS_WEAPON_HAND_CALL
#endif
using Prepare=std::uint32_t(GEARS_WEAPON_HAND_CALL*)(void*,Frame const*,Proposal*) noexcept;
using Commit=std::uint32_t(GEARS_WEAPON_HAND_CALL*)(void*,Frame const*,Proposal const*,Decision const*) noexcept;
// May run outside GT: this callback can only atomically disarm publication.
// It must not touch game/GT-owned state, call the engine or wait for callbacks.
using Invalidate=void(GEARS_WEAPON_HAND_CALL*)(void*,Invalidation const*) noexcept;
struct Registration {
    std::uint32_t size{sizeof(Registration)},abi_version{version};
    std::uint32_t frame_bytes{sizeof(Frame)},proposal_bytes{sizeof(Proposal)},decision_bytes{sizeof(Decision)},reserved{};
    void* context{};Prepare prepare{};Commit commit{};Invalidate invalidate{};
};
struct RegistrationResult {
    std::uint32_t size{sizeof(RegistrationResult)},abi_version{version};
    std::uint64_t cookie{},native_started_qpc{};
};
using Register=Reply(GEARS_WEAPON_HAND_CALL*)(Registration const*,RegistrationResult*) noexcept;
using Unregister=Reply(GEARS_WEAPON_HAND_CALL*)(std::uint64_t) noexcept;
inline constexpr char register_export[]="GearsVrRegisterWeaponHandParticipantV1";
inline constexpr char unregister_export[]="GearsVrUnregisterWeaponHandParticipantV1";
#undef GEARS_WEAPON_HAND_CALL

// Native pins the SAME module containing all three callback code addresses
// before publishing the registration. One registration/cookie at a time.
// It invokes prepare once on a certified Scene GT frame, copies Proposal,
// updates its sole arbiter once, then commit with that identical frozen Frame
// and Proposal plus the decision. No engine calls/reentry from either callback;
// Native may hold stereo_mutex. Invalid/rejected proposals grant nothing.
// Rendering retries consume the already committed result, never re-arbitrate.
// A registered participant's failed prepare/commit is NOT an empty free hand:
// invalidate owner context, release only owned resources and require a valid
// release before any later grant. Registration changes advance the owner
// lifecycle (or explicitly invalidate it); no lease crosses a callback cookie.
// Run the broker for all eligible Scene GT frames, including outside cover.
//
// Unregister first stops admission and invalidates the participant. It returns
// draining until the exact cookie has no in-flight callbacks; quiescent alone
// permits module-pin release. No blocking wait while inside a callback/GT or
// holding stereo_mutex. Cookie mismatch never disables a later registration.
// PG keeps context/storage alive until quiescent; Native stop follows the same
// drain protocol. A timeout is still draining, not permission to free memory.
static_assert(sizeof(ObjectIdentity)==40&&sizeof(FrameKey)==56&&sizeof(Lease)==24);
static_assert(sizeof(void*)==8&&sizeof(gears_vr::ControllerHand)==304&&sizeof(gears_vr::ControllerInputFrame)==688);
static_assert(sizeof(gears_vr::StereoSample)==312&&sizeof(Frame)==1384&&alignof(Frame)==8);
static_assert(offsetof(Frame,key)==16&&offsetof(Frame,now_ns)==72&&offsetof(Frame,world)==80);
static_assert(offsetof(Frame,controllers)==280&&offsetof(Frame,stereo)==968&&offsetof(Frame,reference)==1280);
static_assert(offsetof(Frame,press_token)==1360&&offsetof(Frame,left_grip_down)==1368);
static_assert(sizeof(Proposal)==136&&sizeof(Decision)==128&&sizeof(Invalidation)==32);
static_assert(sizeof(Registration)==56&&sizeof(RegistrationResult)==24&&offsetof(Registration,prepare)==32);
static_assert(std::is_standard_layout_v<Frame>&&std::is_trivially_copyable_v<Frame>);
static_assert(std::is_standard_layout_v<Proposal>&&std::is_trivially_copyable_v<Proposal>);
static_assert(std::is_standard_layout_v<Decision>&&std::is_trivially_copyable_v<Decision>);
static_assert(std::is_standard_layout_v<Registration>&&std::is_trivially_copyable_v<Registration>);
} // namespace gears_weapon_hand
