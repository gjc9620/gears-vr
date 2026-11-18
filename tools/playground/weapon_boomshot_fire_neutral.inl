// Included in vrfire after its existing input/actor readers. Read-only: no
// yield, edge dispatch, release, original state change or cached permission.
bool boomshot_inspection_neutral(wh::Frame const& frame) noexcept {
    if(mode.load(std::memory_order_acquire)!=running||!weaponhand::current_gt(nullptr)||
       !unified_weapon_fire::manual_profile()||!nativeequipment::current(nullptr,frame)||
       !playground::instance(frame.weapon.address,L"Weap_Boomer_base")||
       input.owns_press()||input.awaiting_release()||lancerrefire::has_initial_debt()||lancerrefire::release_required())return false;
    const auto& right=frame.controllers.hands[1];
    if(right.trigger_active!=1||!std::isfinite(right.trigger)||right.trigger<0||right.trigger>.35f)return false;
    lr::Game game{};
    if(!read_game(actors(frame),&frame,game)||!game.lancer||!game.r2_known||game.r2_held)return false;
    return mode.load(std::memory_order_acquire)==running&&!input.owns_press()&&!input.awaiting_release()&&
        !lancerrefire::has_initial_debt()&&!lancerrefire::release_required()&&nativeequipment::current(nullptr,frame);
}
