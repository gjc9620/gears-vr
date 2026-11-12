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
enum class Status {okay,alias,core,units,neutral,policy,view,physical,head_marker,numerical,cover,cover_component,cover_scale,cover_atom};
inline constexpr std::uint64_t maximum_age_ns=50'000'000;
// A current, independently supplied cover placement. This is data for the
// existing planner, not collision/exposure certification or a Native write
// permit. Feet must be explicitly supplied by the entry/foot-placement owner;
// a wall-facing torso must not silently rotate or relocate old foot anchors.
struct CoverPlacement {
    gears_weapon_hand::ObjectIdentity world{},pawn{},mesh{},link{};
    std::uint64_t world_generation{},scene{},publication{},reference{};
    std::int64_t observed_ns{};std::int32_t slot{-1};std::uint32_t type{};
    vr::CoverViewAnchor anchor{};
    // Continuous tracking/view heading, supplied independently of the wall.
    // Its neutral eye equals anchor's; turning the body cannot turn the HMD.
    vr::CoverViewAnchor view_anchor{};
    body::Pose feet_world[2]{};
};
// Original C0 geometry, deliberately separate from a certified neutral/Goal.
// An action/transition pose is not automatically a neutral calibration. The
// live owner must still bind neutral action, surface, reach and collision.
struct CoverPoseMeasurement {
    CoverPlacement binding{};
    vr::Pose eye{};
    body::Pose root{},pelvis{},feet[2]{};
};
// Math over an independently admitted source. observed_ns is the original
// Scene publication for Core, or the actual V3 metadata observation end.
inline Status measure_cover_pose_math(core::CoreSnapshot const& source,std::int64_t observed_ns,body::Model const& model,
    body::Frame const& frame,marker::Marker const& authored,Policy const& policy,
    body::Workspace& work,CoverPoseMeasurement& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&source,sizeof source),range(&model,sizeof model),range(&frame,sizeof frame),
        range(&authored,sizeof authored),range(&policy,sizeof policy),range(&work,sizeof work),range(&out,sizeof out)};
    if(!disjoint(ranges,7))return Status::alias;
    if(observed_ns<=0||observed_ns>policy.now_ns||
       std::uint64_t(policy.now_ns-observed_ns)>maximum_age_ns)return Status::core;
    if(!(source.flags&core::rig::cover_key_known)||!source.cover_type||source.cover_slot<0)return Status::cover;
    if(frame.model_scale!=1||frame.units_per_metre!=source.units_per_metre)return Status::units;
    if(marker::detail::context(model,frame,model.canonical)!=marker::Status::okay)return Status::head_marker;
    for(unsigned i=0;i<16;++i)work.component.m[i]=frame.component_to_world.values[i];
    if(!similarity(work.component)||std::abs(length(row(work.component,0))-1)>2e-5)return Status::cover_component;
    for(auto const& atom:frame.local)if(!unit_animation_scale(atom.scale))return Status::cover_scale;
    if(gears_cover_passive_atoms::validate(frame.local.data(),model.parents.data(),body::bones,work.component,policy.scope==body::Scope::fixed_cover)!=body::ik::Status::okay)return Status::cover_atom;
    std::copy(frame.local.begin(),frame.local.end(),work.pose.local);
    if(!rebuild(model,work))return Status::numerical;
    CoverPoseMeasurement next{};marker::EyeHmdWorld eye{};
    if(marker::eye_from_head(authored,model,frame,{pose(work.pose.world[body::head])},eye)!=marker::Status::okay)return Status::head_marker;
    next.eye=eye.pose;next.root=pose(work.pose.world[body::root]);next.pelvis=pose(work.pose.world[body::pelvis]);
    for(unsigned i=0;i<2;++i)next.feet[i]=pose(work.pose.world[body::legs[i][2]]);
    auto& key=next.binding;key.world=source.world;key.pawn=source.pawn;key.mesh=source.mesh;key.link=source.cover_link;
    key.world_generation=source.world_generation;key.scene=source.scene_invocation;key.publication=source.publication_sequence;
    key.reference=source.reference_generation;key.slot=source.cover_slot;key.type=source.cover_type;
    key.observed_ns=observed_ns; // Original receipt; never refresh any source clock.
    out=next;return Status::okay;
}
inline Status measure_cover_pose(core::CoreSnapshot const& source,body::Model const& model,
    body::Frame const& frame,marker::Marker const& authored,Policy const& policy,
    body::Workspace& work,CoverPoseMeasurement& out) noexcept {
    if(!core::core_snapshot_valid(source,policy.now_ns,maximum_age_ns,policy.allow_sim))return Status::core;
    return measure_cover_pose_math(source,std::int64_t(source.published_ns),model,frame,authored,policy,work,out);
}
inline Status measure_cover_pose(gears_body_tick_source::Input const& input,body::Model const& model,
    body::Frame const& frame,marker::Marker const& authored,Policy const& policy,
    body::Workspace& work,CoverPoseMeasurement& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&input,sizeof input),range(&model,sizeof model),range(&frame,sizeof frame),
        range(&authored,sizeof authored),range(&policy,sizeof policy),range(&work,sizeof work),range(&out,sizeof out)};
    if(!disjoint(ranges,7))return Status::alias;
    if(gears_body_tick_source::classify(input,input.observed.parent_scene,policy.now_ns,policy.allow_sim)!=gears_body_tick_source::Verdict::okay)return Status::core;
    return measure_cover_pose_math(input.observed.parent_scene,input.metadata_end_ns,model,frame,authored,policy,work,out);
}
inline Status place_cover(Neutral const& calibrated,vr::CoverViewAnchor const& anchor,
                         vr::CoverViewAnchor const& view_anchor,
                         body::Pose const (&feet)[2],Neutral& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&calibrated,sizeof calibrated),range(&anchor,sizeof anchor),
                          range(&view_anchor,sizeof view_anchor),range(feet,sizeof feet),range(&out,sizeof out)};
    if(!disjoint(ranges,5))return Status::alias;
    body::Quat root{};
    if(!vr::cover_view_basis(calibrated.eye_anchor)||!vr::cover_view_basis(anchor)||!vr::cover_view_basis(view_anchor)||
       anchor.neutral_eye_world.x!=view_anchor.neutral_eye_world.x||
       anchor.neutral_eye_world.y!=view_anchor.neutral_eye_world.y||
       anchor.neutral_eye_world.z!=view_anchor.neutral_eye_world.z||
       !finite_vector(calibrated.pelvis_world)||!normalized(calibrated.root_bone_world_q,root)||
       !goal_pose(feet[0])||!goal_pose(feet[1]))return Status::neutral;
    const auto f=calibrated.eye_anchor.forward_world;
    const auto target=anchor.forward_world;
    const double angle=std::atan2(f.x*target.y-f.y*target.x,f.x*target.x+f.y*target.y);
    const auto turn=yaw(angle);
    Neutral next{};next.eye_anchor=view_anchor;
    next.pelvis_world=add(anchor.neutral_eye_world,rotate(turn,sub(calibrated.pelvis_world,calibrated.eye_anchor.neutral_eye_world)));
    next.root_bone_world_q=product(turn,root);
    next.feet_world[0]=feet[0];next.feet_world[1]=feet[1];
    if(!finite_vector(next.pelvis_world))return Status::numerical;
    out=next;return Status::okay;
}
inline Status prepare_axes_math(Neutral const& neutral,vr::CoverViewAnchor grip_basis,core::CoreSnapshot const& source,
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
    if(!vr::cover_view_basis(neutral.eye_anchor)||!vr::cover_view_basis(grip_basis)||
       neutral.eye_anchor.neutral_eye_world.x!=grip_basis.neutral_eye_world.x||
       neutral.eye_anchor.neutral_eye_world.y!=grip_basis.neutral_eye_world.y||
       neutral.eye_anchor.neutral_eye_world.z!=grip_basis.neutral_eye_world.z||!finite_vector(neutral.pelvis_world)||
       !normalized(neutral.root_bone_world_q,root)||!goal_pose(neutral.feet_world[0])||!goal_pose(neutral.feet_world[1]))return Status::neutral;
    Output candidate{};
    const auto& basis=neutral.eye_anchor;
    if(!vr::prepare_cover_view(grip_basis,basis,committed,source.units_per_metre,candidate.render))return Status::view;
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
    g.committed_grip_world={u*grip_basis.right_world.x*committed.right,u*grip_basis.right_world.y*committed.right,u*committed.up};
    g.feet_world[0]=neutral.feet_world[0];g.feet_world[1]=neutral.feet_world[1];
    const marker::EyeHmdWorld eye{candidate.desired_eye};marker::HeadBoneWorld head{};
    if(marker::head_from_eye(authored,model,frame,eye,head)!=marker::Status::okay)return Status::head_marker;
    g.head_world=head.pose;
    if(!finite_vector(g.physical_world)||!finite_vector(g.committed_grip_world)||!goal_pose(g.head_world))return Status::numerical;
    // Float render-origin rounding is retained in desired_eye; never feed it
    // back as a physical displacement or rebase the fixed Neutral.
    out=candidate;return Status::okay;
}
inline Status prepare_math(Neutral const& neutral,core::CoreSnapshot const& source,
    vr::StereoSample const& tracking,body::Model const& model,body::Frame const& frame,marker::Marker const& authored,
    vr::CoverOffset const& committed,Policy const& policy,Output& out) noexcept {
    return prepare_axes_math(neutral,neutral.eye_anchor,source,tracking,model,frame,authored,committed,policy,out);
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
// Shared admission for initial placement and later conversion of a frozen
// fitted neutral. Checking this binding does not reapply placement rotations.
inline Status cover_placement_binding(CoverPlacement const& placement,
    core::CoreSnapshot const& source,Policy const& policy) noexcept {
    if(policy.scope!=body::Scope::fixed_cover)return Status::policy;
    if(!(source.flags&core::rig::cover_key_known)||!placement.type||placement.type!=source.cover_type||
       placement.slot<0||placement.slot!=source.cover_slot||
       !gears_weapon_hand::same(placement.world,source.world)||!gears_weapon_hand::same(placement.pawn,source.pawn)||
       !gears_weapon_hand::same(placement.mesh,source.mesh)||!gears_weapon_hand::same(placement.link,source.cover_link)||
       placement.world_generation!=source.world_generation||placement.scene!=source.scene_invocation||
       placement.publication!=source.publication_sequence||placement.reference!=source.reference_generation||
       placement.observed_ns<=0||placement.observed_ns>policy.now_ns||
       std::uint64_t(policy.now_ns-placement.observed_ns)>maximum_age_ns)return Status::cover;
    return Status::okay;
}
inline Status cover_placement_current(CoverPlacement const& placement,
    core::CoreSnapshot const& source,Policy const& policy) noexcept {
    if(!core::core_snapshot_valid(source,policy.now_ns,maximum_age_ns,policy.allow_sim))return Status::core;
    return cover_placement_binding(placement,source,policy);
}
inline Status cover_placement_current(CoverPlacement const& placement,
    gears_body_tick_source::Input const& input,Policy const& policy) noexcept {
    if(gears_body_tick_source::classify(input,input.observed.parent_scene,policy.now_ns,policy.allow_sim)!=gears_body_tick_source::Verdict::okay)return Status::core;
    if(placement.observed_ns<input.metadata_begin_ns||placement.observed_ns>input.metadata_end_ns)return Status::cover;
    return cover_placement_binding(placement,input.observed.parent_scene,policy);
}
inline Status place_cover_head_math(Neutral const& calibrated,CoverPlacement const& placement,
    core::CoreSnapshot const& source,body::Model const& model,body::Frame const& frame,
    marker::Marker const& authored,Policy const& policy,Neutral& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&calibrated,sizeof calibrated),range(&placement,sizeof placement),range(&source,sizeof source),
        range(&model,sizeof model),range(&frame,sizeof frame),range(&authored,sizeof authored),
        range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,8))return Status::alias;
    const auto binding=cover_placement_binding(placement,source,policy);
    if(binding!=Status::okay)return binding;
    Neutral placed{};
    const auto status=place_cover(calibrated,placement.anchor,placement.view_anchor,placement.feet_world,placed);
    if(status!=Status::okay)return status;
    // The authored eye offset rotates with the head, not with the torso.
    // Keep the reference spine under the same HEAD pivot when body heading
    // and continuous view heading differ. This uses neither current HMD nor
    // animated/previous output; no extra camera displacement is introduced.
    vr::CoverViewResult wall_view{},tracking_view{};vr::Pose wall_eye{},tracking_eye{};
    if(!vr::prepare_cover_view(placement.anchor,{},source.units_per_metre,wall_view)||
       !vr::prepare_cover_view(placement.view_anchor,{},source.units_per_metre,tracking_view)||
       !vr::rig_tracking::pose(wall_view.base_view,source.reference,source.reference,source.units_per_metre,wall_eye)||
       !vr::rig_tracking::pose(tracking_view.base_view,source.reference,source.reference,source.units_per_metre,tracking_eye))return Status::view;
    wall_eye.position={};tracking_eye.position={};
    marker::HeadBoneWorld wall_head{},tracking_head{};
    if(marker::head_from_eye(authored,model,frame,{wall_eye},wall_head)!=marker::Status::okay||
       marker::head_from_eye(authored,model,frame,{tracking_eye},tracking_head)!=marker::Status::okay)return Status::head_marker;
    placed.pelvis_world=add(placed.pelvis_world,sub(tracking_head.pose.position,wall_head.pose.position));
    out=placed;return Status::okay;
}
inline Status place_cover_head(Neutral const& calibrated,CoverPlacement const& placement,
    core::CoreSnapshot const& source,body::Model const& model,body::Frame const& frame,
    marker::Marker const& authored,Policy const& policy,Neutral& out) noexcept {
    if(!core::core_snapshot_valid(source,policy.now_ns,maximum_age_ns,policy.allow_sim))return Status::core;
    return place_cover_head_math(calibrated,placement,source,model,frame,authored,policy,out);
}
inline Status place_cover_head(Neutral const& calibrated,CoverPlacement const& placement,
    gears_body_tick_source::Input const& input,body::Model const& model,body::Frame const& frame,
    marker::Marker const& authored,Policy const& policy,Neutral& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&calibrated,sizeof calibrated),range(&placement,sizeof placement),range(&input,sizeof input),
        range(&model,sizeof model),range(&frame,sizeof frame),range(&authored,sizeof authored),range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,8))return Status::alias;
    if(gears_body_tick_source::classify(input,input.observed.parent_scene,policy.now_ns,policy.allow_sim)!=gears_body_tick_source::Verdict::okay)return Status::core;
    if(placement.observed_ns<input.metadata_begin_ns||placement.observed_ns>input.metadata_end_ns)return Status::cover;
    return place_cover_head_math(calibrated,placement,input.observed.parent_scene,model,frame,authored,policy,out);
}
inline Status prepare_cover(Neutral const& calibrated,CoverPlacement const& placement,
    core::CoreSnapshot const& source,body::Model const& model,body::Frame const& frame,
    marker::Marker const& authored,vr::CoverOffset const& committed,Policy const& policy,Output& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&calibrated,sizeof calibrated),range(&placement,sizeof placement),range(&source,sizeof source),
        range(&model,sizeof model),range(&frame,sizeof frame),range(&authored,sizeof authored),range(&committed,sizeof committed),
        range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,9))return Status::alias;
    Neutral placed{};
    const auto status=place_cover_head(calibrated,placement,source,model,frame,authored,policy,placed);
    if(status!=Status::okay)return status;
    return prepare_axes_math(placed,placement.anchor,source,source.stereo,model,frame,authored,committed,policy,out);
}
inline Status prepare_cover(Neutral const& calibrated,CoverPlacement const& placement,
    gears_body_tick_source::Input const& input,body::Model const& model,body::Frame const& frame,
    marker::Marker const& authored,vr::CoverOffset const& committed,Policy const& policy,Output& out) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&calibrated,sizeof calibrated),range(&placement,sizeof placement),range(&input,sizeof input),
        range(&model,sizeof model),range(&frame,sizeof frame),range(&authored,sizeof authored),range(&committed,sizeof committed),
        range(&policy,sizeof policy),range(&out,sizeof out)};
    if(!disjoint(ranges,9))return Status::alias;
    Neutral placed{};
    const auto status=place_cover_head(calibrated,placement,input,model,frame,authored,policy,placed);
    if(status!=Status::okay)return status;
    return prepare_axes_math(placed,placement.anchor,input.observed.parent_scene,input.observed.stereo,
        model,frame,authored,committed,policy,out);
}
} // namespace gears_body_goal_view
