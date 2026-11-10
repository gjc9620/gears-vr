#if defined(GEARS_PLAYGROUND_HB_SERVICE_LOSS_OBSERVE) && GEARS_PLAYGROUND_HB_SERVICE_LOSS_OBSERVE
{
    static unsigned service_loss_rows{};
    if(log_file&&service_loss_rows<8){
        const auto service_withdrawal=weaponhand::client.withdrawal_on_gt();
        fwprintf(log_file,L"hammerburst_service_loss gate=%u count=%u first_debt=%u firing_debt=%u cancel_before=%u client_phase=%u registration_alive=%u metadata_epoch=%llu disarm_reason=%u withdrawal_failure=%u withdrawal_difference=%u withdrawal_frame=%llu withdrawal_sequence=%llu withdrawal_age_ns=%lld withdrawal_left_down=%u withdrawal_candidate_flags=%u withdrawal_lease=%u\n",
            service_rejection,tx::completion.count(),unsigned(tx::first_debt),unsigned(tx::firing_debt),unsigned(tx::cancel_requested),
            unsigned(weaponhand::client.phase()),unsigned(weaponhand::client.registration_alive(tx::certificate.held.native_cookie,tx::certificate.held.native_started)),
            weaponhand::client.metadata_generation_on_gt(),weaponhand::client.last_disarm_reason(),
            unsigned(service_withdrawal.failure),service_withdrawal.game_difference,service_withdrawal.frame,
            service_withdrawal.sequence,service_withdrawal.input_age_ns,service_withdrawal.left_grip_down,
            service_withdrawal.candidate.flags,service_withdrawal.candidate.lease);
        fflush(log_file);++service_loss_rows;
    }
}
#endif
