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
};
static_assert(std::is_standard_layout_v<Backend>&&std::is_trivially_copyable_v<Backend>);

enum class Status : std::uint8_t {
    idle,baseline_waiting,baseline,unavailable,invalid_backend,reentrant,
    event_ignored,invalid_event,expired_event,future_event,duplicate_press,
    busy,empty_slot,empty_sound_failed,unsupported_stow,attachment_unavailable,
    identity_mismatch,inventory_unavailable,switch_requested,switch_failed,
    switch_pending,switch_timed_out,switch_interrupted,held,hold_failed,clock_invalid
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
#undef GEARS_EQUIPMENT_STATUS
    }
    return "invalid_status";
}
struct Outcome {
    Status status{Status::idle};unsigned slot{4};
    std::uint64_t target{},event_serial{},press_counter{};
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
    bool actor_bound{},input_bound{},baseline_ready{},future_known{},pending{},held_known{},hold_published{},clock_known{};
    std::uint64_t cursor{UINT64_MAX},last_input_sequence{},consumed_press{},last_seen_press{},last_ms{};
    EquipmentIntentEvent future{}; // At most one original event, never retimestamped.
    ObjectIdentity target{},held_identity{};
    unsigned pending_slot{4};
    std::uint64_t deadline_ms{},pending_event_serial{},pending_press{};
};

class Runtime {
    State state_{};
    bool ticking_{},suspended_in_tick_{};
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
        state_.hold_published=false;
        if(b.clear_hold)b.clear_hold(b.context);
    }
    void input_boundary(Backend const& b) noexcept {
        state_.input={};state_.input_bound=state_.baseline_ready=state_.future_known=false;
        state_.cursor=UINT64_MAX;state_.last_input_sequence=state_.consumed_press=state_.last_seen_press=0;
        state_.future={};if(b.reset_input)b.reset_input(b.context);
    }
    void clear_pending() noexcept {
        state_.pending=false;state_.target={};state_.pending_slot=4;
        state_.deadline_ms=state_.pending_event_serial=state_.pending_press=0;
    }
    void clear_model(Backend const& b,WeaponEquipmentModel& model) noexcept {
        clear_binding(b);model.reset();clear_pending();state_.held_known=false;state_.held_identity={};
    }
    Outcome pending_out(Status status) const noexcept {
        return {status,state_.pending_slot,state_.target.address,state_.pending_event_serial,state_.pending_press};
    }
    Outcome lost(Backend const& b) noexcept {
        const auto out=pending_out(Status::unavailable);suspend(b);return out;
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
    // GT-only: close the input bridge/drop undispatched events. A switch already
    // sent to the game remains observable; this does NOT cancel/roll it back.
    void suspend(Backend const& b) noexcept {
        if(ticking_)suspended_in_tick_=true;
        input_boundary(b);clear_binding(b);
    }
    // Caller must have drained PG callbacks. Never reset while an original
    // call is on the stack; a reentrant request only disarms this tick.
    void reset_after_drain(Backend const& b,WeaponEquipmentModel& model) noexcept {
        if(ticking_){suspend(b);return;}
        suspend(b);model.reset();state_={};suspended_in_tick_=false;
    }
    Outcome tick(Frame const& f,Backend const& b,WeaponEquipmentModel& model) noexcept {
        if(ticking_)return {Status::reentrant};
        ticking_=true;suspended_in_tick_=false;
        struct Exit {bool& flag;~Exit(){flag=false;}} exit{ticking_};
        if(!complete(b)){suspend(b);return {Status::invalid_backend};}
        std::uint64_t now{};
        if(!clock(b,now))return bad_clock(b,model);
        if(state_.pending&&now>=state_.deadline_ms){
            const auto out=pending_out(Status::switch_timed_out);clear_model(b,model);return out;
        }
        if(!current(f,b))return lost(b);
        const auto owner=actor(f);
        if(!state_.actor_bound||!same(state_.actor,owner)){
            clear_model(b,model);input_boundary(b);state_.actor=owner;state_.actor_bound=true;
        }
        auto const& right=f.controllers.hands[1];
        const auto press=std::uint64_t(right.pressed[gears_vr::control_squeeze]);
        const auto input_key=input(f);
        if(!state_.input_bound||!same(state_.input,input_key)||
           f.controllers.sequence<state_.last_input_sequence||press<state_.last_seen_press){
            input_boundary(b);state_.input=input_key;state_.input_bound=true;
        }
        state_.last_input_sequence=f.controllers.sequence;state_.last_seen_press=press;
        if(!gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active))return lost(b);

        Status input_status=Status::idle;
        if(!state_.baseline_ready){
            EquipmentIntentEvent discarded{};
            (void)b.next(b.context,f,state_.cursor,discarded);
            if(!current(f,b))return lost(b);
            // next intentionally returns false after a successful baseline.
            // Missing/invalid input leaves MAX intact and MUST NOT arm input.
            state_.baseline_ready=state_.cursor!=UINT64_MAX;
            if(state_.baseline_ready)state_.consumed_press=press;
            input_status=state_.baseline_ready?Status::baseline:Status::baseline_waiting;
        }

        EquipmentSnapshot game{};
        if(!b.inventory(b.context,f,game)){
            clear_binding(b);return {Status::inventory_unavailable};
        }
        if(!current(f,b))return lost(b);
        if(!game.valid()||game.world!=f.world.address||game.pawn!=f.pawn.address||game.current_weapon!=f.weapon.address){
            clear_binding(b);return {Status::identity_mismatch};
        }
        if(!clock(b,now))return bad_clock(b,model);
        if(state_.pending&&now>=state_.deadline_ms){
            const auto out=pending_out(Status::switch_timed_out);clear_model(b,model);return out;
        }
        if(state_.pending){
            auto out=pending_out(Status::switch_pending);
            ObjectIdentity target{};
            if(!model.switch_pending()||game.weapon_in_slot[state_.pending_slot]!=state_.target.address||
               !identify(b,state_.target.address,target)||!gears_weapon_hand::same(target,state_.target)){
                clear_model(b,model);out.status=Status::switch_interrupted;return out;
            }
            if(!current(f,b))return lost(b);
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
                if(!current(f,b))return lost(b);
                if(!attached){clear_model(b,model);return {Status::attachment_unavailable};}
                if(!state_.hold_published){
                    const auto out=commit_hold(f,b,game,model,model,actual,{Status::held,model.held_slot(),actual.address});
                    if(out.status!=Status::held)return out;
                }
            }
        }
        if(input_status!=Status::idle)return {input_status};

        EquipmentIntentEvent event{};
        if(state_.future_known){event=state_.future;state_.future_known=false;state_.future={};}
        else {
            const auto cursor=state_.cursor;
            const bool have_event=b.next(b.context,f,state_.cursor,event);
            if(!current(f,b))return lost(b);
            if(!have_event){
                // The local Frame input can discover a source gap/calibration
                // boundary independently of Runtime's cohort. Drop old presses
                // and require a successful fresh baseline; retain game pending.
                if(state_.cursor==UINT64_MAX){input_boundary(b);return {Status::baseline_waiting};}
                return state_.pending?pending_out(Status::switch_pending):Outcome{};
            }
            if(state_.cursor==UINT64_MAX||state_.cursor<=cursor||event.serial!=state_.cursor)return {Status::invalid_event};
        }
        Outcome out{Status::invalid_event,event.inventory_slot,0,event.serial,press};
        if(!current(f,b))return lost(b);
        if(event.kind!=gears_weapon::EquipmentIntentKind::slot){out.status=Status::event_ignored;return out;}
        if(event.inventory_slot>=4||!event_tuple(event,f))return out;
        if(!clock(b,now))return bad_clock(b,model);
        if(!fresh(event,now)){out.status=Status::expired_event;return out;}
        if(event.input_sequence>f.controllers.sequence){
            state_.future=event;state_.future_known=true;out.status=Status::future_event;return out;
        }
        if(!right.last_squeeze_press_sequence||right.last_squeeze_press_sequence>event.input_sequence||
           !gears_vr::controller_pose_usable(right.last_squeeze_press_grip,right.last_squeeze_press_flags,1))return out;
        if(press<=state_.consumed_press){out.status=Status::duplicate_press;return out;}
        // Latest press may skip several presses that missed all body slots.
        // Consume before any callback, including busy/empty/unsupported/failure.
        state_.consumed_press=press;
        auto proposed=model;const auto decision=proposed.tap(body(event.inventory_slot),game);
        out.slot=decision.inventory_slot<4?decision.inventory_slot:event.inventory_slot;out.target=decision.weapon;
        if(decision.event==gears_weapon::EquipmentEvent::busy){out.status=Status::busy;return out;}
        if(decision.event==gears_weapon::EquipmentEvent::request_stow){out.status=Status::unsupported_stow;return out;}
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
        if(decision.event==gears_weapon::EquipmentEvent::request_unholster){
            if(!gears_weapon_hand::same(target,f.weapon)){out.status=Status::identity_mismatch;return out;}
            const bool attached=b.attached(b.context,f,game);
            if(!current(f,b))return lost(b);
            if(!clock(b,now))return bad_clock(b,model);
            if(!fresh(event,now)){out.status=Status::expired_event;return out;}
            if(!attached){out.status=Status::attachment_unavailable;return out;}
            return commit_hold(f,b,game,model,proposed,target,out);
        }
        if(!clock(b,now))return bad_clock(b,model);
        if(!fresh(event,now)){out.status=Status::expired_event;return out;}
        if(now>(std::numeric_limits<std::uint64_t>::max)()-5000){out.status=Status::clock_invalid;return out;}
        clear_binding(b);
        // One original call only. Returning true starts observation, not a hold.
        if(!b.switch_slot(b.context,f,game,out.slot)){out.status=Status::switch_failed;return out;}
        model=proposed;state_.held_known=false;state_.held_identity={};state_.pending=true;
        state_.target=target;state_.pending_slot=out.slot;state_.deadline_ms=now+5000;
        state_.pending_event_serial=event.serial;state_.pending_press=press;
        out.status=Status::switch_requested;
        if(!current(f,b)){suspend(b);out.status=Status::unavailable;}
        return out;
    }
};
} // namespace gears_equipment_runtime
