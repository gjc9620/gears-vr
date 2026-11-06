#pragma once
#include "committed_hand_endpoint.h"

// Supplemental, read-only pair of ACTUAL C0 palms. Existing endpoint/visual
// V2 endpoint purpose is explicit. These observations confer no action or writer.
namespace gears_committed_interaction {
namespace hand=gears_weapon_hand;
inline constexpr unsigned version=4;
struct Frame {
    unsigned size{sizeof(Frame)},abi_version{version};
    gears_committed_hand::Frame right{};
    gears_vr::Pose reference{},left_grip{},left_wrist{};
    gears_vr::Pose physical_left_grip{}; // Fresh unsnapped palm with the same fixed-arm reach limit; never cosmetic fitting.
    double model_scale{};
};
struct Request {
    unsigned size{sizeof(Request)},abi_version{version},response_bytes{sizeof(Frame)},reserved{};
    std::uint64_t cookie{},native_started{};
};
// Availability/identity can match while the completed pose still predates a
// new button edge. Acquisition must wait for the selected control's cohort;
// this comparison alone grants no contact, pose, source or action authority.
inline bool same_left_control_cohort(Frame const& actual,hand::Frame const& expected) noexcept {
    if(expected.interaction_control>unsigned(gears_vr::LeftInteractionControl::pinch))return false;
    auto const& a=actual.right.controllers;auto const& b=expected.controllers;
    if(a.source!=b.source||a.session!=b.session||a.space_epoch!=b.space_epoch||
       a.input_epoch!=b.input_epoch||a.profile_generation!=b.profile_generation||
       a.sequence>b.sequence||a.sync_ns>b.sync_ns)return false;
    const unsigned control=hand::left_control(expected);
    auto const& old=a.hands[0];auto const& current=b.hands[0];
    const auto active=[&](gears_vr::ControllerHand const& h){
        return control==gears_vr::control_trigger?h.trigger_active:h.squeeze_active;
    };
    return active(old)==1&&active(current)==1&&
        old.pressed[control]==current.pressed[control]&&old.released[control]==current.released[control]&&
        hand::control_held(old,control)==hand::control_held(current,control);
}
inline bool shape(Request const& r) noexcept {
    return r.size==sizeof r&&r.abi_version==version&&r.response_bytes==sizeof(Frame)&&!r.reserved&&r.cookie&&r.native_started;
}
inline bool matches(Frame const& f,hand::Frame const& a,std::uint64_t held,std::int64_t now,bool sim) noexcept {
    const auto& r=f.right;
    return f.size==sizeof f&&f.abi_version==version&&held&&
        r.purpose==gears_committed_hand::Purpose::held_lancer&&gears_committed_hand::current_result(r,now,sim)&&
        gears_vr::stereo_pose_valid(f.left_grip)&&gears_vr::stereo_pose_valid(f.left_wrist)&&gears_vr::stereo_pose_valid(f.physical_left_grip)&&
        std::isfinite(f.model_scale)&&f.model_scale>0&&gears_vr::stereo_same_pose(f.reference,a.reference)&&
        gears_vr::controller_pose_usable(r.controllers.hands[0].grip,r.controllers.hands[0].grip_flags,r.controllers.hands[0].grip_active)&&
        r.hold_generation==held&&r.parent.registration_cookie==a.key.registration_cookie&&r.parent.native_started_qpc==a.key.native_started_qpc&&
        r.parent.game_pid==a.key.game_pid&&r.parent.game_start==a.key.game_start&&r.parent.thread_id==a.key.thread_id&&
        r.parent.world_generation==a.key.world_generation&&r.parent.lifecycle==a.key.lifecycle&&r.parent.frame<=a.key.frame&&
        hand::same(r.world,a.world)&&hand::same(r.controller,a.controller)&&hand::same(r.pawn,a.pawn)&&
        hand::same(r.mesh,a.mesh)&&hand::same(r.weapon,a.weapon)&&r.reference_generation==a.reference_generation&&
        !a.recentered&&!a.heading_deferred&&r.stereo.source==a.stereo.source&&r.stereo.session==a.stereo.session&&
        r.stereo.space_epoch==a.stereo.space_epoch&&r.controllers.input_epoch==a.controllers.input_epoch&&
        r.controllers.profile_generation==a.controllers.profile_generation;
}
#if defined(_WIN32)
using Read=gears_rig_tracking::Reply(__cdecl*)(Request const*,Frame*) noexcept;
#else
using Read=gears_rig_tracking::Reply(*)(Request const*,Frame*) noexcept;
#endif
inline constexpr char read_export[]="GearsVrReadCommittedHandInteractionV4";
static_assert(std::is_trivially_copyable_v<Frame>&&std::is_standard_layout_v<Frame>);
}
