#include "../vr-common/weapon_reload_transaction.h"
#include <cstdio>
#include <cstdlib>

namespace inv {
struct Identity {unsigned value{7};bool valid()const noexcept{return value!=0;}
    bool operator==(Identity const& b)const noexcept{return value==b.value;}};
using Inventory=gears_weapon::ReloadTxnInventory<Identity>;
using Request=gears_weapon::ReloadTxnRequest<Identity>;
using Transaction=gears_weapon::ReloadTransaction<Identity>;
bool counts_equal(Inventory const& a,Inventory const& b)noexcept{
    return a.identity==b.identity&&a.capacity==b.capacity&&a.loaded==b.loaded&&a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;
}
struct ObservedInventory {
    Inventory current{};bool available{true};unsigned reads{};
    bool sample_owned(Identity const& owner,Inventory& out)noexcept{
        ++reads;if(!available||!(owner==current.identity))return false;out=current;return true;
    }
};
}
namespace lancerreload_state {
struct RewardReceipt {bool dispatched{},applied{},call_confirmed{};};
unsigned new_reward_calls{},drain_calls{};
bool perfect_reward(inv::Request const&,inv::Inventory const&,inv::ObservedInventory&,RewardReceipt& r)noexcept{
    if(r.applied)return true;
    if(!r.dispatched){++new_reward_calls;return false;}
    ++drain_calls;if(!r.call_confirmed)return false;r.applied=true;return true;
}
}
#include "weapon_hammerburst_reload_cleanup.inl"

unsigned checks{},cases{};
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
namespace g=gears_weapon;
struct Fixture {
    inv::Transaction transaction;
    inv::Inventory opening{{7},1,78,70,3};
    inv::ObservedInventory observer;
    lancerreload_state::RewardReceipt reward{};
    Fixture(){lancerreload_state::new_reward_calls=lancerreload_state::drain_calls=0;
        C(transaction.bind(opening));C(transaction.begin(opening,1,g::ReloadIntentSource::mechanism_open,1,g::ReloadTxnMode::magazine,78,0,1));
        observer.current=opening;
    }
    inv::Request dispatch_insert(){
        C(transaction.stow_original(1));C(transaction.reserve_magazine_reusing_loaded(2));
        C(transaction.request_insertion(2));inv::Request request{};C(transaction.take_request(request));return request;
    }
    void committed_insert(){
        auto request=dispatch_insert();observer.current=request.expected;observer.current.revision=2;
        C(transaction.acknowledge({request.token,true,observer.current}));
    }
    void completion(bool perfect=true){committed_insert();C(transaction.confirm_closed(10,perfect?100:0));
        inv::Request request{};C(transaction.take_request(request));C(request.result==(perfect?g::ReloadTxnResult::perfect:g::ReloadTxnResult::ordinary));}
    bool settle(){return cleanup_completion::settle(transaction,observer,opening,reward);}
};
int main(){
    {
        Fixture f;f.completion();C(f.settle());
        C(f.transaction.stage()==g::ReloadTxnStage::interrupted&&f.transaction.result()==g::ReloadTxnResult::none);
        C(f.transaction.inventory().loaded==73&&f.transaction.inventory().reserve==0);
        const auto reads=f.observer.reads;for(unsigned i=0;i<10;++i)C(f.settle());
        C(f.observer.reads==reads&&lancerreload_state::new_reward_calls==0&&lancerreload_state::drain_calls==0);++cases;
    }
    {
        Fixture f;f.completion();f.reward.dispatched=true;
        C(!f.settle()&&f.transaction.pending());C(lancerreload_state::new_reward_calls==0);
        f.reward.call_confirmed=true;C(f.settle());C(f.reward.applied&&!f.transaction.pending());
        C(f.transaction.inventory().loaded==73&&f.transaction.inventory().reserve==0);
        C(f.transaction.result()==g::ReloadTxnResult::perfect&&lancerreload_state::drain_calls==2);
        C(f.settle()&&lancerreload_state::drain_calls==2&&lancerreload_state::new_reward_calls==0);++cases;
    }
    {
        Fixture f;f.completion();f.reward={true,true,true};C(f.settle());
        C(f.transaction.result()==g::ReloadTxnResult::perfect&&lancerreload_state::drain_calls==0&&lancerreload_state::new_reward_calls==0);++cases;
    }
    {
        Fixture f;f.completion();f.observer.current.loaded=74;C(!f.settle()&&f.transaction.pending());
        C(lancerreload_state::new_reward_calls==0&&lancerreload_state::drain_calls==0);++cases;
    }
    {
        Fixture f;f.completion();f.observer.available=false;C(!f.settle()&&f.transaction.pending());
        C(lancerreload_state::new_reward_calls==0);++cases;
    }
    for(unsigned committed=0;committed<2;++committed){
        Fixture f;auto request=f.dispatch_insert();
        if(committed){f.observer.current=request.expected;f.observer.current.revision=2;}
        C(f.settle());C(f.transaction.inventory().loaded==(committed?73u:70u));
        C(f.transaction.inventory().reserve==(committed?0u:3u));C(lancerreload_state::new_reward_calls==0);++cases;
    }
    {
        Fixture f;C(f.transaction.restore_original(1));C(f.transaction.confirm_closed(10,100));
        inv::Request request{};C(f.transaction.take_request(request));C(f.settle());
        C(f.transaction.result()==g::ReloadTxnResult::inspection&&f.transaction.inventory().loaded==70&&f.transaction.inventory().reserve==3);
        C(lancerreload_state::new_reward_calls==0);++cases;
    }
    {
        Fixture f;f.completion(false);C(f.settle());
        C(f.transaction.result()==g::ReloadTxnResult::ordinary&&f.transaction.inventory().loaded==73);
        C(lancerreload_state::new_reward_calls==0);++cases;
    }
    {
        Fixture f;f.completion();f.observer.current.identity.value=8;C(!f.settle()&&f.transaction.pending());
        C(lancerreload_state::new_reward_calls==0);++cases;
    }
    std::printf("HammerburstReloadCleanupTest: %u cases, %u checks passed; real transaction, no new reward dispatch\n",cases,checks);return 0;
}
