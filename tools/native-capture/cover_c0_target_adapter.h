#pragma once
#include "cover_neutral_goal.h"
#include "../vr-common/cover_body_target.h"

// Bridge the existing Scene cover target to an already fitted C0 neutral.
// Pure owned-value conversion. The caller still owns current placement,
// geometry/occupancy and the sole C0 transaction; no permission is minted here.
namespace gears_cover_c0_target {
namespace ng=gears_cover_neutral_goal;
namespace a=gears_body_goal_view;
namespace b=a::body;
namespace v=gears_vr;
enum class Code {prepared,unavailable,alias,source,key,tracking,placement,target,convert,view};
struct Prepared {
    a::Output body{};
    v::CoverBodyTargetKey key{};
    std::int64_t target_observed_ns{},tracking_received_ns{};
};
inline bool same(v::CoverPhysicalDisplacement const& x,v::CoverPhysicalDisplacement const& y) noexcept {
    return x.right==y.right&&x.up==y.up&&x.forward==y.forward;
}
template<class Source> Code prepare(ng::Result const& frozen,a::CoverPlacement const& placement,
    Source const& source,v::CoverBodyTargetKey const& current,v::CoverBodyTarget const& target,
    b::Model const& model,b::Frame const& frame,a::marker::Marker const& marker,
    a::Policy const& policy,Prepared& output) noexcept {
    using namespace b::d;
    const Range ranges[]={range(&frozen,sizeof frozen),range(&placement,sizeof placement),range(&source,sizeof source),
        range(&current,sizeof current),range(&target,sizeof target),range(&model,sizeof model),range(&frame,sizeof frame),
        range(&marker,sizeof marker),range(&policy,sizeof policy),range(&output,sizeof output)};
    if(!disjoint(ranges,10))return Code::alias;
    if(a::cover_placement_current(placement,source,policy)!=a::Status::okay)return Code::source;
    auto const& parent=ng::parent(source);auto const& tracking=ng::tracking(source);
    if(!v::cover_body_target_current(target,current,policy.now_ns)||
       current.world!=parent.world.address||current.pawn!=parent.pawn.address||current.mesh!=parent.mesh.address||
       current.link!=parent.cover_link.address||current.world_generation!=parent.world_generation||
       current.slot!=parent.cover_slot||current.thread!=parent.native.game_thread||current.scene!=parent.scene_invocation||
       current.xr_session!=tracking.session||current.xr_space!=tracking.space_epoch||
       current.reference!=parent.reference_generation||current.sequence!=tracking.sequence)return Code::key;
    if(!v::stereo_same(target.tracking.sample,tracking)||
       !v::stereo_same_pose(target.tracking.reference,parent.reference)||
       target.tracking.reference_generation!=parent.reference_generation||
       target.units_per_metre!=parent.units_per_metre)return Code::tracking;
    if(!ng::same(placement.anchor,target.anchor)||!ng::same(placement.view_anchor,target.view_anchor))return Code::placement;
    // A Scene eye request cannot promote itself to a bone/physics/write result.
    if(target.head_bone_goal_valid||target.pose_applied||target.same_pose_epoch_verified||target.collision_verified)
        return Code::target;
    if(target.observed_ns<tracking.received_ns)return Code::target;
    v::CoverBodyTarget checked{};
    if(!v::prepare_cover_body_target(current,current,target.anchor,target.view_anchor,target.committed_view,
        target.virtual_offset,target.tracking,policy.now_ns,target.units_per_metre,checked,policy.allow_sim)||
       !same(target.physical,checked.physical)||target.desired_right_metres!=checked.desired_right_metres||
       target.desired_up_metres!=checked.desired_up_metres||target.desired_forward_metres!=checked.desired_forward_metres||
       !ng::same(target.desired_eye_world,checked.desired_eye_world))return Code::target;
    // checked.observed_ns is never copied or published. Keep the original
    // request and tracking clocks; recomputation above is validation only.
    Prepared candidate{};
    if(ng::convert(frozen,placement,source,model,frame,marker,target.virtual_offset,policy,candidate.body)!=ng::Status::okay)
        return Code::convert;
    if(candidate.body.render.base_view!=target.committed_view.base_view||
       !ng::same(candidate.body.render.base_eye_world,target.committed_view.base_eye_world)||
       length(sub(candidate.body.desired_eye.position,target.desired_eye_world))>v::view_origin_max_error)return Code::view;
    candidate.key=current;candidate.target_observed_ns=target.observed_ns;
    candidate.tracking_received_ns=tracking.received_ns;
    output=candidate;return Code::prepared;
}
} // namespace gears_cover_c0_target
