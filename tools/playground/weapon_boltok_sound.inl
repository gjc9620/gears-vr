// Dispatch existing original Boltok sound notifies, never mechanical motion.
namespace boltok_sound {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
inv::Identity identity{};std::array<std::uint64_t,5> tokens{};
gears_weapon::ReloadSupplyRejection empty_supply;
void event(hand::Frame const& f,unsigned kind,std::uint64_t token) noexcept {
    static const wchar_t* names[]={L"CockOpen",L"EjectClip",L"InsertClip",L"CockClose",L"CockCloseFinish"};
    if(kind>=tokens.size()||!token||!nativeequipment::current(nullptr,f)||
       !playground::instance(f.weapon.address,L"Weap_LocustPistol"))return;
    const auto current=inv::identity(f);if(!(identity==current)){identity=current;tokens={};}
    if(token<=tokens[kind])return;
    playground::Args call(f.weapon.address,names[kind]);
    const auto owner=call.valid?get<uptr>(call.r.function+0x40):0;
    const auto package=owner?get<uptr>(owner+0x40):0;
    if(!call.valid||call.r.size||!registered(owner)||!registered(package)||
       !playground::named(owner,L"Weap_LocustPistol")||!playground::named(package,L"WarfareGame")||
       get<uptr>(package+0x40)||!nativeequipment::current(nullptr,f))return;
    tokens[kind]=token; // Ambiguous original invocation is never replayed.
    const bool dispatched=call.run();
    if(log_file){fwprintf(log_file,L"boltok_sound kind=%u token=%llu dispatched=%u heard_verified=0\n",
        kind,token,unsigned(dispatched));fflush(log_file);}
}
void empty(hand::Frame const& f) noexcept {
    if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||!f.press_token||
       !nativeequipment::current(nullptr,f))return;
    const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
    if(gears_weapon::body_slot(nativeequipment::calibration,body,f.controllers.hands[0].grip.position,0)!=
       gears_weapon::BodySlot::shoulder_ammo||!empty_supply.reject(f.key.lifecycle,f.press_token))return;
    gears_weapon::EquipmentSnapshot cue{};cue.world=f.world.address;cue.pawn=f.pawn.address;cue.current_weapon=f.weapon.address;
    bool dispatched{};try{dispatched=vrequip::play_empty_slot_sound(cue,4);}catch(...){}
    if(log_file){fwprintf(log_file,L"boltok_empty_supply token=%llu sound=%u new_round_created=0 heard_verified=0\n",
        f.press_token,unsigned(dispatched));fflush(log_file);}
}
}
