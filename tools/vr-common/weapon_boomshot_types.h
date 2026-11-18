#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include "weapon_reload_transaction.h"

namespace gears_weapon {

// Saved evidence: analysis/boomshot-development-20261109/animation-audit.json, MP_WarMachine.war
// SHA256 55f40ca44051413fe6db19816f97b09849c5e104319d93e9d5d159cbe6fb2840.
// AR_Fire_Boomshot and AR_Reload translate Boomshot_Barrel. The magazine track
// has one key. These facts do not identify a hand action, opening/closing order,
// load quantity, firing cost, hand contact or usable perfect-reload deadline.
// No such defaults, Lancer mounts or replacement projectile/recoil are supplied.
inline constexpr std::int64_t boomshot_perfect_duration_us=4000000;
inline constexpr char boomshot_original_base_class[]="Weap_Boomer_base";
inline constexpr char boomshot_original_barrel_bone[]="Boomshot_Barrel";
inline constexpr char boomshot_original_magazine_bone[]="Boomshot_Magazine";

enum class BoomshotPart : std::uint8_t {
    none=0,barrel=1,magazine=2,feed_port=4,left_shoulder=8
};
enum class BoomshotAmmoKind : std::uint8_t { unknown,projectile,verified_carrier };
enum class BoomshotAmmoSource : std::uint8_t { other,left_shoulder };
enum class BoomshotStepKind : std::uint8_t { move_mechanism,insert_ammunition };
enum class BoomshotCondition : std::uint8_t { empty,partial,after_shot,inspection };
enum class BoomshotOutcome : std::uint8_t { none,perfect,ordinary };
enum class BoomshotOperation : std::uint8_t { none,reload,shot_cycle,inspection };
enum class BoomshotStage : std::uint8_t {
    unbound,ready,mechanism,take_ammunition,holding_ammunition,inventory_commit,
    completion_commit,fire_handoff,interrupted
};
enum class BoomshotEvent : std::uint8_t {
    none,rejected,invalid,full,no_ammo,started,grabbed,released,step_completed,
    request_inventory,inventory_committed,request_completion,reload_completed,
    cycle_completed,request_fire,shot_recorded,motion_reset,animation_handoff,
    restored,interrupted,inspection_completed
};
struct BoomshotIdentity {
    std::uint64_t world{},pawn{},weapon{},generation{};
    constexpr bool valid() const noexcept {return world&&pawn&&weapon&&generation;}
    constexpr bool operator==(BoomshotIdentity const& b) const noexcept {
        return world==b.world&&pawn==b.pawn&&weapon==b.weapon&&generation==b.generation;
    }
};
struct BoomshotStamp {
    BoomshotIdentity identity{};
    std::uint64_t binding{},sequence{},motion_epoch{};
    std::int64_t game_tick{},monotonic_ns{}; // game_tick is original WorldInfo time in microseconds.
    constexpr bool valid() const noexcept {
        return identity.valid()&&binding&&sequence&&motion_epoch&&game_tick>=0&&monotonic_ns>0;
    }
};
struct BoomshotInventory {
    BoomshotIdentity identity{};
    std::uint64_t revision{};
    std::uint32_t loaded{},reserve{};
    bool infinite_reserve{};
};
struct BoomshotStep {
    BoomshotStepKind kind{BoomshotStepKind::move_mechanism};
    BoomshotPart part{BoomshotPart::none};
    std::uint64_t phase_id{},geometry_id{};
    std::uint32_t units{};
    std::int64_t minimum_motion_ns{};
    bool return_required{};
};
// W07 fixes open -> insert one carrier -> close. Actual travel, direction and
// phase identity still require current-instance calibration. The after-shot
// plan has zero manual steps: the next opening belongs to the next reload.
struct BoomshotPlan {
    BoomshotIdentity identity{};
    std::uint64_t config_id{},plan_id{};
    BoomshotCondition condition{BoomshotCondition::empty};
    std::array<BoomshotStep,12> steps{};
    std::uint8_t count{};
    bool current_instance_verified{},original_order_verified{},quantity_verified{};
    // The current product path exchanges a spent drum with one replacement.
    // The original barrel-open/close path remains a separate inspection plan.
    bool direct_exchange{};
    constexpr bool valid(BoomshotIdentity const& id,std::uint64_t config,
               BoomshotCondition expected,std::uint32_t& units) const noexcept {
        units=0;
        if(!(identity==id)||config_id!=config||!plan_id||condition!=expected||
           !current_instance_verified||!original_order_verified||!quantity_verified||
           count>steps.size())return false;
        if(expected==BoomshotCondition::after_shot)return count==0&&!direct_exchange;
        const bool inspection=expected==BoomshotCondition::inspection;
        if(inspection){
            if(direct_exchange)return false;
            if(count!=2||steps[0].kind!=BoomshotStepKind::move_mechanism||steps[0].part!=BoomshotPart::barrel||
               steps[1].kind!=BoomshotStepKind::move_mechanism||steps[1].part!=BoomshotPart::barrel)return false;
        }else if(direct_exchange){
            if(expected!=BoomshotCondition::empty||count!=1||
               steps[0].kind!=BoomshotStepKind::insert_ammunition||steps[0].part!=BoomshotPart::feed_port||
               steps[0].units!=1||steps[0].return_required)return false;
        }else if(expected!=BoomshotCondition::empty||count!=3||
           steps[0].kind!=BoomshotStepKind::move_mechanism||steps[0].part!=BoomshotPart::barrel||
           steps[1].kind!=BoomshotStepKind::insert_ammunition||steps[1].units!=1||
           steps[2].kind!=BoomshotStepKind::move_mechanism||steps[2].part!=BoomshotPart::barrel)return false;
        unsigned inserts=0;
        for(unsigned i=0;i<count;++i){
            auto const& step=steps[i];
            if(!step.phase_id||!step.geometry_id||step.minimum_motion_ns<0)return false;
            for(unsigned j=0;j<i;++j)if(steps[j].phase_id==step.phase_id)return false;
            switch(step.kind){
                case BoomshotStepKind::move_mechanism:
                    if(step.units||(step.part!=BoomshotPart::barrel&&
                                    step.part!=BoomshotPart::magazine))return false;
                    break;
                case BoomshotStepKind::insert_ammunition:
                    if(expected==BoomshotCondition::after_shot||step.part!=BoomshotPart::feed_port||
                       !step.units||step.units>10000||step.return_required||
                       units>10000-step.units)return false;
                    units+=step.units;++inserts;break;
                default:return false;
            }
        }
        return inspection?(inserts==0&&units==0):(inserts==1&&units==1);
    }
};
// Zero-target inspection certifies only the actual original one-round mechanism.
// It conveys no projectile, ammo-item, insertion or reward authority.
struct BoomshotInspectionConfig {
    BoomshotIdentity identity{};std::uint64_t config_id{};std::uint32_t capacity{};
    std::int64_t maximum_sample_gap_ns{};bool current_instance_verified{},original_class_verified{};
    constexpr bool valid() const noexcept {
        return identity.valid()&&config_id&&capacity==1&&maximum_sample_gap_ns>0&&
            maximum_sample_gap_ns<=100000000&&current_instance_verified&&original_class_verified;
    }
};
// Normal mechanical reload certificate. It grants neither BeginFire nor any
// projectile/recoil receipt. The original synchronous fire consumer owns those.
struct BoomshotReloadConfig {
    BoomshotIdentity identity{};std::uint64_t config_id{},ammunition_type{};
    BoomshotAmmoKind ammunition_kind{BoomshotAmmoKind::unknown};std::uint32_t capacity{};
    std::int64_t maximum_sample_gap_ns{};bool current_instance_verified{},original_class_verified{};
    constexpr bool valid() const noexcept {
        return identity.valid()&&config_id&&ammunition_type&&ammunition_kind==BoomshotAmmoKind::verified_carrier&&capacity==1&&
            maximum_sample_gap_ns>0&&maximum_sample_gap_ns<=250000000&&current_instance_verified&&original_class_verified;
    }
};
struct BoomshotInstanceConfig {
    BoomshotIdentity identity{};
    std::uint64_t config_id{},ammunition_type{};
    BoomshotAmmoKind ammunition_kind{BoomshotAmmoKind::unknown};
    std::uint32_t capacity{},original_ammo_cost{},original_projectile_count{};
    std::int64_t maximum_sample_gap_ns{};
    BoomshotPlan after_shot{};
    bool current_instance_verified{},original_class_verified{},original_fire_semantics_verified{};
    constexpr BoomshotReloadConfig reload() const noexcept {
        return {identity,config_id,ammunition_type,ammunition_kind,capacity,maximum_sample_gap_ns,current_instance_verified,original_class_verified};
    }
    constexpr BoomshotInspectionConfig inspection() const noexcept {
        return {identity,config_id,capacity,maximum_sample_gap_ns,current_instance_verified,original_class_verified};
    }
    constexpr bool valid() const noexcept {
        std::uint32_t units{};
        return identity.valid()&&config_id&&ammunition_type&&
            ammunition_kind==BoomshotAmmoKind::verified_carrier&&capacity==1&&
            original_ammo_cost==1&&original_projectile_count&&
            original_projectile_count<=10000&&maximum_sample_gap_ns>0&&maximum_sample_gap_ns<=100000000&&
            current_instance_verified&&original_class_verified&&original_fire_semantics_verified&&
            after_shot.valid(identity,config_id,BoomshotCondition::after_shot,units);
    }
};
struct BoomshotFrame {
    BoomshotStamp stamp{};
    BoomshotInventory observed{};
    bool left_tracked{},right_tracked{},right_hand_equipped{},original_interaction_allowed{},
         original_can_fire{},original_owns_hands{};
};
struct BoomshotPerfectWindow {
    BoomshotIdentity identity{};
    std::uint64_t config_id{},original_reload_serial{};
    std::int64_t end_ticks{}; // Legacy evidence only; never controls the four-second product rule.
    bool current_instance_verified{}; // Unknown permits ordinary completion.
};
struct BoomshotReloadStart {
    std::uint64_t binding{},r3_press_edge{},original_reload_serial{};
    BoomshotPlan plan{};
    BoomshotPerfectWindow perfect{};
    bool original_reload_accepted{},automatic_completion_suppressed{};
};
// Gun-only zero-target operation. No ForceReload, native reload serial,
// insertion or reward is admitted by this separate certificate.
struct BoomshotInspectionStart {
    std::uint64_t binding{},animation_serial{};
    BoomshotPlan plan{};
    bool original_gun_animation_owned{},native_reload_absent{};
};
struct BoomshotInspectionExit {
    BoomshotIdentity identity{};
    std::uint64_t binding{},token{},animation_serial{};
};
struct BoomshotAmmoReceipt {
    std::uint64_t binding{},original_reload_serial{},plan_id{},phase_id{},item_serial{},ammunition_type{};
    std::uint32_t units{};
    BoomshotAmmoKind kind{BoomshotAmmoKind::unknown};
    BoomshotAmmoSource source{BoomshotAmmoSource::other};
    bool left_grip_held{};
};
// Geometry is certified by the live adapter using this gun's measured mount,
// hand poses and continuous stroke, never by elapsed animation time alone.
struct BoomshotMotionReceipt {
    std::uint64_t binding{},operation_serial{},plan_id{},phase_id{},geometry_id{},
                  motion_epoch{},begin_sequence{};
    std::int64_t begin_ns{};
    bool player_driven{},continuous{},endpoints_and_direction_verified{},return_verified{},left_grip_held{};
};
// Current core expectation only; carries no input, geometry or Native grant.
struct BoomshotMechanismState {
    BoomshotIdentity identity{};
    std::uint64_t binding{},operation_serial{},plan_id{},phase_id{},geometry_id{};
    std::uint8_t step{};bool return_required{};
};
struct BoomshotInsertionReceipt {
    BoomshotMotionReceipt motion{};
    std::uint64_t item_serial{};
    bool approached_from_outside{},aligned{},seated{};
};
struct BoomshotInventoryRequest {
    std::uint64_t binding{},token{},original_reload_serial{},plan_id{},phase_id{},item_serial{};
    BoomshotInventory before{};
    std::uint32_t after_loaded{},after_reserve{};
    std::int64_t physical_finish_tick{};
};
struct BoomshotCompletionRequest {
    std::uint64_t binding{},token{},operation_serial{},plan_id{};
    BoomshotInventory inventory{};
    BoomshotOperation operation{BoomshotOperation::none};
    BoomshotOutcome outcome{BoomshotOutcome::none};
    std::int64_t started_tick{},physical_finish_tick{};
};
struct BoomshotFireRequest {
    std::uint64_t binding{},token{},trigger_press_edge{};
    BoomshotInventory before{};
};
struct BoomshotOriginalReceipt {
    BoomshotStamp stamp{};
    std::uint64_t request_token{},original_event_sequence{};
    BoomshotInventory observed{};
    bool accepted{},original_effect_applied_once{};
    ReloadTxnResult result_applied{ReloadTxnResult::none};
    bool original_ready{}; // Completion owner confirms original weapon readiness.
};
struct BoomshotShotReceipt {
    BoomshotOriginalReceipt original{};
    std::uint32_t original_projectiles_spawned{};
    bool original_recoil_applied{};
};
struct BoomshotGuide {
    BoomshotPart part{BoomshotPart::none};
    float red{1},green{1},blue{0},alpha{1};
};

}
