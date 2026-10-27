#pragma once
#include <array>
#include <cstdint>
#include <limits>

namespace gears_weapon {

// Saved evidence: analysis/boomshot-animation-tracks.json, MP_WarMachine.war
// SHA256 55f40ca44051413fe6db19816f97b09849c5e104319d93e9d5d159cbe6fb2840.
// AR_Fire_Boomshot and AR_Reload translate Boomshot_Barrel. The magazine track
// has one key. These facts do not identify a hand action, opening/closing order,
// load quantity, firing cost, hand contact or usable perfect-reload deadline.
// No such defaults, Lancer mounts or replacement projectile/recoil are supplied.
inline constexpr char boomshot_original_base_class[]="Weap_Boomer_base";
inline constexpr char boomshot_original_barrel_bone[]="Boomshot_Barrel";
inline constexpr char boomshot_original_magazine_bone[]="Boomshot_Magazine";

enum class BoomshotPart : std::uint8_t {
    none=0,barrel=1,magazine=2,feed_port=4,left_shoulder=8
};
enum class BoomshotAmmoKind : std::uint8_t { unknown,projectile,verified_carrier };
enum class BoomshotAmmoSource : std::uint8_t { other,left_shoulder };
enum class BoomshotStepKind : std::uint8_t { move_mechanism,insert_ammunition };
enum class BoomshotCondition : std::uint8_t { empty,partial,after_shot };
enum class BoomshotOutcome : std::uint8_t { none,perfect,ordinary };
enum class BoomshotOperation : std::uint8_t { none,reload,shot_cycle };
enum class BoomshotStage : std::uint8_t {
    unbound,ready,mechanism,take_ammunition,holding_ammunition,inventory_commit,
    completion_commit,fire_handoff,interrupted
};
enum class BoomshotEvent : std::uint8_t {
    none,rejected,invalid,full,no_ammo,started,grabbed,released,step_completed,
    request_inventory,inventory_committed,request_completion,reload_completed,
    cycle_completed,request_fire,shot_recorded,motion_reset,animation_handoff,
    restored,interrupted
};
struct BoomshotIdentity {
    std::uint64_t world{},pawn{},weapon{},generation{};
    bool valid() const noexcept {return world&&pawn&&weapon&&generation;}
    bool operator==(BoomshotIdentity const& b) const noexcept {
        return world==b.world&&pawn==b.pawn&&weapon==b.weapon&&generation==b.generation;
    }
};
struct BoomshotStamp {
    BoomshotIdentity identity{};
    std::uint64_t binding{},sequence{},motion_epoch{};
    std::int64_t game_tick{},monotonic_ns{};
    bool valid() const noexcept {
        return identity.valid()&&binding&&sequence&&motion_epoch&&game_tick>=0&&monotonic_ns>0;
    }
};
struct BoomshotInventory {
    BoomshotIdentity identity{};
    std::uint64_t revision{};
    std::uint32_t loaded{},reserve{};
};
struct BoomshotStep {
    BoomshotStepKind kind{BoomshotStepKind::move_mechanism};
    BoomshotPart part{BoomshotPart::none};
    std::uint64_t phase_id{},geometry_id{};
    std::uint32_t units{};
    std::int64_t minimum_motion_ns{};
    bool return_required{};
};
// Exact current-instance sequence, not a generic open/load/close template.
// An insert-only plan and a separately verified after-shot plan are legal.
// Quantities include every physical item; a named bone is not a manual phase.
struct BoomshotPlan {
    BoomshotIdentity identity{};
    std::uint64_t config_id{},plan_id{};
    BoomshotCondition condition{BoomshotCondition::empty};
    std::array<BoomshotStep,12> steps{};
    std::uint8_t count{};
    bool current_instance_verified{},original_order_verified{},quantity_verified{};
    bool valid(BoomshotIdentity const& id,std::uint64_t config,
               BoomshotCondition expected,std::uint32_t& units) const noexcept {
        units=0;
        if(!(identity==id)||config_id!=config||!plan_id||condition!=expected||
           !current_instance_verified||!original_order_verified||!quantity_verified||
           count>steps.size())return false;
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
        return expected==BoomshotCondition::after_shot?!inserts:(inserts>0);
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
    bool valid() const noexcept {
        std::uint32_t units{};
        return identity.valid()&&config_id&&ammunition_type&&
            (ammunition_kind==BoomshotAmmoKind::projectile||
             ammunition_kind==BoomshotAmmoKind::verified_carrier)&&capacity&&capacity<=10000&&
            original_ammo_cost&&original_ammo_cost<=capacity&&original_projectile_count&&
            original_projectile_count<=10000&&maximum_sample_gap_ns>0&&
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
    std::int64_t end_ticks{}; // Relative to accepted R3 on the original game clock.
    bool current_instance_verified{}; // Unknown permits ordinary completion.
};
struct BoomshotReloadStart {
    std::uint64_t binding{},r3_press_edge{},original_reload_serial{};
    BoomshotPlan plan{};
    BoomshotPerfectWindow perfect{};
    bool original_reload_accepted{},automatic_completion_suppressed{};
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

// Pure decision layer. Only original receipts change the inventory mirror.
// Taking ammo reserves it locally; insertion proposes exactly one original
// transfer. Fire requests preserve the original input, projectiles and recoil.
// The adapter owns native timer suppression, idempotent execution by token,
// geometry, audio/rewards, yellow rendering and authoritative reconciliation.
// Process mutation receipts before ordinary inventory observation in that tick.
// One game-thread owner holds this noncopyable transaction for its lifetime.
class BoomshotInteraction {
    BoomshotInstanceConfig config_{};
    BoomshotInventory inventory_{};
    BoomshotPlan plan_{};
    BoomshotPerfectWindow perfect_{};
    BoomshotStage stage_{BoomshotStage::unbound};
    BoomshotOperation operation_{BoomshotOperation::none};
    BoomshotInventoryRequest inventory_request_{};
    BoomshotCompletionRequest completion_request_{};
    BoomshotFireRequest fire_request_{};
    std::uint64_t next_token_{1},binding_{},sequence_{},motion_epoch_{},motion_floor_{},
                  last_r3_{},reload_serial_{},operation_serial_{},last_trigger_{},
                  last_original_event_{},held_item_{},last_item_serial_{};
    std::int64_t tick_{},ns_{},motion_floor_ns_{},started_tick_{};
    std::uint32_t held_units_{};
    std::uint8_t cursor_{};
    bool tracked_{},original_can_fire_{},animation_owns_hands_{},motion_broken_{true};

    BoomshotInteraction(BoomshotInteraction const&)=default;
    BoomshotInteraction& operator=(BoomshotInteraction const&)=default;

    std::uint64_t token() noexcept {
        if(next_token_==(std::numeric_limits<std::uint64_t>::max)())return 0;
        return next_token_++;
    }
    bool same_inventory(BoomshotInventory const& other,bool same_revision=true) const noexcept {
        return other.identity==inventory_.identity&&other.loaded==inventory_.loaded&&
            other.reserve==inventory_.reserve&&
            (same_revision?other.revision==inventory_.revision:other.revision>=inventory_.revision);
    }
    void break_motion() noexcept {
        tracked_=false;motion_broken_=true;motion_floor_=sequence_;motion_floor_ns_=ns_;
    }
    BoomshotEvent invalidate() noexcept {
        stage_=BoomshotStage::interrupted;held_item_=0;held_units_=0;break_motion();
        inventory_request_={};completion_request_={};fire_request_={};
        return BoomshotEvent::interrupted;
    }
    bool valid_stamp(BoomshotStamp const& s) const noexcept {
        return s.valid()&&s.identity==config_.identity&&s.binding==binding_&&
            s.sequence>sequence_&&s.game_tick>=tick_&&s.monotonic_ns>ns_;
    }
    void consume(BoomshotStamp const& s) noexcept {
        sequence_=s.sequence;tick_=s.game_tick;ns_=s.monotonic_ns;
    }
    bool frame(BoomshotFrame const& f,BoomshotEvent& event) noexcept {
        event=BoomshotEvent::rejected;
        if(stage_==BoomshotStage::unbound||stage_==BoomshotStage::interrupted)return false;
        if(!valid_stamp(f.stamp)){
            if(f.stamp.binding==binding_)break_motion();
            return false;
        }
        const auto gap=ns_?f.stamp.monotonic_ns-ns_:0;
        const bool new_epoch=motion_epoch_&&f.stamp.motion_epoch!=motion_epoch_;
        consume(f.stamp);motion_epoch_=f.stamp.motion_epoch;
        if(!same_inventory(f.observed)||!f.right_hand_equipped){
            event=invalidate();return false;
        }
        original_can_fire_=f.original_can_fire;
        if(f.original_owns_hands){
            event=animation_owns_hands_?BoomshotEvent::none:BoomshotEvent::animation_handoff;
            animation_owns_hands_=true;break_motion();return false;
        }
        if(!f.original_interaction_allowed){break_motion();event=BoomshotEvent::none;return false;}
        if(!f.left_tracked||!f.right_tracked){
            break_motion();event=BoomshotEvent::motion_reset;return false;
        }
        if(animation_owns_hands_){
            animation_owns_hands_=false;motion_broken_=true;event=BoomshotEvent::restored;
        }
        if(motion_broken_||new_epoch||gap>config_.maximum_sample_gap_ns){
            motion_floor_=sequence_;motion_floor_ns_=ns_;tracked_=true;motion_broken_=false;
            if(event!=BoomshotEvent::restored)event=BoomshotEvent::motion_reset;
            return false;
        }
        tracked_=true;event=BoomshotEvent::none;return true;
    }
    bool receipt(BoomshotOriginalReceipt const& r,std::uint64_t request) noexcept {
        if(stage_==BoomshotStage::interrupted||!request||r.request_token!=request||
           !valid_stamp(r.stamp)||!r.original_event_sequence||
           r.original_event_sequence<=last_original_event_)return false;
        consume(r.stamp);
        if(r.stamp.motion_epoch!=motion_epoch_){motion_epoch_=r.stamp.motion_epoch;break_motion();}
        last_original_event_=r.original_event_sequence;return true;
    }
    bool motion(BoomshotMotionReceipt const& r,BoomshotFrame const& f) const noexcept {
        auto const& step=plan_.steps[cursor_];
        return r.binding==binding_&&r.operation_serial==operation_serial_&&r.plan_id==plan_.plan_id&&
            r.phase_id==step.phase_id&&r.geometry_id==step.geometry_id&&r.motion_epoch==motion_epoch_&&
            r.begin_sequence>=motion_floor_&&r.begin_sequence<f.stamp.sequence&&
            r.begin_ns>=motion_floor_ns_&&r.begin_ns<f.stamp.monotonic_ns&&
            f.stamp.monotonic_ns-r.begin_ns>=step.minimum_motion_ns&&r.player_driven&&
            r.continuous&&r.endpoints_and_direction_verified&&r.left_grip_held&&
            (!step.return_required||r.return_verified);
    }
    BoomshotEvent advance(std::int64_t physical_tick) noexcept {
        motion_floor_=sequence_;motion_floor_ns_=ns_;
        if(++cursor_<plan_.count){
            stage_=plan_.steps[cursor_].kind==BoomshotStepKind::insert_ammunition?
                BoomshotStage::take_ammunition:BoomshotStage::mechanism;
            return BoomshotEvent::step_completed;
        }
        const auto nonce=token();if(!nonce)return invalidate();
        const auto outcome=operation_==BoomshotOperation::reload?
            (perfect_.current_instance_verified&&physical_tick>=started_tick_&&
             physical_tick-started_tick_<=perfect_.end_ticks?
                BoomshotOutcome::perfect:BoomshotOutcome::ordinary):BoomshotOutcome::none;
        completion_request_={binding_,nonce,operation_serial_,plan_.plan_id,inventory_,operation_,
            outcome,started_tick_,physical_tick};
        stage_=BoomshotStage::completion_commit;return BoomshotEvent::request_completion;
    }
public:
    BoomshotInteraction()=default;
    // Rebind only after fresh native inventory/mechanical reconciliation. Nonces
    // and visual item serials survive reset; old same-address receipts cannot bind.
    void reset() noexcept {
        const auto next=next_token_,item=last_item_serial_,r3=last_r3_,trigger=last_trigger_;
        *this=BoomshotInteraction{};next_token_=next;last_item_serial_=item;
        last_r3_=r3;last_trigger_=trigger;
    }
    bool bind(BoomshotInstanceConfig const& config,BoomshotInventory const& observed) noexcept {
        if(stage_!=BoomshotStage::unbound||!config.valid()||!(observed.identity==config.identity)||
           !observed.revision||observed.loaded>config.capacity||observed.reserve>1000000)return false;
        const auto nonce=token();if(!nonce)return false;
        config_=config;inventory_=observed;binding_=nonce;stage_=BoomshotStage::ready;return true;
    }
    BoomshotEvent observe(BoomshotFrame const& f) noexcept {
        BoomshotEvent event{};frame(f,event);return event;
    }
    void tracking_lost() noexcept {break_motion();}
    BoomshotEvent interrupt() noexcept {
        if(stage_==BoomshotStage::unbound||stage_==BoomshotStage::interrupted)
            return BoomshotEvent::rejected;
        // No native debit occurred for held ammo, so no native refund is issued.
        // Preserve the last confirmed inventory and cursor for reconciliation.
        return invalidate();
    }
    BoomshotEvent begin_r3(BoomshotFrame const& f,BoomshotReloadStart const& start) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::ready||!frame(f,event))return event;
        if(start.binding!=binding_||!start.r3_press_edge||start.r3_press_edge<=last_r3_||
           !start.original_reload_serial||start.original_reload_serial<=reload_serial_||
           !start.original_reload_accepted||!start.automatic_completion_suppressed)
            return BoomshotEvent::rejected;
        std::uint32_t units{};
        if(!start.plan.valid(config_.identity,config_.config_id,inventory_.loaded?
            BoomshotCondition::partial:BoomshotCondition::empty,units)||
           start.perfect.end_ticks<0||(start.perfect.current_instance_verified&&
            (!(start.perfect.identity==config_.identity)||start.perfect.config_id!=config_.config_id||
             start.perfect.original_reload_serial!=start.original_reload_serial)))return BoomshotEvent::invalid;
        last_r3_=start.r3_press_edge;
        if(inventory_.loaded==config_.capacity)return BoomshotEvent::full;
        if(!inventory_.reserve||units>inventory_.reserve)return BoomshotEvent::no_ammo;
        if(units>config_.capacity-inventory_.loaded)return BoomshotEvent::invalid;
        plan_=start.plan;perfect_=start.perfect;reload_serial_=start.original_reload_serial;
        operation_serial_=reload_serial_;operation_=BoomshotOperation::reload;
        cursor_=0;started_tick_=tick_;motion_floor_=sequence_;motion_floor_ns_=ns_;
        stage_=plan_.steps[0].kind==BoomshotStepKind::insert_ammunition?
            BoomshotStage::take_ammunition:BoomshotStage::mechanism;
        return BoomshotEvent::started;
    }
    BoomshotEvent grab_left_shoulder(BoomshotFrame const& f,BoomshotAmmoReceipt const& r) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::take_ammunition||!frame(f,event))return event;
        auto const& step=plan_.steps[cursor_];
        if(r.binding!=binding_||r.original_reload_serial!=reload_serial_||r.plan_id!=plan_.plan_id||
           r.phase_id!=step.phase_id||!r.item_serial||r.item_serial<=last_item_serial_||
           r.ammunition_type!=config_.ammunition_type||r.kind!=config_.ammunition_kind||
           r.source!=BoomshotAmmoSource::left_shoulder||!r.left_grip_held||
           r.units!=step.units||r.units>inventory_.reserve)return BoomshotEvent::rejected;
        held_item_=last_item_serial_=r.item_serial;held_units_=r.units;
        motion_floor_=sequence_;motion_floor_ns_=ns_;stage_=BoomshotStage::holding_ammunition;
        return BoomshotEvent::grabbed;
    }
    BoomshotEvent release_ammunition(BoomshotFrame const& f,std::uint64_t item) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::holding_ammunition||!frame(f,event))return event;
        if(!item||item!=held_item_)return BoomshotEvent::rejected;
        held_item_=0;held_units_=0;motion_floor_=sequence_;motion_floor_ns_=ns_;
        stage_=BoomshotStage::take_ammunition;return BoomshotEvent::released;
    }
    BoomshotEvent complete_mechanism(BoomshotFrame const& f,BoomshotMotionReceipt const& r) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::mechanism||!frame(f,event))return event;
        if(!motion(r,f))return BoomshotEvent::rejected;
        return advance(tick_);
    }
    BoomshotEvent insert_ammunition(BoomshotFrame const& f,BoomshotInsertionReceipt const& r) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::holding_ammunition||!frame(f,event))return event;
        if(r.item_serial!=held_item_||!r.approached_from_outside||!r.aligned||!r.seated||
           !motion(r.motion,f))return BoomshotEvent::rejected;
        if(held_units_>inventory_.reserve||held_units_>config_.capacity-inventory_.loaded)
            return invalidate();
        const auto nonce=token();if(!nonce)return invalidate();
        inventory_request_={binding_,nonce,reload_serial_,plan_.plan_id,plan_.steps[cursor_].phase_id,
            held_item_,inventory_,inventory_.loaded+held_units_,inventory_.reserve-held_units_,tick_};
        stage_=BoomshotStage::inventory_commit;return BoomshotEvent::request_inventory;
    }
    BoomshotEvent acknowledge_inventory(BoomshotOriginalReceipt const& r) noexcept {
        if(stage_!=BoomshotStage::inventory_commit||!receipt(r,inventory_request_.token))
            return BoomshotEvent::rejected;
        if(!r.accepted){
            if(!same_inventory(r.observed,false))return invalidate();
            inventory_=r.observed;inventory_request_={};motion_floor_=sequence_;motion_floor_ns_=ns_;
            stage_=BoomshotStage::holding_ammunition;return BoomshotEvent::rejected;
        }
        if(!r.original_effect_applied_once||!(r.observed.identity==config_.identity)||
           r.observed.revision<=inventory_request_.before.revision||
           r.observed.loaded!=inventory_request_.after_loaded||
           r.observed.reserve!=inventory_request_.after_reserve)return invalidate();
        const auto physical_tick=inventory_request_.physical_finish_tick;
        inventory_=r.observed;inventory_request_={};held_item_=0;held_units_=0;
        const auto advanced=advance(physical_tick);
        return advanced==BoomshotEvent::step_completed?BoomshotEvent::inventory_committed:advanced;
    }
    BoomshotEvent acknowledge_completion(BoomshotOriginalReceipt const& r) noexcept {
        if(stage_!=BoomshotStage::completion_commit||!receipt(r,completion_request_.token))
            return BoomshotEvent::rejected;
        if(!r.accepted||!r.original_effect_applied_once||!same_inventory(r.observed,false))return invalidate();
        inventory_=r.observed;const auto operation=operation_;completion_request_={};
        operation_=BoomshotOperation::none;stage_=BoomshotStage::ready;break_motion();
        return operation==BoomshotOperation::reload?BoomshotEvent::reload_completed:BoomshotEvent::cycle_completed;
    }
    BoomshotEvent request_original_fire(BoomshotFrame const& f,std::uint64_t trigger_edge) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::ready||!frame(f,event))return event;
        if(!original_can_fire_||!trigger_edge||trigger_edge<=last_trigger_)return BoomshotEvent::rejected;
        last_trigger_=trigger_edge;
        if(inventory_.loaded<config_.original_ammo_cost)return BoomshotEvent::no_ammo;
        const auto nonce=token();if(!nonce)return invalidate();
        fire_request_={binding_,nonce,trigger_edge,inventory_};stage_=BoomshotStage::fire_handoff;
        return BoomshotEvent::request_fire;
    }
    BoomshotEvent after_original_shot(BoomshotShotReceipt const& shot) noexcept {
        auto const& r=shot.original;
        if(stage_!=BoomshotStage::fire_handoff||!receipt(r,fire_request_.token))return BoomshotEvent::rejected;
        if(!r.accepted){
            if(!same_inventory(r.observed,false))return invalidate();
            inventory_=r.observed;fire_request_={};stage_=BoomshotStage::ready;break_motion();
            return BoomshotEvent::rejected;
        }
        if(!r.original_effect_applied_once||!shot.original_recoil_applied||
           shot.original_projectiles_spawned!=config_.original_projectile_count||
           !(r.observed.identity==config_.identity)||r.observed.revision<=inventory_.revision||
           r.observed.loaded!=inventory_.loaded-config_.original_ammo_cost||
           r.observed.reserve!=inventory_.reserve)return invalidate();
        inventory_=r.observed;fire_request_={};original_can_fire_=false;break_motion();
        plan_=config_.after_shot;operation_serial_=r.original_event_sequence;
        operation_=BoomshotOperation::shot_cycle;cursor_=0;started_tick_=tick_;
        if(plan_.count)stage_=BoomshotStage::mechanism;
        else{operation_=BoomshotOperation::none;stage_=BoomshotStage::ready;}
        return BoomshotEvent::shot_recorded;
    }
    std::uint64_t binding_token() const noexcept {return binding_;}
    BoomshotStage stage() const noexcept {return stage_;}
    std::uint8_t step_index() const noexcept {return cursor_;}
    BoomshotInventory inventory() const noexcept {return inventory_;}
    std::uint32_t held_units() const noexcept {return held_units_;}
    std::uint32_t available_reserve() const noexcept {return inventory_.reserve-held_units_;}
    bool animation_owns_hands() const noexcept {return animation_owns_hands_;}
    bool needs_reconciliation() const noexcept {return stage_==BoomshotStage::interrupted;}
    bool can_request_fire() const noexcept {
        return stage_==BoomshotStage::ready&&tracked_&&!animation_owns_hands_&&original_can_fire_&&
            inventory_.loaded>=config_.original_ammo_cost;
    }
    BoomshotInventoryRequest inventory_request() const noexcept {return inventory_request_;}
    BoomshotCompletionRequest completion_request() const noexcept {return completion_request_;}
    BoomshotFireRequest fire_request() const noexcept {return fire_request_;}
    BoomshotGuide guide() const noexcept {
        if(stage_==BoomshotStage::take_ammunition)return {BoomshotPart::left_shoulder};
        if(stage_==BoomshotStage::holding_ammunition)return {BoomshotPart::feed_port};
        if(stage_==BoomshotStage::mechanism)return {plan_.steps[cursor_].part};
        return {};
    }
};
}
