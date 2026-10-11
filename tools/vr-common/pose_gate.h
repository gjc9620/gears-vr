#pragma once
#include "pose_math.h"
#include <cstdint>
#include <optional>

namespace gears_vr {
// Producer stamps samples with the same monotonic clock used by the consumer.
// These flags mirror OpenXR's orientation/position VALID and TRACKED bits.
struct PoseSample {
    Pose head;
    std::uint64_t sequence=0;
    std::int64_t received_ns=0;
    std::uint64_t location_flags=0;
    bool session_focused=false;
};
enum class PoseStatus { unarmed, inactive, untracked, stale, replayed, invalid, recentered, applied };
struct PoseResult { Viewpoint view; PoseStatus status; };

class PoseGate {
public:
    explicit PoseGate(double units_per_metre=100, std::int64_t maximum_age_ns=100000000)
        : scale_(units_per_metre), age_(maximum_age_ns) {
        if(!std::isfinite(scale_) || scale_<=0 || age_<=0)
            throw std::invalid_argument("Invalid pose gate configuration");
    }
    // Integration must explicitly arm only after the real headset stream is verified.
    // A runtime reporting TRACKED alone cannot establish a remote headset connection.
    void arm(bool enabled) { armed_=enabled; neutral_.reset(); }
    void recenter() { neutral_.reset(); }

    PoseResult update(Viewpoint clean, PoseSample const& sample, std::int64_t now_ns,
                      bool game_focused) {
        auto reject=[&](PoseStatus status) { neutral_.reset(); return PoseResult{clean,status}; };
        if(!armed_)return reject(PoseStatus::unarmed);
        if(!game_focused || !sample.session_focused)return reject(PoseStatus::inactive);
        if((sample.location_flags & 15)!=15)return reject(PoseStatus::untracked);
        // Future dates, zero dates and subtraction overflow cannot pass freshness checks.
        if(now_ns<=0 || sample.received_ns<=0 || sample.received_ns>now_ns ||
           now_ns-sample.received_ns>age_)return reject(PoseStatus::stale);
        // Reuse of the newest sample between headset updates is allowed; backwards
        // sequences and changed payload under the same sequence are not.
        if(!sample.sequence || sample.sequence<last_sequence_)return reject(PoseStatus::replayed);
        if(sample.sequence==last_sequence_ && last_sample_ && !same(sample,*last_sample_))
            return reject(PoseStatus::replayed);
        try {
            normalized(sample.head.orientation);
            if(!finite(sample.head.position))throw std::invalid_argument("Invalid head position");
            auto neutral=neutral_.value_or(sample.head);
            auto result=relative_view(clean,neutral,sample.head,scale_);
            bool establishing=!neutral_.has_value();
            neutral_=neutral; last_sequence_=sample.sequence; last_sample_=sample;
            return {result,establishing?PoseStatus::recentered:PoseStatus::applied};
        } catch(std::invalid_argument const&) { return reject(PoseStatus::invalid); }
    }
private:
    static bool same(PoseSample const& a,PoseSample const& b) {
        return a.received_ns==b.received_ns && a.location_flags==b.location_flags &&
            a.session_focused==b.session_focused && a.head.position.x==b.head.position.x &&
            a.head.position.y==b.head.position.y && a.head.position.z==b.head.position.z &&
            a.head.orientation.x==b.head.orientation.x && a.head.orientation.y==b.head.orientation.y &&
            a.head.orientation.z==b.head.orientation.z && a.head.orientation.w==b.head.orientation.w;
    }
    bool armed_=false;
    double scale_;
    std::int64_t age_;
    std::uint64_t last_sequence_=0;
    std::optional<Pose> neutral_;
    std::optional<PoseSample> last_sample_;
};
} // namespace gears_vr
