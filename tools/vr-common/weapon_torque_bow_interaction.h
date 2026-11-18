#pragma once
#include <array>
#include <cstdint>
#include <limits>

namespace gears_weapon {

// Saved UnrealScript declarations under analysis/re-kb/scripts/WarfareGame/:
// Weap_Bow_base.Charge.EndFire -> CheckFireShot -> Fire.BeginState, which calls
// FireAmmunition. Charge.Tick can also call CheckFireShot at the hold limit;
// CanReleaseFire may defer firing. These are static declarations, not a retail
// R2/abort test. No charge duration, reload order, ammo capacity or abort entry
// is inferred here. Tension's moving bow bones retain the original skinning
// (analysis/torque-bow-20261109/original-bow-animation-tracks.json, map SHA256
// 55f40ca44051413fe6db19816f97b09849c5e104319d93e9d5d159cbe6fb2840).
inline constexpr char torque_bow_original_class[]="Weap_Bow_base";

struct TorqueBowIdentity {
    std::uint64_t world{},pawn{},controller{},weapon{},world_generation{},
                  xr_session{},space_epoch{},input_epoch{};
    bool valid() const noexcept {
        return world&&pawn&&controller&&weapon&&world_generation&&
            xr_session&&space_epoch&&input_epoch;
    }
};
inline bool operator==(TorqueBowIdentity const& a,TorqueBowIdentity const& b) noexcept {
    return a.world==b.world&&a.pawn==b.pawn&&a.controller==b.controller&&
        a.weapon==b.weapon&&a.world_generation==b.world_generation&&
        a.xr_session==b.xr_session&&a.space_epoch==b.space_epoch&&a.input_epoch==b.input_epoch;
}
// One game-thread adapter issues strictly increasing sequences and monotonic
// timestamps for all commands and receipts. Game-clock ticks may pause.
struct TorqueBowStamp {
    TorqueBowIdentity identity{};
    std::uint64_t sequence{};
    std::int64_t monotonic_ns{},game_tick{};
    std::int64_t original_control_ns{}; // Optional authenticated raw edge; never a replacement clock.
};
struct TorqueBowInventory {
    std::uint64_t revision{};
    std::uint32_t loaded{},reserve{};
};
enum class TorqueBowReloadAction : std::uint8_t { insert_arrow,verified_mechanism };
struct TorqueBowReloadStep {
    TorqueBowReloadAction action{TorqueBowReloadAction::insert_arrow};
    std::uint64_t component{}; // Adapter-certified part of this exact instance.
};
struct TorqueBowConfig {
    TorqueBowIdentity identity{};
    std::uint64_t instance_config_serial{};
    std::uint32_t capacity{};
    // Adapter-certified game-clock units. Pause must not advance this clock.
    std::int64_t game_ticks_per_second{};
    std::int64_t maximum_pose_age_ns{100000000}; // VR lease, not original timing.
    bool original_class_verified{},original_charge_entry_verified{},
         original_end_fire_verified{},non_firing_abort_verified{},
         manual_reload_phase_verified{},reload_plan_verified{};
    std::uint32_t reload_step_count{};
    std::array<TorqueBowReloadStep,8> reload_steps{};
    bool valid() const noexcept {
        if(!identity.valid()||!instance_config_serial||!original_class_verified||
           capacity!=1||maximum_pose_age_ns<=0||maximum_pose_age_ns>1000000000)
            return false;
        if(!reload_plan_verified)return reload_step_count==0;
        if(!manual_reload_phase_verified||reload_step_count!=1||game_ticks_per_second<=0||
           game_ticks_per_second>std::numeric_limits<std::int64_t>::max()/4)
            return false;
        unsigned insertions=0;
        for(std::uint32_t i=0;i<reload_step_count;++i){
            auto const& step=reload_steps[i];
            if(!step.component)return false;
            if(step.action==TorqueBowReloadAction::insert_arrow)++insertions;
            else if(step.action!=TorqueBowReloadAction::verified_mechanism)return false;
        }
        // Bow reload ends at arrow lock; no invented bolt/string/mechanism.
        return insertions==1;
    }
};
struct TorqueBowOriginalState {
    bool ready{},charging{},pending_end_fire{},r2_held{},animation_owns_hands{};
    bool pending_fire{},firing{};
};
struct TorqueBowSample {
    TorqueBowStamp stamp{};
    TorqueBowInventory inventory{};
    TorqueBowOriginalState original{};
    bool right_tracked{},left_tracked{},right_weapon_held{},trigger_down{};
    // The unchanged XR sync clock, distinct from the GT command/receipt time.
    // Processing a frame late must not create another full pose lease.
    std::int64_t input_sync_ns{};
    std::int64_t continuous_from_sync_ns{}; // Adapter-verified full raw path from exact previous pose, not a fresh time.
};
struct TorqueBowReloadStart {
    TorqueBowStamp stamp{};
    std::uint64_t left_grip_edge{},original_reload_serial{};
    bool original_reload_accepted{},automatic_fill_suppressed{};
    bool fresh_left_grip{},left_shoulder_contact{};
    // Original Reloading.BeginState GAME time. The receipt may arrive on a
    // later GT frame; receipt latency must never restart the four-second clock.
    std::int64_t original_start_tick{-1};
};
enum class TorqueBowAmmoSource : std::uint8_t { other,left_shoulder };
struct TorqueBowArrowReceipt {
    std::uint64_t arrow_serial{};
    TorqueBowAmmoSource source{TorqueBowAmmoSource::other};
    bool matching_torque_bow_ammo{},tracked_left_hand{};
    std::uint64_t left_grip_edge{};
    bool fresh_left_grip{},left_shoulder_contact{};
};
// The adapter must recognize the configured part's complete continuous travel;
// a touch, animation duration, or a bone name alone cannot certify this receipt.
struct TorqueBowStepReceipt {
    std::uint64_t instance_config_serial{},original_reload_serial{},component{},
                  motion_begin_sequence{},arrow_serial{};
    bool geometry_verified{},aligned{},continuous{},complete{},tracked_left_hand{};
};
// edge_serial is an increasing adapter-issued R2 edge token, not a held level.
// continuous_input excludes an edge synthesized on tracking/session recovery.
struct TorqueBowTriggerEdge {
    std::uint64_t edge_serial{};
    bool right_tracked{},continuous_input{},down{};
};
enum class TorqueBowOutcome : std::uint8_t { none,ordinary,perfect };
enum class TorqueBowAction : std::uint8_t { none,begin_original_charge,end_original_fire,
                                          abort_original_charge,complete_original_reload };
enum class TorqueBowStage : std::uint8_t { unbound,ready,reload_steps,reload_commit,
    charge_begin,charging,charge_end,await_original,charge_abort,interrupted };
enum class TorqueBowEvent : std::uint8_t { rejected,observed,full,no_ammo,reload_started,
    arrow_grabbed,arrow_released,step_completed,request_ready,release_buffered,
    charge_started,release_accepted,abort_required,aborted,shot_recorded,
    safety_violation,ordinary,perfect,interrupted };
enum class TorqueBowGuidePart : std::uint8_t { none,left_shoulder,arrow_feed,verified_mechanism };
struct TorqueBowGuide {
    TorqueBowGuidePart part{TorqueBowGuidePart::none};
    std::uint64_t component{};
    float red{1},green{1},blue{0},alpha{1};
};
struct TorqueBowRequest {
    TorqueBowIdentity identity{};
    std::uint64_t token{},original_reload_serial{},arrow_serial{};
    TorqueBowAction action{TorqueBowAction::none};
    TorqueBowInventory before{},after{};
    TorqueBowOutcome outcome{TorqueBowOutcome::none};
    std::int64_t authored_ns{},valid_until_ns{},finished_game_tick{};
    bool requires_right_tracking{};
};
struct TorqueBowActionReceipt {
    TorqueBowStamp stamp{};
    std::uint64_t request_token{},original_event_serial{};
    bool accepted{};
    TorqueBowInventory inventory{};
    TorqueBowOriginalState original{};
    TorqueBowOutcome reload_result_applied{TorqueBowOutcome::none};
};
// Deliver real projectile events before action receipts/snapshots containing
// their ammo delta. An ammo decrement alone is never evidence of a projectile.
struct TorqueBowShotReceipt {
    TorqueBowStamp stamp{};
    std::uint64_t original_event_serial{},projectile_serial{};
    TorqueBowInventory inventory{};
    bool original_projectile_spawned{};
};

// Pure intent/receipt model, with no game calls, geometry recognizer, rendering
// or clock access. Config booleans are live-adapter certification contracts;
// there is currently NO proven Torque Bow non-firing abort, so production must
// leave it false and charging disabled. Holding forever is unsafe: the original
// has a hold-limit shot. A live adapter must execute a certified abort promptly,
// recheck identity/pose lease before begin/EndFire, and retain ownership until
// its receipt. Never implement abort as StopFire/EndFire/ButtonRelease('R2').
class TorqueBowInteraction {
    TorqueBowConfig config_{};
    TorqueBowInventory inventory_{};
    TorqueBowOriginalState original_{};
    TorqueBowStage stage_{TorqueBowStage::unbound};
    TorqueBowRequest pending_{};
    TorqueBowStamp buffered_release_{};
    std::uint64_t sequence_{},next_token_{1},last_original_event_{},last_projectile_{},
        last_loaded_token_{},last_cancel_edge_{},requested_cancel_edge_{},
        last_trigger_edge_{},last_r3_edge_{},last_left_grip_edge_{},reload_serial_{},last_arrow_{},arrow_{},
        step_begin_sequence_{},motion_floor_{};
    std::int64_t last_ns_{},last_tick_{},pose_ns_{},reload_start_tick_{};
    std::uint32_t step_{};
    bool bound_{},right_valid_{},left_valid_{},trigger_armed_{},dispatched_{},
         arrow_seated_{},lost_during_charge_{},shot_recorded_{},faulted_{},
         cancel_pending_{},cancel_wait_release_{};

    bool accepts(TorqueBowStamp const& s) const noexcept {
        return bound_&&s.identity==config_.identity&&s.sequence>sequence_&&
            s.monotonic_ns>last_ns_&&s.monotonic_ns>0&&s.game_tick>=0&&s.game_tick>=last_tick_;
    }
    void consume(TorqueBowStamp const& s) noexcept {
        sequence_=s.sequence;last_ns_=s.monotonic_ns;last_tick_=s.game_tick;
    }
    bool same_inventory(TorqueBowInventory const& i) const noexcept {
        return i.revision>=inventory_.revision&&i.loaded==inventory_.loaded&&i.reserve==inventory_.reserve;
    }
    bool fresh(std::int64_t ns,bool both=false) const noexcept {
        return right_valid_&&(!both||left_valid_)&&pose_ns_>0&&ns>=pose_ns_&&
            ns-pose_ns_<=config_.maximum_pose_age_ns;
    }
    bool original_quiet() const noexcept {
        return !original_.charging&&!original_.pending_end_fire&&!original_.pending_fire&&!original_.firing&&
            !original_.r2_held&&!original_.animation_owns_hands;
    }
    bool original_idle() const noexcept {return original_.ready&&original_quiet();}
    bool fire_attempt() const noexcept {
        return stage_==TorqueBowStage::charge_begin||stage_==TorqueBowStage::charging||
            stage_==TorqueBowStage::charge_end||stage_==TorqueBowStage::await_original||
            stage_==TorqueBowStage::charge_abort;
    }
    bool queue(TorqueBowAction action,TorqueBowStamp const& s) noexcept {
        if(pending_.token||next_token_==std::numeric_limits<std::uint64_t>::max()){
            faulted_=true;return false;
        }
        pending_={};pending_.identity=config_.identity;pending_.token=next_token_++;
        pending_.action=action;pending_.before=pending_.after=inventory_;
        pending_.authored_ns=s.monotonic_ns;pending_.finished_game_tick=s.game_tick;
        pending_.requires_right_tracking=action==TorqueBowAction::begin_original_charge||
            action==TorqueBowAction::end_original_fire;
        const auto limit=std::numeric_limits<std::int64_t>::max()-config_.maximum_pose_age_ns;
        pending_.valid_until_ns=s.monotonic_ns>limit?std::numeric_limits<std::int64_t>::max():
            s.monotonic_ns+config_.maximum_pose_age_ns;
        if(s.original_control_ns>0){
            const auto deadline=s.original_control_ns>limit?std::numeric_limits<std::int64_t>::max():
                s.original_control_ns+config_.maximum_pose_age_ns;
            if(deadline<pending_.valid_until_ns)pending_.valid_until_ns=deadline;
        }
        dispatched_=false;return true;
    }
    TorqueBowEvent request_abort() noexcept {
        cancel_pending_=true;
        buffered_release_={};trigger_armed_=false;
        if(pending_.token){
            if(dispatched_)return TorqueBowEvent::abort_required; // Reconcile the call first.
            if(pending_.action==TorqueBowAction::begin_original_charge){
                pending_={};cancel_pending_=false;stage_=faulted_?TorqueBowStage::interrupted:TorqueBowStage::ready;
                return TorqueBowEvent::aborted; // Nothing reached the game.
            }
            if(pending_.action==TorqueBowAction::abort_original_charge)return TorqueBowEvent::abort_required;
            pending_={};
        }
        if(!config_.non_firing_abort_verified){faulted_=true;stage_=TorqueBowStage::interrupted;
            return TorqueBowEvent::abort_required;}
        const TorqueBowStamp s{config_.identity,sequence_,last_ns_,last_tick_};
        if(!queue(TorqueBowAction::abort_original_charge,s))return TorqueBowEvent::interrupted;
        stage_=TorqueBowStage::charge_abort;return TorqueBowEvent::abort_required;
    }
    TorqueBowEvent finish_step(TorqueBowStamp const& s) noexcept {
        ++step_;step_begin_sequence_=s.sequence;
        if(step_<config_.reload_step_count)return TorqueBowEvent::step_completed;
        if(!arrow_seated_||!arrow_)return interrupt();
        if(!queue(TorqueBowAction::complete_original_reload,s))return interrupt();
        pending_.original_reload_serial=reload_serial_;pending_.arrow_serial=arrow_;
        ++pending_.after.loaded;--pending_.after.reserve;
        pending_.outcome=s.game_tick-reload_start_tick_<=4*config_.game_ticks_per_second?
                TorqueBowOutcome::perfect:TorqueBowOutcome::ordinary;
        stage_=TorqueBowStage::reload_commit;return TorqueBowEvent::request_ready;
    }
public:
    bool pose_current(std::int64_t now_ns,bool both=false) const noexcept {return bound_&&fresh(now_ns,both);}
    bool can_begin_reload(std::int64_t now_ns) const noexcept {
        return bound_&&!faulted_&&stage_==TorqueBowStage::ready&&!pending_.token&&config_.reload_plan_verified&&
            original_idle()&&fresh(now_ns,true)&&!inventory_.loaded&&inventory_.reserve;
    }
    // No reset that could silently forget an owned original charge or in-flight
    // inventory call. The owner must drain/reconcile requests before destruction.
    bool bind(TorqueBowConfig const& c,TorqueBowInventory const& i) noexcept {
        if(bound_||!c.valid()||!i.revision||i.loaded>c.capacity||i.reserve>1000000)return false;
        config_=c;inventory_=i;bound_=true;stage_=TorqueBowStage::ready;return true;
    }
    TorqueBowStage stage() const noexcept {return stage_;}
    TorqueBowIdentity identity() const noexcept {return config_.identity;}
    std::uint64_t held_arrow_serial() const noexcept {return held_arrows()?arrow_:0;}
    std::uint64_t instance_config_serial() const noexcept {return config_.instance_config_serial;}
    bool has_fire_attempt() const noexcept {return fire_attempt();}
    bool original_shot_recorded() const noexcept {return shot_recorded_;}
    // Passive presentation receipts. These never authorize an action and are
    // not inferred from proximity, an attempted call or disappearing Charge.
    std::uint64_t last_original_projectile_serial() const noexcept {return last_projectile_;}
    std::uint64_t last_loaded_token() const noexcept {return last_loaded_token_;}
    std::uint64_t last_confirmed_cancel_edge() const noexcept {return last_cancel_edge_;}
    TorqueBowInventory inventory() const noexcept {return inventory_;}
    std::uint32_t available_reserve() const noexcept {return inventory_.reserve-(arrow_?1u:0u);}
    std::uint32_t held_arrows() const noexcept {return arrow_&&!arrow_seated_?1u:0u;}
    std::uint32_t seated_uncommitted_arrows() const noexcept {return arrow_seated_?1u:0u;}
    std::uint32_t total() const noexcept {
        return inventory_.loaded+available_reserve()+held_arrows()+seated_uncommitted_arrows();
    }
    bool has_pending_request() const noexcept {return pending_.token!=0;}
    TorqueBowGuide guide() const noexcept {
        TorqueBowGuide g{};
        if(stage_!=TorqueBowStage::reload_steps||faulted_)return g;
        auto const& step=config_.reload_steps[step_];g.component=step.component;
        if(step.action==TorqueBowReloadAction::verified_mechanism)g.part=TorqueBowGuidePart::verified_mechanism;
        else g.part=arrow_?TorqueBowGuidePart::arrow_feed:TorqueBowGuidePart::left_shoulder;
        return g;
    }
    bool can_request_fire(std::int64_t ns,TorqueBowInventory const& observed) const noexcept {
        return bound_&&!faulted_&&stage_==TorqueBowStage::ready&&!pending_.token&&
            config_.original_charge_entry_verified&&config_.original_end_fire_verified&&
            config_.non_firing_abort_verified&&fresh(ns)&&original_idle()&&trigger_armed_&&
            inventory_.loaded&&observed.revision==inventory_.revision&&same_inventory(observed);
    }
    // Calling twice never submits twice. The live adapter must revalidate the
    // request's identity and original state, and the pose lease for fire calls,
    // immediately before execution. An already dispatched call always needs a
    // receipt, including rejection; an expired undispatched fire call is cancelled.
    bool take_request(TorqueBowRequest& out,std::int64_t now_ns) noexcept {
        if(!pending_.token||dispatched_)return false;
        if(now_ns<pending_.authored_ns)return false;
        if(pending_.requires_right_tracking&&(!fresh(now_ns)||
           now_ns>pending_.valid_until_ns||lost_during_charge_)){
            tracking_lost();return false;
        }
        if(pending_.action==TorqueBowAction::begin_original_charge&&!original_idle()){
            pending_={};trigger_armed_=false;stage_=TorqueBowStage::ready;return false;
        }
        out=pending_;dispatched_=true;return true;
    }
    TorqueBowEvent tracking_lost() noexcept {
        right_valid_=left_valid_=trigger_armed_=false;motion_floor_=sequence_;
        if(stage_==TorqueBowStage::reload_steps&&!arrow_seated_)arrow_=0;
        if(!fire_attempt())return TorqueBowEvent::observed;
        lost_during_charge_=true;return request_abort();
    }
    // Poll even when the producer publishes no frames. No timeout generates an
    // EndFire; an original timed charge must be cancelled through safe abort.
    TorqueBowEvent expire_pose(std::int64_t now_ns) noexcept {
        if(!bound_||!pose_ns_||now_ns<pose_ns_)return TorqueBowEvent::rejected;
        return now_ns-pose_ns_>config_.maximum_pose_age_ns?
            tracking_lost():TorqueBowEvent::observed;
    }
    TorqueBowEvent interrupt() noexcept {
        faulted_=true;right_valid_=left_valid_=trigger_armed_=false;motion_floor_=sequence_;
        if(fire_attempt()){lost_during_charge_=true;return request_abort();}
        if(stage_!=TorqueBowStage::reload_commit){arrow_=0;arrow_seated_=false;}
        if(!pending_.token)stage_=TorqueBowStage::interrupted;
        return TorqueBowEvent::interrupted;
    }
    TorqueBowEvent observe(TorqueBowSample const& s) noexcept {
        if(!accepts(s.stamp)){
            if(bound_&&!(s.stamp.identity==config_.identity))return interrupt();
            tracking_lost();return TorqueBowEvent::rejected;
        }
        const bool clock_valid=s.input_sync_ns>0&&s.input_sync_ns<=s.stamp.monotonic_ns&&
            s.stamp.monotonic_ns-s.input_sync_ns<=config_.maximum_pose_age_ns&&
            (!pose_ns_||s.input_sync_ns>=pose_ns_);
        const bool proven_path=pose_ns_>0&&s.continuous_from_sync_ns==pose_ns_&&s.input_sync_ns>pose_ns_;
        const bool gap=pose_ns_&&(!clock_valid||
            (s.input_sync_ns-pose_ns_>config_.maximum_pose_age_ns&&!proven_path));
        consume(s.stamp);
        if(!same_inventory(s.inventory))return interrupt();
        inventory_.revision=s.inventory.revision;original_=s.original;
        const bool restoring=gap||!right_valid_||!left_valid_;
        right_valid_=s.right_tracked&&s.right_weapon_held&&clock_valid;
        left_valid_=s.left_tracked&&clock_valid;pose_ns_=clock_valid?s.input_sync_ns:0;
        if(original_.animation_owns_hands)return interrupt();
        if(!right_valid_)return tracking_lost();
        if(restoring||!left_valid_)motion_floor_=s.stamp.sequence;
        if(!left_valid_&&stage_==TorqueBowStage::reload_steps&&!arrow_seated_)arrow_=0;
        if(gap&&fire_attempt()){lost_during_charge_=true;return request_abort();}
        if(stage_==TorqueBowStage::await_original&&!pending_.token&&original_idle())
            stage_=faulted_?TorqueBowStage::interrupted:TorqueBowStage::ready;
        if(stage_==TorqueBowStage::ready&&!s.trigger_down&&!cancel_wait_release_)trigger_armed_=true;
        return TorqueBowEvent::observed;
    }
    TorqueBowEvent begin_left_shoulder(TorqueBowReloadStart const& s,
                                      TorqueBowArrowReceipt const& a) noexcept {
        if(!accepts(s.stamp)||faulted_||stage_!=TorqueBowStage::ready||pending_.token||
           !fresh(s.stamp.monotonic_ns,true)||!original_quiet()||
           !config_.reload_plan_verified||!s.left_grip_edge||s.left_grip_edge<=last_left_grip_edge_||
           !s.original_reload_serial||s.original_reload_serial<=reload_serial_||
           !s.original_reload_accepted||!s.automatic_fill_suppressed||
           s.original_start_tick<0||s.original_start_tick>s.stamp.game_tick||
           !s.fresh_left_grip||!s.left_shoulder_contact||!a.arrow_serial||
           a.arrow_serial<=last_arrow_||!a.tracked_left_hand||
           a.source!=TorqueBowAmmoSource::left_shoulder||!a.matching_torque_bow_ammo||
           a.left_grip_edge!=s.left_grip_edge||!a.fresh_left_grip||!a.left_shoulder_contact)
            return TorqueBowEvent::rejected;
        // The accepted original is already in Reloading, so it need not still
        // report Active/ready. Its certified acceptance and suppressed refill
        // are required above; Charge/pending input/animation still reject it.
        consume(s.stamp);last_left_grip_edge_=s.left_grip_edge;
        if(inventory_.loaded==config_.capacity)return TorqueBowEvent::full;
        if(!inventory_.reserve)return TorqueBowEvent::no_ammo;
        reload_serial_=s.original_reload_serial;
        reload_start_tick_=s.original_start_tick;step_=0;arrow_=last_arrow_=a.arrow_serial;arrow_seated_=false;
        step_begin_sequence_=motion_floor_=s.stamp.sequence;trigger_armed_=false;
        stage_=TorqueBowStage::reload_steps;return TorqueBowEvent::reload_started;
    }
    // Fresh R3 is consumed exclusively by cancellation, including an accepted
    // EndFire waiting on Pawn. Never route this edge to reload as well.
    TorqueBowEvent cancel_charge(TorqueBowStamp const& s,TorqueBowTriggerEdge const& e) noexcept {
        if(!accepts(s)||!e.edge_serial||e.edge_serial<=last_r3_edge_||!e.down||
           !e.right_tracked||!e.continuous_input||!fresh(s.monotonic_ns)||
           !fire_attempt()||stage_==TorqueBowStage::charge_abort||shot_recorded_||original_.firing||cancel_pending_)
            return TorqueBowEvent::rejected;
        consume(s);last_r3_edge_=e.edge_serial;requested_cancel_edge_=e.edge_serial;cancel_wait_release_=true;
        return request_abort();
    }
    TorqueBowEvent grab_left_shoulder(TorqueBowStamp const& s,TorqueBowArrowReceipt const& a) noexcept {
        if(!accepts(s)||faulted_||stage_!=TorqueBowStage::reload_steps||arrow_||
           config_.reload_steps[step_].action!=TorqueBowReloadAction::insert_arrow||
           !fresh(s.monotonic_ns,true)||!a.arrow_serial||a.arrow_serial<=last_arrow_||
           !a.tracked_left_hand||a.source!=TorqueBowAmmoSource::left_shoulder||
           !a.matching_torque_bow_ammo||!available_reserve()||!a.fresh_left_grip||
           !a.left_shoulder_contact||a.left_grip_edge<=last_left_grip_edge_)return TorqueBowEvent::rejected;
        consume(s);last_left_grip_edge_=a.left_grip_edge;arrow_=last_arrow_=a.arrow_serial;step_begin_sequence_=s.sequence;
        return TorqueBowEvent::arrow_grabbed;
    }
    TorqueBowEvent release_arrow(TorqueBowStamp const& s,std::uint64_t serial) noexcept {
        if(!accepts(s)||stage_!=TorqueBowStage::reload_steps||arrow_seated_||!arrow_||arrow_!=serial)
            return TorqueBowEvent::rejected;
        consume(s);arrow_=0;step_begin_sequence_=s.sequence;return TorqueBowEvent::arrow_released;
    }
    TorqueBowEvent complete_step(TorqueBowStamp const& s,TorqueBowStepReceipt const& r) noexcept {
        if(!accepts(s)||faulted_||stage_!=TorqueBowStage::reload_steps||!fresh(s.monotonic_ns,true)||
           r.instance_config_serial!=config_.instance_config_serial||r.original_reload_serial!=reload_serial_||
           r.component!=config_.reload_steps[step_].component||!r.geometry_verified||!r.aligned||
           !r.continuous||!r.complete||!r.tracked_left_hand||r.motion_begin_sequence<step_begin_sequence_||
           r.motion_begin_sequence<motion_floor_||r.motion_begin_sequence>=s.sequence)
            return TorqueBowEvent::rejected;
        if(config_.reload_steps[step_].action==TorqueBowReloadAction::insert_arrow){
            if(!arrow_||arrow_seated_||r.arrow_serial!=arrow_)return TorqueBowEvent::rejected;
            arrow_seated_=true;
        }else if(r.arrow_serial)return TorqueBowEvent::rejected;
        consume(s);return finish_step(s);
    }
    TorqueBowEvent trigger(TorqueBowStamp const& s,TorqueBowTriggerEdge const& e) noexcept {
        if(!accepts(s)||faulted_||!e.edge_serial||e.edge_serial<=last_trigger_edge_)
            return TorqueBowEvent::rejected;
        if(s.original_control_ns<0||s.original_control_ns>s.monotonic_ns||
           (s.original_control_ns&&s.monotonic_ns-s.original_control_ns>config_.maximum_pose_age_ns)){
            consume(s);return tracking_lost();
        }
        if(!fresh(s.monotonic_ns)||!e.right_tracked||!e.continuous_input){
            consume(s);return tracking_lost();
        }
        if(e.down){
            if(!can_request_fire(s.monotonic_ns,inventory_))return TorqueBowEvent::rejected;
            consume(s);last_trigger_edge_=e.edge_serial;trigger_armed_=false;
            lost_during_charge_=shot_recorded_=cancel_pending_=false;requested_cancel_edge_=0;buffered_release_={};
            if(!queue(TorqueBowAction::begin_original_charge,s))return interrupt();
            stage_=TorqueBowStage::charge_begin;return TorqueBowEvent::request_ready;
        }
        if(cancel_wait_release_){
            consume(s);last_trigger_edge_=e.edge_serial;cancel_wait_release_=false;
            if(stage_==TorqueBowStage::ready)trigger_armed_=true;
            return TorqueBowEvent::observed;
        }
        // Original fatigue may already have spawned the arrow while the real
        // trigger remained down. Its later actual release still owns the R2
        // key hand-back; it must not leave that key stuck or become a new shot.
        if(stage_==TorqueBowStage::await_original&&shot_recorded_&&original_.r2_held&&!pending_.token){
            consume(s);last_trigger_edge_=e.edge_serial;
            if(!queue(TorqueBowAction::end_original_fire,s))return interrupt();
            stage_=TorqueBowStage::charge_end;return TorqueBowEvent::request_ready;
        }
        if((stage_!=TorqueBowStage::charging&&stage_!=TorqueBowStage::charge_begin)||
           lost_during_charge_||buffered_release_.sequence)return TorqueBowEvent::rejected;
        consume(s);last_trigger_edge_=e.edge_serial;
        if(stage_==TorqueBowStage::charge_begin){buffered_release_=s;return TorqueBowEvent::release_buffered;}
        if(!queue(TorqueBowAction::end_original_fire,s))return interrupt();
        stage_=TorqueBowStage::charge_end;return TorqueBowEvent::request_ready;
    }
    // A genuinely observed tracked UP level can rearm after cancellation or
    // recovery without inventing a release edge. It never submits EndFire.
    TorqueBowEvent observe_trigger_up(TorqueBowStamp const& s,bool tracked_up) noexcept {
        if(!accepts(s)||!tracked_up||!fresh(s.monotonic_ns)||faulted_||
           stage_!=TorqueBowStage::ready||pending_.token||!original_idle())return TorqueBowEvent::rejected;
        consume(s);cancel_wait_release_=false;trigger_armed_=true;return TorqueBowEvent::observed;
    }
    // A real projectile receipt is required even if EndFire was accepted. This
    // also records the ORIGINAL hold-limit shot without inventing a VR timer.
    TorqueBowEvent record_original_shot(TorqueBowShotReceipt const& r) noexcept {
        if(!accepts(r.stamp)||!fire_attempt()||
           (stage_==TorqueBowStage::charge_begin&&(!dispatched_||pending_.action!=TorqueBowAction::begin_original_charge))||shot_recorded_||
           !r.original_projectile_spawned||r.original_event_serial<=last_original_event_||
           !r.projectile_serial||r.projectile_serial<=last_projectile_)return TorqueBowEvent::rejected;
        if(!inventory_.loaded||r.inventory.revision<=inventory_.revision||
           r.inventory.loaded!=inventory_.loaded-1||r.inventory.reserve!=inventory_.reserve)
            return interrupt();
        consume(r.stamp);inventory_=r.inventory;last_original_event_=r.original_event_serial;
        last_projectile_=r.projectile_serial;shot_recorded_=true;original_.ready=false;
        if(lost_during_charge_)faulted_=true;
        if(!dispatched_&&pending_.action==TorqueBowAction::end_original_fire)pending_={};
        if(!pending_.token)stage_=TorqueBowStage::await_original;
        return lost_during_charge_?TorqueBowEvent::safety_violation:TorqueBowEvent::shot_recorded;
    }
    TorqueBowEvent acknowledge(TorqueBowActionReceipt const& r) noexcept {
        if(!accepts(r.stamp)||!pending_.token||!dispatched_||r.request_token!=pending_.token||
           !r.original_event_serial||r.original_event_serial<=last_original_event_)
            return TorqueBowEvent::rejected;
        const auto request=pending_;
        const bool reload=request.action==TorqueBowAction::complete_original_reload;
        if((!reload||!r.accepted)&&r.reload_result_applied!=TorqueBowOutcome::none){
            faulted_=true;return TorqueBowEvent::interrupted;
        }
        if(r.accepted&&reload){
            if(r.inventory.revision<=request.before.revision||
               r.inventory.loaded!=request.after.loaded||r.inventory.reserve!=request.after.reserve||
               r.reload_result_applied!=request.outcome){faulted_=true;return TorqueBowEvent::interrupted;}
        }else if(!same_inventory(r.inventory)){faulted_=true;return TorqueBowEvent::interrupted;}
        consume(r.stamp);last_original_event_=r.original_event_serial;
        inventory_=r.inventory;original_=r.original;pending_={};dispatched_=false;
        if(!r.accepted){
            if(reload){arrow_=0;arrow_seated_=false;faulted_=true;stage_=TorqueBowStage::interrupted;
                return TorqueBowEvent::interrupted;}
            if(request.action==TorqueBowAction::abort_original_charge){faulted_=true;
                stage_=TorqueBowStage::interrupted;return TorqueBowEvent::abort_required;}
            if(original_.charging||original_.pending_end_fire||original_.r2_held)return request_abort();
            stage_=faulted_?TorqueBowStage::interrupted:TorqueBowStage::ready;
            trigger_armed_=false;return TorqueBowEvent::rejected;
        }
        if(reload){
            last_loaded_token_=request.token;
            arrow_=0;arrow_seated_=false;trigger_armed_=false;original_.ready=false;
            stage_=faulted_?TorqueBowStage::interrupted:TorqueBowStage::ready;
            return request.outcome==TorqueBowOutcome::perfect?TorqueBowEvent::perfect:TorqueBowEvent::ordinary;
        }
        if(request.action==TorqueBowAction::begin_original_charge){
            if(!original_.charging||!original_.r2_held||original_.pending_end_fire){
                faulted_=true;return request_abort();}
            stage_=TorqueBowStage::charging;
            if(!fresh(r.stamp.monotonic_ns))lost_during_charge_=true;
            if(lost_during_charge_||faulted_||cancel_pending_)return request_abort();
            if(buffered_release_.sequence){
                const auto release=buffered_release_;buffered_release_={};
                if(!fresh(r.stamp.monotonic_ns)||r.stamp.monotonic_ns-release.monotonic_ns>
                   config_.maximum_pose_age_ns){lost_during_charge_=true;return request_abort();}
                if(!queue(TorqueBowAction::end_original_fire,release))return interrupt();
                stage_=TorqueBowStage::charge_end;return TorqueBowEvent::request_ready;
            }
            return TorqueBowEvent::charge_started;
        }
        if(request.action==TorqueBowAction::end_original_fire){
            if(original_.r2_held){faulted_=true;return request_abort();}
            stage_=TorqueBowStage::await_original;
            if(cancel_pending_||(lost_during_charge_&&(original_.charging||original_.pending_end_fire)))return request_abort();
            return TorqueBowEvent::release_accepted;
        }
        if(original_.charging||original_.pending_end_fire||original_.pending_fire||original_.firing||original_.r2_held){
            faulted_=true;stage_=TorqueBowStage::interrupted;return TorqueBowEvent::abort_required;}
        if(requested_cancel_edge_&&!faulted_&&!lost_during_charge_&&!shot_recorded_)
            last_cancel_edge_=requested_cancel_edge_;
        requested_cancel_edge_=0;cancel_pending_=false;trigger_armed_=false;original_.ready=false;
        stage_=faulted_?TorqueBowStage::interrupted:TorqueBowStage::ready;
        return faulted_?TorqueBowEvent::safety_violation:TorqueBowEvent::aborted;
    }
};
}
