// Bow-only one-shot player feedback on the EXISTING normal input dispatcher.
// It does not call BeginFire, reload, a stock repeat timer or any input setter.
namespace torquebow_feedback {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace input=torquebow_input;
gears_torque_bow_feedback::Intent intent;unsigned rows{};
void lost() noexcept {intent.lost();}
bool press_shoulder(hand::Frame const& f) noexcept {
    const auto& left=f.controllers.hands[0];
    return input::shoulder(f)&&gears_weapon::body_slot(nativeequipment::calibration,
        {f.stereo.head.position,f.reference.orientation},left.last_squeeze_press_grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;
}
bool empty(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    auto& backend=input::backend;const auto& last=backend.last;
    return game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&
        backend.last_known&&hand::same_frame(backend.last_source,f)&&last.original.ready&&last.fully_stopped&&
        last.loaded<=1&&!last.reserve&&input::driver.bound()&&input::driver.stage()==gears_weapon::TorqueBowStage::ready&&
        !input::pending()&&!input::driver.interaction().held_arrow_serial()&&backend.geometry_current(f)&&
        input::driver.inventory().loaded==last.loaded&&!input::driver.inventory().reserve;
}
void sample(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& decision,bool source_observed) noexcept {
    auto& backend=input::backend;
    bool slot_free=hand::same(f.key,decision.key)&&decision.press_token==f.press_token&&f.press_token&&
        decision.status==hand::DecisionStatus::no_candidate&&hand::lease_valid(decision.lease,f.key.lifecycle)&&
        decision.lease.kind==hand::LeaseKind::none;
    for(auto const& lease:game.object_incumbents)slot_free=slot_free&&lease.kind==hand::LeaseKind::none;
    const bool eligible=source_observed&&slot_free&&empty(f,game)&&press_shoulder(f);
    if(!intent.advance(f,backend.hold(f),backend.now(),eligible))return;
    // The physical press was consumed above, even if reflection/currentness
    // now fails. Only another real release/press can request another cue.
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,f))return;
    torquebow_empty_sound::Entry first{},closing{};input::driver_api::Snapshot before{},after{};
    if(!torquebow_empty_sound::read(f,first)||!backend.observe(f,game,before)||!empty(f,game)||
       !torquebow_empty_sound::read(f,closing)||!torquebow_empty_sound::same(first,closing)||
       !nativeequipment::current(nullptr,f))return;
    Request call{};call.object=f.weapon.address;call.function=first.function.address;call.size=0;
    const bool returned=invoke(&call);
    const bool verified=returned&&backend.observe(f,game,after)&&empty(f,game)&&
        before.loaded==after.loaded&&before.reserve==after.reserve&&nativeequipment::current(nullptr,f);
    if(log_file&&rows++<16){
        fwprintf(log_file,L"torquebow_no_ammo press=%llu frame=%llu sequence=%llu original_sync_ns=%lld returned=%u neutral_inventory_verified=%u loaded=%u reserve=%u cue=%llx no_fire_or_reload=1 replay=0\n",
            f.press_token,f.key.frame,f.controllers.sequence,f.controllers.sync_ns,unsigned(returned),unsigned(verified),
            before.loaded,before.reserve,first.cue.address);fflush(log_file);
    }
}
}
