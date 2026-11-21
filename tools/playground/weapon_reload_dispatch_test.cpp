#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cwchar>
#include <cstdio>
using uptr=std::uint64_t;
namespace lancerreload_runtime {extern std::atomic<bool> outstanding;}
namespace fixture {
uptr selected{};bool shared_lease{};
std::array<unsigned,5> calls{},retired{},begins{},blocked{};
std::array<bool,5> old_pending{};
bool candidate(unsigned slot) noexcept {
    ++calls[slot];
    // Mock the unchanged original-state service: a pending old owner retains
    // its single lease even though candidate selection is independent.
    if(shared_lease)++blocked[slot];else ++begins[slot];
    return true;
}
void retire(unsigned slot,uptr) noexcept {
    if(old_pending[slot])++retired[slot];
    // Model a different just-finished owner clearing the legacy shared flag.
    lancerreload_runtime::outstanding.store(false);
}
void reset(uptr wanted) {
    selected=wanted;shared_lease=true;calls={};retired={};begins={};blocked={};
    old_pending.fill(true);if(wanted>=1&&wanted<=5)old_pending[unsigned(wanted-1)]=false;
}
}
namespace vrequip {struct Model {uptr held_weapon()const noexcept{return fixture::selected;}} model;}
namespace playground {
bool instance(uptr object,const wchar_t* name) noexcept {
    constexpr const wchar_t* classes[]{L"Weap_SniperRifle",L"Weap_LocustAssaultRifle",L"Weap_COGPistol",L"Weap_LocustPistol",L"Weap_AssaultRifle",L"Weap_Boomer_base"};
    return object>=1&&object<=6&&std::wcscmp(name,classes[object-1])==0;
}
}
#define CONSUMER(NAME,SLOT) namespace NAME { \
bool tick(uptr) noexcept{return fixture::candidate(SLOT);} \
bool pending_work() noexcept{return fixture::old_pending[SLOT];} \
void service_unselected(uptr,uptr selected) noexcept{fixture::retire(SLOT,selected);} }
CONSUMER(longshot_runtime,0)
CONSUMER(hammerburst_runtime,1)
CONSUMER(snub_runtime,2)
CONSUMER(boltok_runtime,3)
namespace lancerreload_runtime {
std::atomic<bool> outstanding{},stop_requested{};
bool pending_work() noexcept{return fixture::old_pending[4];}
void tick_selected(uptr) noexcept{fixture::candidate(4);}
void service_unselected(uptr,uptr selected) noexcept{fixture::retire(4,selected);}
}
namespace boomshot_runtime {
bool pending{};unsigned ticks{},services{};
bool tick(uptr)noexcept{++ticks;return pending;}
bool pending_work()noexcept{return pending;}
void service_unselected(uptr pc,uptr)noexcept{assert(pc==50);++services;}
}
namespace torquebow_runtime {void tick(uptr) noexcept{}}
namespace gnasher_state {
bool pending{},last_stopping{};unsigned services{};uptr last_selected{};
bool pending_work() noexcept{return pending;}
void service_unselected(uptr pc,uptr selected,bool stopping) noexcept{
 assert(pc==50);++services;last_selected=selected;last_stopping=stopping;
 lancerreload_runtime::outstanding.store(false); // Must be recomposed by the final flush.
}
}
#include "weapon_reload_dispatch.inl"
int main() {
    for(uptr selected=1;selected<=5;++selected){
        fixture::reset(selected);lancerreload_runtime::tick(50);
        assert(lancerreload_runtime::outstanding.load());
        for(unsigned i=0;i<5;++i){
            assert(fixture::calls[i]==unsigned(i==selected-1));
            assert(fixture::begins[i]==0);
            assert(fixture::blocked[i]==unsigned(i==selected-1));
            assert(fixture::retired[i]==unsigned(i!=selected-1));
        }
        fixture::shared_lease=false;lancerreload_runtime::tick(50);
        assert(fixture::begins[unsigned(selected-1)]==1);
    }
    for(uptr selected:{uptr(0),uptr(7)}){
        fixture::reset(selected);lancerreload_runtime::tick(50);
        for(unsigned i=0;i<5;++i){assert(fixture::calls[i]==0);assert(fixture::retired[i]==1);}
        assert(lancerreload_runtime::outstanding.load());
    }
    fixture::old_pending.fill(false);lancerreload_runtime::tick(50);
    assert(!lancerreload_runtime::outstanding.load());
    gnasher_state::pending=true;lancerreload_runtime::stop_requested=true;
    fixture::selected=0;lancerreload_runtime::tick(50);
    assert(lancerreload_runtime::outstanding.load()&&gnasher_state::last_selected==0&&gnasher_state::last_stopping);
    gnasher_state::pending=false;lancerreload_runtime::tick(50);
    assert(!lancerreload_runtime::outstanding.load());
    fixture::selected=0;boomshot_runtime::pending=true;const auto services=boomshot_runtime::services;lancerreload_runtime::tick(50);
    assert(lancerreload_runtime::outstanding.load()&&boomshot_runtime::services==services+1);
    boomshot_runtime::pending=false;fixture::selected=6;fixture::calls={};const auto ticks=boomshot_runtime::ticks;lancerreload_runtime::tick(50);
    assert(boomshot_runtime::ticks==ticks+1&&!lancerreload_runtime::outstanding.load());for(auto call:fixture::calls)assert(!call);
    std::puts("Production reload dispatch: one selected candidate; foreign retirement pending does not select it; shared lease still blocks new begin (mock services).");
}
