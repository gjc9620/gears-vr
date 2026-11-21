#include "weapon_hand_offline_fixture.h"
namespace {
#include "weapon_lancer_asset_read.inl"
namespace unified_weapon_fire {
bool retained_metadata(gripdamage_metadata::Reader&) noexcept {return true;} // Existing common retention boundary only.
}
#include "weapon_torque_bow_projectile_class.inl"
struct ArrowClassMemory:Memory {
    uptr engine_weapon{},field{},inner{},arrow{},arrow_base{},class_property{},array_property{};
    std::array<uptr,2> projectiles{};
    ArrowClassMemory(){
        engine_weapon=cls(L"Weapon",engine,actor_class);put(weapon_class,0x60,engine_weapon);
        put(concrete,0x48,text(L"Weap_Bow_base"));
        array_property=node(L"ArrayProperty",class_type,core);class_property=node(L"ClassProperty",class_type,core);
        field=property(engine_weapon,L"WeaponProjectiles",array_property,0x140,16);
        inner=node(L"Inner",class_property,field);put(inner,0x70,1u);put(inner,0x74,8u);put(inner,0x94,0u);put(field,0xd0,inner);
        arrow_base=cls(L"Proj_Arrow_base",warfare,actor_class);
        arrow=cls(L"Proj_Arrow",node(L"warfaregamecontentweapons",package_type),arrow_base);
        projectiles[0]=arrow;put(weapon,0x140,meta::Table{reinterpret_cast<uptr>(projectiles.data()),1,2});tables();
    }
};
}
int main(){
    auto memory=std::make_unique<ArrowClassMemory>();auto& m=*memory;abi::ObjectIdentity cls{};
    assert(torquebow_projectile_class::read(m.frame,cls)&&cls.address==m.arrow);
    auto saved=m.projectiles[0];m.projectiles[0]=m.weapon_class;assert(!torquebow_projectile_class::read(m.frame,cls));m.projectiles[0]=saved;
    Memory::put(m.arrow,0x60,m.arrow);assert(!torquebow_projectile_class::read(m.frame,cls));Memory::put(m.arrow,0x60,m.arrow_base);
    Memory::put(m.inner,0x50,m.object_type);assert(!torquebow_projectile_class::read(m.frame,cls));Memory::put(m.inner,0x50,m.class_property);
    Memory::put(m.weapon,0x140,meta::Table{reinterpret_cast<uptr>(m.projectiles.data()),0,2});assert(!torquebow_projectile_class::read(m.frame,cls));
    Memory::put(m.weapon,0x140,meta::Table{reinterpret_cast<uptr>(m.projectiles.data()),1,2});
    Memory::put(m.arrow_base,0x40,m.engine);assert(!torquebow_projectile_class::read(m.frame,cls));Memory::put(m.arrow_base,0x40,m.warfare);
    mutate_read=reinterpret_cast<uptr>(m.projectiles.data());mutation=[&]{m.projectiles[0]=m.weapon_class;};
    assert(!torquebow_projectile_class::read(m.frame,cls));m.projectiles[0]=m.arrow;
    // A same-name derived override must not hide the original Engine.Weapon array.
    m.property(m.concrete,L"WeaponProjectiles",m.array_property,0x150,16);m.tables();
    assert(!torquebow_projectile_class::read(m.frame,cls));
    std::puts("Bow original mode0 projectile class: exact array owner/type, arrow ancestry, malformed array, cyclic/replaced class and override rejection passed; owned-memory, mock common-retention boundary.");
}
