#pragma once
#include "equipment_intent_bridge.h"
#include "weapon_lancer_chainsaw.h"

namespace gears_weapon {
// The game-thread adapter must certify these identities and original states
// before building a gesture sample. Starter geometry is an explicit world
// pose attached to the current gun, never a guessed offset from the HMD.
struct LancerSawGamePose {
    std::uint64_t world{},world_generation{};
    std::uintptr_t controller{},pawn{},weapon{};
    Vector starter_anchor_raw{},pull_axis_world{};
    bool lancer_active{},right_owns_weapon{},game_can_melee{},original_hold{},
         original_execution{},original_ready{},original_button_held{};
    std::uint64_t original_victim{};
    LancerSawContact contact{};
};

inline bool lancer_saw_sample_from_world(HandWorldFrame const& pose,
        LancerSawGamePose const& game,std::uint64_t now_ms,
        LancerSawSample& out) noexcept {
    if(!game.world||!game.world_generation||!game.controller||!game.pawn||!game.weapon||
       game.controller!=pose.controller||game.pawn!=pose.pawn||game.weapon!=pose.weapon||
       game.world_generation!=pose.world_generation||
       !pose.session||!pose.sequence||!pose.space_epoch||!pose.input_epoch||
       !pose.created_ms||now_ms<pose.created_ms||now_ms-pose.created_ms>100||
       !pose.valid_mask||(pose.valid_mask&~3u)||
       pose.grip_down_mask&~pose.valid_mask||
       !std::isfinite(pose.units_per_metre)||pose.units_per_metre<10||
       pose.units_per_metre>500||
       ((pose.valid_mask&2)&&!gears_vr::finite(pose.right.position))||
       ((pose.valid_mask&1)&&!gears_vr::finite(pose.left.position))||
       !gears_vr::finite(game.starter_anchor_raw)||
       !gears_vr::finite(game.pull_axis_world)||
       std::abs(length(game.pull_axis_world)-1.)>.1)return false;
    const auto to_metres=1./pose.units_per_metre;
    LancerSawSample candidate{};
    candidate.world=game.world;candidate.pawn=game.pawn;candidate.weapon=game.weapon;
    candidate.session=pose.session;candidate.space_epoch=pose.space_epoch;
    candidate.input_epoch=pose.input_epoch;
    candidate.world_generation=pose.world_generation;
    candidate.sequence=pose.sequence;candidate.created_ms=pose.created_ms;
    if(pose.valid_mask&1)candidate.left_palm_m=scaled(pose.left.position,to_metres);
    candidate.starter_anchor_m=scaled(game.starter_anchor_raw,to_metres);
    candidate.pull_axis_m=game.pull_axis_world;
    candidate.lancer_active=game.lancer_active;
    candidate.right_owns_weapon=game.right_owns_weapon;
    candidate.right_tracked=(pose.valid_mask&2)!=0;
    candidate.left_tracked=(pose.valid_mask&1)!=0;
    candidate.left_grip_down=(pose.grip_down_mask&1)!=0;
    candidate.game_can_melee=game.game_can_melee;
    candidate.original_hold=game.original_hold;
    candidate.original_execution=game.original_execution;
    candidate.original_ready=game.original_ready;
    candidate.original_button_held=game.original_button_held;
    candidate.original_victim=game.original_victim;
    candidate.contact=game.contact;
    out=candidate;return true;
}
} // namespace gears_weapon
