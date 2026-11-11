#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <cwchar>
// Sole existing GT writes AND dumps this passive ring. Worker cleanup must
// never read it. No locks, engine calls, source observations or action permits.
namespace hb_tick_cost_tail {
struct Row {
    std::int64_t begin{},original_begin{},original_end{},equipment_begin{},equipment_end{},end{};
    std::int64_t last_input_sync{},last_input_observed{},captured{};
    std::uint64_t event{};
    unsigned first_debt{},cancel{},count{},owned{},armed{},fire_early{},reload_early{},pre_reload{};
};
inline std::array<Row,128> rows{};
inline std::uint64_t total{};
inline std::array<Row,64> first_rows{};
inline std::array<std::uint64_t,64> first_ordinals{};
inline std::uint64_t first_seen{};
inline unsigned first_kept{};
inline void record(Row const& row) noexcept {
    if(total==UINT64_MAX)return;
    rows[total%rows.size()]=row;++total;
    if(row.first_debt&&row.event&&row.captured>0){
        if(first_seen!=UINT64_MAX)++first_seen;
        if(first_kept<first_rows.size()){first_rows[first_kept]=row;first_ordinals[first_kept]=total;++first_kept;}
    }
}
inline void dump(FILE* file) noexcept {
    if(!file)return;
    const auto count=(total<rows.size()?total:std::uint64_t(rows.size()));
    std::fwprintf(file,L"hammerburst_tick_cost_summary total=%llu retained=%llu writer=existing_gt action=0\n",total,count);
    for(unsigned series=0;series<2;++series){
      const auto begin=series?std::uint64_t(0):total-count;const auto end=series?std::uint64_t(first_kept):total;
      for(auto n=begin;n<end;++n){const auto& r=series?first_rows[n]:rows[n%rows.size()];
        const auto ordinal=series?first_ordinals[n]:n+1;
        std::fwprintf(file,L"%ls ordinal=%llu begin_ns=%lld original_begin_ns=%lld original_end_ns=%lld equipment_begin_ns=%lld equipment_end_ns=%lld end_ns=%lld last_input_sync_ns=%lld last_input_observed_ns=%lld event=%llu captured_ns=%lld first_debt=%u cancel=%u count=%u owned=%u armed=%u fire_early=%u reload_early=%u pre_reload=%u pre_original_ns=%lld original_ns=%lld service_ns=%lld equipment_ns=%lld follow_ns=%lld total_ns=%lld action=0\n",
            series?L"hammerburst_first_tick_cost":L"hammerburst_tick_cost",ordinal,r.begin,r.original_begin,r.original_end,r.equipment_begin,r.equipment_end,r.end,
            r.last_input_sync,r.last_input_observed,r.event,r.captured,r.first_debt,r.cancel,r.count,r.owned,r.armed,
            r.fire_early,r.reload_early,r.pre_reload,r.original_begin-r.begin,r.original_end-r.original_begin,
            r.equipment_begin-r.original_end,r.equipment_end-r.equipment_begin,r.end-r.equipment_end,r.end-r.begin);
    }
    }
    std::fwprintf(file,L"hammerburst_first_tick_summary seen=%llu retained=%u capacity=64 truncated=%u action=0\n",first_seen,first_kept,unsigned(first_seen>first_kept));
    std::fflush(file);
}
}
