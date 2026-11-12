#pragma once
#include "cover_neutral_goal.h"
#include "../vr-common/cover_body_target.h"

namespace gears_cover_committed {
namespace ng=gears_cover_neutral_goal;
namespace a=gears_body_goal_view;
namespace v=gears_vr;
// Persistent INTERACTION state, not a tracking receipt or movement/write permit.
// A release may hold this offset indefinitely in its unchanged cover domain.
// Every C0 application must independently admit its CURRENT source/tracking.
struct State {
    v::CoverBodyTargetKey key{};
    gears_weapon_hand::ObjectIdentity pawn{},mesh{},link{};
    v::CoverViewAnchor anchor{},view_anchor{};
    v::CoverOffset offset{};v::CoverRange range{};
    v::Pose reference{};v::StereoSource tracking_source{};
    double units_per_metre{};unsigned type{};
    std::int64_t observed_ns{};
};
inline bool epochs(v::CoverEpochs const& x,v::CoverEpochs const& y)noexcept{
    return x.world==y.world&&x.pawn==y.pawn&&x.cover==y.cover&&x.slot==y.slot&&
        x.session==y.session&&x.space==y.space&&x.reference==y.reference;
}
inline bool valid(State const& s,std::int64_t now)noexcept{
    const auto& r=s.range;
    return v::cover_body_target_key_valid(s.key)&&(s.type==1||s.type==2)&&
        s.pawn.address==s.key.pawn&&s.mesh.address==s.key.mesh&&s.link.address==s.key.link&&
        s.observed_ns>0&&s.observed_ns<=now&&std::isfinite(s.units_per_metre)&&s.units_per_metre>=10&&s.units_per_metre<=500&&
        v::cover_view_basis(s.anchor)&&v::cover_view_basis(s.view_anchor)&&
        ng::same(s.anchor.neutral_eye_world,s.view_anchor.neutral_eye_world)&&v::stereo_pose_valid(s.reference)&&
        std::isfinite(r.min_right)&&std::isfinite(r.max_right)&&std::isfinite(r.min_up)&&std::isfinite(r.max_up)&&
        r.min_right<=0&&r.max_right>=0&&r.min_up==0&&r.max_up>=0&&
        std::isfinite(s.offset.right)&&std::isfinite(s.offset.up)&&
        s.offset.right>=r.min_right&&s.offset.right<=r.max_right&&s.offset.up>=r.min_up&&s.offset.up<=r.max_up&&
        (s.type==2||(s.offset.up==0&&r.max_up==0));
}
template<class Source>bool domain(State const& s,Source const& current,a::Policy const& policy)noexcept{
    const auto& parent=ng::parent(current);const auto& tracking=ng::tracking(current);
    // Old Scene/sequence/clocks are intentionally NOT relabelled or compared
    // to the new sample. They remain diagnostics of the interaction update.
    // Complete object identities, cover type/slot and reference cohort bind it.
    return valid(s,policy.now_ns)&&(parent.flags&a::core::rig::cover_key_known)&&
        s.key.world==parent.world.address&&s.key.world_generation==parent.world_generation&&
        gears_weapon_hand::same(s.pawn,parent.pawn)&&gears_weapon_hand::same(s.mesh,parent.mesh)&&
        gears_weapon_hand::same(s.link,parent.cover_link)&&s.type==parent.cover_type&&s.key.slot==parent.cover_slot&&
        s.key.thread==parent.native.game_thread&&s.key.xr_session==tracking.session&&s.key.xr_space==tracking.space_epoch&&
        s.key.reference==parent.reference_generation&&s.tracking_source==tracking.source&&
        v::stereo_same_pose(s.reference,parent.reference)&&s.units_per_metre==parent.units_per_metre;
}
} // namespace gears_cover_committed
