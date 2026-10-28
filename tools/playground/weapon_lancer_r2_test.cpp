#include "weapon_lancer_r2.h"
#include <vector>
#define main weapon_lancer_r2_owned_fixture_tests
#include "weapon_rig_binding_test.cpp"
#undef main
namespace {
namespace lr=gears_lancer_r2;
using R2=gears_weapon::OriginalR2Action;using Result=gears_weapon::OriginalR2Status;
struct Trial {
    Fixture f;lr::Input input;lr::Game game{};std::int64_t stamp=now;
    std::vector<R2> calls;bool succeeds=true;
    explicit Trial(unsigned source=0):f(source){game.actors={f.hand.world,f.hand.controller,f.hand.pawn,f.hand.mesh,f.hand.weapon};
        game.known=game.lancer=game.main_hand_held=game.r2_known=true;f.hand.controllers.hands[1].trigger=0;}
    bool execute(R2 action,std::uint64_t pc){CHECK(pc==game.actors.controller.address);calls.push_back(action);
        if(succeeds)game.r2_held=action==R2::press;return succeeds;}
    Result advance(bool current=true,bool stopping=false,wh::Frame const* frame=nullptr){
        return input.advance(frame?frame:&f.hand,current,game,stamp,[&](R2 a,std::uint64_t p){return execute(a,p);},stopping);}
    Result yield(){return input.yield(game,[&](R2 a,std::uint64_t p){return execute(a,p);});}
    void next(std::int64_t delta=10000000){stamp+=delta;++f.hand.key.frame;
        ++f.hand.stereo.sequence;f.hand.controllers.sequence=f.hand.stereo.sequence;
        f.hand.now_ns=stamp;f.hand.stereo.received_ns=stamp-1000000;
        f.hand.controllers.received_ns=stamp-2000000;f.hand.controllers.sync_ns=stamp-3000000;}
    void press(){next();auto& h=f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;
        const auto result=advance();
        if(result!=Result::pressed||!input.owns_press())std::fprintf(stderr,"press diagnostic: result=%u source=%u calls=%zu checks=%u p=%u r=%u\n",
            unsigned(result),unsigned(input.source_status()),calls.size(),checks,h.pressed[gears_vr::control_trigger],h.released[gears_vr::control_trigger]);
        CHECK(result==Result::pressed&&input.owns_press());}
    void release(){next();auto& h=f.hand.controllers.hands[1];++h.released[gears_vr::control_trigger];h.trigger=0;
        CHECK(advance()==Result::released&&!input.owns_press());}
    void baseline(){CHECK(advance()==Result::baseline&&calls.empty());}
};
}
int main(){
    for(unsigned source=0;source<2;++source){Trial t(source);t.baseline();t.press();t.next();
        CHECK(t.advance()==Result::accepted&&t.calls.size()==1);t.release();CHECK(t.calls.size()==2);
        CHECK(t.input.has_actors()&&lr::same(t.input.actors(),t.game.actors)&&t.input.reset_drained());}
    {Trial t;t.baseline();t.next();auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];
        ++h.released[gears_vr::control_trigger];CHECK(t.advance()==Result::tapped&&t.calls.size()==2&&!t.input.owns_press());}
    for(unsigned loss=0;loss<5;++loss){Trial t;t.baseline();t.press();CHECK(!t.input.reset_drained());
        t.next();if(loss==1)t.f.hand.controllers.sync_ns=t.stamp-100000001;
        if(loss==2)t.f.hand.now_ns=t.stamp-100000001;
        if(loss==3)t.f.hand.stereo.received_ns=t.stamp-100000001;
        if(loss==4)t.f.hand.controllers.sync_ns=t.stamp+1;
        CHECK(t.advance(loss!=0)==Result::released&&!t.input.owns_press()&&t.calls.size()==2);
        t.next();CHECK(t.advance()==Result::baseline);t.next();CHECK(t.advance()==Result::blocked&&t.calls.size()==2);
        t.next();++t.f.hand.controllers.hands[1].released[gears_vr::control_trigger];t.f.hand.controllers.hands[1].trigger=0;
        CHECK(t.advance()==Result::accepted);t.press();CHECK(t.calls.size()==3);}
    for(unsigned boundary=0;boundary<3;++boundary){Trial t;t.baseline();t.press();t.next();
        if(boundary==0)++t.f.hand.key.registration_cookie;if(boundary==1)++t.f.hand.key.lifecycle;
        if(boundary==2)++t.f.hand.key.native_started_qpc;
        CHECK(t.advance()==Result::released&&t.input.source_status()==lr::SourceStatus::cohort_changed);
        t.next();CHECK(t.advance()==Result::baseline);t.next();CHECK(t.advance()==Result::blocked&&t.calls.size()==2);}
    for(unsigned blocked=0;blocked<3;++blocked){Trial t;t.baseline();
        if(blocked==0)t.game.busy=true;if(blocked==1)t.game.animation=true;if(blocked==2)t.game.main_hand_held=false;
        t.next();++t.f.hand.controllers.hands[1].pressed[gears_vr::control_trigger];t.f.hand.controllers.hands[1].trigger=1;
        CHECK(t.advance()==Result::blocked&&t.calls.empty());}
    {Trial t;t.baseline();t.game.r2_held=true;t.next();++t.f.hand.controllers.hands[1].pressed[gears_vr::control_trigger];
        CHECK(t.advance()==Result::suppressed&&!t.input.owns_press());t.yield();CHECK(t.calls.empty());}
    for(unsigned changed=0;changed<4;++changed){Trial t;t.baseline();t.press();t.next();
        if(changed==0)++t.game.actors.weapon.name_number;if(changed==1)++t.game.actors.mesh.name_number;
        if(changed==2)t.game.r2_known=false;if(changed==3)t.game.lancer=false;
        CHECK(t.advance()==Result::abort_required&&t.input.owns_press()&&t.calls.size()==1&&!t.input.reset_drained());}
    {Trial t;t.baseline();t.press();t.succeeds=false;
        CHECK(t.yield()==Result::abort_required&&t.input.owns_press()&&t.calls.size()==2&&!t.input.reset_drained());}
    {Trial t;t.baseline();t.press();CHECK(t.advance(false,true)==Result::released&&!t.input.owns_press());}
    {Trial t;t.game={};CHECK(t.advance(false,true)==Result::accepted&&t.calls.empty());}
    {Trial t;t.baseline();t.press();t.game={};CHECK(t.advance(false,true)==Result::abort_required&&t.input.owns_press()&&t.calls.size()==1);}
    {Trial t;t.baseline();const auto old=t.f.hand.controllers.sync_ns;t.next();t.f.hand.controllers.sync_ns=old;
        t.advance();CHECK(t.input.source_status()==lr::SourceStatus::sync_invalid&&t.calls.empty());}
    {Trial t;t.baseline();t.next();--t.f.hand.controllers.sequence;t.advance();
        CHECK(t.input.source_status()==lr::SourceStatus::inactive&&t.calls.empty());}
    {Trial t;t.baseline();t.next();t.advance();t.next();t.f.hand.stereo.sequence-=2;
        t.f.hand.controllers.sequence=t.f.hand.stereo.sequence;t.advance();
        CHECK(t.input.source_status()==lr::SourceStatus::regressed&&t.calls.empty());}
    for(std::int64_t age:{100000000LL,100000001LL}){Trial t;
        t.f.hand.now_ns=t.stamp-age;t.f.hand.controllers.sync_ns=t.stamp-age;
        const auto result=t.advance();CHECK((age==100000000?result==Result::baseline:t.input.source_status()==lr::SourceStatus::stale)&&t.calls.empty());}
    {Trial t;t.baseline();t.press();CHECK(t.yield()==Result::released);t.next();t.f.hand.controllers.hands[1].trigger=0;
        ++t.f.hand.controllers.hands[1].released[gears_vr::control_trigger];
        CHECK(t.advance()==Result::baseline&&t.calls.size()==2);t.press();CHECK(t.calls.size()==3);}
    {Trial t;t.baseline();CHECK(t.input.reset_drained());t.next();
        ++t.f.hand.controllers.hands[1].pressed[gears_vr::control_trigger];t.f.hand.controllers.hands[1].trigger=1;
        CHECK(t.advance()==Result::baseline);t.next();CHECK(t.advance()==Result::accepted&&t.calls.empty());}
    {Trial t;t.baseline();t.press();t.game.busy=true;t.next();
        CHECK(t.advance()==Result::released&&!t.input.owns_press()&&t.calls.size()==2);}
    for(std::int64_t elapsed:{99000000LL,99000001LL}){Trial t;t.baseline();t.next(elapsed);t.advance();
        CHECK(t.input.source_status()==(elapsed==99000000?lr::SourceStatus::ready:lr::SourceStatus::sync_invalid)&&t.calls.empty());}
    // A real new edge with bounded producer spacing remains valid even when
    // consumer work makes the previous sample older than100ms. No replay:
    // the current sample itself is43ms old, then its explicit release is owed.
    for(unsigned source=0;source<2;++source){Trial t(source);t.baseline();t.next(70000000);
        auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;
        t.stamp+=40000000;
        CHECK(t.advance()==Result::pressed&&t.input.owns_press()&&t.calls.size()==1);
        CHECK(t.input.source_status()==lr::SourceStatus::ready);t.release();CHECK(t.calls.size()==2);}
    {Trial t;t.baseline();t.press();const auto owner=t.game.actors.controller.address;
        const auto source=t.input.source_status();
        CHECK(!lr::tick_admitted(false,owner,owner,owner));
        CHECK(!lr::tick_admitted(true,owner+8,owner,owner));
        CHECK(!lr::tick_admitted(true,owner+8,0,owner));
        CHECK(t.input.owns_press()&&t.calls.size()==1&&t.input.source_status()==source);
        CHECK(lr::tick_admitted(true,owner,owner,owner));t.release();CHECK(t.calls.size()==2);
        CHECK(!lr::tick_admitted(true,0,0,0));}
    {Trial t;t.baseline();t.press();const auto previous=t.game.actors.controller.address;
        t.game.actors.controller.address+=8;t.f.hand.controller=t.game.actors.controller;
        CHECK(lr::tick_admitted(true,t.game.actors.controller.address,t.f.hand.controller.address,previous));
        t.next();CHECK(t.advance()==Result::abort_required&&t.input.owns_press()&&t.calls.size()==1);}
    {Trial t;t.baseline();t.next();++t.f.hand.controllers.hands[1].pressed[gears_vr::control_trigger];
        t.f.hand.controllers.hands[1].trigger_active=0;
        CHECK(t.advance()==Result::blocked&&!t.input.owns_press()&&t.calls.empty());}
    {Trial t;t.baseline();t.press();t.next();t.f.hand.controllers.hands[1].trigger=0;
        t.f.hand.controllers.hands[1].trigger_active=0;
        CHECK(t.advance()==Result::released&&!t.input.owns_press()&&t.calls.size()==2);}
    for(unsigned source=0;source<2;++source){
        // DW: a second DOWN arrives after a previous-sync gap. It must never
        // replay when the new source establishes its baseline while DOWN.
        Trial t(source);t.baseline();t.press();t.release();
        const auto prior=t.input.last_sync_ns();t.next(100000001);
        ++t.f.hand.controllers.hands[1].pressed[gears_vr::control_trigger];t.f.hand.controllers.hands[1].trigger=1;
        CHECK(t.advance()==Result::accepted&&t.input.source_status()==lr::SourceStatus::sync_invalid);
        CHECK(t.calls.size()==2&&!t.input.owns_press()&&t.input.last_sync_ns()==prior);
        t.next();CHECK(t.advance()==Result::baseline);t.next();CHECK(t.advance()==Result::blocked&&t.calls.size()==2);
        t.next();++t.f.hand.controllers.hands[1].released[gears_vr::control_trigger];t.f.hand.controllers.hands[1].trigger=0;
        CHECK(t.advance()==Result::accepted&&t.calls.size()==2);t.press();
        CHECK(t.calls.size()==3&&t.input.last_observed_ns()==t.stamp);
        CHECK(t.advance(false,true)==Result::released&&!t.input.owns_press()&&t.calls.size()==4);
    }
    std::printf("WeaponLancerR2Test: %u checks passed; wrapper + original ledger, owned data only\n",checks);return 0;
}
