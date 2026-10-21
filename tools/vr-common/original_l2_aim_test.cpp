#include "original_l2_aim.h"
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
    frame.hands[0].grip_active=1;frame.hands[0].grip_flags=15;
    frame.hands[0].grip.orientation.w=1;
    OriginalL2Context context{11,12,13,true,false,false,true};
    OriginalL2Aim aim;std::vector<OriginalL2Action> calls;
    auto execute=[&](OriginalL2Action action,std::uint64_t pc){
        assert(pc==12);calls.push_back(action);return true;};
    auto advance=[&]{return aim.advance(frame,stereo,context,1000000100,execute);};
    auto next=[&]{++frame.sequence;++stereo.sequence;};
    assert(advance()==OriginalL2Status::baseline);
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::pressed&&aim.owns_press());
    context.game_target_key=true;
    next();assert(advance()==OriginalL2Status::accepted&&calls.size()==1);
    next();++frame.hands[0].released[control_trigger];
    assert(advance()==OriginalL2Status::released&&!aim.owns_press());
    context.game_target_key=false;
    assert(calls.size()==2&&calls[0]==OriginalL2Action::press&&
           calls[1]==OriginalL2Action::release);
    next();++frame.hands[0].pressed[control_trigger];
    ++frame.hands[0].released[control_trigger];
    assert(advance()==OriginalL2Status::tapped&&calls.size()==4&&!aim.owns_press());
    context.game_target_key=true;
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::suppressed&&calls.size()==4);
    context.game_target_key=false;
    next();++frame.hands[0].released[control_trigger];
    assert(advance()==OriginalL2Status::suppressed&&calls.size()==4);
    context.toggle_targeting=true;
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::blocked&&calls.size()==4);
    next();++frame.hands[0].released[control_trigger];
    assert(advance()==OriginalL2Status::accepted&&calls.size()==4);
    context.toggle_targeting=false;
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::pressed&&aim.owns_press());
    context.game_target_key=true;
    frame.received_ns=800000000;
    assert(advance()==OriginalL2Status::recovered&&!aim.owns_press());
    context.game_target_key=false;frame.received_ns=1000000000;
    ++frame.input_epoch;next();
    frame.hands[0].pressed[control_trigger]=frame.hands[0].released[control_trigger]=0;
    assert(advance()==OriginalL2Status::baseline);
    next();++frame.hands[0].pressed[control_trigger];
    assert(advance()==OriginalL2Status::pressed&&aim.owns_press());
    context.game_target_key=true;context.weapon=99;
    assert(advance()==OriginalL2Status::abort_required&&aim.owns_press());
    assert(advance()==OriginalL2Status::abort_required);
    std::puts("original L2 hold aim edges, tap, co-input, toggle gate and loss handling passed");
}
