#pragma once
#include <array>
#include <cstdint>
#include <limits>

namespace gears_weapon {
// Saved LS_Reload/LS_Reload_Fail evidence has 50/160 b_LS_Bolt rotation
// keys (analysis/longshot-animation-tracks-verified.json, WarfareGame.u
// SHA256 91c0f56e9302db2b3991fc3d8646abc1eaff2b33b8b021ce8df436d8cd681d13).
// This proves neither live mechanical order nor hand travel. Saved ammo work
// identifies Longshot's bullet_spent_single as an EMPTY case, not a live round.
// There is deliberately no default step order, mount, cartridge or deadline.
inline constexpr char longshot_reload_class[]="Weap_SniperRifle";
inline constexpr std::int64_t longshot_perfect_duration_us=4000000;
enum class LongshotAction : std::uint8_t {unknown,open_breech,eject_case,take_round,insert_round,close_breech};
enum class LongshotReloadStage : std::uint8_t {idle,step,inventory_commit,original_completion,complete,interrupted};
enum class LongshotReloadEvent : std::uint8_t {
    none,rejected,invalid,full,no_ammo,started,step_completed,round_grabbed,round_released,
    inventory_requested,inventory_committed,completion_requested,perfect,ordinary,interrupted
};
struct LongshotReloadIdentity {
    std::uint64_t world{},pawn{},weapon{},world_generation{},session{},space_epoch{},
        input_epoch{},profile_generation{},binding_generation{};
    bool valid() const noexcept {
        return world&&pawn&&weapon&&world_generation&&session&&space_epoch&&
            input_epoch&&profile_generation&&binding_generation;
    }
    bool operator==(LongshotReloadIdentity const& b) const noexcept {
        return world==b.world&&pawn==b.pawn&&weapon==b.weapon&&world_generation==b.world_generation&&
            session==b.session&&space_epoch==b.space_epoch&&input_epoch==b.input_epoch&&
            profile_generation==b.profile_generation&&binding_generation==b.binding_generation;
    }
};
struct LongshotReloadStamp {
    LongshotReloadIdentity identity{};
    std::uint64_t sequence{};
    std::int64_t observed_ns{},game_tick{};
    bool right_tracked{},left_tracked{},weapon_owned{};
    bool valid(std::int64_t now_ns) const noexcept {
        return identity.valid()&&sequence&&game_tick>=0&&observed_ns>0&&
            now_ns>=observed_ns&&now_ns-observed_ns<=100000000;
    }
};
struct LongshotReloadStep {
    LongshotAction action{LongshotAction::unknown};
    std::uint64_t part{},calibration{},before_state{},after_state{};
    bool needs_left_hand{};
};
struct LongshotReloadPlan {
    LongshotReloadIdentity identity{};
    std::uint64_t generation{},original_reload_serial{},initial_state{},final_ready_state{};
    std::array<LongshotReloadStep,8> steps{};
    unsigned count{};
    bool current_instance_verified{};
    bool valid() const noexcept {
        if(!identity.valid()||!generation||!original_reload_serial||!initial_state||
           !final_ready_state||!current_instance_verified||count!=5)return false;
        // Reuse the preserved W04 semantic plan. Physical paths/state ids are
        // still supplied by the authenticated current-instance calibration.
        constexpr LongshotAction order[]={LongshotAction::open_breech,LongshotAction::eject_case,
            LongshotAction::take_round,LongshotAction::insert_round,LongshotAction::close_breech};
        if(steps[0].part!=steps[4].part||steps[0].calibration!=steps[4].calibration)return false;
        unsigned grabs=0,inserts=0;auto state=initial_state;
        for(unsigned i=0;i<count;++i){
            const auto s=steps[i];
            if(s.action!=order[i]||s.needs_left_hand!=(i!=1))return false;
            if(!s.before_state||!s.after_state||s.before_state!=state)return false;
            switch(s.action){
                case LongshotAction::take_round:if(++grabs!=1||inserts)return false;break;
                case LongshotAction::insert_round:
                    if(grabs!=1||++inserts!=1||!s.part||!s.calibration)return false;break;
                case LongshotAction::open_breech:case LongshotAction::eject_case:
                case LongshotAction::close_breech:
                    if(!s.part||!s.calibration||s.before_state==s.after_state)return false;break;
                default:return false;
            }
            state=s.after_state;
        }
        return grabs==1&&inserts==1&&state==final_ready_state;
    }
};
struct LongshotInventory {
    LongshotReloadIdentity identity{};
    std::uint64_t revision{};
    std::uint32_t loaded{},reserve{}; // Capacity is verified as ONE at begin.
};
struct LongshotShot {
    LongshotReloadIdentity identity{};
    std::uint64_t original_event_sequence{};
    std::int64_t game_tick{},ticks_per_second{};
    bool confirmed{}; // Original successful fire receipt, never a trigger edge.
    bool valid(LongshotReloadStamp const& start) const noexcept {
        return confirmed&&identity==start.identity&&original_event_sequence&&
            game_tick>=0&&game_tick<=start.game_tick&&ticks_per_second>0&&
            ticks_per_second<=std::numeric_limits<std::int64_t>::max()/4;
    }
};
struct LongshotReloadStart {
    LongshotReloadStamp stamp{};
    LongshotReloadPlan plan{};
    LongshotInventory inventory{};
    LongshotShot shot{};
    std::uint64_t physical_start_edge{};
    bool original_class_verified{},single_round_capacity_verified{},
        original_reload_accepted{},automatic_refill_held{},inventory_reconciled{};
};
enum class LongshotAmmoSource : std::uint8_t {other,left_shoulder};
enum class LongshotAmmoKind : std::uint8_t {other,spent_case,live_longshot_round};
struct LongshotRoundReceipt {
    std::uint64_t item_serial{};
    LongshotAmmoSource source{LongshotAmmoSource::other};
    LongshotAmmoKind kind{LongshotAmmoKind::other};
    bool live_round_identity_verified{};
};
// Adapter contract: continuous calibrated 6DoF contact/travel, NOT animation
// elapsed time or the presence of b_LS_Bolt. Recognition remains external.
struct LongshotStepReceipt {
    std::uint64_t plan_generation{},original_reload_serial{},part{},calibration{},
        motion_begin_sequence{},before_state{},after_state{},item_serial{};
    unsigned step_index{};
    LongshotAction action{LongshotAction::unknown};
    bool measured_travel{},continuous{},complete_travel{},aligned{};
};
struct LongshotInventoryRequest {
    LongshotReloadIdentity identity{};
    std::uint64_t token{},original_reload_serial{},item_serial{},before_revision{};
    std::uint32_t before_loaded{},before_reserve{},after_loaded{},after_reserve{};
};
struct LongshotInventoryReceipt {
    std::uint64_t token{},original_reload_serial{};
    bool accepted{};
    LongshotInventory observed{};
};
struct LongshotCompletionRequest {
    LongshotReloadIdentity identity{};
    std::uint64_t token{},original_reload_serial{},plan_generation{},inventory_revision{},final_ready_state{};
    std::int64_t started_tick{},finished_tick{};
    LongshotReloadEvent proposed_result{LongshotReloadEvent::ordinary};
};
struct LongshotCompletionReceipt {
    std::uint64_t token{},original_reload_serial{},plan_generation{},final_ready_state{},original_event_sequence{};
    bool accepted{},original_ready{};
    LongshotInventory observed{};
};
enum class LongshotGuideRegion : std::uint8_t {none,left_shoulder,weapon_part};
struct LongshotReloadGuide {
    LongshotGuideRegion region{LongshotGuideRegion::none};
    LongshotAction action{LongshotAction::unknown};
    std::uint64_t part{};
    float red{1},green{1},blue{0},alpha{1}; // Current step only, yellow intent.
};

// Pure single-round transaction. Original inventory changes only on an exact
// acknowledged insertion commit. Final reward/handoff is another once-only
// request; requests are not proof of game-side success. Fire, damage, spread,
// recoil, audio, animations and native ready rules remain owned by the game.
class LongshotReload {
    LongshotReloadIdentity identity_{};
    LongshotReloadPlan plan_{};
    LongshotInventory inventory_{};
    LongshotShot shot_{};
    LongshotInventoryRequest inventory_request_{};
    LongshotCompletionRequest completion_request_{};
    LongshotReloadStage stage_{LongshotReloadStage::idle};
    unsigned step_{};
    std::uint64_t next_token_{1},last_sequence_{},last_edge_{},last_reload_{},
        step_enter_sequence_{},held_item_{},last_item_{},last_original_event_{},completed_shot_{};
    std::int64_t last_ns_{},last_tick_{},started_tick_{},physical_finish_tick_{};
    bool inventory_dispatched_{},completion_dispatched_{};
    LongshotReloadEvent outcome_{LongshotReloadEvent::rejected};

    bool accepts(LongshotReloadStamp const& s,std::int64_t now) const noexcept {
        return s.valid(now)&&s.identity==identity_&&s.sequence>last_sequence_&&
            s.observed_ns>=last_ns_&&s.game_tick>=last_tick_;
    }
    void consume(LongshotReloadStamp const& s) noexcept {
        last_sequence_=s.sequence;last_ns_=s.observed_ns;last_tick_=s.game_tick;
    }
    bool active() const noexcept {
        return stage_==LongshotReloadStage::step||stage_==LongshotReloadStage::inventory_commit||
            stage_==LongshotReloadStage::original_completion;
    }
    bool same_inventory(LongshotInventory const& i) const noexcept {
        return i.identity==identity_&&i.revision>=inventory_.revision&&
            i.loaded==inventory_.loaded&&i.reserve==inventory_.reserve;
    }
    std::uint64_t token() noexcept {
        if(next_token_==std::numeric_limits<std::uint64_t>::max())return 0;
        return next_token_++;
    }
    LongshotReloadEvent advance(LongshotReloadStamp const& stamp) noexcept {
        ++step_;step_enter_sequence_=stamp.sequence;
        if(step_<plan_.count)return LongshotReloadEvent::step_completed;
        const auto request_token=token();if(!request_token)return interrupt();
        const bool perfect=shot_.confirmed&&physical_finish_tick_>=started_tick_&&
            physical_finish_tick_-started_tick_<4*shot_.ticks_per_second;
        outcome_=perfect?LongshotReloadEvent::perfect:LongshotReloadEvent::ordinary;
        completion_request_={identity_,request_token,plan_.original_reload_serial,plan_.generation,
            inventory_.revision,plan_.final_ready_state,started_tick_,physical_finish_tick_,outcome_};
        completion_dispatched_=false;stage_=LongshotReloadStage::original_completion;
        return LongshotReloadEvent::completion_requested;
    }
    bool receipt_matches(LongshotReloadStamp const& s,LongshotStepReceipt const& r,
        std::int64_t now) const noexcept {
        if(stage_!=LongshotReloadStage::step||step_>=plan_.count||!accepts(s,now))return false;
        const auto p=plan_.steps[step_];
        return r.plan_generation==plan_.generation&&r.original_reload_serial==plan_.original_reload_serial&&
            r.step_index==step_&&r.action==p.action&&r.part==p.part&&r.calibration==p.calibration&&
            r.before_state==p.before_state&&r.after_state==p.after_state&&r.measured_travel&&
            r.continuous&&r.complete_travel&&r.motion_begin_sequence>=step_enter_sequence_&&
            r.motion_begin_sequence<s.sequence;
    }
public:
    LongshotReloadEvent begin_manual(LongshotReloadStart const& start,std::int64_t now_ns) noexcept {
        const auto& s=start.stamp;const auto& p=start.plan;const auto& i=start.inventory;
        if(active())return LongshotReloadEvent::rejected;
        if(!s.valid(now_ns)||!s.right_tracked||!s.weapon_owned||!p.valid()||!(p.identity==s.identity)||
           !(i.identity==s.identity)||!i.revision||i.loaded>1||i.reserve>1000000||!start.physical_start_edge||
           !start.original_class_verified||!start.single_round_capacity_verified||!start.original_reload_accepted||
           !start.automatic_refill_held||!start.inventory_reconciled||
           p.steps[p.count-1].action!=LongshotAction::close_breech)
            return LongshotReloadEvent::invalid;
        if(start.shot.confirmed&&!start.shot.valid(s))return LongshotReloadEvent::invalid;
        const bool same=identity_.valid()&&identity_==s.identity;
        if(identity_.valid()&&((same&&(!accepts(s,now_ns)||start.physical_start_edge<=last_edge_||
            p.original_reload_serial<=last_reload_))||(!same&&s.identity.binding_generation<=identity_.binding_generation)))
            return LongshotReloadEvent::rejected;
        if(same&&(i.revision<inventory_.revision||(i.revision==inventory_.revision&&
           (i.loaded!=inventory_.loaded||i.reserve!=inventory_.reserve))))return LongshotReloadEvent::invalid;
        if(i.loaded)return LongshotReloadEvent::full;
        if(!i.reserve)return LongshotReloadEvent::no_ammo;
        if(same&&start.shot.confirmed&&start.shot.original_event_sequence<=completed_shot_)
            return LongshotReloadEvent::rejected;
        if(same&&shot_.confirmed&&start.shot.confirmed&&
           (start.shot.original_event_sequence<shot_.original_event_sequence||
            (start.shot.original_event_sequence==shot_.original_event_sequence&&
             (start.shot.game_tick!=shot_.game_tick||start.shot.ticks_per_second!=shot_.ticks_per_second))))
            return LongshotReloadEvent::invalid;
        identity_=s.identity;plan_=p;inventory_=i;shot_=start.shot;
        last_edge_=start.physical_start_edge;last_reload_=p.original_reload_serial;
        if(!same){last_original_event_=0;completed_shot_=0;}
        consume(s);started_tick_=shot_.confirmed?shot_.game_tick:s.game_tick;
        physical_finish_tick_=s.game_tick;step_enter_sequence_=s.sequence;
        held_item_=0;step_=0;inventory_request_={};completion_request_={};
        inventory_dispatched_=completion_dispatched_=false;outcome_=LongshotReloadEvent::rejected;
        stage_=LongshotReloadStage::step;return LongshotReloadEvent::started;
    }
    // Called with live samples even without hand actions. A loss of tracking or
    // ownership ends the attempt; it never freezes the original game clock.
    LongshotReloadEvent observe(LongshotReloadStamp const& s,std::int64_t now_ns) noexcept {
        if(!active())return LongshotReloadEvent::rejected;
        if(!s.valid(now_ns)||!(s.identity==identity_))return interrupt();
        if(!accepts(s,now_ns))return LongshotReloadEvent::rejected;
        consume(s);
        if(!s.weapon_owned||!s.right_tracked||!s.left_tracked)return interrupt();
        return LongshotReloadEvent::none;
    }
    LongshotReloadEvent grab_left_shoulder(LongshotReloadStamp const& s,
        LongshotRoundReceipt const& round,std::int64_t now_ns) noexcept {
        if(stage_!=LongshotReloadStage::step||plan_.steps[step_].action!=LongshotAction::take_round||
           !accepts(s,now_ns))return LongshotReloadEvent::rejected;
        if(!s.weapon_owned||!s.right_tracked||!s.left_tracked)return interrupt();
        if(!round.item_serial||round.item_serial<=last_item_||!round.live_round_identity_verified||
           round.source!=LongshotAmmoSource::left_shoulder||round.kind!=LongshotAmmoKind::live_longshot_round||
           !inventory_.reserve||held_item_)return LongshotReloadEvent::rejected;
        consume(s);held_item_=last_item_=round.item_serial;physical_finish_tick_=s.game_tick;
        const auto result=advance(s);
        return result==LongshotReloadEvent::step_completed?LongshotReloadEvent::round_grabbed:result;
    }
    LongshotReloadEvent release_round(LongshotReloadStamp const& s,std::uint64_t item,
        std::int64_t now_ns) noexcept {
        if(stage_!=LongshotReloadStage::step||!accepts(s,now_ns)||!item||item!=held_item_)
            return LongshotReloadEvent::rejected;
        if(!s.weapon_owned||!s.right_tracked||!s.left_tracked)return interrupt();
        consume(s);interrupt();
        // Return the reservation without guessing how to undo any interleaved
        // mechanical steps. A fresh accepted R3/plan must reconcile that state.
        return LongshotReloadEvent::round_released;
    }
    LongshotReloadEvent complete_step(LongshotReloadStamp const& s,LongshotStepReceipt const& r,
        std::int64_t now_ns) noexcept {
        if(!receipt_matches(s,r,now_ns)||r.action==LongshotAction::take_round)
            return LongshotReloadEvent::rejected;
        const auto step=plan_.steps[step_];
        if(!s.weapon_owned||!s.right_tracked||((step.needs_left_hand||r.action==LongshotAction::insert_round)&&
            !s.left_tracked))return interrupt();
        if(r.action==LongshotAction::insert_round&&(!held_item_||r.item_serial!=held_item_||!r.aligned))
            return LongshotReloadEvent::rejected;
        consume(s);physical_finish_tick_=s.game_tick;
        if(r.action!=LongshotAction::insert_round)return advance(s);
        const auto request_token=token();if(!request_token)return interrupt();
        inventory_request_={identity_,request_token,plan_.original_reload_serial,held_item_,inventory_.revision,
            inventory_.loaded,inventory_.reserve,1,inventory_.reserve-1};
        inventory_dispatched_=false;stage_=LongshotReloadStage::inventory_commit;
        return LongshotReloadEvent::inventory_requested;
    }
    bool take_inventory_request(LongshotInventoryRequest& out) noexcept {
        if(stage_!=LongshotReloadStage::inventory_commit||inventory_dispatched_)return false;
        out=inventory_request_;inventory_dispatched_=true;return true;
    }
    LongshotReloadEvent acknowledge_inventory(LongshotReloadStamp const& s,
        LongshotInventoryReceipt const& receipt,std::int64_t now_ns) noexcept {
        if(stage_!=LongshotReloadStage::inventory_commit||!inventory_dispatched_||!accepts(s,now_ns)||
           receipt.token!=inventory_request_.token||receipt.original_reload_serial!=plan_.original_reload_serial||
           !(receipt.observed.identity==identity_))return LongshotReloadEvent::rejected;
        if(!s.weapon_owned||!s.right_tracked||!s.left_tracked)return interrupt();
        if(!receipt.accepted){
            if(!same_inventory(receipt.observed))return interrupt();
            consume(s);inventory_=receipt.observed;inventory_request_={};stage_=LongshotReloadStage::step;
            step_enter_sequence_=s.sequence;return LongshotReloadEvent::rejected;
        }
        if(receipt.observed.revision<=inventory_request_.before_revision||
           receipt.observed.loaded!=inventory_request_.after_loaded||
           receipt.observed.reserve!=inventory_request_.after_reserve)return interrupt();
        consume(s);inventory_=receipt.observed;held_item_=0;inventory_request_={};
        stage_=LongshotReloadStage::step;
        const auto result=advance(s);
        return result==LongshotReloadEvent::step_completed?LongshotReloadEvent::inventory_committed:result;
    }
    bool take_completion_request(LongshotCompletionRequest& out) noexcept {
        if(stage_!=LongshotReloadStage::original_completion||completion_dispatched_)return false;
        out=completion_request_;completion_dispatched_=true;return true;
    }
    LongshotReloadEvent acknowledge_completion(LongshotReloadStamp const& s,
        LongshotCompletionReceipt const& receipt,std::int64_t now_ns) noexcept {
        if(stage_!=LongshotReloadStage::original_completion||!completion_dispatched_||!accepts(s,now_ns)||
           receipt.token!=completion_request_.token||receipt.original_reload_serial!=plan_.original_reload_serial||
           receipt.plan_generation!=plan_.generation||!receipt.original_event_sequence||
           receipt.original_event_sequence<=last_original_event_||!(receipt.observed.identity==identity_))
            return LongshotReloadEvent::rejected;
        if(!s.weapon_owned||!s.right_tracked||!s.left_tracked||!receipt.accepted||!receipt.original_ready||
           receipt.final_ready_state!=plan_.final_ready_state||!same_inventory(receipt.observed))return interrupt();
        consume(s);inventory_=receipt.observed;last_original_event_=receipt.original_event_sequence;
        if(shot_.confirmed)completed_shot_=shot_.original_event_sequence;
        completion_request_={};stage_=LongshotReloadStage::complete;return outcome_;
    }
    LongshotReloadEvent interrupt() noexcept {
        if(!active())return LongshotReloadEvent::rejected;
        // A held visual round releases its reservation; no speculative game
        // inventory rollback. Outstanding native commits need reconciliation.
        held_item_=0;inventory_request_={};completion_request_={};
        outcome_=LongshotReloadEvent::rejected;stage_=LongshotReloadStage::interrupted;
        return LongshotReloadEvent::interrupted;
    }
    LongshotReloadGuide guide() const noexcept {
        if(stage_!=LongshotReloadStage::step||step_>=plan_.count)return {};
        const auto step=plan_.steps[step_];
        return {step.action==LongshotAction::take_round?LongshotGuideRegion::left_shoulder:
            LongshotGuideRegion::weapon_part,step.action,step.part};
    }
    LongshotReloadStage stage() const noexcept {return stage_;}
    unsigned step_index() const noexcept {return step_;}
    LongshotInventory inventory() const noexcept {return inventory_;}
    std::uint64_t held_round() const noexcept {return held_item_;}
    std::uint32_t reserved_rounds() const noexcept {return held_item_?1u:0u;}
    std::uint32_t available_reserve() const noexcept {return inventory_.reserve-reserved_rounds();}
    bool vr_reload_gate_clear() const noexcept {return stage_==LongshotReloadStage::complete;}
    bool needs_inventory_reconciliation() const noexcept {return stage_==LongshotReloadStage::interrupted;}
};
} // namespace gears_weapon
