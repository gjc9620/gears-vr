// Physical closure uses the original accepted GAME clock after a fully
// inserted magazine is resumed without another native ForceReload.
bool closure_clock(hand::Frame const& source,std::int64_t& tick,std::int64_t& accepted_begin,std::int64_t& perfect_end) noexcept {
    accepted_begin=perfect_end=0;
    if(!serial)return false;
    if(!native_exited){
        if(!lancerreload_state::closure_time(source,serial,tick))return false;
        accepted_begin=lancerreload_state::lease.accepted_begin_ms;
        perfect_end=lancerreload_state::lease.perfect_end_ms;return true;
    }
    if(!park_control::enabled()||!parked_plan.valid||!transaction.feed_present()||
       parked_native_lease.serial!=serial||!(parked_native_lease.identity==inv::identity(source))||
       !fresh_restore_main(source)||!geometry_current(source))return false;
    float game_now{};
    if(!restored_game_time(source,game_now)||!std::isfinite(game_now)||
       !std::isfinite(parked_native_lease.reward_start)||game_now<parked_native_lease.reward_start||
       parked_native_lease.accepted_begin_ms<=0)return false;
    const double elapsed=std::ceil((double(game_now)-double(parked_native_lease.reward_start))*1000.0);
    if(!std::isfinite(elapsed)||elapsed<0||elapsed>=double(INT64_MAX-parked_native_lease.accepted_begin_ms))return false;
    accepted_begin=parked_native_lease.accepted_begin_ms;
    perfect_end=parked_native_lease.perfect_end_ms;
    tick=accepted_begin+std::int64_t(elapsed);return true;
}
