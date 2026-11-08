#include "weapon_reload_transaction.h"
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Assertions required
#endif
using namespace gears_weapon;
struct Identity {unsigned value{1};constexpr bool valid()const{return value!=0;}
    constexpr bool operator==(Identity const& b)const{return value==b.value;}};
// Synthetic capacities exercise the shared ledger contract. They do not
// certify the actual HB instance capacity, clip or audio dispatcher.
int main(){
    unsigned cases{};
    for(unsigned capacity=1;capacity<=60;++capacity)for(unsigned loaded=0;loaded<=capacity;++loaded)
    for(unsigned reserve:{0u,1u,3u,100u}){
        ReloadTransaction<Identity> tx;ReloadTxnInventory<Identity> before{{1},1,capacity,loaded,reserve};
        ReloadTxnRequest<Identity> q{};const bool fills=loaded<capacity&&reserve;
        assert(tx.bind(before));assert(tx.begin(before,1,ReloadIntentSource::mechanism_open,fills?1:0,
            ReloadTxnMode::magazine,capacity,100,1));assert(tx.stow_original(1));
        if(!loaded&&!reserve){assert(!tx.reserve_magazine_reusing_loaded(2));continue;}
        assert(tx.reserve_magazine_reusing_loaded(2));
        const auto added=loaded<capacity?(capacity-loaded<reserve?capacity-loaded:reserve):0;
        assert(tx.held_magazine_rounds()==loaded+added);assert(tx.reserved()==added);
        if(!added){
            assert(tx.seat_reused_rounds(2));assert(!tx.take_request(q));
        }else{
            assert(tx.request_insertion(2));assert(tx.take_request(q));assert(!tx.take_request(q));
            assert(q.expected.loaded==loaded+added&&q.expected.reserve==reserve-added);
            auto after=q.expected;after.revision=2;
            assert(tx.acknowledge({q.token,true,after}));
        }
        assert(tx.inventory().loaded+tx.inventory().reserve==loaded+reserve);
        assert(tx.confirm_closed(200,0));
        if(!added){assert(!tx.take_request(q));assert(tx.result()==ReloadTxnResult::inspection);assert(tx.mechanical_ready());++cases;continue;}
        assert(tx.take_request(q));
        const auto result=added?ReloadTxnResult::ordinary:ReloadTxnResult::none;
        if(!added)assert(q.call==ReloadTxnCall::exit_without_fill&&q.result==ReloadTxnResult::inspection);
        assert(tx.acknowledge({q.token,true,tx.inventory(),result}));assert(tx.mechanical_ready());++cases;
    }
    ReloadSupplyRejection empty;
    assert(empty.reject(1,1));assert(!empty.reject(1,1));assert(empty.reject(1,2));
    assert(!empty.reject(1,2));assert(empty.reject(2,1));
    std::printf("HB supply ledger: %u conservation/completion cases; zero supply and press deduplication passed\n",cases);
}
