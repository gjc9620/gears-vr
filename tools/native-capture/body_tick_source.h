#pragma once
#include "body_tick_input.h"
// Distinct consumer data contract. cache revision is an immutable association,
// never proof of a live owner or eligibility for old TickInput/Core consumers.
namespace gears_body_tick_source {
namespace tick=gears_body_tick_input;
using Verdict=tick::Verdict;
struct Input {tick::Input observed{};std::uint64_t scene_cache_revision{};};
inline Verdict classify(Input const& input,gears_native_rig_tracking::CoreSnapshot const& original,
    std::int64_t now,bool allow_sim) noexcept {
    std::int64_t scene_ns{};
    if(!input.scene_cache_revision||!tick::hard_shape(input.observed,original,now,allow_sim,scene_ns))return Verdict::hard;
    const auto& t=input.observed;
    const auto old=[&](std::int64_t stamp,std::uint64_t age){return std::uint64_t(now-stamp)>age;};
    if(old(t.begin_ns,tick::tracking_age_ns)||old(t.end_ns,tick::tracking_age_ns)||
       old(t.stereo.received_ns,tick::tracking_age_ns)||old(t.controllers.received_ns,tick::tracking_age_ns)||
       old(t.controllers.sync_ns,tick::tracking_age_ns)||std::uint64_t(now)-original.published_ns>tick::parent_age_ns||
       old(scene_ns,tick::parent_age_ns))return Verdict::source_age_only;
    return Verdict::okay;
}
static_assert(std::is_standard_layout_v<Input>&&std::is_trivially_copyable_v<Input>);
} // namespace gears_body_tick_source
