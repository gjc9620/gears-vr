// Private trial: retain ONLY an already released original finite-burst debt.
// This witness cannot supply a Frame, tracking sample, input or shot permit.
// The original event expiry and every dispatch/source check remain in force.
#if defined(GEARS_PLAYGROUND_HB_STALE_DEBT_WAIT) && GEARS_PLAYGROUND_HB_STALE_DEBT_WAIT
hand::ExistingActionLifetime released_lifetime{};
void clear_released_lifetime() noexcept {released_lifetime={};}
void remember_released_source(hand::Frame const& f) noexcept {
    released_lifetime={};
    weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,released_lifetime);
}
bool retain_stale_debt() noexcept {
    if(!released_lifetime.epoch||tx::first_debt||!tx::firing_debt||tx::cancel_requested||
       tx::completion.state()!=tx::policy::State::committed||!tx::completion.count()||tx::completion.count()>=6||
       vrfire::input.owns_press()||!targets(tx::certificate.held.weapon.address))return false;
    const auto withdrawal=weaponhand::client.withdrawal_on_gt();
    if(withdrawal.failure!=hand::FrameReadFailure::stale_before||
       withdrawal.frame<released_lifetime.parent.key.frame||
       !weaponhand::client.existing_action_lifetime_on_gt(released_lifetime))return false;
    static unsigned rows{};
    if(log_file&&rows<8){
        fwprintf(log_file,L"hammerburst_stale_debt_wait count=%u release_frame=%llu withdrawal_frame=%llu withdrawal_age_ns=%lld metadata_epoch=%llu no_dispatch=1\n",
            tx::completion.count(),released_lifetime.parent.key.frame,withdrawal.frame,withdrawal.input_age_ns,
            released_lifetime.epoch);
        fflush(log_file);++rows;
    }
    return true;
}
#else
void clear_released_lifetime() noexcept {}
void remember_released_source(hand::Frame const&) noexcept {}
bool retain_stale_debt() noexcept {return false;}
#endif
