#pragma once
#include "../vr-common/weapon_lancer_saw_input.h"
#include "../vr-common/weapon_hand_client.h"

namespace hammerburst_magazine_pin {
namespace hand=gears_weapon_hand;
struct Observation {
    hand::ObjectIdentity weapon{},component{},world{},pawn{},mesh{},controller_identity{};
    hand::LeaseKind lease_kind{};
    std::uint64_t item{},lease_id{},lease_lifecycle{},hold_generation{},stage{},controller{};
};
struct Seam {
    bool (*lifetime)(void*,hand::ExistingActionLifetime const&) noexcept{};
};
struct Pin {
    hand::Frame parent{};hand::ExistingActionLifetime lifetime{};
    gears_lancer_saw_input::HoldInput input;Observation observed{};bool known{};
    void reset() noexcept {*this={};}
};
inline bool bind(Pin& pin,hand::Frame const& f,hand::Decision const& decision,std::uint64_t item,
    hand::ObjectIdentity const& component,std::uint64_t stage,Observation const& current,
    gears_vr::StereoSample const& stereo,gears_hand_continuity::TrackedInput const& input,
    gears_hand_continuity::Publisher publisher,std::int64_t now,bool allow_sim,
    hand::ExistingActionLifetime const& lifetime) noexcept {
    pin.reset();if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||
       !gears_hand_continuity::same_input(input.input,f.controllers)||
       !hand::same(decision.key,f.key)||decision.press_token!=f.press_token||
       decision.release_required||(decision.status!=hand::DecisionStatus::granted&&
       decision.status!=hand::DecisionStatus::retained)||decision.lease.id!=f.press_token||
       !hand::lease_valid(decision.lease,f.key.lifecycle)||!item||!hand::identity_valid(component)||
       !hand::identity_valid(f.weapon)||!hand::identity_valid(f.world)||!hand::identity_valid(f.pawn)||
       !hand::identity_valid(f.mesh)||!hand::identity_valid(f.controller)||
       !hand::equipment_identity_valid(lifetime.parent)||!hand::equipment_matches(f,lifetime.parent)||
       !hand::same(current.world,f.world)||!hand::same(current.pawn,f.pawn)||
       !hand::same(current.mesh,f.mesh)||!hand::same(current.controller_identity,f.controller)||
       current.lease_kind!=decision.lease.kind||
       !((stage==4&&decision.lease.kind==hand::LeaseKind::magazine)||
         (stage==6&&decision.lease.kind==hand::LeaseKind::ammo))||
       current.item!=item||current.lease_id!=decision.lease.id||
       current.lease_lifecycle!=decision.lease.lifecycle||!current.hold_generation||
       current.stage!=stage||current.controller!=f.controller.address||
       !hand::same(current.weapon,f.weapon)||!hand::same(current.component,component)||
       !lifetime.epoch)return false;
    if(!pin.input.bind(stereo,input,publisher,now,allow_sim,hand::left_control(f)))return false;
    pin.parent=f;pin.lifetime=lifetime;pin.observed=current;pin.observed.item=item;
    pin.observed.stage=stage;pin.observed.component=component;pin.known=true;return true;
}
inline bool retain(Pin& pin,hand::FrameReadObservation const& failure,Seam const& seam,void* context,
    gears_vr::StereoSample const& stereo,gears_hand_continuity::TrackedInput const& input,
    gears_hand_continuity::Publisher publisher,std::int64_t now,bool allow_sim,
    Observation const& current) noexcept {
    if(!pin.known||failure.game_difference||failure.frame_difference||
       (failure.failure!=hand::FrameReadFailure::publication&&
       failure.failure!=hand::FrameReadFailure::stale_before&&
       failure.failure!=hand::FrameReadFailure::thread_or_unarmed))return false;
    if(!seam.lifetime||!hand::same(current.weapon,pin.observed.weapon)||
       !hand::same(current.component,pin.observed.component)||current.item!=pin.observed.item||
       !hand::same(current.world,pin.observed.world)||!hand::same(current.pawn,pin.observed.pawn)||
       !hand::same(current.mesh,pin.observed.mesh)||
       !hand::same(current.controller_identity,pin.observed.controller_identity)||
       current.lease_kind!=pin.observed.lease_kind||
       current.lease_id!=pin.observed.lease_id||current.lease_lifecycle!=pin.observed.lease_lifecycle||
       current.hold_generation!=pin.observed.hold_generation||current.stage!=pin.observed.stage||
       current.controller!=pin.observed.controller||!seam.lifetime(context,pin.lifetime))return false;
    return pin.input.refresh(stereo,input,publisher,now,allow_sim);
}
} // namespace hammerburst_magazine_pin
