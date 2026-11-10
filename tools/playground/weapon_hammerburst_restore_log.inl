// Bounded receipts for existing transitions. Inventory here is the private
// ledger; game acceptance additionally requires independent typed readback.
void log_restore(char const* phase,hand::Frame const* source=nullptr) noexcept {
#if defined(GEARS_PLAYGROUND_HB_PARK_RESTORE) && GEARS_PLAYGROUND_HB_PARK_RESTORE
    static unsigned rows{};
    if(!log_file||rows>=64)return;
    const auto counts=transaction.inventory();
    const bool native_owned=lancerreload_state::lease.acquired&&lancerreload_state::lease.serial==serial&&
        lancerreload_state::lease.identity==opening.identity;
    const auto& accepted=native_owned?lancerreload_state::lease:parked_native_lease;
    fwprintf(log_file,L"hammerburst_restore phase=%hs weapon=%llx world=%llx hold=%llu lifecycle=%llu serial=%llu old_item=%llu angle=%.9g stage=%u ledger_stage=%u loaded=%u reserve=%u capacity=%u feed=%u native_owned=%u accepted_begin_ms=%lld perfect_end_ms=%lld reward_start=%.9g uncertain=%u frame=%llu press=%llu left_known=%u left_down=%u left_tracked=%u source_sync_ns=%lld\n",
        phase,opening.identity.weapon.address,opening.identity.world.address,opening.identity.hold_generation,
        opening.identity.lifecycle,serial,old_item,angle,unsigned(stage),unsigned(transaction.stage()),
        counts.loaded,counts.reserve,counts.capacity,unsigned(transaction.feed_present()),unsigned(native_owned),
        accepted.accepted_begin_ms,accepted.perfect_end_ms,double(accepted.reward_start),unsigned(restore_native_uncertain),
        source?source->key.frame:0,source?source->press_token:0,source?source->left_grip_known:0,
        source?source->left_grip_down:0,source?source->left_tracked:0,source?source->controllers.sync_ns:0);
    fflush(log_file);++rows;
#else
    (void)phase;(void)source;
#endif
}
