#pragma once
#include <cstdint>
#include <limits>

namespace gears_vr {
struct ReferenceContinuity {
    std::uint64_t epoch{};
    bool stable{},pending{},exhausted{};
};

// This counter belongs to one live Native gate instance. A reference reset
// must never reset it: the next pose must expose even an otherwise hidden
// reset/recenter/loss to a consumer which only sees the newest pose.
class ReferenceContinuityCounter {
    std::uint64_t epoch_{1};
    bool broken_{true},exhausted_{};
public:
    explicit ReferenceContinuityCounter(std::uint64_t first=1) noexcept
        :epoch_(first),exhausted_(!first) {}
    void boundary() noexcept {
        broken_=true;
        if(exhausted_)return;
        if(epoch_==(std::numeric_limits<std::uint64_t>::max)()){
            exhausted_=true;return;
        }
        ++epoch_;
    }
    void break_observed() noexcept {
        // Repeated invalid samples are one interruption, not a new epoch on
        // every poll. Explicit boundaries still advance independently.
        if(!broken_)boundary();
    }
    void accepted() noexcept {if(!exhausted_)broken_=false;}
    ReferenceContinuity snapshot(bool have_reference,bool pending) const noexcept {
        return {epoch_,have_reference&&!pending&&!broken_&&!exhausted_,pending,exhausted_};
    }
};
} // namespace gears_vr
