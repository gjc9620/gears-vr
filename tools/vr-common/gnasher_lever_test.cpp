#include "gnasher_lever.h"
#include "weapon_reload_model.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;

int main(){
    constexpr auto travel=original_gnasher_lever_travel;
    assert(travel.valid());
    constexpr ReloadPolicy shotgun{LoadUnit::round,8,3,true,true,100,
        MechanismAction::lever_down_return,MechanismAction::lever_down_return};
    ReloadModel gun;
    assert(gun.begin(10,shotgun,6,2,1000)==ReloadEvent::started);
    for(int i=0;i<2;++i){assert(gun.grab(10)==ReloadEvent::grabbed);
        const auto event=gun.insert(10,1010+i*10);
        assert(event==(i?ReloadEvent::mechanism_needed:ReloadEvent::inserted));}
    assert(!gun.can_fire()&&gun.pending_action()==MechanismAction::lever_down_return);
    GnasherLever lever;
    assert(lever.arm(10,3,4));
    LeverPoseSample sample{10,3,4,1,1000000000,0,true,true,true};
    assert(!lever.observe(sample,travel));
    const double path[]={.1,.2,.34,.48,.34,.2,.1,0};
    bool complete=false;
    for(double angle:path){sample.sequence++;sample.monotonic_ns+=20000000;
        sample.lever_angle_radians=angle;complete=lever.observe(sample,travel);}
    assert(complete&&lever.phase()==LeverPhase::unarmed);
    assert(gun.cycle(10,1030,MechanismAction::lever_down_return)==ReloadEvent::perfect);
    assert(gun.can_fire()&&gun.after_shot(10,7)&&!gun.can_fire());
    assert(lever.arm(10,3,4));sample.sequence++;sample.monotonic_ns+=20000000;
    assert(!lever.observe(sample,travel)); // Re-arm starts a new complete stroke.
    sample.sequence++;sample.monotonic_ns+=20000000;sample.lever_angle_radians=0;
    assert(!lever.observe(sample,travel));
    sample.sequence++;sample.monotonic_ns+=20000000;sample.lever_angle_radians=.8;
    assert(!lever.observe(sample,travel)&&lever.phase()==LeverPhase::unarmed);
    // The original mechanism reaches only about .566 rad; an overswing
    // beyond the calibrated ceiling cannot count as a physical cycle.
    assert(lever.arm(10,3,4));
    const double overswing[]={0,.1,.2,.3,.4,.5,.6,.72};
    for(double angle:overswing){
        sample.sequence++;sample.monotonic_ns+=20000000;
        sample.lever_angle_radians=angle;
        assert(!lever.observe(sample,travel));
    }
    assert(lever.phase()==LeverPhase::unarmed);
    sample.left_front_held=false;sample.sequence++;sample.monotonic_ns+=20000000;
    assert(!lever.observe(sample,travel)&&lever.phase()==LeverPhase::unarmed);
    assert(!gun.can_fire());
    assert(lever.arm(10,3,4));sample.left_front_held=true;sample.space_epoch=5;
    sample.sequence++;sample.monotonic_ns+=20000000;
    assert(!lever.observe(sample,travel)&&lever.phase()==LeverPhase::unarmed);
    assert(lever.arm(10,3,5));sample.lever_angle_radians=0;
    sample.sequence++;sample.monotonic_ns+=20000000;
    assert(!lever.observe(sample,travel));
    complete=false;
    for(double angle:path){sample.sequence++;sample.monotonic_ns+=20000000;
        sample.lever_angle_radians=angle;complete=lever.observe(sample,travel);}
    assert(complete&&gun.cycle(10,1070,MechanismAction::lever_down_return)==ReloadEvent::cycled);
    assert(gun.can_fire());
    std::puts("Gnasher bilateral lever travel, shot gate, jump and tracking boundaries passed");
}
