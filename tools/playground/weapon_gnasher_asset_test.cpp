#ifdef NDEBUG
#undef NDEBUG
#endif
#include "weapon_hand_offline_fixture.h"
namespace {
#include "weapon_lancer_asset_read.inl"
struct AssetMemory:Memory {
    uptr component{},asset{};
    AssetMemory(){
        auto asset_class=cls(L"SkeletalMesh",engine);
        property(weapon_class,L"Mesh",component_type,0x118,8);
        property(mesh_class,L"HiddenGame",bool_type,0x110,4,2);
        property(mesh_class,L"SkeletalMesh",object_type,0x120,8);
        component=node(L"LiveGnasherMesh",mesh_class,weapon);
        asset=node(L"COG_Gnasher_Skel",asset_class,engine);
        put(weapon,0x118,component);put(component,0x120,asset);tables();frame.key.thread_id=GetCurrentThreadId();
    }
};
}
int main(){
    auto m=std::make_unique<AssetMemory>();
    const auto cls=m->cls(L"Weap_Shotgun",m->warfare,m->weapon_class);
    const auto package=m->node(L"COG_Gnasher",m->package_type);
    const auto group=m->node(L"Mesh",m->package_type,package);
    Memory::put(m->weapon,0x50,cls);Memory::put(m->asset,0x48,m->text(L"COG_Gnasher_Skel"));
    Memory::put(m->asset,0x40,group);m->frame.weapon=m->identity(m->weapon);m->tables();
    lancermain_asset::Evidence out{};
    using S=lancermain_asset::Status;using A=lancermain_asset::AssetScope;
    assert(lancermain_asset::read(m->frame,out,A::basic_hold)==S::ready);
    assert(lancermain_asset::read(m->frame,out,A::supported_instant_fire)==S::unsupported_weapon);
    assert(lancermain_asset::read(m->frame,out,A::lancer_only)==S::unsupported_weapon);
    assert(lancermain_asset::read(m->frame,out,A::frag_hold)==S::unsupported_weapon);
    assert(lancermain_asset::read(m->frame,out,A::gnasher_custom)==S::ready);
    assert(lancermain_asset::read(m->frame,out,A::supported_r2_fire)==S::ready);
    assert(lancermain_asset::read(m->frame,out,A::boomshot_projectile)==S::unsupported_weapon);
    assert(lancermain_asset::read(m->frame,out,A::torque_bow_projectile)==S::unsupported_weapon);
    Memory::put(group,0x48,m->text(L"OtherGroup"));m->tables();
    assert(lancermain_asset::read(m->frame,out,A::basic_hold)==S::unsupported_asset);
    Memory::put(group,0x48,m->text(L"Mesh"));Memory::put(package,0x48,m->text(L"OtherPackage"));m->tables();
    assert(lancermain_asset::read(m->frame,out,A::basic_hold)==S::unsupported_asset);
    Memory::put(package,0x48,m->text(L"COG_Gnasher"));Memory::put(m->asset,0x48,m->text(L"LongShot"));m->tables();
    assert(lancermain_asset::read(m->frame,out,A::basic_hold)==S::unsupported_asset);
    Memory::put(m->asset,0x48,m->text(L"COG_Gnasher_Skel"));Memory::put(m->weapon,0x50,m->concrete);
    m->frame.weapon=m->identity(m->weapon);m->tables();
    assert(lancermain_asset::read(m->frame,out,A::gnasher_custom)==S::unsupported_weapon);
    std::puts("Gnasher asset: exact original mesh/group/package required; dedicated custom/R2 scope never grants ordinary instant fire (12 cases)");
}
