// Explain the same rejected read; never retry it, manufacture a Frame, or
// change release/cancellation. The withdrawal may predate this attempt, so
// retain both frame numbers rather than treating it as a causal verdict.
{
    static unsigned source_loss_rows{},idle_source_loss_rows{};
    const bool record_source_loss=hb_pending_before?source_loss_rows<12:(hb_bound_before&&idle_source_loss_rows<6);
    if(log_file&&record_source_loss&&(!have_source||!current)){
        fwprintf(log_file,L"hammerburst_source_loss pending_before=%u copy_ok=%u current=%u owned_before=%u result=%u copy_failure=%u copy_difference=%u copy_frame=%llu copy_sequence=%llu copy_age_ns=%lld commit_age_ns=%lld since_commit_ns=%lld copy_candidate_flags=%u withdrawal_failure=%u withdrawal_difference=%u withdrawal_frame=%llu withdrawal_sequence=%llu withdrawal_age_ns=%lld withdrawal_left_down=%u withdrawal_candidate_flags=%u withdrawal_lease=%u game_known=%u game_r2=%u admitted_ns=%lld\n",
            unsigned(hb_pending_before),unsigned(have_source),unsigned(current),unsigned(r2_owned_before),unsigned(result),
            unsigned(copy_observation.failure),copy_observation.game_difference,copy_observation.frame,copy_observation.sequence,
            copy_observation.input_age_ns,copy_observation.commit_check_input_age_ns,copy_observation.since_commit_check_ns,
            copy_observation.candidate.flags,unsigned(source_withdrawal.failure),source_withdrawal.game_difference,
            source_withdrawal.frame,source_withdrawal.sequence,source_withdrawal.input_age_ns,source_withdrawal.left_grip_down,
            source_withdrawal.candidate.flags,source_withdrawal.candidate.lease,unsigned(game.known),unsigned(game.r2_held),input_admission_ns);
        fflush(log_file);if(hb_pending_before)++source_loss_rows;else ++idle_source_loss_rows;
    }
}
