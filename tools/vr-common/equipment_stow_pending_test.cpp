#include "equipment_stow_pending.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <limits>
using namespace gears_weapon;

namespace {
struct Key {
    std::uint64_t world{},weapon{},generation{};
    bool operator==(Key const& other) const noexcept {
        return world==other.world&&weapon==other.weapon&&generation==other.generation;
    }
};
using Pending=EquipmentStowPending<Key>;
constexpr Key owned{11,12,13};

void burst_wait_can_become_ready_only_once(){
    Pending request;
    assert(request.poll(true,owned,100,true,true,false)==StowPendingResult::cancelled);
    assert(request.begin(owned,100,1000)&&request.active()&&request.key()==owned);
    for(const auto now:{100u,100u,101u,700u})
        assert(request.poll(true,owned,now,false,true,false)==StowPendingResult::waiting);
    assert(request.poll(true,owned,701,true,true,false)==StowPendingResult::ready&&!request.active());
    assert(request.poll(true,owned,702,true,true,false)==StowPendingResult::cancelled);
    assert(request.begin(owned,703,10)); // A distinct explicit request may run.
    assert(request.poll(true,owned,703,true,true,false)==StowPendingResult::ready);
}

void unknown_or_replaced_identity_cannot_reappear_as_ready(){
    for(unsigned changed=0;changed<4;++changed){
        Pending request;assert(request.begin(owned,100,1000));
        auto foreign=owned;
        if(changed==1)++foreign.world;
        if(changed==2)++foreign.weapon;
        if(changed==3)++foreign.generation;
        assert(request.poll(changed!=0,foreign,101,true,true,false)==StowPendingResult::cancelled);
        assert(!request.active()&&request.key()==owned);
        assert(request.poll(true,owned,102,true,true,false)==StowPendingResult::cancelled);
    }
}

void authentication_clock_deadline_and_stop_precede_readiness(){
    for(unsigned failure=0;failure<4;++failure){
        Pending request;assert(request.begin(owned,100,10));
        assert(request.poll(true,owned,105,false,true,false)==StowPendingResult::waiting);
        const std::uint64_t now=failure==0?104:failure==1?110:106;
        assert(request.poll(true,owned,now,true,failure!=2,failure==3)==StowPendingResult::cancelled);
        assert(!request.active());
        assert(request.poll(true,owned,107,true,true,false)==StowPendingResult::cancelled);
    }
}

void repeated_polls_and_reentrant_begin_never_extend_the_deadline(){
    Pending request;assert(request.begin(owned,100,10));
    auto other=owned;++other.weapon;
    assert(!request.begin(other,105,1000)&&request.key()==owned&&request.active());
    for(unsigned i=0;i<20;++i)
        assert(request.poll(true,owned,109,false,true,false)==StowPendingResult::waiting);
    assert(request.poll(true,owned,110,true,true,false)==StowPendingResult::cancelled);
    assert(request.poll(true,owned,111,true,true,false)==StowPendingResult::cancelled);
}

void timeout_bounds_and_clock_overflow_are_rejected(){
    Pending request;
    constexpr auto maximum=std::numeric_limits<std::uint64_t>::max();
    assert(!request.begin(owned,0,0)&&!request.active());
    assert(!request.begin(owned,0,Pending::maximum_timeout_ms+1));
    assert(!request.begin(owned,0,maximum));
    assert(!request.begin(owned,maximum,1));
    assert(!request.begin(owned,maximum-999,1000));
    assert(request.poll(true,owned,0,true,true,false)==StowPendingResult::cancelled);
    assert(request.begin(owned,maximum-1000,1000));
    assert(request.poll(true,owned,maximum-1,false,true,false)==StowPendingResult::waiting);
    assert(request.poll(true,owned,maximum,true,true,false)==StowPendingResult::cancelled);
    assert(request.begin(owned,maximum-100,100));
    assert(request.poll(true,owned,maximum-1,false,true,false)==StowPendingResult::waiting);
    assert(request.poll(true,owned,0,true,true,false)==StowPendingResult::cancelled); // Wrapped clock.
    assert(request.begin(owned,0,1));
    assert(request.poll(true,owned,0,true,true,false)==StowPendingResult::ready);
}

void lifecycle_reset_discards_the_request_after_callbacks_drain(){
    Pending request;assert(request.begin(owned,100,1000));
    request.reset();assert(!request.active()&&request.key()==Key{});
    assert(request.poll(true,owned,101,true,true,false)==StowPendingResult::cancelled);
    assert(request.begin(owned,102,1));
    assert(request.poll(true,owned,103,true,true,false)==StowPendingResult::cancelled);
}
}

int main(){
    burst_wait_can_become_ready_only_once();
    unknown_or_replaced_identity_cannot_reappear_as_ready();
    authentication_clock_deadline_and_stop_precede_readiness();
    repeated_polls_and_reentrant_begin_never_extend_the_deadline();
    timeout_bounds_and_clock_overflow_are_rejected();
    lifecycle_reset_discards_the_request_after_callbacks_drain();
    std::puts("equipment stow pending waits within one identity/deadline and terminates before unsafe readiness");
}
