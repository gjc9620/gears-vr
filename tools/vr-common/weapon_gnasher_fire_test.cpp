#include "weapon_gnasher_fire.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#v);std::exit(1);}}while(false)
using namespace gears_weapon;
int main(){
    constexpr GnasherIdentity id{1,2,3,4,5,6,7,8};
    GnasherInventory before{id,1,8,10},after{id,2,7,10};
    GnasherFireCycle cycle;CHECK(cycle.bind(before));CHECK(!cycle.blocked());
    CHECK(cycle.returned(10,1000,200));CHECK(cycle.blocked()&&cycle.pending());
    CHECK(!cycle.returned(10,1001,201)); // Duplicate root/aliased pellet receipt.
    CHECK(!cycle.confirm(before));CHECK(cycle.pending()); // No fabricated ammo proof.
    CHECK(cycle.confirm(after));CHECK(!cycle.pending()&&cycle.confirmed());
    const auto r=cycle.receipt();CHECK(r.original_event_sequence==10&&r.monotonic_ns==1000&&r.game_tick==200);
    CHECK(r.observed.loaded==7&&r.observed.reserve==10);CHECK(!cycle.confirm(after));
    CHECK(!cycle.request_cycle(9,11));CHECK(cycle.request_cycle(10,11));CHECK(!cycle.request_cycle(10,12));
    CHECK(!cycle.acknowledge_cycle(12,after));CHECK(cycle.blocked());
    auto changed=after;changed.reserve=9;CHECK(!cycle.acknowledge_cycle(11,changed));
    CHECK(cycle.acknowledge_cycle(11,after));CHECK(!cycle.blocked());
    CHECK(!cycle.acknowledge_cycle(11,after));CHECK(!cycle.returned(10,2000,300));
    CHECK(cycle.prepare(after));CHECK(cycle.returned(20,2000,300));
    auto next=after;next.loaded=6;next.revision=3;CHECK(cycle.confirm(next));
    CHECK(cycle.blocked()); // Timer/elapsed time alone has no clearing operation.
    GnasherFireCycle unexpected;CHECK(unexpected.bind(before));CHECK(unexpected.returned(1,1,1));
    CHECK(!unexpected.returned(2,2,2));CHECK(unexpected.faulted()&&unexpected.blocked());
    GnasherFireCycle wrong;CHECK(wrong.bind(before));CHECK(wrong.returned(1,1,1));
    changed=after;++changed.identity.world_generation;CHECK(!wrong.confirm(changed)&&wrong.pending());
    changed=after;changed.loaded=6;CHECK(!wrong.confirm(changed)&&wrong.faulted()&&wrong.blocked());
    GnasherFireCycle stale;CHECK(stale.bind(before));changed=before;--changed.revision;CHECK(!stale.prepare(changed));
    CHECK(stale.returned(1,1,1));changed=after;changed.revision=1;CHECK(!stale.confirm(changed));
    GnasherFireCycle rebound;CHECK(rebound.bind(before));CHECK(rebound.returned(1,1,1));
    auto fresh=after;++fresh.identity.profile_generation;
    CHECK(!rebound.rebind_observation(fresh));CHECK(rebound.pending());
    CHECK(rebound.confirm(after));CHECK(rebound.rebind_observation(fresh));
    CHECK(rebound.blocked()&&rebound.confirmed());
    CHECK(rebound.receipt().identity==id); // Historical evidence retains its original identity.
    CHECK(rebound.inventory().identity==fresh.identity);
    auto changed_actor=fresh;++changed_actor.identity.weapon;CHECK(!rebound.rebind_observation(changed_actor));
    auto changed_counts=fresh;++changed_counts.loaded;CHECK(!rebound.rebind_observation(changed_counts));
    CHECK(rebound.request_cycle(1,50));CHECK(!rebound.rebind_observation(fresh));
    CHECK(rebound.acknowledge_cycle(50,fresh));CHECK(!rebound.blocked());
    GnasherFireCycle uncertain;CHECK(!uncertain.unresolved_return(1));CHECK(uncertain.bind(before));
    CHECK(!uncertain.unresolved_return(0));CHECK(uncertain.unresolved_return(30));
    CHECK(uncertain.blocked()&&uncertain.faulted()&&!uncertain.confirmed()&&!uncertain.pending());
    CHECK(!uncertain.unresolved_return(30));CHECK(!uncertain.confirm(after));CHECK(!uncertain.prepare(before));
    CHECK(!uncertain.rebind_observation(before));CHECK(!uncertain.request_cycle(30,1));
    CHECK(!uncertain.acknowledge_cycle(1,before));CHECK(uncertain.inventory().loaded==8);

    const GnasherInventoryRequest insert{id,1,7,1,8,10,9,9};
    const GnasherInventoryReceipt inserted{1,true,{id,3,9,9}};
    for(unsigned field=0;field<12;++field){GnasherFireCycle guard;CHECK(guard.bind(before));auto req=insert;auto ack=inserted;
        if(field==0)req.token=0;if(field==1)req.round_serial=0;if(field==2)++req.before_revision;
        if(field==3)++req.before_loaded;if(field==4)++req.before_reserve;if(field==5)++req.after_loaded;
        if(field==6)++req.after_reserve;if(field==7)++ack.request_token;if(field==8)ack.accepted=false;
        if(field==9)ack.observed.revision=1;if(field==10)++ack.observed.identity.weapon;if(field==11)++ack.observed.loaded;
        CHECK(!guard.acknowledge_insert(req,ack));CHECK(!guard.blocked()&&guard.inventory().loaded==8);}
    GnasherFireCycle loading;CHECK(loading.bind(before));CHECK(loading.acknowledge_insert(insert,inserted));
    CHECK(loading.blocked()&&loading.reload_cycle_pending()&&!loading.confirmed());CHECK(!loading.acknowledge_insert(insert,inserted));
    CHECK(!loading.request_cycle(1,10));CHECK(loading.request_cycle(0,10));CHECK(loading.acknowledge_cycle(10,inserted.observed));
    CHECK(!loading.blocked()&&!loading.reload_cycle_pending()&&loading.prepare(inserted.observed));
    // A later reload does not borrow a previously completed historical shot.
    GnasherFireCycle history;CHECK(history.bind(before));CHECK(history.returned(10,1000,200));CHECK(history.confirm(after));
    CHECK(history.request_cycle(10,50));CHECK(history.acknowledge_cycle(50,after));
    const GnasherInventoryRequest refill{id,60,8,2,7,10,8,9};
    CHECK(history.acknowledge_insert(refill,{60,true,{id,4,8,9}}));
    CHECK(!history.confirmed()&&history.receipt().original_event_sequence==10&&history.reload_cycle_pending());
    CHECK(!history.request_cycle(10,61));CHECK(history.request_cycle(0,61));CHECK(history.acknowledge_cycle(61,{id,4,8,9}));
    std::puts("Gnasher original-shot consumer: root dedup, actual one-round receipt, retained cycle debt, identity/revision and one-time completion checks passed");
}
