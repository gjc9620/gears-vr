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
// Diagnostic copy only, consumed on normal GT tick, never an action ticket.
struct AppliedEvidence {
    std::uint64_t writes[4]{},shot{},epoch{};
    std::uint64_t input_sequence{};
    std::int64_t input_sync_ns{},sampled_ns{};
    std::int32_t aim[3]{},original_aim[3]{};
    float origin[3]{};
    double right_grip[3]{};
};
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE) && GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE
bool applied_evidence(AppliedEvidence&) noexcept;
bool set_running(bool value) noexcept;
bool prepare_current(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept;
bool ready_for(gears_weapon_hand::Frame const&) noexcept;
void clear_source() noexcept;
void begin(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
bool normal_return(gears_weapon_call::Invocation const&) noexcept;
bool cleanup(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
#else
inline bool applied_evidence(AppliedEvidence&) noexcept {return false;}
inline bool set_running(bool value) noexcept {return !value;}
inline bool prepare_current(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept {return false;}
inline bool ready_for(gears_weapon_hand::Frame const&) noexcept {return false;}
inline void clear_source() noexcept {}
#endif
}
