// Included after the owned cleanup routine. Parking releases all shared
// original/visual resources before equipment can stow this weapon. The saved
// transaction is private data, never an old hand grant or a live engine owner.
namespace park_control {
constexpr bool enabled() noexcept {
#if defined(GEARS_PLAYGROUND_HB_PARK_RESTORE) && GEARS_PLAYGROUND_HB_PARK_RESTORE
    return true;
#else
    return false;
#endif
}
bool owns_stow(hand::Frame const& source) noexcept {
    return enabled()&&stage!=Stage::idle&&hand::same(source.weapon,opening.identity.weapon);
}
bool pending_resources() noexcept {return stage!=Stage::idle&&stage!=Stage::parked;}
bool finish() noexcept {
    if(stage!=Stage::parking||!parked_plan.valid)return false;
    if(anim::ownership.active&&!(anim::ownership.inventory==opening.identity))return false;
    if(visual::active&&!hand::same(visual::saved.weapon,opening.identity.weapon))return false;
    // These routines drain their existing durable receipts on retry; no old
    // SetPosition, inventory insertion, reward or new original begin is sent.
    if(lancerreload_state::lease.acquired&&(!serial||lancerreload_state::lease.serial!=serial||
       !(lancerreload_state::lease.identity==opening.identity)))return false;
    if((lancerreload_state::lease.acquired&&!lancerreload_state::recover_owned())||!restore_animation()||!visual::restore())return false;
    inv::Inventory parked_observation{};
    if(!observer.sample_owned(opening.identity,parked_observation)||
       !inv::counts_equal(parked_observation,parked_plan.transaction.inventory()))return false;
    transaction=parked_plan.transaction;
    held={};stroke.reset();insertion.reset();extraction.reset();
    new_item=feed_item=feed_sequence=0;feed_ns=0;released=false;switch_requested=false;
    native_exited=true;stage=Stage::parked;
    lancerreload_runtime::outstanding.store(false,std::memory_order_release);
    log_restore("parked");
    return true;
}
bool begin() noexcept {
    if(!enabled()||restore_native_uncertain)return false;
    if(stage==Stage::parked)return true;
    if(stage==Stage::parking)return finish();
    if(stage==Stage::restoring){
        // Repark the rebound ledger unchanged; cosmetic restoration acquired
        // no native reload. Never replace its saved original timing lease.
        if(!parked_plan.valid||
           !inv::counts_equal(transaction.inventory(),parked_plan.transaction.inventory())||
           (anim::ownership.active&&!(anim::ownership.inventory==opening.identity))||
           (visual::active&&!hand::same(visual::saved.weapon,opening.identity.weapon)))return false;
        if(lancerreload_state::lease.acquired){
            if(!serial||lancerreload_state::lease.serial!=serial||!(lancerreload_state::lease.identity==opening.identity))return false;
            park_ledger::Plan resumed_plan{};
            if(!park_ledger::prepare(parked_plan.resume_stage,transaction,old_item,0,false,resumed_plan))return false;
            parked_plan=resumed_plan;parked_native_lease=lancerreload_state::lease;
        }
        stage=Stage::parking;return finish();
    }
    park_ledger::Plan proposed{};
    if(!park_ledger::prepare(stage,transaction,old_item,new_item,extraction.complete(),proposed))return false;
    const bool resumed_visual_owner=serial&&native_exited&&parked_plan.valid&&!lancerreload_state::lease.acquired&&
        parked_native_lease.serial==serial&&parked_native_lease.identity==opening.identity&&
        anim::ownership.active&&anim::ownership.visual_only&&anim::ownership.serial==serial;
    if(serial&&!resumed_visual_owner&&(!lancerreload_state::lease.acquired||lancerreload_state::lease.serial!=serial||!(lancerreload_state::lease.identity==opening.identity)))return false;
    if(!serial&&lancerreload_state::lease.acquired)return false;
    if(anim::ownership.active&&!(anim::ownership.inventory==opening.identity))return false;
    if(anim::ownership.seek_pending&&!anim::reconcile_seek())return false;
    if(!resumed_visual_owner)parked_native_lease=serial?lancerreload_state::lease:lancerreload_state::Lease{};
    parked_plan=proposed;stage=Stage::parking;
    // Publish the parking phase before original exit/display restoration.
    // A partial recovery keeps this plan and cannot lose committed ammunition.
    return finish();
}
bool prepare_stow(hand::Frame const& source,bool& changed) noexcept {
    changed=false;
    if(!owns_stow(source))return true;
    if(!nativeequipment::current(nullptr,source)||
       !inv::same_instance_across_hold(opening.identity,inv::identity(source)))return false;
    if(stage==Stage::parked)return true;
    if(!begin())return false;
    changed=true;return true; // Equipment must reacquire a fresh publication.
}
}
