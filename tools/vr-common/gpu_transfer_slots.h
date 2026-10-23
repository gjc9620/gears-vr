#pragma once
#include <array>
#include <cstdint>
#include <limits>

// Fixed-size ownership decisions, shared by the real transport and its tests.
// A CPU acknowledgement is insufficient: each slot has separate GPU fences.
namespace gears_gpu_slots {
inline constexpr unsigned count = 2;
inline constexpr unsigned none = count;
inline constexpr std::uint64_t removed = std::numeric_limits<std::uint64_t>::max();
struct Lease {
    std::uint64_t generation{}, sequence{};
    unsigned slot{none};
};
inline bool valid(Lease const& lease) {
    return lease.generation && lease.sequence && lease.sequence != removed && lease.slot < count;
}
inline bool same(Lease const& a, Lease const& b) {
    return valid(a) && valid(b) && a.generation == b.generation && a.sequence == b.sequence && a.slot == b.slot;
}
struct FenceState {
    std::uint64_t sequence{}, ready{}, done{};
    bool in_flight{}, quarantined{};
};
inline bool producer_complete(FenceState const& s) {
    return !s.quarantined && s.ready != removed && s.ready >= s.sequence;
}
inline bool reusable(FenceState const& s) {
    return producer_complete(s) && !s.in_flight && s.done != removed && s.done >= s.sequence;
}
inline unsigned choose(std::array<FenceState, count> const& slots, unsigned start) {
    for (unsigned offset = 0; offset < count; ++offset) {
        const unsigned index = (start + offset) % count;
        if (reusable(slots[index])) return index;
    }
    return none;
}
inline unsigned next(std::array<std::uint64_t, count> const& published, std::uint64_t last) {
    if (last >= removed - 1) return none;
    // The two loads do not form an atomic snapshot. Never skip a missing
    // sequence: a publication may have appeared in the first slot after its
    // load but before a newer publication was observed in the second slot.
    for (unsigned i = 0; i < count; ++i) if (published[i] == last + 1) return i;
    return none;
}
enum class ConsumerReady {proceed,defer,device_removed};
inline ConsumerReady consumer_ready(std::uint64_t serial,std::uint64_t ready,std::uint64_t done,bool ready_only){
    if(ready==removed||done==removed)return ConsumerReady::device_removed;
    return ready_only&&ready<serial?ConsumerReady::defer:ConsumerReady::proceed;
}
inline bool drained(std::array<FenceState, count> const& slots) {
    for (auto const& slot : slots) if (!reusable(slot)) return false;
    return true;
}
}
