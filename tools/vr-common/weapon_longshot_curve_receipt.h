#pragma once
#include "weapon_hand_participant.h"
#include "controller_continuity_frame.h"
#include <cmath>

// Data receipt from the selected-node observations already performed at BOTH
// ends of read_curve. It is usable only inside that exact Frame/input/context.
// It carries no new Frame, clock, lease or action authority. The shared seek
// still checks current ownership, original functions/arguments and100ms before
// dispatch. This only avoids a third getter with no intervening engine call.
namespace gears_longshot_curve {
namespace hand=gears_weapon_hand;
struct Receipt {
    hand::FrameKey key{};
    hand::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{},component{},asset{},node{},sequence{};
    gears_vr::ControllerInputFrame raw{};
    std::uint64_t reference{},press{},session{},sequence_number{},space{},profile{},input_epoch{};
    std::int64_t sync_ns{},predicted_time{},frame_now_ns{};
    std::uint32_t flags{},control{},frame_size{},frame_version{},left_known{},left_down{},left_tracked{};
    float reload_start{},length{};
    bool complete{};
};
static_assert(std::is_trivially_copyable_v<Receipt>);
template<class Observation>
Receipt capture(hand::Frame const& f,Observation const& curve_sample) noexcept {
    Receipt r{};r.key=f.key;r.world=f.world;r.controller=f.controller;r.pawn=f.pawn;r.mesh=f.mesh;r.weapon=f.weapon;r.raw=f.controllers;
    r.component=curve_sample.component;r.asset=curve_sample.asset;r.node=curve_sample.node;r.sequence=curve_sample.sequence;
    r.reference=f.reference_generation;r.press=f.press_token;r.flags=f.flags;r.control=f.interaction_control;
    r.frame_now_ns=f.now_ns;r.frame_size=f.size;r.frame_version=f.abi_version;
    r.left_known=f.left_grip_known;r.left_down=f.left_grip_down;r.left_tracked=f.left_tracked;
    r.session=f.controllers.session;r.sequence_number=f.controllers.sequence;r.space=f.controllers.space_epoch;
    r.profile=f.controllers.profile_generation;r.input_epoch=f.controllers.input_epoch;
    r.sync_ns=f.controllers.sync_ns;r.predicted_time=f.controllers.predicted_time;
    r.reload_start=curve_sample.reload_start;r.length=curve_sample.length;
    r.complete=f.key.frame&&f.key.game_pid&&f.key.thread_id&&f.controllers.sync_ns>0&&
        hand::identity_valid(curve_sample.node)&&hand::identity_valid(curve_sample.sequence)&&
        hand::same(curve_sample.weapon,f.weapon)&&std::isfinite(r.reload_start)&&std::isfinite(r.length)&&r.length>0;
    return r;
}
template<class Observation>
bool matches(Receipt const& r,hand::Frame const& f,Observation const& current) noexcept {
    return r.complete&&hand::same(r.key,f.key)&&gears_hand_continuity::same_input(r.raw,f.controllers)&&r.sync_ns==f.controllers.sync_ns&&
        r.session==f.controllers.session&&r.sequence_number==f.controllers.sequence&&
        r.space==f.controllers.space_epoch&&r.profile==f.controllers.profile_generation&&
        r.input_epoch==f.controllers.input_epoch&&r.predicted_time==f.controllers.predicted_time&&
        r.reference==f.reference_generation&&r.press==f.press_token&&r.flags==f.flags&&r.control==f.interaction_control&&
        r.frame_now_ns==f.now_ns&&r.frame_size==f.size&&r.frame_version==f.abi_version&&
        r.left_known==f.left_grip_known&&r.left_down==f.left_grip_down&&r.left_tracked==f.left_tracked&&
        hand::same(r.world,f.world)&&hand::same(r.controller,f.controller)&&hand::same(r.pawn,f.pawn)&&
        hand::same(r.mesh,f.mesh)&&hand::same(r.weapon,f.weapon)&&hand::same(r.weapon,current.weapon)&&
        hand::same(r.component,current.component)&&hand::same(r.asset,current.asset)&&
        hand::same(r.node,current.node)&&hand::same(r.sequence,current.sequence)&&
        r.reload_start==current.reload_start&&r.length==current.length;
}
}
