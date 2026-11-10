// Private test observation after Input.advance. Reuses its exact original
// admission clock/source; no action, clock renewal, game query or input write.
{
    static std::int64_t last_consumed_log_ns{};
    static std::uint32_t last_consumed_down{},last_consumed_up{};
    static unsigned consumed_logs{};
    if(log_file&&nativelancerfollow::bound_is_hammerburst&&have_source&&consumed_logs<768){
        const auto& observed_hand=frame.controllers.hands[1];
        const auto observed_down=observed_hand.pressed[gears_vr::control_trigger];
        const auto observed_up=observed_hand.released[gears_vr::control_trigger];
        if(input_admission_ns>=last_consumed_log_ns&&
           (input_admission_ns-last_consumed_log_ns>=250000000||observed_down!=last_consumed_down||observed_up!=last_consumed_up)){
            fwprintf(log_file,L"hammerburst_input_consumed frame=%llu sequence=%llu current=%u status=%u source_status=%u owned=%u game_known=%u game_r2=%u trigger_active=%u trigger_value=%.3f trigger_pressed=%u trigger_released=%u input_sync_ns=%lld admitted_ns=%lld active_at_admission=%u preparation_budget=%u\n",
                frame.key.frame,frame.controllers.sequence,unsigned(current),unsigned(result),unsigned(input.source_status()),
                unsigned(input.owns_press()),unsigned(game.known),unsigned(game.r2_held),observed_hand.trigger_active,double(observed_hand.trigger),
                observed_down,observed_up,frame.controllers.sync_ns,input_admission_ns,
                unsigned(wh::frame_active(frame,input_admission_ns)),unsigned(preparation_budget));
            fflush(log_file);last_consumed_log_ns=input_admission_ns;last_consumed_down=observed_down;last_consumed_up=observed_up;++consumed_logs;
        }
    }
}
