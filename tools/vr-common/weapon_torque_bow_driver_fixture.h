#pragma once
#include "weapon_torque_bow_fire_driver.h"
#include <cassert>
#include <cstdio>
namespace d=gears_torque_bow_fire_driver;namespace w=gears_weapon;namespace h=gears_weapon_hand;namespace v=gears_vr;
using E=w::TorqueBowEvent;using R=w::TorqueBowCallResult;
namespace {
struct Backend {
    std::int64_t time{1000000000},guard_sync{};bool native=true,current=true,readable=true,preparable=true,veto=true;
    bool owner{},fenced{},queue{},deferred{},hold_fire{},post_shot_stopped{};
    unsigned presses{},releases{},cancels{},settles{},recorded{},preparations{},post_shot_clears{};
    std::uint64_t shot{},hold_generation{1};R press_result{R::accepted},release_result{R::accepted};
    d::Snapshot actual{{true,false,false,false,false},1,5,1000,true};
    std::int64_t now() noexcept {time+=1000;return time;}
    std::uint64_t hold(h::Frame const&) noexcept {return hold_generation;}
    bool owned() const noexcept {return owner;}
    bool has_spawn() const noexcept {return queue;}
    bool reload_owned() const noexcept {return false;}
    R reload_accept(h::Frame const&,h::GripGameState const&,d::ReloadAcceptance&) noexcept {return R::rejected;}
    bool reload_complete(h::Frame const&,h::GripGameState const&,w::TorqueBowRequest const&) noexcept {return false;}
    bool reload_reject(w::TorqueBowRequest const&) noexcept {return false;}
    void reload_stop() noexcept {}
    void reload_service(h::Frame const*) noexcept {}
    bool reload_receipt(h::Frame const*,w::TorqueBowActionReceipt&,d::Snapshot&) noexcept {return false;}
    bool reload_retire(std::uint64_t,w::TorqueBowInteraction const&) noexcept {return false;}
    bool observe(h::Frame const& f,h::GripGameState const&,d::Snapshot& out) noexcept {
        if(!current||!v::controller_frame_matches(f.controllers,f.stereo,now())||time<f.controllers.sync_ns||time-f.controllers.sync_ns>100000000)return false;
        out=actual;if(out.fully_stopped)out.original.ready=true;return true;
    }
    bool configure(h::Frame const& f,h::GripGameState const&,d::Snapshot const&,w::TorqueBowConfig& config) noexcept {
        if(!native)return false;
        config.identity={f.world.address,f.pawn.address,f.controller.address,f.weapon.address,f.key.world_generation,
            f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch};
        config.instance_config_serial=1;config.capacity=1;
        config.original_class_verified=config.original_charge_entry_verified=config.original_end_fire_verified=config.non_firing_abort_verified=true;
        return true;
    }
    bool prepare(h::Frame const&,h::GripGameState const&) noexcept {
        ++preparations;return current&&preparable&&(!owner||!fenced);
    }
    bool guard(h::Frame const& f) noexcept {guard_sync=f.controllers.sync_ns;return veto;}
    R press(h::Frame const&) noexcept {
        if(press_result==R::rejected)return R::rejected;
        assert(!owner&&native&&actual.loaded==1);++presses;owner=true;fenced=false;
        actual.original={false,true,false,true,false,true,false};actual.fully_stopped=false;return press_result;
    }
    void arrow(bool key_up) noexcept {
        assert(owner&&actual.loaded==1);queue=true;++shot;actual.loaded=0;
        actual.original={key_up,false,false,!key_up,false,false,false};actual.fully_stopped=key_up;
    }
    R release(h::Frame const&) noexcept {
        if(release_result==R::rejected)return R::rejected;
        assert(owner&&!fenced);++releases;
        if(deferred){actual.original={false,true,true,false,false,true,false};actual.fully_stopped=false;}
        else if(actual.loaded){arrow(true);if(hold_fire){actual.original.firing=true;actual.fully_stopped=false;}}
        else{actual.original={true,false,false,false,false};actual.fully_stopped=true;}
        return release_result;
    }
    bool read_owned(d::Snapshot& out) noexcept {if(!owner||!readable)return false;out=actual;return true;}
    void fence() noexcept {fenced=true;}
    bool cancel(w::TorqueBowCancelRoute,d::Snapshot& out) noexcept {
        if(!readable||queue)return false;
        if(actual.original.firing){
            if(!recorded)return false;
            if(!post_shot_stopped){++post_shot_clears;post_shot_stopped=true;}
            actual.original.r2_held=actual.original.pending_fire=actual.original.pending_end_fire=false;
            return false; // Only the original tick may finish Fire; no forced transition.
        }
        if(owner){++cancels;owner=false;actual.original={false,false,false,false,false};actual.fully_stopped=true;}
        out=actual;return actual.fully_stopped;
    }
    bool settle(d::Snapshot& out) noexcept {
        if(!owner||!actual.fully_stopped||queue)return false;
        ++settles;owner=false;out=actual;return true;
    }
    d::Shot account(w::TorqueBowInteraction& model,w::TorqueBowStamp const& stamp,std::uint64_t event,E& received) noexcept {
        if(!queue)return d::Shot::none;
        if(!readable)return d::Shot::unavailable;
        if(actual.loaded)return d::Shot::waiting;
        auto inventory=model.inventory();++inventory.revision;--inventory.loaded;
        received=model.record_original_shot({stamp,event,shot,inventory,true});
        if(received!=E::shot_recorded&&received!=E::safety_violation)return d::Shot::conflict;
        queue=false;++recorded;return d::Shot::recorded;
    }
};
template<class BackendType=Backend>struct DriverFixture {
    d::Driver driver;BackendType backend;h::Frame frame{};h::GripGameState game{};
    DriverFixture(){
        frame.key={1,2,3,4,5,6,7,8};frame.world={16,0,256,1,2,0,1};frame.pawn={24,0,256,2,3,0,1};
        frame.controller={32,0,256,3,4,0,1};frame.mesh={40,0,256,4,5,0,1};frame.weapon={48,0,256,5,6,0,1};
        frame.reference_generation=1;frame.left_tracked=1;game.known=game.main_hand_held=true;
        auto& input=frame.controllers;auto& stereo=frame.stereo;
        input.session=stereo.session=1;input.space_epoch=stereo.space_epoch=1;input.sequence=stereo.sequence=20;
        input.focused=stereo.focused=1;input.predicted_time=stereo.predicted_time=100;
        for(auto& hand:input.hands){hand.grip_active=1;hand.grip_flags=15;}
        input.hands[1].trigger_active=1;input.hands[1].buttons_active=v::controller_stick_click;stamp();
    }
    void stamp(){frame.controllers.sync_ns=frame.controllers.received_ns=frame.stereo.received_ns=backend.time;}
    void next(std::int64_t delta=10000000){backend.time+=delta;++frame.key.frame;++frame.controllers.sequence;
        frame.stereo.sequence=frame.controllers.sequence;stamp();}
    void trigger(bool down){auto& hand=frame.controllers.hands[1];
        ++(down?hand.pressed[v::control_trigger]:hand.released[v::control_trigger]);hand.trigger=down?1.f:0.f;}
    void r3(){auto& hand=frame.controllers.hands[1];++hand.pressed[v::control_stick];++hand.released[v::control_stick];}
    d::Status run(){return driver.tick(backend,frame,game);}
    void charge(){assert(run()==d::Status::baseline);next();trigger(true);run();
        assert(backend.owner&&backend.presses==1&&driver.stage()==w::TorqueBowStage::charging);}
};
using F=DriverFixture<>;
}
