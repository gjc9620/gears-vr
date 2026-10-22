#include "hand_animation_observation.h"
#include <cassert>
#include <cstdio>
using namespace gears_hand_animation;
int main(){
    Ledger ledger;
    const auto request=ledger.enter(Kind::use_request,Source::bridge_invoke);
    const auto pickup=ledger.enter(Kind::grab_pickup,Source::hook);
    assert(request&&pickup&&pickup.parent==request.call&&pickup.depth==1);
    assert(request.pickup_request==1&&pickup.pickup_request==1);
    assert(ledger.bridge_mask()==(std::uint64_t{1}<<unsigned(Kind::use_request)));
    assert(ledger.hook_mask()==(std::uint64_t{1}<<unsigned(Kind::grab_pickup)));
    assert(ledger.leave(pickup)&&ledger.leave(request));
    const auto internal=ledger.enter(Kind::slot_play,Source::call_function);
    assert(internal&&internal.source==Source::call_function&&ledger.leave(internal));
    assert(ledger.call_mask()==(std::uint64_t{1}<<unsigned(Kind::slot_play)));
    assert(ledger.bridge_mask()==(std::uint64_t{1}<<unsigned(Kind::use_request)));
    ledger.poll({true,true,0,false});
    auto start=ledger.poll({true,true,53,false});
    assert(start.mask==1&&start.pickup_episode==1);
    auto replace=ledger.poll({true,true,42,false});
    assert(replace.mask==1&&replace.pickup_episode==2);
    ledger.poll({true,true,0,false});
    const auto melee=ledger.enter(Kind::melee_request,Source::bridge_invoke);
    assert(melee.melee_request==1&&ledger.leave(melee));
    auto melee_start=ledger.poll({true,true,0,true});
    assert(melee_start.mask==2&&melee_start.melee_episode==1);
    ledger.poll({true,true,0,false});
    assert(!ledger.ownership_proved());
    // Missing state reads and a later idle poll cannot create a proven release.
    ledger.poll({false,false,0,false});
    ledger.poll({true,true,0,false});
    assert(!ledger.ownership_proved()&&!ledger.started_active());
    Ledger late;late.poll({true,true,53,true});
    assert(late.started_active()&&!late.ownership_proved());
    Ledger bounded;
    for(unsigned i=0;i<max_calls;++i){
        const auto event=bounded.enter(Kind::body_end,Source::hook);
        assert(event&&bounded.leave(event));
    }
    assert(!bounded.enter(Kind::body_end,Source::hook)&&bounded.overflow());
    Ledger recursive;std::array<Token,max_depth> stack{};
    for(auto& event:stack){event=recursive.enter(Kind::body_play,Source::hook);assert(event);}
    assert(!recursive.enter(Kind::body_play,Source::hook)&&recursive.overflow());
    for(unsigned i=max_depth;i;--i)assert(recursive.leave(stack[i-1]));
    assert(!recursive.depth());
    Ledger wrong_order;
    const auto a=wrong_order.enter(Kind::melee_begin,Source::hook);
    wrong_order.enter(Kind::melee_end,Source::hook);
    assert(!wrong_order.leave(a)&&wrong_order.overflow());
    Ledger invalid;
    assert(!invalid.enter(Kind::count,Source::hook)&&invalid.overflow());
    std::puts("HandAnimationObservationTest passed: source isolation, correlation, missed boundaries and bounds");
}
