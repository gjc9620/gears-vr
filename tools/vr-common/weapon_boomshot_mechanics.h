#pragma once
#include "weapon_boomshot_reload_accounting.h"
namespace gears_weapon {
// Pure decision layer. Only original receipts change the inventory mirror.
// Taking ammo reserves it locally; insertion proposes exactly one original
// transfer. Fire requests preserve the original input, projectiles and recoil.
// The adapter owns native timer suppression, idempotent execution by token,
// geometry, audio/rewards, yellow rendering and authoritative reconciliation.
// Process mutation receipts before ordinary inventory observation in that tick.
// One game-thread owner holds this noncopyable transaction for its lifetime.
class BoomshotInteraction {
    BoomshotInstanceConfig config_{};
    BoomshotInspectionConfig inspection_config_{};
    BoomshotReloadAccounting accounting_{};
    BoomshotPlan plan_{};
    BoomshotStage stage_{BoomshotStage::unbound};
    BoomshotOperation operation_{BoomshotOperation::none};
    BoomshotFireRequest fire_request_{};
    BoomshotInspectionExit inspection_exit_{};
    std::uint64_t next_token_{1},binding_{},sequence_{},motion_epoch_{},motion_floor_{},
                  last_reload_intent_{},reload_serial_{},operation_serial_{},last_trigger_{},
                  last_original_event_{},held_item_{},last_inspection_serial_{},direct_seat_token_{};
    std::int64_t tick_{},ns_{},motion_floor_ns_{},started_tick_{};
    std::int64_t closed_tick_{};
    std::uint8_t cursor_{};
    bool tracked_{},original_can_fire_{},animation_owns_hands_{},motion_broken_{true};
    bool fire_dispatched_{},inspection_exit_dispatched_{};
    bool inspection_only_{},reload_only_{};

    BoomshotInteraction(BoomshotInteraction const&)=default;
    BoomshotInteraction& operator=(BoomshotInteraction const&)=default;

    constexpr std::uint64_t token() noexcept {
        if(next_token_==(std::numeric_limits<std::uint64_t>::max)())return 0;
        return next_token_++;
    }
    constexpr bool same_inventory(BoomshotInventory const& other,bool same_revision=true) const noexcept {
        return other.identity==accounting_.inventory().identity&&other.loaded==accounting_.inventory().loaded&&
            other.reserve==accounting_.inventory().reserve&&other.infinite_reserve==accounting_.inventory().infinite_reserve&&
            (same_revision?other.revision==accounting_.inventory().revision:other.revision>=accounting_.inventory().revision);
    }
    constexpr void break_motion() noexcept {
        tracked_=false;motion_broken_=true;motion_floor_=sequence_;motion_floor_ns_=ns_;
    }
    constexpr BoomshotEvent invalidate() noexcept {
        stage_=BoomshotStage::interrupted;held_item_=0;break_motion();
        accounting_.interrupt();
        if(!fire_dispatched_)fire_request_={};
        return BoomshotEvent::interrupted;
    }
    constexpr bool valid_stamp(BoomshotStamp const& s) const noexcept {
        return s.valid()&&s.identity==config_.identity&&s.binding==binding_&&
            s.sequence>sequence_&&s.game_tick>=tick_&&s.monotonic_ns>ns_;
    }
    constexpr void consume(BoomshotStamp const& s) noexcept {
        sequence_=s.sequence;tick_=s.game_tick;ns_=s.monotonic_ns;
    }
    constexpr bool frame(BoomshotFrame const& f,BoomshotEvent& event) noexcept {
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
    constexpr bool receipt(BoomshotOriginalReceipt const& r,std::uint64_t request) noexcept {
        if(!request||r.request_token!=request||
           !valid_stamp(r.stamp)||!r.original_event_sequence||
           r.original_event_sequence<=last_original_event_)return false;
        consume(r.stamp);
        if(r.stamp.motion_epoch!=motion_epoch_){motion_epoch_=r.stamp.motion_epoch;break_motion();}
        last_original_event_=r.original_event_sequence;return true;
    }
    constexpr bool motion(BoomshotMotionReceipt const& r,BoomshotFrame const& f) const noexcept {
        auto const& step=plan_.steps[cursor_];
        return r.binding==binding_&&r.operation_serial==operation_serial_&&r.plan_id==plan_.plan_id&&
            r.phase_id==step.phase_id&&r.geometry_id==step.geometry_id&&r.motion_epoch==motion_epoch_&&
            r.begin_sequence>=motion_floor_&&r.begin_sequence<f.stamp.sequence&&
            r.begin_ns>=motion_floor_ns_&&r.begin_ns<f.stamp.monotonic_ns&&
            f.stamp.monotonic_ns-r.begin_ns>=step.minimum_motion_ns&&r.player_driven&&
            r.continuous&&r.endpoints_and_direction_verified&&r.left_grip_held&&
            (!step.return_required||r.return_verified);
    }
    constexpr BoomshotEvent advance(std::int64_t physical_tick) noexcept {
        motion_floor_=sequence_;motion_floor_ns_=ns_;
        if(++cursor_<plan_.count){
            stage_=plan_.steps[cursor_].kind==BoomshotStepKind::insert_ammunition?
                BoomshotStage::take_ammunition:BoomshotStage::mechanism;
            return BoomshotEvent::step_completed;
        }
        closed_tick_=physical_tick;
        if(!accounting_.close(physical_tick))return invalidate();
        if(operation_==BoomshotOperation::inspection){
            const auto nonce=token();if(!nonce)return invalidate();
            inspection_exit_={config_.identity,binding_,nonce,operation_serial_};inspection_exit_dispatched_=false;
        }
        stage_=BoomshotStage::completion_commit;return BoomshotEvent::request_completion;
    }
public:
    BoomshotInteraction()=default;
    // Rebind only after fresh native inventory/mechanical reconciliation. Nonces
    // and visual item serials survive reset; old same-address receipts cannot bind.
    constexpr bool reset() noexcept {
        if(stage_!=BoomshotStage::unbound&&stage_!=BoomshotStage::ready)return false;
        const auto next=next_token_,intent=last_reload_intent_,trigger=last_trigger_,inspection=last_inspection_serial_;
        auto account=accounting_;*this=BoomshotInteraction{};
        accounting_=account;next_token_=next;last_reload_intent_=intent;last_trigger_=trigger;last_inspection_serial_=inspection;return true;
    }
    constexpr bool bind(BoomshotInstanceConfig const& config,BoomshotInventory const& observed) noexcept {
        if(stage_!=BoomshotStage::unbound||!config.valid()||!(observed.identity==config.identity)||
           !observed.revision||observed.loaded>config.capacity||observed.reserve>1000000)return false;
        const auto nonce=token();if(!nonce)return false;
        if(!accounting_.bind_idle(observed,nonce))return false;
        config_=config;inspection_config_=config.inspection();binding_=nonce;stage_=BoomshotStage::ready;return true;
    }
    constexpr bool bind_reload(BoomshotReloadConfig const& config,BoomshotInventory const& inventory) noexcept {
        if(stage_!=BoomshotStage::unbound||!config.valid()||!(inventory.identity==config.identity)||
           !inventory.revision||inventory.loaded>config.capacity||inventory.reserve>1000000)return false;
        const auto nonce=token();if(!nonce||!accounting_.bind_idle(inventory,nonce))return false;
        config_.identity=config.identity;config_.config_id=config.config_id;config_.ammunition_type=config.ammunition_type;
        config_.ammunition_kind=config.ammunition_kind;config_.capacity=config.capacity;config_.maximum_sample_gap_ns=config.maximum_sample_gap_ns;
        config_.current_instance_verified=config.current_instance_verified;config_.original_class_verified=config.original_class_verified;
        inspection_config_=config_.inspection();reload_only_=true;binding_=nonce;stage_=BoomshotStage::ready;return true;
    }
    constexpr bool bind_inspection(BoomshotInspectionConfig const& config,BoomshotInventory const& observed) noexcept {
        if(stage_!=BoomshotStage::unbound||!config.valid()||!(observed.identity==config.identity)||
           !observed.revision||observed.loaded>config.capacity||observed.reserve>1000000)return false;
        const auto nonce=token();if(!nonce||!accounting_.bind_idle(observed,nonce))return false;
        // Populate only fields used by the shared stamp/motion checks. Leave
        // every fire/ammo certificate unset, and explicitly gate those APIs.
        config_.identity=config.identity;config_.config_id=config.config_id;config_.capacity=config.capacity;
        config_.maximum_sample_gap_ns=config.maximum_sample_gap_ns;
        inspection_config_=config;inspection_only_=true;binding_=nonce;stage_=BoomshotStage::ready;return true;
    }
    constexpr BoomshotEvent observe(BoomshotFrame const& f) noexcept {
        BoomshotEvent event{};frame(f,event);return event;
    }
    constexpr void tracking_lost() noexcept {break_motion();}
    constexpr BoomshotEvent interrupt() noexcept {
        if(stage_==BoomshotStage::unbound||stage_==BoomshotStage::interrupted)
            return BoomshotEvent::rejected;
        // No native debit occurred for held ammo, so no native refund is issued.
        // Preserve the last confirmed inventory and cursor for reconciliation.
        return invalidate();
    }
    // Owner authenticates contact/opening intent and supplies one monotonic stream
    // shared by physical gestures and auxiliary input. Starting is not a receipt
    // that the opening stroke has already reached its calibrated endpoint.
    constexpr BoomshotEvent begin_reload(BoomshotFrame const& f,BoomshotReloadStart const& start,
                                        std::uint64_t intent,ReloadIntentSource source) noexcept {
        if(inspection_only_)return BoomshotEvent::rejected;
        if(source!=ReloadIntentSource::mechanism_open&&source!=ReloadIntentSource::auxiliary_button)
            return BoomshotEvent::rejected;
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::ready||!frame(f,event))return event;
        if(start.binding!=binding_||!intent||intent<=last_reload_intent_||
           !start.original_reload_serial||start.original_reload_serial<=reload_serial_||
           !start.original_reload_accepted||!start.automatic_completion_suppressed)
            return BoomshotEvent::rejected;
        if(accounting_.inventory().loaded==config_.capacity)return BoomshotEvent::full;
        std::uint32_t units{};
        if(!start.plan.valid(config_.identity,config_.config_id,accounting_.inventory().loaded?
            BoomshotCondition::partial:BoomshotCondition::empty,units)||
           start.perfect.end_ticks<0||(start.perfect.current_instance_verified&&
            (!(start.perfect.identity==config_.identity)||start.perfect.config_id!=config_.config_id||
             start.perfect.original_reload_serial!=start.original_reload_serial)))return BoomshotEvent::invalid;
        last_reload_intent_=intent;
        if(!accounting_.inventory().infinite_reserve&&(!accounting_.inventory().reserve||units>accounting_.inventory().reserve))return BoomshotEvent::no_ammo;
        if(units>config_.capacity-accounting_.inventory().loaded)return BoomshotEvent::invalid;
        if(!accounting_.begin(config_.reload(),f,start,intent,source))return BoomshotEvent::rejected;
        plan_=start.plan;reload_serial_=start.original_reload_serial;
        operation_serial_=reload_serial_;operation_=BoomshotOperation::reload;
        cursor_=0;started_tick_=tick_;motion_floor_=sequence_;motion_floor_ns_=ns_;
        stage_=plan_.steps[0].kind==BoomshotStepKind::insert_ammunition?
            BoomshotStage::take_ammunition:BoomshotStage::mechanism;
        return BoomshotEvent::started;
    }
    constexpr BoomshotEvent begin_r3(BoomshotFrame const& f,BoomshotReloadStart const& start) noexcept {
        return begin_reload(f,start,start.r3_press_edge,ReloadIntentSource::auxiliary_button);
    }
    constexpr BoomshotEvent begin_inspection(BoomshotFrame const& f,BoomshotInspectionStart const& start,
        std::uint64_t intent) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::ready||!frame(f,event))return event;
        if(start.binding!=binding_||!intent||intent<=last_reload_intent_||
           !start.animation_serial||start.animation_serial<=last_inspection_serial_||
           !start.original_gun_animation_owned||!start.native_reload_absent||
           !accounting_.begin_inspection(inspection_config_,f,start,intent))return BoomshotEvent::rejected;
        plan_=start.plan;last_reload_intent_=intent;operation_serial_=last_inspection_serial_=start.animation_serial;
        operation_=BoomshotOperation::inspection;cursor_=0;started_tick_=tick_;
        motion_floor_=sequence_;motion_floor_ns_=ns_;stage_=BoomshotStage::mechanism;return BoomshotEvent::started;
    }
    constexpr BoomshotEvent grab_left_shoulder(BoomshotFrame const& f,BoomshotAmmoReceipt const& r) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::take_ammunition||!frame(f,event))return event;
        if(!accounting_.reserve(r))return BoomshotEvent::rejected;
        held_item_=r.item_serial;motion_floor_=sequence_;motion_floor_ns_=ns_;
        stage_=BoomshotStage::holding_ammunition;return BoomshotEvent::grabbed;
    }
    constexpr BoomshotEvent release_ammunition(BoomshotFrame const& f,std::uint64_t item) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::holding_ammunition||!frame(f,event))return event;
        if(!item||item!=held_item_||!accounting_.release(item))return BoomshotEvent::rejected;
        held_item_=0;motion_floor_=sequence_;motion_floor_ns_=ns_;
        stage_=BoomshotStage::take_ammunition;return BoomshotEvent::released;
    }
    constexpr BoomshotEvent complete_mechanism(BoomshotFrame const& f,BoomshotMotionReceipt const& r) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::mechanism||!frame(f,event))return event;
        if(!motion(r,f))return BoomshotEvent::rejected;
        return advance(tick_);
    }
    constexpr BoomshotEvent insert_ammunition(BoomshotFrame const& f,BoomshotInsertionReceipt const& r) noexcept {
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::holding_ammunition||!frame(f,event))return event;
        if(r.item_serial!=held_item_||!r.approached_from_outside||!r.aligned||!r.seated||
           !motion(r.motion,f)||!accounting_.insert(r.item_serial))return BoomshotEvent::rejected;
        stage_=BoomshotStage::inventory_commit;return BoomshotEvent::request_inventory;
    }
    constexpr bool take_inspection_exit(BoomshotInspectionExit& out) noexcept {
        if((stage_!=BoomshotStage::completion_commit&&stage_!=BoomshotStage::interrupted)||
           operation_!=BoomshotOperation::inspection||!inspection_exit_.token||inspection_exit_dispatched_)return false;
        out=inspection_exit_;inspection_exit_dispatched_=true;return true;
    }
    constexpr BoomshotEvent acknowledge_inspection(BoomshotOriginalReceipt const& r) noexcept {
        if(operation_!=BoomshotOperation::inspection||!inspection_exit_dispatched_||!inspection_exit_.token||
           !receipt(r,inspection_exit_.token))return BoomshotEvent::rejected;
        if(!r.accepted||!r.original_effect_applied_once||!r.original_ready||r.result_applied!=ReloadTxnResult::none||
           !same_inventory(r.observed,false)||!accounting_.sync_idle(r.observed))return invalidate();
        const bool suspended=stage_==BoomshotStage::interrupted;
        inspection_exit_={};inspection_exit_dispatched_=false;operation_=BoomshotOperation::none;
        if(!suspended)stage_=BoomshotStage::ready;break_motion();return BoomshotEvent::inspection_completed;
    }
    using ReloadRequest=BoomshotReloadAccounting::Request;
    constexpr bool take_reload_request(ReloadRequest& out) noexcept {return accounting_.take(out);}
    constexpr bool take_completion_request(ReloadRequest& out) noexcept {
        if(!accounting_.take(out))return false;
        if(out.call!=ReloadTxnCall::finish&&out.call!=ReloadTxnCall::exit_without_fill)return false;
        return true;
    }
    constexpr bool dispatched_reload_request(ReloadRequest& out) const noexcept {return accounting_.dispatched(out);}
    constexpr BoomshotEvent acknowledge_inventory(BoomshotOriginalReceipt const& r) noexcept {
        ReloadRequest q{};
        if(!accounting_.dispatched(q)||q.call!=ReloadTxnCall::insert||!receipt(r,q.token))return BoomshotEvent::rejected;
        const bool suspended=stage_==BoomshotStage::interrupted;
        if(r.accepted&&!r.original_effect_applied_once){accounting_.unknown_outcome();return invalidate();}
        auto i=r.observed;
        auto next=accounting_;
        ReloadTxnInventory<BoomshotIdentity> observed{i.identity,i.revision,1,i.loaded,i.reserve,i.infinite_reserve};
        ReloadTxnReceipt<BoomshotIdentity> acknowledgement{r.request_token,r.accepted,observed,ReloadTxnResult::none};
        if(!next.acknowledge(acknowledgement))return invalidate();
        const bool direct_seat=!suspended&&r.accepted&&plan_.direct_exchange;
        if(direct_seat)direct_seat_token_=q.token;
        accounting_=next;
        held_item_=0;cursor_=r.accepted?(plan_.direct_exchange?plan_.count:2):1;
        motion_floor_=sequence_;motion_floor_ns_=ns_;
        if(!suspended){
            if(!r.accepted)stage_=BoomshotStage::take_ammunition;
            else if(plan_.direct_exchange){
                // Inventory receipt alone is not drum seating. Owner must
                // authenticate the original seat and supply its game time.
                stage_=BoomshotStage::completion_commit;
                return BoomshotEvent::inventory_committed;
            }else stage_=BoomshotStage::mechanism;
        }
        return r.accepted?BoomshotEvent::inventory_committed:BoomshotEvent::rejected;
    }
    // Direct exchange closes only after owner authenticates the original seat.
    // The receipt is a one-shot proof for the queued finish request; using the
    // consumed inventory receipt or an earlier frame's game time is not accepted.
    constexpr BoomshotEvent acknowledge_seated(BoomshotOriginalReceipt const& r) noexcept {
        if(stage_!=BoomshotStage::completion_commit||!plan_.direct_exchange||inspection_only_||
           !direct_seat_token_)return BoomshotEvent::rejected;
        if(r.request_token!=direct_seat_token_||!valid_stamp(r.stamp)||!r.original_event_sequence||
           r.original_event_sequence<=last_original_event_||!r.accepted||!r.original_effect_applied_once||
           !same_inventory(r.observed))return BoomshotEvent::rejected;
        auto next=accounting_;
        if(!next.close(r.stamp.game_tick))return invalidate();
        consume(r.stamp);last_original_event_=r.original_event_sequence;
        accounting_=next;direct_seat_token_=0;closed_tick_=r.stamp.game_tick;
        return BoomshotEvent::request_completion;
    }
    constexpr BoomshotEvent acknowledge_completion(BoomshotOriginalReceipt const& r) noexcept {
        ReloadRequest q{};
        if(!accounting_.dispatched(q)||q.call!=ReloadTxnCall::finish||!receipt(r,q.token))return BoomshotEvent::rejected;
        const bool suspended=stage_==BoomshotStage::interrupted;
        const auto applied=q.result==ReloadTxnResult::inspection?ReloadTxnResult::none:q.result;
        if(r.accepted&&(!r.original_effect_applied_once||!r.original_ready||r.result_applied!=applied)){
            accounting_.unknown_outcome();return invalidate();
        }
        auto i=r.observed;
        if(!accounting_.acknowledge({r.request_token,r.accepted,{i.identity,i.revision,1,i.loaded,i.reserve,i.infinite_reserve},r.result_applied}))return invalidate();
        if(!r.accepted)return invalidate();
        if(!suspended){operation_=BoomshotOperation::none;stage_=BoomshotStage::ready;break_motion();}
        return q.result==ReloadTxnResult::inspection?BoomshotEvent::inspection_completed:BoomshotEvent::reload_completed;
    }
    // Native certifies current mechanism state: 0 before open, 1 open/unfilled,
    // 2 seated/open, 3 closed. This is not inferred from animation time.
    constexpr bool resume_input_current(BoomshotFrame const& f,std::uint64_t intent,std::uint64_t serial,
                          std::uint8_t mechanism_state,bool original_ready) const noexcept {
        if(stage_!=BoomshotStage::interrupted||inspection_exit_.token||!valid_stamp(f.stamp)||!f.left_tracked||!f.right_tracked||
           !f.right_hand_equipped||!f.original_interaction_allowed||f.original_owns_hands||mechanism_state!=cursor_)return false;
        auto next=accounting_;
        return next.resume(f.observed,intent,binding_,serial)&&(!next.ready()||original_ready)&&
            (cursor_!=plan_.count||next.ready()||next.close(closed_tick_));
    }
    constexpr bool resume(BoomshotFrame const& f,std::uint64_t intent,std::uint64_t serial,
                          std::uint8_t mechanism_state,bool original_ready) noexcept {
        if(!resume_input_current(f,intent,serial,mechanism_state,original_ready))return false;
        auto next=accounting_;
        if(!next.resume(f.observed,intent,binding_,serial)||(next.ready()&&!original_ready))return false;
        if(cursor_==plan_.count&&!next.ready()&&!next.close(closed_tick_))return false;
        accounting_=next;consume(f.stamp);motion_epoch_=f.stamp.motion_epoch;last_reload_intent_=intent;
        held_item_=0;break_motion();
        stage_=next.ready()?BoomshotStage::ready:cursor_==plan_.count?BoomshotStage::completion_commit:
            plan_.steps[cursor_].kind==BoomshotStepKind::insert_ammunition?BoomshotStage::take_ammunition:BoomshotStage::mechanism;
        if(next.ready())operation_=BoomshotOperation::none;
        return true;
    }
    constexpr BoomshotEvent request_original_fire(BoomshotFrame const& f,std::uint64_t trigger_edge) noexcept {
        if(inspection_only_||reload_only_)return BoomshotEvent::rejected;
        BoomshotEvent event{BoomshotEvent::rejected};
        if(stage_!=BoomshotStage::ready||!frame(f,event))return event;
        if(!original_can_fire_||!trigger_edge||trigger_edge<=last_trigger_)return BoomshotEvent::rejected;
        last_trigger_=trigger_edge;
        if(accounting_.inventory().loaded<config_.original_ammo_cost)return BoomshotEvent::no_ammo;
        const auto nonce=token();if(!nonce)return invalidate();
        fire_request_={binding_,nonce,trigger_edge,accounting_.inventory()};stage_=BoomshotStage::fire_handoff;
        return BoomshotEvent::request_fire;
    }
    constexpr BoomshotEvent after_original_shot(BoomshotShotReceipt const& shot) noexcept {
        auto const& r=shot.original;
        if((stage_!=BoomshotStage::fire_handoff&&stage_!=BoomshotStage::interrupted)||
           !fire_dispatched_||!receipt(r,fire_request_.token))return BoomshotEvent::rejected;
        const bool suspended=stage_==BoomshotStage::interrupted;
        if(!r.accepted){
            if(r.original_effect_applied_once||shot.original_projectiles_spawned||shot.original_recoil_applied||
               !same_inventory(r.observed,false))return invalidate();
            if(!accounting_.sync_idle(r.observed))return invalidate();
            fire_request_={};fire_dispatched_=false;
            if(!suspended)stage_=BoomshotStage::ready;
            break_motion();
            return BoomshotEvent::rejected;
        }
        if(!r.original_effect_applied_once||!shot.original_recoil_applied||
           shot.original_projectiles_spawned!=config_.original_projectile_count||
           !(r.observed.identity==config_.identity)||r.observed.revision<=accounting_.inventory().revision||
           r.observed.loaded!=accounting_.inventory().loaded-config_.original_ammo_cost||
           r.observed.reserve!=accounting_.inventory().reserve||r.observed.infinite_reserve!=accounting_.inventory().infinite_reserve)return invalidate();
        if(!accounting_.sync_idle(r.observed))return invalidate();
        fire_request_={};fire_dispatched_=false;original_can_fire_=false;break_motion();
        plan_=config_.after_shot;operation_serial_=r.original_event_sequence;
        operation_=BoomshotOperation::shot_cycle;cursor_=0;started_tick_=tick_;
        if(plan_.count)stage_=BoomshotStage::mechanism;
        else{operation_=BoomshotOperation::none;stage_=BoomshotStage::ready;}
        if(suspended)stage_=BoomshotStage::interrupted;
        return BoomshotEvent::shot_recorded;
    }
    // Distinct from reload recovery: idle/fire interruption has no open reload
    // to resume. Never discard an unresolved dispatched shot to regain control.
    constexpr bool resume_ready(BoomshotFrame const& f,bool original_ready,bool mechanism_closed) noexcept {
        if(stage_!=BoomshotStage::interrupted||operation_!=BoomshotOperation::none||
           fire_request_.token||fire_dispatched_||!accounting_.ready()||!original_ready||!mechanism_closed||
           !valid_stamp(f.stamp)||!same_inventory(f.observed,false)||!f.left_tracked||!f.right_tracked||
           !f.right_hand_equipped||!f.original_interaction_allowed||f.original_owns_hands)return false;
        if(!accounting_.sync_idle(f.observed))return false;
        consume(f.stamp);motion_epoch_=f.stamp.motion_epoch;original_can_fire_=f.original_can_fire;
        stage_=BoomshotStage::ready;break_motion();return true;
    }
    constexpr bool take_fire_request(BoomshotFireRequest& out) noexcept {
        if(stage_!=BoomshotStage::fire_handoff||!fire_request_.token||fire_dispatched_)return false;
        out=fire_request_;fire_dispatched_=true;return true;
    }
    constexpr bool reload_input_current(BoomshotFrame const& f) const noexcept {
        return !inspection_only_&&stage_==BoomshotStage::ready&&tracked_&&!motion_broken_&&!animation_owns_hands_&&
            valid_stamp(f.stamp)&&f.stamp.motion_epoch==motion_epoch_&&f.stamp.monotonic_ns-ns_<=config_.maximum_sample_gap_ns&&
            same_inventory(f.observed)&&f.left_tracked&&f.right_tracked&&f.right_hand_equipped&&f.original_interaction_allowed&&!f.original_owns_hands;
    }
    constexpr bool current_motion_seed(BoomshotStamp const& s) const noexcept {
        return stage_==BoomshotStage::mechanism&&tracked_&&!motion_broken_&&!animation_owns_hands_&&s.valid()&&
            s.identity==config_.identity&&s.binding==binding_&&s.sequence==sequence_&&s.motion_epoch==motion_epoch_&&
            s.game_tick==tick_&&s.monotonic_ns==ns_;
    }
    constexpr bool current_insertion_seed(BoomshotStamp const& s) const noexcept {
        return stage_==BoomshotStage::holding_ammunition&&tracked_&&!motion_broken_&&!animation_owns_hands_&&s.valid()&&
            s.identity==config_.identity&&s.binding==binding_&&s.sequence==sequence_&&s.motion_epoch==motion_epoch_&&
            s.game_tick==tick_&&s.monotonic_ns==ns_;
    }
    constexpr bool next_insertion_sample(BoomshotStamp const& s) const noexcept {
        return stage_==BoomshotStage::holding_ammunition&&tracked_&&!motion_broken_&&!animation_owns_hands_&&valid_stamp(s)&&
            s.motion_epoch==motion_epoch_&&s.monotonic_ns-ns_<=config_.maximum_sample_gap_ns;
    }
    constexpr bool next_motion_sample(BoomshotStamp const& s) const noexcept {
        return stage_==BoomshotStage::mechanism&&tracked_&&!motion_broken_&&!animation_owns_hands_&&valid_stamp(s)&&
            s.motion_epoch==motion_epoch_&&s.monotonic_ns-ns_<=config_.maximum_sample_gap_ns;
    }
    constexpr bool mechanism_state(BoomshotMechanismState& out) const noexcept {
        out={};if(stage_!=BoomshotStage::mechanism||cursor_>=plan_.count)return false;
        const auto& step=plan_.steps[cursor_];
        if(step.kind!=BoomshotStepKind::move_mechanism||step.part!=BoomshotPart::barrel)return false;
        out={config_.identity,binding_,operation_serial_,plan_.plan_id,step.phase_id,step.geometry_id,cursor_,step.return_required};
        return true;
    }
    constexpr std::uint64_t binding_token() const noexcept {return binding_;}
    constexpr bool inspection_only() const noexcept {return inspection_only_;}
    constexpr BoomshotStage stage() const noexcept {return stage_;}
    constexpr std::uint8_t step_index() const noexcept {return cursor_;}
    constexpr BoomshotInventory inventory() const noexcept {return accounting_.inventory();}
    constexpr std::uint32_t held_units() const noexcept {return accounting_.reserved();}
    constexpr std::uint32_t available_reserve() const noexcept {return accounting_.available();}
    constexpr bool animation_owns_hands() const noexcept {return animation_owns_hands_;}
    bool needs_reconciliation() const noexcept {return stage_==BoomshotStage::interrupted;}
    constexpr bool can_request_fire() const noexcept {
        return !inspection_only_&&!reload_only_&&stage_==BoomshotStage::ready&&tracked_&&!animation_owns_hands_&&original_can_fire_&&
            accounting_.inventory().loaded>=config_.original_ammo_cost;
    }
    constexpr BoomshotFireRequest fire_request() const noexcept {return fire_request_;}
    constexpr BoomshotGuide guide() const noexcept {
        if(stage_==BoomshotStage::take_ammunition)return {BoomshotPart::left_shoulder};
        if(stage_==BoomshotStage::holding_ammunition)return {BoomshotPart::feed_port};
        if(stage_==BoomshotStage::mechanism)return {plan_.steps[cursor_].part};
        return {};
    }
};
}
