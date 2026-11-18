#pragma once

#include <cstdint>

namespace gears_boltok_close_release {

struct Witness {
    unsigned source{};
    std::uint64_t publisher_pid{}, publisher_start{};
    std::uint64_t session{}, space_epoch{}, input_epoch{}, profile_generation{};
    std::uint64_t stream{}, left_tracking{}, right_tracking{};
    std::uint64_t sequence{}, pressed{}, released{};
    std::int64_t sync_ns{};
    bool held{};
    bool focused{};
    bool valid{};
};

class Guard {
public:
    bool begin(Witness const& witness) noexcept {
        reset();
        if (!valid(witness) || !witness.held) return false;
        baseline_ = witness;
        known_ = true;
        return true;
    }

    // Negative evidence only. The caller must have authenticated a current
    // bundle independently; this witness must be an original history sample
    // from that same bundle, with all timestamps left unchanged.
    bool revoke(std::int64_t endpoint_observed_ns, std::int64_t now_ns,
        Witness const& history) const noexcept {
        if (!known_ || !valid(history)) return false;
        if (!exact_domain(baseline_, history)) return false;
        if (history.sequence <= baseline_.sequence) return false;
        if (history.sync_ns <= baseline_.sync_ns) return false;
        if (history.sync_ns > endpoint_observed_ns ||
            history.sync_ns > now_ns) return false;
        if (now_ns < history.sync_ns || now_ns-history.sync_ns>100000000) return false;
        if (now_ns < endpoint_observed_ns ||
            now_ns - endpoint_observed_ns > 100000000)
            return false;
        if (history.released <= baseline_.released) return false;
        if (history.pressed != baseline_.pressed) return false;
        if (history.held) return false;
        return true;
    }

    bool same_domain(Witness const& witness) const noexcept {
        return known_ && valid(witness) && exact_domain(baseline_,witness);
    }

    bool known() const noexcept { return known_; }

    void reset() noexcept {
        baseline_ = Witness{};
        known_ = false;
    }

private:
    static bool valid(Witness const& w) noexcept {
        return w.valid && w.focused &&
            w.publisher_pid && w.publisher_start && w.session &&
            w.space_epoch && w.input_epoch && w.profile_generation &&
            w.stream && w.left_tracking && w.right_tracking &&
            w.sequence && w.sync_ns > 0;
    }

    static bool exact_domain(Witness const& a, Witness const& b) noexcept {
        return a.source == b.source &&
            a.publisher_pid == b.publisher_pid &&
            a.publisher_start == b.publisher_start &&
            a.session == b.session &&
            a.space_epoch == b.space_epoch &&
            a.input_epoch == b.input_epoch &&
            a.profile_generation == b.profile_generation &&
            a.stream == b.stream &&
            a.left_tracking == b.left_tracking &&
            a.right_tracking == b.right_tracking;
    }

    Witness baseline_{};
    bool known_{};
};

} // namespace gears_boltok_close_release
