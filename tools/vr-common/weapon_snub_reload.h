#pragma once
#include "body_slots.h"
#include "weapon_reload_transaction.h"
#include <algorithm>
#include <array>
#include <cstdint>

namespace gears_weapon {

// Saved evidence: analysis/snub-animation-tracks.json, WarfareGame.u SHA256
// 91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13.
// HG_Fire translates b_HG_Slide; AR_Reload animates that slide, both wings
// and b_HG_Magazine. Mesh manifests confirm rigid parts, not hand contacts,
// travel, phase ordering, or empty/partial reload differences. Accordingly
// there is no built-in mechanical plan, capacity, grip, travel or deadline.
inline constexpr char snub_original_class[]="Weap_COGPistol";
inline constexpr char snub_original_mesh[]="COG_Pistol.COG_Pistol";
inline constexpr BodySlot snub_body_slot=BodySlot::pistol; // Right hand, left pocket.
inline constexpr BodySlot snub_ammo_slot=BodySlot::shoulder_ammo; // Left hand.
inline constexpr std::array<char const*,4> snub_original_part_bones{
    "b_HG_Magazine","b_HG_Slide","b_HG_Wing_L","b_HG_Wing_R"};

enum class SnubPart : std::uint8_t {
    none=0,magazine=1,slide=2,left_wing=4,right_wing=8,feed_port=16,left_shoulder=32
};
using SnubPartMask=std::uint8_t;
inline constexpr SnubPartMask snub_part(SnubPart part) noexcept {
    return static_cast<SnubPartMask>(part);
}
enum class SnubStepKind : std::uint8_t { eject_magazine,insert_magazine,move_original_parts };
enum class SnubReloadCondition : std::uint8_t { empty,partial };
enum class SnubReloadStage : std::uint8_t {
    idle,mechanism,old_magazine,take_magazine,holding_magazine,inventory_pending,
    pending_completion,complete,interrupted
};
enum class SnubReloadEvent : std::uint8_t {
    rejected,invalid,full,no_ammo,started,ejected,grabbed,released,inserted,
    mechanism_completed,perfect,ordinary,committed,interrupted,resumed,motion_reset,
    insertion_requested,cancelled
};
struct SnubReloadIdentity {
    std::uint64_t world{},pawn{},weapon{},world_generation{};
    bool valid() const noexcept {return world&&pawn&&weapon&&world_generation;}
    bool operator==(SnubReloadIdentity const& b) const noexcept {
        return world==b.world&&pawn==b.pawn&&weapon==b.weapon&&world_generation==b.world_generation;
    }
};
// Increasing event sequence on the game thread, game reload clock (including
// pauses), and an adapter continuity generation covering XR session, reference
// space, input profile, tracking gaps and recenter. It is not a wall clock.
struct SnubReloadStamp {
    SnubReloadIdentity identity{};
    std::uint64_t sequence{},motion_epoch{};
    std::int64_t game_tick{};
    bool valid() const noexcept {return identity.valid()&&sequence&&motion_epoch&&game_tick>=0;}
};
struct SnubReloadStep {
    SnubStepKind kind{SnubStepKind::eject_magazine};
    SnubPartMask parts{};
    std::uint64_t phase_id{},measured_travel_id{};
};
// The adapter verifies the original parts/curve and the owner-selected VR
// order. Empty requires one coupled slide/wing action AFTER insertion; partial
// finishes at insertion. These VR branches are not an original chamber model.
struct SnubReloadPlan {
    SnubReloadIdentity identity{};
    std::uint64_t id{},feed_mount_id{};
    SnubReloadCondition condition{SnubReloadCondition::empty};
    std::array<SnubReloadStep,10> steps{};
    std::uint8_t count{};
    bool original_order_verified{},current_instance_verified{};
    bool valid(SnubReloadIdentity const& current,std::uint32_t loaded) const noexcept {
        if(!(identity==current)||!id||!feed_mount_id||!original_order_verified||
           !current_instance_verified||count!=(loaded?2:3)||
           condition!=(loaded?SnubReloadCondition::partial:SnubReloadCondition::empty))return false;
        unsigned ejections=0,insertions=0;
        constexpr auto moving=snub_part(SnubPart::slide)|snub_part(SnubPart::left_wing)|
                              snub_part(SnubPart::right_wing);
        for(unsigned i=0;i<count;++i){
            auto const& step=steps[i];
            if(!step.phase_id)return false;
            for(unsigned j=0;j<i;++j)if(steps[j].phase_id==step.phase_id)return false;
            switch(step.kind){
                case SnubStepKind::eject_magazine:
                    if(i||step.parts!=snub_part(SnubPart::magazine))return false;
                    ++ejections;break;
                case SnubStepKind::insert_magazine:
                    if(i!=1||step.parts!=snub_part(SnubPart::feed_port))return false;
                    ++insertions;break;
                case SnubStepKind::move_original_parts:
                    if(i!=2||loaded||!(step.parts&snub_part(SnubPart::slide))||
                       (step.parts&~moving)||!step.measured_travel_id)return false;
                    break;
                default:return false;
            }
        }
        return ejections==1&&insertions==1;
    }
};
struct SnubPerfectWindow {
    SnubReloadIdentity identity{};
    std::uint64_t original_reload_serial{};
    std::int64_t end_ticks{}; // Relative to physical-start acceptance, not clip length.
    bool current_instance_verified{}; // False permits ordinary completion.
};
struct SnubReloadStart {
    SnubReloadStamp stamp{};
    std::uint64_t r3_press_edge{},original_reload_serial{};
    std::uint32_t capacity{},loaded{},reserve{};
    SnubReloadPlan plan{};
    SnubPerfectWindow perfect{};
    std::uint8_t original_inventory_slot{255};
    bool original_class_verified{},right_hand_equipped{},original_reload_accepted{};
    // Physical input uses intent_id directly. R3 remains an optional auxiliary
    // wrapper; neither a held button nor generated R3 grants a physical intent.
    std::uint64_t intent_id{},inventory_revision{},old_magazine_item{};
    ReloadIntentSource intent_source{ReloadIntentSource::magazine_pull};
    bool infinite_reserve{};
};
struct SnubEjectionReceipt {
    std::uint64_t original_reload_serial{},plan_id{},phase_id{};
    bool original_magazine_detached{},snub_magazine_binding_verified{};
};
struct SnubMagazineReceipt {
    std::uint64_t item_serial{}; // Increasing within this transaction, never reused.
    BodySlot source{BodySlot::none};
    bool snub_magazine_verified{},left_hand_tracked{},left_grip_held{},right_hand_tracked{};
};
struct SnubInsertionReceipt {
    std::uint64_t item_serial{},feed_mount_id{},motion_begin_sequence{};
    bool aligned{},continuous{},seated{},left_hand_tracked{},left_grip_held{},right_hand_tracked{};
};
// Cancellation/recovery only: the owning GT adapter has restored the original
// feed visual and closed parts and re-read the original inventory. This is not
// a hand insertion, an ammo refund or permission to resurrect a dropped prop.
struct SnubOriginalReturnReceipt {
    std::uint64_t item_serial{};
    bool original_feed_restored{},mechanism_closed{},current_instance_verified{};
};
struct SnubPartMotionReceipt {
    std::uint64_t original_reload_serial{},plan_id{},phase_id{},measured_travel_id{},
                  motion_begin_sequence{};
    SnubPartMask parts{};
    bool endpoints_verified{},continuous{},player_driven{},left_hand_tracked{},
         left_grip_held{},right_hand_tracked{};
};
struct SnubReloadCompletion {
    SnubReloadIdentity identity{};
    std::uint64_t original_reload_serial{};
    std::uint32_t loaded{},reserve{};
    std::int64_t started_tick{},finished_tick{};
    SnubReloadEvent result{SnubReloadEvent::rejected};
    ReloadTxnRequest<SnubReloadIdentity> native_request{};
};
struct SnubGuide {
    SnubPartMask parts{};
    float red{1},green{1},blue{0},alpha{1}; // Yellow render intent only.
};

using SnubInventory=ReloadTxnInventory<SnubReloadIdentity>;
using SnubInventoryRequest=ReloadTxnRequest<SnubReloadIdentity>;
using SnubInventoryReceipt=ReloadTxnReceipt<SnubReloadIdentity>;

// One shared ledger owns native counts, reservations, requests and late-call
// reconciliation. Hand/animation receipts only advance mechanics. An insertion
// intent never changes inventory; only its authenticated native receipt does.
class SnubReload {
    SnubReloadStage stage_{SnubReloadStage::idle};
    SnubReloadIdentity identity_{};
    SnubReloadPlan plan_{};
    SnubPerfectWindow perfect_{};
    ReloadTransaction<SnubReloadIdentity> ledger_{};
    std::uint64_t intent_{},r3_edge_{},serial_{},sequence_{},motion_epoch_{},step_since_{},
                  motion_since_{},old_item_{},held_item_{},last_item_{},grab_sequence_{};
    std::int64_t start_tick_{},tick_{},finish_tick_{},physical_closed_tick_{-1};
    std::uint8_t cursor_{};
    bool old_held_{},perfect_eligible_{},completion_taken_{},close_requested_{};
    std::int64_t window_at_close_{};
    SnubReloadEvent outcome_{SnubReloadEvent::rejected};

    bool active() const noexcept {
        return stage_!=SnubReloadStage::idle&&stage_!=SnubReloadStage::complete&&
               stage_!=SnubReloadStage::interrupted;
    }
    bool accepts(SnubReloadStamp const& s,bool same_motion=true) const noexcept {
        return s.valid()&&s.identity==identity_&&s.sequence>sequence_&&s.game_tick>=tick_&&
            (same_motion?s.motion_epoch==motion_epoch_:s.motion_epoch>=motion_epoch_);
    }
    void consume(SnubReloadStamp const& s) noexcept {sequence_=s.sequence;tick_=s.game_tick;}
    SnubReloadEvent close(SnubReloadStamp const& s) noexcept {
        if(!close_requested_){
            if(physical_closed_tick_<start_tick_||physical_closed_tick_>s.game_tick)
                return SnubReloadEvent::rejected;
            finish_tick_=physical_closed_tick_;
            window_at_close_=perfect_eligible_?perfect_.end_ticks:0;
        }
        if(!ledger_.confirm_closed(finish_tick_,window_at_close_))return SnubReloadEvent::rejected;
        close_requested_=true;completion_taken_=false;
        const bool inspection=ledger_.inserted()==0;
        outcome_=inspection?SnubReloadEvent::cancelled:
            physical_reload_result(ledger_.target(),ledger_.inserted(),start_tick_,
                finish_tick_,window_at_close_)==ReloadTxnResult::perfect?
                SnubReloadEvent::perfect:SnubReloadEvent::ordinary;
        stage_=ledger_.stage()==ReloadTxnStage::complete?SnubReloadStage::complete:
            SnubReloadStage::pending_completion;
        return outcome_;
    }
    SnubReloadEvent advance(SnubReloadStamp const& s,SnubReloadEvent event) noexcept {
        consume(s);step_since_=s.sequence;++cursor_;
        if(cursor_==plan_.count)return close(s);
        stage_=plan_.steps[cursor_].kind==SnubStepKind::insert_magazine?
            (old_held_?SnubReloadStage::old_magazine:SnubReloadStage::take_magazine):
            SnubReloadStage::mechanism;
        return event;
    }
    bool valid_stroke(SnubReloadStamp const& s,std::uint64_t began,
                      std::uint64_t since) const noexcept {
        return accepts(s)&&began>=(std::max)(since,motion_since_)&&began<s.sequence;
    }
public:
    // Never erase an unresolved inventory/exit call or unfinished mechanics.
    bool reset() noexcept {
        if(stage_!=SnubReloadStage::idle&&stage_!=SnubReloadStage::complete)return false;
        if(ledger_.pending())return false;
        stage_=SnubReloadStage::idle;return true;
    }
    SnubReloadEvent begin(SnubReloadStart const& s) noexcept {
        if(stage_!=SnubReloadStage::idle&&stage_!=SnubReloadStage::complete)
            return SnubReloadEvent::rejected;
        if(!s.stamp.valid()||!s.intent_id||!s.inventory_revision||!s.old_magazine_item||
           (s.intent_source!=ReloadIntentSource::magazine_pull&&
            s.intent_source!=ReloadIntentSource::auxiliary_button)||
           (s.intent_source==ReloadIntentSource::magazine_pull&&s.r3_press_edge)||
           (s.intent_source==ReloadIntentSource::auxiliary_button&&
            (!s.r3_press_edge||s.r3_press_edge<=r3_edge_))||
           !s.original_class_verified||!s.right_hand_equipped||
           s.original_inventory_slot!=0||!s.capacity||s.capacity>10000||
           s.loaded>s.capacity||s.reserve>1000000||!s.plan.valid(s.stamp.identity,s.loaded)||
           s.perfect.end_ticks<0||(s.perfect.current_instance_verified&&
           (!(s.perfect.identity==s.stamp.identity)||
            s.perfect.original_reload_serial!=s.original_reload_serial||s.perfect.end_ticks<=0)))
            return SnubReloadEvent::invalid;
        if(identity_.valid()&&(!(s.stamp.identity==identity_)||!accepts(s.stamp,false)||
           s.intent_id<=intent_||s.old_magazine_item<=last_item_))
            return SnubReloadEvent::rejected;
        const auto target=s.infinite_reserve?s.capacity-s.loaded:
            (std::min)(s.capacity-s.loaded,s.reserve);
        if(target?(!s.original_reload_accepted||!s.original_reload_serial||
                   (serial_&&s.original_reload_serial<=serial_)):
                  (s.original_reload_accepted||s.original_reload_serial||s.perfect.current_instance_verified))
            return SnubReloadEvent::invalid;
        SnubInventory observed{s.stamp.identity,s.inventory_revision,s.capacity,s.loaded,
            s.reserve,s.infinite_reserve};
        auto next=ledger_;
        if(next.stage()==ReloadTxnStage::unbound){if(!next.bind(observed))return SnubReloadEvent::invalid;}
        else if(!next.sync_idle(observed))return SnubReloadEvent::rejected;
        if(!next.begin(observed,s.intent_id,s.intent_source,s.original_reload_serial,
                ReloadTxnMode::magazine,s.capacity,s.stamp.game_tick,s.old_magazine_item))
            return SnubReloadEvent::rejected;
        ledger_=next;
        identity_=s.stamp.identity;plan_=s.plan;perfect_=s.perfect;
        intent_=s.intent_id;if(s.r3_press_edge)r3_edge_=s.r3_press_edge;
        if(s.original_reload_serial)serial_=s.original_reload_serial;
        old_item_=last_item_=s.old_magazine_item;old_held_=true;
        held_item_=grab_sequence_=0;cursor_=0;
        start_tick_=s.stamp.game_tick;finish_tick_=0;physical_closed_tick_=-1;
        motion_epoch_=s.stamp.motion_epoch;step_since_=motion_since_=s.stamp.sequence;
        perfect_eligible_=s.perfect.current_instance_verified;
        completion_taken_=close_requested_=false;window_at_close_=0;outcome_=SnubReloadEvent::rejected;
        consume(s.stamp);stage_=SnubReloadStage::mechanism;return SnubReloadEvent::started;
    }
    SnubReloadEvent begin_r3(SnubReloadStart const& source) noexcept {
        auto value=source;value.intent_source=ReloadIntentSource::auxiliary_button;
        value.intent_id=value.r3_press_edge;return begin(value);
    }
    SnubReloadEvent confirm_ejection(SnubReloadStamp const& s,
                                     SnubEjectionReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::mechanism||!accepts(s)||
           plan_.steps[cursor_].kind!=SnubStepKind::eject_magazine||
           r.original_reload_serial!=ledger_serial()||r.plan_id!=plan_.id||
           r.phase_id!=plan_.steps[cursor_].phase_id||!r.original_magazine_detached||
           !r.snub_magazine_binding_verified)return SnubReloadEvent::rejected;
        return advance(s,SnubReloadEvent::ejected);
    }
    SnubReloadEvent release_original(SnubReloadStamp const& s,std::uint64_t item) noexcept {
        if(stage_!=SnubReloadStage::old_magazine||!accepts(s)||!old_held_||
           item!=old_item_||!ledger_.stow_original(item))return SnubReloadEvent::rejected;
        old_held_=false;consume(s);stage_=SnubReloadStage::take_magazine;
        return SnubReloadEvent::released; // Discarded visual is destroyed by adapter.
    }
    SnubReloadEvent restore_original(SnubReloadStamp const& s,SnubInsertionReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::old_magazine||!old_held_||r.item_serial!=old_item_||
           !valid_insertion(s,r,step_since_)||!ledger_.restore_original(old_item_))
            return SnubReloadEvent::rejected;
        old_held_=false;consume(s);cursor_=plan_.count;physical_closed_tick_=s.game_tick;return close(s);
    }
    SnubReloadEvent grab_left_shoulder(SnubReloadStamp const& s,
                                       SnubMagazineReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::take_magazine||!accepts(s)||!r.item_serial||
           r.item_serial<=last_item_||r.source!=snub_ammo_slot||!r.snub_magazine_verified||
           !r.left_hand_tracked||!r.left_grip_held||!r.right_hand_tracked||
           !ledger_.reserve_item(r.item_serial))return SnubReloadEvent::rejected;
        consume(s);held_item_=last_item_=r.item_serial;grab_sequence_=s.sequence;
        stage_=SnubReloadStage::holding_magazine;
        return SnubReloadEvent::grabbed;
    }
    SnubReloadEvent release_magazine(SnubReloadStamp const& s,std::uint64_t item) noexcept {
        if(stage_!=SnubReloadStage::holding_magazine||!accepts(s)||!item||item!=held_item_)
            return SnubReloadEvent::rejected;
        if(!ledger_.release_item(item))return SnubReloadEvent::rejected;
        consume(s);held_item_=0;
        stage_=SnubReloadStage::take_magazine;return SnubReloadEvent::released;
    }
    SnubReloadEvent insert_magazine(SnubReloadStamp const& s,
                                    SnubInsertionReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::holding_magazine||r.item_serial!=held_item_||
           !valid_insertion(s,r,grab_sequence_)||!ledger_.request_insertion(held_item_))
            return SnubReloadEvent::rejected;
        consume(s);stage_=SnubReloadStage::inventory_pending;
        // A later native acknowledgement cannot make the player's already
        // completed partial-reload insertion miss its timing window.
        if(plan_.condition==SnubReloadCondition::partial)physical_closed_tick_=s.game_tick;
        return SnubReloadEvent::insertion_requested;
    }
    bool take_inventory_request(SnubInventoryRequest& out) noexcept {
        return stage_==SnubReloadStage::inventory_pending&&
            ledger_.pending_call()==ReloadTxnCall::insert&&ledger_.take_request(out);
    }
    SnubReloadEvent acknowledge_inventory(SnubReloadStamp const& s,SnubInventoryReceipt const& r) noexcept {
        const bool stopped=stage_==SnubReloadStage::interrupted;
        if((stage_!=SnubReloadStage::inventory_pending&&!stopped)||!accepts(s,!stopped)||
           ledger_.pending_call()!=ReloadTxnCall::insert||!ledger_.acknowledge(r))
            return SnubReloadEvent::rejected;
        held_item_=0;
        if(stopped){
            consume(s);if(r.accepted){++cursor_;step_since_=s.sequence;}
            else physical_closed_tick_=-1;
            return SnubReloadEvent::interrupted; // A late commit cannot unlock fire.
        }
        if(!r.accepted){consume(s);physical_closed_tick_=-1;stage_=SnubReloadStage::take_magazine;return SnubReloadEvent::released;}
        return advance(s,SnubReloadEvent::inserted);
    }
    SnubReloadEvent move_original_parts(SnubReloadStamp const& s,
                                        SnubPartMotionReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::mechanism)return SnubReloadEvent::rejected;
        auto const& step=plan_.steps[cursor_];
        if(step.kind!=SnubStepKind::move_original_parts||r.original_reload_serial!=ledger_serial()||
           r.plan_id!=plan_.id||r.phase_id!=step.phase_id||r.parts!=step.parts||
           r.measured_travel_id!=step.measured_travel_id||!r.endpoints_verified||
           !r.continuous||!r.player_driven||!r.left_hand_tracked||!r.left_grip_held||
           !r.right_hand_tracked||!valid_stroke(s,r.motion_begin_sequence,step_since_))
            return SnubReloadEvent::rejected;
        physical_closed_tick_=s.game_tick;
        return advance(s,SnubReloadEvent::mechanism_completed);
    }
    // A tracking/reference discontinuity invalidates strokes, never pauses or
    // restarts the game reload clock. A fresh path must begin after this stamp.
    SnubReloadEvent motion_discontinuity(SnubReloadStamp const& s) noexcept {
        if(!active()||!accepts(s,false)||s.motion_epoch==motion_epoch_)
            return SnubReloadEvent::rejected;
        consume(s);motion_epoch_=s.motion_epoch;motion_since_=s.sequence;
        return SnubReloadEvent::motion_reset;
    }
    SnubReloadEvent interrupt() noexcept {
        if(!active()||!ledger_.interrupt())return SnubReloadEvent::rejected;
        if(!ledger_.pending())held_item_=0;
        perfect_eligible_=false;
        stage_=SnubReloadStage::interrupted;return SnubReloadEvent::interrupted;
    }
    // After stow, only this same original attempt can resume, with its exact
    // reconciled counts and phase. It retains seated ammo and remaining steps,
    // and can only finish ordinary. Death/world changes require reset instead.
    SnubReloadEvent resume(SnubReloadStamp const& s,std::uint64_t fresh_intent,
                           std::uint64_t serial,SnubInventory const& observed,
                           bool original_phase_and_equipment_verified) noexcept {
        if(stage_!=SnubReloadStage::interrupted||!accepts(s,false)||serial!=ledger_serial()||
           !original_phase_and_equipment_verified||
           ledger_.pending()||fresh_intent<=intent_)return SnubReloadEvent::rejected;
        if(!ledger_.resume(observed,fresh_intent,serial))return SnubReloadEvent::rejected;
        intent_=fresh_intent;consume(s);motion_epoch_=s.motion_epoch;step_since_=motion_since_=s.sequence;
        if(ledger_.stage()==ReloadTxnStage::complete){stage_=SnubReloadStage::complete;return SnubReloadEvent::committed;}
        if(cursor_==plan_.count)return close(s);
        stage_=plan_.steps[cursor_].kind==SnubStepKind::insert_magazine?
            (old_held_?SnubReloadStage::old_magazine:SnubReloadStage::take_magazine):SnubReloadStage::mechanism;
        return SnubReloadEvent::resumed;
    }
    SnubReloadEvent cancel_to_original(SnubReloadStamp const& s,std::uint64_t fresh_intent,
        SnubInventory const& observed,SnubOriginalReturnReceipt const& receipt) noexcept {
        if(stage_!=SnubReloadStage::interrupted||!accepts(s,false)||ledger_.pending()||
           ledger_.inserted()||fresh_intent<=intent_||receipt.item_serial!=old_item_||
           !receipt.original_feed_restored||!receipt.mechanism_closed||!receipt.current_instance_verified)
            return SnubReloadEvent::rejected;
        auto next=ledger_;
        if(!next.resume(observed,fresh_intent,ledger_serial())||!next.restore_original(old_item_))
            return SnubReloadEvent::rejected;
        ledger_=next;intent_=fresh_intent;old_held_=false;held_item_=0;
        consume(s);motion_epoch_=s.motion_epoch;cursor_=plan_.count;physical_closed_tick_=s.game_tick;
        return close(s); // q=0 needs no ForceReload/exit; accepted reload exits without refill.
    }
    // Key external inventory/reward application by identity + original serial.
    // Taking this intent never proves the original reward has been applied.
    bool take_completion(SnubReloadCompletion& out) noexcept {
        if(stage_!=SnubReloadStage::pending_completion||completion_taken_)return false;
        SnubInventoryRequest request{};
        if(ledger_.pending_call()==ReloadTxnCall::insert||!ledger_.take_request(request))return false;
        completion_taken_=true;
        out={identity_,request.reload_serial,loaded(),reserve(),start_tick_,finish_tick_,outcome_,request};return true;
    }
    SnubReloadEvent acknowledge_completion(SnubReloadStamp const& s,SnubInventoryReceipt const& receipt,
                                           bool original_ready) noexcept {
        const bool stopped=stage_==SnubReloadStage::interrupted;
        if((stage_!=SnubReloadStage::pending_completion&&!stopped)||!completion_taken_||
           !accepts(s,false)||!original_ready||ledger_.pending_call()==ReloadTxnCall::insert||
           !ledger_.acknowledge(receipt))return SnubReloadEvent::rejected;
        consume(s);stage_=ledger_.stage()==ReloadTxnStage::complete?SnubReloadStage::complete:
            SnubReloadStage::interrupted;
        return stage_==SnubReloadStage::complete?SnubReloadEvent::committed:SnubReloadEvent::interrupted;
    }
    SnubGuide guide() const noexcept {
        if(stage_==SnubReloadStage::old_magazine)return {snub_part(SnubPart::feed_port)};
        if(stage_==SnubReloadStage::take_magazine)
            return {snub_part(SnubPart::left_shoulder)};
        if(stage_==SnubReloadStage::holding_magazine)return {snub_part(SnubPart::feed_port)};
        if(stage_==SnubReloadStage::mechanism)return {plan_.steps[cursor_].parts};
        return {};
    }
    SnubReloadStage stage() const noexcept {return stage_;}
    std::uint8_t current_step() const noexcept {return cursor_;}
    std::uint32_t loaded() const noexcept {return ledger_.inventory().loaded;}
    std::uint32_t reserve() const noexcept {return ledger_.inventory().reserve;}
    std::uint32_t available_reserve() const noexcept {return ledger_.available_reserve();}
    std::uint32_t held() const noexcept {return ledger_.reserved();}
    std::uint32_t total() const noexcept {return loaded()+reserve();} // Reservation is not an extra round.
    bool feed_present() const noexcept {return ledger_.feed_present();}
    bool can_fire() const noexcept {return stage_==SnubReloadStage::complete&&ledger_.mechanical_ready()&&loaded()>0;}
    SnubReloadCondition condition() const noexcept {return plan_.condition;}
    SnubInventory inventory() const noexcept {return ledger_.inventory();}
    SnubReloadEvent outcome() const noexcept {return outcome_;}
private:
    // Inspection has no original reload serial, even following a prior attempt.
    std::uint64_t ledger_serial() const noexcept {return ledger_.target()?serial_:0;}
    bool valid_insertion(SnubReloadStamp const& s,SnubInsertionReceipt const& r,std::uint64_t since) const noexcept {
        return r.feed_mount_id==plan_.feed_mount_id&&r.aligned&&r.continuous&&r.seated&&
            r.left_hand_tracked&&r.left_grip_held&&r.right_hand_tracked&&
            valid_stroke(s,r.motion_begin_sequence,since);
    }
};
} // namespace gears_weapon
