#include "original_r2_fire.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace gears_vr;
using namespace gears_weapon;

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
    std::puts("original R2 edge ownership, rapid tap, co-input, stale recovery and bow abort gate passed");
}
