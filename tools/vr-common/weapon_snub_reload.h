#pragma once
#include "body_slots.h"
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
    idle,mechanism,take_magazine,holding_magazine,pending_completion,complete,interrupted
};
enum class SnubReloadEvent : std::uint8_t {
    rejected,invalid,full,no_ammo,started,ejected,grabbed,released,inserted,
    mechanism_completed,perfect,ordinary,committed,interrupted,resumed,motion_reset
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
// The live adapter must verify this plan against the named gun's actual
// animation/state, separately for empty and partial reloads. Wings may be
// coupled to another part; a named bone does not imply a separate hand action.
struct SnubReloadPlan {
    SnubReloadIdentity identity{};
    std::uint64_t id{},feed_mount_id{};
    SnubReloadCondition condition{SnubReloadCondition::empty};
    std::array<SnubReloadStep,10> steps{};
    std::uint8_t count{};
    bool original_order_verified{},current_instance_verified{};
    bool valid(SnubReloadIdentity const& current,std::uint32_t loaded) const noexcept {
        if(!(identity==current)||!id||!feed_mount_id||!original_order_verified||
           !current_instance_verified||count<2||count>steps.size()||
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
                    if(!i||step.parts!=snub_part(SnubPart::feed_port))return false;
                    ++insertions;break;
                case SnubStepKind::move_original_parts:
                    if(!i||!step.parts||(step.parts&~moving)||!step.measured_travel_id)return false;
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
    std::int64_t end_ticks{}; // Relative to accepted R3 reload, not AR_Reload length.
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
};
struct SnubGuide {
    SnubPartMask parts{};
    float red{1},green{1},blue{0},alpha{1}; // Yellow render intent only.
};

// Pure Snub transaction, with evidence-gated phase receipts rather than guessed
// hand geometry. Local counts are proposed inventory, not game-memory writes.
// The adapter must suspend automatic reload completion, hold/resume original
// animation phases, authenticate every receipt, apply inventory and original
// reward/audio once, render guidance, and retain mechanical state across stow.
class SnubReload {
    SnubReloadStage stage_{SnubReloadStage::idle};
    SnubReloadIdentity identity_{};
    SnubReloadPlan plan_{};
    SnubPerfectWindow perfect_{};
    std::uint64_t r3_edge_{},serial_{},sequence_{},motion_epoch_{},step_since_{},
                  motion_since_{},held_item_{},last_item_{},grab_sequence_{};
    std::int64_t start_tick_{},tick_{},finish_tick_{};
    std::uint32_t capacity_{},loaded_{},reserve_{},held_{},target_{};
    std::uint8_t cursor_{};
    bool feed_present_{true},perfect_eligible_{},completion_taken_{};
    SnubReloadEvent outcome_{SnubReloadEvent::rejected};

    bool active() const noexcept {
        return stage_==SnubReloadStage::mechanism||stage_==SnubReloadStage::take_magazine||
               stage_==SnubReloadStage::holding_magazine;
    }
    bool accepts(SnubReloadStamp const& s,bool same_motion=true) const noexcept {
        return s.valid()&&s.identity==identity_&&s.sequence>sequence_&&s.game_tick>=tick_&&
            (same_motion?s.motion_epoch==motion_epoch_:s.motion_epoch>=motion_epoch_);
    }
    void consume(SnubReloadStamp const& s) noexcept {sequence_=s.sequence;tick_=s.game_tick;}
    SnubReloadEvent advance(SnubReloadStamp const& s,SnubReloadEvent event) noexcept {
        consume(s);step_since_=s.sequence;++cursor_;
        if(cursor_==plan_.count){
            finish_tick_=s.game_tick;
            outcome_=perfect_eligible_&&finish_tick_-start_tick_<=perfect_.end_ticks?
                SnubReloadEvent::perfect:SnubReloadEvent::ordinary;
            stage_=SnubReloadStage::pending_completion;return outcome_;
        }
        stage_=plan_.steps[cursor_].kind==SnubStepKind::insert_magazine?
            SnubReloadStage::take_magazine:SnubReloadStage::mechanism;
        return event;
    }
    bool valid_stroke(SnubReloadStamp const& s,std::uint64_t began,
                      std::uint64_t since) const noexcept {
        return accepts(s)&&began>=(std::max)(since,motion_since_)&&began<s.sequence;
    }
public:
    // Reset only after authoritative inventory/mechanical reconciliation.
    // Reset itself is neither an inventory refund nor a reward rollback.
    void reset() noexcept {
        const auto identity=identity_;
        const auto sequence=sequence_,serial=serial_,edge=r3_edge_;
        *this=SnubReload{};
        identity_=identity;sequence_=sequence;serial_=serial;r3_edge_=edge;
    }
    SnubReloadEvent begin_r3(SnubReloadStart const& s) noexcept {
        if(stage_!=SnubReloadStage::idle&&stage_!=SnubReloadStage::complete)
            return SnubReloadEvent::rejected;
        if(!s.stamp.valid()||!s.r3_press_edge||!s.original_reload_serial||
           !s.original_class_verified||!s.right_hand_equipped||!s.original_reload_accepted||
           s.original_inventory_slot!=0||!s.capacity||s.capacity>10000||
           s.loaded>s.capacity||s.reserve>1000000||!s.plan.valid(s.stamp.identity,s.loaded)||
           s.perfect.end_ticks<0||(s.perfect.current_instance_verified&&
           (!(s.perfect.identity==s.stamp.identity)||
            s.perfect.original_reload_serial!=s.original_reload_serial||s.perfect.end_ticks<=0)))
            return SnubReloadEvent::invalid;
        if(identity_.valid()&&(!(s.stamp.identity==identity_)||
           !accepts(s.stamp,false)||s.r3_press_edge<=r3_edge_||s.original_reload_serial<=serial_))
            return SnubReloadEvent::rejected;
        if(s.loaded==s.capacity)return SnubReloadEvent::full;
        if(!s.reserve)return SnubReloadEvent::no_ammo;
        identity_=s.stamp.identity;plan_=s.plan;perfect_=s.perfect;
        r3_edge_=s.r3_press_edge;serial_=s.original_reload_serial;
        capacity_=s.capacity;loaded_=s.loaded;reserve_=s.reserve;held_=target_=0;
        held_item_=last_item_=grab_sequence_=0;cursor_=0;
        start_tick_=s.stamp.game_tick;finish_tick_=0;
        motion_epoch_=s.stamp.motion_epoch;step_since_=motion_since_=s.stamp.sequence;
        feed_present_=true;perfect_eligible_=s.perfect.current_instance_verified;
        completion_taken_=false;outcome_=SnubReloadEvent::rejected;
        consume(s.stamp);stage_=SnubReloadStage::mechanism;return SnubReloadEvent::started;
    }
    SnubReloadEvent confirm_ejection(SnubReloadStamp const& s,
                                     SnubEjectionReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::mechanism||!accepts(s)||
           plan_.steps[cursor_].kind!=SnubStepKind::eject_magazine||
           r.original_reload_serial!=serial_||r.plan_id!=plan_.id||
           r.phase_id!=plan_.steps[cursor_].phase_id||!r.original_magazine_detached||
           !r.snub_magazine_binding_verified)return SnubReloadEvent::rejected;
        reserve_+=loaded_;loaded_=0;feed_present_=false;
        target_=(std::min)(capacity_,reserve_);
        return advance(s,SnubReloadEvent::ejected);
    }
    SnubReloadEvent grab_left_shoulder(SnubReloadStamp const& s,
                                       SnubMagazineReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::take_magazine||!accepts(s)||!r.item_serial||
           r.item_serial<=last_item_||r.source!=snub_ammo_slot||!r.snub_magazine_verified||
           !r.left_hand_tracked||!r.left_grip_held||!r.right_hand_tracked||!target_||
           reserve_<target_)return SnubReloadEvent::rejected;
        consume(s);held_item_=last_item_=r.item_serial;grab_sequence_=s.sequence;
        held_=target_;reserve_-=held_;stage_=SnubReloadStage::holding_magazine;
        return SnubReloadEvent::grabbed;
    }
    SnubReloadEvent release_magazine(SnubReloadStamp const& s,std::uint64_t item) noexcept {
        if(stage_!=SnubReloadStage::holding_magazine||!accepts(s)||!item||item!=held_item_)
            return SnubReloadEvent::rejected;
        consume(s);reserve_+=held_;held_=0;held_item_=0;
        stage_=SnubReloadStage::take_magazine;return SnubReloadEvent::released;
    }
    SnubReloadEvent insert_magazine(SnubReloadStamp const& s,
                                    SnubInsertionReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::holding_magazine||r.item_serial!=held_item_||
           r.feed_mount_id!=plan_.feed_mount_id||!r.aligned||!r.continuous||!r.seated||
           !r.left_hand_tracked||!r.left_grip_held||!r.right_hand_tracked||
           !valid_stroke(s,r.motion_begin_sequence,grab_sequence_))return SnubReloadEvent::rejected;
        loaded_+=held_;held_=0;held_item_=0;feed_present_=true;
        return advance(s,SnubReloadEvent::inserted);
    }
    SnubReloadEvent move_original_parts(SnubReloadStamp const& s,
                                        SnubPartMotionReceipt const& r) noexcept {
        if(stage_!=SnubReloadStage::mechanism)return SnubReloadEvent::rejected;
        auto const& step=plan_.steps[cursor_];
        if(step.kind!=SnubStepKind::move_original_parts||r.original_reload_serial!=serial_||
           r.plan_id!=plan_.id||r.phase_id!=step.phase_id||r.parts!=step.parts||
           r.measured_travel_id!=step.measured_travel_id||!r.endpoints_verified||
           !r.continuous||!r.player_driven||!r.left_hand_tracked||!r.left_grip_held||
           !r.right_hand_tracked||!valid_stroke(s,r.motion_begin_sequence,step_since_))
            return SnubReloadEvent::rejected;
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
        if(!active()&&stage_!=SnubReloadStage::pending_completion)return SnubReloadEvent::rejected;
        reserve_+=held_;held_=0;held_item_=0;perfect_eligible_=false;
        completion_taken_=true;outcome_=SnubReloadEvent::rejected;
        stage_=SnubReloadStage::interrupted;return SnubReloadEvent::interrupted;
    }
    // After stow, only this same original attempt can resume, with its exact
    // reconciled counts and phase. It retains seated ammo and remaining steps,
    // and can only finish ordinary. Death/world changes require reset instead.
    SnubReloadEvent resume(SnubReloadStamp const& s,std::uint64_t serial,
                           std::uint32_t loaded,std::uint32_t reserve,
                           bool original_phase_and_equipment_verified) noexcept {
        if(stage_!=SnubReloadStage::interrupted||!accepts(s,false)||serial!=serial_||
           loaded!=loaded_||reserve!=reserve_||!original_phase_and_equipment_verified||
           cursor_>=plan_.count)return SnubReloadEvent::rejected;
        consume(s);motion_epoch_=s.motion_epoch;step_since_=motion_since_=s.sequence;
        completion_taken_=false;
        stage_=plan_.steps[cursor_].kind==SnubStepKind::insert_magazine?
            SnubReloadStage::take_magazine:SnubReloadStage::mechanism;
        return SnubReloadEvent::resumed;
    }
    // Key external inventory/reward application by identity + original serial.
    // Taking this intent never proves the original reward has been applied.
    bool take_completion(SnubReloadCompletion& out) noexcept {
        if(stage_!=SnubReloadStage::pending_completion||completion_taken_)return false;
        completion_taken_=true;
        out={identity_,serial_,loaded_,reserve_,start_tick_,finish_tick_,outcome_};return true;
    }
    SnubReloadEvent acknowledge_completion(SnubReloadStamp const& s,std::uint64_t serial,
                                           std::uint32_t loaded,std::uint32_t reserve,
                                           bool original_ready,bool original_result_applied) noexcept {
        if(stage_!=SnubReloadStage::pending_completion||!completion_taken_||!accepts(s,false)||
           serial!=serial_||loaded!=loaded_||reserve!=reserve_||!original_ready||
           !original_result_applied)return SnubReloadEvent::rejected;
        consume(s);stage_=SnubReloadStage::complete;return SnubReloadEvent::committed;
    }
    SnubGuide guide() const noexcept {
        if(stage_==SnubReloadStage::take_magazine)
            return {snub_part(SnubPart::left_shoulder)};
        if(stage_==SnubReloadStage::holding_magazine)return {snub_part(SnubPart::feed_port)};
        if(stage_==SnubReloadStage::mechanism)return {plan_.steps[cursor_].parts};
        return {};
    }
    SnubReloadStage stage() const noexcept {return stage_;}
    std::uint8_t current_step() const noexcept {return cursor_;}
    std::uint32_t loaded() const noexcept {return loaded_;}
    std::uint32_t reserve() const noexcept {return reserve_;}
    std::uint32_t held() const noexcept {return held_;}
    std::uint32_t total() const noexcept {return loaded_+reserve_+held_;}
    bool feed_present() const noexcept {return feed_present_;}
    bool can_fire() const noexcept {return stage_==SnubReloadStage::complete&&feed_present_&&loaded_>0;}
};
} // namespace gears_weapon
