#include "weapon_longshot_scope_control.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace gears_weapon::longshot_scope_control;
namespace {
unsigned checks{};
void check(bool value,char const* message){
    ++checks;if(!value){std::fprintf(stderr,"Longshot scope control: %s\n",message);std::abort();}
}
struct Fixture {
    Control control;
    Frame frame{};
    Fixture(){
        frame.stamp={{1,2,3,4,5,6,7,8,9},1,1000000000,0,true,true,true};
        frame.eyes[0]={.07,0,0,true};frame.click_active=frame.scope_ready=true;
    }
    Result tick(std::int64_t delta=10000000){
        ++frame.stamp.sequence;frame.stamp.observed_ns+=delta;
        return control.observe(frame,frame.stamp.observed_ns);
    }
    void ready(){check(!tick().near_eye,"first frame primes input");tick();
        check(tick(80000000).near_eye,"stable eye box enters after 80ms");}
    Result press(){++frame.pressed;frame.click_down=true;return tick();}
    Result release(){++frame.released;frame.click_down=false;return tick();}
};
void geometry(){
    for(auto d:{.04,.07,.10})check(in_eye_box({d,.025,15,true},false),"inclusive entry bounds");
    check(!in_eye_box({.039,0,0,true},false),"too close");
    check(!in_eye_box({.101,0,0,true},false),"too far for entry");
    check(in_eye_box({.13,.04,22,true},true),"retention box");
    check(!in_eye_box({-.07,0,0,true},true),"wrong side");
    check(!in_eye_box({.07,std::numeric_limits<double>::quiet_NaN(),0,true},false),"invalid geometry");
    gears_vr::Pose ocular{},eye{};eye.position.z=.07;
    const auto e=contact(ocular,eye,true);
    check(e.tracked&&std::abs(e.distance_m-.07)<1e-12&&e.angle_degrees==0,"ocular frame geometry");
    eye.position.x=.03;check(!in_eye_box(contact(ocular,eye,true),false),"lateral offset");
}
void clicks(){
    Fixture f;f.ready();check(f.press().switch_magnification,"near press dispatches");
    check(f.tick().consume_click&&!f.tick().switch_magnification,"held click retained without repeats");
    check(!f.release().consume_click,"release closes consumption");
    check(f.press().switch_magnification,"next press dispatches");
    f.frame.eyes[0].distance_m=.2;
    auto left=f.tick();check(!left.near_eye&&left.consume_click&&!left.switch_magnification,"exit retains consumed press");
    check(!f.release().consume_click,"release outside restores routing");
    check(!f.press().switch_magnification&&!f.tick().consume_click,"outside click belongs to other mapping");
    f.frame.eyes[0].distance_m=.07;f.tick();
    check(!f.tick(80000000).switch_magnification,"entering while held cannot zoom");
    f.release();check(f.press().switch_magnification,"fresh press after held entry");
}
void interruptions(){
    Fixture f;f.ready();f.frame.scope_ready=false;
    auto pending=f.press();check(pending.consume_click&&!pending.switch_magnification,"preparing scope consumes without dispatch");
    f.frame.scope_ready=true;check(!f.tick().switch_magnification,"ready cannot replay held press");
    f.release();check(f.press().switch_magnification,"new ready press");
    f.frame.stamp.right_tracked=false;
    check(f.tick().consume_click,"tracking loss retains consumed press");
    f.frame.stamp.right_tracked=true;
    check(!f.tick().switch_magnification,"tracking recovery primes held click");
    f.release();f.tick();f.tick(80000000);
    f.frame.menu=true;check(!f.press().switch_magnification,"menu prevents scope dispatch");
    f.frame.menu=false;f.tick();check(!f.tick(80000000).switch_magnification,"menu exit does not replay");
    f.release();check(f.press().switch_magnification,"fresh menu-return press");
    ++f.frame.stamp.identity.weapon;check(!f.tick().switch_magnification,"switching weapon primes input");
    f.frame.reloading=true;check(!f.tick().near_eye,"reload exits scope");
}
void freshness(){
    Fixture f;f.ready();check(f.press().switch_magnification,"initial dispatch");
    check(!f.control.observe(f.frame,f.frame.stamp.observed_ns).switch_magnification,"same frame cannot replay");
    check(!f.control.observe(f.frame,f.frame.stamp.observed_ns+100000001).near_eye,"expired frame exits");
    f.release();f.tick();f.tick(80000000);
    f.frame.pressed+=2;f.frame.released+=1;f.frame.click_down=true;
    check(!f.tick().switch_magnification,"skipped multiple presses cannot replay");
    f.release();check(!f.tick(200000000).near_eye,"poll gap restarts dwell");
}
void cached_exits(){
    for(unsigned reason=0;reason<3;++reason){
        Fixture f;f.ready();check(f.press().switch_magnification,"near press before cached exit");
        const auto sequence=f.frame.stamp.sequence;
        if(reason==0)f.frame.menu=true;
        if(reason==1)f.frame.reloading=true;
        if(reason==2)f.frame.eyes[0].distance_m=.2;
        const auto result=f.control.observe(f.frame,f.frame.stamp.observed_ns);
        check(!result.near_eye&&!result.switch_magnification,"same-source pause/reload/eye exit closes scope");
        check(result.consume_click,"cached exit retains already consumed press");
        check(f.frame.stamp.sequence==sequence,"exit does not fabricate producer sample");
        f.frame.menu=f.frame.reloading=false;f.frame.eyes[0].distance_m=.07;
        check(!f.control.observe(f.frame,f.frame.stamp.observed_ns).near_eye,"same-source return cannot reopen scope");
        check(!f.tick().switch_magnification,"held return cannot replay zoom");
        f.tick(80000000);f.release();check(f.press().switch_magnification,"fresh dwell/release/press restores zoom");
    }
    Fixture f;f.ready();check(f.press().switch_magnification,"near press before regression");
    auto old=f.frame;--old.stamp.sequence;--old.stamp.observed_ns;
    const auto result=f.control.observe(old,f.frame.stamp.observed_ns);
    check(!result.near_eye&&!result.switch_magnification&&result.consume_click,"regressed observation cannot retain scope");
    check(!f.control.observe(f.frame,f.frame.stamp.observed_ns).near_eye,"rejected older sample cannot renew original observation");
    {Fixture entering;entering.tick();entering.tick();
        entering.frame.eyes[0].distance_m=.2;
        check(!entering.control.observe(entering.frame,entering.frame.stamp.observed_ns).near_eye,"cached eye loss clears pending dwell");
        entering.frame.eyes[0].distance_m=.07;
        check(!entering.tick(80000000).near_eye,"first fresh return starts a new dwell");
        check(entering.tick(80000000).near_eye,"fresh continuous dwell can enter");}
    {Fixture collision;collision.ready();check(collision.press().switch_magnification,"press before colliding counters");
        auto mixed=collision.frame;++mixed.pressed;++mixed.released;
        const auto rejected=collision.control.observe(mixed,mixed.stamp.observed_ns);
        check(!rejected.near_eye&&rejected.consume_click&&!rejected.switch_magnification,"same-source changed counters reject without release");
        collision.release();collision.tick(80000000);
        check(collision.press().switch_magnification,"colliding counters do not poison original high-water marks");}
}
}
int main(){
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    geometry();clicks();interruptions();freshness();cached_exits();
    std::printf("Longshot eye-box and R3 routing: %u checks passed\n",checks);}
