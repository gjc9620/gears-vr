#pragma once
#include "body_slots.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace gears_weapon {

// Saved evidence: analysis/boltok-pistol-animation-audit-20261022.json and
// analysis/weapon-partitions/boltok-verified-Object*/part.json. WarfareGame.u
// SHA256 91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13.
// The default mesh uses alow_poly/Object01..11, while its default animation
// set has Snub's five b_HG_* tracks. These do NOT identify a cylinder, slide,
// magazine, mechanical order, hand contact, live cartridge or capacity.
// There is consequently no default reload plan, geometry, count or deadline.
inline constexpr char boltok_original_class[]="Weap_LocustPistol";
inline constexpr char boltok_original_mesh[]="Geist_Pistol.Boltock_Pistol";
inline constexpr BodySlot boltok_body_slot=BodySlot::pistol; // Right hand, left pocket.
inline constexpr BodySlot boltok_ammo_slot=BodySlot::shoulder_ammo; // Left hand.
inline constexpr std::array<char const*,11> boltok_original_part_bones{
    "Object01","Object02","Object03","Object04","Object05","Object06",
    "Object07","Object08","Object09","Object10","Object11"};
using BoltokPartMask=std::uint16_t;
inline constexpr BoltokPartMask boltok_object_part(unsigned number) noexcept {
    return number>=1&&number<=11?static_cast<BoltokPartMask>(1u<<(number-1)):BoltokPartMask{};
}

enum class BoltokStepKind : std::uint8_t {unknown,move_original_parts,insert_ammunition};
enum class BoltokReloadCondition : std::uint8_t {empty,partial};
enum class BoltokReloadStage : std::uint8_t {
    unbound,ready,mechanism,take_ammunition,holding_ammunition,inventory_pending,
    completion_pending,fire_pending,interrupted
};
enum class BoltokReloadEvent : std::uint8_t {
    rejected,invalid,full,no_ammo,started,grabbed,released,step_completed,
    inventory_requested,inventory_committed,completion_requested,perfect,ordinary,
    fire_requested,shot_recorded,ready_observed,interrupted,inspection_completed
};
struct BoltokReloadIdentity {
    std::uint64_t world{},pawn{},weapon{},world_generation{},binding_generation{},
        session{},space_epoch{},input_epoch{},profile_generation{};
    bool valid() const noexcept {
        return world&&pawn&&weapon&&world_generation&&binding_generation&&session&&
            space_epoch&&input_epoch&&profile_generation;
    }
    bool operator==(BoltokReloadIdentity const& b) const noexcept {
        return world==b.world&&pawn==b.pawn&&weapon==b.weapon&&
            world_generation==b.world_generation&&binding_generation==b.binding_generation&&
            session==b.session&&space_epoch==b.space_epoch&&input_epoch==b.input_epoch&&
            profile_generation==b.profile_generation;
    }
};
// Input generation is part of identity. Tracking gaps/recenter/profile changes
// require interrupt + authoritative reconciliation + a new binding generation.
struct BoltokReloadStamp {
    BoltokReloadIdentity identity{};
    std::uint64_t sequence{};
    std::int64_t observed_ns{},game_tick{};
    bool right_tracked{},left_tracked{},weapon_owned{};
    bool valid(std::int64_t now_ns) const noexcept {
        return identity.valid()&&sequence&&observed_ns>0&&game_tick>=0&&
            now_ns>=observed_ns&&now_ns-observed_ns<=100000000;
    }
};
struct BoltokInventory {
    BoltokReloadIdentity identity{};
    std::uint64_t revision{},ammunition_type{};
    std::uint32_t capacity{},loaded{},reserve{};
    bool valid() const noexcept {
        return identity.valid()&&revision&&ammunition_type&&capacity&&capacity<=10000&&
            loaded<=capacity&&reserve<=1000000;
    }
};
struct BoltokBinding {
    BoltokReloadStamp stamp{};
    BoltokInventory inventory{};
    std::uint32_t original_shot_ammo_cost{};
    std::uint8_t original_inventory_slot{255};
    bool original_class_verified{},inventory_reconciled{},original_ready{};
};
struct BoltokReloadStep {
    BoltokStepKind kind{BoltokStepKind::unknown};
    BoltokPartMask parts{}; // Verified Object* attribution, not guessed function.
    std::uint64_t phase_id{},contact_id{},travel_id{},before_state{},after_state{};
    std::uint32_t rounds{}; // Exact verified amount for this insertion, otherwise zero.
};
struct BoltokReloadPlan {
    BoltokReloadIdentity identity{};
    std::uint64_t generation{},original_reload_serial{},initial_state{},final_ready_state{};
    BoltokReloadCondition condition{BoltokReloadCondition::empty};
    std::array<BoltokReloadStep,24> steps{};
    unsigned count{};
    bool current_instance_verified{},original_order_verified{};
    // Optional authenticated branch for each step boundary. Zero means no
    // calibrated direct close from that state; never infer it from the last
    // normal step or skip a required mechanical action.
    std::array<BoltokReloadStep,24> early_closures{};
    bool valid(BoltokInventory const& inventory,bool allow_inspection=false) const noexcept {
        if(!inventory.valid()||!(identity==inventory.identity)||!generation||!original_reload_serial||
           !initial_state||!final_ready_state||!current_instance_verified||
           !original_order_verified||!count||count>steps.size()||
           condition!=(inventory.loaded?BoltokReloadCondition::partial:BoltokReloadCondition::empty))
            return false;
        auto state=initial_state;
        std::uint32_t rounds=0;
        const auto target=(std::min)(inventory.capacity-inventory.loaded,inventory.reserve);
        if(!target&&!allow_inspection)return false;
        for(unsigned i=0;i<count;++i){
            auto const& s=steps[i];
            if(!s.parts||(s.parts&~BoltokPartMask{0x7ff})||!s.phase_id||!s.contact_id||
               !s.travel_id||s.before_state!=state||!s.after_state||s.after_state==state)
                return false;
            for(unsigned j=0;j<i;++j)if(steps[j].phase_id==s.phase_id)return false;
            if(s.kind==BoltokStepKind::insert_ammunition){
                if(!s.rounds||s.rounds>target-rounds)return false;
                rounds+=s.rounds;
            }else if(s.kind!=BoltokStepKind::move_original_parts||s.rounds)return false;
            state=s.after_state;
        }
        // Only data/ammo invariants constrain order. Ejection/closure/cocking
        // must come from the actual instance; none is invented here. Existing
        // live rounds stay loaded; a plan needing their removal is unsupported.
        return rounds==target&&state==final_ready_state;
    }
};
struct BoltokPerfectWindow {
    BoltokReloadIdentity identity{};
    std::uint64_t original_reload_serial{},plan_generation{};
    std::int64_t end_ticks{}; // Original perfect END relative to accepted R3.
    bool current_instance_verified{}; // Unknown deadline permits ordinary reload.
};
struct BoltokReloadStart {
    BoltokReloadStamp stamp{};
    BoltokInventory inventory{};
    BoltokReloadPlan plan{};
    BoltokPerfectWindow perfect{};
    std::uint64_t r3_press_edge{};
    bool original_reload_accepted{},automatic_refill_held{};
    std::int64_t accepted_begin_tick{-1}; // Authenticated original begin, not opening endpoint.
    bool mechanical_ownership_verified{}; // Existing original-animation owner, including inspection.
};
enum class BoltokAmmoKind : std::uint8_t {other,spent_case,snub_magazine,live_boltok_ammunition};
struct BoltokAmmoReceipt {
    std::uint64_t item_serial{},original_reload_serial{},plan_generation{},phase_id{},ammunition_type{};
    std::uint32_t rounds{};
    BodySlot source{BodySlot::none};
    BoltokAmmoKind kind{BoltokAmmoKind::other};
    bool live_ammunition_verified{},left_grip_held{};
};
// Produced by the geometry adapter from a continuous player-driven trajectory,
// not a completed animation or a bone name. Begin/end share this binding's
// generation; fresh endpoints alone cannot stand in for intervening samples.
struct BoltokMotionReceipt {
    std::uint64_t original_reload_serial{},plan_generation{},phase_id{},contact_id{},travel_id{},
        before_state{},after_state{},motion_begin_sequence{};
    BoltokPartMask parts{};
    std::int64_t motion_begin_ns{},maximum_sample_gap_ns{};
    unsigned tracked_samples{};
    bool measured_6dof_travel{},continuous{},complete_travel{},aligned{},
        player_driven{},left_grip_held{};
};
struct BoltokInsertionReceipt {
    std::uint64_t item_serial{};
    BoltokMotionReceipt motion{};
};
struct BoltokInventoryRequest {
    BoltokReloadIdentity identity{};
    std::uint64_t token{},original_reload_serial{},plan_generation{},item_serial{},before_revision{};
    std::uint32_t before_loaded{},before_reserve{},after_loaded{},after_reserve{};
};
struct BoltokCompletionRequest {
    BoltokReloadIdentity identity{};
    std::uint64_t token{},original_reload_serial{},plan_generation{},final_ready_state{},inventory_revision{};
    std::int64_t started_tick{},finished_tick{};
    BoltokReloadEvent proposed_result{BoltokReloadEvent::ordinary};
};
struct BoltokFireRequest {
    BoltokReloadIdentity identity{};
    std::uint64_t token{},trigger_edge{},before_revision{};
    std::uint32_t before_loaded{},before_reserve{},ammunition_cost{};
};
struct BoltokOriginalReceipt {
    std::uint64_t token{},original_reload_serial{},plan_generation{},original_event_sequence{};
    BoltokInventory observed{};
    bool accepted{},original_effect_applied_once{},original_ready{};
    std::uint64_t original_ready_state{}; // Completion only.
    BoltokReloadEvent applied_reload_result{BoltokReloadEvent::rejected};
};
struct BoltokShotReceipt {
    BoltokOriginalReceipt original{};
    bool original_shot_verified{},original_ballistics_retained{},original_recoil_retained{};
};
enum class BoltokGuideRegion : std::uint8_t {none,left_shoulder,weapon_part};
struct BoltokReloadGuide {
    BoltokGuideRegion region{BoltokGuideRegion::none};
    BoltokPartMask parts{};
    std::uint64_t contact_id{};
    float red{1},green{1},blue{0},alpha{1}; // Current step yellow render intent.
};

// Pure Boltok protocol. It never writes inventory, fires, rewards, plays audio,
// or drives a mesh. The game adapter must authenticate plans and input, hold
// automatic reload timers, commit request tokens exactly once, preserve the
// original damage/spread/recoil/cadence, render guidance and reconcile any
// interruption before rebinding. Real-game wiring and model calibration remain.
class BoltokReload {
    BoltokReloadStage stage_{BoltokReloadStage::unbound};
    BoltokReloadIdentity identity_{};
    BoltokInventory inventory_{};
    BoltokReloadPlan plan_{};
    BoltokPerfectWindow perfect_{};
    BoltokInventoryRequest inventory_request_{};
    BoltokCompletionRequest completion_request_{};
    BoltokFireRequest fire_request_{};
    std::uint64_t sequence_{},next_token_{},reload_serial_{},last_r3_{},last_trigger_{},
        last_original_event_{},last_item_{},held_item_{},motion_floor_{};
    std::int64_t ns_{},tick_{},started_tick_{},last_action_tick_{},motion_floor_ns_{};
    std::uint32_t held_{},shot_cost_{},opening_loaded_{};
    unsigned cursor_{};
    bool original_ready_{},last_left_tracked_{},motion_floor_tracked_{};

    bool accepts(BoltokReloadStamp const& s,std::int64_t now) const noexcept {
        return s.valid(now)&&s.identity==identity_&&s.sequence>sequence_&&
            s.observed_ns>ns_&&s.game_tick>=tick_&&s.right_tracked&&s.weapon_owned;
    }
    void consume(BoltokReloadStamp const& s) noexcept {
        sequence_=s.sequence;ns_=s.observed_ns;tick_=s.game_tick;last_left_tracked_=s.left_tracked;
    }
    void break_motion() noexcept {
        motion_floor_=sequence_;motion_floor_ns_=ns_;motion_floor_tracked_=last_left_tracked_;
    }
    bool same_inventory(BoltokInventory const& v) const noexcept {
        return v.valid()&&v.identity==identity_&&v.capacity==inventory_.capacity&&
            v.ammunition_type==inventory_.ammunition_type;
    }
    bool counts(BoltokInventory const& v,std::uint32_t loaded,std::uint32_t reserve) const noexcept {
        return same_inventory(v)&&v.loaded==loaded&&v.reserve==reserve;
    }
    bool receipt(BoltokOriginalReceipt const& r,std::uint64_t token) const noexcept {
        return token&&r.token==token&&r.original_event_sequence>last_original_event_&&
            r.observed.identity==identity_;
    }
    bool motion_step(BoltokReloadStamp const& s,BoltokMotionReceipt const& r,
                     BoltokReloadStep const& step) const noexcept {
        return s.left_tracked&&r.original_reload_serial==reload_serial_&&
            r.plan_generation==plan_.generation&&r.phase_id==step.phase_id&&r.parts==step.parts&&
            r.contact_id==step.contact_id&&r.travel_id==step.travel_id&&
            r.before_state==step.before_state&&r.after_state==step.after_state&&
            r.motion_begin_sequence>=motion_floor_&&r.motion_begin_sequence<s.sequence&&
            r.motion_begin_ns>=motion_floor_ns_&&r.motion_begin_ns<s.observed_ns&&
            (motion_floor_tracked_||(r.motion_begin_sequence>motion_floor_&&
                                    r.motion_begin_ns>motion_floor_ns_))&&
            r.maximum_sample_gap_ns>0&&r.maximum_sample_gap_ns<=100000000&&
            r.maximum_sample_gap_ns<=s.observed_ns-r.motion_begin_ns&&r.tracked_samples>=2&&
            s.observed_ns-r.motion_begin_ns<=
                std::int64_t(r.tracked_samples-1)*r.maximum_sample_gap_ns&&
            r.measured_6dof_travel&&r.continuous&&r.complete_travel&&r.aligned&&
            r.player_driven&&r.left_grip_held;
    }
    bool motion(BoltokReloadStamp const& s,BoltokMotionReceipt const& r) const noexcept {
        return motion_step(s,r,plan_.steps[cursor_]);
    }
    std::uint64_t token() noexcept {
        if(next_token_==(std::numeric_limits<std::uint64_t>::max)())return 0;
        return ++next_token_;
    }
    BoltokReloadEvent invalidate() noexcept {
        stage_=BoltokReloadStage::interrupted;original_ready_=false;
        return BoltokReloadEvent::interrupted;
    }
    BoltokReloadEvent advance() noexcept {
        ++cursor_;break_motion();
        if(cursor_!=plan_.count){
            stage_=plan_.steps[cursor_].kind==BoltokStepKind::insert_ammunition?
                BoltokReloadStage::take_ammunition:BoltokReloadStage::mechanism;
            return BoltokReloadEvent::step_completed;
        }
        const auto request_token=token();
        if(!request_token)return invalidate();
        const auto result=inventory_.loaded==opening_loaded_?BoltokReloadEvent::inspection_completed:
            perfect_.current_instance_verified&&
            last_action_tick_-started_tick_<=perfect_.end_ticks?
            BoltokReloadEvent::perfect:BoltokReloadEvent::ordinary;
        completion_request_={identity_,request_token,reload_serial_,plan_.generation,
            plan_.final_ready_state,inventory_.revision,started_tick_,last_action_tick_,result};
        stage_=BoltokReloadStage::completion_pending;
        return BoltokReloadEvent::completion_requested;
    }
public:
    // No reset clears token/edge/event history. Rebinding requires a strictly
    // newer generation and game-side reconciliation of all outstanding tokens;
    // dropping this object is not an inventory rollback or reward cancellation.
    bool bind(BoltokBinding const& b,std::int64_t now) noexcept {
        if(!b.stamp.valid(now)||!b.stamp.right_tracked||!b.stamp.weapon_owned||
           !b.inventory.valid()||!(b.inventory.identity==b.stamp.identity)||
           !b.original_class_verified||!b.inventory_reconciled||b.original_inventory_slot!=0||
           !b.original_shot_ammo_cost||b.original_shot_ammo_cost>b.inventory.capacity||
           b.stamp.identity.binding_generation<=identity_.binding_generation||
           (stage_!=BoltokReloadStage::unbound&&stage_!=BoltokReloadStage::interrupted))return false;
        identity_=b.stamp.identity;inventory_=b.inventory;shot_cost_=b.original_shot_ammo_cost;
        plan_={};perfect_={};inventory_request_={};completion_request_={};fire_request_={};
        held_=0;held_item_=0;cursor_=0;original_ready_=b.original_ready;
        // R3/trigger/item/event sequences are scoped to the authenticated binding.
        reload_serial_=last_r3_=last_trigger_=last_item_=last_original_event_=0;
        consume(b.stamp);break_motion();stage_=BoltokReloadStage::ready;return true;
    }
    BoltokReloadEvent interrupt() noexcept {
        if(stage_==BoltokReloadStage::unbound||stage_==BoltokReloadStage::interrupted)
            return BoltokReloadEvent::rejected;
        // Held ammo is a reservation in the last observed reserve count, not
        // a spent round. Keep it until the adapter reconciles pending commits.
        return invalidate();
    }
    // Call for a new input sample even when no action occurs. Loss of ownership,
    // tracking, freshness or generation fails closed until a reconciled rebind.
    BoltokReloadEvent observe_frame(BoltokReloadStamp const& s,std::int64_t now) noexcept {
        if(stage_==BoltokReloadStage::unbound||stage_==BoltokReloadStage::interrupted)
            return BoltokReloadEvent::rejected;
        if(!(s.identity==identity_)||!s.valid(now)||!s.right_tracked||!s.weapon_owned)
            return invalidate();
        if(s.sequence<=sequence_||s.observed_ns<=ns_||s.game_tick<tick_)
            return BoltokReloadEvent::rejected;
        consume(s);
        if(!s.left_tracked)break_motion();
        return BoltokReloadEvent::ready_observed;
    }
private:
    BoltokReloadEvent begin_activation(BoltokReloadStart const& s,std::uint64_t activation,
                                       std::int64_t now,bool physical=false) noexcept {
        if(stage_!=BoltokReloadStage::ready||!accepts(s.stamp,now))return BoltokReloadEvent::rejected;
        if(!activation||activation<=last_r3_||
           !counts(s.inventory,inventory_.loaded,inventory_.reserve)||
           s.inventory.revision!=inventory_.revision)return BoltokReloadEvent::invalid;
        if(!physical&&inventory_.loaded==inventory_.capacity){
            consume(s.stamp);last_r3_=activation;return BoltokReloadEvent::full;
        }
        if(!physical&&!inventory_.reserve){
            consume(s.stamp);last_r3_=activation;return BoltokReloadEvent::no_ammo;
        }
        const bool inspection=physical&&(!inventory_.reserve||inventory_.loaded==inventory_.capacity);
        if((!s.original_reload_accepted&&!inspection)||!s.automatic_refill_held||
           (physical&&!s.mechanical_ownership_verified)||!s.plan.valid(inventory_,inspection)||
           s.plan.original_reload_serial<=reload_serial_||s.perfect.end_ticks<0||
           (s.perfect.current_instance_verified&&(!(s.perfect.identity==identity_)||
            s.perfect.original_reload_serial!=s.plan.original_reload_serial||
            s.perfect.plan_generation!=s.plan.generation||s.perfect.end_ticks<=0)))
            return BoltokReloadEvent::invalid;
        consume(s.stamp);last_r3_=activation;reload_serial_=s.plan.original_reload_serial;
        plan_=s.plan;perfect_=s.perfect;cursor_=0;started_tick_=tick_;last_action_tick_=tick_;
        opening_loaded_=inventory_.loaded;
        original_ready_=false;break_motion();
        stage_=plan_.steps[0].kind==BoltokStepKind::insert_ammunition?
            BoltokReloadStage::take_ammunition:BoltokReloadStage::mechanism;
        return BoltokReloadEvent::started;
    }
public:
    // Legacy offline protocol entry. Production VR uses begin_grab_open below;
    // a button alone is not evidence that the support/cylinder was opened.
    BoltokReloadEvent begin_r3(BoltokReloadStart const& s,std::int64_t now) noexcept {
        return begin_activation(s,s.r3_press_edge,now);
    }
    // The adapter supplies the real arbiter press token and measured opening
    // stroke. Never synthesize R3, replay the opening, or accept bulk ammo on
    // this physical path. A rejected opening leaves every clock/token intact.
    BoltokReloadEvent begin_grab_open(BoltokReloadStart const& s,std::uint64_t grip_token,
                                     BoltokMotionReceipt const& opening,std::int64_t now) noexcept {
        if(!s.plan.count||s.plan.count>s.plan.steps.size()||s.inventory.capacity!=6||
           s.accepted_begin_tick<tick_||s.accepted_begin_tick>s.stamp.game_tick||
           s.plan.steps[0].kind!=BoltokStepKind::move_original_parts||s.plan.count<2||
           s.plan.steps[s.plan.count-1].kind!=BoltokStepKind::move_original_parts)
            return BoltokReloadEvent::invalid;
        for(unsigned i=0;i<s.plan.count;++i)
            if(s.plan.steps[i].kind==BoltokStepKind::insert_ammunition&&s.plan.steps[i].rounds!=1)
                return BoltokReloadEvent::invalid;
        // Check against the pre-opening motion floor, not the endpoint that
        // begin_activation consumes. The supplied trajectory must span actual
        // tracked samples since this binding's last observation.
        auto probe=*this;probe.plan_=s.plan;probe.reload_serial_=s.plan.original_reload_serial;
        probe.cursor_=0;
        if(!probe.motion(s.stamp,opening))return BoltokReloadEvent::rejected;
        auto candidate=*this;
        const auto result=candidate.begin_activation(s,grip_token,now,true);
        if(result!=BoltokReloadEvent::started)return result;
        candidate.started_tick_=s.accepted_begin_tick;
        candidate.advance(); // Opening is already physically complete.
        *this=candidate;
        return BoltokReloadEvent::started;
    }
    BoltokReloadEvent grab_left_shoulder(BoltokReloadStamp const& s,BoltokAmmoReceipt const& r,
                                          std::int64_t now) noexcept {
        if(stage_!=BoltokReloadStage::take_ammunition||!accepts(s,now)||!s.left_tracked)
            return BoltokReloadEvent::rejected;
        auto const& step=plan_.steps[cursor_];
        if(!r.item_serial||r.item_serial<=last_item_||r.original_reload_serial!=reload_serial_||
           r.plan_generation!=plan_.generation||r.phase_id!=step.phase_id||
           r.ammunition_type!=inventory_.ammunition_type||r.rounds!=step.rounds||
           r.rounds>inventory_.reserve||r.source!=boltok_ammo_slot||
           r.kind!=BoltokAmmoKind::live_boltok_ammunition||!r.live_ammunition_verified||
           !r.left_grip_held)return BoltokReloadEvent::rejected;
        consume(s);held_item_=last_item_=r.item_serial;held_=r.rounds;break_motion();
        stage_=BoltokReloadStage::holding_ammunition;return BoltokReloadEvent::grabbed;
    }
    BoltokReloadEvent release_ammunition(BoltokReloadStamp const& s,std::uint64_t item,
                                         std::int64_t now) noexcept {
        if(stage_!=BoltokReloadStage::holding_ammunition||!accepts(s,now)||
           !item||item!=held_item_)return BoltokReloadEvent::rejected;
        consume(s);held_=0;held_item_=0;break_motion();
        stage_=BoltokReloadStage::take_ammunition;return BoltokReloadEvent::released;
    }
    BoltokReloadEvent move_original_parts(BoltokReloadStamp const& s,BoltokMotionReceipt const& r,
                                           std::int64_t now) noexcept {
        if(stage_!=BoltokReloadStage::mechanism||!accepts(s,now)||
           plan_.steps[cursor_].kind!=BoltokStepKind::move_original_parts||!motion(s,r))
            return BoltokReloadEvent::rejected;
        consume(s);last_action_tick_=tick_;return advance();
    }
    BoltokReloadEvent close_early(BoltokReloadStamp const& s,BoltokMotionReceipt const& r,
                                  std::int64_t now) noexcept {
        // No closure with held ammo, an unresolved original write, or an
        // unmeasured shortcut. The branch must be in the authenticated plan.
        if(stage_!=BoltokReloadStage::take_ammunition||!accepts(s,now))
            return BoltokReloadEvent::rejected;
        const auto& close=plan_.early_closures[cursor_];
        if(close.kind!=BoltokStepKind::move_original_parts||close.rounds||!close.parts||
           (close.parts&~BoltokPartMask{0x7ff})||!close.phase_id||!close.contact_id||!close.travel_id||
           close.before_state!=plan_.steps[cursor_].before_state||
           close.after_state!=plan_.final_ready_state||close.before_state==close.after_state||
           !motion_step(s,r,close))return BoltokReloadEvent::rejected;
        const auto request_token=token();if(!request_token)return invalidate();
        consume(s);last_action_tick_=tick_;break_motion();
        const auto result=inventory_.loaded==opening_loaded_?
            BoltokReloadEvent::inspection_completed:BoltokReloadEvent::ordinary;
        completion_request_={identity_,request_token,reload_serial_,plan_.generation,
            plan_.final_ready_state,inventory_.revision,started_tick_,last_action_tick_,result};
        stage_=BoltokReloadStage::completion_pending;
        return BoltokReloadEvent::completion_requested;
    }
    BoltokReloadEvent insert_ammunition(BoltokReloadStamp const& s,BoltokInsertionReceipt const& r,
                                         std::int64_t now) noexcept {
        if(stage_!=BoltokReloadStage::holding_ammunition||!accepts(s,now)||
           !r.item_serial||r.item_serial!=held_item_||!motion(s,r.motion)||
           held_>inventory_.reserve||held_>inventory_.capacity-inventory_.loaded)
            return BoltokReloadEvent::rejected;
        const auto request_token=token();if(!request_token)return invalidate();
        consume(s);last_action_tick_=tick_;
        inventory_request_={identity_,request_token,reload_serial_,plan_.generation,held_item_,
            inventory_.revision,inventory_.loaded,inventory_.reserve,
            inventory_.loaded+held_,inventory_.reserve-held_};
        stage_=BoltokReloadStage::inventory_pending;return BoltokReloadEvent::inventory_requested;
    }
    BoltokReloadEvent acknowledge_inventory(BoltokReloadStamp const& s,BoltokOriginalReceipt const& r,
                                             std::int64_t now) noexcept {
        if(stage_!=BoltokReloadStage::inventory_pending||!accepts(s,now)||
           !receipt(r,inventory_request_.token)||r.original_reload_serial!=reload_serial_||
           r.plan_generation!=plan_.generation)return BoltokReloadEvent::rejected;
        auto const request=inventory_request_;
        if(!r.accepted){
            if(r.original_effect_applied_once||!counts(r.observed,request.before_loaded,request.before_reserve)||
               r.observed.revision<request.before_revision)return invalidate();
            consume(s);last_original_event_=r.original_event_sequence;inventory_=r.observed;
            inventory_request_={};break_motion();stage_=BoltokReloadStage::holding_ammunition;
            return BoltokReloadEvent::rejected;
        }
        if(!r.original_effect_applied_once||!counts(r.observed,request.after_loaded,request.after_reserve)||
           r.observed.revision<=request.before_revision)return invalidate();
        consume(s);last_original_event_=r.original_event_sequence;inventory_=r.observed;
        held_=0;held_item_=0;inventory_request_={};
        const auto advanced=advance();
        return advanced==BoltokReloadEvent::step_completed?BoltokReloadEvent::inventory_committed:advanced;
    }
    BoltokReloadEvent acknowledge_completion(BoltokReloadStamp const& s,BoltokOriginalReceipt const& r,
                                              std::int64_t now) noexcept {
        if(stage_!=BoltokReloadStage::completion_pending||!accepts(s,now)||
           !receipt(r,completion_request_.token)||r.original_reload_serial!=reload_serial_||
           r.plan_generation!=plan_.generation)return BoltokReloadEvent::rejected;
        if(!r.accepted||!r.original_effect_applied_once||!r.original_ready||
           r.original_ready_state!=plan_.final_ready_state||
           r.applied_reload_result!=completion_request_.proposed_result||
           !counts(r.observed,inventory_.loaded,inventory_.reserve)||
           r.observed.revision<inventory_.revision)return invalidate();
        const auto result=completion_request_.proposed_result;
        consume(s);last_original_event_=r.original_event_sequence;inventory_=r.observed;
        completion_request_={};original_ready_=true;stage_=BoltokReloadStage::ready;
        break_motion();return result;
    }
    BoltokReloadEvent request_original_fire(BoltokReloadStamp const& s,std::uint64_t trigger_edge,
                                             std::int64_t now) noexcept {
        if(stage_!=BoltokReloadStage::ready||!accepts(s,now)||!original_ready_||
           !trigger_edge||trigger_edge<=last_trigger_)return BoltokReloadEvent::rejected;
        consume(s);last_trigger_=trigger_edge;
        if(inventory_.loaded<shot_cost_)return BoltokReloadEvent::no_ammo;
        const auto request_token=token();if(!request_token)return invalidate();
        fire_request_={identity_,request_token,trigger_edge,inventory_.revision,
            inventory_.loaded,inventory_.reserve,shot_cost_};
        stage_=BoltokReloadStage::fire_pending;return BoltokReloadEvent::fire_requested;
    }
    BoltokReloadEvent after_original_shot(BoltokReloadStamp const& s,BoltokShotReceipt const& shot,
                                          std::int64_t now) noexcept {
        auto const& r=shot.original;
        if(stage_!=BoltokReloadStage::fire_pending||!accepts(s,now)||
           !receipt(r,fire_request_.token)||r.original_reload_serial||r.plan_generation)
            return BoltokReloadEvent::rejected;
        if(!r.accepted){
            if(r.original_effect_applied_once||!counts(r.observed,inventory_.loaded,inventory_.reserve)||
               r.observed.revision<inventory_.revision)return invalidate();
        }else if(!r.original_effect_applied_once||!shot.original_shot_verified||
            !shot.original_ballistics_retained||!shot.original_recoil_retained||
            !counts(r.observed,fire_request_.before_loaded-shot_cost_,fire_request_.before_reserve)||
            r.observed.revision<=fire_request_.before_revision)return invalidate();
        consume(s);last_original_event_=r.original_event_sequence;inventory_=r.observed;
        fire_request_={};original_ready_=r.original_ready;stage_=BoltokReloadStage::ready;
        break_motion();return r.accepted?BoltokReloadEvent::shot_recorded:BoltokReloadEvent::rejected;
    }
    // Original readiness/cadence is authoritative; no invented manual cycle
    // is added after a Boltok shot. Changed inventory needs reconciliation.
    BoltokReloadEvent observe_original_ready(BoltokReloadStamp const& s,BoltokInventory const& observed,
                                              bool ready,std::int64_t now) noexcept {
        if(stage_!=BoltokReloadStage::ready||!accepts(s,now))return BoltokReloadEvent::rejected;
        if(!counts(observed,inventory_.loaded,inventory_.reserve)||observed.revision<inventory_.revision)
            return invalidate();
        consume(s);inventory_=observed;original_ready_=ready;return BoltokReloadEvent::ready_observed;
    }
    BoltokReloadStage stage() const noexcept {return stage_;}
    unsigned current_step() const noexcept {return cursor_;}
    BoltokInventory inventory() const noexcept {return inventory_;}
    std::uint32_t loaded() const noexcept {return inventory_.loaded;}
    std::uint32_t reserve() const noexcept {return inventory_.reserve-held_;}
    std::uint32_t held() const noexcept {return held_;}
    std::uint32_t total() const noexcept {return loaded()+reserve()+held_;}
    // State gate only. The frame overload and request method also require
    // current tracking, ownership, input identity and freshness.
    bool can_fire() const noexcept {
        return stage_==BoltokReloadStage::ready&&original_ready_&&inventory_.loaded>=shot_cost_;
    }
    bool can_fire(BoltokReloadStamp const& s,std::int64_t now) const noexcept {
        return can_fire()&&accepts(s,now);
    }
    bool needs_reconciliation() const noexcept {return stage_==BoltokReloadStage::interrupted;}
    BoltokInventoryRequest inventory_request() const noexcept {return inventory_request_;}
    BoltokCompletionRequest completion_request() const noexcept {return completion_request_;}
    BoltokFireRequest fire_request() const noexcept {return fire_request_;}
    BoltokReloadGuide guide() const noexcept {
        if(stage_==BoltokReloadStage::take_ammunition)return {BoltokGuideRegion::left_shoulder,0,0};
        if(stage_==BoltokReloadStage::mechanism||stage_==BoltokReloadStage::holding_ammunition){
            auto const& step=plan_.steps[cursor_];
            return {BoltokGuideRegion::weapon_part,step.parts,step.contact_id};
        }
        return {};
    }
};
} // namespace gears_weapon
