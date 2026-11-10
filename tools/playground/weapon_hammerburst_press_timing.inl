// Bounded timing only; never a source, deadline, retry or execution permit.
struct HammerburstPressTiming {
    inline static unsigned rows{};
    bool active{},invoked{};unsigned phase{};
    std::uint64_t frame{};std::int64_t sync{},stamp[8]{};
    HammerburstPressTiming(bool wanted,wh::Frame const* f) noexcept {
        active=wanted&&f&&log_file&&rows<12;
        if(active){++rows;frame=f->key.frame;sync=f->controllers.sync_ns;mark(0);}
    }
    void mark(unsigned value) noexcept {if(active&&value<8){phase=value;stamp[value]=weaponhand::clock_ns(nullptr);}}
    ~HammerburstPressTiming() noexcept {
        if(!active)return;
        fwprintf(log_file,L"hammerburst_press_timing frame=%llu phase=%u invoked=%u sync_ns=%lld entry_ns=%lld entry_age_ns=%lld entry_checks_ns=%lld asset_ns=%lld contract_ns=%lld button_schema_ns=%lld final_reserve_ns=%lld owned_and_dispatch_ns=%lld\n",
            frame,phase,unsigned(invoked),sync,stamp[0],stamp[0]-sync,
            stamp[2]?stamp[2]-stamp[0]:-1,stamp[3]?stamp[3]-stamp[2]:-1,
            stamp[4]?stamp[4]-stamp[3]:-1,stamp[5]?stamp[5]-stamp[4]:-1,
            stamp[6]?stamp[6]-stamp[5]:-1,stamp[7]?stamp[7]-stamp[6]:-1);
        fflush(log_file);
    }
};
