// The authenticated original LongShot skeleton has Root0, LensCap1, Scope2,
// Bolt3. Reuse the current named-bone reader and existing animation owner.
namespace longshot_parts {
namespace hand=gears_weapon_hand;
namespace anim=hammerburst_animation;
gears_equipment_names::Lookup names({L"b_LS_Root",L"b_LS_Bolt",L"b_LS_Scope"});
bool read_bolt_pair(hand::Frame const& f,anim::Parts& out) noexcept {
    return anim::read_part_group(f,out,lancermain_asset::HeldAsset::longshot,names,
        {L"b_LS_Root",L"b_LS_Bolt",L"b_LS_Scope"},{0,3,2},2);
}
bool read(hand::Frame const& f,anim::Parts& out) noexcept {
    return anim::read_part_group(f,out,lancermain_asset::HeldAsset::longshot,names,
        {L"b_LS_Root",L"b_LS_Bolt",L"b_LS_Scope"},{0,3,2});
}
}
