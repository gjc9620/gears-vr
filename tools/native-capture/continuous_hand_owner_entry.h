#pragma once
#include "body_c0_budget.h"
#include "body_tick_tracking_probe.h"
#include "body_tick_input_live.h"
#include "body_tick_source_live.h"
#include "body_tick_commit.h"
#include <cstdint>
#include <type_traits>
namespace gears_continuous_hand_entry {
// Stack-owned POD only. Original inputs survive RawInvoke's result packet.
struct Entry {
    std::uintptr_t rcx{},rdx{},r8{},r9{},ra{},rsp{};
    std::uint64_t invocation{};
    unsigned kind{},thread{},depth{};
    bool tick_depth{},compose_depth{},control_depth{},selected{};
};
static_assert(std::is_trivially_copyable_v<Entry> && std::is_trivially_destructible_v<Entry>);
}
