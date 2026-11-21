#define NOMINMAX
#include <windows.h>
#include <cstdlib>
#include "weapon_torque_bow_driver_fixture.h"
namespace {
struct CheckedBackend:Backend {
    unsigned observations{},reject_at{};bool change_after_read{};
    bool observe(h::Frame const& f,h::GripGameState const& g,d::Snapshot& out) noexcept {
        ++observations;if(observations==reject_at)return false;
        const bool valid=Backend::observe(f,g,out);
        if(valid&&change_after_read){change_after_read=false;actual.loaded=0;}
        return valid;
    }
};
using IdleFixture=DriverFixture<CheckedBackend>;
void unchanged_idle_and_new_press(){
    IdleFixture f;f.run();const auto initial=f.backend.observations;assert(initial==2);
    f.next();f.frame.controllers.hands[1].grip.position.x+=.001;f.run();
    assert(f.backend.observations==initial+1&&!f.backend.presses&&!f.driver.pending());
    f.next();f.trigger(true);f.backend.reject_at=f.backend.observations+2;f.run();
    assert(f.backend.observations==f.backend.reject_at&&!f.backend.presses&&!f.backend.owner&&!f.backend.releases);
    IdleFixture tap;tap.run();tap.next();tap.trigger(true);tap.trigger(false);
    tap.backend.reject_at=tap.backend.observations+2;tap.run();
    assert(tap.backend.observations==tap.backend.reject_at&&!tap.backend.presses&&!tap.backend.releases);
    // A complete tap between GT observations is a changed counter, never idle.
}
void action_boundaries_keep_second_read(){
    for(unsigned action=0;action<3;++action){
        IdleFixture f;f.charge();f.next();
        if(action==0)f.trigger(false);else if(action==1)f.r3();
        f.backend.reject_at=f.backend.observations+2;f.run();
        assert(f.backend.observations==f.backend.reject_at&&!f.backend.owner&&f.backend.cancels==1);
        assert(!f.backend.releases&&!f.backend.recorded); // No ordinary shot on a refused release/R3/held sample.
    }
    IdleFixture left;left.run();left.next();left.frame.left_grip_down=1;
    left.backend.reject_at=left.backend.observations+2;left.run();
    assert(left.backend.observations==left.backend.reject_at&&!left.backend.presses);
    IdleFixture cohort;cohort.run();cohort.next();++cohort.frame.controllers.input_epoch;
    const auto before=cohort.backend.observations;cohort.run();
    assert(cohort.backend.observations>=before+2&&!cohort.backend.presses);
}
void idle_never_hides_loss_or_grants_later_fire(){
    IdleFixture unreadable;unreadable.run();unreadable.next();
    unreadable.backend.reject_at=unreadable.backend.observations+1;unreadable.run();
    assert(unreadable.backend.observations==unreadable.backend.reject_at&&!unreadable.backend.presses);
    IdleFixture inventory;inventory.run();inventory.next();inventory.backend.change_after_read=true;inventory.run();
    assert(!inventory.backend.presses&&inventory.backend.actual.loaded==0);
    inventory.next();inventory.trigger(true);inventory.run();
    assert(!inventory.backend.presses&&!inventory.backend.owner); // Next intent checks real inventory again.
    IdleFixture gap;gap.run();gap.next(100000001);gap.run();
    assert(gap.driver.diagnostic().decoded==unsigned(gears_torque_bow_controls::Status::invalid));
    assert(gap.driver.diagnostic().reason==4&&!gap.backend.presses); // Original100ms gap is still rejected.
}
}
int main(){
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    unchanged_idle_and_new_press();action_boundaries_keep_second_read();idle_never_hides_loss_or_grants_later_fire();
    std::puts("Bow idle observation: no-action snapshot reduction; fresh R2/UP/R3/held/left-grip/cohort retain second read; refusal, inventory change and100ms gap never grant fire. Injected backend only.");
}
