#include "weapon_lancer_saw_input.h"
#include <cassert>
#include <cstdio>
namespace s=gears_lancer_saw_input;
using Status=s::Status;
s::Observation live(){
    s::Observation o{};
    o.actors.world={0x1000,0,0x9000,1,1,0,1};
    o.actors.controller={0x2000,0,0x9000,2,2,0,1};
    o.actors.pawn={0x3000,0,0x9000,3,3,0,1};
    o.actors.weapon={0x4000,0x3000,0x9000,4,4,0,1};
    o.controller_known=o.button_known=o.actors_current=o.current_lancer=o.can_start=true;return o;
}
unsigned release_tail_cases(){
    gears_vr::StereoSample stereo{};stereo.session=11;stereo.sequence=100;stereo.predicted_time=55;
    stereo.received_ns=1000000000;stereo.focused=stereo.visible=stereo.should_render=1;
    stereo.head_flags=stereo.view_flags=15;stereo.head.orientation.w=1;
    stereo.eyes[0].pose.position.x=-.032;stereo.eyes[1].pose.position.x=.032;
    for(auto& eye:stereo.eyes){eye.pose.orientation.w=1;eye.fov={-.7,.7,.7,-.7};}
    gears_hand_continuity::TrackedInput t{};auto& c=t.input;
    c.session=stereo.session;c.sequence=stereo.sequence;c.space_epoch=stereo.space_epoch;
    c.predicted_time=stereo.predicted_time;c.received_ns=c.sync_ns=stereo.received_ns;
    c.focused=1;c.input_epoch=c.profile_generation=1;
    for(auto& h:c.hands){h.grip_active=h.aim_active=h.squeeze_active=1;h.grip_flags=h.aim_flags=15;}
    c.hands[0].squeeze=1;c.hands[0].pressed[gears_vr::control_squeeze]=1;
    t.stream_epoch=t.tracking_epoch[0]=t.tracking_epoch[1]=1;
    const gears_hand_continuity::Publisher publisher{44,55};s::HoldInput pin;unsigned cases{};
    assert(pin.bind(stereo,t,publisher,c.sync_ns+1000000,false,gears_vr::control_squeeze,true));++cases;
    auto publish=[&](std::int64_t ns){++c.sequence;stereo.sequence=c.sequence;
        stereo.received_ns=c.sync_ns=c.received_ns=ns;};
    // Holding at the mechanical endpoint is indefinite, not a3s maximum.
    for(unsigned i=1;i<=80;++i){publish(1000000000LL+50000000LL*i);
        assert(pin.refresh(stereo,t,publisher,c.sync_ns+1000000,false)&&!pin.release_tail_started());++cases;}
    publish(5050000000);c.hands[0].squeeze=0;++c.hands[0].released[gears_vr::control_squeeze];
    assert(pin.refresh(stereo,t,publisher,c.sync_ns+1000000,false)&&pin.release_tail_started());++cases;
    // Identity/fresh tracking/focus stay mandatory during the released tail.
    for(unsigned mode=0;mode<5;++mode){auto candidate=pin;auto bad=t;auto eye=stereo;auto owner=publisher;
        if(mode==0)bad.input.hands[0].grip_flags=0;
        if(mode==1)++bad.tracking_epoch[1];
        if(mode==2)eye.focused=bad.input.focused=0;
        if(mode==3)++owner.start;
        if(mode==4)++bad.input.input_epoch;
        assert(!candidate.refresh(eye,bad,owner,c.sync_ns+1000000,false));++cases;}
    // New squeeze/release during the tail does NOT restart/extend the timer.
    for(unsigned i=1;i<=59;++i){publish(5050000000LL+50000000LL*i);
        if(i==20){c.hands[0].squeeze=1;++c.hands[0].pressed[gears_vr::control_squeeze];}
        if(i==30){c.hands[0].squeeze=0;++c.hands[0].released[gears_vr::control_squeeze];}
        assert(pin.refresh(stereo,t,publisher,c.sync_ns+1000000,false));++cases;}
    publish(8050000000LL);
    assert(pin.release_tail_expired(c.sync_ns)&&!pin.refresh(stereo,t,publisher,c.sync_ns,false));++cases;
    return cases;
}
unsigned hold_input_cases(){
    gears_vr::StereoSample stereo{};stereo.session=11;stereo.sequence=100;stereo.predicted_time=55;
    stereo.received_ns=1000000000;stereo.focused=stereo.visible=stereo.should_render=1;
    stereo.head_flags=stereo.view_flags=15;stereo.head.orientation.w=1;
    stereo.eyes[0].pose.position.x=-.032;stereo.eyes[1].pose.position.x=.032;
    for(auto& eye:stereo.eyes){eye.pose.orientation.w=1;eye.fov={-.7,.7,.7,-.7};}
    gears_hand_continuity::TrackedInput t{};auto& c=t.input;
    c.session=stereo.session;c.sequence=stereo.sequence;c.space_epoch=stereo.space_epoch;
    c.predicted_time=stereo.predicted_time;c.received_ns=c.sync_ns=stereo.received_ns;
    c.focused=1;c.input_epoch=c.profile_generation=1;
    for(auto& h:c.hands){h.grip_active=h.aim_active=h.squeeze_active=1;h.grip_flags=h.aim_flags=15;}
    c.hands[0].squeeze=1;c.hands[0].pressed[gears_vr::control_squeeze]=1;
    t.stream_epoch=t.tracking_epoch[0]=t.tracking_epoch[1]=1;
    const gears_hand_continuity::Publisher publisher{44,55};s::HoldInput pin;
    assert(pin.bind(stereo,t,publisher,1001000000,false));
    // The old game Frame would be104ms old. This is a separately published
    // current input at4ms age; accepting it does not renew that old Frame.
    stereo.received_ns=c.sync_ns=c.received_ns=1100000000;++stereo.sequence;++c.sequence;
    assert(pin.refresh(stereo,t,publisher,1104000000,false)&&pin.sync_ns()==1100000000);
    assert(pin.refresh(stereo,t,publisher,1200000000,false));
    assert(!pin.refresh(stereo,t,publisher,1200000001,false));
    unsigned cases=4;
    for(unsigned changed=0;changed<23;++changed){
        auto sample=stereo;auto input=t;auto producer=publisher;auto candidate=pin;
        switch(changed){
        case 0:++producer.pid;break;case 1:++producer.start;break;
        case 2:++input.stream_epoch;break;case 3:++input.tracking_epoch[0];break;
        case 4:++input.tracking_epoch[1];break;
        case 5:sample.source=input.input.source=gears_vr::StereoSource::MetaOperatorSimulated;break;
        case 6:++sample.session;++input.input.session;break;
        case 7:++sample.space_epoch;++input.input.space_epoch;break;
        case 8:++input.input.input_epoch;break;case 9:++input.input.profile_generation;break;
        case 10:--sample.sequence;--input.input.sequence;break;
        case 11:--input.input.sync_ns;break;case 12:sample.head_flags=0;break;
        case 13:sample.focused=input.input.focused=0;break;
        case 14:input.input.hands[1].grip_flags=0;break;
        case 15:sample.should_render=0;break;
        case 16:sample.received_ns=1104000001;break;
        case 17:input.input.sync_ns=1000000000;break;
        case 18:input.input.hands[0].squeeze=0;break;
        case 19:input.input.hands[0].grip_flags=0;break;
        case 20:input.input.hands[0].squeeze_active=0;input.input.hands[0].squeeze=0;break;
        // A release and new press between two game ticks may leave squeeze1;
        // cumulative counters still stop the old motor instead of latching.
        case 21:++input.input.hands[0].released[gears_vr::control_squeeze];++input.input.hands[0].pressed[gears_vr::control_squeeze];break;
        case 22:++input.input.hands[0].released[gears_vr::control_squeeze];break;
        }
        assert(!candidate.refresh(sample,input,producer,1104000000,false));++cases;
    }
    // Real SourceTracker, not a manually held epoch: many XR syncs continue
    // while the GAME does not poll for333ms. A true producer pause, tracking
    // loss, hidden release/regrip or stale latest sample must still deny it.
    for(unsigned mode=0;mode<5;++mode){
        auto sample=stereo;auto input=t.input;gears_hand_continuity::SourceTracker tracker;
        gears_hand_continuity::TrackedInput tracked{};assert(tracker.update(input,tracked));
        s::HoldInput candidate;assert(candidate.bind(sample,tracked,publisher,input.sync_ns+1000000,false));
        for(unsigned i=1;i<=30;++i){
            ++input.sequence;input.sync_ns+=11100000;input.received_ns=input.sync_ns;
            if(mode==1&&i==10){input.sync_ns+=200000000;input.received_ns=input.sync_ns;}
            if(mode==2)input.hands[0].grip_flags=i==10?0u:15u;
            if(mode==3&&i==10){++input.hands[0].pressed[gears_vr::control_squeeze];++input.hands[0].released[gears_vr::control_squeeze];}
            assert(tracker.update(input,tracked));
        }
        sample.sequence=input.sequence;sample.received_ns=input.sync_ns;
        assert(candidate.refresh(sample,tracked,publisher,input.sync_ns+(mode==4?100000001:47000000),false)==(mode==0));++cases;
    }
    pin.reset();assert(!pin.refresh(stereo,t,publisher,1104000000,false));++cases;
    // E uses the index, not Grip. Squeeze cannot mask a released index or a
    // release/re-pinch hidden between game ticks; it grants no movement.
    auto pinch=t;pinch.input.hands[0].trigger_active=1;pinch.input.hands[0].trigger=1;
    pinch.input.hands[0].pressed[gears_vr::control_trigger]=1;
    s::HoldInput small;assert(small.bind(stereo,pinch,publisher,1104000000,false,gears_vr::control_trigger));++cases;
    for(unsigned failure=0;failure<5;++failure){auto changed=pinch;auto sample=stereo;
        if(failure==0)changed.input.hands[0].trigger=0;
        if(failure==1){++changed.input.hands[0].pressed[gears_vr::control_trigger];++changed.input.hands[0].released[gears_vr::control_trigger];}
        if(failure==2)++changed.stream_epoch;
        if(failure==3){sample.received_ns+=100000001;changed.input.sync_ns=changed.input.received_ns=sample.received_ns;}
        if(failure==4)changed.input.hands[0].grip_flags=0;
        assert(!small.refresh(sample,changed,publisher,1104000000,false));++cases;
    }
    pinch.input.hands[0].squeeze=0;
    assert(small.refresh(stereo,pinch,publisher,1104000000,false));++cases;
    return cases;
}
unsigned ready_stop_cases(){
    unsigned cases{},calls{};auto o=live();o.fire_mode_known=true;
    auto stop=[&](){++calls;return s::Call{true,true};};
    s::ReadyStop ready;assert(ready.service(o,stop)==Status::idle&&!calls);++cases;
    ready.arm(o.actors);
    // Already quiet is proof of restoration and must not force another stop.
    assert(ready.service(o,stop)==Status::released&&!ready.owed()&&!calls);++cases;
    o.special_move=33;o.fire_mode=9;o.motor_running=o.doing_melee=true;ready.arm(o.actors);
    const auto rejected=[&](auto change){auto bad=o;change(bad);assert(ready.service(bad,stop)==Status::cleanup_debt&&ready.owed()&&!calls);++cases;};
    rejected([](auto& v){v.controller_known=false;});
    rejected([](auto& v){v.button_known=false;});
    rejected([](auto& v){v.button_held=true;});
    rejected([](auto& v){v.actors_current=false;});
    rejected([](auto& v){v.current_lancer=false;});
    rejected([](auto& v){v.fire_mode_known=false;});
    rejected([](auto& v){++v.actors.weapon.registry_index;});
    rejected([](auto& v){++v.actors.pawn.registry_index;});
    auto animation=o;animation.special_move=34;
    assert(ready.service(animation,stop)==Status::deferred_animation&&!calls);++cases;
    animation=o;animation.doing_special=true;
    assert(ready.service(animation,stop)==Status::deferred_animation&&!calls);++cases;
    animation=o;animation.special_move=12;
    assert(ready.service(animation,stop)==Status::deferred_animation&&!calls);++cases;
    assert(ready.service(o,[](){return s::Call{};})==Status::cleanup_debt&&ready.owed());++cases;
    assert(ready.service(o,stop)==Status::release_pending&&calls==1&&ready.owed());++cases;
    assert(ready.service(o,stop)==Status::cleanup_debt&&calls==1);++cases;
    auto quiet=live();quiet.fire_mode_known=true;
    assert(ready.service(quiet,stop)==Status::released&&calls==1&&!ready.owed());++cases;
    // A false return may still have executed; never dispatch that stop twice.
    ready.arm(o.actors);
    assert(ready.service(o,[&](){++calls;return s::Call{true,false};})==Status::cleanup_debt);++cases;
    assert(ready.service(o,stop)==Status::cleanup_debt&&calls==2);++cases;
    assert(ready.service(quiet,stop)==Status::released&&!ready.owed()&&calls==2);++cases;
    // Motor/SM flags alone cannot release the fire-mode9 switch gate.
    ready.arm(quiet.actors);quiet.fire_mode=9;
    assert(ready.service(quiet,stop)==Status::release_pending&&calls==3&&ready.owed());++cases;
    quiet.fire_mode=0;
    assert(ready.service(quiet,stop)==Status::released&&!ready.owed()&&calls==3);++cases;
    return cases;
}
int main(){
    {
        auto o=live();o.button_held=o.motor_running=o.doing_melee=true;o.special_move=33;
        assert(s::ready_motion_context(o));
        o.special_move=34;assert(!s::ready_motion_context(o));
        o.special_move=21;assert(!s::ready_motion_context(o));
        o.special_move=33;o.doing_special=true;assert(!s::ready_motion_context(o));
        o.doing_special=false;o.actors_current=false;assert(!s::ready_motion_context(o));
        o.actors_current=true;o.motor_running=false;assert(!s::ready_motion_context(o));
    }
    const auto source_cases=hold_input_cases()+release_tail_cases();
    const auto stop_cases=ready_stop_cases();
    unsigned presses{},releases{};
    const auto press=[&](){++presses;return s::Call{true,true};};
    const auto release=[&](){++releases;return s::Call{true,true};};
    s::Button button;auto o=live();
    o.button_held=true;assert(button.begin(o,true,press)==Status::not_started&&presses==0&&!button.owed());
    o=live();assert(button.begin(o,false,press)==Status::not_started&&presses==0);
    assert(button.begin(o,true,press)==Status::owned&&presses==1&&button.owed());
    assert(button.begin(o,true,press)==Status::cleanup_debt&&presses==1);
    o.button_held=true;o.special_move=33;
    assert(button.service(o,false,release)==Status::owned&&releases==0);
    // An execution entry wins over loss/stop. Release happens only after the
    // original attack leaves34 and the special-attack flag clears.
    o.special_move=34;o.doing_special=true;
    assert(button.service(o,true,release)==Status::deferred_animation&&releases==0);
    o.special_move=33;
    assert(button.service(o,true,release)==Status::deferred_animation&&releases==0);
    o.doing_special=false;
    assert(button.service(o,false,release)==Status::release_pending&&releases==1&&button.owed());
    assert(button.service(o,true,release)==Status::cleanup_debt&&releases==1); // no repeated release
    o.button_held=false;
    assert(button.service(o,true,release)==Status::released&&!button.owed());
    // A call that may have pressed B must retain cleanup debt even on failure.
    o=live();assert(button.begin(o,true,[](){return s::Call{true,false};})==Status::cleanup_debt&&button.owed());
    o.button_held=true;auto foreign=o;foreign.actors.pawn.registry_index++;
    assert(button.service(foreign,true,release)==Status::cleanup_debt&&releases==1);
    foreign.actors=live().actors;foreign.actors_current=false;foreign.actors.pawn.address+=8;
    assert(button.service(foreign,true,release)==Status::cleanup_debt&&releases==1);
    foreign.button_held=false;
    assert(button.service(foreign,true,release)==Status::released&&!button.owed());
    o=live();assert(button.begin(o,true,[](){return s::Call{false,false};})==Status::not_started&&!button.owed());
    // Identity validation failure cannot turn another controller's up state
    // into restoration of this owner's B press.
    assert(button.begin(o,true,press)==Status::owned);o.button_held=false;o.actors.controller.registry_index++;
    assert(button.service(o,true,release)==Status::cleanup_debt&&button.owed());
    std::puts("Lancer saw input ownership: once-only press/release, animation deferral and foreign-pawn cleanup debt passed");
    std::printf("Existing-hold live input: %u publisher/cohort/tracking/focus/100ms cases; no fresh Frame or new-action permit\n",source_cases);
    std::printf("Original ready cancellation: %u B-up/actor/SM34/mode9/once-only/restoration cases passed\n",stop_cases);
}
