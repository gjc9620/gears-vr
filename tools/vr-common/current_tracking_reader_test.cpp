#include "current_tracking_reader.h"
#include "controller_cohort_publication.h"
#include <cstdio>
#include <thread>

namespace {
unsigned checks{},failures{};
#define C(value) do {++checks;if(!(value)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#value);}}while(0)
using Failure=gears_current_tracking::Failure;
struct Fixture {
    gears_vr::StereoBridge stereo;
    gears_vr::ControllerInputBridge controllers;
    gears_hand_continuity::InputBridge continuity;
    gears_current_tracking::Reader reader;
    gears_vr::StereoSample s{},got_s{};
    gears_vr::ControllerInputFrame c{},got_c{};
    gears_hand_continuity::TrackedInput t{},got_t{};
    gears_hand_continuity::Publisher got_p{};
    DWORD pid{GetCurrentProcessId()};std::int64_t now{1000000100};
    Fixture(){
        C(stereo.open(pid,true));C(controllers.open(pid,true));C(continuity.open(pid,true));
        s.session=41;s.sequence=1;s.space_epoch=2;s.predicted_time=1020;s.received_ns=now;
        s.focused=s.visible=s.should_render=1;s.head_flags=s.view_flags=15;
        s.head.orientation.w=1;
        s.eyes[0].pose.position.x=-.032;s.eyes[1].pose.position.x=.032;
        for(auto& eye:s.eyes){eye.fov={-.7,.7,.7,-.7};eye.pose.orientation.w=1;}
        c.source=s.source;c.session=s.session;c.sequence=s.sequence;c.space_epoch=s.space_epoch;
        c.predicted_time=s.predicted_time;c.received_ns=now;c.sync_ns=now-100;c.focused=1;
        for(auto& h:c.hands){h.grip_active=h.aim_active=h.squeeze_active=1;h.grip_flags=h.aim_flags=15;}
        t.input=c;t.stream_epoch=t.tracking_epoch[0]=t.tracking_epoch[1]=1;
    }
    void publish(){C(controllers.publish(c));C(continuity.publish(t));C(stereo.publish(s));}
    Failure read(std::int64_t at=1000000100){return reader.read(pid,[at](){return at;},false,got_s,got_c,got_t,got_p);}
    void empty(){C(!got_s.sequence&&!got_c.sequence&&!got_t.input.sequence&&!got_p.pid&&!got_p.start);}
};
template<class Action>
void while_publisher_busy(Fixture const& f,unsigned bridge,Action action){
    wchar_t name[128]{};
    if(bridge==2)swprintf_s(name,L"Local\\GearsVR.StereoLock.%lu",f.pid);
    else swprintf_s(name,bridge?L"Local\\GearsVR.ControllerContinuityLock.v4.%lu.%llu":
        L"Local\\GearsVR.ControllerInputLock.v4.%lu.%llu",f.pid,
        static_cast<unsigned long long>(gears_vr::controller_process_start(GetCurrentProcess())));
    HANDLE mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,name);
    HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    C(mutex&&ready&&release);
    DWORD acquired=WAIT_FAILED,waited=WAIT_FAILED;
    std::thread holder([&]{acquired=WaitForSingleObject(mutex,1000);SetEvent(ready);
        waited=WaitForSingleObject(release,1000);if(acquired==WAIT_OBJECT_0)ReleaseMutex(mutex);});
    C(WaitForSingleObject(ready,1000)==WAIT_OBJECT_0);
    action();SetEvent(release);holder.join();
    C(acquired==WAIT_OBJECT_0&&waited==WAIT_OBJECT_0);
    CloseHandle(release);CloseHandle(ready);CloseHandle(mutex);
}
}
int main(){
    {
        Fixture f;
        for(unsigned sequence=1;sequence<=8;++sequence){f.s.sequence=f.c.sequence=sequence;
            f.s.predicted_time=f.c.predicted_time=10000000000ll+sequence*10000000ll;
            f.c.sync_ns=1000000000ll+sequence*10000000ll;f.c.received_ns=f.s.received_ns=f.c.sync_ns+100;
            f.t.input=f.c;f.publish();}
        f.now=f.c.received_ns+1000;gears_hand_continuity::MotionHistory history{};
        C(f.reader.read(f.pid,[&]{return f.now;},false,f.got_s,f.got_c,f.got_t,f.got_p,&history)==Failure::none);
        C(history.count==8&&history.frames[0].input.sequence==1&&history.frames[7].input.sequence==8);
        C(gears_hand_continuity::same_input(history.frames[7].input,f.got_c));
        auto wrong=f.c;++wrong.input_epoch;
        C(f.reader.read_matching(f.pid,[&]{return f.now;},false,f.s,wrong,f.got_t,f.got_p,nullptr,&history)==Failure::cohort);
        C(!history.count&&!f.got_t.input.sequence&&!f.got_p.pid);
    }
    {
        unsigned calls{};
        C(gears_current_tracking::bounded_transport_read([&](bool& busy){busy=++calls<3;return busy?Failure::controller_read:Failure::none;})==Failure::none);
        C(calls==3);
        calls=0;C(gears_current_tracking::bounded_transport_read([&](bool& busy){++calls;busy=true;return Failure::continuity_read;})==Failure::continuity_busy);
        C(calls==3);
        calls=0;C(gears_current_tracking::bounded_transport_read([&](bool&){++calls;return Failure::cohort;})==Failure::cohort);
        C(calls==1); // Invalid identity/cohort is not a retry condition.
        for(unsigned bridge=0;bridge<3;++bridge){Fixture f;f.publish();C(f.read()==Failure::none);
            while_publisher_busy(f,bridge,[&]{const auto failure=f.read();
                C(failure==(bridge==0?Failure::controller_busy:bridge==1?Failure::continuity_busy:Failure::stereo_busy));f.empty();});
            C(f.read()==Failure::none);
            C(f.got_s.received_ns==f.s.received_ns&&f.got_c.sync_ns==f.c.sync_ns);
            C(gears_hand_continuity::same_input(f.got_t.input,f.c));
        }
    }
    {
        Fixture f;f.publish();C(f.read()==Failure::none);
        C(gears_vr::stereo_same(f.s,f.got_s));C(gears_hand_continuity::same_input(f.c,f.got_c));
        C(gears_hand_continuity::same_input(f.c,f.got_t.input));C(f.got_p.pid==f.pid&&f.got_p.start);
        // Unrelated readers may be closed/reopened without retiring this GT's handles.
        gears_vr::ControllerInputBridge other;C(other.open(f.pid,false));other.close();C(f.read()==Failure::none);
        Failure wrong{};std::thread foreign([&]{gears_vr::StereoSample s{};gears_vr::ControllerInputFrame c{};
            gears_hand_continuity::TrackedInput t{};gears_hand_continuity::Publisher p{};
            wrong=f.reader.read(f.pid,[&](){return f.now;},false,s,c,t,p);});foreign.join();C(wrong==Failure::wrong_owner);
        C(f.read()==Failure::none);
        C(f.reader.read_matching(f.pid,[&](){return f.now;},false,f.s,f.c,f.got_t,f.got_p)==Failure::none);
        C(gears_hand_continuity::same_input(f.c,f.got_t.input));
        auto wrong_input=f.c;wrong_input.hands[1].squeeze=.9f;
        C(f.reader.read_matching(f.pid,[&](){return f.now;},false,f.s,wrong_input,f.got_t,f.got_p)==Failure::cohort);
        C(!f.got_t.input.sequence&&!f.got_p.pid&&!f.got_p.start);
        C(f.reader.read_matching(f.pid,[&](){return f.now+100000001;},false,f.s,f.c,f.got_t,f.got_p)==Failure::controller_read);
        C(!f.got_t.input.sequence&&!f.got_p.pid&&!f.got_p.start);
        C(f.read(f.now+100000001)==Failure::controller_read);f.empty();
        C(f.read()==Failure::none);
        f.reader.close_after_drain();C(f.read()==Failure::none); // Explicit idle reopen.
    }
    {
        Fixture f;f.t.input.hands[0].grip.position.x=.5;f.publish();C(f.read()==Failure::cohort);f.empty();
    }
    {
        Fixture f;++f.s.sequence;f.publish();C(f.read()==Failure::controller_read);f.empty();
    }
    {
        Fixture f;f.s.focused=0;f.publish();C(f.read()==Failure::stereo_active||f.read()==Failure::stereo_focus);f.empty();
    }
    {
        Fixture f;f.s.head_flags=0;f.publish();C(f.read()==Failure::stereo_tracking);f.empty();
    }
    {
        Fixture f;f.publish();C(f.read()==Failure::none);
        // Remove only the test publisher after its readers close, then prove a
        // fresh Reader cannot silently create/accept a missing publication.
        f.reader.close_after_drain();f.continuity.close();C(f.read()==Failure::open);f.empty();
        C(f.continuity.open(f.pid,true));C(f.continuity.publish(f.t));C(f.read()==Failure::none);
    }
    {
        Fixture f;f.publish();unsigned calls{};
        C(f.reader.read(f.pid,[&](){return calls++?f.now:f.now-1000;},false,f.got_s,f.got_c,f.got_t,f.got_p)==Failure::none);
        C(f.got_s.received_ns==f.now&&f.got_c.received_ns==f.now); // No publisher timestamp is renewed.
        calls=0;
        C(f.reader.read(f.pid,[&](){return calls++?f.now-1:f.now;},false,f.got_s,f.got_c,f.got_t,f.got_p)==Failure::clock);f.empty();
        C(f.read(f.now-1)==Failure::controller_read);f.empty(); // Truly future samples still fail.
    }
    for(bool block_continuity:{false,true}){
        Fixture f;f.publish();C(f.read()==Failure::none);
        const auto old_stereo=f.s;const auto old_controllers=f.c;
        ++f.s.sequence;f.s.received_ns+=11000000;++f.s.predicted_time;
        f.c.sequence=f.s.sequence;f.c.received_ns=f.s.received_ns;f.c.sync_ns=f.s.received_ns-100;
        f.c.predicted_time=f.s.predicted_time;f.t.input=f.c;
        gears_controller_cohort::Result result{};unsigned controller_calls{},continuity_calls{},eye_calls{};
        const auto publish=[&]{return gears_controller_cohort::publish(true,true,true,
            [&]{++controller_calls;return f.controllers.publish(f.c);},
            [&]{++continuity_calls;return f.continuity.publish(f.t);},
            [&]{++eye_calls;return f.stereo.publish(f.s);});};
        while_publisher_busy(f,block_continuity,[&]{result=publish();});
        C(controller_calls==1&&continuity_calls==1&&eye_calls==0);
        C(!result.eyes_attempted&&!result.eyes);
        C(block_continuity?(result.controllers&&!result.continuity):(!result.controllers&&result.continuity));
        C(f.read(f.s.received_ns)==Failure::none);
        C(gears_vr::stereo_same(f.got_s,old_stereo)&&gears_hand_continuity::same_input(f.got_c,old_controllers));
        C(f.got_s.received_ns==old_stereo.received_ns&&f.got_c.sync_ns==old_controllers.sync_ns);
        // No fallback-age renewal: the old complete packet still expires.
        C(f.read(old_stereo.received_ns+100000001)==Failure::controller_read);f.empty();
        // Next actual source sample, not a retry of the skipped cohort.
        ++f.s.sequence;f.s.received_ns+=11000000;++f.s.predicted_time;
        f.c.sequence=f.s.sequence;f.c.received_ns=f.s.received_ns;f.c.sync_ns=f.s.received_ns-100;
        f.c.predicted_time=f.s.predicted_time;f.t.input=f.c;
        result=publish();C(result.eyes_attempted&&result.eyes&&result.controllers&&result.continuity);
        C(controller_calls==2&&continuity_calls==2&&eye_calls==1);
        C(f.read(f.s.received_ns)==Failure::none&&f.got_s.sequence==3&&f.got_c.sequence==3);
    }
    {
        // Head-only and controller-without-companion modes retain their
        // existing behavior; a reference transition still observes hands
        // exactly once without exposing a new eye cohort.
        unsigned controllers{},continuity{},eyes{};
        auto c=[&]{++controllers;return true;};auto t=[&]{++continuity;return true;};auto s=[&]{++eyes;return true;};
        auto r=gears_controller_cohort::publish(true,false,true,c,t,s);
        C(r.eyes&&controllers==0&&continuity==0&&eyes==1);
        r=gears_controller_cohort::publish(true,true,false,c,t,s);
        C(r.eyes&&controllers==1&&continuity==0&&eyes==2);
        r=gears_controller_cohort::publish(false,true,true,c,t,s);
        C(!r.eyes_attempted&&!r.eyes&&controllers==2&&continuity==1&&eyes==2);
        r=gears_controller_cohort::publish(true,true,true,c,t,[&]{++eyes;return false;});
        C(r.eyes_attempted&&!r.eyes&&controllers==3&&continuity==2&&eyes==3);
    }
    {
        // Real same-process publishers/ring/Reader. This touches no game/XR.
        Fixture f;f.publish();
        const auto advance=[&]{++f.s.sequence;f.now+=10000000;f.s.received_ns=f.now;++f.s.predicted_time;
            f.c.sequence=f.s.sequence;f.c.predicted_time=f.s.predicted_time;f.c.received_ns=f.now;f.c.sync_ns=f.now-100;};
        advance();auto& r=f.c.hands[1];r.squeeze=1;r.pressed[gears_vr::control_squeeze]=1;
        r.last_squeeze_press_sequence=f.c.sequence;r.last_squeeze_press_flags=r.grip_flags;
        r.grip.position={-.2,1.4,.02};r.last_squeeze_press_grip=r.grip;
        f.t.input=f.c;f.publish();const auto actual_edge=f.t;
        advance();r.squeeze=0;r.released[gears_vr::control_squeeze]=1;
        r.grip.position={.3,1.1,-.2};f.t.input=f.c;f.publish();
        gears_hand_continuity::TrackedInput edge{};
        const auto read=[&]{return f.reader.read_matching(f.pid,[&]{return f.now;},false,f.s,f.c,f.got_t,f.got_p,&edge);};
        C(read()==Failure::none);C(gears_hand_continuity::same_input(edge.input,actual_edge.input));
        C(edge.input.sync_ns==actual_edge.input.sync_ns&&edge.input.sequence==2);
        C(gears_hand_continuity::recorded_right_squeeze_press(f.got_t,edge));
        for(unsigned bridge=0;bridge<2;++bridge){
            while_publisher_busy(f,bridge,[&]{
                C(read()==(bridge?Failure::continuity_busy:Failure::controller_busy));
                C(!f.got_t.input.sequence&&!f.got_p.pid&&!f.got_p.start&&!edge.input.sequence);
            });
            // Releasing transport contention must retain the exact requested
            // packet and original edge. No new cohort or refreshed timestamp.
            C(read()==Failure::none);
            C(gears_hand_continuity::same_input(f.got_t.input,f.c));
            C(gears_hand_continuity::same_input(edge.input,actual_edge.input));
            C(edge.input.sync_ns==actual_edge.input.sync_ns);
        }
        // A different requested cohort clears the optional proof too.
        auto mismatch=f.c;++mismatch.input_epoch;
        C(f.reader.read_matching(f.pid,[&]{return f.now;},false,f.s,mismatch,f.got_t,f.got_p,&edge)==Failure::cohort);
        C(!edge.input.sequence);
        C(read()==Failure::none&&edge.input.sequence==2);
        // Once the real ring overwrites the edge, never reconstruct it from
        // counters, last-grip fields or this reader's previous successful read.
        for(unsigned i=0;i<gears_hand_continuity::ContinuityPacket::capacity;++i){advance();f.t.input=f.c;f.publish();}
        C(read()==Failure::none);C(!edge.input.sequence&&f.got_t.input.sequence==f.c.sequence);
    }
    std::printf("current_tracking_reader: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
