#include "weapon_reload_model.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;

int main(){
    constexpr ReloadPolicy shotgun{LoadUnit::round,8,3,true,true,100,
        MechanismAction::lever_down_return,MechanismAction::lever_down_return};
    ReloadModel short_reload;
    assert(short_reload.begin(11,shotgun,6,5,1000)==ReloadEvent::started);
    assert(short_reload.target()==2); // Missing fewer than three shells.
    assert(short_reload.grab(11)==ReloadEvent::grabbed);
    assert(short_reload.insert(11,1020)==ReloadEvent::inserted);
    assert(short_reload.grab(11)==ReloadEvent::grabbed);
    assert(short_reload.insert(11,1030)==ReloadEvent::mechanism_needed);
    assert(short_reload.pending_action()==MechanismAction::lever_down_return);
    assert(short_reload.cycle(11,1039,MechanismAction::bolt)==ReloadEvent::rejected);
    assert(short_reload.cycle(11,1040,MechanismAction::lever_down_return)==ReloadEvent::perfect);
    assert(short_reload.cycle(11,1041,MechanismAction::lever_down_return)==ReloadEvent::rejected);
    assert(short_reload.loaded()==8&&short_reload.reserve()==3&&short_reload.total()==11);
    assert(short_reload.can_fire());
    assert(short_reload.after_shot(11,7)&&!short_reload.can_fire());
    assert(short_reload.pending_action()==MechanismAction::lever_down_return);
    assert(short_reload.cycle(11,1050,MechanismAction::lever_down_return)==ReloadEvent::cycled&&short_reload.can_fire());

    ReloadModel already_equipped;
    assert(already_equipped.prime(51,shotgun,3,4));
    assert(already_equipped.can_fire());
    assert(already_equipped.after_shot(51,2)&&!already_equipped.can_fire());
    assert(!already_equipped.prime(51,shotgun,2,4)); // Cannot erase the pending lever.
    assert(already_equipped.pending_action()==MechanismAction::lever_down_return);
    assert(already_equipped.cycle(51,1050,MechanismAction::bolt)==ReloadEvent::rejected);
    assert(already_equipped.cycle(51,1050,MechanismAction::lever_down_return)==ReloadEvent::cycled);
    assert(already_equipped.can_fire()&&already_equipped.loaded()==2);
    assert(!already_equipped.after_shot(52,1));
    assert(already_equipped.after_shot(51,1)&&!already_equipped.can_fire());
    already_equipped.reset();
    auto deadline_unknown=shotgun;deadline_unknown.perfect_end_ticks=0;
    assert(already_equipped.prime(52,deadline_unknown,1,0));
    assert(already_equipped.can_fire()&&already_equipped.after_shot(52,0));
    assert(!already_equipped.can_fire());
    assert(already_equipped.cycle(52,1051,MechanismAction::lever_down_return)==ReloadEvent::cycled);
    assert(already_equipped.begin(52,deadline_unknown,0,1,1052)==ReloadEvent::invalid);

    ReloadModel sparse;
    assert(sparse.begin(12,shotgun,2,1,1000)==ReloadEvent::started);
    assert(sparse.target()==1); // Reserve fewer than three shells.
    assert(sparse.grab(12)==ReloadEvent::grabbed);
    assert(sparse.total()==3&&sparse.loaded()==2&&sparse.reserve()==0&&sparse.held()==1);
    assert(sparse.insert(12,1010)==ReloadEvent::mechanism_needed);
    assert(sparse.cycle(12,1100,MechanismAction::lever_down_return)==ReloadEvent::ordinary); // Deadline is exclusive.
    assert(sparse.total()==3);

    constexpr ReloadPolicy magazine{LoadUnit::magazine,30,30,true,false,250};
    ReloadModel rifle;
    assert(rifle.begin(20,magazine,5,18,500)==ReloadEvent::started);
    assert(!rifle.feed_present()&&!rifle.can_fire()&&rifle.loaded()==0&&rifle.reserve()==23);
    assert(rifle.grab(20)==ReloadEvent::grabbed);
    assert(rifle.held()==23&&rifle.loaded()==0&&rifle.total()==23);
    assert(rifle.grab(20)==ReloadEvent::rejected); // No duplicate magazine.
    assert(rifle.insert(21,510)==ReloadEvent::rejected); // Wrong weapon cannot consume it.
    assert(rifle.release(20)==ReloadEvent::released);
    assert(rifle.reserve()==23&&rifle.held()==0&&rifle.total()==23);
    assert(rifle.grab(20)==ReloadEvent::grabbed);
    assert(rifle.insert(20,520)==ReloadEvent::mechanism_needed);
    assert(rifle.loaded()==23&&rifle.reserve()==0&&rifle.total()==23);
    assert(rifle.cycle(20,600,MechanismAction::bolt)==ReloadEvent::perfect);
    assert(rifle.can_fire());
    assert(rifle.begin(22,magazine,1,2,700)==ReloadEvent::rejected);
    rifle.reset();
    assert(rifle.begin(22,magazine,1,2,700)==ReloadEvent::started);

    ReloadModel interrupted_magazine;
    assert(interrupted_magazine.begin(21,magazine,10,5,100)==ReloadEvent::started);
    assert(interrupted_magazine.cancel(21)==ReloadEvent::cancelled);
    assert(interrupted_magazine.total()==15&&!interrupted_magazine.feed_present());
    assert(!interrupted_magazine.can_fire()&&interrupted_magazine.cycle(21,120,MechanismAction::bolt)==ReloadEvent::rejected);
    assert(interrupted_magazine.begin(21,magazine,0,15,200)==ReloadEvent::started);
    assert(interrupted_magazine.grab(21)==ReloadEvent::grabbed);
    assert(interrupted_magazine.insert(21,220)==ReloadEvent::mechanism_needed);
    assert(interrupted_magazine.cycle(21,230,MechanismAction::bolt)==ReloadEvent::perfect);

    ReloadModel cancelled;
    assert(cancelled.begin(30,shotgun,3,4,800)==ReloadEvent::started);
    assert(cancelled.grab(30)==ReloadEvent::grabbed);
    assert(cancelled.insert(30,820)==ReloadEvent::inserted);
    assert(cancelled.grab(30)==ReloadEvent::grabbed);
    assert(cancelled.cancel(30)==ReloadEvent::cancelled);
    assert(cancelled.loaded()==4&&cancelled.reserve()==3&&cancelled.held()==0&&cancelled.total()==7);
    assert(cancelled.mechanism_pending()&&!cancelled.can_fire());
    assert(cancelled.cycle(30,830,MechanismAction::lever_down_return)==ReloadEvent::cycled&&cancelled.can_fire());
    assert(cancelled.begin(30,shotgun,cancelled.loaded(),cancelled.reserve(),1000)==ReloadEvent::started);
    assert(cancelled.grab(30)==ReloadEvent::grabbed);
    assert(cancelled.insert(30,1100)==ReloadEvent::inserted);
    assert(cancelled.grab(30)==ReloadEvent::grabbed);
    assert(cancelled.insert(30,1101)==ReloadEvent::inserted);
    assert(cancelled.grab(30)==ReloadEvent::grabbed);
    assert(cancelled.insert(30,1102)==ReloadEvent::mechanism_needed);
    assert(cancelled.cycle(30,1103,MechanismAction::lever_down_return)==ReloadEvent::ordinary);
    assert(cancelled.total()==7);

    ReloadModel empty,full;
    assert(empty.begin(40,shotgun,3,0,0)==ReloadEvent::no_ammo);
    assert(full.begin(41,shotgun,8,2,0)==ReloadEvent::full);
    ReloadPolicy broken=shotgun;broken.shot_action=MechanismAction::none;
    assert(empty.begin(40,broken,3,2,0)==ReloadEvent::invalid);
    assert(empty.stage()==ReloadStage::idle&&full.stage()==ReloadStage::idle);
    std::puts("reload count conservation, equipped shotgun priming, exclusive deadline and interruption checks passed");
}
