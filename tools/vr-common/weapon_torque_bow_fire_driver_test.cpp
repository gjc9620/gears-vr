#include "weapon_torque_bow_driver_fixture.h"
namespace {
void normal_and_short_tap(){
    F f;f.charge();const auto preparations=f.backend.preparations;
    f.driver.before_original_tick(f.backend,f.frame,f.game);f.run();
    assert(f.backend.presses==1&&f.backend.releases==0&&f.backend.cancels==0);
    assert(f.backend.preparations>preparations); // Same packet services a current ray without another edge.
    f.next();f.trigger(false);f.run();assert(f.backend.releases==1&&f.backend.recorded==1);
    assert(f.driver.inventory().loaded==0&&!f.backend.owner&&!f.driver.pending());
    F tap;tap.run();tap.next();tap.trigger(true);tap.trigger(false);tap.run();
    assert(tap.backend.presses==1&&tap.backend.releases==1&&tap.backend.recorded==1&&tap.driver.inventory().loaded==0);
}
void cancel_and_rearm(){
    for(bool simultaneous_up:{false,true}){
        F f;f.charge();f.next();f.r3();if(simultaneous_up)f.trigger(false);f.run();
        assert(f.backend.guard_sync==f.frame.controllers.sync_ns);
        assert(f.backend.cancels==1&&!f.backend.releases&&!f.backend.recorded&&f.driver.inventory().loaded==1);
        assert(!f.driver.pending());f.next();f.run();assert(f.backend.presses==1);
        if(!simultaneous_up){f.next();f.trigger(false);f.run();}
        f.next();f.trigger(true);f.run();assert(f.backend.presses==2&&f.backend.owner);
    }
    F pending;pending.charge();pending.backend.deferred=true;pending.next();pending.trigger(false);pending.run();
    assert(pending.backend.owner&&pending.driver.stage()==w::TorqueBowStage::await_original);
    pending.next();pending.r3();pending.run();assert(pending.backend.cancels==1&&!pending.backend.queue);
}
void unknown_loss_and_fatigue(){
    F unknown;unknown.backend.press_result=R::unknown;unknown.run();unknown.next();unknown.trigger(true);unknown.run();
    assert(unknown.backend.presses==1&&unknown.backend.cancels==1&&!unknown.backend.releases&&!unknown.driver.pending());
    unknown.next();unknown.run();assert(unknown.backend.presses==1);
    F loss;loss.charge();loss.backend.time+=100000001;loss.driver.poll(loss.backend);
    assert(loss.backend.cancels==1&&!loss.backend.releases&&!loss.driver.pending());
    loss.next();loss.run();assert(loss.backend.presses==1); // Recovered held trigger is a baseline.
    F fatigue;fatigue.charge();fatigue.backend.arrow(false);fatigue.next();fatigue.run();
    assert(fatigue.backend.recorded==1&&fatigue.driver.inventory().loaded==0&&fatigue.backend.owner);
    fatigue.next();fatigue.r3();fatigue.trigger(false);fatigue.run();
    assert(fatigue.backend.releases==1&&fatigue.backend.recorded==1&&!fatigue.backend.owner); // R3 cannot suppress real key-up after a shot.
    F uncertain;uncertain.charge();uncertain.backend.release_result=R::unknown;uncertain.backend.hold_fire=true;
    uncertain.next();uncertain.trigger(false);assert(uncertain.run()==d::Status::conflict);
    assert(uncertain.backend.recorded==1&&uncertain.driver.inventory().loaded==0&&uncertain.driver.pending());
    assert(uncertain.backend.post_shot_clears==1&&uncertain.backend.releases==1&&!uncertain.backend.cancels);
    uncertain.next();uncertain.run();assert(uncertain.backend.post_shot_clears==1&&uncertain.backend.releases==1);
    uncertain.backend.actual.original.firing=false;uncertain.backend.actual.fully_stopped=true;
    uncertain.next();uncertain.run();assert(!uncertain.driver.pending()&&!uncertain.backend.owner);
    assert(uncertain.backend.recorded==1&&uncertain.backend.releases==1&&uncertain.driver.inventory().loaded==0);
}
void boundaries(){
    F missing;missing.backend.native=false;missing.run();missing.next();missing.trigger(true);missing.run();
    assert(!missing.backend.presses&&!missing.driver.bound());
    assert(!missing.driver.hold_generation());
    F gap;gap.charge();gap.next(100000001);gap.run();assert(gap.backend.cancels==1&&!gap.backend.releases);
    F wrong;wrong.charge();auto alien=wrong.frame;alien.controller.address+=8;
    wrong.driver.tick(wrong.backend,alien,wrong.game);assert(wrong.backend.owner&&!wrong.backend.cancels);
    F changed;changed.charge();changed.next();++changed.frame.controllers.space_epoch;++changed.frame.stereo.space_epoch;
    changed.run();assert(changed.backend.cancels==1&&!changed.backend.releases&&changed.backend.presses==1);
    F foreign;foreign.charge();foreign.next();++foreign.backend.actual.reserve;foreign.run();
    assert(foreign.backend.cancels==1&&!foreign.backend.releases&&foreign.driver.inventory().reserve==5);
    F stale;stale.charge();stale.backend.preparable=false;stale.driver.before_original_tick(stale.backend,stale.frame,stale.game);
    assert(stale.backend.cancels==1&&!stale.backend.releases);
    F unguarded;unguarded.charge();unguarded.backend.veto=false;unguarded.next();unguarded.trigger(false);unguarded.run();
    assert(unguarded.backend.cancels==1&&!unguarded.backend.releases&&!unguarded.backend.recorded);
    F terminal;terminal.charge();terminal.backend.readable=false;terminal.next();++terminal.frame.key.world_generation;
    terminal.run();assert(terminal.driver.pending()&&terminal.backend.owner&&!terminal.backend.releases);
    assert(!terminal.driver.reset_drained(terminal.backend)); // Lost incarnation keeps the old journal, never adopts a new actor.
    F stop;stop.charge();stop.driver.stop(stop.backend,w::TorqueBowCancelRoute::put_down);
    assert(!stop.driver.pending()&&!stop.backend.owner&&!stop.backend.releases&&stop.driver.reset_drained(stop.backend));
    assert(!stop.driver.hold_generation()&&!stop.driver.interaction().held_arrow_serial());
    F rehold;rehold.run();rehold.next();rehold.trigger(true);++rehold.backend.hold_generation;rehold.run();
    assert(!rehold.backend.presses&&!rehold.driver.pending()); // Same actor/new hold is a baseline; no stowed edge replay.
    assert(rehold.driver.hold_generation()==rehold.backend.hold_generation&&
        rehold.driver.interaction().identity().weapon==rehold.frame.weapon.address);
    F travel;travel.backend.actual.game_tick=9000;travel.run();travel.next();++travel.frame.key.world_generation;
    travel.backend.actual.game_tick=100;assert(travel.run()==d::Status::baseline);
    travel.next();travel.trigger(true);travel.run();assert(travel.backend.presses==1&&travel.backend.owner);
}
}
int main(){normal_and_short_tap();cancel_and_rearm();unknown_loss_and_fatigue();boundaries();
    std::puts("Bow fire driver: real intent model+XR decoder, original pre/post-tick repeat, short tap, exclusive R3, deferred release, rearm, unknown/loss cleanup, fatigue key handback and context boundaries passed; injected original backend only.");}
