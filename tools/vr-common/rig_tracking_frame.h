#pragma once
#include "weapon_hand_participant.h"
#include "rig_tracking_math.h"
#include <limits>

// Supplementary prepared-render data only. Frozen hand/call/action V1 layouts
// are unchanged. Reading this value grants no engine call, setter, bone lease,
// restore stage or ownership of a pose workspace. It is not a GHW1 certificate.
namespace gears_rig_tracking {
constexpr std::uint32_t version=2; // Exact GVC4 controller payload.
constexpr std::uint64_t maximum_age_ns=100000000;
enum class Status:std::uint32_t {unavailable,ready,stopped};
enum class Reply:std::uint32_t {accepted,unavailable,invalid,wrong_cookie,stopping};
enum Flags:std::uint32_t {controllers_matched=1,producer_continuity_available=2,cover_key_known=4};
enum ViewPolicy:std::uint32_t {first_person=1,cover=2,run_stabilized=4,camera_closer=8};
enum WorldPoseMask:std::uint32_t {head_pose=1,left_grip_pose=2,right_grip_pose=4};
struct NativeKey {
    std::uint32_t game_pid{},game_thread{};
    std::uint64_t game_start{},native_started_qpc{},registration_cookie{};
};
struct ObservedContinuity {
    // Native-observed nonwrapping boundaries. They survive missed Native data
    // reads, but cannot prove that Native observed every XR loss/reset frame.
    std::uint64_t stream{},reference{},head_tracking{},hand_tracking[2]{};
};
struct ProducerContinuity {
    // Zero/unavailable unless an actual matching XR continuity companion was
    // read. Never fill these from reference_generation or an observed counter.
    std::uint64_t stream{},tracking[2]{};
};
enum ReferenceFlags:std::uint32_t {reference_stable=1,reference_pending=2,reference_exhausted=4};
struct NativeReference {
    // Actual snapshot from this Native StereoGate's independent continuity
    // counter. This is neither reference_generation nor an XR producer epoch.
    std::uint64_t epoch{};std::uint32_t flags{},reserved{};
};
struct Frame {
    std::uint32_t size{sizeof(Frame)},abi_version{version};
    Status status{Status::unavailable};std::uint32_t flags{};
    NativeKey native{};
    std::uint64_t publication_sequence{},scene_invocation{},world_generation{},binding_generation{};
    std::uint64_t reference_generation{},published_ns{};
    std::int64_t scene_qpc{},qpc_frequency{};
    ObservedContinuity observed{};ProducerContinuity producer{};NativeReference native_reference{};
    std::uint32_t controller_publisher_pid{},view_policy{};
    std::uint64_t controller_publisher_start{};
    gears_weapon_hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{},cover_link{};
    std::int32_t cover_slot{-1};std::uint32_t cover_type{};
    double units_per_metre{};
    // source_view is the copied native camera. base_view is the final adjusted
    // base used to prepare this stereo frame, BEFORE HMD/eye composition.
    gears_vr::StereoMatrix source_view{},base_view{};
    gears_vr::Pose reference{};
    gears_vr::StereoSample stereo{};
    gears_vr::ControllerInputFrame controllers{}; // One selected frame, not a second bridge read.
    // Convenience transforms from precisely the raw data and base above. Grip
    // orientation uses UE X-forward/Y-right/Z-up; hand-bone calibration is extra.
    std::uint32_t world_pose_mask{},reserved{};
    gears_vr::Pose world_head{},world_grip[2]{};
};
struct ReadRequest {
    std::uint32_t size{sizeof(ReadRequest)},abi_version{version};
    std::uint64_t registration_cookie{},native_started_qpc{},minimum_publication_sequence{};
    std::uint64_t maximum_source_age_ns{50000000};
};
inline bool request_valid(ReadRequest const& r) noexcept {
    return r.size==sizeof(r)&&r.abi_version==version&&r.registration_cookie&&r.native_started_qpc&&
        r.maximum_source_age_ns&&r.maximum_source_age_ns<=maximum_age_ns;
}
inline bool same(NativeKey const& a,NativeKey const& b) noexcept {
    return a.game_pid==b.game_pid&&a.game_thread==b.game_thread&&a.game_start==b.game_start&&
        a.native_started_qpc==b.native_started_qpc&&a.registration_cookie==b.registration_cookie;
}
inline bool observed_valid(ObservedContinuity const& c) noexcept {
    return c.stream&&c.reference&&c.head_tracking&&c.hand_tracking[0]&&c.hand_tracking[1];
}
inline bool producer_empty(ProducerContinuity const& c) noexcept {
    return !c.stream&&!c.tracking[0]&&!c.tracking[1];
}
namespace detail {
// One data policy for public V1 and the distinct Native-owned snapshot type.
// Size/version and external registration authentication belong to the wrapper.
template<class Payload>
inline bool payload_valid(Payload const& f,std::int64_t now,std::uint64_t age,bool allow_sim) noexcept {
    using namespace gears_vr;
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
       !stereo_tracking_valid(f.stereo,allow_sim)||!f.stereo.space_epoch||!age||age>maximum_age_ns||
       now<=0||f.published_ns>std::uint64_t(now)||std::uint64_t(now)-f.published_ns>age||
       f.stereo.received_ns<=0||std::uint64_t(f.stereo.received_ns)>f.published_ns||
       now<f.stereo.received_ns||std::uint64_t(now-f.stereo.received_ns)>age)return false;
    if(f.flags&producer_continuity_available){if(!(f.flags&controllers_matched)||!f.producer.stream||!f.producer.tracking[0]||!f.producer.tracking[1])return false;}
    else if(!producer_empty(f.producer))return false;
    if(f.flags&cover_key_known){if(!f.cover_type||!gears_weapon_hand::identity_valid(f.cover_link)||f.cover_slot<0)return false;}
    else if(f.cover_type||f.cover_slot!=-1||!gears_weapon_hand::identity_valid(f.cover_link,true)||f.cover_link.address)return false;
    if((f.world_pose_mask&~(head_pose|left_grip_pose|right_grip_pose))||!(f.world_pose_mask&head_pose)||!stereo_pose_valid(f.world_head))return false;
    if(f.flags&controllers_matched){
        if(!f.controller_publisher_pid||!f.controller_publisher_start||!controller_frame_matches(f.controllers,f.stereo,now,allow_sim)||
           f.controllers.received_ns<=0||f.controllers.sync_ns<=0||std::uint64_t(f.controllers.received_ns)>f.published_ns||
           std::uint64_t(f.controllers.sync_ns)>f.published_ns)return false;
    }else if((f.world_pose_mask&(left_grip_pose|right_grip_pose))||f.controller_publisher_pid||f.controller_publisher_start)return false;
    for(unsigned i=0;i<2;++i)if(f.world_pose_mask&(left_grip_pose<<i)){
        const auto& h=f.controllers.hands[i];
        if(!(f.flags&controllers_matched)||!controller_pose_usable(h.grip,h.grip_flags,h.grip_active)||!stereo_pose_valid(f.world_grip[i]))return false;
    }
    return true;
}
// Copy only shared data fields, including nested object representations. Never
// copy a type's size/version, a registration cookie or internal latch metadata.
// This is a pure projection helper, not a validator or an authorization step.
template<class From,class To>
inline void copy_payload(From const& from,To& to) noexcept {
    const auto member=[](auto const& source,auto& destination) noexcept {
        using Source=std::remove_cv_t<std::remove_reference_t<decltype(source)>>;
        using Destination=std::remove_reference_t<decltype(destination)>;
        static_assert(std::is_same_v<Source,Destination>&&std::is_trivially_copyable_v<Source>);
        std::memcpy(&destination,&source,sizeof(destination));
    };
    member(from.status,to.status);member(from.flags,to.flags);
    member(from.native.game_pid,to.native.game_pid);member(from.native.game_thread,to.native.game_thread);
    member(from.native.game_start,to.native.game_start);member(from.native.native_started_qpc,to.native.native_started_qpc);
    member(from.publication_sequence,to.publication_sequence);member(from.scene_invocation,to.scene_invocation);
    member(from.world_generation,to.world_generation);member(from.binding_generation,to.binding_generation);
    member(from.reference_generation,to.reference_generation);member(from.published_ns,to.published_ns);
    member(from.scene_qpc,to.scene_qpc);member(from.qpc_frequency,to.qpc_frequency);
    member(from.observed,to.observed);member(from.producer,to.producer);member(from.native_reference,to.native_reference);
    member(from.controller_publisher_pid,to.controller_publisher_pid);member(from.view_policy,to.view_policy);
    member(from.controller_publisher_start,to.controller_publisher_start);
    member(from.world,to.world);member(from.controller,to.controller);member(from.pawn,to.pawn);member(from.mesh,to.mesh);
    member(from.weapon,to.weapon);member(from.cover_link,to.cover_link);member(from.cover_slot,to.cover_slot);member(from.cover_type,to.cover_type);
    member(from.units_per_metre,to.units_per_metre);member(from.source_view,to.source_view);member(from.base_view,to.base_view);
    member(from.reference,to.reference);member(from.stereo,to.stereo);member(from.controllers,to.controllers);
    member(from.world_pose_mask,to.world_pose_mask);member(from.reserved,to.reserved);
    member(from.world_head,to.world_head);member(from.world_grip,to.world_grip);
}
} // namespace detail
inline bool frame_valid(Frame const& f,std::int64_t now,std::uint64_t age,bool allow_sim=false) noexcept {
    return f.size==sizeof(f)&&f.abi_version==version&&f.native.registration_cookie&&detail::payload_valid(f,now,age,allow_sim);
}
#if defined(_WIN32)
using Read=Reply(__cdecl*)(ReadRequest const*,Frame*) noexcept;
#else
using Read=Reply(*)(ReadRequest const*,Frame*) noexcept;
#endif
inline constexpr char read_export[]="GearsVrReadRigTrackingFrameV1";
static_assert(sizeof(NativeKey)==32&&sizeof(ObservedContinuity)==40&&sizeof(ProducerContinuity)==24&&sizeof(NativeReference)==16&&sizeof(ReadRequest)==40);
static_assert(std::is_standard_layout_v<Frame>&&std::is_trivially_copyable_v<Frame>);
static_assert(std::is_standard_layout_v<ReadRequest>&&std::is_trivially_copyable_v<ReadRequest>);
} // namespace gears_rig_tracking
