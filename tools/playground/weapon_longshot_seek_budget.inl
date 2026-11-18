// A conservative scheduling decision, never a longer source/hand lifetime.
// All original stroke checks run on a stack copy before this is considered.
// A defer commits no stroke observation, native pose, grant or endpoint.
static_assert(std::is_trivially_copyable_v<tracks::BoltStroke>);
bool insufficient_seek_budget(hand::Frame const& f,std::int64_t now) noexcept {
    constexpr std::int64_t lifetime=100000000,reserve=20000000;
    const auto input=f.controllers.sync_ns,frame=f.now_ns;
    if(input<=0||frame<=0||now<input||now<frame)return false;
    const auto input_age=now-input,frame_age=now-frame;
    if(input_age>lifetime||frame_age>lifetime)return false;
    return input_age>=lifetime-reserve||frame_age>=lifetime-reserve;
}
void trace_seek_deferred(hand::Frame const& f,tracks::BoltSample const& input,
    double wanted,std::int64_t previous,std::int64_t now) noexcept {
    static unsigned rows{};
    if(!log_file||rows>=24)return;++rows;
    fwprintf(log_file,L"longshot_seek_deferred frame=%llu sequence=%llu serial=%llu source_sync_ns=%lld frame_now_ns=%lld observed_ns=%lld input_age_ns=%lld frame_age_ns=%lld prior_stroke_sync_ns=%lld current_wanted=%.9g deferred_wanted=%.9g actual_progress=%.9g no_dispatch=1 no_stroke_commit=1 next_real_Frame_required=1\n",
        f.key.frame,f.controllers.sequence,generation,f.controllers.sync_ns,f.now_ns,now,
        now-f.controllers.sync_ns,now-f.now_ns,previous,stroke.wanted_progress(),wanted,input.actual_progress);
    fflush(log_file);
}
