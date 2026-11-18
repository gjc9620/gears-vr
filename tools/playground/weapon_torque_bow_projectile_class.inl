// Original WeaponProjectiles[0], the mode selected by Bow.Fire.BeginState.
// No call to ProjectileFire/GetProjectileClass and no replacement projectile.
namespace torquebow_projectile_class {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
bool arrow_class(meta::Reader& r,uptr cls) noexcept {
    std::array<uptr,meta::max_parents> visited{};unsigned count{};
    for(auto current=cls;current;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==current)return false;visited[count++]=current;
        meta::Structure s{};unsigned kind{};
        if(!r.structure(current,s)||!r.path(r.result.identities[s.identity].klass,kind)||
           !weaponhand::core_type(r,kind,L"Class"))return false;
        if(r.named(s.identity,L"Proj_Arrow_base")){
            unsigned package{};
            return r.path(r.result.identities[s.identity].outer,package)&&r.named(package,L"WarfareGame")&&
                !r.result.identities[package].outer;
        }
        current=s.super;
    }
    return false;
}
bool read(hand::Frame const& f,hand::ObjectIdentity& out) noexcept {
    out={};meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};meta::Table entries{},again{};uptr inner{},cls{},same_class{};meta::Property element{};unsigned owner_package{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||
       !weaponhand::derives(r,f.weapon.address,L"Weap_Bow_base",L"WarfareGame")||
       !weaponhand::field(r,f.weapon.address,L"WeaponProjectiles",L"ArrayProperty",16,field)||
       !r.named(field.owner,L"Weapon")||!r.path(schema.identities[field.owner].outer,owner_package)||
       !r.named(owner_package,L"Engine")||schema.identities[owner_package].outer||
       !r.at(f.weapon.address,field.property.offset,entries)||!entries.data||(entries.data&7)||
       entries.count<1||entries.count>32||entries.count>entries.capacity||entries.capacity>128||
       !r.at(schema.identities[field.property.identity].address,0xd0,inner)||!r.property(inner,element)||
       !weaponhand::core_type(r,element.type,L"ClassProperty")||element.element_bytes!=8||element.dimension!=1||
       !copy(&cls,entries.data,8)||!cls||!lancermain_asset::identify(r,cls,out)||!arrow_class(r,cls)||
       schema.field_count+2>schema.fields.size())return false;
    schema.fields[schema.field_count++]=field.property;schema.fields[schema.field_count++]=element;
    if(!r.at(f.weapon.address,field.property.offset,again)||!meta::same_table(entries,again)||
       !copy(&same_class,entries.data,8)||same_class!=cls||!unified_weapon_fire::retained_metadata(r))return false;
    return true;
}
}
