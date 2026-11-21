#include "weapon_torque_bow_driver_fixture.h"
#ifdef _WIN32
#include <windows.h>
#include <cstdlib>
#endif
namespace {
struct DeferredBegin:Backend {
    unsigned variant{};bool fail_read{};
    R press(h::Frame const& f) noexcept {
        const auto reply=Backend::press(f);
        if(reply==R::accepted){
            actual.original={false,false,false,true,false,false,false};
            if(variant==1)actual.original.pending_end_fire=true;
            if(variant==2)actual.original.pending_fire=true;
            if(variant==3)actual.original.animation_owns_hands=true;
            if(variant==4)actual.original.r2_held=false;
            if(variant==5)actual.original.ready=true;
            if(fail_read)readable=false;
        }
        return reply;
    }
    void original_tick(){assert(owner&&!fenced);actual.original.charging=true;}
};
using D=DriverFixture<DeferredBegin>;
void begin(D& f){f.run();f.next();f.trigger(true);f.run();}
void normal_tick_then_real_up(){
    D f;begin(f);
    assert(f.driver.stage()==w::TorqueBowStage::charge_begin&&f.backend.owner);
    assert(f.backend.presses==1&&!f.backend.cancels&&!f.backend.releases);
    f.driver.before_original_tick(f.backend,f.frame,f.game);f.run();
    assert(f.backend.presses==1&&!f.backend.cancels&&!f.backend.releases);
    f.backend.original_tick();f.run();
    assert(f.driver.stage()==w::TorqueBowStage::charging&&f.backend.presses==1);
    f.next();f.trigger(false);f.run();
    assert(f.backend.releases==1&&f.backend.recorded==1&&f.driver.inventory().loaded==0);
    assert(!f.backend.owner&&!f.driver.pending());
}
void buffered_real_up(){
    D f;f.run();f.next();f.trigger(true);f.trigger(false);f.run();
    assert(f.driver.stage()==w::TorqueBowStage::charge_begin&&f.backend.presses==1);
    assert(!f.backend.releases&&!f.backend.recorded&&!f.backend.cancels);
    f.backend.original_tick();f.next();f.run();
    assert(f.backend.releases==1&&f.backend.recorded==1&&f.driver.inventory().loaded==0);
    assert(!f.backend.owner&&!f.driver.pending());
}
void fresh_r3_during_pending_begin(){
    D f;begin(f);f.next();f.r3();f.run();
    assert(f.backend.cancels==1&&!f.backend.owner&&!f.driver.pending());
    assert(!f.backend.releases&&!f.backend.recorded&&f.driver.inventory().loaded==1);
    f.next();f.run();assert(f.backend.presses==1);
    f.next();f.trigger(false);f.run();f.next();f.trigger(true);f.run();
    assert(f.backend.presses==2&&f.driver.stage()==w::TorqueBowStage::charge_begin);
}
void fixed_request_deadline(){
    D f;begin(f);f.next(60000000);f.run();
    assert(f.backend.owner&&!f.backend.cancels&&f.backend.presses==1);
    f.next(60000000);f.run(); // Fresh producer packets cannot renew the old request deadline.
    assert(f.backend.cancels==1&&!f.backend.owner&&!f.backend.releases&&!f.backend.recorded);
    assert(!f.driver.pending()&&f.driver.inventory().loaded==1);
}
void loss_and_boundaries(){
    for(unsigned kind=0;kind<4;++kind){
        D f;begin(f);f.next(kind==0?100000001:10000000);
        if(kind==1)f.backend.veto=false;
        if(kind==2){++f.frame.controllers.space_epoch;++f.frame.stereo.space_epoch;}
        if(kind==3)++f.backend.hold_generation;
        f.run();assert(f.backend.cancels==1&&!f.backend.releases&&!f.backend.recorded);
        assert(f.backend.presses==1&&f.driver.inventory().loaded==1);
    }
    D stopped;begin(stopped);stopped.driver.stop(stopped.backend,w::TorqueBowCancelRoute::active);
    assert(stopped.backend.cancels==1&&!stopped.backend.releases&&!stopped.driver.pending());
}
void unexpected_state_and_unknown_call(){
    for(unsigned variant=1;variant<=5;++variant){
        D f;f.backend.variant=variant;begin(f);
        assert(f.backend.cancels==1&&!f.backend.releases&&!f.backend.recorded&&!f.driver.pending());
    }
    D unknown;unknown.backend.press_result=R::unknown;begin(unknown);
    assert(unknown.backend.cancels==1&&!unknown.backend.releases&&!unknown.driver.pending());
    D unreadable;unreadable.backend.fail_read=true;begin(unreadable);
    assert(unreadable.backend.owner&&unreadable.driver.pending()&&!unreadable.backend.releases);
    unreadable.backend.readable=true;unreadable.driver.poll(unreadable.backend);
    assert(unreadable.backend.cancels==1&&!unreadable.driver.pending()&&!unreadable.backend.releases);
}
}
int main(){
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    normal_tick_then_real_up();buffered_real_up();fresh_r3_during_pending_begin();
    fixed_request_deadline();loss_and_boundaries();unexpected_state_and_unknown_call();
    std::puts("Accepted original begin awaits a normal Charge transition once, with fixed request deadline; real UP, pending R3/rearm, source loss, context, unknown/read failure and strict unexpected-state cleanup passed. Injected backend only.");
}
