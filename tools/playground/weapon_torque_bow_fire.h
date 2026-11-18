#pragma once
#include "../vr-common/weapon_call_boundary.h"
#include "../vr-common/weapon_shot_scope.h"
namespace torquebow_input_owner {
// GT-local journal identity only; no engine call, Native callback or new lease.
std::uint64_t projectile_owner(gears_weapon_hand::Frame const&) noexcept;
}
namespace torquebow_fire {
struct SpawnReceipt {
    gears_weapon_hand::Frame source{};
    gears_weapon_hand::ObjectIdentity projectile{},projectile_class{};
    std::uint64_t call{},shot{},input_serial{};std::int64_t observed_ns{};
};
bool prepare(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept;
bool ready_for(gears_weapon_hand::Frame const&,std::int64_t) noexcept;
void clear_source() noexcept;
void begin(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
bool returned(gears_weapon_call::Invocation const&) noexcept;
bool cleanup(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
bool peek_spawn(SpawnReceipt&) noexcept;
bool consume_spawn(std::uint64_t) noexcept;
}
