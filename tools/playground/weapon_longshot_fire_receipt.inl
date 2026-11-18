// Passive consumer of an authenticated original CalcWeaponFire invocation.
// It records a possible shot time. A later GT read confirms ammo consumption
// before exposing that time to the manual reload transaction.
namespace longshot_fire {
namespace hand=gears_weapon_hand;
namespace inv=lancerreload_inventory;
struct Pending {
    inv::Identity identity{};
    gears_weapon::LongshotShot shot{};
    bool active{};
};
Pending pending{};
Pending confirmed{};
gears_weapon::LongshotReloadIdentity reload_identity(hand::Frame const& f) noexcept {
    return {f.world.address,f.pawn.address,f.weapon.address,f.key.world_generation,
        f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,
        f.controllers.profile_generation,vrequip::model.hold_generation()};
}
void trace(hand::Frame const& f,std::uint64_t shot) noexcept {
    if(!shot||!weaponhand::current_gt(nullptr)||!nativelancerfollow::bound_is_longshot||
       !nativelancerfollow::bound_key_known||!hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       !hand::frame_active(f,weaponhand::clock_ns(nullptr))||
       !weaponhand::client.registration_alive(f.key.registration_cookie,f.key.native_started_qpc))return;
    const auto id=inv::identity(f);
    if((pending.active&&pending.identity==id&&shot<=pending.shot.original_event_sequence)||
       (confirmed.active&&confirmed.identity==id&&shot<=confirmed.shot.original_event_sequence))return;
    float seconds{};
    if(!lancerreload_ui::object(f.world)||!lancerreload_ui::object(f.weapon)||
       !lancerreload_state::time_field(f.world.address,L"TimeSeconds",seconds))return;
    // Integer microseconds retain the original game-clock pause behavior.
    // Float precision of the source clock still limits runtime boundary data.
    const double micros=double(seconds)*1000000.;
    if(!std::isfinite(micros)||micros<0||micros>9e15)return;
    pending={id,{reload_identity(f),shot,static_cast<std::int64_t>(micros),1000000,false},true};
}
void service(hand::Frame const& f) noexcept {
    if(!pending.active)return;
    if(!(pending.identity==inv::identity(f))){pending={};return;}
    if(!nativeequipment::current(nullptr,f))return;
    int used{};
    if(!inv::integer(f.weapon.address,L"AmmoUsedCount",used))return;
    // Capacity was authenticated by the normal inventory reader. Query it
    // here only after an actual trace, outside the original call callback.
    inv::Counts counts{};
    if(!inv::read(f,counts)||counts.capacity!=1||used!=counts.used||used!=1)return;
    if(!nativeequipment::current(nullptr,f))return;
    confirmed=pending;confirmed.shot.confirmed=true;pending={};
    if(log_file){fwprintf(log_file,L"longshot_actual_fire_receipt shot=%llu game_tick_us=%lld used=%d capacity=%d weapon=%llx world=%llx pawn=%llx hold=%llu\n",
        confirmed.shot.original_event_sequence,confirmed.shot.game_tick,used,counts.capacity,
        static_cast<unsigned long long>(confirmed.shot.identity.weapon),static_cast<unsigned long long>(confirmed.shot.identity.world),
        static_cast<unsigned long long>(confirmed.shot.identity.pawn),confirmed.shot.identity.binding_generation);fflush(log_file);}
}
}
