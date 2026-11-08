#pragma once
#include "../vr-common/weapon_two_hand_hold.h"
#include "full_body_c0_planner.h"
#include "hand_core_target_adapter.h"
#include "../vr-common/free_hand_eye_targets.h"
#include "../vr-common/hand_shoulder_pose.h"
#include "../vr-common/hand_finger_pose.h"
#include "../vr-common/hand_object_grasp.h"

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
    gears_vr::Pose physical_wrist[2]{}; // Same fresh target with fixed-arm reach, before cosmetic fitting.
    two::Input inputs[2]{};two::Solution solutions[2]{};
    gears_hand_shoulder::Solution shoulder_solutions[2]{};
    double finger_closure[2]{},finger_pinch[2]{}; // Current raw inputs; cosmetic only.
    gears_hand_fingers::PoseBlend finger_blend[2]{};
    gears_hand_object_grasp::Target left_visual{}; // Caller-authenticated cosmetic goal only.
    gears_hand_object_grasp::BlendState left_blend{};
    gears_hand_object_grasp::BlendState right_blend{};
    std::array<std::uint64_t,16> grasp_context{};
    bool left_visual_applied{};
    std::array<std::uint64_t,2> finger_available{{UINT64_MAX,UINT64_MAX}}; // Owned full model by default; live caller supplies current LOD DesiredBones.
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
inline bool fingers(body::Model const& model,body::Workspace& w,unsigned side,double amount,
    std::array<std::uint64_t,2> const& available,gears_hand_fingers::Shape shape=gears_hand_fingers::Shape::free,
    double shape_weight=1.,double trigger=0.,gears_hand_fingers::PoseBlend* blend=nullptr) noexcept {
    if(side>1||!std::isfinite(amount)||amount<0||amount>1)return false;
    // Five three-joint finger branches only. Preserve27/28 and51/52 weapon
    // helpers, forearm helpers, all translations/scales and the wrist itself.
    const unsigned first=side?36u:12u,wrist=side?35u:11u;
    for(unsigned n=0;n<15;++n){const unsigned i=first+n;
        if(model.parents[i]!=int(n%3?i-1:wrist))return false;
        if(!(available[i/64]&(std::uint64_t(1)<<(i%64))))continue;
        if(amount==0&&!blend){std::memcpy(w.pose.local[i].q,model.reference[i].q,16);continue;}
        auto const& open=model.reference[i].q;body::Quat q{};
        const body::Quat reference{open[0],open[1],open[2],open[3]};
        if(!gears_hand_fingers::rotation(side,n,amount,reference,q,shape,shape_weight,trigger)||
           (blend&&!blend->joint(n,reference,q,q)))return false;
        if(q.x==reference.x&&q.y==reference.y&&q.z==reference.z&&q.w==reference.w)
            std::memcpy(w.pose.local[i].q,open,16);
        else if(!body::d::set_quat(w.pose.local[i],q))return false;
    }
    return body::d::rebuild(model,w);
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
    work.left_visual_applied=false;
    if(right_mount!=adapter::MountKind::free_hand&&right_mount!=adapter::MountKind::held_lancer&&right_mount!=adapter::MountKind::held_frag)return detail::fail(Status::binding,1,adapter::Status::unknown_mount);
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
        if(!unit_animation_scale(frame.local[i].scale)||std::memcmp(&frame.local[i].scale,&inout.local[i].scale,4)||
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
    bool side_prepared[2]{};
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
        side_prepared[side]=true;
    }
    gears_hand_object_grasp::Target visual_goals[2]{work.left_visual,{}};
    if(work.left_visual.kind==gears_hand_object_grasp::Kind::support&&!work.left_visual.passive_socket){
        visual_goals[0]={};gears_weapon_two_hand::Solution support{};
        if(independent&&side_prepared[0]&&side_prepared[1]){
            visual_goals[0]={gears_hand_object_grasp::Kind::support,
                {work.eye_targets[0].grip_world.position,gears_vr::normalized(gears_vr::multiply(
                    work.eye_targets[1].grip_world.orientation,work.left_visual.grip.orientation))},1.};
        }
        if(independent&&side_prepared[0]&&side_prepared[1]&&right_mount==adapter::MountKind::held_lancer&&
           gears_weapon_two_hand::solve(work.eye_targets[1].grip_world,work.eye_targets[0].grip_world,
               work.left_visual.grip,frame.units_per_metre,support)){
            visual_goals[0]={gears_hand_object_grasp::Kind::support,support.left,1.};
            visual_goals[1]={gears_hand_object_grasp::Kind::support,support.right,1.};
        }
    }
    for(unsigned side=0;side<2;++side){
        if(!side_prepared[side])continue;
        auto& b=work.bindings[side];
        if(independent){
            // Upper-arm deformation helpers9/33 are a separate skin branch
            // from the elbow. Updating only the three joint centres leaves
            // the old aiming-animation twist in the visible biceps, even
            // when every measured wrist/segment is correct. Restore their
            // authored local rest relation under the newly solved arm; no
            // translation/scale, target, or controller orientation changes.
            const unsigned helper=side?33u:9u;
            if(model.parents[helper]!=int(b.authored_mount.chain[0]))return detail::fail(Status::reference,side);
            std::memcpy(w.pose.local[helper].q,model.reference[helper].q,16);
            if(!body::d::rebuild(model,w))return detail::fail(Status::numerical,side);
        }
        // Fixed-length solve. The current body-only C0 elbow is an explicit
        // bend pole; the shared solver falls back to the shoulder frame if
        // collinear. No previous-frame state or animated-wrist marker fitting.
        const auto& chain=b.authored_mount.chain;auto& input=work.inputs[side];input={};
        input.shoulder=body::d::pose(w.pose.world[chain[0]]);input.elbow=body::d::pose(w.pose.world[chain[1]]);input.wrist=body::d::pose(w.pose.world[chain[2]]);
        input.target=work.computed_wrist[side];input.authored={true,input.elbow.position};input.lengths.mode=two::LengthMode::preserve;
        const auto reachable_target=[&](body::Pose& target){
            const double upper=length(body::d::sub(input.elbow.position,input.shoulder.position));
            const double lower=length(body::d::sub(input.wrist.position,input.elbow.position));
            const auto delta=body::d::sub(target.position,input.shoulder.position);
            const double distance=length(delta),minimum=std::abs(upper-lower)+.001,maximum=upper+lower-.001;
            if(!std::isfinite(distance)||distance<1e-6||maximum<=minimum)return false;
            const double reachable=(std::max)(minimum,(std::min)(distance,maximum));
            if(reachable!=distance)target.position=body::d::add(input.shoulder.position,mul(delta,reachable/distance));
            return true;
        };
        work.physical_wrist[side]=work.computed_wrist[side];
        if(independent&&!reachable_target(work.physical_wrist[side]))return detail::fail(Status::numerical,side);
        if(independent){
            // Preserve computed_wrist/eye_targets as the physical observation.
            // Only the existing single C0 skin solve receives the visual pose.
            auto adjusted=input.target;
            auto& blend=side?work.right_blend:work.left_blend;
            if(blend.update(visual_goals[side],work.eye_targets[side].grip_world,
               b.authored_mount.calibration,frame.units_per_metre,now,adjusted)){
                input.target=adjusted;if(!side)work.left_visual_applied=true;
            }
        }
        input.share_forearm_twist=true;
        if(independent){
            // Marcus's wrist +X is the longitudinal hand axis. Choose a
            // feasible elbow before allowing the authored pole to fold the
            // wrist back. Preserve full tracked orientation and fixed lengths.
            // A sustained45-degree bend was visible while simply holding the
            // recorded rifle. Prefer a straighter hand/forearm relation by
            // moving the elbow on its fixed-length circle, never by changing
            // the tracked wrist rotation or stretching either arm segment.
            input.preferred_wrist_bend=.34906585039886590; // 20-degree preference.
            input.max_wrist_pole_turn=1.04719755119659775; // At most 60 degrees from body pole.
            // Original aiming animations can place the opposite elbow inward
            // and above the wrist. Use the coherent body's outward/downward
            // pole for independent hands, not that animated elbow. Wrist
            // orientation may adjust it only inside the bounded arc above.
            const auto outward=body::d::sub(inout.shoulders_world[side].position,inout.shoulders_world[1-side].position);
            const auto up=row(w.component,2);
            const double width=length(outward),up_length=length(up);
            if(width<1e-6||up_length<1e-6)return detail::fail(Status::numerical,side);
            input.authored.point=body::d::add(input.shoulder.position,
                body::d::sub(mul(outward,.35/width),mul(up,1/up_length)));
            // A real controller can exceed this avatar's fixed arm span. Keep
            // its full rotation and project only the endpoint onto the arm's
            // reachable shell; never stretch bones or move the camera/body.
            if(!reachable_target(input.target))return detail::fail(Status::numerical,side);
        }
        auto solved=two::Status::invalid_input;
        if(independent){
            const unsigned clavicle=side?30u:6u;
            if(model.parents[chain[0]]!=int(clavicle))return detail::fail(Status::reference,side);
            gears_hand_shoulder::Input shoulder_input{};shoulder_input.arm=input;
            shoulder_input.clavicle=body::d::pose(w.pose.world[clavicle]);shoulder_input.up=row(w.component,2);
            shoulder_input.outward=body::d::sub(inout.shoulders_world[side].position,inout.shoulders_world[1-side].position);
            shoulder_input.units_per_metre=frame.units_per_metre;
            solved=gears_hand_shoulder::solve(shoulder_input,work.shoulder_solutions[side]);
            if(solved==two::Status::solved){
                auto const& shoulder=work.shoulder_solutions[side];work.solutions[side]=shoulder.arm;
                if(shoulder.adjusted&&!body::d::world_rotation(model,w,clavicle,shoulder.clavicle.orientation))
                    return detail::fail(Status::numerical,side);
            }
        }else solved=two::solve(input,work.solutions[side]);
        if(solved!=two::Status::solved)return detail::fail(solved==two::Status::unreachable?Status::unreachable:Status::numerical,side,
            solved==two::Status::unreachable?adapter::Status::unreachable:adapter::Status::numerical_failure);
        const body::Pose target[]={work.solutions[side].shoulder,work.solutions[side].elbow,work.solutions[side].wrist};
        for(unsigned i=0;i<3;++i)if(!body::d::world_rotation(model,w,chain[i],target[i].orientation))return detail::fail(Status::numerical,side);
        if(!detail::pose_matches(w.pose.world[chain[2]],input.target))return detail::fail(Status::residual,side);
        // All current Lancer contacts go through this one finger writer.
        // Main-hand holding has its own wrap, independent of stock animation
        // and of whether the user keeps squeezing after acquiring the gun.
        auto shape=side==0&&work.left_blend.kind!=gears_hand_object_grasp::Kind::none?
            static_cast<gears_hand_fingers::Shape>(work.left_blend.kind):gears_hand_fingers::Shape::free;
        double closure=work.finger_closure[side];
        if(side==0&&work.left_visual.kind!=gears_hand_object_grasp::Kind::none&&shape!=gears_hand_fingers::Shape::free)
            closure=1.; // The admitted grasp, not button type, selects its preset.
        else if(side==0&&shape==gears_hand_fingers::Shape::magazine)
            closure=std::max(work.finger_pinch[side],closure);
        else if(side==0&&shape==gears_hand_fingers::Shape::loading)
            closure=work.finger_pinch[side];
        else if(side==0&&shape==gears_hand_fingers::Shape::free&&work.finger_pinch[side]>closure){
            shape=gears_hand_fingers::Shape::loading;closure=work.finger_pinch[side];
        }
        if(side&&right_mount==adapter::MountKind::held_lancer){shape=gears_hand_fingers::Shape::weapon;closure=1.;}
#if defined(GEARS_PRIVATE_MAIN_GRIP_AB) && GEARS_PRIVATE_MAIN_GRIP_AB
        // Private recorded A/B only. Same C0 writer, fixed bone translations,
        // original tracked squeeze and ownership. Never an action/grasp grant.
        // Production held-grip closure above remains independent of squeeze.
        if(side&&right_mount==adapter::MountKind::held_lancer)closure=work.finger_closure[side];
#endif
        if(side&&right_mount==adapter::MountKind::held_frag){shape=gears_hand_fingers::Shape::grenade;closure=1.;}
        if(independent&&(!work.finger_blend[side].begin(now)||
           !detail::fingers(model,w,side,closure,work.finger_available,shape,
               side==0&&work.left_visual_applied?work.left_blend.weight:1.,work.finger_pinch[side],&work.finger_blend[side])))
            return detail::fail(Status::numerical,side);
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
namespace detail {
inline Result compose_eye_current(body::Model const& model,body::Frame const& frame,
    gears_body_tick_input::Input const& input,std::uint64_t compose,
    hand::ObjectIdentity const& asset,gears_vr::StereoMatrix const& base,
    gears_vr::Pose const& desired_eye,std::int64_t now,bool allow_sim,
    Workspace& work,body::Output& inout,adapter::MountKind right_mount=adapter::MountKind::free_hand) noexcept {
    namespace free=gears_free_hand_eye_targets;
    auto const& s=input.parent_scene;auto const& t=input.stereo;auto const& c=input.controllers;
    const std::array<std::uint64_t,16> grasp_context{{s.native.game_start,s.native.native_started_qpc,
        s.world_generation,s.mesh.address,t.session,t.space_epoch,input.reference_revision,
        c.input_epoch,c.profile_generation,input.continuity.tracking_epoch[0],input.continuity.tracking_epoch[1],
        s.weapon.address,s.weapon.registry_index,s.weapon.name_index,s.weapon.name_number,std::uint64_t(right_mount)}};
    // Both hands now participate in support fitting. A recovered right-hand
    // stream or a different held weapon must not inherit a skin offset from
    // the previous grasp; no old offset becomes current tracking/action data.
    if(work.grasp_context!=grasp_context){work.left_blend={};work.right_blend={};work.finger_blend[0]={};work.finger_blend[1]={};work.grasp_context=grasp_context;}
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
    for(unsigned side=0;side<2;++side){
        work.finger_closure[side]=c.hands[side].squeeze_active?c.hands[side].squeeze:0;
        work.finger_pinch[side]=c.hands[side].trigger_active?c.hands[side].trigger:0;
    }
    return compose_common(model,frame,s,asset,base,now,allow_sim,work,inout,right_mount,
        [&](adapter::ModelBinding const& binding,unsigned side){
            auto const result=free::prepare_one(work.eye_frame,work.tracking_frame,side,
                binding.authored_mount.calibration,frame.units_per_metre,work.eye_targets[side]);
            if(result){work.computed_wrist[side]=work.eye_targets[side].wrist_world;return adapter::Status::okay;}
            return result.hand==free::HandStatus::untracked?adapter::Status::hand_untracked:adapter::Status::numerical_failure;
        },true);
}
} // namespace detail
inline Result compose_eye(body::Model const& model,body::Frame const& frame,
    gears_body_tick_input::Input const& input,std::uint64_t compose,
    hand::ObjectIdentity const& asset,gears_vr::StereoMatrix const& base,
    gears_vr::Pose const& desired_eye,std::int64_t now,bool allow_sim,
    Workspace& work,body::Output& inout,adapter::MountKind right_mount=adapter::MountKind::free_hand) noexcept {
    if(gears_body_tick_input::classify(input,input.parent_scene,now,allow_sim)!=gears_body_tick_input::Verdict::okay)
        return detail::fail(Status::invalid_core);
    return detail::compose_eye_current(model,frame,input,compose,asset,base,desired_eye,now,allow_sim,work,inout,right_mount);
}
inline Result compose_eye(body::Model const& model,body::Frame const& frame,
    gears_body_tick_source::Input const& input,std::uint64_t compose,
    hand::ObjectIdentity const& asset,gears_vr::StereoMatrix const& base,
    gears_vr::Pose const& desired_eye,std::int64_t now,bool allow_sim,
    Workspace& work,body::Output& inout,adapter::MountKind right_mount=adapter::MountKind::free_hand) noexcept {
    // Keep the distinct source contract. Its current tracking ticket remains
    // bounded to40ms and its authenticated scene to50ms. Old Scene tracking
    // is metadata here; erasing this type into legacy TickInput re-applied
    // that consumer's extra stale-tracking checks to a different contract.
    if(gears_body_tick_source::classify(input,input.observed.parent_scene,now,allow_sim)!=gears_body_tick_source::Verdict::okay)
        return detail::fail(Status::invalid_core);
    return detail::compose_eye_current(model,frame,input.observed,compose,asset,base,desired_eye,now,allow_sim,work,inout,right_mount);
}
inline Result compose(body::Model const& model,body::Frame const& frame,core::CoreSnapshot const& source,
    hand::ObjectIdentity const& asset,gears_vr::StereoMatrix const& base,std::int64_t now,bool allow_sim,
    Workspace& work,body::Output& inout,adapter::MountKind right_mount=adapter::MountKind::free_hand) noexcept {
    using namespace body::d;
    const Range ranges[]={range(&model,sizeof model),range(&frame,sizeof frame),range(&source,sizeof source),range(&asset,sizeof asset),
        range(&base,sizeof base),range(&work,sizeof work),range(&inout,sizeof inout)};
    if(!disjoint(ranges,7))return detail::fail(Status::alias);
    if(right_mount!=adapter::MountKind::free_hand&&right_mount!=adapter::MountKind::held_lancer&&right_mount!=adapter::MountKind::held_frag)return detail::fail(Status::binding,1,adapter::Status::unknown_mount);
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
