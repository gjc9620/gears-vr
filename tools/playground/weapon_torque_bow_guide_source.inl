// Typed read-only original Charge progress. The original GAME Tick, bonus
// multiplier, pending release and fatigue remain the sole timer consumers.
namespace torquebow_guide_source {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
struct Progress {double count{},maximum{};};
template<std::size_t N>bool charge_float(meta::Reader& r,hand::Frame const& f,wchar_t const(&label)[N],
    weaponhand::BoolField& field,float& out) noexcept {
    unsigned package{};
    return weaponhand::field(r,f.weapon.address,label,L"FloatProperty",4,field)&&
        r.named(field.owner,L"Weap_Bow_base")&&r.path(r.result.identities[field.owner].outer,package)&&
        r.named(package,L"WarfareGame")&&!r.result.identities[package].outer&&
        torquebow_arrow_source::retain(r,field.property)&&r.at(f.weapon.address,field.property.offset,out)&&std::isfinite(out);
}
bool progress(hand::Frame const& f,Progress& out) noexcept {
    out={};if(!weaponhand::current_gt(nullptr))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField count_field{},max_field{};float count{},maximum{},again_count{},again_max{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.weapon)||
       !weaponhand::derives(r,f.weapon.address,L"Weap_Bow_base",L"WarfareGame")||
       !charge_float(r,f,L"ChargeTimeCount",count_field,count)||!charge_float(r,f,L"MaxChargeTime",max_field,maximum)||
       count<0||maximum<=0||maximum>10000||count>100000||count_field.property.offset==max_field.property.offset||
       !torquebow_arrow_source::retained(r)||!r.at(f.weapon.address,count_field.property.offset,again_count)||
       !r.at(f.weapon.address,max_field.property.offset,again_max)||count!=again_count||maximum!=again_max||
       !torquebow_arrow_source::retained(r))return false;
    out={count,maximum};return true;
}
}
