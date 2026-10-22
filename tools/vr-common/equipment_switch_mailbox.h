#pragma once
#include <atomic>

namespace gears_weapon {
// One outstanding request, including the entire consumer operation. This
// ledger owns no game state; the take()/claim_idle() winner must complete only after
// the original call, readback, model update and completion log have finished.
class EquipmentSwitchMailbox {
    enum class Phase : unsigned { idle,reserved,queued,processing };
    std::atomic<Phase> phase_{Phase::idle};
    std::atomic<unsigned> observed_{};
    unsigned slot_{4}; // Published by queued release; immutable while processing.
public:
    static constexpr unsigned no_slot=4;

    bool enqueue(unsigned slot) noexcept {
        if(slot>=no_slot)return false;
        auto expected=Phase::idle;
        if(!phase_.compare_exchange_strong(expected,Phase::reserved,
                std::memory_order_acq_rel,std::memory_order_acquire))return false;
        // Hide the old result before publishing any part of the new request.
        observed_.store(0,std::memory_order_relaxed);
        slot_=slot;
        phase_.store(Phase::queued,std::memory_order_release);
        return true;
    }

    unsigned take() noexcept {
        auto expected=Phase::queued;
        if(!phase_.compare_exchange_strong(expected,Phase::processing,
                std::memory_order_acq_rel,std::memory_order_acquire))return no_slot;
        return slot_;
    }

    // Claim an intent/deferred operation without an external slot. This uses
    // the same exclusion as enqueue/take and preserves the observed result.
    // The winner owns completion, including any explicit deferred transfer.
    bool claim_idle() noexcept {
        auto expected=Phase::idle;
        return phase_.compare_exchange_strong(expected,Phase::processing,
            std::memory_order_acq_rel,std::memory_order_acquire);
    }

    // Observations may update an idle result too, for later original-game
    // completion callbacks. They never release the request's busy state.
    void observe(unsigned status) noexcept {
        observed_.store(status,std::memory_order_release);
    }

    bool complete() noexcept {
        auto expected=Phase::processing;
        return phase_.compare_exchange_strong(expected,Phase::idle,
            std::memory_order_acq_rel,std::memory_order_acquire);
    }

    bool pending() const noexcept {
        return phase_.load(std::memory_order_acquire)!=Phase::idle;
    }

    unsigned status() const noexcept {
        if(pending())return 0;
        const auto result=observed_.load(std::memory_order_acquire);
        // enqueue may have reserved the mailbox while we read the result.
        return pending()?0:result;
    }

    // Caller must first drain all producers, consumers and observers. This is
    // lifecycle initialization/teardown, not cancellation of an active callback.
    void reset() noexcept {
        slot_=no_slot;
        observed_.store(0,std::memory_order_relaxed);
        phase_.store(Phase::idle,std::memory_order_release);
    }
};
}
