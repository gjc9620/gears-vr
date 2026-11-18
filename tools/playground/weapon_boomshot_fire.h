#pragma once
#include "../vr-common/weapon_call_boundary.h"
#include "../vr-common/weapon_shot_scope.h"
namespace boomshot_runtime {bool blocks_fire(gears_weapon_hand::Frame const&) noexcept;}
namespace boomshot_fire {
bool prepare(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept;
bool ready_for(gears_weapon_hand::Frame const&,std::int64_t) noexcept;
void clear_source() noexcept;
void begin(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
bool returned(gears_weapon_call::Invocation const&) noexcept;
bool cleanup(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
}
