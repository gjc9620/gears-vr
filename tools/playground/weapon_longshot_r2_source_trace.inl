// Existing tick POD only, after the original action. No queries or clocks.
{
    static unsigned longshot_source_rows{};
    static bool longshot_press_seen{};
    static std::array<std::uint64_t,10> longshot_source_previous{};
    if(log_file&&nativelancerfollow::bound_is_longshot&&longshot_source_rows<16){
        const auto& right=frame.controllers.hands[1];
        const auto pressed=have_source?right.pressed[gears_vr::control_trigger]:0;
        const auto released=have_source?right.released[gears_vr::control_trigger]:0;
        const std::array<std::uint64_t,10> key{{frame.world.address,frame.weapon.address,pressed,released,
            unsigned(have_source),unsigned(current),unsigned(preparation_budget),unsigned(muzzle_ready),
            unsigned(input.source_status()),unsigned(result)}};
        if((pressed||longshot_press_seen)&&key!=longshot_source_previous){
            longshot_press_seen=true;longshot_source_previous=key;++longshot_source_rows;
            fwprintf(log_file,L"longshot_r2_source frame=%llu weapon=%llx source_sequence=%llu source_sync_ns=%lld observed_ns=%lld copied=%u current=%u pressed=%u released=%u trigger=%.9g active=%u preparation_budget=%u muzzle_ready=%u source_status=%u result=%u continuity_matched=%u continuity_failure=%u copy_elapsed_ns=%lld action_elapsed_ns=%lld frame_now_ns=%lld r2_begin_ns=%lld r2_copy_done_ns=%lld muzzle_begin_ns=%lld held=%u animation=%u busy=%u press_ready=%u defer_eligible=%u awaiting_release_before=%u press_calls=%u execution_plus_one=%u diagnostic_only=1\n",
                frame.key.frame,static_cast<unsigned long long>(frame.weapon.address),frame.controllers.sequence,
                frame.controllers.sync_ns,now,unsigned(have_source),unsigned(current),pressed,released,
                double(right.trigger),right.trigger_active,unsigned(preparation_budget),unsigned(muzzle_ready),
                unsigned(input.source_status()),unsigned(result),unsigned(continuity_matched),unsigned(continuity_failure),
                copy_finished-source_started,action_finished-now,frame.now_ns,source_started,copy_finished,observation_started,
                longshot_dispatch_flags[0],longshot_dispatch_flags[1],longshot_dispatch_flags[2],longshot_dispatch_flags[3],
                longshot_dispatch_flags[4],longshot_dispatch_flags[5],longshot_dispatch_flags[6],longshot_dispatch_flags[7]);
            fflush(log_file);
        }
    }
}
