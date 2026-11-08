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
    std::uint64_t frame{};
    std::int64_t input_sync_ns{},sampled_ns{};
    std::int32_t aim[3]{},original_aim[3]{};
    float origin[3]{};
    float original_socket_origin[3]{};
    std::int32_t socket_rotation[3]{};
    double right_grip[3]{};
    double world_grip[3]{},target_mesh[3]{};
    bool follow_target_known{};
};
struct ReadinessEvidence {
    bool valid{},same_frame{},frame_current{},reserved_frame_current{},held{},registration{},live{},reserved_live{},epoch{};
    std::int64_t sync_age{-1},stereo_age{-1},frame_age{-1},sample_age{-1},render_age{-1};
};
// Read-only values copied at the actual CalcWeaponFire callsite. End is the
// requested ray endpoint, not a collision result or a rendered reticle hit.
struct FireTraceEvidence {
    std::uint64_t count{},shot{},epoch{},frame{},input_sequence{};
    std::int64_t input_sync_ns{},observed_ns{};
    float start[3]{},end[3]{},pushed[3]{},muzzle_origin[3]{};
    std::int32_t muzzle_aim[3]{};
    // Same actual shot's source, not a separately sampled planning target.
    double sight_rear[3]{},sight_front[3]{},sight_zero[3]{};
    double sight_units{},sight_model_scale{};
    bool sight_zero_known{};
};
// One normal-tick export per changed shot. Counts here belong to this shot,
// unlike AppliedEvidence::writes, which are cumulative since the control epoch.
struct ShotTrajectoryEvidence {
    std::uint64_t shot{},epoch{},revision{},writes[4]{};
    std::uint64_t writes_at_trace[4]{};
    std::uint64_t write_frame{},write_input_sequence{};
    std::int64_t write_input_sync_ns{};
    FireTraceEvidence trace{};
    bool trace_valid{},write_source_consistent{true},trace_source_consistent{true};
    bool impact_valid{};
    unsigned impact_stage{};
    float impact_location[3]{};
};
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE) && GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE
bool applied_evidence(AppliedEvidence&) noexcept;
bool fire_trace_evidence(FireTraceEvidence&) noexcept;
bool next_shot_trajectory(ShotTrajectoryEvidence&) noexcept;
bool admission_evidence(std::array<std::uint64_t,11>&) noexcept;
bool rejected_source_evidence(std::array<std::int64_t,10>&) noexcept;
bool hud_admission_evidence(std::array<std::uint64_t,8>&) noexcept;
struct HudArgumentEvidence {std::array<unsigned char,32> bytes{};std::uint64_t offset{},start_property{},end_property{};std::uint32_t callee_name{};bool valid{};};
bool hud_argument_evidence(HudArgumentEvidence&) noexcept;
std::uint64_t shot_trajectory_dropped_updates() noexcept;
bool set_running(bool value) noexcept;
bool prepare_current(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept;
bool ready_for(gears_weapon_hand::Frame const&,std::int64_t dispatch_reserve_ns=0) noexcept;
bool readiness_evidence(gears_weapon_hand::Frame const&,std::int64_t,ReadinessEvidence&) noexcept;
void clear_source() noexcept;
void begin(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
bool hud_begin(gears_weapon_call::Invocation const&) noexcept;
void configure_hud_receiver(void* image) noexcept;
bool normal_return(gears_weapon_call::Invocation const&) noexcept;
bool cleanup(gears_weapon_call::Invocation const&,gears_weapon_shot::Token const&) noexcept;
#else
inline bool applied_evidence(AppliedEvidence&) noexcept {return false;}
inline bool fire_trace_evidence(FireTraceEvidence&) noexcept {return false;}
inline bool next_shot_trajectory(ShotTrajectoryEvidence&) noexcept {return false;}
inline bool admission_evidence(std::array<std::uint64_t,11>&) noexcept {return false;}
inline bool rejected_source_evidence(std::array<std::int64_t,10>&) noexcept {return false;}
inline bool hud_admission_evidence(std::array<std::uint64_t,8>&) noexcept {return false;}
inline std::uint64_t shot_trajectory_dropped_updates() noexcept {return 0;}
inline bool set_running(bool value) noexcept {return !value;}
inline bool prepare_current(gears_weapon_hand::Frame const&,gears_weapon_hand::GripGameState const&) noexcept {return false;}
inline bool ready_for(gears_weapon_hand::Frame const&,std::int64_t=0) noexcept {return false;}
inline bool readiness_evidence(gears_weapon_hand::Frame const&,std::int64_t,ReadinessEvidence&) noexcept {return false;}
inline void clear_source() noexcept {}
#endif
}
