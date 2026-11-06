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
    Result advance(bool current=true,bool stopping=false,wh::Frame const* frame=nullptr,lr::Continuity const* continuity=nullptr){
        return input.advance(frame?frame:&f.hand,current,game,stamp,[&](R2 a,std::uint64_t p){return execute(a,p);},stopping,continuity);}
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
lr::Continuity witness(Trial const& t){return {{t.f.hand.controllers,1,{1,1}},{100,200}};}
}
int main(){
    // The actual equipment adapter seals this ledger before index acquisition.
    // Attachment completion with that trigger still down never fires; an
    // authentic release/new press returns to ordinary held-trigger firing.
    for(unsigned cold=0;cold<2;++cold){Trial t;if(!cold)t.baseline();
        t.game.main_hand_held=false;t.next();auto& h=t.f.hand.controllers.hands[1];
        ++h.pressed[gears_vr::control_trigger];h.trigger=1;
        CHECK(t.yield()!=Result::abort_required&&t.calls.empty());
        t.game.main_hand_held=true;
        for(unsigned i=0;i<5;++i){t.next();t.advance();CHECK(t.calls.empty()&&!t.input.owns_press());}
        t.next();++h.released[gears_vr::control_trigger];h.trigger=0;t.advance();
        t.press();CHECK(t.calls.size()==1);t.release();CHECK(t.calls.size()==2);
    }
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
    for(unsigned source=0;source<2;++source){
        // Actual49768 timing shape: current source stays fresh, producer
        // sync-to-sync100.0004ms, consumer interval93ms. A matched continuous
        // publisher is required; no relaxed100ms deadline or repeated edge.
        Trial t(source);t.f.hand.controllers.sync_ns=t.stamp-54000000;
        auto proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::baseline);
        const auto old=t.f.hand.controllers.sync_ns;t.next(93000000);
        t.f.hand.controllers.sync_ns=old+100000400;t.f.hand.controllers.received_ns=t.f.hand.controllers.sync_ns+1000000;
        auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;
        proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::pressed&&t.calls.size()==1&&t.input.continuous_poll_gap());
        t.next();proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::accepted&&t.calls.size()==1&&!t.input.continuous_poll_gap());
        t.next();++h.released[gears_vr::control_trigger];h.trigger=0;proof=witness(t);
        CHECK(t.advance(true,false,nullptr,&proof)==Result::released&&t.calls.size()==2);
    }
    for(unsigned bad=0;bad<8;++bad){Trial t;t.f.hand.controllers.sync_ns=t.stamp-54000000;
        auto proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::baseline);
        const auto old=t.f.hand.controllers.sync_ns;t.next(bad==5?200000001:93000000);
        t.f.hand.controllers.sync_ns=old+100000400;t.f.hand.controllers.received_ns=t.f.hand.controllers.sync_ns+1000000;
        auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;proof=witness(t);
        if(bad==0)++proof.publisher.pid;if(bad==1)++proof.publisher.start;
        if(bad==2)++proof.tracking.stream_epoch;if(bad==3)++proof.tracking.tracking_epoch[1];
        if(bad==4)proof.tracking.input.hands[1].trigger=0;
        if(bad==6){t.f.hand.controllers.sync_ns=t.stamp-100000001;proof=witness(t);}
        t.advance(true,false,nullptr,bad==7?nullptr:&proof);
        CHECK(t.calls.empty()&&!t.input.owns_press()&&t.input.source_status()!=lr::SourceStatus::ready&&!t.input.continuous_poll_gap());
    }
    for(const auto interval:{101000000LL,150000000LL,200000000LL,200000001LL}){
        Trial t;auto proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::baseline);
        t.next();auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;
        proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::pressed&&t.input.owns_press());
        t.next(interval);proof=witness(t);const auto result=t.advance(true,false,nullptr,&proof);
        if(interval<=200000000){
            CHECK(result==Result::accepted&&t.input.owns_press()&&t.calls.size()==1&&t.input.continuous_poll_gap());
            // A genuine subsequent UP must release normally, without replay.
            t.next();++h.released[gears_vr::control_trigger];h.trigger=0;proof=witness(t);
            CHECK(t.advance(true,false,nullptr,&proof)==Result::released&&t.calls.size()==2);
        }else CHECK(result==Result::released&&!t.input.owns_press()&&t.calls.size()==2&&
                   t.input.source_status()==lr::SourceStatus::sync_invalid);
    }
    {Trial t;auto proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::baseline);
        t.next();auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;
        proof=witness(t);++proof.tracking.tracking_epoch[0];
        CHECK(t.advance(true,false,nullptr,&proof)==Result::pressed&&t.calls.size()==1); // independent left loss
        t.next();proof=witness(t);++proof.tracking.tracking_epoch[1];
        CHECK(t.advance(true,false,nullptr,&proof)==Result::released&&!t.input.owns_press()&&t.calls.size()==2);}
    for(unsigned source=0;source<2;++source){
        Trial t(source);t.baseline();t.game.press_ready=false;
        t.next(100000001);t.advance();CHECK(t.input.source_status()==lr::SourceStatus::sync_invalid);
        t.next();CHECK(t.advance()==Result::baseline&&t.calls.empty()&&!t.input.awaiting_release());
        // Idle has no muzzle, but the next new DOWN uses an actual ready ray.
        t.game.press_ready=true;t.press();t.release();CHECK(t.calls.size()==2);
        t.game.press_ready=false;t.next();auto& h=t.f.hand.controllers.hands[1];
        ++h.pressed[gears_vr::control_trigger];++h.released[gears_vr::control_trigger];
        CHECK(t.advance()==Result::baseline&&t.calls.size()==2); // unseen tap is never replayed
        t.game.press_ready=true;t.next();CHECK(t.advance()==Result::accepted&&t.calls.size()==2);t.press();t.release();
    }
    {Trial t;t.baseline();t.game.press_ready=false;t.next();auto& h=t.f.hand.controllers.hands[1];
        ++h.pressed[gears_vr::control_trigger];h.trigger=1;
        CHECK(t.advance()==Result::blocked&&t.calls.empty());t.game.press_ready=true;t.next();
        CHECK(t.advance()!=Result::pressed&&t.calls.empty()&&!t.input.owns_press()); // no late replay while held
        t.next();++h.released[gears_vr::control_trigger];h.trigger=0;t.advance();t.press();}
    {Trial t;t.baseline();t.press();t.game.press_ready=false;t.next();
        CHECK(t.advance()==Result::released&&!t.input.owns_press()&&t.calls.size()==2);}
    for(unsigned source=0;source<2;++source){
        Trial t(source);auto proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::baseline);
        t.game.defer_new_press_for_muzzle=true;t.game.press_ready=false;t.next();
        auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;proof=witness(t);
        CHECK(t.advance(true,false,nullptr,&proof)==Result::deferred&&t.calls.empty()&&!t.input.owns_press());
        t.next();proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::deferred&&t.calls.empty());
        t.next();proof=witness(t);t.game.press_ready=true;
        CHECK(t.advance(true,false,nullptr,&proof)==Result::pressed&&t.input.owns_press()&&t.calls.size()==1);
        t.next();proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::accepted&&t.calls.size()==1);
        t.release();CHECK(t.calls.size()==2);
    }
    for(const auto delay:{127000000LL,127000001LL}){
        Trial t;auto proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::baseline);
        t.game.defer_new_press_for_muzzle=true;t.game.press_ready=false;t.next();
        auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;proof=witness(t);
        CHECK(t.advance(true,false,nullptr,&proof)==Result::deferred&&t.calls.empty());
        // Fresh samples can carry this SAME held intent past100ms without
        // moving its original250ms deadline. No retained Frame is accepted.
        t.next(120000000);proof=witness(t);
        CHECK(t.advance(true,false,nullptr,&proof)==Result::deferred&&t.calls.empty());
        t.next(delay);proof=witness(t);t.game.press_ready=true;const auto result=t.advance(true,false,nullptr,&proof);
        if(delay==127000000)CHECK(result==Result::pressed&&t.calls.size()==1);
        else {CHECK(result==Result::blocked&&t.calls.empty()&&!t.input.owns_press());
            t.next();proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::accepted&&t.calls.empty());
            t.next();++h.released[gears_vr::control_trigger];h.trigger=0;proof=witness(t);t.advance(true,false,nullptr,&proof);t.press();}
    }
    for(unsigned loss=0;loss<8;++loss){
        Trial t;auto proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::baseline);
        t.game.defer_new_press_for_muzzle=true;t.game.press_ready=false;t.next();
        auto& h=t.f.hand.controllers.hands[1];++h.pressed[gears_vr::control_trigger];h.trigger=1;proof=witness(t);
        CHECK(t.advance(true,false,nullptr,&proof)==Result::deferred);
        t.next();proof=witness(t);t.game.press_ready=true;
        if(loss==0)++proof.tracking.tracking_epoch[1];if(loss==1)++proof.publisher.start;
        if(loss==2)++t.f.hand.key.registration_cookie;if(loss==3)t.game.busy=true;
        if(loss==4){++h.released[gears_vr::control_trigger];h.trigger=0;proof=witness(t);}
        if(loss==5){++h.released[gears_vr::control_trigger];++h.pressed[gears_vr::control_trigger];proof=witness(t);}
        const auto result=t.advance(loss!=6,false,nullptr,loss==7?nullptr:&proof);
        CHECK(result!=Result::pressed&&t.calls.empty()&&!t.input.owns_press());
        t.game.busy=false;t.next();proof=witness(t);t.advance(true,false,nullptr,&proof);
        CHECK(t.calls.empty()); // Recovery never replays the cancelled edge.
    }
    {Trial t;auto proof=witness(t);CHECK(t.advance(true,false,nullptr,&proof)==Result::baseline);
        t.game.defer_new_press_for_muzzle=true;t.next();auto& h=t.f.hand.controllers.hands[1];
        ++h.pressed[gears_vr::control_trigger];h.trigger=1;bool ready_after_metadata=false;
        const auto advance=[&](){proof=witness(t);return t.input.advance(&t.f.hand,true,t.game,t.stamp,
            [&](R2 a,std::uint64_t pc){
                if(a==R2::press&&!ready_after_metadata)return gears_weapon::OriginalR2Execution::not_ready;
                return gears_weapon::r2_execution(t.execute(a,pc));
            },false,&proof);};
        CHECK(advance()==Result::deferred&&t.calls.empty());t.next();CHECK(advance()==Result::deferred&&t.calls.empty());
        ready_after_metadata=true;t.next();CHECK(advance()==Result::pressed&&t.calls.size()==1);t.release();
    }
    for(unsigned loss=0;loss<7;++loss){
        Trial t;t.baseline();t.press();t.game.press_ready=false;t.game.retain_owned_for_muzzle=true;t.next();
        if(loss==1)t.game.busy=true;if(loss==2)t.game.animation=true;if(loss==3)t.game.main_hand_held=false;
        if(loss==4)t.f.hand.controllers.sync_ns=t.stamp-100000001;
        if(loss==5)++t.f.hand.key.lifecycle;
        const auto result=t.advance(loss!=6);
        if(loss==0){CHECK(result==Result::accepted&&t.input.owns_press()&&t.calls.size()==1);t.release();}
        else CHECK(result==Result::released&&!t.input.owns_press()&&t.calls.size()==2);
    }
    std::printf("WeaponLancerR2Test: %u checks passed; wrapper + original ledger, owned data only\n",checks);return 0;
}
