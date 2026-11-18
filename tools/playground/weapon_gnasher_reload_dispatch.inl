// Included inside gnasher_runtime. Existing original-state, inventory, hand
// arbiter and round-resource owners remain the only writers for their domains.
namespace reload {
bool enabled() noexcept {
    return profile.known&&profile.geometry.mechanical.feed_calibrated&&profile.geometry.feed_evidence&&
        profile.geometry.round.matches_feed(profile.geometry.mechanical);
}
bool shoulder(hand::Frame const& f) noexcept {
    return f.left_tracked&&f.left_grip_known&&f.left_grip_down&&
        weapon::body_slot(nativeequipment::calibration,{f.stereo.head.position,f.reference.orientation},
            f.controllers.hands[0].grip.position,0)==weapon::BodySlot::shoulder_ammo;
}
bool requests_ammo(hand::Frame const& f) noexcept {
    if(!enabled())return false;
    const auto* e=gnasher_fire::entry(f,false);
    if(!e||!(e->identity==inv::identity(f)))return false;
    // Metadata only in the source-reader callback: no Native/client/engine
    // query, new lease, inventory action or source timestamp is manufactured.
    if(gnasher_round::retains_ammo_route(f))return true;
    if(gnasher_inventory::blocks_fire(f.weapon)||e->mechanical.inventory_request().token||
       e->mechanical.cycle_request().token||e->mechanical.held_round_serial())return false;
    const auto stage=e->mechanical.stage();
    return (stage==weapon::GnasherMechanicalStage::ready||stage==weapon::GnasherMechanicalStage::feed_port||
            (stage==weapon::GnasherMechanicalStage::lever&&!e->mechanical.reload_active()))&&shoulder(f);
}
bool owns_reload(hand::Frame const& f) noexcept {
    if(!enabled())return false;
    const auto* r=gnasher_state::record(f);
    return r&&r->stage==gnasher_state::Stage::feeding&&r->identity==inv::identity(f)&&
        lancerreload_state::owns(f,r->serial);
}
bool withdraw_unused(hand::Frame const& f,gnasher_fire::Entry& e) noexcept {
    if(!enabled())return false;
    const auto item=e.mechanical.held_round_serial();
    if(!item||e.mechanical.inventory_request().token||gnasher_round::captured_for(f,item))return false;
    // A revoked grip cannot move/insert an item. Withdraw its exact display
    // before releasing only the private reservation; original ammo is intact.
    if(gnasher_round::release_for_stow(e.identity,item))
        (void)e.mechanical.release_round(e.mechanical.inventory().identity,item);
    return true;
}
bool dispatch(hand::Frame const& f,hand::Decision const& d,gnasher_fire::Entry& e) noexcept {
    if(!enabled()||!(e.identity==inv::identity(f))||!nativeequipment::current(nullptr,f))return false;
    (void)gnasher_inventory::retire_settled(f);
    const auto request=e.mechanical.inventory_request();
    if(request.token){
        auto* r=gnasher_state::record(f);
        if(request.input_sequence!=f.controllers.sequence||request.physical_completion_ns!=f.controllers.sync_ns){
            if(!gnasher_inventory::blocks_fire(f.weapon))(void)gnasher_inventory::reject_undispatched_round(f);
            return true;
        }
        if(!r||r->stage!=gnasher_state::Stage::feeding||!owns_reload(f)||
           !gnasher_round::show(f,d))return true;
        const auto result=gnasher_inventory::commit_round(f,r->serial,lancerreload_state::owns);
        if(result==gnasher_inventory::Result::rejected&&!gnasher_inventory::blocks_fire(f.weapon)&&
           e.mechanical.inventory_request().token)(void)gnasher_inventory::reject_undispatched_round(f);
        return true;
    }
    if(e.mechanical.held_round_serial()){
        (void)gnasher_round::show(f,d);return true;
    }
    if(gnasher_inventory::blocks_fire(f.weapon))return true;
    if(gnasher_state::fresh_ammo_grant(f,d)){
        if(!shoulder(f))return true;
        if(!e.mechanical.reload_active()){
            // Changing to original Reloading may replace this stopped shot
            // clip, but it never pays the shot's mechanical debt.
            if(gnasher_animation::owner.active){
                if(!gnasher_animation::release_after_exit(f,animation_serial))return true;
                animation_serial=0;curve_known=down_seen=false;
            }
            if(!gnasher_state::begin(f,d))return true;
        }
        if(!owns_reload(f)||!nativeequipment::current(nullptr,f))return true;
        auto proposed=e.mechanical;
        if(proposed.grab_round(proposed.inventory().identity,d.lease.id,weaponhand::clock_ns(nullptr)).event!=
           weapon::GnasherMechanicalEvent::round_grabbed)return true;
        // capture is the existing exact fresh shoulder-grant/resource check.
        // Publish its paired private reservation only after it succeeds.
        if(!gnasher_round::capture(f,d,d.lease.id,profile.geometry.round,profile.geometry.mechanical))return true;
        e.mechanical=proposed;(void)gnasher_round::show(f,d);return true;
    }
    if(e.mechanical.reload_active()&&gnasher_state::fresh_part_grant(f,d,hand::LeaseKind::support)){
        const auto previous=e.mechanical;
        auto proposed=previous;
        const auto close_intent=proposed.request_close(proposed.inventory().identity,weaponhand::clock_ns(nullptr));
        e.mechanical=proposed;
        if(close_intent.event==weapon::GnasherMechanicalEvent::reload_cancelled){
            (void)gnasher_state::close_empty(f,d);
            // Before original exit admission, close_empty performs read-only
            // checks and leaves feeding intact on rejection. Do not strand
            // that original owner with an already-cancelled mechanical reload.
            // Once exit is admitted, its existing settlement owns all retries.
            const auto* r=gnasher_state::record(f);
            if(r&&r->stage==gnasher_state::Stage::feeding)e.mechanical=previous;
        }
        // A newly accepted close intent does not also consume a lever stroke
        // in this Frame. The existing physical/actual-bone path follows next.
        return true;
    }
    return false;
}
} // namespace reload
