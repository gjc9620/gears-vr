#pragma once
#include <cstdint>

// Selection within the existing128-record exporter, never another recorder,
// input permit or gameplay state. Default output remains first128 records.
namespace gears_lancer_trajectory_log {
constexpr unsigned record_limit=128,head_shots=16,tail_shots=64;
constexpr bool valid_total(std::uint64_t total) noexcept {return total>=tail_shots&&total<=1000000;}
constexpr bool selected(std::uint64_t shot,std::uint64_t finite_total) noexcept {
    return !valid_total(finite_total)||shot<=head_shots||shot>finite_total-tail_shots;
}
}
