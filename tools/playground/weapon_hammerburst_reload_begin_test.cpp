// Execute the actual production begin adapter with owned deterministic
// original-call observations. No game process, memory writer or input.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace hand {struct Frame {unsigned owner{7};};}
namespace inv {
struct Inventory {unsigned identity{7},capacity{78},loaded{70},reserve{3};bool infinite_reserve{};
    bool valid()const{return identity&&capacity&&loaded<=capacity;}};
}
namespace lancerreload_state {
enum class BeginResult {rejected,acquired,recovery_required};
struct RewardReceipt {unsigned eligibility_owner{};bool eligibility_known{},eligible{},applied{};};
std::vector<unsigned> calls;
bool qualification_ok=true,qualified=true;
BeginResult original_result=BeginResult::acquired;
unsigned captured_loaded{},captured_reserve{},next_serial=11;
bool capture_reward_eligibility(inv::Inventory const& before,RewardReceipt& out) noexcept {
    calls.push_back(1);captured_loaded=before.loaded;captured_reserve=before.reserve;
    if(!qualification_ok)return false;
    out.eligibility_owner=before.identity;out.eligibility_known=true;out.eligible=qualified;return true;
}
BeginResult begin(hand::Frame const&,std::uint64_t& serial) noexcept {
    calls.push_back(2);if(original_result!=BeginResult::rejected)serial=next_serial++;
    return original_result;
}
}
#include "weapon_hammerburst_reload_begin.inl"

unsigned checks{},cases{};
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
void reset(){using namespace lancerreload_state;calls.clear();qualification_ok=qualified=true;original_result=BeginResult::acquired;}
int main(){
    using namespace lancerreload_state;
    for(unsigned loaded:{0u,70u}){
        reset();inv::Inventory before{};before.loaded=loaded;RewardReceipt reward{};std::uint64_t serial=99;
        C(reload_begin::acquire({},before,serial,reward)==BeginResult::acquired);
        C(calls==std::vector<unsigned>({1,2}));C(serial&&reward.eligibility_known&&reward.eligible);
        C(reward.eligibility_owner==before.identity&&captured_loaded==loaded&&captured_reserve==3);
        C(before.loaded==loaded&&before.reserve==3&&!reward.applied);++cases;
    }
    for(unsigned mode=0;mode<3;++mode){
        reset();inv::Inventory before{};before.loaded=mode==0?78:70;before.reserve=mode==1?0:3;
        before.infinite_reserve=mode==2; if(mode==2)before.reserve=0;
        RewardReceipt reward{};std::uint64_t serial=99;
        C(reload_begin::acquire({},before,serial,reward)==BeginResult::acquired);
        if(mode<2){C(calls.empty()&&serial==0&&!reward.eligibility_known);}
        else{C(calls==std::vector<unsigned>({1,2})&&serial&&reward.eligibility_known);}
        ++cases;
    }
    {
        reset();qualification_ok=false;RewardReceipt reward{};std::uint64_t serial=99;
        C(reload_begin::acquire({},inv::Inventory{},serial,reward)==BeginResult::rejected);
        C(calls==std::vector<unsigned>({1})&&serial==0&&!reward.eligibility_known);++cases;
    }
    {
        reset();original_result=BeginResult::rejected;RewardReceipt reward{};std::uint64_t serial{};
        C(reload_begin::acquire({},inv::Inventory{},serial,reward)==BeginResult::rejected);
        C(!reward.eligibility_known&&serial==0);
        original_result=BeginResult::acquired;qualified=false;
        C(reload_begin::acquire({},inv::Inventory{},serial,reward)==BeginResult::acquired);
        C(calls==std::vector<unsigned>({1,2,1,2})&&reward.eligibility_known&&!reward.eligible);++cases;
    }
    {
        reset();original_result=BeginResult::recovery_required;RewardReceipt reward{};std::uint64_t serial{};
        C(reload_begin::acquire({},inv::Inventory{},serial,reward)==BeginResult::recovery_required);
        C(serial&&reward.eligibility_known&&!reward.applied);++cases;
    }
    {
        reset();RewardReceipt reward{99,true,true,true};qualified=false;std::uint64_t serial{};
        C(reload_begin::acquire({},inv::Inventory{},serial,reward)==BeginResult::acquired);
        C(reward.eligibility_owner==7&&reward.eligibility_known&&!reward.eligible&&!reward.applied);++cases;
    }
    {
        reset();inv::Inventory invalid{};invalid.loaded=79;RewardReceipt reward{};std::uint64_t serial=99;
        C(reload_begin::acquire({},invalid,serial,reward)==BeginResult::rejected);
        C(calls.empty()&&serial==0&&!reward.eligibility_known);++cases;
    }
    std::printf("HammerburstReloadBeginTest: %u cases, %u checks passed\n",cases,checks);return 0;
}
