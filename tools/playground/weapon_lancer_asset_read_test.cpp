#include "weapon_hand_offline_fixture.h"
namespace {
#include "weapon_lancer_asset_read.inl"
namespace la=lancermain_asset;
unsigned cases{};
struct AssetMemory:Memory {
    uptr component{},asset{},asset_class{},weapon_mesh_property{},hidden_property{},asset_property{};
    explicit AssetMemory(bool object_mesh=false){
        asset_class=cls(L"SkeletalMesh",engine);
        weapon_mesh_property=property(weapon_class,L"Mesh",object_mesh?object_type:component_type,0x118,8);
        hidden_property=property(mesh_class,L"HiddenGame",bool_type,0x110,4,2);
        asset_property=property(mesh_class,L"SkeletalMesh",object_type,0x120,8);
        component=node(L"LiveLancerMesh",mesh_class,weapon);asset=node(L"COGAssaultRifle",asset_class,engine);
        put(weapon,0x118,component);put(component,0x120,asset);tables();frame.key.thread_id=GetCurrentThreadId();
    }
    la::Status read_asset(la::Evidence& out){return la::read(frame,out);}
    void final_mutation(std::function<void()> action){
        const auto trigger=component+0x120;mutate_read=trigger;
        mutation=[trigger,action]{mutate_read=trigger;mutation=action;};
    }
};
void rejects(AssetMemory& m){
    la::Evidence out{m.identity(m.mesh),m.identity(m.weapon)};const auto before=out;
    assert(m.read_asset(out)!=la::Status::ready);
    assert(abi::same(out.component,before.component)&&abi::same(out.asset,before.asset));++cases;
}
template<class Change>void rejection(Change change){auto m=std::make_unique<AssetMemory>();change(*m);rejects(*m);}
}
int main(){
    // Actual Frag uses its own class/asset package and never gains the
    // existing firearm, muzzle, or Lancer mechanical eligibility.
    {AssetMemory m;const auto frag=m.cls(L"Weap_FragGrenade",m.warfare,m.weapon_class);
        const auto package=m.node(L"COG_Bolo_Grenade",m.package_type);
        Memory::put(m.weapon,0x50,frag);Memory::put(m.asset,0x48,m.text(L"Frag_Grenade"));
        Memory::put(m.asset,0x40,package);m.frame.weapon=m.identity(m.weapon);m.tables();
        la::Evidence out{};assert(la::read(m.frame,out,la::AssetScope::frag_hold)==la::Status::ready);
        assert(gears_weapon_hand::same(out.asset,m.identity(m.asset)));
        assert(la::read(m.frame,out,la::AssetScope::basic_hold)==la::Status::unsupported_weapon);
        assert(la::read(m.frame,out,la::AssetScope::supported_instant_fire)==la::Status::unsupported_weapon);
        Memory::put(package,0x48,m.text(L"OtherGrenadePackage"));m.tables();
        assert(la::read(m.frame,out,la::AssetScope::frag_hold)==la::Status::unsupported_asset);++cases;
    }
    {AssetMemory m;la::Evidence out{};
        assert(la::read(m.frame,out,la::AssetScope::frag_hold)==la::Status::unsupported_weapon);++cases;}
    for(bool object_mesh:{false,true}){auto m=std::make_unique<AssetMemory>(object_mesh);la::Evidence out{};
        assert(m->read_asset(out)==la::Status::ready&&abi::same(out.component,m->identity(m->component))&&
            abi::same(out.asset,m->identity(m->asset)));++cases;}
    rejection([](auto& m){const auto pistol=m.cls(L"Weap_Pistol",m.warfare,m.weapon_class);
        Memory::put(m.weapon,0x50,pistol);m.frame.weapon=m.identity(m.weapon);m.tables();});
    rejection([](auto& m){Memory::put(m.concrete,0x40,m.engine);});
    rejection([](auto& m){Memory::put(m.asset_class,0x40,m.warfare);});
    rejection([](auto& m){Memory::put(m.mesh_class,0x40,m.warfare);});
    rejection([](auto& m){Memory::put(m.asset,0x48,m.text(L"OtherRifle"));m.tables();});
    rejection([](auto& m){auto name=Memory::get<meta::RawName>(m.asset,0x48);name.number=1;Memory::put(m.asset,0x48,name);});
    rejection([](auto& m){Memory::put(m.asset,0x50,m.mesh_class);});
    rejection([](auto& m){Memory::put(m.component,0x50,m.asset_class);});
    rejection([](auto& m){Memory::put(m.component,0x48,m.text(L"Default__LancerMesh"));m.tables();});
    rejection([](auto& m){Memory::put(m.asset,0x48,m.text(L"Default__COGAssaultRifle"));m.tables();});
    rejection([](auto& m){Memory::put(m.weapon,0x48,m.text(L"Default__Rifle"));m.frame.weapon=m.identity(m.weapon);m.tables();});
    rejection([](auto& m){Memory::put(m.component,0x110,2u);});
    rejection([](auto& m){Memory::put(m.weapon,0x110,m.pc);});
    rejection([](auto& m){Memory::put(m.weapon,0x118,uptr(0));});
    rejection([](auto& m){Memory::put(m.component,0x120,uptr(0));});
    rejection([](auto& m){Memory::put(m.weapon_mesh_property,0x50,m.bool_type);});
    rejection([](auto& m){Memory::put(m.asset_property,0x50,m.component_type);});
    rejection([](auto& m){const auto fake=m.node(L"FakeCore",m.package_type);
        const auto type=m.node(L"ObjectProperty",m.class_type,fake);Memory::put(m.asset_property,0x50,type);m.tables();});
    rejection([](auto& m){++m.frame.mesh.name_number;});
    rejection([](auto& m){++m.frame.weapon.registry_index;});
    rejection([](auto& m){m.objects[m.frame.weapon.registry_index]=0;});
    rejection([](auto& m){m.frame.key.thread_id=GetCurrentThreadId()+1;});
    rejection([](auto&){game_thread.store(0);});
    rejection([](auto& m){fail_read=m.weapon+0x118;});
    rejection([](auto& m){fail_read=m.component+0x110;});
    rejection([](auto& m){fail_read=m.component+0x120;});
    rejection([](auto& m){fail_read=m.hidden_property+0xd0;});
    rejection([](auto& m){m.final_mutation([&m]{Memory::put(m.weapon,0x110,m.pc);});});
    rejection([](auto& m){m.final_mutation([&m]{Memory::put(m.weapon,0x118,m.mesh);});});
    rejection([](auto& m){m.final_mutation([&m]{Memory::put(m.component,0x120,uptr(0));});});
    rejection([](auto& m){m.final_mutation([&m]{Memory::put(m.asset_property,0x94,0x128u);});});
    rejection([](auto& m){m.final_mutation([&m]{Memory::put(m.mesh_class,0x88,504u);});});
    rejection([](auto& m){m.final_mutation([&m]{Memory::put(m.component,0x110,2u);});});
    rejection([](auto& m){m.final_mutation([&m]{Memory::put(m.hidden_property,0xd0,4u);});});
    rejection([](auto& m){m.final_mutation([&m]{Memory::put(m.asset,0x40,m.warfare);});});
    rejection([](auto& m){const auto name=Memory::get<meta::RawName>(m.asset,0x48);
        const auto entry=m.names[name.index];m.final_mutation([entry]{Memory::put(entry,20,wchar_t(L'X'));});});
    rejection([](auto& m){m.final_mutation([&m]{const auto index=Memory::get<unsigned>(m.asset,8);m.objects[index]=0;});});
    rejection([](auto& m){m.final_mutation([]{auto table=Memory::get<meta::Table>(base,0x1c24730);
        --table.capacity;Memory::put(base,0x1c24730,table);});});
    rejection([](auto& m){m.final_mutation([]{auto table=Memory::get<meta::Table>(base,0x1c34800);
        --table.capacity;Memory::put(base,0x1c34800,table);});});
    // An inserted earlier selector must be rejected even though the originally
    // selected Mesh descriptor and pointer are still individually unchanged.
    rejection([](auto& m){m.final_mutation([&m]{const auto previous=Memory::get<uptr>(m.weapon_class,0x80);
        const auto replacement=m.node(L"Mesh",m.component_type,m.weapon_class);
        Memory::put(replacement,0x70,1u);Memory::put(replacement,0x74,8u);Memory::put(replacement,0x94,0x118u);
        Memory::put(replacement,0x68,previous);Memory::put(m.weapon_class,0x80,replacement);m.tables();});});
    std::printf("WeaponLancerAssetReadTest: %u cases passed (owned memory only)\n",cases);return 0;
}
