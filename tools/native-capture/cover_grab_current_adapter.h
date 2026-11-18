#pragma once
#include "body_tick_source.h"
#include "full_body_c0_planner.h"
#include "hand_core_target_adapter.h"
#include "../vr-common/cover_grab_pose.h"
#include "../vr-common/cover_input_owner.h"
#include "../vr-common/free_hand_eye_targets.h"

// Pure association of a committed Scene owner snapshot with a newer current V3
// frame. The owner still owns the logical edge; this converts tracking only.
namespace gears_cover_grab_current {
namespace source=gears_body_tick_source;
namespace grab=gears_cover_grab_pose;
namespace free=gears_free_hand_eye_targets;
namespace vr=gears_vr;
enum class Status {prepared,alias,source,binding,hand,geometry};

inline Status prepare(source::Input const& current,grab::mount::Signature signature,
    double model_scale,vr::CoverInputGrabSnapshot const& snapshot,
    vr::LeftInteractionControl selected_control,grab::Key const& expected,free::EyeFrame const& eye,
    std::uint64_t compose,grab::Mode mode,std::int64_t now,bool allow_sim,grab::Frame& out) noexcept {
    using namespace gears_full_body_c0::d;
    const Range ranges[]={range(&current,sizeof current),range(&signature,sizeof signature),
        range(&snapshot,sizeof snapshot),range(&selected_control,sizeof selected_control),
        range(&expected,sizeof expected),range(&eye,sizeof eye),range(&compose,sizeof compose),
        range(&mode,sizeof mode),range(&now,sizeof now),range(&out,sizeof out)};
    if(!disjoint(ranges,10))return Status::alias;
    auto const& t=current.observed;auto const& s=t.parent_scene;auto const& c=t.controllers;
    auto const& parent=s.controllers;auto const& old=parent.hands[0];auto const& hand=c.hands[0];
    if(source::classify(current,s,now,allow_sim)!=source::Verdict::okay)return Status::source;
    if(!(s.flags&gears_rig_tracking::controllers_matched)||
       !(s.flags&gears_rig_tracking::producer_continuity_available))return Status::source;
    if(!snapshot.lifecycle||snapshot.frame!=s.scene_invocation||snapshot.frame!=snapshot.cover.sequence)
        return Status::binding;
    if(!grab::valid(expected)||!grab::same(expected.epochs,snapshot.cover.epochs)||
       snapshot.hand.hand.session_epoch!=expected.epochs.session||
       snapshot.hand.hand.space_epoch!=expected.epochs.space)return Status::binding;
    if(snapshot.hand.hand.sequence!=parent.sequence||
       snapshot.hand.hand.observed_ns!=parent.received_ns||
       snapshot.hand.hand.left_position_metres.x!=old.grip.position.x||
       snapshot.hand.hand.left_position_metres.y!=old.grip.position.y||
       snapshot.hand.hand.left_position_metres.z!=old.grip.position.z)return Status::hand;
    if((snapshot.hand.status!=vr::CoverControllerStatus::accepted&&
        snapshot.hand.status!=vr::CoverControllerStatus::baseline)||!snapshot.hand.continuity_generation||
       !snapshot.hand.hand.input_active||!snapshot.hand.hand.position_valid||
       !snapshot.hand.hand.position_tracked)return Status::hand;
    if(!grab::fresh(snapshot.cover.observed_ns,now)||!grab::fresh(snapshot.hand.hand.observed_ns,now)||
       !snapshot.cover.cover_valid||!snapshot.cover.interaction_enabled)return Status::binding;
    if(!(s.flags&gears_rig_tracking::cover_key_known)||!s.cover_type||
       !gears_weapon_hand::identity_valid(s.cover_link)||s.cover_slot<0)return Status::binding;
    if(selected_control!=vr::LeftInteractionControl::grip)return Status::hand;
    if(c.source!=parent.source||c.session!=parent.session||c.space_epoch!=parent.space_epoch||
       c.input_epoch!=parent.input_epoch||c.profile_generation!=parent.profile_generation||
       c.sequence<parent.sequence||t.continuity.stream_epoch!=s.producer.stream||
       t.continuity.tracking_epoch[0]!=s.producer.tracking[0])return Status::source;
    if(hand.pressed[vr::control_squeeze]!=old.pressed[vr::control_squeeze]||
       hand.released[vr::control_squeeze]!=old.released[vr::control_squeeze])return Status::hand;
    if(!hand.squeeze_active||!vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active))
        return Status::hand;
    if(snapshot.hand.hand.lgrip_down&&(old.squeeze==0.f||hand.squeeze==0.f))return Status::hand;
    if((mode==grab::Mode::approach||mode==grab::Mode::held)&&snapshot.cover.edge.id!=expected.edge)
        return Status::binding;
    if(mode!=grab::Mode::none&&mode!=grab::Mode::approach&&mode!=grab::Mode::held&&
       mode!=grab::Mode::release)return Status::binding;
    if(mode==grab::Mode::held){
        const auto request=snapshot.interaction.request_id;
        if(!snapshot.hand.hand.lgrip_down||!snapshot.interaction.offset_certified||
           snapshot.interaction.phase!=vr::CoverPhase::grabbing||
           snapshot.interaction.edge_id!=expected.edge||
           snapshot.ownership.owner!=vr::CoverLeftHandOwner::cover||
           !request||snapshot.ownership.request_id!=request||
           !snapshot.interaction.owns_left_hand||snapshot.lease.kind!=vr::LeftHandInteraction::cover||
           snapshot.lease.id!=request||snapshot.lease.lifecycle!=snapshot.lifecycle)
            return Status::binding;
    }else if(mode==grab::Mode::none||mode==grab::Mode::release){
        if(snapshot.ownership.owner!=vr::CoverLeftHandOwner::free||snapshot.ownership.request_id||
           snapshot.hand.hand.lgrip_down||snapshot.interaction.owns_left_hand||
           snapshot.interaction.request_left_hand||snapshot.lease.kind!=vr::LeftHandInteraction::none||
           snapshot.lease.lifecycle||snapshot.lease.id)return Status::binding;
    }
    auto const& k=eye.key;
    if(k.game_pid!=s.native.game_pid||k.game_thread!=s.native.game_thread||
       k.game_start!=s.native.game_start||k.native_started_qpc!=s.native.native_started_qpc||
       k.world_generation!=s.world_generation||k.mesh!=s.mesh.address||
       k.binding_generation!=s.binding_generation||k.tick!=t.tick||k.compose!=compose||
       k.reference_revision!=t.reference_revision||k.source!=t.stereo.source||
       k.stereo_session!=t.stereo.session||k.stereo_sequence!=t.stereo.sequence||
       k.space_epoch!=t.stereo.space_epoch||k.controller_sequence!=c.sequence||
       k.input_epoch!=c.input_epoch||k.profile_generation!=c.profile_generation||
       k.predicted_time!=t.stereo.predicted_time)return Status::binding;
    free::TrackingFrame tracking{k,t.stereo,c};
    if(free::frame_status(eye,tracking,s.units_per_metre)!=free::Status::okay)return Status::binding;
    grab::mount::Mount mount{};free::HandTarget physical{};
    if(!grab::mount::marcus115(signature,grab::mount::Side::left,model_scale,s.units_per_metre,mount)||
       !free::prepare_one(eye,tracking,0,mount.calibration,s.units_per_metre,physical))
        return Status::geometry;
    grab::Frame next{};next.expected=expected;next.cover=snapshot.cover;
    next.hand=snapshot.hand.hand;next.hand.sequence=c.sequence;next.hand.observed_ns=c.received_ns;
    next.hand.left_position_metres=hand.grip.position;
    if(mode==grab::Mode::none||mode==grab::Mode::release)next.hand.lgrip_down=false;
    next.interaction=snapshot.interaction;
    next.ownership=snapshot.ownership;next.physical_grip_world=physical.grip_world;
    next.actual_geometry_signature=signature;next.model_scale=model_scale;
    next.units_per_metre=s.units_per_metre;next.now_ns=now;next.mode=mode;
    out=next;return Status::prepared;
}
} // namespace gears_cover_grab_current
