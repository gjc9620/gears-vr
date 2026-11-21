#include <cstdio>
#include <cwchar>
#include "../vr-common/weapon_hand_client.h"
#define GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW 1
namespace {
namespace playground {inline bool instance(std::uintptr_t,wchar_t const*) noexcept;}
FILE* log_file{};
unsigned lancer_calls{},hb_calls{},snub_calls{},boltok_calls{},longshot_calls{};
void reset_calls(){lancer_calls=hb_calls=snub_calls=boltok_calls=longshot_calls=0;}
unsigned foreign_calls(){return lancer_calls+hb_calls+boltok_calls+longshot_calls;}
// Spies replace only candidate dependencies. They never arbitrate or grant a
// hand lease. The real production observer validates the artificial objects.
namespace nativelancerfollow {
inline bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& g) noexcept {g.mount_known=true;return true;}
}
namespace lancerreload_contact {
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& g) noexcept {++lancer_calls;g.magazine_observation_pending=true;}
inline void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace hammerburst_runtime {
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++hb_calls;}
}
namespace snub_runtime {
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++snub_calls;}
inline void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace boltok_runtime {
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++boltok_calls;}
inline void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace longshot_runtime {
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&) noexcept {++longshot_calls;}
inline void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&) noexcept {}
}
namespace torquebow_runtime {
inline bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState& g)noexcept {g.mount_known=true;return true;}
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&)noexcept {}
inline void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&)noexcept {}
}
namespace gnasher_runtime {
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&)noexcept {}
inline void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&)noexcept {}
}
namespace boomshot_runtime {
inline bool fill_support_mount(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&)noexcept {return false;}
inline void propose(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState&)noexcept {}
inline void committed(gears_weapon_hand::Frame const&,gears_weapon_hand::Decision const&)noexcept {}
}
}
#include "weapon_hand_offline_fixture.h"
namespace {namespace playground {
// The old frozen observer used this one game-adapter predicate. Resolve it
// with the same real metadata matcher, not a permissive constant. The current
// observer already uses that matcher directly. No engine call or grant.
inline bool instance(std::uintptr_t address,wchar_t const* label) noexcept {
    if(std::wcscmp(label,L"Weap_LocustAssaultRifle"))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    return meta::tables(base,schema.objects,schema.names)&&
        weaponhand::derives(r,address,L"Weap_LocustAssaultRifle",L"WarfareGame");
}
}}
#define CHECK(value) do {if(!(value)){std::fprintf(stderr,"FAIL line%u: %s; Snub=%u foreign=%u\n",unsigned(__LINE__),#value,snub_calls,foreign_calls());return 1;}}while(false)
int main(){
    unsigned cases{};
    {auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(L"Weap_COGPistol"));m->tables();
        reset_calls();abi::GripGameState g{};CHECK(m->read(g)&&g.known&&g.main_hand_held);
        CHECK(snub_calls==1&&foreign_calls()==0&&!g.magazine_observation_pending);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(L"Weap_COGPistol"));m->tables();
        reset_calls();abi::GripGameState g{};CHECK(m->read(g,true)&&g.known);
        CHECK(snub_calls==0&&foreign_calls()==0);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(L"Weap_COGPistol"));
        const auto foreign=m->node(L"ForeignWeaponPackage",m->package_type);Memory::put(m->concrete,0x40,foreign);m->tables();
        reset_calls();abi::GripGameState g{};CHECK(m->read(g)&&g.main_hand_held);CHECK(snub_calls==0);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(L"Weap_COGPistol"));m->tables();vrequip::model.reset();
        reset_calls();abi::GripGameState g{};CHECK(m->read(g)&&!g.main_hand_held);CHECK(snub_calls==0);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(L"Weap_COGPistol"));m->tables();fail_read=m->pawn+0x128;
        reset_calls();abi::GripGameState g{};CHECK(!m->read(g)&&!g.known);CHECK(snub_calls==0&&foreign_calls()==0);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(L"Weap_COGPistol"));m->tables();
        auto name=Memory::get<meta::RawName>(m->weapon,0x48);++name.number;Memory::put(m->weapon,0x48,name);
        reset_calls();abi::GripGameState g{};CHECK(!m->read(g));CHECK(snub_calls==0&&foreign_calls()==0);++cases;}
    {auto m=std::make_unique<Memory>();Memory::put(m->concrete,0x48,m->text(L"Weap_AssaultRifle"));m->tables();reset_calls();abi::GripGameState g{};
        CHECK(m->read(g)&&g.main_hand_held);CHECK(snub_calls==0&&lancer_calls==1);++cases;}
    std::printf("Snub candidate routing passed%u production-observer fixture cases; no gameplay grants tested\n",cases);
}
