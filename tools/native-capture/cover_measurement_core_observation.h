#pragma once
#include "body_tick_source.h"

// Immutable copies only. No clock, engine read, publication, or permission.
// The original measurement remains the sole accept/reject decision.
namespace gears_cover_measurement_core {
namespace core=gears_native_rig_tracking;
struct Witness {
    unsigned mode{},valid{},non_age_valid{},expired{},controllers_match{},verdict{~0u},hard_detail{};
    std::int64_t now{},stereo_received{},controller_received{},controller_sync{};
    std::int64_t metadata_begin{},metadata_end{},tick_begin{},tick_end{};
    std::uint64_t limit{},published{},ticket{},reference_revision{};
    unsigned size{},abi{},flags{},status{};
};
inline Witness observe(core::CoreSnapshot const& source,std::int64_t now,std::uint64_t limit,bool allow_sim) noexcept {
    Witness w{};w.mode=1;w.now=now;w.limit=limit;w.published=source.published_ns;
    w.stereo_received=source.stereo.received_ns;w.controller_received=source.controllers.received_ns;
    w.controller_sync=source.controllers.sync_ns;w.ticket=source.source_ticket;
    w.reference_revision=source.native_reference_revision;w.size=source.size;w.abi=source.abi_version;
    w.flags=source.flags;w.status=unsigned(source.status);
    w.valid=core::core_snapshot_valid(source,now,limit,allow_sim);
    w.non_age_valid=core::core_snapshot_non_age_valid(source,now,limit,allow_sim);
    // Only these two exact ceilings use the measurement's supplied limit.
    // Future/invalid clocks stay hard failures; no unsigned age underflow.
    if(now>0&&source.published_ns&&source.published_ns<=std::uint64_t(now)&&
       std::uint64_t(now)-source.published_ns>limit)w.expired|=1;
    if(source.stereo.received_ns>0&&now>=source.stereo.received_ns&&
       std::uint64_t(now-source.stereo.received_ns)>limit)w.expired|=2;
    if(source.flags&core::rig::controllers_matched)
        w.controllers_match=gears_vr::controller_frame_matches(source.controllers,source.stereo,now,allow_sim)?1:0;
    else w.controllers_match=2; // Not required by this source; not a valid cohort claim.
    return w;
}
inline Witness observe(gears_body_tick_source::Input const& source,std::int64_t now,std::uint64_t limit,bool allow_sim) noexcept {
    auto w=observe(source.observed.parent_scene,now,limit,allow_sim);w.mode=2;
    w.metadata_begin=source.metadata_begin_ns;w.metadata_end=source.metadata_end_ns;
    w.tick_begin=source.observed.begin_ns;w.tick_end=source.observed.end_ns;
    w.verdict=unsigned(gears_body_tick_source::classify(source,source.observed.parent_scene,now,allow_sim));
    w.hard_detail=gears_body_tick_source::rejection_detail(source,source.observed.parent_scene,now,allow_sim);
    return w;
}
} // namespace gears_cover_measurement_core
