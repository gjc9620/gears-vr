#pragma once
#include "anchor_observer.h"
#include <atomic>
#include <cstring>
#include <limits>
// Native-owned immutable-view storage. Callers serialize publication at the
// existing bootstrap/world-close sites. This is not a Game-memory certificate.
// Native-owned invariant: backing storage/revision memory is not modified by
// const_cast, raw pointer writes, external patching or RAM corruption. Those
// are outside this publisher contract; revision is not a memory-integrity hash.
namespace gears_anchor_native {
template<std::uint64_t Limit=(std::numeric_limits<std::uint64_t>::max)()> class ConfigStore {
    gears_anchor::Config value_{};
    std::atomic<std::uint64_t> revision_{};
    std::uint64_t last_{};bool failed_{};
public:
    gears_anchor::Config const& view() const noexcept{return value_;}
    std::uint64_t revision() const noexcept{return revision_.load(std::memory_order_acquire);}
    bool preflight(gears_anchor::Config const& next) noexcept {
        if(failed_)return false;
        if(last_==Limit&&(!last_||std::memcmp(&value_,&next,sizeof value_))){
            // Reject new publication without withdrawing the last stable
            // metadata needed by retained cleanup/rollback.
            failed_=true;return false;
        }
        return true;
    }
    bool publish(gears_anchor::Config const& next) noexcept {
        if(!preflight(next))return false;
        if(last_&&!std::memcmp(&value_,&next,sizeof value_))return true;
        // Readers of Config itself still require the existing world admission.
        // Zero closes metadata-current checks while the rare publication runs.
        revision_.store(0,std::memory_order_release);
        std::memcpy(&value_,&next,sizeof value_);
        revision_.store(++last_,std::memory_order_release);return true;
    }
};
}
