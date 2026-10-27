#pragma once
#include "body_head_authored_mount.h"
#include "body_tick_input.h"
#include "body_tick_source.h"
#include "../vr-common/native_rig_tracking_frame.h"
#include "../vr-common/cover_view.h"
#include "../vr-common/rig_tracking_math.h"

// Pure data conversion only. Neutral is fixed by the caller independently of
// animated Local, Core.world_head, previous Output, and any renderer FP lifts.
// No Native admission, owner authentication, engine read/write or calibration.
namespace gears_body_goal_view {
namespace body=gears_full_body_c0;
namespace marker=gears_body_head_authored;
namespace vr=gears_vr;
namespace core=gears_native_rig_tracking;
struct Neutral {
    vr::CoverViewAnchor eye_anchor{};
    body::Vec pelvis_world{};
    body::Quat root_bone_world_q{};
    body::Pose feet_world[2]{};
};
struct Policy {
    std::int64_t now_ns{};bool allow_sim{};
    body::Scope scope{body::Scope::fixed_cover};
    body::Posture posture{body::Posture::preserve_c0};
};
struct Output {
    body::Goal goal{};
    vr::CoverViewResult render{}; // Final pre-HMD base, committed virtual only.
    vr::Pose desired_eye{};      // UE world +X forward,+Y right,+Z up.
};
enum class Status {okay,alias,core,units,neutral,policy,view,physical,head_marker,numerical};
inline constexpr std::uint64_t maximum_age_ns=50'000'000;
inline Status prepare_math(Neutral const& neutral,core::CoreSnapshot const& source,
    vr::StereoSample const& tracking,body::Model const& model,body::Frame const& frame,marker::Marker const& authored,
    vr::CoverOffset const& committed,Policy const& policy,Output& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&neutral,sizeof neutral),range(&source,sizeof source),range(&model,sizeof model),
        range(&frame,sizeof frame),range(&authored,sizeof authored),range(&committed,sizeof committed),range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,8))return Status::alias;
    if(frame.units_per_metre!=source.units_per_metre||frame.model_scale!=1)return Status::units;
    if((policy.scope!=body::Scope::stationary&&policy.scope!=body::Scope::fixed_cover)||
       (policy.posture!=body::Posture::preserve_c0&&policy.posture!=body::Posture::reference_neutral))return Status::policy;
    body::Quat root{};
    if(!vr::cover_view_basis(neutral.eye_anchor)||!finite_vector(neutral.pelvis_world)||
       !normalized(neutral.root_bone_world_q,root)||!goal_pose(neutral.feet_world[0])||!goal_pose(neutral.feet_world[1]))return Status::neutral;
    Output candidate{};
    if(!vr::prepare_cover_view(neutral.eye_anchor,committed,source.units_per_metre,candidate.render))return Status::view;
    // This helper ALREADY returns a proper UE pose. Do not remap its axes.
    if(!vr::rig_tracking::pose(candidate.render.base_view,source.reference,tracking.head,
                              source.units_per_metre,candidate.desired_eye))return Status::view;
    vr::CoverPhysicalDisplacement physical{};
    if(!vr::cover_physical_displacement(source.reference,tracking.head,physical))return Status::physical;
    const auto& a=neutral.eye_anchor;const auto u=source.units_per_metre;
    auto& g=candidate.goal;g.scope=policy.scope;g.posture=policy.posture;
    g.target_mode=body::BodyTargetMode::world_absolute;g.body_yaw_delta=0;
    g.absolute.neutral_pelvis_world=neutral.pelvis_world;
    g.absolute.root_orientation.value=neutral.root_bone_world_q;
    g.physical_world={u*(a.right_world.x*physical.right+a.forward_world.x*physical.forward),
                      u*(a.right_world.y*physical.right+a.forward_world.y*physical.forward),u*physical.up};
    g.committed_grip_world={u*a.right_world.x*committed.right,u*a.right_world.y*committed.right,u*committed.up};
    g.feet_world[0]=neutral.feet_world[0];g.feet_world[1]=neutral.feet_world[1];
    const marker::EyeHmdWorld eye{candidate.desired_eye};marker::HeadBoneWorld head{};
    if(marker::head_from_eye(authored,model,frame,eye,head)!=marker::Status::okay)return Status::head_marker;
    g.head_world=head.pose;
    if(!finite_vector(g.physical_world)||!finite_vector(g.committed_grip_world)||!goal_pose(g.head_world))return Status::numerical;
    // Float render-origin rounding is retained in desired_eye; never feed it
    // back as a physical displacement or rebase the fixed Neutral.
    out=candidate;return Status::okay;
}
inline Status prepare(Neutral const& neutral,core::CoreSnapshot const& source,
    body::Model const& model,body::Frame const& frame,marker::Marker const& authored,
    vr::CoverOffset const& committed,Policy const& policy,Output& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&neutral,sizeof neutral),range(&source,sizeof source),range(&model,sizeof model),
        range(&frame,sizeof frame),range(&authored,sizeof authored),range(&committed,sizeof committed),range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,8))return Status::alias;
    if(!core::core_snapshot_valid(source,policy.now_ns,maximum_age_ns,policy.allow_sim))return Status::core;
    return prepare_math(neutral,source,source.stereo,model,frame,authored,committed,policy,out);
}
inline Status prepare(Neutral const& neutral,gears_body_tick_input::Input const& input,
    body::Model const& model,body::Frame const& frame,marker::Marker const& authored,
    vr::CoverOffset const& committed,Policy const& policy,Output& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&neutral,sizeof neutral),range(&input,sizeof input),range(&model,sizeof model),
        range(&frame,sizeof frame),range(&authored,sizeof authored),range(&committed,sizeof committed),range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,8))return Status::alias;
    if(gears_body_tick_input::classify(input,input.parent_scene,policy.now_ns,policy.allow_sim)!=gears_body_tick_input::Verdict::okay)return Status::core;
    return prepare_math(neutral,input.parent_scene,input.stereo,model,frame,authored,committed,policy,out);
}
inline Status prepare(Neutral const& neutral,gears_body_tick_source::Input const& input,
    body::Model const& model,body::Frame const& frame,marker::Marker const& authored,
    vr::CoverOffset const& committed,Policy const& policy,Output& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&neutral,sizeof neutral),range(&input,sizeof input),range(&model,sizeof model),
        range(&frame,sizeof frame),range(&authored,sizeof authored),range(&committed,sizeof committed),range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,8))return Status::alias;
    if(gears_body_tick_source::classify(input,input.observed.parent_scene,policy.now_ns,policy.allow_sim)!=gears_body_tick_source::Verdict::okay)return Status::core;
    return prepare_math(neutral,input.observed.parent_scene,input.observed.stereo,model,frame,authored,committed,policy,out);
}
} // namespace gears_body_goal_view
