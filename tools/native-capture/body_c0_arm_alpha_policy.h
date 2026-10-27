#pragma once
#include "cover_pose_phase.h"

// Private synchronous evidence only, never a permission or reusable receipt.
// Native owner independently authenticates its current joint/consumer scope.
namespace gears_body_c0_arm_alpha {
namespace pose=gears_cover_pose_phase;
enum class Stage:std::uint32_t {compute=1,entry=2,token=3,sample=4,owner=5,post_owner=6,applied=7};
enum class RetainedControl:std::uint32_t {not_applicable,denied,verified};
// Diagnostic only. Flags: 1 adjacent same-node last compute, 2 valid live
// token, 4 live_checked, 8 returned, 16 owner called, 32 owner granted.
// kind UINT32_MAX / end 0 denotes an unconfirmed role. Zero scale values are
// observations only when their corresponding read_mask bits were recorded.
struct Diagnostic {
    std::uint64_t compose{},outer_tick{},compute_ordinal{},weight_ordinal{},token{};
    std::uintptr_t node_argument{},scope_address{};std::uint64_t raw_rdx{};
    std::int64_t qpc{};
    std::uint32_t thread{},kind{},end{},stage{},reason{},flags{},original_alpha_bits{},final_alpha_bits{},read_mask{},read_mask_after{},cached_scale_word{},cached_scale_word_after{};
    std::int32_t scale_count{},scale_count_after{};
    std::uint32_t compute_failure{},weight_failure{},owner_current_before{},owner_current_after{};
};
static_assert(std::is_trivially_copyable_v<Diagnostic>&&std::is_standard_layout_v<Diagnostic>);

// Diagnostic copies after existing arbitration, never a proof or alpha input.
struct WeightReturn {
    std::uint64_t compose{},outer_tick{},compute_ordinal{},weight_ordinal{},node_argument{},mesh{},
        scope_address{},entry_rsp{},caller{},thread{},kind{UINT64_MAX},end{UINT64_MAX},flags{},
        original_alpha_bits{UINT64_MAX},final_alpha_bits{UINT64_MAX};
    pose::Identity node{},root{};pose::ArrayHeader output{},affected_header{};
    std::array<std::int32_t,3> affected{{-1,-1,-1}};
};
static_assert(std::is_trivially_copyable_v<WeightReturn>&&std::is_standard_layout_v<WeightReturn>);
struct Proof {
    pose::OwnerKey owner{};
    std::uint64_t compose{},outer_tick{},token{},compute_ordinal{},weight_ordinal{};
    std::uint32_t thread{},side{},kind{},affected_count{},original_alpha_bits{},cached_scale_flag{};
    std::uintptr_t scope_address{},node_argument{},entry_rsp{};
    pose::Metadata metadata{};pose::Identity node{},root{};
    std::array<std::int32_t,3> affected{};
    pose::ArrayHeader output{},affected_header{},scale{};
    bool normal_return{};
};
static_assert(std::is_trivially_copyable_v<Proof>);
}
namespace cover_pose_phase_live {struct Scope;}
namespace full_body_c0_native_live {
void arm_alpha_diagnostic(gears_body_c0_arm_alpha::Diagnostic const&) noexcept;
void arm_weight_return(gears_body_c0_arm_alpha::WeightReturn const&) noexcept;
gears_body_c0_arm_alpha::RetainedControl retained_control_current(cover_pose_phase_live::Scope const&,std::uint64_t) noexcept;
bool arm_control_policy_enabled() noexcept;
// Active permission is separate from the requested/reporting configuration.
bool arm_control_policy_active() noexcept;
// Read-only binding to the existing hand owner scope, before Native joint
// planning. This is admission to validation only, never alpha permission.
bool arm_control_scope(cover_pose_phase_live::Scope const*&,std::uint64_t& outer_tick) noexcept;
bool owns_arm_control(gears_body_c0_arm_alpha::Proof const&) noexcept;
}
