// Existing sample/call results only. No Engine query, new authority or retry.
unsigned sample_gate{};
#if __has_include("weapon_source_validation_scope.inl")
void trace_validation(hand::Frame const& f,weaponhand::source_validation::Scope const& scope,unsigned phase) noexcept {
    static unsigned rows{},mask{};static std::uint64_t serial{};
    if(serial!=generation){serial=generation;mask=0;}
    if(!log_file||phase>6||rows>=8||(mask&(1u<<phase)))return;
    mask|=1u<<phase;++rows;const auto* proof=scope.proof;
    fwprintf(log_file,L"longshot_bolt_validation frame=%llu phase=%u serial=%llu calls=%u fallbacks=%u sealed=%u disabled=%u reads=%u ranges=%u bytes=%u failure=%u exact_fresh_rereads=1\n",
        f.key.frame,phase,generation,scope.validations,scope.fallbacks,unsigned(scope.sealed),unsigned(scope.disabled),
        proof?proof->count:0,proof?proof->merged:0,proof?proof->used:0,proof?unsigned(proof->failure):0);fflush(log_file);
}
#endif
void trace_motion(hand::Frame const& f,unsigned phase,bool valid,tracks::BoltSample const& sample,
                  V handle,std::int64_t previous,std::int64_t now) noexcept {
    static unsigned rows{},mask{};static std::uint64_t traced_serial{},traced_world{};static uptr traced_weapon{};
    if(!log_file||rows>=24||phase>6)return;
    if(traced_serial!=generation||traced_weapon!=f.weapon.address||traced_world!=f.key.world_generation){
        mask=0;traced_serial=generation;traced_weapon=f.weapon.address;traced_world=f.key.world_generation;
    }
    const unsigned bit=1u<<phase;if(mask&bit)return;mask|=bit;++rows;
    const auto source=valid?sample.stamp.observed_ns:f.controllers.sync_ns;
    const auto gap=previous>0&&source>=previous?source-previous:-1;
    const auto age=now>0&&source>0&&now>=source?now-source:-1;
    const auto physical_hand=sample.hand_in_root_m;
    fwprintf(log_file,L"longshot_bolt_motion phase=%u interrupted=%u weapon=%llx serial=%llu frame=%llu sequence=%llu source_sync_ns=%lld previous_stroke_sync_ns=%lld gap_ns=%lld age_ns=%lld frame_now_ns=%lld frame_age_ns=%lld sample_valid=%u sample_gate_cached=%u capture_gate_cached=%u action_gate_cached=%u curve_gate_cached=%u seek_pending_cached=%u actual_progress_before=%.9g wanted_progress=%.9g position_error_before=%.9g angle_error_before=%.9g hand=%.9g,%.9g,%.9g handle=%.9g,%.9g,%.9g grip_down=%u lease_current=%u ref=%llu space=%llu input_epoch=%llu cache_time=%.9g node_values_are_cached=1\n",
        phase,unsigned(phase==2||phase==3||phase==4),static_cast<unsigned long long>(f.weapon.address),generation,
        f.key.frame,f.controllers.sequence,source,previous,gap,age,f.now_ns,now>0&&f.now_ns>0&&now>=f.now_ns?now-f.now_ns:-1,unsigned(valid),sample_gate,longshot_clip::capture_gate,
        action_gate,longshot_animation::last_read.gate,unsigned(hammerburst_animation::ownership.seek_pending),
        valid?sample.actual_progress:-1.,stroke.wanted_progress(),valid?sample.position_error_m:-1.,valid?sample.orientation_error_degrees:-1.,
        physical_hand.x,physical_hand.y,physical_hand.z,handle.x,handle.y,handle.z,unsigned(sample.grip_down),unsigned(sample.lease_current),
        f.reference_generation,f.stereo.space_epoch,f.controllers.input_epoch,double(hammerburst_animation::ownership.last.time));fflush(log_file);
}
