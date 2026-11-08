#include "weapon_support_grip.h"
#include "weapon_hand_owner_adapter.h"
#include "weapon_lancer_contact_route.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#if defined(NDEBUG)
#error WeaponSupportGripTest requires active assertions
#endif
using namespace gears_weapon_hand;

// Explicit local Native host model: one real shared arbiter, no engine/OS/XR
// transport. These tests cannot certify live mesh mounts or a callback lease.
struct Host {
    Frame frame{};GripGameState game{};SupportGripParticipant participant;
    SupportSocketPreference socket;bool auto_socket{},another_socket{};std::uint64_t hold{1};
    gears_vr::LeftHandInteractionOwner owner;
    std::uint64_t press{};bool previous_down{};
    ObjectIdentity object(std::uint64_t address){return {address,0x8000,0x9000,unsigned(address/8),7,0,1};}
    Host(){
        frame.key={11,22,33,1,1,1,44,55};frame.flags=scene_owned|input_matched|gameplay_interactable;
        frame.now_ns=1000000000;frame.world=object(0x1000);frame.controller=object(0x2000);
        frame.pawn=object(0x3000);frame.mesh=object(0x4000);frame.weapon=object(0x5000);
        frame.reference_generation=1;frame.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
        auto& s=frame.stereo;s.session=1;s.sequence=100;s.received_ns=frame.now_ns-10000000;
        s.predicted_time=123;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;
        s.head={{0,1.6,0},{0,0,0,1}};s.eyes[0].pose.position={-.032,1.6,0};s.eyes[1].pose.position={.032,1.6,0};
        for(auto& eye:s.eyes){eye.pose.orientation={0,0,0,1};eye.fov={-.8,.8,.8,-.8};}
        auto& c=frame.controllers;c.session=1;c.sequence=100;c.space_epoch=s.space_epoch;c.predicted_time=123;c.focused=1;
        frame.left_grip_known=frame.left_tracked=1;
        c.received_ns=s.received_ns;c.sync_ns=c.received_ns-1;
        for(auto& hand:c.hands){hand.grip_active=1;hand.grip_flags=15;}
        c.hands[1].grip.position={0,1,0};c.hands[0].grip.position={0,1,-.5};c.hands[0].squeeze_active=1;
        game.known=game.main_hand_held=game.mount_known=true;game.kind=WeaponKind::firearm;
        game.mount_asset=object(0x6000);game.mount_revision=1;
        game.mount.support_grip_in_weapon={0,0,-.5};game.mount.muzzle_in_weapon={0,0,-1};
        assert(frame_active(frame,frame.now_ns));
    }
    void next(){
        ++frame.key.frame;++frame.stereo.sequence;frame.controllers.sequence=frame.stereo.sequence;
        frame.now_ns+=10000000;frame.stereo.received_ns+=10000000;
        frame.controllers.received_ns=frame.stereo.received_ns;frame.controllers.sync_ns=frame.controllers.received_ns-1;
    }
    Proposal prepare(bool down){
        auto& hand=frame.controllers.hands[0];
        const auto control=left_control(frame);
        if(down&&!previous_down){++press;++hand.pressed[control];
            if(control==gears_vr::control_squeeze){hand.last_squeeze_press_grip=hand.grip;
                hand.last_squeeze_press_flags=hand.grip_flags;hand.last_squeeze_press_sequence=frame.controllers.sequence;}}
        if(!down&&previous_down)++hand.released[control];
        if(control==gears_vr::control_trigger){hand.trigger_active=1;hand.trigger=down?1.f:0.f;}
        else hand.squeeze=down?1.f:0.f;
        previous_down=down;
        frame.left_tracked=gears_vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)?1u:0u;
        frame.left_grip_down=down&&frame.left_tracked?1u:0u;frame.press_token=press;
        if(auto_socket&&socket.observe(frame,game,hold,frame.now_ns,another_socket))game.magazine_observation_pending=false;
        Proposal p{};assert(participant.prepare(frame,game,p)==Reply::accepted);assert(proposal_valid(p,frame));return p;
    }
    Decision arbitrate(Proposal const& p,bool down,std::uint64_t cover=0){
        gears_vr::LeftHandInteractionFrame request{};request.lifecycle=frame.key.lifecycle;
        request.frame=frame.key.frame;request.press_token=press;request.context_valid=true;
        request.now_ns=frame.now_ns;
        auto const& hand=frame.controllers.hands[0];
        request.tracked=gears_vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active);
        request.grip_down=down&&request.tracked;append_weapon_proposal(p,request);
        request.requests.cover_request_id=cover;
        return owner_decision(frame,p,owner.update(request),frame.now_ns+1000000);
    }
    Grip step(bool down,std::uint64_t cover=0){
        const auto p=prepare(down);const auto d=arbitrate(p,down,cover);
        assert(participant.commit(frame,p,d)==Reply::accepted);const auto result=participant.snapshot().grip;next();return result;
    }
};

int main(){
#if defined(_MSC_VER)
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    unsigned cases{};
    {++cases;Host h;h.auto_socket=true;assert(h.step(false)==Grip::single);
        h.game.magazine_observation_pending=true;
        assert(h.step(true)==Grip::two); // Actual hovered support + real edge; unknown unrelated magazine.
        assert(h.participant.snapshot().left_hand.kind==LeaseKind::support);
    }
    // A cold held press, stale input, changed identity/basis/mount/hold,
    // tracking loss, known competing part and another selected socket NEVER
    // obtain the affinity exception. Unknown contact remains pending.
    for(unsigned changed=0;changed<21;++changed){
        ++cases;Host h;h.auto_socket=true;assert(h.step(false)==Grip::single);
        switch(changed){
        case 0:h.socket.reset();break;
        case 1:++h.hold;break;
        case 2:++h.frame.key.lifecycle;break;
        case 3:++h.frame.key.world_generation;break;
        case 4:++h.frame.key.registration_cookie;break;
        case 5:++h.frame.controllers.input_epoch;break;
        case 6:++h.frame.controllers.profile_generation;break;
        case 7:++h.frame.reference_generation;break;
        case 8:h.frame.reference.position.x=.01;break;
        case 9:h.game.mount_revision++;break;
        case 10:h.game.mount_asset.name_number++;break;
        case 11:h.frame.weapon.name_number++;break;
        case 12:h.game.mount.support_grip_in_weapon.z=-.51;break;
        case 13:h.game.acquire_metres=.11;break;
        case 14:h.frame.controllers.hands[0].grip.position.x=.2;break;
        case 15:h.another_socket=true;break;
        case 16:h.frame.interaction_control=1;break;
        case 17:h.game.busy=true;break;
        case 18:h.frame.controllers.hands[0].grip_active=0;break;
        case 19:h.frame.key.frame=0;break;
        case 20:h.hold=0;break;
        }
        h.game.magazine_observation_pending=true;
        h.frame.left_grip_down=1;h.frame.controllers.hands[0].squeeze=1;
        assert(!h.socket.observe(h.frame,h.game,h.hold,h.frame.now_ns,h.another_socket));
    }
    for(unsigned part=0;part<2;++part){++cases;Host h;h.auto_socket=true;h.step(false);
        h.game.magazine_observation_pending=true;
        h.game.request_magazine=part==0;h.game.request_ammo=part==1;
        const auto p=h.prepare(true);const auto d=h.arbitrate(p,true);
        assert(!p.support&&d.lease.kind==(part?LeaseKind::ammo:LeaseKind::magazine));
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);
    }
    {++cases;Host h;SupportSocketPreference preference;
        assert(!preference.observe(h.frame,h.game,1,h.frame.now_ns,false));
        h.frame.left_grip_down=1;h.frame.controllers.hands[0].squeeze=1;
        const auto expired=h.frame.controllers.received_ns+100000001;
        assert(!preference.observe(h.frame,h.game,1,expired,false));
        assert(!preference.observe(h.frame,h.game,1,h.frame.now_ns,false)); // No stored permission after expiry.
    }
    for(unsigned control=0;control<2;++control){
        ++cases;Host h;h.frame.interaction_control=control;assert(h.step(false)==Grip::single);
        h.game.magazine_observation_pending=true;
        auto p=h.prepare(true);auto d=h.arbitrate(p,true);
        assert(d.status==DecisionStatus::no_candidate&&!d.release_required);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);h.next();
        //46860 token2: a current KNOWN fore-end contact after the pending
        // installed-mag observation must not consume the held input as a miss.
        h.game.magazine_observation_pending=false;p=h.prepare(true);d=h.arbitrate(p,true);
        assert(p.support&&!p.magazine&&!p.ammo&&!p.magazine_observation_pending);
        assert(d.new_grant&&d.lease.kind==LeaseKind::support&&d.press_token==1);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);
        assert(h.participant.snapshot().grip==Grip::two);h.next();
        assert(h.step(true)==Grip::two);assert(h.step(false)==Grip::single);
    }
    for(unsigned control=0;control<2;++control){
        ++cases;Host h;h.frame.interaction_control=control;
        assert(h.step(false)==Grip::single);h.game.request_magazine=true;
        auto p=h.prepare(true);auto d=h.arbitrate(p,true);
        assert(d.new_grant&&d.lease.kind==LeaseKind::magazine);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);
        const auto held=d.lease;const auto granted=h.frame.key.frame;h.next();
        h.game.request_magazine=false;h.game.magazine_observation_pending=true;
        //68424: installed-mag geometry is unknown but the existing C grasp
        // must survive, without proposing any new contact or borrowing a pose.
        for(unsigned i=0;i<60;++i){
            h.game.object_incumbents={};
            assert(gears_weapon::retain_lancer_part_incumbent(h.frame,held,granted,true,h.game.object_incumbents));
            p=h.prepare(true);d=h.arbitrate(p,true);
            assert(d.status==DecisionStatus::retained&&same(d.lease,held)&&!d.new_grant);
            assert(!p.magazine_observation_pending&&!p.magazine&&!p.ammo);
            assert(h.participant.commit(h.frame,p,d)==Reply::accepted);h.next();
        }
        for(unsigned invalid=0;invalid<7;++invalid){
            auto f=h.frame;auto lease=held;std::array<Lease,2> owners{};
            bool identity=true;std::uint64_t grant_frame=granted;
            switch(invalid){
            case 0:f.left_grip_down=0;break;
            case 1:f.left_grip_known=0;break;
            case 2:f.left_tracked=0;break;
            case 3:identity=false;break;
            case 4:++f.key.lifecycle;break;
            case 5:++f.press_token;break;
            case 6:grant_frame=f.key.frame;break;
            }
            assert(!gears_weapon::retain_lancer_part_incumbent(f,lease,grant_frame,identity,owners));
            assert(owners[0].kind==LeaseKind::none&&owners[1].kind==LeaseKind::none);
        }
        h.game.object_incumbents={};assert(h.step(false)==Grip::single);
        h.frame.left_grip_down=1; // Exercise a genuine occupied-owner conflict.
        auto owners=std::array<Lease,2>{Lease{LeaseKind::ammo,0,h.frame.key.lifecycle,71},
            Lease{LeaseKind::support,0,h.frame.key.lifecycle,72}};
        const auto before=owners;
        assert(!gears_weapon::retain_lancer_part_incumbent(h.frame,held,granted,true,owners));
        assert(same(owners[0],before[0])&&same(owners[1],before[1]));
    }
    for(unsigned control=0;control<2;++control){
        ++cases;Host h;h.frame.interaction_control=control;
        h.game.magazine_observation_pending=true; // Unknown pose while released cannot reserve a press.
        assert(h.step(false)==Grip::single);
        h.game.magazine_observation_pending=true;
        auto p=h.prepare(true);assert(p.magazine_observation_pending&&!p.support&&!p.magazine&&!p.ammo);
        auto d=h.arbitrate(p,true);assert(d.status==DecisionStatus::no_candidate&&!d.release_required);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);h.next();
        h.game.magazine_observation_pending=false;h.game.request_magazine=true;
        p=h.prepare(true);d=h.arbitrate(p,true);
        assert(d.status==DecisionStatus::granted&&d.lease.kind==LeaseKind::magazine&&d.press_token==1);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);
    }
    {++cases;Host h;assert(h.step(false)==Grip::single);
        h.game.magazine_observation_pending=true;auto p=h.prepare(true);auto d=h.arbitrate(p,true);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);h.next();
        h.frame.now_ns+=150000000;h.frame.stereo.received_ns+=150000000;h.next();
        h.game.magazine_observation_pending=false;h.game.request_magazine=true;
        p=h.prepare(true);d=h.arbitrate(p,true);assert(d.status==DecisionStatus::needs_release&&d.release_required);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);
    }
    {++cases;Host h;assert(h.step(false)==Grip::single);assert(h.step(true)==Grip::two);
        auto s=h.participant.snapshot();assert(s.left_hand.kind==LeaseKind::support&&s.press_token==1);
        // Main hand is a toggled equipment hold: no right squeeze is required.
        assert(h.frame.controllers.hands[1].squeeze==0&&s.grip==Grip::two);
        assert(s.current(s.key,s.weapon,s.input_received_ns+100000000));
        assert(!s.current(s.key,s.weapon,s.input_received_ns+100000001));
        assert(h.step(false)==Grip::single);}
    {++cases;Host h;h.frame.controllers.hands[0].grip.position.x=.20;
        assert(h.step(false)==Grip::single&&h.step(true)==Grip::single);
        h.frame.controllers.hands[0].grip.position.x=0;
        assert(h.step(true)==Grip::single);assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);}
    {++cases;Host h;assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);
        h.frame.controllers.hands[0].grip.position.x=.17;assert(h.step(true)==Grip::two);
        h.frame.controllers.hands[0].grip.position.x=.26;assert(h.step(true)==Grip::two);
        h.frame.controllers.hands[0].grip.position.x=0;assert(h.step(true)==Grip::two);
        assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);}
    {++cases;Host h;assert(h.step(false)==Grip::single);assert(h.step(true,99)==Grip::single);
        assert(h.participant.snapshot().left_hand.kind==LeaseKind::cover);
        Host incumbent;assert(incumbent.step(false)==Grip::single&&incumbent.step(true)==Grip::two);
        assert(incumbent.step(true,99)==Grip::two);}
    {++cases;Host h;h.game.object_incumbents[0]={LeaseKind::ammo,0,1,71};
        assert(h.step(true)==Grip::single&&h.participant.snapshot().left_hand.kind==LeaseKind::ammo);
        Host conflict;conflict.game.object_incumbents={Lease{LeaseKind::ammo,0,1,71},Lease{LeaseKind::magazine,0,1,72}};
        assert(conflict.step(true)==Grip::unknown);}
    {++cases;Host h;assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);
        ++h.frame.controllers.profile_generation;++h.frame.controllers.input_epoch;
        assert(h.step(true)!=Grip::two);assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);
        ++h.game.mount_revision;assert(h.step(true)!=Grip::two);}
    {++cases;Host h;const auto p=h.prepare(false);auto d=h.arbitrate(p,false);
        auto changed=h.frame;changed.controllers.hands[0].grip.position.x=.01;
        assert(h.participant.commit(changed,p,d)==Reply::invalid&&h.participant.snapshot().grip==Grip::unknown);
        Host nonce;const auto np=nonce.prepare(false);auto nd=nonce.arbitrate(np,false);++nd.proposal_nonce;
        assert(nonce.participant.commit(nonce.frame,np,nd)==Reply::invalid);}
    {++cases;Host h;const auto p=h.prepare(false);const auto d=h.arbitrate(p,false);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);
        assert(h.participant.commit(h.frame,p,d)==Reply::invalid);
        Proposal duplicate{};assert(h.participant.prepare(h.frame,h.game,duplicate)==Reply::invalid);}
    {++cases;Host h;h.game.main_hand_held=false;h.frame.weapon={};
        assert(h.step(false)==Grip::unknown);assert(h.step(true,99)==Grip::unknown);
        assert(h.participant.snapshot().left_hand.kind==LeaseKind::cover);
        Host animation;animation.game.animation_owns_hands=true;assert(animation.step(false)==Grip::unknown);
        Host right;right.frame.controllers.hands[1].grip_flags=3;assert(right.step(false)==Grip::unknown);}
    {++cases;Host h;h.game.kind=WeaponKind::hand_thrown;assert(h.step(false)==Grip::excluded);
        assert(h.step(true)==Grip::excluded);
        Host unmeasured;unmeasured.game.mount_known=false;assert(unmeasured.step(false)==Grip::single);
        assert(unmeasured.step(true)==Grip::single);}
    {++cases;Host h;assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);
        h.frame.controllers.hands[0].grip_flags=3;assert(h.step(true)==Grip::unknown);
        h.frame.controllers.hands[0].grip_flags=15;assert(h.step(true)!=Grip::two);
        assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);}
    {++cases;Host h;const auto p=h.prepare(false);auto d=h.arbitrate(p,false);
        d.status=DecisionStatus::granted;d.new_grant=1;d.release_required=0;d.press_token=1;
        d.lease={LeaseKind::support,0,1,1};auto unavailable=p;unavailable.support=0;
        assert(h.participant.commit(h.frame,unavailable,d)==Reply::invalid);
        Host unknown;unknown.game.known=false;Proposal none{};
        assert(unknown.participant.prepare(unknown.frame,unknown.game,none)==Reply::invalid);}
    {++cases;Host h;const auto p=h.prepare(false);auto d=h.arbitrate(p,false);
        // Keep the exact original proposal and frame: a decision cannot invent
        // an LGrip press or override the frozen decoded Native logical state.
        d.status=DecisionStatus::granted;d.new_grant=1;d.release_required=0;d.press_token=1;
        d.lease={LeaseKind::support,0,1,1};
        assert(h.participant.commit(h.frame,p,d)==Reply::invalid);
        Host busy;busy.game.object_incumbents[0]={LeaseKind::ammo,0,1,71};
        const auto bp=busy.prepare(true);auto bd=busy.arbitrate(bp,true);
        bd.status=DecisionStatus::granted;bd.new_grant=1;bd.release_required=0;bd.lease={LeaseKind::support,0,1,1};
        assert(busy.participant.commit(busy.frame,bp,bd)==Reply::invalid);}
    {++cases;Host h;assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);
        h.game.object_incumbents[0]={LeaseKind::ammo,0,1,71};const auto p=h.prepare(true);auto d=h.arbitrate(p,true);
        d.status=DecisionStatus::retained;d.new_grant=0;d.release_required=0;d.lease={LeaseKind::support,0,1,1};
        assert(h.participant.commit(h.frame,p,d)==Reply::invalid);
        Host candidate;candidate.game.request_ammo=true;assert(candidate.step(false)==Grip::single);
        const auto cp=candidate.prepare(true);auto cd=candidate.arbitrate(cp,true);
        cd.status=DecisionStatus::granted;cd.new_grant=1;cd.release_required=0;cd.lease={LeaseKind::support,0,1,1};
        assert(candidate.participant.commit(candidate.frame,cp,cd)==Reply::invalid);}
    {++cases;Host h;gears_vr::LeftHandInteractionFrame native{};
        native.lifecycle=1;native.frame=1;native.press_token=1;native.context_valid=native.tracked=native.grip_down=true;
        native.incumbents[0]={gears_vr::LeftHandInteraction::cover,1,99};
        Proposal empty{};append_weapon_proposal(empty,native);
        assert(native.incumbents[0].kind==gears_vr::LeftHandInteraction::cover&&native.incumbents[0].id==99);
        Proposal ammo{};ammo.incumbents[0]={LeaseKind::ammo,0,1,71};append_weapon_proposal(ammo,native);
        assert(native.incumbents[0].kind==gears_vr::LeftHandInteraction::cover&&
            native.incumbents[1].kind==gears_vr::LeftHandInteraction::ammo);
        const auto d=h.owner.update(native);assert(d.status==gears_vr::LeftHandInteractionStatus::incumbent_conflict&&
            d.lease.kind==gears_vr::LeftHandInteraction::none);}
    {++cases;Host h;h.frame.interaction_control=unsigned(gears_vr::LeftInteractionControl::pinch);
        assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);
        const auto lease=h.participant.snapshot().left_hand;
        h.frame.controllers.hands[0].grip.position.x=.35;
        assert(h.step(true)==Grip::two&&same(h.participant.snapshot().left_hand,lease));
        assert(h.step(false)==Grip::single);
        assert(h.step(true)==Grip::single); // fresh far-away press cannot acquire
        h.frame.controllers.hands[0].grip.position.x=0;
        assert(h.step(true)==Grip::single); // hovering while held supplies no new edge
        assert(h.step(false)==Grip::single&&h.step(true)==Grip::two);
    }
    std::printf("WeaponSupportGripTest passed cases=%u; local host/shared owner, no runtime transport or mount acceptance\n",cases);
}
