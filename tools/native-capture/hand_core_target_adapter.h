#pragma once
#include "../vr-common/native_rig_tracking_frame.h"
#include "../vr-common/hand_authored_mount_profile.h"
#include "../vr-common/weapon_lancer_wrist_mount.h"
#include "cover_control_output_pose.h"
#include "body_tick_input.h"
#include "body_tick_source.h"

// Private caller-owned data and mathematics only. The caller authenticates the
// current Native/Tick/owner/asset and the selected nodes before using any output.
// No engine call, clock query, bridge read, writer permission or certificate.
namespace gears_hand_core_target {
namespace core=gears_native_rig_tracking;
namespace rig=gears_rig_tracking;
namespace mount=gears_hand_authored_mount;
namespace control=gears_cover_control_output_pose;
namespace arm=gears_hand_two_bone;
namespace hand=gears_weapon_hand;
using Side=mount::Side;
using Pose=gears_vr::Pose;
// Internal, explicitly selected authored data. Selecting this is not a held
// equipment proof; the live C0 caller independently admits the current query.
enum class MountKind:std::uint32_t {free_hand,held_lancer};

struct ModelBinding {
    core::NativeSessionKey native{};
    hand::ObjectIdentity world{},pawn{},mesh{},asset{};
    std::uint64_t world_generation{},binding_generation{};
    mount::Model model{mount::Model::unknown};
    mount::Signature geometry_signature{};
    std::uint32_t bone_count{};
    double model_to_world_scale{};
    mount::Mount authored_mount{};
    MountKind mount_kind{MountKind::free_hand};
};
// Always supplied explicitly. Missing companion continuity is valid Core data;
// only a caller selecting this stricter preparation policy requires it here.
struct PreparationPolicy {bool require_producer_continuity;};
struct Target {
    // Keep these large objects in caller-owned persistent/TLS workspace, never
    // in an assembly/POD hook Scope. memcpy preserves all source padding bytes.
    core::CoreSnapshot source{};
    ModelBinding binding{};
    Side side{Side::unknown};
    Pose wrist_world{}; // The one full target shared by Limb and Single.
    std::int64_t checked_at_ns{};
    std::uint64_t maximum_source_age_ns{};
    bool allow_sim{};
    PreparationPolicy policy{};
};
struct TickTarget {
    gears_body_tick_input::Input input{};
    ModelBinding binding{};
    gears_vr::StereoMatrix render_base{};
    Side side{Side::unknown};
    Pose wrist_world{};
    std::int64_t checked_at_ns{};
    bool allow_sim{};
};
struct SourceTarget {
    gears_body_tick_source::Input input{};
    ModelBinding binding{};
    gears_vr::StereoMatrix render_base{};
    Side side{Side::unknown};
    Pose wrist_world{};
    std::int64_t checked_at_ns{};
    bool allow_sim{};
};

enum class Status : std::uint32_t {
    okay,alias,invalid_core,stale_scene,stale_controller,binding_mismatch,
    unknown_mount,missing_continuity,hand_untracked,invalid_node,invalid_target,
    invalid_matrix,invalid_policy,degenerate,unreachable,numerical_failure
};
struct LimbData {
    hand::ObjectIdentity asset{};
    std::uint32_t bone_count{},affected_count{};
    std::array<std::uint32_t,3> affected{};
    control::Matrix64 component[3]{},component_to_world{};
    arm::Pole authored{},previous{}; // Caller-supplied world points, unchanged.
};
struct SingleData {
    hand::ObjectIdentity asset{};
    std::uint32_t bone_count{},affected_count{},affected{};
    control::Matrix64 component{},component_to_world{};
};
// Outputs remain unchanged on every failure. Scratch may change. The complete
// target, node data, workspace and output ranges must be live and disjoint.
struct Workspace {
    control::LimbInput limb{};
    control::SingleInput single{};
    control::Workspace control_work{};
};

namespace detail {
inline bool fresh(std::int64_t stamp,std::int64_t now,std::uint64_t age) noexcept {
    return stamp>0&&now>=stamp&&std::uint64_t(now-stamp)<=age;
}
// Same checked QPC conversion as rig_tracking_live::scene_fresh. This is not a
// new Scene/meshTick mapping; both QPC and frequency are the frozen originals.
inline bool scene_fresh(std::int64_t ticks,std::int64_t frequency,std::int64_t now,std::uint64_t age) noexcept {
    if(ticks<=0||frequency<=0)return false;
    const auto t=std::uint64_t(ticks),f=std::uint64_t(frequency),seconds=t/f,remainder=t%f;
    constexpr auto limit=std::uint64_t((std::numeric_limits<std::int64_t>::max)());
    if(seconds>limit/1000000000||remainder>limit/1000000000)return false;
    const auto ns=seconds*1000000000+remainder*1000000000/f;
    return ns<=limit&&fresh(std::int64_t(ns),now,age);
}
inline bool same_mount(mount::Mount const& a,mount::Mount const& b) noexcept {
    const auto& x=a.calibration;const auto& y=b.calibration;
    return a.model==b.model&&a.side==b.side&&a.provenance==b.provenance&&a.input_space==b.input_space&&
        a.geometry_signature==b.geometry_signature&&a.bone_count==b.bone_count&&a.hand_bone==b.hand_bone&&a.chain==b.chain&&
        a.model_to_world_scale==b.model_to_world_scale&&a.units_per_metre==b.units_per_metre&&x.known&&y.known&&
        gears_vr::stereo_same_pose({x.grip_to_wrist_metres,x.grip_to_wrist_rotation},{y.grip_to_wrist_metres,y.grip_to_wrist_rotation});
}
inline Status binding_valid(core::CoreSnapshot const& source,ModelBinding const& binding,Side side) noexcept {
    if(!mount::valid_side(side)||!core::same(source.native,binding.native)||
       !hand::same(source.world,binding.world)||!hand::same(source.pawn,binding.pawn)||!hand::same(source.mesh,binding.mesh)||
       !hand::identity_valid(binding.asset)||source.world_generation!=binding.world_generation||
       source.binding_generation!=binding.binding_generation)return Status::binding_mismatch;
    mount::Mount expected{};
    bool supported{};
    if(binding.mount_kind==MountKind::free_hand)
        supported=mount::marcus115(binding.geometry_signature,side,binding.model_to_world_scale,source.units_per_metre,expected);
    else if(binding.mount_kind==MountKind::held_lancer&&side==Side::right){
        gears_weapon_lancer_wrist_mount::Profile held{};
        supported=gears_weapon_lancer_wrist_mount::marcus115(binding.geometry_signature,binding.model_to_world_scale,source.units_per_metre,held);
        if(supported)expected=held.mount;
    }
    if(binding.model!=mount::Model::marcus115||!supported||
       binding.bone_count!=expected.bone_count||!same_mount(binding.authored_mount,expected))return Status::unknown_mount;
    return Status::okay;
}
inline Status source_valid(core::CoreSnapshot const& source,Side side,std::int64_t now,std::uint64_t age,
    bool allow_sim,PreparationPolicy policy) noexcept {
    if(!core::core_snapshot_valid(source,now,age,allow_sim))return Status::invalid_core;
    if(!scene_fresh(source.scene_qpc,source.qpc_frequency,now,age))return Status::stale_scene;
    if(!(source.flags&rig::controllers_matched)||!mount::valid_side(side))return Status::hand_untracked;
    if(!fresh(source.controllers.received_ns,now,age)||!fresh(source.controllers.sync_ns,now,age))return Status::stale_controller;
    if(policy.require_producer_continuity&&!(source.flags&rig::producer_continuity_available))return Status::missing_continuity;
    const auto index=static_cast<unsigned>(side);const auto& input=source.controllers.hands[index];
    if(!(source.world_pose_mask&(rig::left_grip_pose<<index))||
       !gears_vr::controller_pose_usable(input.grip,input.grip_flags,input.grip_active))return Status::hand_untracked;
    return Status::okay;
}
// This rechecks data at the recorded preparation instant, not current live
// validity. The caller still obtains/validates its current Tick latch and stop
// state. A Target is ordinary mutable caller data, not an authorization token.
inline bool target_valid(Target const& target) noexcept {
    return source_valid(target.source,target.side,target.checked_at_ns,target.maximum_source_age_ns,
        target.allow_sim,target.policy)==Status::okay&&
        binding_valid(target.source,target.binding,target.side)==Status::okay&&gears_vr::stereo_pose_valid(target.wrist_world);
}
inline Status convert(control::Status status) noexcept {
    switch(status){case control::Status::solved:return Status::okay;case control::Status::alias:return Status::alias;
    case control::Status::invalid_matrix:return Status::invalid_matrix;case control::Status::invalid_target:return Status::invalid_target;
    case control::Status::invalid_policy:return Status::invalid_policy;case control::Status::degenerate:return Status::degenerate;
    case control::Status::unreachable:return Status::unreachable;default:return Status::numerical_failure;}
}
} // namespace detail

inline Status prepare_hand(core::CoreSnapshot const& source,ModelBinding const& binding,Side side,
    std::int64_t now,std::uint64_t max_age,bool allow_sim,PreparationPolicy policy,Target& output) noexcept {
    using namespace control::ik::detail;
    const Range ranges[]={range(&source,sizeof(source)),range(&binding,sizeof(binding)),range(&output,sizeof(output))};
    if(!disjoint(ranges,3))return Status::alias;
    const auto valid=detail::source_valid(source,side,now,max_age,allow_sim,policy);if(valid!=Status::okay)return valid;
    const auto bound=detail::binding_valid(source,binding,side);if(bound!=Status::okay)return bound;
    Pose wrist{};
    if(gears_hand_wrist::native_wrist_target(source.world_grip[static_cast<unsigned>(side)],binding.authored_mount.calibration,
        source.units_per_metre,wrist)!=gears_hand_wrist::Status::okay)return Status::invalid_target;
    // No rejecting operation follows this commit. Keep the large snapshot in
    // caller-owned output rather than adding another full Core on hook stack.
    std::memcpy(&output.source,&source,sizeof(source));std::memcpy(&output.binding,&binding,sizeof(binding));
    output.wrist_world=wrist;output.side=side;output.checked_at_ns=now;output.maximum_source_age_ns=max_age;
    output.allow_sim=allow_sim;output.policy=policy;return Status::okay;
}

namespace detail {
template<class Input,class Target>
inline Status prepare_tracking_math(Input const& input,gears_body_tick_input::Input const& tracking,
    ModelBinding const& binding,Side side,gears_vr::StereoMatrix const& render_base,
    std::int64_t now,bool allow_sim,Target& output) noexcept {
    const auto& source=tracking.parent_scene;
    const auto bound=detail::binding_valid(source,binding,side);if(bound!=Status::okay)return bound;
    Pose grip{},wrist{};
    if(!gears_vr::rig_tracking::pose(render_base,source.reference,tracking.controllers.hands[static_cast<unsigned>(side)].grip,
        source.units_per_metre,grip)||gears_hand_wrist::native_wrist_target(grip,binding.authored_mount.calibration,
        source.units_per_metre,wrist)!=gears_hand_wrist::Status::okay)return Status::invalid_target;
    std::memcpy(&output.input,&input,sizeof input);std::memcpy(&output.binding,&binding,sizeof binding);
    output.render_base=render_base;output.side=side;output.wrist_world=wrist;output.checked_at_ns=now;output.allow_sim=allow_sim;
    return Status::okay;
}
} // namespace detail
inline Status prepare_hand(gears_body_tick_input::Input const& input,ModelBinding const& binding,Side side,
    gears_vr::StereoMatrix const& render_base,std::int64_t now,bool allow_sim,TickTarget& output) noexcept {
    using namespace control::ik::detail;
    const Range ranges[]={range(&input,sizeof input),range(&binding,sizeof binding),range(&render_base,sizeof render_base),range(&output,sizeof output)};
    if(!disjoint(ranges,4))return Status::alias;
    if(gears_body_tick_input::classify(input,input.parent_scene,now,allow_sim)!=gears_body_tick_input::Verdict::okay)return Status::invalid_core;
    return detail::prepare_tracking_math(input,input,binding,side,render_base,now,allow_sim,output);
}
inline Status prepare_hand(gears_body_tick_source::Input const& input,ModelBinding const& binding,Side side,
    gears_vr::StereoMatrix const& render_base,std::int64_t now,bool allow_sim,SourceTarget& output) noexcept {
    using namespace control::ik::detail;
    const Range ranges[]={range(&input,sizeof input),range(&binding,sizeof binding),range(&render_base,sizeof render_base),range(&output,sizeof output)};
    if(!disjoint(ranges,4))return Status::alias;
    if(gears_body_tick_source::classify(input,input.observed.parent_scene,now,allow_sim)!=gears_body_tick_source::Verdict::okay)return Status::invalid_core;
    return detail::prepare_tracking_math(input,input.observed,binding,side,render_base,now,allow_sim,output);
}
// Exact object representation from the same immutable latch, not a nearest or
// reconstructed sample comparison. No stamp, epoch or source value is updated.
inline bool same_source(Target const& target,core::CoreSnapshot const& source) noexcept {
    return std::memcmp(&target.source,&source,sizeof(source))==0;
}

inline Status solve_limb(Target const& target,LimbData const& data,arm::LengthMode mode,
    Workspace& work,control::Matrix64* output) noexcept {
    using namespace control::ik::detail;
    const Range ranges[]={range(&target,sizeof(target)),range(&data,sizeof(data)),range(&work,sizeof(work)),range(output,3*sizeof(*output))};
    if(!disjoint(ranges,4))return Status::alias;
    if(!detail::target_valid(target))return Status::invalid_target;
    const auto& profile=target.binding.authored_mount;
    if(!hand::same(data.asset,target.binding.asset)||data.bone_count!=profile.bone_count||data.affected_count!=3||data.affected!=profile.chain)return Status::invalid_node;
    if(mode!=arm::LengthMode::preserve&&mode!=arm::LengthMode::adaptive)return Status::invalid_policy;
    for(unsigned i=0;i<3;++i)work.limb.component[i]=data.component[i];
    work.limb.component_to_world=data.component_to_world;work.limb.wrist_world=target.wrist_world;
    work.limb.mode=mode;work.limb.authored=data.authored;work.limb.previous=data.previous;
    return detail::convert(control::solve(work.limb,work.control_work,output));
}
inline Status replace_wrist_rotation(Target const& target,SingleData const& data,Workspace& work,control::Matrix64* output) noexcept {
    using namespace control::ik::detail;
    const Range ranges[]={range(&target,sizeof(target)),range(&data,sizeof(data)),range(&work,sizeof(work)),range(output,sizeof(*output))};
    if(!disjoint(ranges,4))return Status::alias;
    if(!detail::target_valid(target))return Status::invalid_target;
    const auto& profile=target.binding.authored_mount;
    if(!hand::same(data.asset,target.binding.asset)||data.bone_count!=profile.bone_count||data.affected_count!=1||data.affected!=profile.hand_bone)return Status::invalid_node;
    work.single.component=data.component;work.single.component_to_world=data.component_to_world;
    work.single.world_orientation=target.wrist_world.orientation;
    return detail::convert(control::replace_wrist_rotation(work.single,work.control_work,output));
}
static_assert(std::is_standard_layout_v<ModelBinding>&&std::is_trivially_copyable_v<ModelBinding>);
static_assert(std::is_standard_layout_v<SourceTarget>&&std::is_trivially_copyable_v<SourceTarget>);
static_assert(std::is_standard_layout_v<Target>&&std::is_trivially_copyable_v<Target>);
static_assert(std::is_standard_layout_v<PreparationPolicy>&&std::is_trivially_copyable_v<PreparationPolicy>);
} // namespace gears_hand_core_target
