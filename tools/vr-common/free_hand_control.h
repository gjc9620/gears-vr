#pragma once
#include "equipment_intent_bridge.h"
#include <array>
#include <cstdint>

namespace gears_weapon {

enum FreeHandAnimation : std::uint8_t {
    hand_pickup=1,hand_melee=2,hand_weapon_switch=4,hand_other_fixed_action=8
};
enum class FreeHandPhase : std::uint8_t {
    disabled,invalid_player,unproved_animation_state,original_animation,
    missing_tracking,waiting_new_pose,controller_targets
};

// Only the game-thread adapter may assert these facts. In particular, seeing
// bSwitchingWeapons=false is not proof that pickup/melee/other fixed actions
// have all ended. Missing lifecycle evidence leaves ownership_proved=false.
struct FreeHandGameState {
    std::uint64_t world{},observed_ms{};
    std::uintptr_t controller{},pawn{},mesh{},weapon{};
    bool alive{},player_controlled{},ownership_proved{};
    std::uint8_t fixed_animation_mask{};
    bool valid(std::uint64_t now) const noexcept {
        return world&&controller&&pawn&&mesh&&alive&&player_controlled&&
            observed_ms&&now>=observed_ms&&now-observed_ms<=100&&
            !(fixed_animation_mask&~15u);
    }
};
struct FreeHandTargets {
    std::array<gears_vr::Viewpoint,2> world{}; // Left/right controller grip; no palm calibration implied.
    std::uint64_t generation{},session{},sequence{},space_epoch{},input_epoch{},world_generation{},pose_ms{};
    double units_per_metre{};
    std::uint8_t tracked_mask{},eligible_mask{};
    // This module has no certified skeletal writer. A future driver must own
    // a reversible original-animation/GC boundary before applying a target.
    std::uint8_t apply_mask{};
    FreeHandPhase phase{FreeHandPhase::disabled};
};

// Full 6DoF targets from the same native XR ticket as both rendered eyes.
// There is no HMD/current-camera input here, hand-to-hand constraint, smoothing
// of authoritative targets, inventory mutation, firing, or animation call.
class FreeHandControl {
    FreeHandGameState player_{};
    std::uint64_t generation_{},session_{},space_epoch_{},input_epoch_{},world_generation_{};
    std::uint64_t last_sequence_{},after_sequence_{};
    bool player_known_{},animation_owned_{},proof_known_{},wait_new_pose_{};

    void boundary() noexcept {if(++generation_==0)generation_=1;}
    static bool viewpoint_valid(gears_vr::Viewpoint const& pose) noexcept {
        return gears_vr::finite(pose.position)&&
            std::abs(pose.position.x)<1e7&&std::abs(pose.position.y)<1e7&&
            std::abs(pose.position.z)<1e7&&
            std::isfinite(pose.rotation.pitch)&&std::isfinite(pose.rotation.yaw)&&
            std::isfinite(pose.rotation.roll);
    }
    bool same_player(FreeHandGameState const& game) const noexcept {
        return player_known_&&player_.world==game.world&&
            player_.controller==game.controller&&player_.pawn==game.pawn&&
            player_.mesh==game.mesh&&player_.weapon==game.weapon;
    }
    static bool pose_valid(HandWorldFrame const& pose,FreeHandGameState const& game,
                           std::uint64_t now) noexcept {
        return pose.session&&pose.sequence&&pose.space_epoch&&pose.input_epoch&&
            pose.world_generation&&pose.controller==game.controller&&
            pose.pawn==game.pawn&&pose.weapon==game.weapon&&
            pose.created_ms&&now>=pose.created_ms&&now-pose.created_ms<=100&&
            std::isfinite(pose.units_per_metre)&&pose.units_per_metre>=10&&
            pose.units_per_metre<=500&&!(pose.valid_mask&~3u)&&
            !(pose.grip_down_mask&~pose.valid_mask)&&
            (!(pose.valid_mask&1)||viewpoint_valid(pose.left))&&
            (!(pose.valid_mask&2)||viewpoint_valid(pose.right));
    }
public:
    void reset() noexcept {
        // A disable/re-enable must not reuse a generation held by a consumer.
        const auto generation=generation_;
        *this=FreeHandControl{};generation_=generation;boundary();
    }
    FreeHandTargets advance(FreeHandGameState const& game,
                            HandWorldFrame const* pose,std::uint64_t now,
                            bool requested) noexcept {
        FreeHandTargets out{};
        if(!requested){reset();out.generation=generation_;return out;}
        if(!game.valid(now)){
            if(player_known_||!generation_){boundary();player_known_=false;}
            wait_new_pose_=true;after_sequence_=last_sequence_;
            out.generation=generation_;out.phase=FreeHandPhase::invalid_player;return out;
        }
        if(!same_player(game)){
            const bool had_player=player_known_||last_sequence_!=0;
            boundary();player_=game;player_known_=true;
            // Retain the last observed animation owner across weapon/mesh
            // changes. Otherwise an animation ending with that identity
            // change could reuse a pose collected before hand-back.
            proof_known_=false;
            wait_new_pose_=had_player;after_sequence_=last_sequence_;
        }

        bool matched=pose&&pose_valid(*pose,game,now);
        if(matched){
            const bool stream_changed=session_!=pose->session||space_epoch_!=pose->space_epoch||
                input_epoch_!=pose->input_epoch||world_generation_!=pose->world_generation;
            if(stream_changed){
                boundary();session_=pose->session;space_epoch_=pose->space_epoch;
                input_epoch_=pose->input_epoch;world_generation_=pose->world_generation;
                last_sequence_=after_sequence_=0;wait_new_pose_=false;
            }else if(pose->sequence<last_sequence_)matched=false;
            if(matched){
                last_sequence_=pose->sequence;
                out.session=pose->session;out.sequence=pose->sequence;
                out.space_epoch=pose->space_epoch;out.input_epoch=pose->input_epoch;
                out.world_generation=pose->world_generation;out.pose_ms=pose->created_ms;
                out.units_per_metre=pose->units_per_metre;
                out.tracked_mask=pose->valid_mask;
                if(pose->valid_mask&1)out.world[0]=pose->left;
                if(pose->valid_mask&2)out.world[1]=pose->right;
            }
        }

        const bool fixed_action=game.fixed_animation_mask!=0;
        if(fixed_action!=animation_owned_){
            boundary();animation_owned_=fixed_action;
            // Leaving an animation must wait for a post-animation input
            // ticket; the last pose observed during that animation cannot
            // immediately reacquire hands (even when still under 100 ms old).
            wait_new_pose_=true;after_sequence_=last_sequence_;
        }
        if(!game.ownership_proved){
            proof_known_=false;wait_new_pose_=true;after_sequence_=last_sequence_;
        }else proof_known_=true;
        out.generation=generation_;
        if(fixed_action){out.phase=FreeHandPhase::original_animation;return out;}
        if(!proof_known_){out.phase=FreeHandPhase::unproved_animation_state;return out;}
        if(!matched||!out.tracked_mask){out.phase=FreeHandPhase::missing_tracking;return out;}
        if(wait_new_pose_&&pose->sequence<=after_sequence_){
            out.phase=FreeHandPhase::waiting_new_pose;return out;
        }
        wait_new_pose_=false;
        out.eligible_mask=out.tracked_mask;
        out.phase=FreeHandPhase::controller_targets;
        return out; // apply_mask remains zero, including in isolated tests.
    }
};
} // namespace gears_weapon
