#pragma once
#include "../vr-common/weapon_support_grip.h"

// Explicit build profile for the owner-started unified candidate. This is not
// an input source, engine-call permit or replacement for Native identity and
// lifecycle admission. Historical experiment builds retain their test guards.
namespace unified_weapon_fire {
inline constexpr bool manual_profile() noexcept {
#if defined(GEARS_PLAYGROUND_UNIFIED_MANUAL_PROFILE) && GEARS_PLAYGROUND_UNIFIED_MANUAL_PROFILE
    return true;
#else
    return false;
#endif
}
inline constexpr bool consumer_compiled() noexcept {
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE) && GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE
    return true;
#else
    return false;
#endif
}
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE) && GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE
bool set_running(bool value) noexcept;
bool prepare_current(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept;
bool ready_for(gears_weapon_hand::Frame const&) noexcept;
void clear_source() noexcept;
void begin(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
bool normal_return(gears_weapon_call::Invocation const&) noexcept;
bool cleanup(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
#else
inline bool set_running(bool value) noexcept {return !value;}
inline bool prepare_current(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept {return false;}
inline bool ready_for(gears_weapon_hand::Frame const&) noexcept {return false;}
inline void clear_source() noexcept {}
#endif
}
