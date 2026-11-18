#pragma once
#include "../vr-common/weapon_call_boundary.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/weapon_shot_scope.h"
namespace gnasher_muzzle {
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
bool prepare(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept;
bool ready_for(gears_weapon_hand::Frame const&,std::int64_t) noexcept;
void clear_source() noexcept;
void begin(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
bool returned(gears_weapon_call::Invocation const&) noexcept;
bool cleanup(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
#else
inline bool prepare(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept{return false;}
inline bool ready_for(gears_weapon_hand::Frame const&,std::int64_t) noexcept{return false;}
inline void clear_source() noexcept{}
inline void begin(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept{}
inline bool returned(gears_weapon_call::Invocation const&) noexcept{return true;}
inline bool cleanup(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept{return true;}
#endif
}
