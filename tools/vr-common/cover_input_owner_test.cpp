#include "cover_input_owner.h"
#include <cstdio>
#include <utility>
using namespace gears_vr;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
bool close(double a,double b){return std::abs(a-b)<1e-8;}
struct Fixture {
    CoverInputOwner kernel;LeftHandInteractionOwner owner;ControllerEdgeCounter edges;
    ControllerInputFrame hands;CoverInputOwnerFrame frame;CoverInputOwnerResult result;
    CoverHeightKey curve[3]{{0,0},{.5,.3},{1,.7}};
    Fixture(CoverEdgeKind kind=CoverEdgeKind::top){
        frame.lifecycle=1;frame.frame=1;frame.now_ns=1000000000;frame.units_per_metre=100;
        frame.cover.sequence=1;frame.cover.observed_ns=frame.now_ns;frame.cover.epochs={1,1,1,1,1,1,1};
        frame.cover.cover_valid=frame.cover.interaction_enabled=frame.owner_context_valid=true;
        frame.cover.range={-.6,.5,0,.7};frame.cover.edge={17,kind,true,true};
        frame.anchor={{1000,-2000,-40},{1,0,0},{0,1,0}};
        frame.height_curve=curve;frame.height_key_count=3;
        frame.stereo.status=StereoStatus::accepted;frame.stereo.reference_generation=1;
        frame.stereo.reference={{0,1.6,0},{}};
        auto& s=frame.stereo.sample;s.session=41;s.sequence=1;s.space_epoch=2;
        s.focused=1;s.visible=1;s.should_render=1;s.view_flags=s.head_flags=15;
        s.head={{0,1.6,0},{}};s.eyes[0].pose={{-.032,1.6,0},{}};s.eyes[1].pose={{.032,1.6,0},{}};
        for(auto& eye:s.eyes)eye.fov={-.7,.7,.7,-.7};
        for(auto& hand:hands.hands){hand.grip_active=1;hand.grip_flags=15;hand.squeeze_active=1;}
        hands.hands[0].grip.position={.2,1.2,-.4};frame.controllers=&hands;publish();
    }
    void publish(){
        auto& s=frame.stereo.sample;s.received_ns=frame.now_ns;s.predicted_time=frame.now_ns+20000000;
        hands.source=s.source;hands.session=s.session;hands.sequence=s.sequence;hands.space_epoch=s.space_epoch;
        hands.focused=1;hands.received_ns=frame.now_ns;hands.sync_ns=frame.now_ns-1000;hands.predicted_time=s.predicted_time;
        edges.update(hands);C(controller_frame_shape_valid(hands));
    }
    CoverInputOwnerResult run(){result=kernel.update(frame,owner);return result;}
    void advance(float squeeze){
        ++frame.frame;++frame.cover.sequence;++frame.stereo.sample.sequence;frame.now_ns+=10000000;
        frame.cover.observed_ns=frame.now_ns;hands.hands[0].squeeze=squeeze;publish();
    }
    CoverInputOwnerResult next(float squeeze){advance(squeeze);return run();}
};
int main(){
    Fixture f;auto r=f.run();C(r.accepted&&r.view_valid&&r.body_weight_valid&&r.requested_up_weight==0);
    C(r.left_hand.status==LeftHandInteractionStatus::released);
    r=f.next(.8f);C(r.interaction.owns_left_hand&&r.left_hand.new_grant&&r.left_hand.lease.kind==LeftHandInteraction::cover);
    C(close(r.interaction.offset.up,0));
    f.hands.hands[0].grip.position.y=.9;r=f.next(.8f);
    C(r.interaction.owns_left_hand&&r.left_hand.status==LeftHandInteractionStatus::retained);
    C(close(r.interaction.offset.up,.3)&&close(r.view.base_eye_world.z,-10)&&close(r.requested_up_weight,.5));
    r=f.next(0);C(!r.interaction.owns_left_hand&&r.left_hand.lease.kind==LeftHandInteraction::none);
    C(close(r.interaction.offset.up,.3));
    f.hands.hands[0].grip.position={2,3,4};r=f.next(0);C(close(r.interaction.offset.up,.3));
    // Physical standing remains possible after releasing the grip. Its height
    // changes the BODY target, while the camera BASE still has only .3 virtual.
    f.frame.stereo.sample.head.position.y+=.2;
    for(auto& eye:f.frame.stereo.sample.eyes)eye.pose.position.y+=.2;
    r=f.next(0);C(close(r.physical.up,.2)&&close(r.requested_up_metres,.5));
    C(close(r.view.base_eye_world.z,-10)&&close(r.requested_up_weight,.75));
    // A true fixed-position head rotation does not change stance or base eye.
    f.frame.stereo.sample.head.orientation={std::sin(.3),0,0,std::cos(.3)};
    for(auto& eye:f.frame.stereo.sample.eyes)eye.pose.orientation=f.frame.stereo.sample.head.orientation;
    r=f.next(0);C(close(r.requested_up_metres,.5)&&close(r.view.base_eye_world.z,-10));
    // Re-grab starts where the offset is held, then reverse motion retracts.
    r=f.next(.8f);C(r.interaction.owns_left_hand&&close(r.interaction.offset.up,.3));
    f.hands.hands[0].grip.position.y+=.15;r=f.next(.8f);C(close(r.interaction.offset.up,.15));
    f.frame.controllers=nullptr;r=f.next(.8f);
    C(!r.interaction.owns_left_hand&&r.view_valid&&close(r.interaction.offset.up,.15));
    f.frame.controllers=&f.hands;r=f.next(.8f);C(!r.interaction.owns_left_hand); // Held recovery cannot grab.
    f.next(0);r=f.next(.8f);C(r.interaction.owns_left_hand);
    C(!f.run().accepted); // A rendering retry does not arbitrate the hand again.
    // The valid cover request wins over NEW weapon proposals in the one owner.
    Fixture priority;priority.frame.weapon_requests.support=true;priority.run();r=priority.next(.8f);
    C(r.interaction.owns_left_hand&&r.left_hand.lease.kind==LeftHandInteraction::cover);
    // A committed weapon lease is preserved and never doubles as a cover grip.
    Fixture occupied;occupied.run();occupied.frame.weapon_incumbents[0]={LeftHandInteraction::support,1,91};
    r=occupied.next(.8f);C(!r.interaction.owns_left_hand&&r.left_hand.lease.kind==LeftHandInteraction::support);
    // Outside cover, the same global owner still grants ordinary weapon input.
    Fixture outside;outside.frame.cover.cover_valid=false;outside.frame.weapon_requests.ammo=true;
    outside.run();r=outside.next(.8f);C(r.accepted&&!r.view_valid&&r.left_hand.lease.kind==LeftHandInteraction::ammo);
    // Both lateral directions use the fixed tracking right axis.
    for(auto edge:{CoverEdgeKind::left,CoverEdgeKind::right}){
        Fixture lateral(edge);lateral.run();lateral.next(.8f);
        lateral.hands.hands[0].grip.position.x+=edge==CoverEdgeKind::left?.2:-.2;r=lateral.next(.8f);
        C(r.interaction.owns_left_hand&&close(r.interaction.offset.right,edge==CoverEdgeKind::left?-.2:.2));
    }
    // The collision gate can inspect only a same-ticket tentative movement.
    // Aborting after inspection must leave neither its drag nor its hand grant.
    Fixture gated;
    CoverWeaponContribution ready{};ready.state=CoverWeaponState::ready;
    auto transact=[&](bool commit){
        auto decoded=gated.kernel.begin(gated.frame,gated.owner);
        C(decoded.accepted);
        auto candidate=gated.kernel.update_with_decoded(decoded.ticket,ready,gated.owner);
        C(candidate.accepted);
        auto movement=gated.kernel.pending_movement(decoded.ticket);
        auto result=gated.kernel.complete(decoded.ticket,commit,gated.owner);
        C(!gated.kernel.pending_movement(decoded.ticket).accepted);
        return std::pair<CoverInputOwnerMovement,CoverInputOwnerResult>{movement,result};
    };
    auto [neutral_candidate,neutral_result]=transact(true);
    C(neutral_candidate.accepted&&!neutral_candidate.committed_basis_matches&&neutral_candidate.candidate_offset_certified);
    C(neutral_result.accepted);
    gated.advance(.8f);auto [pressed,press_result]=transact(true);
    C(pressed.committed_basis_matches&&close(pressed.committed_offset.up,0)&&close(pressed.candidate_offset.up,0));
    C(press_result.interaction.owns_left_hand);
    gated.hands.hands[0].grip.position.y=.9;gated.advance(.8f);
    auto [drag,aborted]=transact(false);
    C(drag.committed_basis_matches&&drag.candidate_offset_certified&&drag.candidate_view_valid&&
      close(drag.committed_offset.up,0)&&close(drag.candidate_offset.up,.3)&&
      close(drag.candidate_view.base_eye_world.z,-10)&&close(drag.physical.up,0));
    C(!aborted.accepted&&aborted.view_valid&&close(aborted.interaction.offset.up,0)&&
      aborted.left_hand.lease.kind==LeftHandInteraction::none);
    gated.advance(.8f);auto [held,recovered]=transact(true);
    C(held.committed_basis_matches&&close(held.candidate_offset.up,0)&&
      !recovered.interaction.owns_left_hand);
    gated.frame.cover.epochs.cover++;gated.advance(0);auto [changed,after_change]=transact(true);
    C(changed.accepted&&!changed.committed_basis_matches&&close(changed.candidate_offset.up,0));
    C(after_change.accepted);
    gated.frame.lifecycle++;gated.advance(0);auto [new_lifecycle,after_restart]=transact(true);
    C(new_lifecycle.accepted&&!new_lifecycle.committed_basis_matches);
    C(after_restart.accepted);
    std::printf("cover_input_owner: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
