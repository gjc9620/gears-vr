#include "weapon_troika_fire_owner.h"
#include <cstdio>
#include <cstdlib>
#include <windows.h>
namespace f=gears_troika_fire;namespace g=gears_troika_geometry;namespace c=gears_troika_control;namespace v=gears_vr;namespace h=gears_weapon_hand;
unsigned checks{};
constexpr unsigned original_escape=0xe0427452u;
#define CHECK(x) do{++checks;if(!(x)){std::printf("FAIL %u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Fake final:f::Backend {
    f::Owner* owner{};bool ready=true,quiet=false,dead=false,escape_start=false,escape_stop=false;unsigned starts{},stops{},qualifications{};
    f::Dispatch start_result=f::Dispatch::returned,stop_result=f::Dispatch::returned;
    bool qualified(f::Run const&) noexcept override {++qualifications;return ready;}
    f::Dispatch start(f::Run const& run) noexcept override {CHECK(owner->owed()&&owner->stopping()&&owner->start_result()==f::Dispatch::uncertain&&run.serial&&owner->run().serial==run.serial);++starts;if(escape_start)RaiseException(original_escape,0,0,nullptr);return start_result;}
    f::Dispatch stop(f::Run const& run) noexcept override {CHECK(owner->owed()&&owner->stopping()&&owner->stop_result()==f::Dispatch::uncertain&&run.serial==owner->run().serial);++stops;if(escape_stop)RaiseException(original_escape,0,0,nullptr);return stop_result;}
    bool quiescent(f::Run const&) noexcept override {return quiet;}
    bool extinct(f::Run const&) noexcept override {return dead;}
};
struct Fixture {
    f::Owner owner;Fake backend;g::Source source{};c::Decision decision{};std::int64_t now=2000000000;
    Fixture(){backend.owner=&owner;auto& s=source.frame;
        source.entry_binding=source.entry_token=1;s.native={1,2,3,4,5};
        s.world={0x1000,0,0x9000,1,1,0,1};s.controller={0x2000,0,0x9000,2,2,0,1};
        s.driver={0x3000,0,0x9000,3,3,0,1};s.driver_mesh={0x4000,0x3000,0x9000,4,4,0,1};
        s.turret={0x5000,0,0x9000,5,5,0,1};s.turret_mesh={0x6000,0x5000,0x9000,6,6,0,1};s.weapon={0x7000,0x5000,0x9000,7,7,0,1};
        s.world_generation=s.reference_generation=1;s.native_reference_revision=2;s.native_reference={3,1,0};s.producer={4,{5,6}};
        s.publisher_pid=7;s.publisher_start=8;s.qpc_frequency=1000000000;s.control_policy=gears_troika_source::mounted_look_suppressed;
        auto& stereo=s.stereo;stereo.session=9;stereo.space_epoch=10;stereo.head_flags=stereo.view_flags=15;
        stereo.head={};
        stereo.focused=stereo.visible=stereo.should_render=1;
        for(unsigned i=0;i<2;++i){stereo.eyes[i].pose={};stereo.eyes[i].pose.position.x=i?.032:-.032;stereo.eyes[i].fov={-.7,.7,.6,-.6};}
        auto& input=s.controllers;input.session=9;input.space_epoch=10;input.profile_generation=11;input.input_epoch=12;input.focused=1;
        for(auto& hand:input.hands){hand.grip_active=hand.aim_active=hand.squeeze_active=hand.trigger_active=1;hand.grip_flags=hand.aim_flags=15;hand.squeeze=1;}
        decision.lifecycle=decision.press_token=1;decision.lease={h::LeaseKind::support,0,1,1};decision.status=h::DecisionStatus::retained;
        decision.release_required=0;decision.left_down=decision.left_known=decision.left_tracked=1;
        decision.right_down=decision.right_known=decision.right_owned=decision.trigger_known=decision.trigger_down=decision.both_owned=decision.fire_requested=1;
        next();fresh_edge();
    }
    void next(std::int64_t delta=10000000){now+=delta;auto& s=source.frame;
        ++s.publication_sequence;++s.source_ticket;++s.scene_invocation;s.scene_qpc=now-2000000;s.published_ns=now-1000000;
        auto& stereo=s.stereo;++stereo.sequence;stereo.predicted_time+=delta;stereo.received_ns=now-3000000;
        auto& input=s.controllers;input.sequence=stereo.sequence;input.predicted_time=stereo.predicted_time;input.received_ns=now-4000000;input.sync_ns=now-5000000;
        decision.query_id=s.publication_sequence;decision.publication_sequence=s.publication_sequence;decision.source_ticket=s.source_ticket;decision.decided_ns=now;
    }
    void fresh_edge(){auto& input=source.frame.controllers;auto& right=input.hands[1];
        right.last_squeeze_press_sequence=input.sequence;right.last_squeeze_press_flags=15;right.last_squeeze_press_grip=right.grip;
        decision.right_press_sequence=input.sequence;decision.right_window_begin_ns=input.sync_ns;
        decision.fire_requested=1;decision.fire_edge_sequence=input.sequence;decision.fire_window_begin_ns=input.sync_ns;}
    bool offer(){g::Request q{};q.source=source;q.query_id=decision.query_id;
        if(!c::valid(decision,q,now))std::printf("invalid check=%u publication=%llu edge=%llu right=%llu/%llu source=%u trigger=%u\n",checks,
            source.frame.publication_sequence,decision.fire_edge_sequence,decision.right_press_sequence,source.frame.controllers.hands[1].last_squeeze_press_sequence,
            unsigned(g::source_valid(source,now,false)),decision.trigger_down);
        CHECK(c::valid(decision,q,now));return owner.offer(source,decision,now,backend);}
    void release(){next();decision.fire_requested=0;decision.fire_edge_sequence=0;decision.fire_window_begin_ns=0;decision.trigger_down=0;CHECK(!offer());}
};
void escaped_offer(Fixture* x){__try{x->offer();CHECK(false);}__except(GetExceptionCode()==original_escape?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){}}
void escaped_stop(Fixture* x){__try{x->owner.service(x->backend);CHECK(false);}__except(GetExceptionCode()==original_escape?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){}}
int main(){
    {Fixture x;CHECK(x.offer()&&x.owner.owed()&&x.backend.starts==1);
        const auto first=x.owner.run();
        for(unsigned i=0;i<20;++i){x.next();CHECK(!x.offer()&&x.owner.owed()&&x.backend.starts==1);}
        CHECK(x.owner.run().control.fire_edge_sequence==first.control.fire_edge_sequence&&x.owner.run().control.fire_window_begin_ns==first.control.fire_window_begin_ns);
        x.backend.quiet=true;x.owner.service(x.backend);CHECK(x.owner.owed()); // Auto state pause is not release.
        x.backend.quiet=false;x.release();CHECK(x.owner.owed()&&x.owner.stopping()&&x.backend.stops==1);
        for(unsigned i=0;i<5;++i)x.owner.service(x.backend);
        CHECK(x.backend.stops==1);x.backend.quiet=true;x.owner.service(x.backend);CHECK(!x.owner.owed());
        x.next();x.decision=first.control;x.decision.publication_sequence=x.source.frame.publication_sequence;
        x.decision.source_ticket=x.source.frame.source_ticket;x.decision.query_id=x.source.frame.publication_sequence;x.decision.decided_ns=x.now;
        CHECK(!x.offer()&&x.backend.starts==1); // Same held edge cannot replay after closure.
        x.backend.quiet=false;x.next();x.fresh_edge();CHECK(x.offer()&&x.backend.starts==2);
    }
    for(auto result:{f::Dispatch::returned,f::Dispatch::uncertain}){
        Fixture x;x.backend.start_result=result;CHECK(x.offer()&&x.owner.owed());
        CHECK(x.owner.stopping()==(result==f::Dispatch::uncertain));x.owner.request_stop();
        x.backend.stop_result=f::Dispatch::uncertain;x.owner.service(x.backend);CHECK(x.owner.owed()&&x.backend.stops==1);
        for(unsigned i=0;i<6;++i)x.owner.service(x.backend);CHECK(x.backend.stops==1);
        x.backend.quiet=true;x.owner.service(x.backend);CHECK(!x.owner.owed());
    }
    {Fixture x;x.backend.start_result=f::Dispatch::not_dispatched;CHECK(!x.offer()&&!x.owner.owed()&&x.backend.starts==1);
        x.next();CHECK(!x.offer()&&x.backend.starts==1); // Consumed attempt does not retry.
    }
    {Fixture x;x.backend.ready=false;CHECK(!x.offer()&&!x.backend.starts);x.next(20000000);x.backend.ready=true;CHECK(x.offer());}
    {Fixture x;x.backend.ready=false;CHECK(!x.offer());x.next(100000001);x.backend.ready=true;CHECK(!x.offer()&&!x.backend.starts);
        x.next();CHECK(!x.offer()&&!x.backend.starts);x.fresh_edge();x.next();CHECK(x.offer());}
    {Fixture x;CHECK(x.offer());x.next();x.backend.ready=false;CHECK(!x.offer()&&x.owner.stopping()&&x.backend.stops==1);}
    {Fixture x;CHECK(x.offer());x.next();++x.source.frame.controllers.input_epoch;CHECK(!x.offer()&&x.owner.stopping()&&x.backend.starts==1);}
    {Fixture x;CHECK(x.offer());x.next();++x.source.entry_token;CHECK(!x.offer()&&x.owner.stopping()&&x.backend.starts==1);}
    {Fixture x;CHECK(x.offer());x.next();x.decision.fire_edge_sequence=x.source.frame.controllers.sequence+1;
        CHECK(!x.owner.offer(x.source,x.decision,x.now,x.backend)&&x.owner.stopping()&&x.backend.stops==1);}
    {Fixture x;CHECK(x.offer());x.backend.stop_result=f::Dispatch::not_dispatched;x.owner.request_stop();x.owner.service(x.backend);CHECK(x.owner.owed());
        x.backend.stop_result=f::Dispatch::returned;x.owner.service(x.backend);CHECK(x.backend.stops==2&&x.owner.owed());}
    {Fixture x;CHECK(x.offer());x.backend.quiet=true;x.owner.request_stop();x.owner.service(x.backend);CHECK(!x.owner.owed()&&!x.backend.stops);}
    {Fixture x;CHECK(x.offer());x.backend.dead=true;x.owner.service(x.backend);CHECK(!x.owner.owed()&&!x.backend.stops);}
    // Model a requester losing its return/assignment path through real SEH.
    // Heap state remains authoritative; no C++ objects are unwound.
    {auto* x=new Fixture;x->backend.escape_start=true;
        escaped_offer(x);
        CHECK(x->owner.owed()&&x->owner.stopping()&&x->owner.start_result()==f::Dispatch::uncertain&&x->backend.starts==1);
        x->owner.service(x->backend);CHECK(x->backend.stops==1&&x->owner.owed());delete x;}
    {auto* x=new Fixture;CHECK(x->offer());x->owner.request_stop();x->backend.escape_stop=true;
        escaped_stop(x);
        CHECK(x->owner.owed()&&x->owner.stop_result()==f::Dispatch::uncertain&&x->backend.stops==1);
        x->owner.service(x->backend);CHECK(x->backend.stops==1);x->backend.quiet=true;x->owner.service(x->backend);CHECK(!x->owner.owed());delete x;}
    std::printf("Troika original R2 responsibility: %u checks passed; synthetic backend only, no game permission.\n",checks);
    return 0;
}
