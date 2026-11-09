#pragma once
#include "cover_interaction.h"
#include "stereo_frame.h"
#include "hand_authored_mount_profile.h"
#include <array>

// Pure cover-point skin planning for the existing C0 hand solver. The caller
// authenticates actual geometry, object identities, input and logical ownership.
// Keys/poses here neither certify those sources nor grant a lease, body motion,
// collision, skeleton write or mechanical action. No weapon profile is selected.
namespace gears_cover_grab_pose {
namespace vr=gears_vr;namespace mount=gears_hand_authored_mount;
enum class Mode {none,approach,held,release};
enum class Code {prepared,inactive,conflict,stale,binding,profile,geometry,distant,orientation};
struct Key {
    vr::CoverEpochs epochs{};
    std::uint64_t edge{},component{},shape_generation{},profile{};
};
inline bool same(vr::CoverEpochs const& a,vr::CoverEpochs const& b) noexcept {
    return a.world==b.world&&a.pawn==b.pawn&&a.cover==b.cover&&a.slot==b.slot&&
        a.session==b.session&&a.space==b.space&&a.reference==b.reference;
}
inline bool valid(Key const& k) noexcept {
    auto const& e=k.epochs;
    return e.world&&e.pawn&&e.cover&&e.slot&&e.session&&e.space&&e.reference&&
        k.edge&&k.component&&k.shape_generation&&k.profile;
}
inline bool same(Key const& a,Key const& b) noexcept {
    return same(a.epochs,b.epochs)&&a.edge==b.edge&&a.component==b.component&&
        a.shape_generation==b.shape_generation&&a.profile==b.profile;
}
struct Profile {
    std::uint64_t id{};
    vr::CoverEdgeKind kind{vr::CoverEdgeKind::none};
    mount::Signature geometry_signature{};
    // Original rig model units, not controller metres. Actual edge-local
    // wrist placement and all local skin rotations are authored per grab point.
    vr::Pose wrist_in_edge{};
    std::array<vr::Quaternion,15> approach{},held{};
    double acquire_metres{},retain_metres{},maximum_wrist_turn{};
};
struct Contact {
    Key key{};
    vr::Pose edge_world{}; // Current surface frame in the shared UE world basis.
    std::int64_t observed_ns{};
};
struct Frame {
    Key expected{};
    vr::CoverObservation cover{};
    vr::CoverHandSample hand{};
    vr::CoverResult interaction{};
    vr::CoverOwnership ownership{};
    vr::Pose physical_grip_world{}; // Raw-input conversion, before skin fitting.
    mount::Signature actual_geometry_signature{};
    double model_scale{},units_per_metre{};
    std::int64_t now_ns{};
    Mode mode{Mode::none};
};
struct Plan {
    Key key{};Mode mode{Mode::none};
    vr::Pose wrist_world{};
    std::array<vr::Quaternion,15> joints{};
    bool active{}; // Prepared cosmetic data only; not applied or accepted skin.
};
inline bool profile_valid(Profile const& p) noexcept {
    constexpr double pi=3.14159265358979323846;
    if(!p.id||(p.kind!=vr::CoverEdgeKind::left&&p.kind!=vr::CoverEdgeKind::right&&p.kind!=vr::CoverEdgeKind::top)||
       p.geometry_signature!=mount::marcus115_signature||!vr::stereo_pose_valid(p.wrist_in_edge)||
       !std::isfinite(p.acquire_metres)||!std::isfinite(p.retain_metres)||
       p.acquire_metres<=0||p.acquire_metres>p.retain_metres||p.retain_metres>.25||
       !std::isfinite(p.maximum_wrist_turn)||p.maximum_wrist_turn<=0||p.maximum_wrist_turn>pi)return false;
    for(auto const& poses:{&p.approach,&p.held})for(auto q:*poses)
        if(!vr::stereo_pose_valid({{},q}))return false;
    return true;
}
inline bool fresh(std::int64_t stamp,std::int64_t now) noexcept {
    return now>0&&stamp>0&&stamp<=now&&now-stamp<=100000000;
}
inline Code prepare(Frame const& f,Contact const& contact,Profile const& profile,Plan& out) noexcept {
    // Rejection/release clears the old proposal. The existing C0 source owns
    // the fresh physical wrist and its release blend; this planner retains no
    // old world target, hand sample, lease or synthetic release/press.
    out={};
    if(f.mode==Mode::none||f.mode==Mode::release)return Code::inactive;
    if(f.mode!=Mode::approach&&f.mode!=Mode::held)return Code::profile;
    if(!valid(f.expected)||!same(f.expected,contact.key)||!same(f.expected.epochs,f.cover.epochs)||
       f.hand.session_epoch!=f.expected.epochs.session||f.hand.space_epoch!=f.expected.epochs.space||
       f.cover.edge.id!=f.expected.edge)return Code::binding;
    if(!fresh(contact.observed_ns,f.now_ns)||
       !fresh(f.cover.observed_ns,f.now_ns)||!fresh(f.hand.observed_ns,f.now_ns))return Code::stale;
    if(!f.cover.sequence||!f.hand.sequence||!f.cover.cover_valid||!f.cover.interaction_enabled||
       !f.cover.edge.allowed||!f.hand.input_active||!f.hand.position_valid||!f.hand.position_tracked||
       !f.interaction.offset_certified)return Code::inactive;
    if(!profile_valid(profile)||f.expected.profile!=profile.id||f.cover.edge.kind!=profile.kind)return Code::profile;
    if(f.ownership.owner==vr::CoverLeftHandOwner::other)return Code::conflict;
    if(f.mode==Mode::held){
        if(!f.hand.lgrip_down)return Code::inactive;
        if(f.ownership.owner!=vr::CoverLeftHandOwner::cover||!f.interaction.owns_left_hand||
           f.interaction.phase!=vr::CoverPhase::grabbing||f.interaction.edge_id!=f.expected.edge||
           !f.ownership.request_id||f.ownership.request_id!=f.interaction.request_id)return Code::conflict;
    }else if(f.hand.lgrip_down||f.ownership.owner!=vr::CoverLeftHandOwner::free||
              f.interaction.owns_left_hand||f.interaction.request_left_hand||
              (f.interaction.phase!=vr::CoverPhase::attached&&f.interaction.phase!=vr::CoverPhase::held)||
              !f.cover.edge.within_acquisition_reach)return Code::conflict;
    if(!vr::finite(f.hand.left_position_metres)||!vr::stereo_pose_valid(contact.edge_world)||
       !vr::stereo_pose_valid(f.physical_grip_world))return Code::geometry;
    mount::Mount calibration{};
    if(f.actual_geometry_signature!=profile.geometry_signature||
       !mount::marcus115(f.actual_geometry_signature,mount::Side::left,f.model_scale,f.units_per_metre,calibration))return Code::profile;
    vr::Pose physical{};
    if(gears_hand_wrist::native_wrist_target(f.physical_grip_world,calibration.calibration,
        f.units_per_metre,physical)!=gears_hand_wrist::Status::okay)return Code::geometry;
    const auto local=profile.wrist_in_edge.position;
    const auto offset=vr::transform(vr::rotation_matrix(contact.edge_world.orientation),
        {local.x*f.model_scale,local.y*f.model_scale,local.z*f.model_scale});
    const vr::Pose target{{contact.edge_world.position.x+offset.x,contact.edge_world.position.y+offset.y,
        contact.edge_world.position.z+offset.z},
        vr::normalized(vr::multiply(contact.edge_world.orientation,profile.wrist_in_edge.orientation))};
    if(!vr::stereo_pose_valid(target))return Code::geometry;
    const double distance=std::hypot(std::hypot(target.position.x-physical.position.x,
        target.position.y-physical.position.y),target.position.z-physical.position.z)/f.units_per_metre;
    if(distance>(f.mode==Mode::held?profile.retain_metres:profile.acquire_metres))return Code::distant;
    const auto a=target.orientation,b=physical.orientation;
    const double dot=std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w);
    if(2*std::acos(std::clamp(dot,0.,1.))>profile.maximum_wrist_turn)return Code::orientation;
    out.key=contact.key;out.mode=f.mode;out.wrist_world=target;
    out.joints=f.mode==Mode::held?profile.held:profile.approach;out.active=true;
    return Code::prepared;
}
} // namespace gears_cover_grab_pose
