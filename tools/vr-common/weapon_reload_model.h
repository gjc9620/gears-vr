#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace gears_weapon {
enum class LoadUnit : std::uint8_t { magazine, round };
// The hand adapter must prove the named geometry and complete travel before
// reporting an action. Gnasher cycles by lowering and returning the right-hand
// grip while the left hand holds the front, not by pumping the front hand.
enum class MechanismAction : std::uint8_t { none, bolt, lever_down_return };
enum class ReloadStage : std::uint8_t { idle, feed_port, holding_ammo, mechanism, complete };
enum class ReloadEvent : std::uint8_t {
    rejected, invalid, full, no_ammo, started, grabbed, released, inserted,
    mechanism_needed, perfect, ordinary, cancelled, cycled
};

// Configured per verified game weapon. The deadline is the original perfect
// window's END measured in game-clock ticks, not the normal reload duration.
struct ReloadPolicy {
    LoadUnit unit{LoadUnit::round};
    std::uint32_t capacity{},maximum_per_cycle{};
    bool final_mechanism{};
    bool cycle_after_each_shot{};
    std::int64_t perfect_end_ticks{};
    MechanismAction reload_action{MechanismAction::bolt};
    MechanismAction shot_action{MechanismAction::none};
};

// Pure transaction model. The live adapter must authenticate the weapon and
// commit every inventory delta exactly once on the game thread. This model
// itself never writes game memory or calls the original reward functions.
class ReloadModel {
    ReloadPolicy policy_{};
    ReloadStage stage_{ReloadStage::idle};
    std::uint64_t weapon_{};
    std::uint32_t loaded_{},reserve_{},target_{},inserted_{},held_{};
    std::int64_t start_tick_{};
    bool reward_issued_{},pending_mechanism_{},feed_present_{true};
    MechanismAction pending_action_{MechanismAction::none};
    ReloadEvent finish(std::int64_t tick) noexcept {
        if(stage_!=ReloadStage::mechanism || tick<start_tick_ || reward_issued_)return ReloadEvent::rejected;
        stage_=ReloadStage::complete;reward_issued_=true;pending_mechanism_=false;
        pending_action_=MechanismAction::none;
        // The original native judge includes the right edge of its perfect
        // interval (vcomiss followed by jbe). VR also accepts earlier finish.
        return tick-start_tick_<=policy_.perfect_end_ticks?ReloadEvent::perfect:ReloadEvent::ordinary;
    }
public:
    void reset() noexcept {*this=ReloadModel{};}
    // Seed an already-equipped original weapon before its first VR shot.
    // The adapter must reset on weapon/world change and obtain both counts
    // from the same authenticated game-thread inventory snapshot. Re-priming
    // an armed model would erase a pending post-shot mechanism, so it fails.
    bool prime(std::uint64_t weapon,ReloadPolicy policy,std::uint32_t loaded,
               std::uint32_t reserve) noexcept {
        if(weapon_||!weapon||policy.capacity==0||policy.capacity>10000||
           policy.maximum_per_cycle==0||policy.maximum_per_cycle>policy.capacity||
           loaded>policy.capacity||reserve>1000000||
           (policy.final_mechanism&&policy.reload_action==MechanismAction::none)||
           (policy.cycle_after_each_shot&&policy.shot_action==MechanismAction::none)||
           (policy.unit!=LoadUnit::round&&policy.unit!=LoadUnit::magazine))return false;
        policy_=policy;weapon_=weapon;loaded_=loaded;reserve_=reserve;
        stage_=ReloadStage::idle;feed_present_=true;return true;
    }
    ReloadEvent begin(std::uint64_t weapon,ReloadPolicy policy,std::uint32_t loaded,
                      std::uint32_t reserve,std::int64_t game_tick) noexcept {
        if(stage_==ReloadStage::feed_port||stage_==ReloadStage::holding_ammo||stage_==ReloadStage::mechanism)
            return ReloadEvent::rejected;
        if(weapon_&&weapon!=weapon_)return ReloadEvent::rejected;
        if(!weapon||policy.capacity==0||policy.capacity>10000||
           policy.maximum_per_cycle==0||policy.maximum_per_cycle>policy.capacity||
           policy.perfect_end_ticks<=0||game_tick<0||
           loaded>policy.capacity||reserve>1000000||
           (policy.final_mechanism&&policy.reload_action==MechanismAction::none)||
           (policy.cycle_after_each_shot&&policy.shot_action==MechanismAction::none)||
           (policy.unit!=LoadUnit::round&&policy.unit!=LoadUnit::magazine))return ReloadEvent::invalid;
        if(loaded==policy.capacity)return ReloadEvent::full;
        if(!reserve)return ReloadEvent::no_ammo;
        policy_=policy;stage_=ReloadStage::feed_port;weapon_=weapon;
        loaded_=loaded;reserve_=reserve;held_=inserted_=0;
        if(!pending_mechanism_)pending_action_=MechanismAction::none;
        if(policy.unit==LoadUnit::magazine){
            // An ejected partial magazine returns its remaining rounds to the
            // available pool. Until a replacement is inserted, no feed exists.
            reserve_+=loaded_;loaded_=0;feed_present_=false;
            target_=(std::min)(policy.maximum_per_cycle,reserve_);
            pending_mechanism_=policy.final_mechanism;
            if(pending_mechanism_)pending_action_=policy.reload_action;
        }else{
            target_=(std::min)({policy.maximum_per_cycle,policy.capacity-loaded_,reserve_});
        }
        start_tick_=game_tick;reward_issued_=false;
        return ReloadEvent::started;
    }
    ReloadEvent grab(std::uint64_t weapon) noexcept {
        if(stage_!=ReloadStage::feed_port||weapon!=weapon_||!target_||held_||inserted_>=target_)
            return ReloadEvent::rejected;
        if(!reserve_)return ReloadEvent::no_ammo;
        held_=policy_.unit==LoadUnit::magazine?target_-inserted_:1;
        if(held_>reserve_)return ReloadEvent::no_ammo;
        reserve_-=held_;stage_=ReloadStage::holding_ammo;
        return ReloadEvent::grabbed;
    }
    ReloadEvent release(std::uint64_t weapon) noexcept {
        if(stage_!=ReloadStage::holding_ammo||weapon!=weapon_||!held_)return ReloadEvent::rejected;
        reserve_+=held_;held_=0;stage_=ReloadStage::feed_port;
        return ReloadEvent::released;
    }
    // Geometry/order validation belongs to the hand and weapon-pose adapter.
    // It must only call insert after the matching item reaches the highlighted
    // feed port with a valid insertion trajectory.
    ReloadEvent insert(std::uint64_t weapon,std::int64_t game_tick) noexcept {
        if(stage_!=ReloadStage::holding_ammo||weapon!=weapon_||!held_||
           game_tick<start_tick_||loaded_>policy_.capacity-held_)return ReloadEvent::rejected;
        loaded_+=held_;inserted_+=held_;held_=0;
        if(policy_.unit==LoadUnit::magazine)feed_present_=true;
        if(policy_.final_mechanism){pending_mechanism_=true;pending_action_=policy_.reload_action;}
        if(inserted_<target_){stage_=ReloadStage::feed_port;return ReloadEvent::inserted;}
        stage_=ReloadStage::mechanism;
        if(policy_.final_mechanism)return ReloadEvent::mechanism_needed;
        return finish(game_tick);
    }
    ReloadEvent cycle(std::uint64_t weapon,std::int64_t game_tick,MechanismAction action) noexcept {
        if(weapon!=weapon_||!pending_mechanism_||action==MechanismAction::none||
           action!=pending_action_||!feed_present_)return ReloadEvent::rejected;
        if((stage_==ReloadStage::idle||stage_==ReloadStage::complete)&&pending_mechanism_){
            pending_mechanism_=false;pending_action_=MechanismAction::none;return ReloadEvent::cycled;
        }
        return finish(game_tick);
    }
    // Called only after the original game has actually fired and decremented
    // the chamber count. It gates the next shotgun shot until a real cycle.
    bool after_shot(std::uint64_t weapon,std::uint32_t loaded_after_shot) noexcept {
        if(weapon!=weapon_||!policy_.cycle_after_each_shot||!can_fire()||
           loaded_after_shot+1!=loaded_)return false;
        loaded_=loaded_after_shot;pending_mechanism_=true;
        pending_action_=policy_.shot_action;return true;
    }
    ReloadEvent cancel(std::uint64_t weapon) noexcept {
        if(weapon!=weapon_||stage_==ReloadStage::idle||stage_==ReloadStage::complete)
            return ReloadEvent::rejected;
        if(held_>std::numeric_limits<std::uint32_t>::max()-reserve_)return ReloadEvent::rejected;
        reserve_+=held_;held_=0;stage_=ReloadStage::idle;
        return ReloadEvent::cancelled;
    }
    ReloadStage stage() const noexcept {return stage_;}
    std::uint32_t loaded() const noexcept {return loaded_;}
    std::uint32_t reserve() const noexcept {return reserve_;}
    std::uint32_t held() const noexcept {return held_;}
    std::uint32_t target() const noexcept {return target_;}
    std::uint32_t inserted() const noexcept {return inserted_;}
    bool mechanism_pending() const noexcept {return pending_mechanism_;}
    MechanismAction pending_action() const noexcept {return pending_action_;}
    bool feed_present() const noexcept {return feed_present_;}
    bool can_fire() const noexcept {
        return weapon_&&(stage_==ReloadStage::idle||stage_==ReloadStage::complete)&&
            feed_present_&&!pending_mechanism_&&loaded_>0;
    }
    std::uint32_t total() const noexcept {return loaded_+reserve_+held_;}
};
}
