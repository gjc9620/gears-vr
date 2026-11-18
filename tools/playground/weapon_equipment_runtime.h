#pragma once
#include "../vr-common/weapon_hand_participant.h"
#include "../vr-common/weapon_equipment_model.h"
#include "../vr-common/equipment_intent_bridge.h"
#include <cstdint>
#include <limits>
#include <type_traits>

// PG PostPlayerTick only. These callbacks may read inventory/call the original
// game; none belongs in Native prepare/commit/observe/NoAuth callbacks. The
// backend authenticates the actual GT and the exact current publication.
namespace gears_equipment_runtime {
using Frame=gears_weapon_hand::Frame;
using ObjectIdentity=gears_weapon_hand::ObjectIdentity;
using EquipmentIntentEvent=gears_weapon::EquipmentIntentEvent;
using EquipmentSnapshot=gears_weapon::EquipmentSnapshot;
using WeaponEquipmentModel=gears_weapon::WeaponEquipmentModel;

struct Backend {
    void* context{};
    std::uint64_t (*clock_ms)(void*) noexcept{};
    std::int64_t (*clock_ns)(void*) noexcept{};
    bool (*current)(void*,Frame const&) noexcept{};
    bool (*next)(void*,Frame const&,std::uint64_t&,EquipmentIntentEvent&) noexcept{};
    bool (*inventory)(void*,Frame const&,EquipmentSnapshot&) noexcept{};
    bool (*attached)(void*,Frame const&,EquipmentSnapshot const&) noexcept{};
    bool (*identify)(void*,std::uint64_t,ObjectIdentity&) noexcept{};
    bool (*switch_slot)(void*,Frame const&,EquipmentSnapshot const&,unsigned) noexcept{};
    bool (*empty_sound)(void*,Frame const&,EquipmentSnapshot const&,unsigned) noexcept{};
    bool (*hold)(void*,Frame const&,EquipmentSnapshot const&) noexcept{};
    void (*clear_hold)(void*) noexcept{};
    void (*reset_input)(void*) noexcept{};
    bool (*held_inventory)(void*,Frame const&,EquipmentSnapshot&) noexcept{};
    // Optional integrated handoff. These callbacks authenticate the original
    // current request and verify their actual engine result, never a trial
    // arena actor. A changed attachment needs a NEW publication before hold.
    bool (*stow)(void*,Frame const&,EquipmentSnapshot const&) noexcept{};
    bool (*unholster)(void*,Frame const&,EquipmentSnapshot const&,bool& changed) noexcept{};
    void (*idle_unheld)(void*,Frame const&,EquipmentSnapshot const&) noexcept{};
    // An accepted stow may first need to park a committed manual reload.
    // changed consumes that publication; dispatch requires a later current
    // one within the ORIGINAL intent deadline, exactly like switch handoff.
    bool (*prepare_stow)(void*,Frame const&,bool& changed) noexcept{};
    // Optional PG-local index acquisition, never accepted from the legacy
    // Native bridge. Authenticate the supported target (or empty slot) and
    // seal the existing firing ledger before changing any attachment.
    bool (*begin_index_acquire)(void*,Frame const&,EquipmentSnapshot const&,unsigned) noexcept{};
    // Optional independently revalidated EXISTING idle grasp only. An unknown
    // observation cannot authorize a new event/selection/hold or snapshot.
    // Failure/rejection and all non-idle paths retain normal revocation.
    bool (*retain_idle_hold)(void*,Frame const&) noexcept{};
    // Observation only after this tick has consumed a quiet input sample.
    // Must include current slots/actors and switch/animation flags; no ammo
    // selection data. Runtime returns before event/selection/hold consumers.
    bool (*quiet_unheld_inventory)(void*,Frame const&,EquipmentSnapshot&) noexcept{};
    // Optional fresh proof of the SAME published hold after baseline inventory
    // loss or an invalid/no-event input gap. It grants no event or action.
    bool (*retain_existing_hold)(void*,Frame const&) noexcept{};
    // May invoke original stop calls. stop_issued is true only for a confirmed
    // original stop for this HOD hold, allowing a bounded completion wait.
    bool (*stow_effects_ready)(void*,Frame const&,bool& stop_issued) noexcept{};
};
static_assert(std::is_standard_layout_v<Backend>&&std::is_trivially_copyable_v<Backend>);

enum class Status : std::uint8_t {
    idle,baseline_waiting,baseline,unavailable,invalid_backend,reentrant,
    event_ignored,invalid_event,expired_event,future_event,duplicate_press,
    busy,empty_slot,empty_sound_failed,unsupported_stow,attachment_unavailable,
    identity_mismatch,inventory_unavailable,switch_requested,switch_failed,
    switch_pending,switch_timed_out,switch_interrupted,held,hold_failed,
    stowed,stow_failed,unholster_pending,unholster_failed,clock_invalid,stow_pending
};
inline char const* status_name(Status s) noexcept {
    switch(s){
#define GEARS_EQUIPMENT_STATUS(name) case Status::name:return #name
        GEARS_EQUIPMENT_STATUS(idle);GEARS_EQUIPMENT_STATUS(baseline_waiting);
        GEARS_EQUIPMENT_STATUS(baseline);GEARS_EQUIPMENT_STATUS(unavailable);
        GEARS_EQUIPMENT_STATUS(invalid_backend);GEARS_EQUIPMENT_STATUS(reentrant);
        GEARS_EQUIPMENT_STATUS(event_ignored);GEARS_EQUIPMENT_STATUS(invalid_event);
        GEARS_EQUIPMENT_STATUS(expired_event);GEARS_EQUIPMENT_STATUS(future_event);
        GEARS_EQUIPMENT_STATUS(duplicate_press);GEARS_EQUIPMENT_STATUS(busy);
        GEARS_EQUIPMENT_STATUS(empty_slot);GEARS_EQUIPMENT_STATUS(empty_sound_failed);
        GEARS_EQUIPMENT_STATUS(unsupported_stow);GEARS_EQUIPMENT_STATUS(attachment_unavailable);
        GEARS_EQUIPMENT_STATUS(identity_mismatch);GEARS_EQUIPMENT_STATUS(inventory_unavailable);
        GEARS_EQUIPMENT_STATUS(switch_requested);GEARS_EQUIPMENT_STATUS(switch_failed);
        GEARS_EQUIPMENT_STATUS(switch_pending);GEARS_EQUIPMENT_STATUS(switch_timed_out);
        GEARS_EQUIPMENT_STATUS(switch_interrupted);GEARS_EQUIPMENT_STATUS(held);
        GEARS_EQUIPMENT_STATUS(hold_failed);GEARS_EQUIPMENT_STATUS(clock_invalid);
        GEARS_EQUIPMENT_STATUS(stowed);GEARS_EQUIPMENT_STATUS(stow_failed);
        GEARS_EQUIPMENT_STATUS(stow_pending);
        GEARS_EQUIPMENT_STATUS(unholster_pending);GEARS_EQUIPMENT_STATUS(unholster_failed);
#undef GEARS_EQUIPMENT_STATUS
    }
    return "invalid_status";
}
struct Outcome {
    Status status{Status::idle};unsigned slot{4};
    std::uint64_t target{},event_serial{},press_counter{};
    unsigned input_control{gears_vr::control_squeeze};
};
struct ActorKey {
    std::uint64_t game_start{},native_started_qpc{},registration_cookie{},world_generation{};
    std::uint32_t game_pid{},thread_id{};
    ObjectIdentity world{},controller{},pawn{},mesh{};
};
struct InputKey {
    std::uint64_t lifecycle{},session{},space_epoch{},profile_generation{},input_epoch{};
    gears_vr::StereoSource source{};
};
struct State {
    ActorKey actor{};InputKey input{};
    bool actor_bound{},input_bound{},baseline_ready{},future_known{},pending{},pending_unholster{},pending_dispatch{},pending_stow{},held_known{},hold_published{},clock_known{};
    std::uint64_t cursor{UINT64_MAX},last_input_sequence{},consumed_press{},last_seen_press{},last_ms{};
    std::uint64_t consumed_index_press{},last_seen_index_press{};
    EquipmentIntentEvent future{}; // At most one original event, never retimestamped.
    EquipmentIntentEvent pending_intent{};
    std::uint64_t accepted_stow_hold{},accepted_stow_trigger_press{};
    ObjectIdentity target{},held_identity{};
    std::uint32_t last_edges[2][2][gears_vr::control_count]{};
    bool edges_known{}; // Optimization eligibility only, never an input grant.
    unsigned pending_slot{4};
    unsigned pending_control{gears_vr::control_squeeze};
    std::uint64_t deadline_ms{},pending_event_serial{},pending_press{},pending_frame{};
};

class Runtime {
    State state_{};
    WeaponEquipmentModel handoff_proposed_{}; // Software intent, never an action/pose permit.
    bool ticking_{},suspended_in_tick_{};
    // Comparison metadata only after an independent live hold proof. No
    // Frame, pose, input event or successful proof is reused as authority.
    struct RetainedCohort {
        ActorKey actor{};InputKey input{};ObjectIdentity weapon{};
        std::uint64_t generation{},sequence{};bool known{};
    } retained_cohort_{};
    static bool complete(Backend const& b) noexcept {
        return b.clock_ms&&b.clock_ns&&b.current&&b.next&&b.inventory&&b.attached&&b.identify&&
            b.switch_slot&&b.empty_sound&&b.hold&&b.clear_hold&&b.reset_input;
    }
    static ActorKey actor(Frame const& f) noexcept {
        return {f.key.game_start,f.key.native_started_qpc,f.key.registration_cookie,f.key.world_generation,
            f.key.game_pid,f.key.thread_id,f.world,f.controller,f.pawn,f.mesh};
    }
    static bool same(ActorKey const& a,ActorKey const& b) noexcept {
        using gears_weapon_hand::same;
        return a.game_start==b.game_start&&a.native_started_qpc==b.native_started_qpc&&
            a.registration_cookie==b.registration_cookie&&a.world_generation==b.world_generation&&
            a.game_pid==b.game_pid&&a.thread_id==b.thread_id&&same(a.world,b.world)&&
            same(a.controller,b.controller)&&same(a.pawn,b.pawn)&&same(a.mesh,b.mesh);
    }
    static InputKey input(Frame const& f) noexcept {
        auto const& c=f.controllers;
        return {f.key.lifecycle,c.session,c.space_epoch,c.profile_generation,c.input_epoch,c.source};
    }
    static bool same(InputKey const& a,InputKey const& b) noexcept {
        return a.lifecycle==b.lifecycle&&a.session==b.session&&a.space_epoch==b.space_epoch&&
            a.profile_generation==b.profile_generation&&a.input_epoch==b.input_epoch&&a.source==b.source;
    }
    bool current(Frame const& f,Backend const& b) const noexcept {
        // Check the ORIGINAL received_ns values again after any backend work;
        // no copy, poll or receipt may renew their source lifetime.
        return !suspended_in_tick_&&b.current(b.context,f)&&!suspended_in_tick_&&
            gears_weapon_hand::frame_active(f,b.clock_ns(b.context));
    }
    bool clock(Backend const& b,std::uint64_t& now) noexcept {
        now=b.clock_ms(b.context);
        if(state_.clock_known&&now<state_.last_ms)return false;
        state_.clock_known=true;state_.last_ms=now;return true;
    }
    void clear_binding(Backend const& b) noexcept {
        retained_cohort_={};
        state_.hold_published=false;
        if(b.clear_hold)b.clear_hold(b.context);
    }
    void input_boundary(Backend const& b) noexcept {
        if(state_.accepted_stow_hold)clear_pending();
        retained_cohort_={};
        state_.edges_known=false;
        state_.input={};state_.input_bound=state_.baseline_ready=state_.future_known=false;
        state_.cursor=UINT64_MAX;state_.last_input_sequence=state_.consumed_press=state_.last_seen_press=0;
        state_.consumed_index_press=state_.last_seen_index_press=0;
        state_.future={};if(b.reset_input)b.reset_input(b.context);
    }
    void clear_pending() noexcept {
        state_.pending=state_.pending_unholster=state_.pending_dispatch=state_.pending_stow=false;state_.target={};state_.pending_slot=4;
        handoff_proposed_={};state_.pending_intent={};
        state_.accepted_stow_hold=state_.accepted_stow_trigger_press=0;
        state_.deadline_ms=state_.pending_event_serial=state_.pending_press=state_.pending_frame=0;
        state_.pending_control=gears_vr::control_squeeze;
    }
    void clear_model(Backend const& b,WeaponEquipmentModel& model) noexcept {
        clear_binding(b);model.reset();clear_pending();state_.held_known=false;state_.held_identity={};
    }
    Outcome pending_out(Status status) const noexcept {
        return {status,state_.pending_slot,state_.target.address,state_.pending_event_serial,state_.pending_press,state_.pending_control};
    }
    Outcome lost(Backend const& b) noexcept {
        const auto out=pending_out(Status::unavailable);suspend(b);return out;
    }
    // Called only AFTER an independent existing-hold proof succeeded.
    // Preserve comparison metadata, never the old proof or input baseline.
    void suspend_proven_hold(Frame const& f,Backend const& b,
        WeaponEquipmentModel const& model,std::uint64_t generation) noexcept {
        RetainedCohort cohort{};
        if(b.retain_existing_hold&&state_.actor_bound&&same(state_.actor,actor(f))&&
           state_.input_bound&&state_.baseline_ready&&same(state_.input,input(f))&&
           state_.held_known&&state_.hold_published&&
           gears_weapon_hand::same(state_.held_identity,f.weapon)&&
           model.hold_generation()==generation&&
           model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
           !state_.pending&&!state_.future_known&&!model.switch_pending()&&
           !model.animation_owns_hands())
            cohort={actor(f),input(f),f.weapon,generation,f.controllers.sequence,true};
        suspend_input(b);
        if(cohort.known&&state_.hold_published&&model.hold_generation()==generation&&
           gears_weapon_hand::same(state_.held_identity,f.weapon))retained_cohort_=cohort;
    }

    bool retain_existing_hold(Frame const& f,Backend const& b,
        WeaponEquipmentModel const& model) noexcept {
        if(suspended_in_tick_||!b.retain_existing_hold||
           !state_.held_known||!state_.hold_published||
           !state_.actor_bound||!state_.input_bound||!state_.baseline_ready||
           !same(state_.actor,actor(f))||!same(state_.input,input(f))||
           !gears_weapon_hand::same(state_.held_identity,f.weapon)||
           !model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
           state_.pending||state_.future_known||model.switch_pending()||
           model.animation_owns_hands())return false;
        const auto generation=model.hold_generation();
        const bool retained=b.retain_existing_hold(b.context,f);
        if(retained&&!suspended_in_tick_&&state_.held_known&&
           state_.hold_published&&model.hold_generation()==generation&&
           gears_weapon_hand::same(state_.held_identity,f.weapon)&&
           model.held_for(f.world.address,f.pawn.address,f.weapon.address)){
            suspend_proven_hold(f,b,model,generation);return true;
        }
        return false;
    }

    Outcome lost_entry(Frame const& f,Backend const& b,WeaponEquipmentModel const& model) noexcept {
        const auto owner=actor(f);
        const auto generation=model.hold_generation();
        // A previous successful independent retention discarded ALL input.
        // Only compare its cohort here, then obtain another live proof. The
        // rejected Frame remains unavailable and cannot dispatch an action.
        const auto cohort=retained_cohort_;
        if(!suspended_in_tick_&&b.retain_existing_hold&&cohort.known&&
           state_.actor_bound&&same(state_.actor,owner)&&same(cohort.actor,owner)&&
           !state_.input_bound&&!state_.baseline_ready&&state_.cursor==UINT64_MAX&&
           same(cohort.input,input(f))&&f.controllers.sequence>=cohort.sequence&&
           state_.held_known&&state_.hold_published&&
           gears_weapon_hand::same(cohort.weapon,f.weapon)&&
           gears_weapon_hand::same(state_.held_identity,f.weapon)&&
           model.hold_generation()==cohort.generation&&
           model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
           !state_.pending&&!state_.future_known&&!model.switch_pending()&&
           !model.animation_owns_hands()){
            const bool retained=b.retain_existing_hold(b.context,f);
            if(retained&&!suspended_in_tick_&&state_.held_known&&state_.hold_published&&
               model.hold_generation()==cohort.generation&&
               gears_weapon_hand::same(state_.held_identity,cohort.weapon)&&
               model.held_for(f.world.address,f.pawn.address,f.weapon.address)){
                auto next=cohort;next.sequence=f.controllers.sequence;
                const auto out=pending_out(Status::unavailable);
                suspend_input(b);
                if(state_.hold_published&&model.hold_generation()==cohort.generation&&
                   gears_weapon_hand::same(state_.held_identity,cohort.weapon))retained_cohort_=next;
                return out;
            }
        }
        const auto& right=f.controllers.hands[1];
        bool edges_match=state_.edges_known;
        for(unsigned side=0;side<2;++side)for(unsigned control=0;control<gears_vr::control_count;++control)
            edges_match=edges_match&&state_.last_edges[side][0][control]==f.controllers.hands[side].pressed[control]&&
                state_.last_edges[side][1][control]==f.controllers.hands[side].released[control];
        if(!suspended_in_tick_&&b.retain_idle_hold&&state_.actor_bound&&same(state_.actor,owner)&&
           state_.input_bound&&same(state_.input,input(f))&&state_.baseline_ready&&edges_match&&
           f.controllers.sequence>=state_.last_input_sequence&&
           right.pressed[gears_vr::control_squeeze]==state_.last_seen_press&&
           right.pressed[gears_vr::control_trigger]==state_.last_seen_index_press&&
           !state_.pending&&!state_.future_known&&!model.switch_pending()&&!model.animation_owns_hands()&&
           state_.held_known&&state_.hold_published&&
           gears_weapon_hand::same(state_.held_identity,f.weapon)&&
           model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
           b.retain_idle_hold(b.context,f)&&!suspended_in_tick_&&state_.held_known&&
           state_.hold_published&&model.hold_generation()==generation&&
           gears_weapon_hand::same(state_.held_identity,f.weapon)){
            const auto out=pending_out(Status::unavailable);
            suspend_proven_hold(f,b,model,generation);return out;
        }
        return lost(b);
    }
    Outcome lost_observed(Frame const& f,Backend const& b,WeaponEquipmentModel const& model,bool eligible) noexcept {
        // Only a previously published, still-held observation may use the
        // caller's independent live-hold proof after current() has failed.
        // The stale Frame remains rejected; only inventory input is dropped.
        const auto generation=model.hold_generation();
        if(eligible&&!suspended_in_tick_&&b.retain_idle_hold&&state_.held_known&&
           gears_weapon_hand::same(state_.held_identity,f.weapon)&&
           model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
           b.retain_idle_hold(b.context,f)&&!suspended_in_tick_&&state_.held_known&&
           state_.hold_published&&model.hold_generation()==generation&&
           gears_weapon_hand::same(state_.held_identity,f.weapon)&&
           model.held_for(f.world.address,f.pawn.address,f.weapon.address)){
            const auto out=pending_out(Status::unavailable);
            suspend_proven_hold(f,b,model,generation);return out;
        }
        return lost(b);
    }
    Outcome bad_clock(Backend const& b,WeaponEquipmentModel& model) noexcept {
        const auto out=pending_out(Status::clock_invalid);clear_model(b,model);suspend(b);return out;
    }
    bool identify(Backend const& b,std::uint64_t address,ObjectIdentity& out) const noexcept {
        return address&&b.identify(b.context,address,out)&&out.address==address&&gears_weapon_hand::identity_valid(out);
    }
    static bool event_tuple(EquipmentIntentEvent const& e,Frame const& f) noexcept {
        return e.serial&&e.input_sequence&&f.weapon.address&&
            e.world_generation==f.key.world_generation&&e.controller==f.controller.address&&
            e.pawn==f.pawn.address&&e.weapon==f.weapon.address&&
            e.input_session==f.controllers.session&&e.space_epoch==f.controllers.space_epoch&&
            e.input_epoch==f.controllers.input_epoch;
    }
    static bool fresh(EquipmentIntentEvent const& e,std::uint64_t now) noexcept {
        return now>=e.created_ms&&now-e.created_ms<=250;
    }
    static gears_weapon::BodySlot body(unsigned slot) noexcept {
        constexpr gears_weapon::BodySlot slots[]={gears_weapon::BodySlot::pistol,gears_weapon::BodySlot::grenade,
            gears_weapon::BodySlot::weapon_2,gears_weapon::BodySlot::weapon_1};
        return slot<4?slots[slot]:gears_weapon::BodySlot::none;
    }
    Outcome republish_hold(Frame const& f,Backend const& b,EquipmentSnapshot const& game,WeaponEquipmentModel& model,Outcome out) noexcept {
        // Only the already-held branch calls this after authenticating the
        // same actor/object identity and actual attachment on a fresh Frame.
        // A publication failure revokes live authority, not that private
        // established grasp. New acquisitions still use commit_hold below.
        const bool accepted=b.hold(b.context,f,game);
        if(!accepted||!current(f,b)){
            suspend(b);out.status=accepted?Status::unavailable:Status::hold_failed;return out;
        }
        std::uint64_t now{};if(!clock(b,now))return bad_clock(b,model);
        state_.hold_published=true;return out;
    }
    Outcome commit_hold(Frame const& f,Backend const& b,EquipmentSnapshot const& game,
                        WeaponEquipmentModel& model,WeaponEquipmentModel const& proposed,
                        ObjectIdentity const& target,Outcome out) noexcept {
        model=proposed;
        const bool accepted=b.hold(b.context,f,game);
        if(!accepted||!current(f,b)){
            clear_model(b,model);
            if(!accepted){out.status=Status::hold_failed;return out;}
            suspend(b);out.status=Status::unavailable;return out;
        }
        std::uint64_t now{};
        if(!clock(b,now))return bad_clock(b,model);
        if(state_.pending&&now>=state_.deadline_ms){
            clear_model(b,model);out.status=Status::switch_timed_out;return out;
        }
        clear_pending();state_.held_identity=target;state_.held_known=state_.hold_published=true;
        out.status=Status::held;return out;
    }
public:
    Runtime()=default;Runtime(Runtime const&)=delete;Runtime& operator=(Runtime const&)=delete;
    State const& state() const noexcept {return state_;}
    // A known original action may temporarily block inventory actions without
    // stopping observation of the sole input stream. Preserve at most one
    // exact fresh event in the EXISTING deferred-event slot; neither its
    // creation stamp nor the250ms action deadline is renewed. Normal tick
    // must still revalidate the tuple, age, inventory and original switch gate.
    // This method never selects/stows/holds an item or calls an engine action.
    template<class RetainExistingHold>
    bool observe_input_while_blocked(Frame const& f,Backend const& b,RetainExistingHold const& retain) noexcept {
        if(ticking_||!complete(b)||state_.pending||!state_.baseline_ready||!state_.actor_bound||
           !state_.input_bound||!state_.held_known)return false;
        ticking_=true;suspended_in_tick_=false;
        struct Exit {bool& flag;~Exit(){flag=false;}} exit{ticking_};
        std::uint64_t now{};
        if(!clock(b,now)||!same(state_.actor,actor(f))||!same(state_.input,input(f))||
           !gears_weapon_hand::same(state_.held_identity,f.weapon)||
           f.controllers.sequence<state_.last_input_sequence||
           std::uint64_t(f.controllers.hands[1].pressed[gears_vr::control_squeeze])<state_.last_seen_press||
           std::uint64_t(f.controllers.hands[1].pressed[gears_vr::control_trigger])<state_.last_seen_index_press){
            suspend(b);return false;
        }
        // A read-only inventory observation can outlive its pose Frame while
        // an independent original-action owner still authenticates the SAME
        // physical hold. Drop input/events, but do not erase that hold merely
        // for this soft failure. Actor/input/counter changes above never use
        // the exception. The caller must recheck its live pin here, not pass
        // a boolean captured before next()/other backend work.
        const auto unavailable=[&]() noexcept {
            if(retain())suspend_input(b);else suspend(b);
            return false;
        };
        if(!current(f,b))return unavailable();
        state_.last_input_sequence=f.controllers.sequence;
        state_.last_seen_press=f.controllers.hands[1].pressed[gears_vr::control_squeeze];
        state_.last_seen_index_press=f.controllers.hands[1].pressed[gears_vr::control_trigger];
        if(state_.future_known){
            if(!fresh(state_.future,now)){state_.future={};state_.future_known=false;}
            return true;
        }
        EquipmentIntentEvent event{};const auto before=state_.cursor;
        const bool available=b.next(b.context,f,state_.cursor,event);
        if(!current(f,b))return unavailable();
        if(state_.cursor==UINT64_MAX){suspend(b);return false;}
        if(!available)return true;
        if(state_.cursor<=before||event.serial!=state_.cursor||!event_tuple(event,f)||
           !clock(b,now)||!fresh(event,now)){
            suspend(b);return false;
        }
        state_.future=event;state_.future_known=true;return true;
    }
    bool observe_input_while_blocked(Frame const& f,Backend const& b) noexcept {
        return observe_input_while_blocked(f,b,[]() noexcept {return false;});
    }
    // GT-only: close the input bridge/drop undispatched events. A switch already
    // sent to the game remains observable; this does NOT cancel/roll it back.
    // The original animation owner may independently authenticate a current
    // held weapon and fresh input while no C0 pose Frame is available. Drop
    // inventory input/events only; this does not publish or renew any hold.
    // Generic source loss still uses suspend() and revokes the publication.
    void suspend_input(Backend const& b) noexcept {
        if(ticking_)suspended_in_tick_=true;
        input_boundary(b);
    }
    void suspend(Backend const& b) noexcept {
        suspend_input(b);clear_binding(b);
    }
    // Missing/stale render publication disarms actions and revokes the hold,
    // but need not erase an already observed input baseline. No Frame, event,
    // pose or clock is retained/renewed here. On the next REAL fresh Frame,
    // actor/input cohorts, the Input publisher/tracking continuity, original
    // press time and every normal inventory/action guard still run. Hard
    // identity/observation failures must continue to use suspend().
    bool suspend_pose_gap(Backend const& b) noexcept {
        if(ticking_||!state_.actor_bound||!state_.input_bound||!state_.baseline_ready){suspend(b);return false;}
        state_.future_known=false;state_.future={};
        clear_binding(b);return true;
    }
    // Caller must have drained PG callbacks. Never reset while an original
    // call is on the stack; a reentrant request only disarms this tick.
    void reset_after_drain(Backend const& b,WeaponEquipmentModel& model) noexcept {
        if(ticking_){suspend(b);return;}
        suspend(b);model.reset();state_={};handoff_proposed_={};suspended_in_tick_=false;
    }
    Outcome tick(Frame const& f,Backend const& b,WeaponEquipmentModel& model) noexcept {
        if(ticking_)return {Status::reentrant};
        ticking_=true;suspended_in_tick_=false;
        struct Exit {bool& flag;~Exit(){flag=false;}} exit{ticking_};
        if(!complete(b)){suspend(b);return {Status::invalid_backend};}
        std::uint64_t now{};
        if(!clock(b,now))return bad_clock(b,model);
        if(state_.pending&&now>=state_.deadline_ms){
            const auto out=pending_out(Status::switch_timed_out);
            if(state_.accepted_stow_hold)clear_pending();else clear_model(b,model);
            return out;
        }
        if(!current(f,b))return lost_entry(f,b,model);
        const auto owner=actor(f);
        if(!state_.actor_bound||!same(state_.actor,owner)){
            clear_model(b,model);input_boundary(b);state_.actor=owner;state_.actor_bound=true;
        }
        auto const& right=f.controllers.hands[1];
        const auto press=std::uint64_t(right.pressed[gears_vr::control_squeeze]);
        const auto index_press=std::uint64_t(right.pressed[gears_vr::control_trigger]);
        const auto input_key=input(f);
        if(!state_.input_bound||!same(state_.input,input_key)||
           f.controllers.sequence<state_.last_input_sequence||press<state_.last_seen_press||index_press<state_.last_seen_index_press){
            input_boundary(b);state_.input=input_key;state_.input_bound=true;
        }
        state_.last_input_sequence=f.controllers.sequence;state_.last_seen_press=press;
        state_.last_seen_index_press=index_press;
        bool input_edges=!state_.edges_known;
        for(unsigned side=0;side<2;++side)for(unsigned control=0;control<gears_vr::control_count;++control){
            auto const& hand=f.controllers.hands[side];
            input_edges=input_edges||state_.last_edges[side][0][control]!=hand.pressed[control]||
                state_.last_edges[side][1][control]!=hand.released[control];
            state_.last_edges[side][0][control]=hand.pressed[control];
            state_.last_edges[side][1][control]=hand.released[control];
        }
        state_.edges_known=true;
        if(!gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active))return lost(b);

        Status input_status=Status::idle;
        if(!state_.baseline_ready){
            EquipmentIntentEvent discarded{};
            (void)b.next(b.context,f,state_.cursor,discarded);
            if(!current(f,b))return lost(b);
            // next intentionally returns false after a successful baseline.
            // Missing/invalid input leaves MAX intact and MUST NOT arm input.
            state_.baseline_ready=state_.cursor!=UINT64_MAX;
            if(state_.baseline_ready){state_.consumed_press=press;state_.consumed_index_press=index_press;}
            input_status=state_.baseline_ready?Status::baseline:Status::baseline_waiting;
        }

        // Held AND unheld input must precede fresh inventory/identity work.
        // Otherwise that observation can age an original recorded press out
        // before it is even decoded. This local event grants no authority: every
        // currentness, inventory, attachment and action-age check still runs.
        // It is discarded on every early return and never retained/retried.
        EquipmentIntentEvent held_input{};
        const bool sampled_input=input_status==Status::idle&&!state_.pending&&!state_.future_known;
        const bool sampled_held=sampled_input&&model.held();
        std::uint64_t held_cursor{};bool have_held_input{};bool observe_only=false;
        if(sampled_input){
            held_cursor=state_.cursor;
            have_held_input=b.next(b.context,f,state_.cursor,held_input);
            observe_only=sampled_held&&!have_held_input&&state_.hold_published&&
                state_.cursor!=UINT64_MAX&&state_.cursor==held_cursor;
            if(!current(f,b)){
                if(sampled_held&&!have_held_input&&state_.cursor==UINT64_MAX&&
                   retain_existing_hold(f,b,model))
                    return pending_out(Status::unavailable);
                return lost_observed(f,b,model,observe_only&&!input_edges);
            }
        }

        EquipmentSnapshot game{};
        // Observation-only idle hold: no selection, sound, switch or new hold
        // can consume this reduced snapshot. Keep live slots/identity/attachment.
        const bool quiet_unheld=b.quiet_unheld_inventory&&sampled_input&&!have_held_input&&!input_edges&&
            state_.cursor!=UINT64_MAX&&state_.cursor==held_cursor&&!model.held()&&
            !model.switch_pending()&&!model.animation_owns_hands();
        const auto read_inventory=quiet_unheld?b.quiet_unheld_inventory:
            observe_only&&b.held_inventory?b.held_inventory:b.inventory;
        if(!read_inventory(b.context,f,game)){
            if(observe_only&&b.held_inventory&&b.retain_idle_hold&&state_.held_known&&
               gears_weapon_hand::same(state_.held_identity,f.weapon)&&
               model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&b.retain_idle_hold(b.context,f)){
                // Still unknown: drop equipment input/events and do not run
                // any action or consume this reduced/failed snapshot.
                suspend_input(b);return {Status::inventory_unavailable};
            }
            if(input_status==Status::baseline&&retain_existing_hold(f,b,model))
                return {Status::inventory_unavailable};
            clear_binding(b);return {Status::inventory_unavailable};
        }
        if(!current(f,b))return lost_observed(f,b,model,observe_only&&!input_edges&&game.valid()&&
            game.world==f.world.address&&game.pawn==f.pawn.address&&game.current_weapon==f.weapon.address);
        if(!game.valid()||game.world!=f.world.address||game.pawn!=f.pawn.address||game.current_weapon!=f.weapon.address){
            clear_binding(b);return {Status::identity_mismatch};
        }
        if(quiet_unheld&&(state_.pending||state_.future_known||model.held()||
            model.switch_pending()||model.animation_owns_hands()))return lost(b);
        if(!clock(b,now))return bad_clock(b,model);
        if(state_.pending&&now>=state_.deadline_ms){
            const auto out=pending_out(Status::switch_timed_out);
            if(state_.accepted_stow_hold)clear_pending();else clear_model(b,model);
            return out;
        }
        if(state_.pending){
            auto out=pending_out(state_.pending_stow?Status::stow_pending:state_.pending_unholster?Status::unholster_pending:Status::switch_pending);
            ObjectIdentity target{};
            const bool expected_model=state_.pending_stow?model.held_for(game.world,game.pawn,state_.target.address):state_.pending_unholster?
                handoff_proposed_.held_for(game.world,game.pawn,state_.target.address):
                state_.pending_dispatch?handoff_proposed_.switch_pending():model.switch_pending();
            if(!expected_model||game.weapon_in_slot[state_.pending_slot]!=state_.target.address||
               !identify(b,state_.target.address,target)||!gears_weapon_hand::same(target,state_.target)){
                clear_model(b,model);out.status=Status::switch_interrupted;return out;
            }
            if(!current(f,b))return lost(b);
            if(state_.pending_stow){
                const bool accepted=state_.accepted_stow_hold!=0;
                if(accepted&&(model.hold_generation()!=state_.accepted_stow_hold||
                              index_press!=state_.accepted_stow_trigger_press)){
                    clear_pending();out.status=Status::switch_interrupted;return out;
                }
                if(!event_tuple(state_.pending_intent,f)||(!accepted&&!fresh(state_.pending_intent,now))){
                    clear_model(b,model);out.status=Status::expired_event;return out;
                }
                if(f.key.frame<=state_.pending_frame||!game.can_switch||game.animation_busy)return out;
                if(!gears_weapon_hand::same(f.weapon,target)||!b.stow){
                    clear_pending();out.status=Status::stow_failed;return out;
                }
                bool stop_issued{};
                if(b.stow_effects_ready&&!b.stow_effects_ready(b.context,f,stop_issued))return out;
                if(!current(f,b))return lost(b);
                if(!clock(b,now))return bad_clock(b,model);
                if(accepted&&now>=state_.deadline_ms){
                    clear_pending();out.status=Status::switch_timed_out;return out;
                }
                if(!accepted&&!fresh(state_.pending_intent,now)){
                    clear_model(b,model);out.status=Status::expired_event;return out;
                }
                if(!b.stow(b.context,f,game)){
                    clear_pending();out.status=Status::stow_failed;return out;
                }
                model=handoff_proposed_;clear_binding(b);clear_pending();state_.held_known=false;state_.held_identity={};
                out.status=Status::stowed;return out;
            }
            if(state_.pending_dispatch){
                if(!event_tuple(state_.pending_intent,f)||!fresh(state_.pending_intent,now)){
                    clear_model(b,model);out.status=Status::expired_event;return out;
                }
                if(f.key.frame<=state_.pending_frame||!game.can_switch||game.animation_busy)return out;
                if(now>(std::numeric_limits<std::uint64_t>::max)()-5000){clear_model(b,model);out.status=Status::clock_invalid;return out;}
                clear_binding(b);
                if(!b.switch_slot(b.context,f,game,out.slot)){clear_model(b,model);out.status=Status::switch_failed;return out;}
                model=handoff_proposed_;handoff_proposed_={};state_.pending_dispatch=false;state_.pending_intent={};
                state_.deadline_ms=now+5000;state_.pending_frame=0;
                out.status=Status::switch_requested;
                if(!current(f,b)){suspend(b);out.status=Status::unavailable;}
                return out;
            }
            if(state_.pending_unholster){
                if(game.current_weapon!=target.address||!gears_weapon_hand::same(f.weapon,target)){
                    clear_model(b,model);out.status=Status::switch_interrupted;return out;
                }
                // The helper changed the attachment, so the original event's
                // source cannot certify this hold. Observe the real result on
                // this new, independently authenticated publication instead.
                if(f.key.frame<=state_.pending_frame||!game.can_switch||game.animation_busy)return out;
                const bool attached=b.attached(b.context,f,game);
                if(!current(f,b))return lost(b);
                if(!clock(b,now))return bad_clock(b,model);
                if(now>=state_.deadline_ms){clear_model(b,model);out.status=Status::switch_timed_out;return out;}
                if(!attached){out.status=Status::attachment_unavailable;return out;}
                return commit_hold(f,b,game,model,handoff_proposed_,target,out);
            }
            auto proposed=model;const auto observation=proposed.observe(game);
            if(observation.event==gears_weapon::EquipmentEvent::interrupted){
                clear_model(b,model);out.status=Status::switch_interrupted;return out;
            }
            if(observation.event==gears_weapon::EquipmentEvent::equipped){
                if(!gears_weapon_hand::same(f.weapon,target)){
                    clear_model(b,model);out.status=Status::switch_interrupted;return out;
                }
                const bool attached=b.attached(b.context,f,game);
                if(!current(f,b))return lost(b);
                if(!clock(b,now))return bad_clock(b,model);
                if(now>=state_.deadline_ms){clear_model(b,model);out.status=Status::switch_timed_out;return out;}
                if(!attached){out.status=Status::attachment_unavailable;return out;}
                return commit_hold(f,b,game,model,proposed,target,out);
            }
        }else{
            // Observe the same real model, including changes caused by ordinary
            // game input. Unknown external switching is never adopted as our request.
            const bool was_held=model.held();
            const auto observation=model.observe(game);
            if(model.switch_pending()||observation.event==gears_weapon::EquipmentEvent::interrupted){
                clear_model(b,model);return {Status::switch_interrupted};
            }
            if(was_held&&model.held()){
                ObjectIdentity actual{};
                if(!state_.held_known||!identify(b,game.current_weapon,actual)||
                   !gears_weapon_hand::same(actual,state_.held_identity)||!gears_weapon_hand::same(actual,f.weapon)){
                    clear_model(b,model);return {Status::identity_mismatch};
                }
                const bool attached=b.attached(b.context,f,game);
                if(!current(f,b))return lost_observed(f,b,model,observe_only&&!input_edges&&attached);
                if(!attached){clear_model(b,model);return {Status::attachment_unavailable};}
                if(!state_.hold_published){
                    const auto out=republish_hold(f,b,game,model,{Status::held,model.held_slot(),actual.address});
                    if(out.status!=Status::held)return out;
                }
            }
        }
        // The reduced snapshot can only observe current inventory/presentation.
        // Keep a dedicated return so no future event path can use unknown ammo.
        if(quiet_unheld){
            if(b.idle_unheld){b.idle_unheld(b.context,f,game);if(!current(f,b))return lost(b);}
            return {};
        }
        if(input_status!=Status::idle){
            if(input_status==Status::baseline&&!model.held()&&!state_.pending&&b.idle_unheld){
                b.idle_unheld(b.context,f,game);
                if(!current(f,b))return lost(b);
            }
            return {input_status};
        }

        EquipmentIntentEvent event{};
        if(state_.future_known){event=state_.future;state_.future_known=false;state_.future={};}
        else {
            const auto cursor=sampled_input?held_cursor:state_.cursor;
            bool have_event{};
            if(sampled_input){event=held_input;have_event=have_held_input;}
            else {
                have_event=b.next(b.context,f,state_.cursor,event);
                if(!current(f,b))return lost(b);
            }
            if(!have_event){
                // The local Frame input can discover a source gap/calibration
                // boundary independently of Runtime's cohort. Drop old presses
                // and require a successful fresh baseline; retain game pending.
                if(state_.cursor==UINT64_MAX){input_boundary(b);return {Status::baseline_waiting};}
                if(!state_.pending&&!model.held()&&b.idle_unheld){
                    b.idle_unheld(b.context,f,game);
                    if(!current(f,b))return lost(b);
                }
                return state_.pending?pending_out(Status::switch_pending):Outcome{};
            }
            if(state_.cursor==UINT64_MAX||state_.cursor<=cursor||event.serial!=state_.cursor)return {Status::invalid_event};
        }
        const bool index_acquire=event.kind==gears_weapon::EquipmentIntentKind::index_acquire_slot;
        const auto action_press=index_acquire?index_press:press;
        const auto action_control=index_acquire?gears_vr::control_trigger:gears_vr::control_squeeze;
        Outcome out{Status::invalid_event,event.inventory_slot,0,event.serial,action_press,action_control};
        if(!current(f,b))return lost(b);
        if(event.kind!=gears_weapon::EquipmentIntentKind::slot&&!index_acquire){out.status=Status::event_ignored;return out;}
        if(event.inventory_slot>=4||!event_tuple(event,f))return out;
        if(!clock(b,now))return bad_clock(b,model);
        if(!fresh(event,now)){out.status=Status::expired_event;return out;}
        if(index_acquire){
            // Index has no producer-cached edge pose: only the exact current
            // held edge from Input may acquire. Never defer future/stale index
            // poses, repurpose held firing, or let index stow/switch a hold.
            if(event.input_sequence!=f.controllers.sequence||event.inventory_slot<2||
               model.held()||state_.pending||!b.begin_index_acquire||
               right.trigger_active!=1||!gears_weapon_hand::control_held(right,gears_vr::control_trigger))return out;
            if(index_press<=state_.consumed_index_press){out.status=Status::duplicate_press;return out;}
            state_.consumed_index_press=index_press;
            if(!b.begin_index_acquire(b.context,f,game,event.inventory_slot)){out.status=Status::event_ignored;return out;}
            if(!current(f,b))return lost(b);
            if(!clock(b,now))return bad_clock(b,model);
            if(!fresh(event,now)){out.status=Status::expired_event;return out;}
        }else{
        if(event.input_sequence>f.controllers.sequence){
            state_.future=event;state_.future_known=true;out.status=Status::future_event;return out;
        }
        if(!right.last_squeeze_press_sequence||right.last_squeeze_press_sequence>event.input_sequence||
           !gears_vr::controller_pose_usable(right.last_squeeze_press_grip,right.last_squeeze_press_flags,1))return out;
        if(press<=state_.consumed_press){out.status=Status::duplicate_press;return out;}
        // Latest press may skip several presses that missed all body slots.
        // Consume before any callback, including busy/empty/unsupported/failure.
        state_.consumed_press=press;
        }
        auto proposed=model;const auto decision=proposed.tap(body(event.inventory_slot),game);
        out.slot=decision.inventory_slot<4?decision.inventory_slot:event.inventory_slot;out.target=decision.weapon;
        if(decision.event==gears_weapon::EquipmentEvent::busy){out.status=Status::busy;return out;}
        if(decision.event==gears_weapon::EquipmentEvent::request_stow){
            if(!b.stow){out.status=Status::unsupported_stow;return out;}
            if(!current(f,b))return lost(b);
            if(!clock(b,now))return bad_clock(b,model);
            if(!fresh(event,now)){out.status=Status::expired_event;return out;}
            bool prepared{},stop_issued{};
            const auto hold_generation=model.hold_generation();
            if(out.slot>=4||game.weapon_in_slot[out.slot]!=decision.weapon||
               !model.held_for(game.world,game.pawn,decision.weapon)||!gears_weapon_hand::same(state_.held_identity,f.weapon)){
                out.status=Status::identity_mismatch;return out;
            }
            if(b.prepare_stow&&!b.prepare_stow(b.context,f,prepared)){out.status=Status::stow_failed;return out;}
            if(!prepared&&b.stow_effects_ready){
                prepared=!b.stow_effects_ready(b.context,f,stop_issued);
                if(!clock(b,now))return bad_clock(b,model);
                if(!fresh(event,now)){out.status=Status::expired_event;return out;}
                if(!current(f,b))return lost(b);
            }
            if(prepared){
                if(!clock(b,now))return bad_clock(b,model);
                if(!fresh(event,now)){out.status=Status::expired_event;return out;}
                ObjectIdentity target{};
                if(!identify(b,decision.weapon,target)||!gears_weapon_hand::same(target,f.weapon)){
                    out.status=Status::identity_mismatch;return out;
                }
                if(stop_issued){
                    if(!current(f,b))return lost(b);
                    if(!clock(b,now))return bad_clock(b,model);
                    if(!fresh(event,now)){out.status=Status::expired_event;return out;}
                }
                const bool accepted=stop_issued&&hold_generation&&model.hold_generation()==hold_generation;
                // R6 observed original HOD effect cleanup about 3 s after Grip.
                constexpr std::uint64_t completion_limit_ms=5000;
                const auto deadline_base=accepted?now:event.created_ms;
                const auto deadline_window=accepted?completion_limit_ms:250;
                if(deadline_base>(std::numeric_limits<std::uint64_t>::max)()-deadline_window){out.status=Status::clock_invalid;return out;}
                handoff_proposed_=proposed;state_.pending=state_.pending_stow=true;
                state_.target=target;state_.pending_slot=out.slot;state_.deadline_ms=deadline_base+deadline_window;
                if(accepted){
                    state_.accepted_stow_hold=hold_generation;
                    state_.accepted_stow_trigger_press=index_press;
                }
                state_.pending_event_serial=event.serial;state_.pending_press=action_press;
                state_.pending_control=action_control;
                state_.pending_frame=f.key.frame;state_.pending_intent=event;
                out.status=Status::stow_pending;return out;
            }
            if(!current(f,b))return lost(b);
            if(!b.stow(b.context,f,game)){out.status=Status::stow_failed;return out;}
            // The adapter verified its real, restorable attachment handoff.
            // No new hold/action can consume the now changed publication.
            model=proposed;clear_binding(b);clear_pending();state_.held_known=false;state_.held_identity={};
            out.status=Status::stowed;return out;
        }
        if(decision.event==gears_weapon::EquipmentEvent::empty_slot){
            if(!current(f,b))return lost(b);
            if(!clock(b,now))return bad_clock(b,model);
            if(!fresh(event,now)){out.status=Status::expired_event;return out;}
            out.status=b.empty_sound(b.context,f,game,out.slot)?Status::empty_slot:Status::empty_sound_failed;return out;
        }
        if(decision.event!=gears_weapon::EquipmentEvent::request_unholster&&
           decision.event!=gears_weapon::EquipmentEvent::request_original_switch){out.status=Status::event_ignored;return out;}
        ObjectIdentity target{};
        if(!identify(b,decision.weapon,target)){out.status=Status::identity_mismatch;return out;}
        if(!current(f,b))return lost(b);
        if(decision.event==gears_weapon::EquipmentEvent::request_unholster&&!gears_weapon_hand::same(target,f.weapon)){
            out.status=Status::identity_mismatch;return out;
        }
        if(!clock(b,now))return bad_clock(b,model);
        if(!fresh(event,now)){out.status=Status::expired_event;return out;}
        bool changed{};
        if(b.unholster&&!b.unholster(b.context,f,game,changed)){
            out.status=decision.event==gears_weapon::EquipmentEvent::request_unholster?Status::unholster_failed:Status::switch_failed;
            return out;
        }
        if(decision.event==gears_weapon::EquipmentEvent::request_unholster){
            if(changed){
                if(!clock(b,now))return bad_clock(b,model);
                if(now>(std::numeric_limits<std::uint64_t>::max)()-5000){out.status=Status::clock_invalid;return out;}
                clear_binding(b);handoff_proposed_=proposed;
                state_.pending=state_.pending_unholster=true;state_.target=target;state_.pending_slot=out.slot;
                state_.deadline_ms=now+5000;state_.pending_event_serial=event.serial;state_.pending_press=action_press;
                state_.pending_control=action_control;
                state_.pending_frame=f.key.frame;
                out.status=Status::unholster_pending;return out;
            }
            const bool attached=b.attached(b.context,f,game);
            if(!current(f,b))return lost(b);
            if(!clock(b,now))return bad_clock(b,model);
            if(!fresh(event,now)){out.status=Status::expired_event;return out;}
            if(!attached){out.status=Status::attachment_unavailable;return out;}
            return commit_hold(f,b,game,model,proposed,target,out);
        }
        if(changed){
            if(!clock(b,now))return bad_clock(b,model);
            if(!fresh(event,now)){out.status=Status::expired_event;return out;}
            if(event.created_ms>(std::numeric_limits<std::uint64_t>::max)()-250){out.status=Status::clock_invalid;return out;}
            clear_binding(b);handoff_proposed_=proposed;
            state_.pending=state_.pending_dispatch=true;state_.target=target;state_.pending_slot=out.slot;
            state_.deadline_ms=event.created_ms+250;state_.pending_event_serial=event.serial;state_.pending_press=action_press;
            state_.pending_control=action_control;
            state_.pending_frame=f.key.frame;state_.pending_intent=event;
            out.status=Status::switch_pending;return out;
        }
        if(!clock(b,now))return bad_clock(b,model);
        if(!fresh(event,now)){out.status=Status::expired_event;return out;}
        if(now>(std::numeric_limits<std::uint64_t>::max)()-5000){out.status=Status::clock_invalid;return out;}
        clear_binding(b);
        // One original call only. Returning true starts observation, not a hold.
        if(!b.switch_slot(b.context,f,game,out.slot)){out.status=Status::switch_failed;return out;}
        model=proposed;state_.held_known=false;state_.held_identity={};state_.pending=true;
        state_.target=target;state_.pending_slot=out.slot;state_.deadline_ms=now+5000;
        state_.pending_event_serial=event.serial;state_.pending_press=action_press;
        state_.pending_control=action_control;
        out.status=Status::switch_requested;
        if(!current(f,b)){suspend(b);out.status=Status::unavailable;}
        return out;
    }
};
} // namespace gears_equipment_runtime
