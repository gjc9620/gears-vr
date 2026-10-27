#pragma once

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
}
