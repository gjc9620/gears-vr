#include "weapon_torque_bow_original.h"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace gears_weapon;
using P=TorqueBowCancelPhase;using R=TorqueBowCallResult;
namespace {
constexpr TorqueBowIdentity id{1,2,3,4,5,6,7,8};
struct Backend {
    TorqueBowCancelSnapshot s{id,{1,1,1},true,false,true,true,false,false};
    std::vector<int> calls;int unknown{},reject{};bool current_owner{true};bool late_receipt{};P expected_late_step{P::leave_charge};
    bool current(TorqueBowIdentity const& owner,std::uint64_t serial){return current_owner&&owner==id&&serial==9;}
    bool read(TorqueBowIdentity const& owner,TorqueBowCancelSnapshot& out){if(!(owner==id))return false;out=s;return true;}
    R call(int n){calls.push_back(n);if(unknown==n)return R::unknown;if(reject==n)return R::rejected;return R::accepted;}
    R reconcile_applied(TorqueBowIdentity const&,std::uint64_t,P step){
        assert(step==expected_late_step);return late_receipt?R::accepted:R::unknown;
    }
    R stop_injection(TorqueBowIdentity const&,std::uint64_t){auto r=call(1);if(r==R::accepted)s.injection_stopped=true;return r;}
    R clear_owned_pending_fire(TorqueBowIdentity const&,std::uint64_t){assert(s.injection_stopped);auto r=call(2);if(r==R::accepted)s.pending_fire=false;return r;}
    R clear_owned_pending_end(TorqueBowIdentity const&,std::uint64_t){auto r=call(3);if(r==R::accepted){s.pending_end_fire=false;
        if(!s.charge)s.end_state_cleanup_verified=true;}return r;}
    R charge_ended(TorqueBowIdentity const&,std::uint64_t){return call(4);}
    R leave_charge(TorqueBowIdentity const&,std::uint64_t,TorqueBowCancelRoute){
        assert(!s.pending_fire&&!s.pending_end_fire&&s.injection_stopped);
        auto r=call(5);if(r==R::accepted){s.charge=false;s.end_state_cleanup_verified=true;}return r;
    }
};
}
int main(){
    {
        Backend b;b.s.charge=false;b.s.fire=true;b.s.inventory.loaded=0;
        b.s.original_input_owned=true;TorqueBowOriginalCancel no_proof;
        assert(!no_proof.begin(9,b.s,TorqueBowCancelRoute::active));
        b.s.original_projectile_accounted=true;TorqueBowOriginalCancel after_shot;
        assert(after_shot.begin(9,b.s,TorqueBowCancelRoute::active));
        assert(!after_shot.drain_on_gt(b)&&after_shot.phase()==P::verify);
        assert((b.calls==std::vector<int>{1,2,3})&&b.s.inventory.loaded==0);
        assert(!after_shot.drain_on_gt(b)&&b.calls.size()==3); // Wait for natural Fire completion, no transition/replay.
        b.s.fire=false;assert(after_shot.drain_on_gt(b)&&b.calls.size()==3);
    }
    {
        Backend b;b.s.charge=false;b.s.fire=true;b.s.inventory.loaded=0;
        b.s.original_input_owned=b.s.original_projectile_accounted=true;b.unknown=2;
        TorqueBowOriginalCancel after_shot;assert(after_shot.begin(9,b.s,TorqueBowCancelRoute::active));
        assert(!after_shot.drain_on_gt(b)&&after_shot.phase()==P::uncertain);
        b.expected_late_step=P::clear_pending;b.late_receipt=true;b.s.pending_fire=false;
        assert(!after_shot.reconcile_on_gt(b)&&after_shot.phase()==P::verify);
        assert((b.calls==std::vector<int>{1,2,3}));b.s.fire=false;assert(after_shot.drain_on_gt(b));
    }
    {
        Backend b;b.s.charge=false;TorqueBowOriginalCancel unowned;
        assert(!unowned.begin(9,b.s,TorqueBowCancelRoute::active));
        b.s.original_input_owned=true;TorqueBowOriginalCancel precharge;
        assert(precharge.begin(9,b.s,TorqueBowCancelRoute::active));assert(!precharge.began_in_charge());
        assert(precharge.drain_on_gt(b));assert((b.calls==std::vector<int>{1,2,3}));
    }
    {
        Backend b;b.s.charge=false;b.s.original_input_owned=true;b.unknown=3;TorqueBowOriginalCancel precharge;
        assert(precharge.begin(9,b.s,TorqueBowCancelRoute::active));assert(!precharge.drain_on_gt(b));
        b.expected_late_step=P::clear_end;b.late_receipt=true;b.s.pending_end_fire=false;b.s.end_state_cleanup_verified=true;
        assert(precharge.reconcile_on_gt(b));assert((b.calls==std::vector<int>{1,2,3}));
    }
    for(auto route:{TorqueBowCancelRoute::active,TorqueBowCancelRoute::put_down}){
        Backend b;TorqueBowOriginalCancel c;assert(c.begin(9,b.s,route));
        assert(c.drain_on_gt(b));assert(c.phase()==P::complete);assert((b.calls==std::vector<int>{1,2,3,4,5}));
        assert(!c.pending());assert(!c.service(b));assert(b.calls.size()==5);
    }
    for(int step=1;step<=5;++step){
        Backend b;b.unknown=step;TorqueBowOriginalCancel c;assert(c.begin(9,b.s,TorqueBowCancelRoute::active));
        assert(!c.drain_on_gt(b));assert(c.phase()==P::uncertain);auto count=b.calls.size();
        b.unknown=0;assert(!c.drain_on_gt(b));assert(b.calls.size()==count&&c.pending()); // no blind replay
    }
    Backend late;late.unknown=5;TorqueBowOriginalCancel pending;
    assert(pending.begin(9,late.s,TorqueBowCancelRoute::active));assert(!pending.drain_on_gt(late));
    assert(!pending.reconcile_on_gt(late));assert(late.calls.size()==5);
    late.s.charge=false;late.s.end_state_cleanup_verified=true;late.late_receipt=true;
    assert(pending.reconcile_on_gt(late));assert(pending.phase()==P::complete&&late.calls.size()==5);
    Backend foreign;TorqueBowOriginalCancel owned;assert(owned.begin(9,foreign.s,TorqueBowCancelRoute::active));
    foreign.current_owner=false;assert(!owned.drain_on_gt(foreign));assert(foreign.calls.empty()&&owned.pending());
    foreign.current_owner=true;foreign.s.identity.weapon=99;assert(!owned.drain_on_gt(foreign));assert(foreign.calls.empty());
    Backend shot;TorqueBowOriginalCancel cancel;assert(cancel.begin(9,shot.s,TorqueBowCancelRoute::active));
    shot.s.fire=true;assert(!cancel.drain_on_gt(shot));assert(cancel.phase()==P::already_fired&&shot.calls.empty());
    Backend drift;TorqueBowOriginalCancel conflict;assert(conflict.begin(9,drift.s,TorqueBowCancelRoute::active));
    --drift.s.inventory.loaded;assert(!conflict.drain_on_gt(drift));assert(conflict.phase()==P::uncertain&&drift.calls.empty());
    Backend reject;reject.reject=3;TorqueBowOriginalCancel retry;assert(retry.begin(9,reject.s,TorqueBowCancelRoute::active));
    assert(!retry.drain_on_gt(reject));assert(retry.phase()==P::clear_end);reject.reject=0;
    assert(retry.drain_on_gt(reject));assert((reject.calls==std::vector<int>{1,2,3,3,4,5}));
    std::puts("Torque Bow original cancellation contract passed; synthetic fault-injection backend only.");
}
