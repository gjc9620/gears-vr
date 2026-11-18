// Original selected Boomshot only. Normal GT named-bone reader, no setters.
namespace boomshot_parts {
namespace hand=gears_weapon_hand;
namespace anim=hammerburst_animation;
gears_equipment_names::Lookup names({L"Boomshot_Root",L"Boomshot_Magazine",L"Boomshot_Barrel"});
bool read(hand::Frame const& f,anim::Parts& out) noexcept {
    return anim::read_part_group(f,out,lancermain_asset::HeldAsset::boomshot,names,
        {L"Boomshot_Root",L"Boomshot_Magazine",L"Boomshot_Barrel"},{0,1,2});
}
}
