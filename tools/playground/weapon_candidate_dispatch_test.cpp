#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cstdio>
#include "../vr-common/weapon_hand_client.h"
#define GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW 1
namespace {
FILE* log_file{};
std::array<unsigned,8> calls{};unsigned selected_producer{};
void candidate(unsigned slot,gears_weapon_hand::GripGameState& g) noexcept {
    ++calls[slot];if(slot!=selected_producer)g.magazine_observation_pending=true;
}
namespace nativelancerfollow {
inline bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& g) noexcept {g.mount_known=true;return true;}
}
#define PRODUCER(NAME,SLOT) namespace NAME { \
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& g) noexcept {candidate(SLOT,g);} \
inline void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {} }
PRODUCER(lancerreload_contact,0)
PRODUCER(hammerburst_runtime,1)
PRODUCER(snub_runtime,2)
PRODUCER(boltok_runtime,3)
PRODUCER(longshot_runtime,4)
PRODUCER(torquebow_runtime,5)
PRODUCER(gnasher_runtime,6)
PRODUCER(boomshot_runtime,7)
namespace boomshot_runtime {inline bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& g)noexcept {g.mount_known=true;return true;}}
namespace torquebow_runtime {
inline bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& g)noexcept {g.mount_known=true;return true;}
}
}
#include "weapon_hand_offline_fixture.h"
#define CHECK(v) do {if(!(v)){std::fprintf(stderr,"FAIL %u: %s\n",unsigned(__LINE__),#v);return 1;}}while(false)
int main() {
    constexpr const wchar_t* names[]{L"Weap_AssaultRifle",L"Weap_LocustAssaultRifle",L"Weap_COGPistol",L"Weap_LocustPistol",L"Weap_SniperRifle",L"Weap_Bow_base",L"Weap_Shotgun",L"Weap_Boomer_base"};
    unsigned cases{};
    for(selected_producer=0;selected_producer<8;++selected_producer){
        auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(names[selected_producer]));m->tables();calls={};
        abi::GripGameState g{};CHECK(m->read(g)&&g.known&&g.main_hand_held&&!g.magazine_observation_pending);
        for(unsigned i=0;i<8;++i)CHECK(calls[i]==unsigned(i==selected_producer));++cases;
        calls={};CHECK(m->read(g,true));for(auto n:calls)CHECK(n==0);++cases;
    }
    {auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(L"UnknownWeapon"));m->tables();calls={};
        abi::GripGameState g{};CHECK(m->read(g)&&g.known);for(auto n:calls)CHECK(n==0);++cases;}
    {auto m=std::make_unique<Memory>();m->tables();vrequip::model.reset();calls={};abi::GripGameState g{};
        CHECK(m->read(g)&&!g.main_hand_held);for(auto n:calls)CHECK(n==0);++cases;}
    {auto m=std::make_unique<Memory>();m->tables();fail_read=m->pawn+0x128;calls={};abi::GripGameState g{};
        CHECK(!m->read(g)&&!g.known);for(auto n:calls)CHECK(n==0);++cases;}
    std::printf("Production candidate observer: %u cases; exactly one owned producer, no foreign pending contamination; no gameplay grants tested\n",cases);
}
