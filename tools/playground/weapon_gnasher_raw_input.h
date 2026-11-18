#pragma once
#include "weapon_rig_binding.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/weapon_gnasher_mechanics.h"
// Data conversion only. Raw prepared world grips, never C0 wrists/skin hands.
namespace gears_gnasher_raw {
namespace wh=gears_weapon_hand;namespace rig=gears_rig_tracking;
// Only an idle, bound, loaded mechanism with no left Grip can omit the
// optional support receipt. This policy never admits a shot or drives a stroke.
inline bool ownership_required(wh::Frame const& source,
    gears_weapon::GnasherMechanicalInteraction const& mechanism) noexcept {
    using Stage=gears_weapon::GnasherMechanicalStage;
    return source.left_grip_down||!source.left_grip_known||
        mechanism.stage()!=Stage::ready||mechanism.hold_suspended()||
        mechanism.reload_active()||mechanism.animation_owns_hands()||
        mechanism.held_round_serial()||mechanism.inventory().loaded==0;
}
inline bool ownership_matches(wh::Frame const& source,wh::GripSnapshot const& ownership,
                              std::int64_t now_ns) noexcept {
    return ownership.current(source.key,source.weapon,now_ns)&&
        wh::same(ownership.world,source.world)&&wh::same(ownership.pawn,source.pawn)&&
        wh::same(ownership.controller,source.controller)&&wh::same(ownership.mesh,source.mesh)&&
        ownership.input_session==source.stereo.session&&ownership.space_epoch==source.stereo.space_epoch&&
        ownership.input_sequence==source.controllers.sequence&&ownership.input_epoch==source.controllers.input_epoch&&
        ownership.profile_generation==source.controllers.profile_generation&&
        ownership.reference_generation==source.reference_generation&&ownership.press_token==source.press_token&&
        ownership.stereo_received_ns==source.stereo.received_ns&&ownership.input_received_ns==source.controllers.received_ns&&
        wh::lease_valid(ownership.left_hand,source.key.lifecycle);
}
inline bool support_held(wh::Frame const& source,wh::GripSnapshot const& ownership,
                        std::int64_t now_ns) noexcept {
    return wh::frame_active(source,now_ns)&&ownership_matches(source,ownership,now_ns)&&
        ownership.grip==wh::Grip::two&&ownership.left_hand.kind==wh::LeaseKind::support&&
        ownership.left_hand.id&&ownership.left_hand.id==source.press_token&&
        source.left_tracked&&source.left_grip_known&&source.left_grip_down;
}
inline bool sample(wh::Frame const& source,rig::Frame const& prepared,
    wh::GripGameState const& game,wh::GripSnapshot const* ownership,
    std::int64_t now_ns,std::int64_t game_tick,std::uint64_t round_serial,
    bool original_animation_active,bool original_ready,
    gears_weapon::GnasherMechanicalSample& out) noexcept {
    out={};
    const auto kind=static_cast<std::uint32_t>(source.stereo.source);
    const auto masks=rig::left_grip_pose|rig::right_grip_pose;
    if(game_tick<0||!game.known||!game.main_hand_held||
       gears_equipment_rig::match(source,prepared,now_ns,kind)!=gears_equipment_rig::Status::matched||
       (prepared.world_pose_mask&masks)!=masks)return false;
    bool support{};
    if(ownership){
        if(!ownership_matches(source,*ownership,now_ns))return false;
        support=support_held(source,*ownership,now_ns);
    }
    if(support&&round_serial)return false; // No simultaneous held round and support lease.
    gears_weapon::GnasherMechanicalSample value{};
    value.identity={source.world.address,source.pawn.address,source.weapon.address,source.key.world_generation,
        source.stereo.session,source.stereo.space_epoch,source.controllers.input_epoch,source.controllers.profile_generation};
    value.sequence=source.controllers.sequence;value.monotonic_ns=source.controllers.sync_ns;value.game_tick=game_tick;
    value.left_grip=prepared.world_grip[0];value.right_grip=prepared.world_grip[1];
    value.left_grip.position=gears_weapon::scaled(value.left_grip.position,1/prepared.units_per_metre);
    value.right_grip.position=gears_weapon::scaled(value.right_grip.position,1/prepared.units_per_metre);
    value.left_tracked=value.right_tracked=value.right_handle_held=true;
    value.left_front_held=support;value.left_round_serial=round_serial;
    value.support_grasp_token=support?ownership->left_hand.id:0;
    value.reference_generation=source.reference_generation;
    value.original_animation_active=original_animation_active;value.original_ready=original_ready;
    if(!value.identity.valid()||!gears_vr::stereo_pose_valid(value.left_grip)||!gears_vr::stereo_pose_valid(value.right_grip))return false;
    out=value;return true;
}
}
