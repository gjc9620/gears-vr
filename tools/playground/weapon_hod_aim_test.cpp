#define main hod_input_fixture_main
#include "weapon_hod_input_test.cpp"
#undef main
#include "../vr-common/original_l2_aim.h"
#define GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL 1
#include "weapon_hod_aim.inl"
struct AimFixture {
    Fixture f;
    AimFixture(){
        hod_aim::input.reset();hod_aim::journal={};hod_aim::retained=false;
        hod_aim::faulted=hod_aim::cancelling=false;aim_presses=aim_releases=0;aim_release_clears=true;
        auto& h=f.sample.source.controllers.hands[0];h.grip_active=1;h.grip_flags=15;h.grip.orientation.w=1;h.trigger_active=1;
        tick();assert(!hod_aim::owed()&&aim_presses==0);
    }
    void tick(){hod_aim::advance(f.sample,hod_state::actual);}
    void press(){f.next();++f.sample.source.controllers.hands[0].pressed[gears_vr::control_trigger];f.sample.source.controllers.hands[0].trigger=1;tick();}
    void release(){f.next();++f.sample.source.controllers.hands[0].released[gears_vr::control_trigger];f.sample.source.controllers.hands[0].trigger=0;tick();}
};
int main(){
    {AimFixture a;a.press();assert(aim_presses==1&&hod_aim::owed());a.release();assert(aim_releases==1&&!hod_aim::owed());}
    {AimFixture a;a.press();aim_release_clears=false;a.release();assert(hod_aim::owed()&&aim_releases==1);
        hod_aim::service();hod_aim::service();assert(aim_releases==1&&hod_aim::owed());
        hod_state::actual.aim_key=hod_state::actual.controller_aim=hod_state::actual.pawn_aim=false;
        hod_aim::service();assert(!hod_aim::owed());}
    {AimFixture a;a.press();current=false;hod_aim::cancel();assert(!hod_aim::owed()&&aim_releases==1);
        current=true;for(unsigned i=0;i<3;++i){a.f.next();a.tick();}assert(aim_presses==1);
        a.release();a.press();assert(aim_presses==2);}
    {AimFixture a;hod_state::actual.toggle_targeting=true;a.press();assert(aim_presses==0&&!hod_aim::owed());}
    {AimFixture a;a.press();auto foreign=a.f.sample.source;foreign.weapon=object(6);
        assert(hod_aim::yield_for_equipment(foreign)&&aim_releases==0);
        selected_weapon=6;hod_aim::cancel();assert(hod_aim::owed()&&aim_releases==0);}
    {AimFixture a;a.press();a.f.press();assert(api::owed());hod_aim::cancel();
        assert(!hod_aim::owed()&&api::owed()&&releases==0);api::cancel();assert(!api::owed());}
    std::puts("HOD aim composition: original L2, stop readback, loss/rearm, toggle rejection, foreign owner and R2 independence passed (synthetic only)");
}
