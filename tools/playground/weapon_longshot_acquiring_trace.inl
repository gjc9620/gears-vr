// Bounded retrospective POD evidence only. Never queried by gameplay.
unsigned contact_observation_gate{};
struct AcquiringTrace {
    hand::Frame const& frame;unsigned before{unsigned(stage)},point{1};
    std::int64_t begin{weaponhand::clock_ns(nullptr)};bool copied{};
    explicit AcquiringTrace(hand::Frame const& f) noexcept:frame(f){contact_observation_gate=0;}
    ~AcquiringTrace() noexcept {
        static unsigned rows{};
        if(!log_file||!nativelancerfollow::bound_is_longshot||rows>=32||
           (before!=unsigned(Stage::acquiring)&&stage!=Stage::acquiring))return;
        const auto end=weaponhand::clock_ns(nullptr);++rows;
        fwprintf(log_file,L"longshot_acquiring_return before=%u after=%u point=%u contact_gate=%u copied=%u frame=%llu serial=%llu token=%llu source_sync_ns=%lld begin_ns=%lld end_ns=%lld elapsed_ns=%lld initial_age_ns=%lld final_age_ns=%lld frame_now_ns=%lld frame_age_ns=%lld left_grip=%u lease=%llu prepared=%u clip_owned=%u clip_stopped=%u\n",
            before,unsigned(stage),point,contact_observation_gate,unsigned(copied),frame.key.frame,serial,frame.press_token,
            frame.controllers.sync_ns,begin,end,end-begin,frame.controllers.sync_ns>0?begin-frame.controllers.sync_ns:-1,
            frame.controllers.sync_ns>0?end-frame.controllers.sync_ns:-1,frame.now_ns,frame.now_ns>0?end-frame.now_ns:-1,unsigned(frame.left_grip_down),held.id,
            unsigned(bolt::prepared),unsigned(longshot_clip::owned()),unsigned(hammerburst_animation::ownership.stopped));
        fflush(log_file);
    }
};
