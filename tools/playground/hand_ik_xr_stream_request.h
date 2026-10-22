#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace gears_hand_xr_trial {
// Private trial ABI only. The runner brackets full UObject identities with
// independent, fresh Native status from this new session, with no handoff.
// These fields are a frozen external baseline, not a synchronous Native getter.
struct ObjectKey {
    std::uint64_t address{},object_class{},outer{};
    std::uint32_t internal_index{},name_index{},name_number{},reserved{};
};
enum Object : unsigned {world_info,level,controller,pawn,mesh,weapon,object_count};
struct Start {
    std::uint32_t schema{},size{},game_pid{},reserved{};
    std::uint64_t game_creation_time{},expected_native_world_generation{},
        native_started_qpc{},baseline_status_qpc{},arm_deadline_ms{};
    std::array<ObjectKey,object_count> objects{};
    float baseline_world_time{};
    std::uint32_t reserved2{};
};
static_assert(std::is_standard_layout_v<Start>&&std::is_trivially_copyable_v<Start>);
static_assert(sizeof(ObjectKey)==40&&sizeof(Start)==304);
inline bool shape(Start const& value,std::uint32_t pid,std::uint64_t creation,
                  std::uint64_t now_ms,std::uint64_t now_qpc,std::uint64_t frequency) noexcept {
    if(value.schema!=1||value.size!=sizeof(Start)||value.game_pid!=pid||!pid||
       value.game_creation_time!=creation||!creation||value.reserved||value.reserved2||
       // Both Native epochs initialize at 1. This fixed-arena experiment
       // forbids every prepare/handoff; it never silently substitutes 1.
       value.expected_native_world_generation!=1||!value.native_started_qpc||
       value.native_started_qpc>value.baseline_status_qpc||
       value.baseline_status_qpc>now_qpc||!frequency||
       now_qpc-value.baseline_status_qpc>frequency||
       value.arm_deadline_ms<now_ms||value.arm_deadline_ms-now_ms>1000||
       !std::isfinite(value.baseline_world_time)||value.baseline_world_time<0)return false;
    for(auto const& key:value.objects)
        if(!key.address||!key.object_class||!key.outer||key.reserved)return false;
    return true;
}
} // namespace gears_hand_xr_trial
