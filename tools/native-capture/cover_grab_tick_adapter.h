#pragma once
#include "body_goal_view_adapter.h"
#include "../vr-common/cover_controller_adapter.h"
#include "../vr-common/cover_grab_pose.h"
#include "../vr-common/free_hand_eye_targets.h"
#include "cover_grab_pose_adapter.h"

// Pure conversion of this already acquired C0 input. No new controller reader,
// edge detector, arbiter, timestamp, world registration or Native permission.
namespace gears_cover_grab_tick {
namespace body=gears_body_goal_view;
namespace source=gears_body_tick_source;
namespace grab=gears_cover_grab_pose;
namespace free=gears_free_hand_eye_targets;
namespace vr=gears_vr;
enum class Status {prepared,alias,source,binding,hand,geometry};
inline Status prepare(source::Input const& input,body::CoverPlacement const& placement,
    free::EyeFrame const& eye,grab::Key const& expected,vr::CoverObservation const& cover,
    vr::CoverControllerResult const& selected_hand,vr::CoverResult const& interaction,
    vr::CoverOwnership const& ownership,grab::mount::Signature const& signature,
    grab::Mode mode,std::int64_t now,bool allow_sim,grab::Frame& out) noexcept {
    using namespace gears_full_body_c0::d;
    const Range ranges[]={range(&input,sizeof input),range(&placement,sizeof placement),range(&eye,sizeof eye),
        range(&expected,sizeof expected),range(&cover,sizeof cover),range(&selected_hand,sizeof selected_hand),
        range(&interaction,sizeof interaction),range(&ownership,sizeof ownership),range(&signature,sizeof signature),range(&out,sizeof out)};
    if(!disjoint(ranges,10))return Status::alias;
    auto const& t=input.observed;auto const& s=t.parent_scene;auto const& c=t.controllers;
    if(source::classify(input,s,now,allow_sim)!=source::Verdict::okay)return Status::source;
    body::Policy policy{};policy.now_ns=now;policy.allow_sim=allow_sim;policy.scope=gears_full_body_c0::Scope::fixed_cover;
    if(!gears_weapon_hand::identity_valid(s.cover_link)||!grab::valid(expected)||!grab::same(expected.epochs,cover.epochs)||
       body::cover_placement_current(placement,input,policy)!=body::Status::okay||
       cover.observed_ns!=placement.observed_ns)return Status::binding;
    auto const& k=eye.key;
    if(k.game_pid!=s.native.game_pid||k.game_thread!=s.native.game_thread||k.game_start!=s.native.game_start||
       k.native_started_qpc!=s.native.native_started_qpc||k.world_generation!=s.world_generation||
       k.mesh!=s.mesh.address||k.binding_generation!=s.binding_generation||k.tick!=t.tick||!k.compose||
       k.reference_revision!=t.reference_revision)return Status::binding;
    free::TrackingFrame tracking{k,t.stereo,c};
    if(free::frame_status(eye,tracking,s.units_per_metre)!=free::Status::okay)return Status::binding;
    auto const& hand=selected_hand.hand;auto const& raw=c.hands[0];
    if((selected_hand.status!=vr::CoverControllerStatus::accepted&&selected_hand.status!=vr::CoverControllerStatus::baseline)||
       !selected_hand.continuity_generation||hand.sequence!=c.sequence||hand.observed_ns!=c.received_ns||
       hand.session_epoch!=expected.epochs.session||hand.space_epoch!=expected.epochs.space||
       !hand.input_active||!hand.position_valid||!hand.position_tracked||
       hand.left_position_metres.x!=raw.grip.position.x||hand.left_position_metres.y!=raw.grip.position.y||
       hand.left_position_metres.z!=raw.grip.position.z||raw.squeeze_active!=1)return Status::hand;
    // lgrip_down is the existing controller adapter's hysteresis/edge result.
    // Do not replace it with a weapon-specific analog threshold or synthesize
    // another press detector. The live owner supplies this exact selected result.
    grab::mount::Mount mount{};free::HandTarget physical{};
    if(!grab::mount::marcus115(signature,grab::mount::Side::left,1,s.units_per_metre,mount)||
       !free::prepare_one(eye,tracking,0,mount.calibration,s.units_per_metre,physical))return Status::geometry;
    grab::Frame next{};next.expected=expected;next.cover=cover;next.hand=hand;
    next.interaction=interaction;next.ownership=ownership;next.physical_grip_world=physical.grip_world;
    next.actual_geometry_signature=signature;next.model_scale=1;next.units_per_metre=s.units_per_metre;
    next.now_ns=now;next.mode=mode;
    out=next;return Status::prepared;
}
struct Workspace {
    grab::Frame frame{};
    gears_cover_grab_pose_adapter::Workspace pose{};
};
struct Result {
    Status input{Status::prepared};
    gears_cover_grab_pose_adapter::Result pose{};
};
// One owned-buffer entry for the existing live C0 owner. Source conversion,
// transition and fixed-length/LOD solve either produce one complete candidate
// or leave inout unchanged. No game reads, publication or commit occurs here.
inline Result compose(source::Input const& input,body::CoverPlacement const& placement,
    free::EyeFrame const& eye,grab::Key const& expected,vr::CoverObservation const& cover,
    vr::CoverControllerResult const& selected_hand,vr::CoverResult const& interaction,
    vr::CoverOwnership const& ownership,grab::Contact const& contact,grab::Profile const& profile,
    gears_full_body_c0::Model const& model,gears_full_body_c0::Frame const& frame,
    std::array<std::uint64_t,2> const& desired_bones,grab::Mode mode,std::int64_t now,bool allow_sim,
    gears_cover_grab_transition::State& state,Workspace& work,gears_full_body_c0::Output& inout) noexcept {
    using namespace gears_full_body_c0::d;
    namespace pose=gears_cover_grab_pose_adapter;
    const Range ranges[]={range(&input,sizeof input),range(&placement,sizeof placement),range(&eye,sizeof eye),
        range(&expected,sizeof expected),range(&cover,sizeof cover),range(&selected_hand,sizeof selected_hand),
        range(&interaction,sizeof interaction),range(&ownership,sizeof ownership),range(&contact,sizeof contact),
        range(&profile,sizeof profile),range(&model,sizeof model),range(&frame,sizeof frame),
        range(&desired_bones,sizeof desired_bones),range(&state,sizeof state),range(&work,sizeof work),range(&inout,sizeof inout)};
    if(!disjoint(ranges,16))return {Status::alias,{pose::Status::alias}};
    const auto converted=prepare(input,placement,eye,expected,cover,selected_hand,interaction,ownership,
        model.signature,mode,now,allow_sim,work.frame);
    if(converted!=Status::prepared){state={};return {converted,{pose::Status::inactive}};}
    const auto result=pose::compose_transition(model,frame,work.frame,contact,profile,desired_bones,state,work.pose,inout);
    return {Status::prepared,result};
}
} // namespace gears_cover_grab_tick
