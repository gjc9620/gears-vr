#include "../vr-common/weapon_reload_transaction.h"
#include <cstdio>
#include <cstdlib>
namespace inv {
struct Identity {
    unsigned weapon{7},hold{1},world{11},session{13},input{17};
    bool valid()const noexcept{return weapon&&hold&&world&&session&&input;}
    bool operator==(Identity const& b)const noexcept{return weapon==b.weapon&&hold==b.hold&&world==b.world&&session==b.session&&input==b.input;}
};
using Inventory=gears_weapon::ReloadTxnInventory<Identity>;
using Transaction=gears_weapon::ReloadTransaction<Identity>;
using Request=gears_weapon::ReloadTxnRequest<Identity>;
bool counts_equal(Inventory const& a,Inventory const& b)noexcept{
    return a.identity==b.identity&&a.capacity==b.capacity&&a.loaded==b.loaded&&a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;
}
bool same_instance_across_hold(Identity const& a,Identity const& b)noexcept{
    return a.valid()&&b.valid()&&a.weapon==b.weapon&&b.hold>=a.hold&&a.world==b.world&&a.session==b.session&&a.input==b.input;
}
}
enum class Stage {idle,acquiring,opening,opened,old_magazine,waiting_ammo,new_magazine,closing,completing,recovering,parking,parked,restoring};
#include "weapon_hammerburst_reload_park_ledger.inl"
unsigned checks{},cases{};
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
namespace g=gears_weapon;
struct Fixture {
    inv::Inventory initial{{},1,78,70,3};inv::Transaction transaction;
    Fixture(){C(transaction.bind(initial));C(transaction.begin(initial,1,g::ReloadIntentSource::mechanism_open,1,g::ReloadTxnMode::magazine,78,0,1));}
    void reserve(){C(transaction.stow_original(1));C(transaction.reserve_magazine_reusing_loaded(2));}
    void insert(){reserve();C(transaction.request_insertion(2));inv::Request q{};C(transaction.take_request(q));
        auto current=q.expected;current.revision=2;C(transaction.acknowledge({q.token,true,current}));}
};
int main(){
    for(auto phase:{Stage::acquiring,Stage::opening,Stage::opened,Stage::old_magazine}){
        Fixture f;park_ledger::Plan p{};C(park_ledger::prepare(phase,f.transaction,1,0,false,p));
        C(p.valid&&p.original_at_feed&&p.transaction.stage()==g::ReloadTxnStage::open);
        C(p.transaction.inventory().loaded==70&&p.transaction.inventory().reserve==3);
        C(f.transaction.stage()==g::ReloadTxnStage::open&&!f.transaction.reserved());
        auto observed=p.transaction.inventory();observed.identity.hold=2;park_ledger::Plan rebound{};
        C(park_ledger::rebind(p,observed,1,rebound));C(rebound.transaction.inventory().identity.hold==2);
        C(!rebound.transaction.mechanical_ready()&&!rebound.transaction.pending());++cases;
    }
    {
        Fixture f;park_ledger::Plan p{};C(park_ledger::prepare(Stage::old_magazine,f.transaction,1,0,true,p));
        C(p.resume_stage==Stage::waiting_ammo&&!p.original_at_feed&&p.transaction.stage()==g::ReloadTxnStage::interrupted);
        C(p.transaction.inventory().loaded==70&&p.transaction.inventory().reserve==3);++cases;
    }
    {
        Fixture f;f.reserve();park_ledger::Plan p{};C(f.transaction.reserved()==3);
        C(park_ledger::prepare(Stage::new_magazine,f.transaction,1,2,false,p));
        C(p.resume_stage==Stage::waiting_ammo&&!p.transaction.reserved());
        C(p.transaction.inventory().loaded==70&&p.transaction.inventory().reserve==3);
        C(f.transaction.reserved()==3&&f.transaction.stage()==g::ReloadTxnStage::holding);++cases;
    }
    for(auto closing_time:{50ll,150ll}){
        Fixture f;f.insert();park_ledger::Plan p{};C(park_ledger::prepare(Stage::closing,f.transaction,1,0,false,p));
        C(p.resume_stage==Stage::closing&&p.transaction.inserted()==3&&p.transaction.feed_present());
        auto observed=p.transaction.inventory();observed.identity.hold=2;park_ledger::Plan rebound{};
        C(park_ledger::rebind(p,observed,1,rebound));
        C(!rebound.transaction.resume(observed,1,1));C(!rebound.transaction.resume(observed,3,2));
        C(rebound.transaction.resume(observed,3,1));C(rebound.transaction.confirm_closed(closing_time,100));
        inv::Request completion{};C(rebound.transaction.take_request(completion));
        C(completion.result==(closing_time<=100?g::ReloadTxnResult::perfect:g::ReloadTxnResult::ordinary));
        C(completion.expected.loaded==73&&completion.expected.reserve==0);++cases;
    }
    for(unsigned changed=0;changed<8;++changed){
        Fixture f;f.insert();park_ledger::Plan p{},rebound{};C(park_ledger::prepare(Stage::closing,f.transaction,1,0,false,p));
        auto observed=p.transaction.inventory();observed.identity.hold=2;
        if(changed==0)++observed.identity.weapon;if(changed==1)++observed.identity.world;
        if(changed==2)++observed.identity.session;if(changed==3)++observed.identity.input;
        if(changed==4)observed.identity.hold=0;if(changed==5)++observed.loaded;
        if(changed==6)++observed.reserve;if(changed==7)observed.infinite_reserve=true;
        C(!park_ledger::rebind(p,observed,1,rebound));C(!rebound.valid);++cases;
    }
    for(auto phase:{Stage::idle,Stage::waiting_ammo,Stage::closing,Stage::completing,Stage::recovering}){
        Fixture f;park_ledger::Plan p{};C(!park_ledger::prepare(phase,f.transaction,1,0,false,p));
        C(!p.valid&&f.transaction.inventory().loaded==70);++cases;
    }
    {
        Fixture f;f.reserve();C(f.transaction.request_insertion(2));park_ledger::Plan p{};
        C(!park_ledger::prepare(Stage::new_magazine,f.transaction,1,2,false,p));C(!p.valid&&f.transaction.pending());++cases;
    }
    std::printf("HammerburstParkLedgerTest: %u cases, %u checks; no engine/input/display operations\n",cases,checks);return 0;
}
