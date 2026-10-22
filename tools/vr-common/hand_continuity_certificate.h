#pragma once
#include <cstdint>
#include <type_traits>

namespace gears_hand_continuity {
// Evidence frozen with one Native world-space pose, not a live Native-reference
// getter. Epochs belong to the stated publisher/incarnation and never wrap.
struct Certificate {
    std::uint64_t native_started_qpc{},xr_publisher_pid{},xr_publisher_start{};
    std::uint64_t stream_epoch{},reference_epoch{},reference_generation{},tracking_epoch[2]{};
    std::uint64_t profile_generation{};
    std::int64_t stereo_received_ns{},input_received_ns{};
    std::uint32_t source{}; // StereoSource value; simulated provenance stays explicit.
};
static_assert(std::is_standard_layout_v<Certificate> && std::is_trivially_copyable_v<Certificate>);
inline bool same_certificate(Certificate const& a,Certificate const& b) noexcept {
    return a.native_started_qpc==b.native_started_qpc&&a.xr_publisher_pid==b.xr_publisher_pid&&
        a.xr_publisher_start==b.xr_publisher_start&&a.stream_epoch==b.stream_epoch&&
        a.reference_epoch==b.reference_epoch&&a.reference_generation==b.reference_generation&&
        a.tracking_epoch[0]==b.tracking_epoch[0]&&a.tracking_epoch[1]==b.tracking_epoch[1]&&
        a.profile_generation==b.profile_generation&&a.stereo_received_ns==b.stereo_received_ns&&
        a.input_received_ns==b.input_received_ns&&a.source==b.source;
}
inline bool certificate_shape_valid(Certificate const& c) noexcept {
    return c.native_started_qpc&&c.xr_publisher_pid&&c.xr_publisher_pid<=0xffffffffULL&&
        c.xr_publisher_start&&c.stream_epoch&&c.reference_epoch&&c.reference_generation&&
        c.tracking_epoch[0]&&c.tracking_epoch[1]&&c.profile_generation&&c.source<=1&&
        c.stereo_received_ns>0&&c.input_received_ns>0;
}
inline bool certificate_recent(Certificate const& c,std::int64_t now_ns) noexcept {
    // Test both original source times; publication cannot make old input fresh.
    return now_ns>0&&c.stereo_received_ns>0&&c.input_received_ns>0&&
        c.stereo_received_ns<=now_ns&&c.input_received_ns<=now_ns&&
        now_ns-c.stereo_received_ns<=100000000&&now_ns-c.input_received_ns<=100000000;
}
} // namespace gears_hand_continuity
