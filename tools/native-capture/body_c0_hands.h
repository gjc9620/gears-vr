#pragma once
#include "full_body_c0_planner.h"
#include "hand_core_target_adapter.h"
#include "../vr-common/free_hand_eye_targets.h"

// Owned-buffer composition only. No clock query, address dereference, engine
// invocation or write permission. The caller authenticates this SAME live
// latch/owner/asset independently and owns the single resulting C0 transaction.
namespace gears_body_c0_hands {
namespace body=gears_full_body_c0;
namespace adapter=gears_hand_core_target;
namespace core=gears_native_rig_tracking;
namespace hand=gears_weapon_hand;
namespace mount=gears_hand_authored_mount;
namespace two=gears_hand_two_bone;
enum class Status {okay,alias,basis,reference,invalid_body,invalid_core,binding,hand_untracked,unreachable,numerical,residual};
struct Result {Status status{Status::okay};unsigned side{};adapter::Status hand_status{adapter::Status::okay};};
// Persistent caller heap/TLS only. No large temporary lives on the hook stack.
// Scratch can change on failure; inout and every other input remain unchanged.
struct Workspace {
    unsigned solved_mask{};
    gears_free_hand_eye_targets::EyeFrame eye_frame{};
    gears_free_hand_eye_targets::TrackingFrame tracking_frame{};
    gears_free_hand_eye_targets::HandTarget eye_targets[2]{};
    body::Workspace body_work{};
    adapter::ModelBinding bindings[2]{};
    adapter::Target targets[2]{};
    adapter::TickTarget tick_targets[2]{};
    adapter::SourceTarget source_targets[2]{};
    gears_vr::Pose computed_wrist[2]{};
    two::Input inputs[2]{};two::Solution solutions[2]{};
    std::array<body::ik::Matrix,7> body_before{};
};
namespace detail {
inline Result fail(Status status,unsigned side=0,adapter::Status why=adapter::Status::okay) noexcept {return {status,side,why};}
inline bool pose_matches(body::ik::Matrix const& actual,body::Pose const& expected) noexcept {
    return body::d::goal_pose(expected)&&body::d::length(body::d::sub(body::d::position(actual),expected.position))<=.002&&
        body::d::angle(body::d::rotation(actual),expected.orientation)<=1e-4;
}
inline Status convert(adapter::Status s) noexcept {
    switch(s){
    case adapter::Status::binding_mismatch:case adapter::Status::unknown_mount:return Status::binding;
    case adapter::Status::hand_untracked:return Status::hand_untracked;
    case adapter::Status::unreachable:return Status::unreachable;
    default:return Status::invalid_core;
    }
}
}
// appends arm/wrist Q to the just-solved complete body output. Success replaces
// inout once; ANY failure leaves its complete representation byte-identical.
// Input, inout and scratch object ranges must all be disjoint.
template<class Prepare>
inline Result compose_common(body::Model const& model,body::Frame const& frame,core::CoreSnapshot const& source,
    hand::ObjectIdentity const& authenticated_asset,gears_vr::StereoMatrix const& render_base,
    std::int64_t now,bool allow_sim,Workspace& work,body::Output& inout,
    adapter::MountKind right_mount,Prepare prepare,bool independent=false) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&source,sizeof source),
        range(&authenticated_asset,sizeof authenticated_asset),range(&render_base,sizeof render_base),
        range(&work,sizeof work),range(&inout,sizeof inout)};
    if(!disjoint(ranges,7))return detail::fail(Status::alias);
    work.solved_mask=0;
    if(right_mount!=adapter::MountKind::free_hand&&right_mount!=adapter::MountKind::held_lancer)return detail::fail(Status::binding,1,adapter::Status::unknown_mount);
    (void)now;(void)allow_sim;
    auto& w=work.body_work;
    if(frame.count!=body::bones||frame.model_scale!=1||frame.units_per_metre!=source.units_per_metre||
       !hand::identity_valid(authenticated_asset))return detail::fail(Status::binding);
    if(model.signature!=mount::marcus115_signature||!mount::canonical_geometry(frame.reference.data(),body::bones,w.canonical)||
       w.canonical!=model.canonical)return detail::fail(Status::reference);
    for(unsigned i=0;i<body::bones;++i){std::int32_t parent{};std::memcpy(&parent,w.canonical.data()+i*36+4,4);
        if(model.parents[i]!=parent||std::memcmp(model.reference[i].q,w.canonical.data()+i*36+8,16)||
           std::memcmp(model.reference[i].translation,w.canonical.data()+i*36+24,12)||model.reference[i].scale!=1)return detail::fail(Status::reference);}
    if(inout.report.status!=body::Status::solved)return detail::fail(Status::invalid_body);
    const double residuals[]={inout.report.head_error,inout.report.foot_error[0],inout.report.foot_error[1],inout.report.max_length_error};
    for(double value:residuals)if(!std::isfinite(value)||value<0||value>.002)return detail::fail(Status::invalid_body);
    const double angles[]={inout.report.head_angle_error,inout.report.foot_angle_error[0],inout.report.foot_angle_error[1]};
    for(double value:angles)if(!std::isfinite(value)||value<0||value>1e-4)return detail::fail(Status::invalid_body);
    if(!std::isfinite(inout.report.pelvis_projection)||inout.report.pelvis_projection<0)return detail::fail(Status::invalid_body);
    for(unsigned i=0;i<16;++i)w.component.m[i]=frame.component_to_world.values[i];
    if(!similarity(w.component)||std::abs(length(row(w.component,0))-1)>2e-5||
       validate(frame.local.data(),model.parents.data(),body::bones,w.component)!=body::ik::Status::okay||
       validate(inout.local.data(),model.parents.data(),body::bones,w.component)!=body::ik::Status::okay)return detail::fail(Status::invalid_body);
    std::array<std::uint64_t,2> changed{};
    for(unsigned i=0;i<body::bones;++i){
        if(frame.local[i].scale!=1||std::memcmp(&frame.local[i].scale,&inout.local[i].scale,4)||
           (i&&std::memcmp(frame.local[i].translation,inout.local[i].translation,12)))return detail::fail(Status::invalid_body);
        if(std::memcmp(&frame.local[i],&inout.local[i],sizeof(body::Atom)))changed[i/64]|=std::uint64_t(1)<<(i%64);
    }
    if(changed!=inout.changed_mask)return detail::fail(Status::invalid_body);
    std::memcpy(w.pose.local,frame.local.data(),sizeof frame.local);
    if(!body::d::rebuild(model,w))return detail::fail(Status::numerical);
    std::copy_n(w.pose.world,body::bones,w.original_world.data());
    std::memcpy(w.pose.local,inout.local.data(),sizeof inout.local);
    if(!body::d::rebuild(model,w))return detail::fail(Status::numerical);
    constexpr unsigned stable[]={body::root,body::pelvis,body::head,body::legs[0][2],body::legs[1][2],8,32};
    const body::Pose* poses[]={&inout.root_world,&inout.pelvis_world,&inout.head_world,&inout.feet_world[0],&inout.feet_world[1],&inout.shoulders_world[0],&inout.shoulders_world[1]};
    for(unsigned i=0;i<7;++i){if(!detail::pose_matches(w.pose.world[stable[i]],*poses[i]))return detail::fail(Status::invalid_body);work.body_before[i]=w.pose.world[stable[i]];}
    for(unsigned side=0;side<2;++side){
        auto& b=work.bindings[side];b={};b.native=source.native;b.world=source.world;b.pawn=source.pawn;b.mesh=source.mesh;b.asset=authenticated_asset;
        b.world_generation=source.world_generation;b.binding_generation=source.binding_generation;b.model=mount::Model::marcus115;
        b.geometry_signature=model.signature;b.bone_count=body::bones;b.model_to_world_scale=frame.model_scale;
        b.mount_kind=side?right_mount:adapter::MountKind::free_hand;
        if(b.mount_kind==adapter::MountKind::held_lancer){
            gears_weapon_lancer_wrist_mount::Profile held{};
            if(!gears_weapon_lancer_wrist_mount::marcus115(model.signature,frame.model_scale,source.units_per_metre,held))return detail::fail(Status::binding,side);
            b.authored_mount=held.mount;
        }else if(!mount::marcus115(model.signature,static_cast<mount::Side>(side),frame.model_scale,source.units_per_metre,b.authored_mount))return detail::fail(Status::binding,side);
        const auto prepared=prepare(b,side);
        if(independent&&prepared==adapter::Status::hand_untracked)continue;
        if(prepared!=adapter::Status::okay)return detail::fail(detail::convert(prepared),side,prepared);
        // Fixed-length solve. The current body-only C0 elbow is an explicit
        // bend pole; the shared solver falls back to the shoulder frame if
        // collinear. No previous-frame state or animated-wrist marker fitting.
        const auto& chain=b.authored_mount.chain;auto& input=work.inputs[side];input={};
        input.shoulder=body::d::pose(w.pose.world[chain[0]]);input.elbow=body::d::pose(w.pose.world[chain[1]]);input.wrist=body::d::pose(w.pose.world[chain[2]]);
        input.target=work.computed_wrist[side];input.authored={true,input.elbow.position};input.lengths.mode=two::LengthMode::preserve;
        if(independent){
            // A real controller can exceed this avatar's fixed arm span. Keep
            // its full rotation and project only the endpoint onto the arm's
            // reachable shell; never stretch bones or move the camera/body.
            const double upper=length(sub(input.elbow.position,input.shoulder.position));
            const double lower=length(sub(input.wrist.position,input.elbow.position));
            const auto delta=sub(input.target.position,input.shoulder.position);
            const double distance=length(delta),minimum=std::abs(upper-lower)+.001,maximum=upper+lower-.001;
            if(!std::isfinite(distance)||distance<1e-6||maximum<=minimum)return detail::fail(Status::numerical,side);
            const double reachable=(std::max)(minimum,(std::min)(distance,maximum));
            if(reachable!=distance)input.target.position=add(input.shoulder.position,mul(delta,reachable/distance));
        }
        const auto solved=two::solve(input,work.solutions[side]);
        if(solved!=two::Status::solved)return detail::fail(solved==two::Status::unreachable?Status::unreachable:Status::numerical,side,
            solved==two::Status::unreachable?adapter::Status::unreachable:adapter::Status::numerical_failure);
        const body::Pose target[]={work.solutions[side].shoulder,work.solutions[side].elbow,work.solutions[side].wrist};
        for(unsigned i=0;i<3;++i)if(!body::d::world_rotation(model,w,chain[i],target[i].orientation))return detail::fail(Status::numerical,side);
        if(!detail::pose_matches(w.pose.world[chain[2]],input.target))return detail::fail(Status::residual,side);
        work.solved_mask|=1u<<side;
    }
    // Both hands must succeed before there is any candidate publication.
    std::memcpy(&w.candidate,&inout,sizeof inout);std::copy_n(w.pose.local,body::bones,w.candidate.local.begin());w.candidate.changed_mask={};
    double max_length_error{};
    for(unsigned i=0;i<body::bones;++i){
        if(std::memcmp(w.pose.local[i].translation,inout.local[i].translation,12)||std::memcmp(&w.pose.local[i].scale,&inout.local[i].scale,4))return detail::fail(Status::numerical);
        if(std::memcmp(&frame.local[i],&w.pose.local[i],sizeof(body::Atom)))w.candidate.changed_mask[i/64]|=std::uint64_t(1)<<(i%64);
        if(i){const auto p=unsigned(model.parents[i]);max_length_error=(std::max)(max_length_error,std::abs(
            length(body::d::sub(position(w.pose.world[i]),position(w.pose.world[p])))-length(body::d::sub(position(w.original_world[i]),position(w.original_world[p])))));}
    }
    for(unsigned i=0;i<5;++i)if(std::memcmp(&w.pose.world[stable[i]],&work.body_before[i],sizeof(body::ik::Matrix)))return detail::fail(Status::residual);
    if(!std::isfinite(max_length_error)||max_length_error>.002)return detail::fail(Status::residual);
    w.candidate.report.max_length_error=max_length_error;
    for(unsigned side=0;side<2;++side)w.candidate.shoulders_world[side]=body::d::pose(w.pose.world[body::shoulders[side]]);
    std::memcpy(&inout,&w.candidate,sizeof inout);return {};
}
// Current admitted tick plus the exact desired_eye that generated the body
// head goal. No second legacy FP/reference offset or previous-frame target.
inline Result compose_eye(body::Model const& model,body::Frame const& frame,
    gears_body_tick_input::Input const& input,std::uint64_t compose,
    hand::ObjectIdentity const& asset,gears_vr::StereoMatrix const& base,
    gears_vr::Pose const& desired_eye,std::int64_t now,bool allow_sim,
    Workspace& work,body::Output& inout,adapter::MountKind right_mount=adapter::MountKind::free_hand) noexcept {
    namespace free=gears_free_hand_eye_targets;
    if(gears_body_tick_input::classify(input,input.parent_scene,now,allow_sim)!=gears_body_tick_input::Verdict::okay)
        return detail::fail(Status::invalid_core);
    auto const& s=input.parent_scene;auto const& t=input.stereo;auto const& c=input.controllers;
    free::Key key{};key.game_pid=s.native.game_pid;key.game_thread=s.native.game_thread;
    key.game_start=s.native.game_start;key.native_started_qpc=s.native.native_started_qpc;
    key.world_generation=s.world_generation;key.mesh=s.mesh.address;key.binding_generation=s.binding_generation;
    key.tick=input.tick;key.compose=compose;key.source=t.source;key.stereo_session=t.session;
    key.stereo_sequence=t.sequence;key.space_epoch=t.space_epoch;key.reference_revision=input.reference_revision;
    key.controller_sequence=c.sequence;key.input_epoch=c.input_epoch;key.profile_generation=c.profile_generation;
    key.predicted_time=t.predicted_time;
    work.eye_frame={key,desired_eye};work.tracking_frame={key,t,c};
    if(free::frame_status(work.eye_frame,work.tracking_frame,frame.units_per_metre)!=free::Status::okay)
        return detail::fail(Status::invalid_core);
    return compose_common(model,frame,s,asset,base,now,allow_sim,work,inout,right_mount,
        [&](adapter::ModelBinding const& binding,unsigned side){
            auto const result=free::prepare_one(work.eye_frame,work.tracking_frame,side,
                binding.authored_mount.calibration,frame.units_per_metre,work.eye_targets[side]);
            if(result){work.computed_wrist[side]=work.eye_targets[side].wrist_world;return adapter::Status::okay;}
            return result.hand==free::HandStatus::untracked?adapter::Status::hand_untracked:adapter::Status::numerical_failure;
        },true);
}
inline Result compose(body::Model const& model,body::Frame const& frame,core::CoreSnapshot const& source,
    hand::ObjectIdentity const& asset,gears_vr::StereoMatrix const& base,std::int64_t now,bool allow_sim,
    Workspace& work,body::Output& inout,adapter::MountKind right_mount=adapter::MountKind::free_hand) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&source,sizeof source),range(&asset,sizeof asset),
        range(&base,sizeof base),range(&work,sizeof work),range(&inout,sizeof inout)};
    if(!disjoint(ranges,7))return detail::fail(Status::alias);
    if(right_mount!=adapter::MountKind::free_hand&&right_mount!=adapter::MountKind::held_lancer)return detail::fail(Status::binding,1,adapter::Status::unknown_mount);
    if(std::memcmp(&source.base_view,&base,sizeof base))return detail::fail(Status::basis);
    return compose_common(model,frame,source,asset,base,now,allow_sim,work,inout,right_mount,
        [&](adapter::ModelBinding const& binding,unsigned side){
            const auto result=adapter::prepare_hand(source,binding,static_cast<adapter::Side>(side),now,50000000,allow_sim,{false},work.targets[side]);
            if(result==adapter::Status::okay)work.computed_wrist[side]=work.targets[side].wrist_world;
            return result;
        });
}
inline Result compose(body::Model const& model,body::Frame const& frame,gears_body_tick_input::Input const& input,
    hand::ObjectIdentity const& asset,gears_vr::StereoMatrix const& base,std::int64_t now,bool allow_sim,
    Workspace& work,body::Output& inout,adapter::MountKind right_mount=adapter::MountKind::free_hand) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&input,sizeof input),range(&asset,sizeof asset),
        range(&base,sizeof base),range(&work,sizeof work),range(&inout,sizeof inout)};
    if(!disjoint(ranges,7))return detail::fail(Status::alias);
    if(gears_body_tick_input::classify(input,input.parent_scene,now,allow_sim)!=gears_body_tick_input::Verdict::okay)return detail::fail(Status::invalid_core);
    return compose_common(model,frame,input.parent_scene,asset,base,now,allow_sim,work,inout,right_mount,
        [&](adapter::ModelBinding const& binding,unsigned side){
            const auto result=adapter::prepare_hand(input,binding,static_cast<adapter::Side>(side),base,now,allow_sim,work.tick_targets[side]);
            if(result==adapter::Status::okay)work.computed_wrist[side]=work.tick_targets[side].wrist_world;
            return result;
        });
}
inline Result compose(body::Model const& model,body::Frame const& frame,gears_body_tick_source::Input const& input,
    hand::ObjectIdentity const& asset,gears_vr::StereoMatrix const& base,std::int64_t now,bool allow_sim,
    Workspace& work,body::Output& inout,adapter::MountKind right_mount=adapter::MountKind::free_hand) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&input,sizeof input),range(&asset,sizeof asset),
        range(&base,sizeof base),range(&work,sizeof work),range(&inout,sizeof inout)};
    if(!disjoint(ranges,7))return detail::fail(Status::alias);
    if(gears_body_tick_source::classify(input,input.observed.parent_scene,now,allow_sim)!=gears_body_tick_source::Verdict::okay)return detail::fail(Status::invalid_core);
    return compose_common(model,frame,input.observed.parent_scene,asset,base,now,allow_sim,work,inout,right_mount,
        [&](adapter::ModelBinding const& binding,unsigned side){
            const auto result=adapter::prepare_hand(input,binding,static_cast<adapter::Side>(side),base,now,allow_sim,work.source_targets[side]);
            if(result==adapter::Status::okay)work.computed_wrist[side]=work.source_targets[side].wrist_world;
            return result;
        });
}

} // namespace gears_body_c0_hands
