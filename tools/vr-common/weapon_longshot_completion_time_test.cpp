#include "weapon_longshot_reload.h"
#include "weapon_reload_transaction.h"
#include <cassert>
#include <cstdio>
struct Identity {unsigned value{1};constexpr bool valid()const{return value!=0;}
    constexpr bool operator==(Identity const& b)const{return value==b.value;}};
int main(){
    using namespace gears_weapon;
    for(auto elapsed:{std::int64_t(3999999),std::int64_t(4000000),std::int64_t(4000001),std::int64_t(8000000)}){
        ReloadTxnInventory<Identity> inventory{};inventory.identity={1};inventory.revision=1;inventory.capacity=1;inventory.reserve=5;
        ReloadTransaction<Identity> transaction;assert(transaction.bind(inventory));
        assert(transaction.begin(inventory,8,ReloadIntentSource::mechanism_open,7,ReloadTxnMode::round,1,1000000));
        assert(transaction.reserve_item(9)&&transaction.request_insertion(9));
        ReloadTxnRequest<Identity> insertion{},completion{};assert(transaction.take_request(insertion));
        auto after=insertion.expected;after.revision=insertion.before.revision+1;
        assert(after.loaded==1&&after.reserve==4&&transaction.acknowledge({insertion.token,true,after,ReloadTxnResult::none}));
        assert(transaction.confirm_closed(1000000+elapsed,longshot_perfect_duration_us-1)&&transaction.take_request(completion));
        assert(completion.result==(elapsed<4000000?ReloadTxnResult::perfect:ReloadTxnResult::ordinary));
    }
    std::puts("Longshot production ledger: actual shot to physical closure <4s perfect, >=4s ordinary; one-round counts conserved");
}
