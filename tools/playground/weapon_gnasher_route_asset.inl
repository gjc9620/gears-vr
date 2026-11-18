// One stack-local metadata discovery. Reuse only identities already parsed
// by this same Reader, then explicitly disable reuse before the unchanged
// identity/structure/name/table closing reread. Close both concrete pointer
// descriptors and values explicitly: generic retained_metadata only closes
// descriptors that its caller actually registered in schema.fields. No result,
// descriptor, pointer value or permission survives this function.
bool route_asset_metadata(hand::Frame const& f,hand::ObjectIdentity const& component,
                          hand::ObjectIdentity const& asset_identity) noexcept {
    namespace meta=gripdamage_metadata;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    r.reuse_initial_identities=true;
    meta::PointerField mesh{},asset{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||
       !weaponhand::object(r,component)||!weaponhand::object(r,asset_identity)||
       !lancermain_asset::mesh_pointer(r,f.weapon.address,mesh)||mesh.value!=component.address||
       !weaponhand::pointer(r,mesh.value,L"SkeletalMesh",asset_identity.address,asset))return false;
    r.reuse_initial_identities=false;
    const std::array<meta::PointerField,2> links{mesh,asset};
    const std::array<uptr,2> owners{f.weapon.address,component.address};
    for(unsigned i=0;i<links.size();++i){
        meta::Property fresh{};uptr value{};
        if(!r.property(schema.identities[links[i].property.identity].address,fresh)||
           !meta::same_property(fresh,links[i].property)||
           !r.at(owners[i],links[i].property.offset,value)||value!=links[i].value)return false;
    }
    return unified_weapon_fire::retained_metadata(r);
}
