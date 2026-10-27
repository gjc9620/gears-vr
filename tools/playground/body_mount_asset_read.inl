// Identity-only metadata carrier; original V1 logic and entry remain unchanged.
namespace lancermain_asset {
Status read_equipment(gears_weapon_hand::EquipmentIdentity const& f,Evidence& out) noexcept {
    const auto gt=weaponhand::current_gt(nullptr);
    if(!gt||(f.key.thread_id&&gt!=f.key.thread_id))return Status::wrong_thread;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!gears_weapon_hand::identity_valid(f.weapon)||!gears_weapon_hand::identity_valid(f.mesh)||
       !meta::tables(schema.module_base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||
       !weaponhand::object(r,f.mesh))return Status::invalid_identity;
    if(!weaponhand::derives(r,f.weapon.address,L"Weap_AssaultRifle",L"WarfareGame"))return Status::unsupported_weapon;
    std::array<meta::PointerField,3> links{};weaponhand::BoolField hidden{};Evidence candidate{};
    if(!f.pawn.address||!weaponhand::pointer(r,f.weapon.address,L"Instigator",f.pawn.address,links[0])||
       !mesh_pointer(r,f.weapon.address,links[1])||!links[1].value)return Status::invalid_link;
    if(!identify(r,links[1].value,candidate.component)||
       !weaponhand::derives(r,links[1].value,L"SkeletalMeshComponent",L"Engine")||
       !weaponhand::boolean(r,links[1].value,L"HiddenGame",hidden))return Status::invalid_component;
    if(hidden.bits&hidden.mask)return Status::hidden_component;
    if(!weaponhand::read_pointer(r,links[1].value,L"SkeletalMesh",links[2])||
       !identify(r,links[2].value,candidate.asset)||
       !weaponhand::derives(r,links[2].value,L"SkeletalMesh",L"Engine"))return Status::unsupported_asset;
    U32 asset_slot{};
    if(!r.path(candidate.asset.address,asset_slot)||!r.named(asset_slot,L"COGAssaultRifle"))return Status::unsupported_asset;
    // Repeat actual selectors too: a newly inserted earlier same-name field
    // must not leave the old selected descriptor looking valid in isolation.
    std::array<meta::PointerField,3> fresh_links{};weaponhand::BoolField fresh_hidden{};
    if(!weaponhand::read_pointer(r,f.weapon.address,L"Instigator",fresh_links[0])||
       !mesh_pointer(r,f.weapon.address,fresh_links[1])||
       !weaponhand::read_pointer(r,candidate.component.address,L"SkeletalMesh",fresh_links[2])||
       !weaponhand::boolean(r,candidate.component.address,L"HiddenGame",fresh_hidden))return Status::changed;
    for(U32 i=0;i<links.size();++i)if(!meta::same_pointer(links[i],fresh_links[i]))return Status::changed;
    if(hidden.object!=fresh_hidden.object||hidden.owner!=fresh_hidden.owner||hidden.mask!=fresh_hidden.mask||
       hidden.bits!=fresh_hidden.bits||!meta::same_property(hidden.property,fresh_hidden.property))return Status::changed;
    const auto identities=schema.identity_count,structures=schema.structure_count;
    for(U32 i=0;i<identities;++i){U32 slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return Status::changed;}
    for(U32 i=0;i<structures;++i){meta::Structure fresh{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,fresh)||
           !meta::same_structure(schema.structures[i],fresh))return Status::changed;}
    const std::array<uptr,3> objects{f.weapon.address,f.weapon.address,candidate.component.address};
    for(U32 i=0;i<links.size();++i){meta::Property fresh{};uptr pointer{};
        if(!r.property(schema.identities[links[i].property.identity].address,fresh)||
           !meta::same_property(links[i].property,fresh)||!r.at(objects[i],links[i].property.offset,pointer)||
           pointer!=links[i].value)return Status::changed;}
    meta::Property property{};U32 mask{},bits{};
    const auto hidden_property=schema.identities[hidden.property.identity].address;
    if(!r.property(hidden_property,property)||!meta::same_property(hidden.property,property)||
       !r.at(hidden_property,0xd0,mask)||mask!=hidden.mask||
       !r.at(hidden.object,hidden.property.offset,bits)||bits!=hidden.bits)return Status::changed;
    meta::Table objects_table{},names_table{};
    if(!meta::tables(schema.module_base,objects_table,names_table)||!meta::same_table(objects_table,schema.objects)||
       !meta::same_table(names_table,schema.names)||base!=schema.module_base||weaponhand::current_gt(nullptr)!=gt||
       r.result.failure.reason!=meta::Reason::none)return Status::changed;
    out=candidate;return Status::ready;
}
}
