// Metadata-only original arrow selection. The original MagazineMesh already
// carries the game's mesh/material; no cache, reconstructed projectile or
// flying-arrow actor is substituted. Selection grants no hand/ammo authority.
namespace torquebow_arrow_source {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
unsigned asset_gate{};
struct Binding {hand::ObjectIdentity weapon{},pawn{},component{},asset{};};
bool same(Binding const& a,Binding const& b) noexcept {
    return hand::same(a.weapon,b.weapon)&&hand::same(a.pawn,b.pawn)&&
        hand::same(a.component,b.component)&&hand::same(a.asset,b.asset);
}
bool retain(meta::Reader& r,meta::Property const& p) noexcept {
    for(unsigned i=0;i<r.result.field_count;++i)if(r.result.fields[i].identity==p.identity)
        return meta::same_property(r.result.fields[i],p);
    if(r.result.field_count==r.result.fields.size())return false;
    r.result.fields[r.result.field_count++]=p;return true;
}
bool retained(meta::Reader& r) noexcept {
    auto& schema=r.result;r.reuse_initial_identities=false;
    const auto count=schema.identity_count,structures=schema.structure_count,fields=schema.field_count;
    for(unsigned i=0;i<count;++i){unsigned slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(unsigned i=0;i<structures;++i){meta::Structure actual{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,actual)||
           !meta::same_structure(schema.structures[i],actual))return false;}
    for(unsigned i=0;i<fields;++i){meta::Property actual{};
        if(!r.property(schema.identities[schema.fields[i].identity].address,actual)||
           !meta::same_property(schema.fields[i],actual))return false;}
    for(unsigned i=0;i<schema.name_count;++i){meta::NameText actual{};
        if(!meta::fresh_name(schema.names,schema.name_text[i].index,actual)||
           !meta::same_name_text(schema.name_text[i],actual))return false;}
    meta::Table objects{},names{};
    return base==schema.module_base&&meta::tables(base,objects,names)&&
        meta::same_table(objects,schema.objects)&&meta::same_table(names,schema.names)&&schema.failure.reason==meta::Reason::none;
}
bool read(hand::Frame const& f,Binding& out) noexcept {
    asset_gate=1;if(!weaponhand::current_gt(nullptr))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    r.reuse_initial_identities=true; // Discovery only; existing fresh closing checks disable it.
    asset_gate=2;if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||!weaponhand::object(r,f.pawn)||
       !weaponhand::derives(r,f.weapon.address,L"Weap_Bow_base",L"WarfareGame"))return false;
    meta::PointerField instigator{},component{},asset{};Binding value{};value.weapon=f.weapon;value.pawn=f.pawn;
    asset_gate=3;if(!weaponhand::pointer(r,f.weapon.address,L"Instigator",f.pawn.address,instigator)||
       !weaponhand::read_pointer(r,f.weapon.address,L"MagazineMesh",component,true)||!component.value||
       !lancermain_asset::identify(r,component.value,value.component)||
       !weaponhand::derives(r,component.value,L"StaticMeshComponent",L"Engine")||
       value.component.outer!=f.weapon.address||
       !weaponhand::read_pointer(r,component.value,L"StaticMesh",asset)||!asset.value||
       !lancermain_asset::identify(r,asset.value,value.asset)||
       !weaponhand::derives(r,asset.value,L"StaticMesh",L"Engine"))return false;
    unsigned item{},package{};
    asset_gate=4;if(!r.path(asset.value,item)||!r.named(item,L"Locust_Torquebow_Rocket")||
       !r.path(schema.identities[item].outer,package)||!r.named(package,L"Locust_Torquebow")||
       schema.identities[package].outer||!weaponhand::derives(r,schema.identities[item].outer,L"Package",L"Core"))return false;
    r.reuse_initial_identities=false;
    meta::PointerField again_instigator{},again_component{},again_asset{};
    asset_gate=5;if(!weaponhand::pointer(r,f.weapon.address,L"Instigator",f.pawn.address,again_instigator)||
       !weaponhand::read_pointer(r,f.weapon.address,L"MagazineMesh",again_component,true)||
       !weaponhand::read_pointer(r,component.value,L"StaticMesh",again_asset)||
       !meta::same_pointer(instigator,again_instigator)||!meta::same_pointer(component,again_component)||
       !meta::same_pointer(asset,again_asset))return false;
    asset_gate=6;if(!retained(r))return false;
    out=value;asset_gate=0;return true;
}
}
