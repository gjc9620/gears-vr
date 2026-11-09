#pragma once
#include <cstdint>
#include "../vr-common/weapon_hand_participant.h"
// Exact-HB optional interface to the existing input/event/source owners.
// No tracking wire, shared arbiter, callback owner or Native ABI is added.
namespace hammerburst_burst_event {
bool targets(std::uint64_t weapon) noexcept;
bool requires_release() noexcept;
bool cleanup_pending() noexcept;
bool prepare_press(gears_weapon_hand::Frame const&) noexcept;
bool owned_press(gears_weapon_hand::Frame const&) noexcept;
void press_failed() noexcept;
bool need_source(gears_weapon_hand::Frame const&) noexcept;
bool source_allowed(gears_weapon_hand::Frame const&,std::int64_t render,
                    std::int64_t sampled,std::int64_t observed) noexcept;
void trace(gears_weapon_hand::Frame const&,std::uint64_t shot,std::int64_t render,
           std::int64_t sampled,std::int64_t observed,bool consistent) noexcept;
bool capture_initial(std::uint64_t object,std::uint64_t function,void const* parameters) noexcept;
bool defer(std::uint64_t object,std::uint64_t function) noexcept;
bool service(std::uint64_t controller) noexcept;
void deliver(gears_weapon_hand::Frame const&) noexcept;
bool release_completed(gears_weapon_hand::Frame const* normal_source) noexcept;
void after_tick(gears_weapon_hand::Frame const* current_source) noexcept;
}
