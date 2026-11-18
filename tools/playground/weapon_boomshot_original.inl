// Original-instance preparation on normal GT. Nothing here grants a hand,
// changes inventory, executes a shot or substitutes the AI projectile class.
namespace boomshot_original {
namespace hand=gears_weapon_hand;
namespace meta=gripdamage_metadata;
template<std::size_t N,std::size_t K>
bool pointer(meta::Reader& r,uptr object,wchar_t const (&label)[N],
             wchar_t const (&kind)[K],meta::PointerField& out) noexcept {
    weaponhand::BoolField field{};
    if(!weaponhand::field(r,object,label,kind,8,field)||
       !r.at(object,field.property.offset,out.value))return false;
    out.property=field.property;out.owner=field.owner;return true;
}
struct Source {
    hammerburst_animation::Parts parts{};
    lancerreload_inventory::Counts counts{};
    hand::ObjectIdentity player_projectile{},carrier{},carrier_component{};
};
// Callback-safe data witness for an already prepared original component.
// No named getter, HandClient/current reentry, mutation or new action grant.
bool carrier_current(hand::Frame const& f,Source const& expected) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField component{},mesh{};
    return meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,f.weapon)&&
        weaponhand::object(r,expected.carrier_component)&&weaponhand::object(r,expected.carrier)&&
        pointer(r,f.weapon.address,L"MagazineMesh",L"ComponentProperty",component)&&component.value==expected.carrier_component.address&&
        pointer(r,component.value,L"StaticMesh",L"ObjectProperty",mesh)&&mesh.value==expected.carrier.address&&
        unified_weapon_fire::retained_metadata(r);
}
#ifndef BOOMSHOT_ORIGINAL_PREPARE_BOUNDARY_FIXTURE
bool prepare(hand::Frame const& f,Source& out) noexcept {
    out={};Source value{};
    if(!nativeequipment::current(nullptr,f)||!playground::instance(f.weapon.address,L"Weap_Boomer")||
       !boomshot_parts::read(f,value.parts)||!lancerreload_inventory::read(f,value.counts)||
       value.counts.capacity!=1)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField player{},ai{},magazine{},mesh{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||
       !pointer(r,f.weapon.address,L"BoomerProjectileClass",L"ClassProperty",player)||
       !pointer(r,f.weapon.address,L"BoomerProjectileAIClass",L"ClassProperty",ai)||
       !player.value||!ai.value||player.value==ai.value||
       !pointer(r,f.weapon.address,L"MagazineMesh",L"ComponentProperty",magazine)||!magazine.value||
       !weaponhand::read_pointer(r,magazine.value,L"StaticMesh",mesh)||!mesh.value||
       !lancermain_asset::identify(r,magazine.value,value.carrier_component)||
       !weaponhand::derives(r,magazine.value,L"StaticMeshComponent",L"Engine")||
       !lancermain_asset::identify(r,player.value,value.player_projectile)||
       !lancermain_asset::identify(r,mesh.value,value.carrier))return false;
    unsigned projectile{},projectile_kind{},projectile_package{},asset{},group{},package{};
    if(!r.path(player.value,projectile)||!r.named(projectile,L"Proj_Boomer")||
       !r.path(r.result.identities[projectile].klass,projectile_kind)||
       !weaponhand::core_type(r,projectile_kind,L"Class")||
       !r.path(r.result.identities[projectile].outer,projectile_package)||
       !r.named(projectile_package,L"warfaregamecontentweapons")||
       r.result.identities[projectile_package].outer||
       !r.path(mesh.value,asset)||!r.named(asset,L"Locust_Boomshot_Magazine")||
       !weaponhand::derives(r,mesh.value,L"StaticMesh",L"Engine"))return false;
    const auto mesh_group=r.result.identities[asset].outer;
    if(!mesh_group||!r.path(mesh_group,group)||!r.named(group,L"Mesh"))return false;
    const auto root=r.result.identities[group].outer;
    if(!root||!r.path(root,package)||!r.named(package,L"Locust_Boomshot")||
       r.result.identities[package].outer||!unified_weapon_fire::retained_metadata(r)||
       !nativeequipment::current(nullptr,f))return false;
    out=value;return true;
}
#endif
}
