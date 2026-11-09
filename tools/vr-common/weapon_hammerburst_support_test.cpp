#include "weapon_hammerburst_support.h"
#include "weapon_hammerburst_foregrip.h"
#include "weapon_hand_owner_adapter.h"
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Assertions must remain enabled
#endif
using namespace gears_weapon_hand;
// Local host copied from the existing support arbiter fixture. No game/HMD proof.
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
    using namespace gears_weapon_hammerburst_support;
    Host h;
    const auto observe=[&](bool down){
        const auto p=h.prepare(down);const auto d=h.arbitrate(p,down);
        assert(h.participant.commit(h.frame,p,d)==Reply::accepted);
        const auto result=mode(h.frame,h.game,d,h.frame.now_ns+1000000);
        h.next();return result;
    };
    assert(observe(false)==Mode::snap);
    assert(h.participant.snapshot().grip==Grip::single);
    assert(observe(true)==Mode::grab);
    assert(h.participant.snapshot().grip==Grip::two);
    assert(observe(true)==Mode::grab);
    assert(observe(false)==Mode::snap);
    assert(h.participant.snapshot().grip==Grip::single);
    h.frame.controllers.hands[0].grip.position.x=.4;
    assert(observe(false)==Mode::free);
    h.frame.controllers.hands[0].grip.position.x=0;
    assert(observe(false)==Mode::snap);
    for(unsigned changed=0;changed<10;++changed){
        Host x;const auto p=x.prepare(false);const auto d=x.arbitrate(p,false);
        auto f=x.frame;auto g=x.game;auto decision=d;auto now=f.now_ns;
        switch(changed){
        case 0:g.busy=true;break;
        case 1:g.animation_owns_hands=true;break;
        case 2:f.left_tracked=0;break;
        case 3:f.controllers.hands[0].squeeze_active=0;break;
        case 4:f.interaction_control=1;break;
        case 5:g.request_magazine=true;break;
        case 6:g.request_ammo=true;break;
        case 7:++decision.key.lifecycle;break;
        case 8:now+=101000000;break;
        case 9:decision.release_required=1;break;
        }
        assert(mode(f,g,decision,now)==Mode::free);
    }
    {Host x;auto p=x.prepare(false);auto d=x.arbitrate(p,false);
     x.frame.controllers.hands[0].squeeze=1;x.frame.left_grip_down=1;
     assert(mode(x.frame,x.game,d,x.frame.now_ns)==Mode::free);}
    { // Passive keep range cannot create action authority.
      Host x;x.frame.controllers.hands[0].grip.position.x=.18;
      const auto p=x.prepare(false);auto d=x.arbitrate(p,false);
      assert(gears_weapon_support_socket::mode(x.frame,x.game,d,x.frame.now_ns)==Mode::free);
      assert(gears_weapon_support_socket::mode(x.frame,x.game,d,x.frame.now_ns,true)==Mode::snap);
      ++d.key.lifecycle;
      assert(gears_weapon_support_socket::mode(x.frame,x.game,d,x.frame.now_ns,true)==Mode::free);
      Host y;y.frame.controllers.hands[0].grip.position.x=.18;
      const auto pressed=y.prepare(true);const auto denied=y.arbitrate(pressed,true);
      assert(gears_weapon_support_socket::mode(y.frame,y.game,denied,y.frame.now_ns,true)==Mode::free);
    }
    // Actual authored HB socket under changing gun rotations: neutral contact
    // remains cosmetic, Grip alone grants support, release returns to Snap.
    // Controller samples are observed unchanged throughout this policy check.
    const gears_vr::Quaternion rotations[]={{0,0,0,1},{0,.7071067811865475,0,.7071067811865475},
        {.7071067811865475,0,0,.7071067811865475},{0,0,.7071067811865475,.7071067811865475},
        {.18301270189221933,.6830127018922193,-.6830127018922193,-.18301270189221933}};
    for(auto rotation:rotations){
        Host x;gears_vr::Vector raw{};
        assert(gears_weapon_hammerburst_foregrip::grip_in_mesh(raw));
        x.game.mount.support_grip_in_weapon={raw.y/100,raw.z/100,-raw.x/100};
        auto place=[&](gears_vr::Vector origin){
            auto& right=x.frame.controllers.hands[1].grip;
            right.position=origin;right.orientation=rotation;
            const auto offset=gears_weapon::rotate(rotation,x.game.mount.support_grip_in_weapon);
            x.frame.controllers.hands[0].grip.position={origin.x+offset.x,origin.y+offset.y,origin.z+offset.z};
        };
        auto verify=[&](bool down,Mode expected,Grip authority){
            const auto proposal=x.prepare(down);const auto decision=x.arbitrate(proposal,down);
            const auto physical=x.frame;
            assert(x.participant.commit(x.frame,proposal,decision)==Reply::accepted);
            assert(mode(x.frame,x.game,decision,x.frame.now_ns)==expected);
            assert(x.participant.snapshot().grip==authority);
            assert(same_frame(physical,x.frame));
            if(expected==Mode::snap)assert(decision.lease.kind==LeaseKind::none);
            x.next();
        };
        place({.2,1.1,-.1});verify(false,Mode::snap,Grip::single);
        place({.32,1.04,-.27});verify(false,Mode::snap,Grip::single);
        verify(true,Mode::grab,Grip::two);
        place({.25,1.16,-.2});verify(true,Mode::grab,Grip::two);
        verify(false,Mode::snap,Grip::single);
        x.frame.controllers.hands[0].grip.position.x+=.5;
        verify(false,Mode::free,Grip::single);
    }
    gears_vr::Vector point{};
    assert(gears_weapon_hammerburst_foregrip::grip_in_mesh(point));
    assert(gears_vr::finite(point));
    std::puts("HB support: Snap/Grab/release/exit, 11 rejection cases, five rotated authored sockets and unchanged physical frames passed; geometry finite");
}
