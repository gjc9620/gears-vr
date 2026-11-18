// Read-only normal-GT preparation, never a cached animation/action permit.
namespace boomshot_inspection_source {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;namespace inv=lancerreload_inventory;
#include "weapon_boomshot_find_sequence.inl"
struct Source {
    hand::FrameKey key{};hand::ObjectIdentity component{},asset{},sequence{};
    Name reload_name{};inv::Counts counts{};
};
unsigned last_gate{};
bool read(hand::Frame const& f,Source& out) noexcept {
    last_gate=1;
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,f)||
       hammerburst_animation::ownership.active||lancerreload_state::lease.acquired)return false;
    inv::Counts counts{},after_counts{};bool reloading{};lancermain_asset::Evidence asset{},after_asset{};
    last_gate=2;
    if(!inv::read(f,counts)||counts.capacity!=1||
       (counts.used&&(counts.reserve||counts.infinite))||
       !lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading)||reloading||
       lancermain_asset::read(f,asset,lancermain_asset::AssetScope::boomshot_projectile)!=lancermain_asset::Status::ready)return false;
    meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
    weaponhand::BoolField named{};meta::RawName raw{},again{};meta::NameText label{};
    last_gate=3;
    if(!meta::tables(base,snapshot.objects,snapshot.names)||!weaponhand::object(r,f.weapon)||
       !weaponhand::field(r,f.weapon.address,L"WeaponReloadAnim",L"NameProperty",8,named)||
       !r.at(f.weapon.address,named.property.offset,raw)||raw.number||
       !meta::fresh_name(snapshot.names,raw.index,label)||!meta::exact_text(label,L"AR_Reload")||
       snapshot.field_count==snapshot.fields.size())return false;
    snapshot.fields[snapshot.field_count++]=named.property;
    playground::Args call(asset.component.address,L"FindAnimSequence");FindLayout before{},after{};
    last_gate=4;
    if(!find_schema(call,asset.component.address,r,before))return false;
    const Name animation_name{raw.index,raw.number};std::memcpy(call.r.params+before.input,&animation_name,8);
    if(!unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,f)||!call.run()||
       !find_schema(call,asset.component.address,r,after)||!(before==after))return false;
    uptr sequence{};std::memcpy(&sequence,call.r.params+before.result,8);Source candidate{};
    last_gate=5;
    if(!lancermain_asset::identify(r,sequence,candidate.sequence)||
       !boomshot_animation::inspection_no_notifies(r,candidate.sequence))return false;
    // Authenticate the actual original getter result a second time. Empty
    // serialized Notifies and a saved AnimSet name cannot substitute for it.
    std::memset(call.r.params+before.result,0,8);last_gate=6;
    if(!nativeequipment::current(nullptr,f)||!call.run()||!find_schema(call,asset.component.address,r,after)||!(before==after))return false;
    uptr repeated{};std::memcpy(&repeated,call.r.params+before.result,8);
    last_gate=7;
    if(repeated!=sequence||!boomshot_animation::inspection_no_notifies(r,candidate.sequence)||
       !r.at(f.weapon.address,named.property.offset,again)||again.index!=raw.index||again.number!=raw.number||
       !inv::read(f,after_counts)||!(counts==after_counts)||
       !lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading)||reloading||
       lancermain_asset::read(f,after_asset,lancermain_asset::AssetScope::boomshot_projectile)!=lancermain_asset::Status::ready||
       !hand::same(asset.component,after_asset.component)||!hand::same(asset.asset,after_asset.asset)||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,f)||
       hammerburst_animation::ownership.active||lancerreload_state::lease.acquired)return false;
    candidate.key=f.key;candidate.component=asset.component;candidate.asset=asset.asset;
    candidate.reload_name=animation_name;candidate.counts=counts;out=candidate;last_gate=0;return true;
}
}
