#include "original_r2_fire.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace gears_vr;
using namespace gears_weapon;

namespace {
struct Trial {
    StereoSample stereo{};ControllerInputFrame frame{};
    OriginalR2Context context{11,12,13,true,false,true,true};
    OriginalR2Fire fire;std::vector<OriginalR2Action> calls;bool succeeds{true};
    explicit Trial(bool start_baseline=true){
        stereo.session=frame.session=7;stereo.sequence=frame.sequence=1;
        stereo.space_epoch=frame.space_epoch=3;stereo.predicted_time=frame.predicted_time=1000;
        stereo.focused=frame.focused=1;frame.sync_ns=999999900;frame.received_ns=1000000000;
        frame.hands[1].grip_active=1;frame.hands[1].grip_flags=15;
        frame.hands[1].grip.orientation.w=1;
        if(start_baseline)assert(advance()==OriginalR2Status::baseline);
    }
    bool execute(OriginalR2Action action,std::uint64_t pc){
        assert(pc==12);calls.push_back(action);return succeeds;
    }
    OriginalR2Status advance(){
        return fire.advance(frame,stereo,context,1000000100,
            [&](OriginalR2Action action,std::uint64_t pc){return execute(action,pc);});
    }
    OriginalR2Status yield(){
        return fire.yield(context,
            [&](OriginalR2Action action,std::uint64_t pc){return execute(action,pc);});
    }
    void next(){++frame.sequence;++stereo.sequence;}
    void press(){
        next();++frame.hands[1].pressed[control_trigger];
        assert(advance()==OriginalR2Status::pressed&&fire.owns_press());context.game_r2_held=true;
    }
};
void cold_yield_still_requires_baseline_release_and_press(){
    Trial t(false);
    assert(t.yield()==OriginalR2Status::accepted&&t.calls.empty()&&!t.fire.owns_press());
    assert(t.yield()==OriginalR2Status::accepted&&t.calls.empty());
    t.frame.hands[1].pressed[control_trigger]=1; // Already held before the first input read.
    t.frame.hands[1].trigger_active=1;t.frame.hands[1].trigger=1;
    assert(t.advance()==OriginalR2Status::baseline&&t.calls.empty());
    assert(t.advance()==OriginalR2Status::rejected&&t.calls.empty());
    t.next();assert(t.advance()==OriginalR2Status::blocked&&t.calls.empty());
    t.next();++t.frame.hands[1].released[control_trigger];
    assert(t.advance()==OriginalR2Status::accepted&&t.calls.empty());
    t.press();assert(t.calls.size()==1);

    // Losing an input baseline cannot turn a formerly bound identity cold.
    Trial known;known.frame.received_ns=800000000;
    assert(known.advance()==OriginalR2Status::rejected);
    ++known.context.weapon;
    assert(known.yield()==OriginalR2Status::abort_required&&known.calls.empty());
    --known.context.weapon;
    assert(known.yield()==OriginalR2Status::abort_required);
}
void baseline_release_proof_requires_active_low_trigger_and_game_key_up(){
    Trial cold(false);assert(cold.yield()==OriginalR2Status::accepted);
    cold.frame.hands[1].trigger_active=1;cold.frame.hands[1].trigger=0;
    assert(cold.advance()==OriginalR2Status::baseline&&cold.calls.empty());
    cold.press();assert(cold.calls.size()==1);
    Trial up;up.press();assert(up.yield()==OriginalR2Status::released);
    up.context.game_r2_held=false;up.frame.hands[1].trigger_active=1;
    up.frame.hands[1].trigger=.35f;up.next();
    // Even missed tap edges on a released baseline cannot replay a shot.
    ++up.frame.hands[1].pressed[control_trigger];up.frame.hands[1].released[control_trigger]+=2;
    assert(up.advance()==OriginalR2Status::baseline&&up.calls.size()==2);
    up.frame.hands[1].trigger=1;up.press();assert(up.calls.size()==3);
    Trial foreign;foreign.context.game_r2_held=true;foreign.next();
    ++foreign.frame.hands[1].pressed[control_trigger];
    assert(foreign.advance()==OriginalR2Status::suppressed);
    assert(foreign.yield()==OriginalR2Status::suppressed&&foreign.calls.empty());
    foreign.context.game_r2_held=false;foreign.frame.hands[1].trigger_active=1;
    foreign.frame.hands[1].trigger=0;foreign.next();++foreign.frame.hands[1].released[control_trigger];
    assert(foreign.advance()==OriginalR2Status::baseline&&foreign.calls.empty());
    foreign.press();assert(foreign.calls.size()==1);
    for(unsigned state=0;state<4;++state){
        Trial held;held.press();assert(held.yield()==OriginalR2Status::released);
        held.context.game_r2_held=state==3;
        auto& hand=held.frame.hands[1];hand.trigger_active=state==2?0:1;
        hand.trigger=state==0?1.f:state==1?.5f:0.f;
        held.next();assert(held.advance()==OriginalR2Status::baseline);
        held.next();assert(held.advance()==OriginalR2Status::blocked&&held.calls.size()==2);
        held.next();++hand.pressed[control_trigger];++hand.released[control_trigger];
        assert(held.advance()==OriginalR2Status::blocked&&!held.fire.owns_press()&&held.calls.size()==2);
    }
}
void synchronous_yield_and_held_rearm(){
    Trial t;t.press();
    assert(t.yield()==OriginalR2Status::released&&!t.fire.owns_press()&&t.calls.size()==2);
    assert(t.yield()==OriginalR2Status::suppressed&&t.calls.size()==2); // Same stale game-held observation.
    t.context.game_r2_held=false;
    assert(t.yield()==OriginalR2Status::accepted&&t.calls.size()==2);
    assert(t.advance()==OriginalR2Status::rejected&&t.calls.size()==2); // No new XR frame.
    t.next();assert(t.advance()==OriginalR2Status::baseline&&t.calls.size()==2);
    t.next();assert(t.advance()==OriginalR2Status::blocked&&t.calls.size()==2); // Still holding.
    // A skipped release/press pair cannot establish the post-yield ordering.
    t.next();++t.frame.hands[1].pressed[control_trigger];++t.frame.hands[1].released[control_trigger];
    assert(t.advance()==OriginalR2Status::blocked&&!t.fire.owns_press());
    t.next();++t.frame.hands[1].released[control_trigger];
    assert(t.advance()==OriginalR2Status::accepted&&t.calls.size()==2);
    t.press();assert(t.calls.size()==3); // Distinct new release then press re-arms.
}
void allow_press_false_yields_before_freshness_and_preserves_rearm_gate(){
    Trial t;t.press();t.context.allow_press=false;t.frame.received_ns=800000000;
    assert(t.advance()==OriginalR2Status::released&&!t.fire.owns_press()&&t.calls.size()==2);
    assert(t.advance()==OriginalR2Status::rejected&&t.calls.size()==2);
    t.context.allow_press=true;t.context.game_r2_held=false;t.frame.received_ns=1000000000;
    ++t.frame.input_epoch;t.frame.hands[1].pressed[control_trigger]=0;
    t.frame.hands[1].released[control_trigger]=0;t.next();
    assert(t.advance()==OriginalR2Status::baseline);
    t.next();++t.frame.hands[1].pressed[control_trigger];
    assert(t.advance()==OriginalR2Status::blocked&&t.calls.size()==2);
    t.next();++t.frame.hands[1].released[control_trigger];
    assert(t.advance()==OriginalR2Status::accepted);t.press();assert(t.calls.size()==3);
}
void unsafe_identity_and_failed_release_are_sticky(){
    for(unsigned field=0;field<7;++field){
        Trial t;t.press();const auto certified=t.context;
        switch(field){
        case 0:++t.context.world;break;case 1:++t.context.controller;break;
        case 2:++t.context.weapon;break;case 3:t.context.ordinary_player=false;break;
        case 4:t.context.world=0;break;case 5:t.context.controller=0;break;
        case 6:t.context.weapon=0;break;
        }
        assert(t.yield()==OriginalR2Status::abort_required&&t.fire.owns_press()&&t.calls.size()==1);
        t.context=certified;
        assert(t.yield()==OriginalR2Status::abort_required&&t.advance()==OriginalR2Status::abort_required);
        assert(t.fire.owns_press()&&t.calls.size()==1);
    }
    for(unsigned safe=0;safe<3;++safe){
        Trial t;t.context.release_on_loss_safe=(safe&1)!=0;t.press();
        t.context.release_on_loss_safe=(safe&2)!=0;
        t.next();assert(t.advance()==OriginalR2Status::accepted); // Cannot overwrite safety at press.
        assert(t.yield()==OriginalR2Status::abort_required&&t.fire.owns_press()&&t.calls.size()==1);
        t.context.release_on_loss_safe=true;
        assert(t.yield()==OriginalR2Status::abort_required&&t.fire.owns_press());
    }
    Trial failed;failed.press();failed.succeeds=false;
    assert(failed.yield()==OriginalR2Status::abort_required&&failed.fire.owns_press()&&failed.calls.size()==2);
    failed.succeeds=true;
    assert(failed.yield()==OriginalR2Status::abort_required&&failed.advance()==OriginalR2Status::abort_required);
    assert(failed.fire.owns_press()&&failed.calls.size()==2);
    Trial blocked;blocked.press();blocked.context.allow_press=false;blocked.context.release_on_loss_safe=false;
    assert(blocked.advance()==OriginalR2Status::abort_required&&blocked.fire.owns_press()&&blocked.calls.size()==1);
}
void edges_while_stowed_do_not_rearm(){
    Trial t;t.press();t.context.allow_press=false;
    assert(t.advance()==OriginalR2Status::released);t.context.game_r2_held=false;
    t.next();++t.frame.hands[1].released[control_trigger];
    assert(t.advance()==OriginalR2Status::blocked);
    t.next();++t.frame.hands[1].pressed[control_trigger];
    assert(t.advance()==OriginalR2Status::blocked&&t.calls.size()==2);
    t.context.allow_press=true;t.next();assert(t.advance()==OriginalR2Status::baseline);
    t.next();assert(t.advance()==OriginalR2Status::blocked&&!t.fire.owns_press());
    t.next();++t.frame.hands[1].released[control_trigger];
    assert(t.advance()==OriginalR2Status::accepted);t.press();assert(t.calls.size()==3);
}
void foreign_presses_are_never_released(){
    Trial foreign;foreign.context.game_r2_held=true;
    assert(foreign.yield()==OriginalR2Status::suppressed&&foreign.calls.empty());
    assert(foreign.yield()==OriginalR2Status::suppressed&&!foreign.fire.owns_press());
    Trial shared;shared.context.game_r2_held=true;shared.next();
    ++shared.frame.hands[1].pressed[control_trigger];
    assert(shared.advance()==OriginalR2Status::suppressed);
    assert(shared.yield()==OriginalR2Status::suppressed&&shared.calls.empty());
    Trial already_up;already_up.press();already_up.context.game_r2_held=false;
    assert(already_up.yield()==OriginalR2Status::recovered&&!already_up.fire.owns_press());
    assert(already_up.calls.size()==1); // The game already released; do not send a second release.
}
} // namespace

int main(){
    StereoSample stereo{};stereo.session=7;stereo.sequence=1;
    stereo.space_epoch=3;stereo.predicted_time=1000;stereo.focused=1;
    ControllerInputFrame frame{};frame.session=7;frame.sequence=1;
    frame.space_epoch=3;frame.predicted_time=1000;frame.focused=1;
    frame.sync_ns=999999900;frame.received_ns=1000000000;
    frame.hands[1].grip_active=1;frame.hands[1].grip_flags=15;
    frame.hands[1].grip.orientation.w=1;
    OriginalR2Context context{11,12,13,true,false,true,true};
    OriginalR2Fire fire;std::vector<OriginalR2Action> calls;
    auto execute=[&](OriginalR2Action action,std::uint64_t pc){
        assert(pc==12);calls.push_back(action);return true;};
    auto advance=[&]{return fire.advance(frame,stereo,context,1000000100,execute);};
    assert(advance()==OriginalR2Status::baseline);
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::pressed&&fire.owns_press());
    context.game_r2_held=true;
    assert(advance()==OriginalR2Status::rejected&&calls.size()==1);
    ++frame.sequence;++stereo.sequence;
    assert(advance()==OriginalR2Status::accepted&&calls.size()==1);
    ++frame.sequence;++stereo.sequence;++frame.hands[1].released[control_trigger];
    assert(advance()==OriginalR2Status::released&&!fire.owns_press());
    context.game_r2_held=false;
    assert(calls.size()==2&&calls[0]==OriginalR2Action::press&&calls[1]==OriginalR2Action::release);
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    ++frame.hands[1].released[control_trigger];
    assert(advance()==OriginalR2Status::tapped&&calls.size()==4&&!fire.owns_press());
    context.game_r2_held=true;
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::suppressed&&calls.size()==4);
    context.game_r2_held=false;
    ++frame.sequence;++stereo.sequence;++frame.hands[1].released[control_trigger];
    assert(advance()==OriginalR2Status::suppressed&&calls.size()==4);
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::pressed&&fire.owns_press());
    context.game_r2_held=true;
    // A missed release/press pair while owned must replay in that order.
    ++frame.sequence;++stereo.sequence;++frame.hands[1].released[control_trigger];
    ++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::tapped&&fire.owns_press());
    assert(calls[calls.size()-2]==OriginalR2Action::release&&
           calls.back()==OriginalR2Action::press);
    ++frame.sequence;++stereo.sequence;++frame.hands[1].released[control_trigger];
    assert(advance()==OriginalR2Status::released&&!fire.owns_press());
    context.game_r2_held=false;
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::pressed&&fire.owns_press());
    context.game_r2_held=true;
    frame.received_ns=800000000; // stale tracked input: safe automatic weapon releases.
    assert(advance()==OriginalR2Status::recovered&&!fire.owns_press());
    context.game_r2_held=false;
    frame.received_ns=1000000000;
    ++frame.input_epoch;++frame.sequence;++stereo.sequence;
    frame.hands[1].pressed[control_trigger]=0;
    frame.hands[1].released[control_trigger]=0;
    assert(advance()==OriginalR2Status::baseline);
    context.release_on_loss_safe=false; // bow-like weapon cannot auto-release.
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::pressed);
    context.game_r2_held=true;
    frame.received_ns=800000000;
    assert(advance()==OriginalR2Status::abort_required&&fire.owns_press());
    auto before=calls.size();
    assert(advance()==OriginalR2Status::abort_required&&calls.size()==before);
    // Missing right-hand tracking must release an already owned safe weapon.
    fire.reset();context.release_on_loss_safe=true;context.game_r2_held=false;
    frame.received_ns=1000000000;++frame.input_epoch;++frame.sequence;++stereo.sequence;
    frame.hands[1].pressed[control_trigger]=frame.hands[1].released[control_trigger]=0;
    assert(advance()==OriginalR2Status::baseline);
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::pressed);
    context.game_r2_held=true;frame.hands[1].grip_flags=0;
    assert(advance()==OriginalR2Status::recovered&&!fire.owns_press());
    // Consumed but blocked/co-owned edges must not reappear on later frames.
    fire.reset();frame.hands[1].grip_flags=15;context.game_r2_held=true;
    ++frame.input_epoch;++frame.sequence;++stereo.sequence;
    frame.hands[1].pressed[control_trigger]=frame.hands[1].released[control_trigger]=0;
    assert(advance()==OriginalR2Status::baseline);
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    ++frame.hands[1].released[control_trigger];
    assert(advance()==OriginalR2Status::suppressed);
    context.game_r2_held=false;
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::pressed&&fire.owns_press());
    context.game_r2_held=true;
    ++frame.sequence;++stereo.sequence;++frame.hands[1].released[control_trigger];
    assert(advance()==OriginalR2Status::released);
    context.game_r2_held=false;context.allow_press=false;
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    assert(advance()==OriginalR2Status::blocked);
    context.allow_press=true;
    ++frame.sequence;++stereo.sequence;
    assert(advance()==OriginalR2Status::accepted&&!fire.owns_press());
    // A failed reflected action latches a fault instead of replaying it.
    ++frame.sequence;++stereo.sequence;++frame.hands[1].released[control_trigger];
    assert(advance()==OriginalR2Status::accepted);
    ++frame.sequence;++stereo.sequence;++frame.hands[1].pressed[control_trigger];
    auto failed=[&](OriginalR2Action,std::uint64_t){return false;};
    assert(fire.advance(frame,stereo,context,1000000100,failed)==OriginalR2Status::abort_required);
    assert(fire.advance(frame,stereo,context,1000000100,execute)==OriginalR2Status::abort_required);
    cold_yield_still_requires_baseline_release_and_press();
    baseline_release_proof_requires_active_low_trigger_and_game_key_up();
    synchronous_yield_and_held_rearm();
    allow_press_false_yields_before_freshness_and_preserves_rearm_gate();
    edges_while_stowed_do_not_rearm();
    unsafe_identity_and_failed_release_are_sticky();foreign_presses_are_never_released();
    std::puts("original R2 edges, safe synchronous yield, sticky abort and post-yield release/press rearm passed");
}
