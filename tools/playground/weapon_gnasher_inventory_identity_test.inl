// Verbatim inventory identity excerpts from saved C29 production source.
namespace hand=gears_weapon_hand;
struct Identity {
    hand::ObjectIdentity world{},pawn{},weapon{};
    std::uint64_t game_start{},native_start{},registration{},lifecycle{},world_generation{},
        hold_generation{},session{},space{},input{};
    bool valid() const noexcept {
        return hand::identity_valid(world)&&hand::identity_valid(pawn)&&hand::identity_valid(weapon)&&
            game_start&&native_start&&registration&&lifecycle&&world_generation&&hold_generation&&session&&space&&input;
    }
    bool operator==(Identity const& b) const noexcept {
        return hand::same(world,b.world)&&hand::same(pawn,b.pawn)&&hand::same(weapon,b.weapon)&&
            game_start==b.game_start&&native_start==b.native_start&&registration==b.registration&&
            lifecycle==b.lifecycle&&world_generation==b.world_generation&&hold_generation==b.hold_generation&&
            session==b.session&&space==b.space&&input==b.input;
    }
};
Identity identity(hand::Frame const& f) noexcept {
    return {f.world,f.pawn,f.weapon,f.key.game_start,f.key.native_started_qpc,f.key.registration_cookie,
        f.key.lifecycle,f.key.world_generation,vrequip::model.hold_generation(),
        f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch};
}
bool same_instance_across_hold(Identity const& before,Identity const& after) noexcept {
    if(!before.valid()||!after.valid()||after.hold_generation<before.hold_generation||
       after.lifecycle<before.lifecycle)return false;
    // Old hand grants remain invalid. A fresh grab may rebind private saved
    // mechanics after the native hand lifecycle advances during a switch.
    auto normalized=after;normalized.hold_generation=before.hold_generation;
    normalized.lifecycle=before.lifecycle;
    return normalized==before; // World/session/input/native registration remain exact.
}
