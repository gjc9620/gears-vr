// weapon_boltok_grip_close.h
#pragma once

#include <cmath>
#include <cstdint>

#include "weapon_boltok_reload.h"

namespace gears_boltok_grip_close {
using gears_weapon::BoltokReloadStamp;
using gears_weapon::BoltokReloadIdentity;

class Closure {
public:
    bool begin(BoltokReloadStamp const& stamp, std::uint64_t token,
        std::int64_t now) noexcept {
        reset();
        if (!stamp.valid(now) || !stamp.left_tracked ||
            !stamp.right_tracked || !stamp.weapon_owned || !token)
            return false;
        identity_ = stamp.identity;
        token_ = token;
        start_sequence_ = stamp.sequence;
        start_sync_ns_ = stamp.observed_ns;
        active_ = true;
        return true;
    }

    bool current(BoltokReloadStamp const& stamp, std::uint64_t token,
        std::int64_t now) const noexcept {
        if (!active_ || !token_ || token != token_) return false;
        if (!stamp.left_tracked || !stamp.right_tracked ||
            !stamp.weapon_owned || !stamp.valid(now)) return false;
        if (!(stamp.identity == identity_)) return false;
        return stamp.sequence >= start_sequence_ &&
            stamp.observed_ns >= start_sync_ns_;
    }

    bool dispatch(BoltokReloadStamp const& stamp, std::uint64_t token,
        std::int64_t now) noexcept {
        if (!current(stamp, token, now) || dispatched_) return false;
        dispatch_sequence_ = stamp.sequence;
        dispatch_sync_ns_ = stamp.observed_ns;
        dispatch_ns_ = now;
        dispatched_ = true;
        return true;
    }

    bool acknowledge(BoltokReloadStamp const& stamp, std::uint64_t token,
        std::int64_t now, std::int64_t actual_observed_ns,
        double actual_angle, bool native_ready) noexcept {
        if (!current(stamp, token, now) || !dispatched_ || completed_)
            return false;
        if (stamp.sequence <= dispatch_sequence_ ||
            stamp.observed_ns < dispatch_sync_ns_)
            return false;
        if (actual_observed_ns < dispatch_ns_ ||
            actual_observed_ns > now ||
            now - actual_observed_ns > 100000000)
            return false;
        if (!native_ready || !std::isfinite(actual_angle) ||
            std::abs(actual_angle) > .02)
            return false;
        completed_ = true;
        active_ = false;
        return true;
    }

    bool active() const noexcept { return active_; }
    bool dispatched() const noexcept { return dispatched_; }
    std::int64_t dispatch_ns() const noexcept { return dispatch_ns_; }
    bool completed() const noexcept { return completed_; }

    void reset() noexcept {
        identity_ = {};
        token_ = 0;
        start_sequence_ = 0;
        start_sync_ns_ = 0;
        dispatch_sequence_ = 0;
        dispatch_sync_ns_ = 0;
        dispatch_ns_ = 0;
        active_ = false;
        dispatched_ = false;
        completed_ = false;
    }

private:
    BoltokReloadIdentity identity_{};
    std::uint64_t token_{};
    std::uint64_t start_sequence_{};std::int64_t start_sync_ns_{};
    std::uint64_t dispatch_sequence_{};std::int64_t dispatch_sync_ns_{};
    std::int64_t dispatch_ns_{};
    bool active_{};
    bool dispatched_{};
    bool completed_{};
};

} // namespace gears_boltok_grip_close
