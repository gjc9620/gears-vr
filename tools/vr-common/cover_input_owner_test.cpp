#include "cover_input_owner.h"
#include <cstdio>
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
        frame.view_anchor=frame.anchor;
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
    CoverInputOwnerResult next(float squeeze){
        ++frame.frame;++frame.cover.sequence;++frame.stereo.sample.sequence;frame.now_ns+=10000000;
        frame.cover.observed_ns=frame.now_ns;hands.hands[0].squeeze=squeeze;publish();return run();
    }
};
int main(){
    {
        Fixture separate(CoverEdgeKind::right);
        const auto continuous=separate.frame.anchor;
        separate.frame.anchor.forward_world={0,1,0};
        separate.frame.anchor.right_world={-1,0,0};
        CoverTrackingBasis axes{};
        C(cover_tracking_basis(separate.frame.stereo.reference,continuous,separate.frame.anchor,axes));
        separate.frame.cover.right_axis_tracking=axes.right;
        separate.frame.cover.up_axis_tracking=axes.up;
        auto& sample=separate.frame.stereo.sample;
        sample.head.position.x+=.15;sample.head.position.z-=.1;
        for(auto& eye:sample.eyes){eye.pose.position.x+=.15;eye.pose.position.z-=.1;}
        separate.publish();
        const auto result=separate.run();CoverViewResult expected{};
        C(prepare_cover_view(continuous,{},100,expected));
        C(result.accepted&&result.view_valid);
        for(unsigned i=0;i<12;++i)C(result.view.base_view[i]==expected.base_view[i]);
        C(close(result.physical.right,.15)&&close(result.physical.forward,.1));
        C(close(result.requested_right_metres,-.1));
        C(separate.next(.8f).interaction.owns_left_hand);
        auto& hand=separate.hands.hands[0].grip.position;
        hand.x-=.12*axes.right.x;hand.y-=.12*axes.right.y;hand.z-=.12*axes.right.z;
        auto dragged=separate.next(.8f);
        C(close(dragged.interaction.offset.right,.12)&&close(dragged.requested_right_metres,.02));
        C(close(dragged.view.base_eye_world.x,988)&&close(dragged.view.base_eye_world.y,-2000));
        for(unsigned i=0;i<12;++i)C(dragged.view.base_view[i]==expected.base_view[i]);
        C(close(separate.next(0).interaction.offset.right,.12));
        C(separate.next(.8f).interaction.owns_left_hand);
        hand.x+=.06*axes.right.x;hand.y+=.06*axes.right.y;hand.z+=.06*axes.right.z;
        dragged=separate.next(.8f);
        C(close(dragged.interaction.offset.right,.06)&&close(dragged.requested_right_metres,-.04));
    }
    {
        Fixture missing;missing.frame.view_anchor={};missing.frame.weapon_requests.support=true;
        C(missing.run().accepted);
        const auto result=missing.next(.8f);
        C(result.accepted&&!result.view_valid&&!result.interaction.owns_left_hand);
        C(result.left_hand.new_grant&&result.left_hand.lease.kind==LeftHandInteraction::support);
    }
    {
        LeftControlSelector select;
        C(!select.update({true,false},{true,true},true).known); // Held attachment is not a new pinch.
        C(select.update({true,false},{true,false},true).known);
        auto p=select.update({true,false},{true,true},true);
        C(p.known&&p.down&&p.control==LeftInteractionControl::pinch);
        p=select.update({true,true},{true,true},true);
        C(p.known&&p.down&&p.control==LeftInteractionControl::pinch);
        p=select.update({true,true},{true,false},true);
        C(p.known&&!p.down&&p.control==LeftInteractionControl::pinch); // Never redirects held Grip.
        C(!select.update({false,false},{true,true},true).known);
        C(!select.update({true,true},{true,true},true).known);
        select.update({true,false},{true,false},true);
        p=select.update({true,true},{true,false},true);C(p.known&&p.down&&p.control==LeftInteractionControl::grip);
    }
    {
        Fixture pinch;pinch.frame.cover.cover_valid=false;pinch.frame.allow_index_pinch=true;
        pinch.frame.weapon_requests.magazine=true;pinch.hands.hands[0].trigger_active=1;pinch.publish();pinch.run();
        pinch.hands.hands[0].trigger=.8f;auto p=pinch.next(0);
        C(p.accepted&&p.left_hand.new_grant&&p.left_hand.lease.kind==LeftHandInteraction::magazine);
        C(!p.interaction.owns_left_hand); // Trigger never becomes a cover Grip.
    }
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
    // Exercise the actual cover/weapon bridge, not only the pure hand arbiter.
    Fixture deferred;deferred.frame.cover.cover_valid=false;deferred.run();
    deferred.frame.weapon_requests.magazine_observation_pending=true;r=deferred.next(.8f);
    C(r.accepted&&r.left_hand.lease.kind==LeftHandInteraction::none&&!r.left_hand.release_required);
    deferred.frame.weapon_requests.magazine_observation_pending=false;
    deferred.frame.weapon_requests.magazine=true;r=deferred.next(.8f);
    C(r.accepted&&r.left_hand.new_grant&&r.left_hand.lease.kind==LeftHandInteraction::magazine);
    const auto deferred_lease=r.left_hand.lease;deferred.frame.weapon_incumbents[0]=deferred_lease;
    r=deferred.next(.8f);C(r.accepted&&!r.left_hand.new_grant&&r.left_hand.status==LeftHandInteractionStatus::retained);
    Fixture known_absent;known_absent.frame.cover.cover_valid=false;known_absent.run();r=known_absent.next(.8f);
    C(r.accepted&&r.left_hand.release_required);
    known_absent.frame.weapon_requests.magazine=true;r=known_absent.next(.8f);
    C(r.accepted&&!r.left_hand.new_grant&&r.left_hand.lease.kind==LeftHandInteraction::none);
    // Both lateral directions use the fixed tracking right axis.
    for(auto edge:{CoverEdgeKind::left,CoverEdgeKind::right}){
        Fixture lateral(edge);lateral.run();lateral.next(.8f);
        lateral.hands.hands[0].grip.position.x+=edge==CoverEdgeKind::left?.2:-.2;r=lateral.next(.8f);
        C(r.interaction.owns_left_hand&&close(r.interaction.offset.right,edge==CoverEdgeKind::left?-.2:.2));
    }
    std::printf("cover_input_owner: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
