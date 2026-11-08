#pragma once
#include "cover_view.h"
#include <type_traits>

namespace gears_vr {
// Caller-authenticated binding. Nonzero addresses/epochs are shape checks,
// never substitutes for the Native object's complete current identity proof.
struct CoverBodyTargetKey {
    CoverEpochs epochs{};
    std::uintptr_t world{},pawn{},mesh{},link{};
    std::uint64_t scene{},xr_session{},xr_space{},reference{},sequence{};
    unsigned thread{};int slot=-1;
    std::uint64_t world_generation{};
};
inline bool same(CoverBodyTargetKey const& a,CoverBodyTargetKey const& b){
    return a.epochs.world==b.epochs.world&&a.epochs.pawn==b.epochs.pawn&&
        a.epochs.cover==b.epochs.cover&&a.epochs.slot==b.epochs.slot&&
        a.epochs.session==b.epochs.session&&a.epochs.space==b.epochs.space&&a.epochs.reference==b.epochs.reference&&
        a.world==b.world&&a.pawn==b.pawn&&a.mesh==b.mesh&&a.link==b.link&&a.slot==b.slot&&a.thread==b.thread&&
        a.scene==b.scene&&a.xr_session==b.xr_session&&a.xr_space==b.xr_space&&a.reference==b.reference&&a.sequence==b.sequence&&a.world_generation==b.world_generation;
}
inline bool cover_body_target_key_valid(CoverBodyTargetKey const& k){
    return k.world&&k.pawn&&k.mesh&&k.link&&k.world_generation&&k.slot>=0&&k.slot<256&&k.thread&&k.scene&&
        k.xr_session&&k.xr_space&&k.reference&&k.sequence&&k.epochs.world&&k.epochs.pawn&&
        k.epochs.cover&&k.epochs.slot&&k.epochs.session&&k.epochs.space&&k.epochs.reference;
}
struct CoverBodyTarget {
    CoverBodyTargetKey key{};
    CoverViewAnchor anchor{};
    CoverViewAnchor view_anchor{};
    CoverViewResult committed_view{}; // Pre-HMD base; virtual offset only.
    CoverOffset virtual_offset{};
    StereoResult tracking{};
    CoverPhysicalDisplacement physical{};
    Vector desired_eye_world{}; // Already contains current HMD translation once.
    double desired_right_metres{},desired_up_metres{},desired_forward_metres{},units_per_metre{};
    std::int64_t observed_ns{};
    // An eye goal is not a solved bone pose. The head-to-eye calibration,
    // engine evaluation phase and collision publication are separate proofs.
    bool head_bone_goal_valid{},pose_applied{},same_pose_epoch_verified{},collision_verified{};
};
static_assert(std::is_trivially_copyable_v<CoverBodyTarget>&&std::is_standard_layout_v<CoverBodyTarget>);

// Freeze the SAME committed camera and tracking sample for all body axes.
// No animation feedback, height saturation, triangle normalization or game
// writes. expected must be the caller's current binding, not a cached old key.
inline bool prepare_cover_body_target(CoverBodyTargetKey const& expected,CoverBodyTargetKey const& key,
    CoverViewAnchor const& anchor,CoverViewAnchor const& view_anchor,CoverViewResult const& committed,CoverOffset virtual_offset,
    StereoResult const& stereo,std::int64_t now_ns,double units_per_metre,CoverBodyTarget& out,bool allow_sim=false){
    if(!cover_body_target_key_valid(key)||!same(expected,key)||!stereo||stereo.heading_deferred||
       key.xr_session!=stereo.sample.session||key.xr_space!=stereo.sample.space_epoch||
       key.reference!=stereo.reference_generation||key.sequence!=stereo.sample.sequence||
       !stereo_tracking_valid(stereo.sample,allow_sim)||!stereo_render_active(stereo.sample)||!stereo_geometry_valid(stereo.sample)||
       now_ns<=0||stereo.sample.received_ns<=0||now_ns<stereo.sample.received_ns||
       now_ns-stereo.sample.received_ns>100000000)return false;
    CoverViewResult checked{};CoverBodyTarget candidate{};CoverPhysicalDisplacement wall_physical{};
    if(!prepare_cover_view(anchor,view_anchor,virtual_offset,units_per_metre,checked)||
       checked.base_eye_world.x!=committed.base_eye_world.x||checked.base_eye_world.y!=committed.base_eye_world.y||
       checked.base_eye_world.z!=committed.base_eye_world.z||checked.base_view!=committed.base_view||
       !cover_physical_displacement(stereo.reference,stereo.sample.head,candidate.physical)||
       !cover_physical_in_wall(anchor,view_anchor,candidate.physical,wall_physical)||
       !cover_tracking_point(view_anchor,committed,stereo.reference,stereo.sample.head.position,units_per_metre,candidate.desired_eye_world))return false;
    candidate.key=key;candidate.anchor=anchor;candidate.view_anchor=view_anchor;
    candidate.committed_view=committed;candidate.virtual_offset=virtual_offset;
    candidate.tracking=stereo;candidate.observed_ns=now_ns;candidate.units_per_metre=units_per_metre;
    candidate.desired_right_metres=virtual_offset.right+wall_physical.right;
    candidate.desired_up_metres=virtual_offset.up+wall_physical.up;
    candidate.desired_forward_metres=wall_physical.forward;
    if(!std::isfinite(candidate.desired_right_metres)||!std::isfinite(candidate.desired_up_metres)||
       !std::isfinite(candidate.desired_forward_metres))return false;
    out=candidate;return true;
}
inline bool cover_body_target_current(CoverBodyTarget const& target,CoverBodyTargetKey const& current,std::int64_t now_ns){
    return cover_body_target_key_valid(current)&&same(target.key,current)&&target.observed_ns>0&&
        now_ns>=target.observed_ns&&now_ns-target.observed_ns<=100000000&&target.tracking.sample.received_ns>0&&
        now_ns>=target.tracking.sample.received_ns&&now_ns-target.tracking.sample.received_ns<=100000000;
}
} // namespace gears_vr
