
#include <windows.h>
#include <thread>
bool inject_wait_failure{};unsigned injected_calls{};
DWORD observed_wait(HANDLE handle,DWORD wait){
 if(inject_wait_failure&&++injected_calls==2)return WAIT_FAILED;
 return ::WaitForSingleObject(handle,wait);
}
#define WaitForSingleObject observed_wait
#include "controller_input_bridge.h"
#undef WaitForSingleObject
#include <cstdio>
#include <cstdlib>

#define CHECK(value) do { if(!(value)) { std::fprintf(stderr,"check failed: %s:%d: %s\n",__FILE__,__LINE__,#value);std::abort(); } } while(0)

int main() {
    using namespace gears_vr;
    StereoSample stereo{};
    stereo.source=StereoSource::RuntimeTracked;stereo.session=41;stereo.sequence=1;
    stereo.space_epoch=2;stereo.predicted_time=1020;stereo.focused=1;
    ControllerInputFrame frame{};
    frame.source=stereo.source;frame.session=stereo.session;frame.sequence=stereo.sequence;
    frame.space_epoch=stereo.space_epoch;frame.predicted_time=stereo.predicted_time;
    frame.sync_ns=1000000000;frame.received_ns=1000000100;frame.focused=1;
    frame.hands[0].grip_active=1;frame.hands[0].grip_flags=15;
    frame.hands[0].grip.orientation.w=1;frame.hands[0].aim_active=1;
    frame.hands[0].aim_flags=15;frame.hands[0].aim.orientation.w=1;
    frame.hands[0].squeeze_active=1;frame.hands[0].squeeze=.7f;
    frame.hands[0].buttons_active=controller_x|controller_stick_click;
    frame.hands[0].buttons=controller_x;
    frame.hands[1].grip_active=1;frame.hands[1].grip_flags=15;
    frame.hands[1].grip.orientation.w=1;
    frame.hands[1].trigger_active=1;frame.hands[1].trigger=.9f;
    frame.hands[1].buttons_active=controller_a;frame.hands[1].buttons=controller_a;
    CHECK(controller_frame_matches(frame,stereo,1000000200));
    CHECK(controller_pose_usable(frame.hands[0].grip,frame.hands[0].grip_flags,frame.hands[0].grip_active));
    CHECK(!controller_pose_usable(frame.hands[1].aim,frame.hands[1].aim_flags,frame.hands[1].aim_active));

    ControllerEdgeCounter edges;
    auto edge=frame;edge.hands[1].buttons=0;edge.hands[1].buttons_active=controller_stick_click;
    edge.hands[1].squeeze_active=1;edge.hands[1].squeeze=0;
    edge.hands[1].trigger=0;edges.update(edge); // first focused frame is a baseline
    CHECK(edge.hands[1].pressed[control_stick]==0&&edge.hands[1].pressed[control_trigger]==0);
    edge.hands[1].buttons=controller_stick_click;edge.hands[1].trigger=.9f;
    edge.hands[1].squeeze=.9f;edge.hands[1].grip.position.x=.25;
    edges.update(edge);
    CHECK(edge.hands[1].pressed[control_stick]==1&&edge.hands[1].pressed[control_trigger]==1);
    CHECK(edge.hands[1].pressed[control_squeeze]==1&&edge.hands[1].last_squeeze_press_grip.position.x==.25);
    edge.hands[1].buttons=0;edge.hands[1].trigger=0;edge.hands[1].squeeze=0;
    edge.hands[1].grip.position.x=1.25;edges.update(edge);
    CHECK(edge.hands[1].released[control_stick]==1&&edge.hands[1].released[control_trigger]==1);
    CHECK(edge.hands[1].last_squeeze_press_grip.position.x==.25);
    // A game tick that only saw baseline and this third XR frame still sees
    // both transitions through cumulative counts.
    edge.focused=0;edges.update(edge);
    CHECK(edge.input_epoch==2&&edge.hands[1].pressed[control_stick]==0);
    edge.focused=1;edge.hands[1].buttons=controller_stick_click;edges.update(edge);
    CHECK(edge.hands[1].pressed[control_stick]==0); // focus regain must not click
    edges.profile_changed();edges.update(edge);
    CHECK(edge.input_epoch==3&&edge.hands[1].pressed[control_stick]==0);
    edge.space_epoch=3;edges.update(edge);
    CHECK(edge.input_epoch==4&&edge.hands[1].pressed[control_stick]==0);

    ControllerInputBridge publisher,duplicate,reader;
    const auto pid=GetCurrentProcessId();
    CHECK(publisher.open(pid,true));CHECK(!duplicate.open(pid,true));
    CHECK(reader.open(pid,false));CHECK(publisher.publish(frame));
    ControllerInputFrame copy{};
    ControllerMatchingRead diagnostic{};
    CHECK(reader.read_matching(stereo,1000000200,copy,false,nullptr,&diagnostic));
    CHECK(diagnostic.code==ControllerMatchingRead::Code::accepted&&diagnostic.latest_sequence==1&&diagnostic.selected_sequence==1);
    CHECK(copy.sequence==1&&copy.hands[0].buttons==controller_x&&copy.hands[1].trigger==.9f);
    CHECK(!reader.read_matching(stereo,1100000101,copy,false,nullptr,&diagnostic)); // older than 100 ms
    CHECK(diagnostic.code==ControllerMatchingRead::Code::frame_mismatch);
    auto wrong=stereo;wrong.space_epoch=3;CHECK(!reader.read_matching(wrong,1000000200,copy));
    wrong=stereo;wrong.predicted_time=1021;CHECK(!reader.read_matching(wrong,1000000200,copy));
    wrong=stereo;wrong.source=StereoSource::MetaOperatorSimulated;
    CHECK(!reader.read_matching(wrong,1000000200,copy,true));
    wrong=stereo;wrong.focused=0;CHECK(!reader.read_matching(wrong,1000000200,copy));

    auto invalid=frame;invalid.sequence=2;invalid.hands[1].trigger=1.1f;
    CHECK(!publisher.publish(invalid));
    invalid=frame;invalid.sequence=2;invalid.hands[0].buttons=controller_a;
    CHECK(!publisher.publish(invalid));
    invalid=frame;invalid.sequence=2;invalid.hands[1].pressed[control_trigger]=3;
    CHECK(!publisher.publish(invalid));
    CHECK(!publisher.publish(frame)); // a frame cannot be overwritten with changed input

    for(std::uint64_t sequence=2;sequence<=10;++sequence) {
        frame.sequence=sequence;frame.received_ns+=1000;frame.sync_ns+=1000;
        stereo.sequence=sequence;
        CHECK(publisher.publish(frame));
        CHECK(reader.read_matching(stereo,frame.received_ns+100,copy));
    }
    wrong=stereo;wrong.sequence=1;CHECK(!reader.read_matching(wrong,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    CHECK(diagnostic.code==ControllerMatchingRead::Code::evicted&&diagnostic.latest_sequence==10);
    wrong=stereo;wrong.sequence=12;CHECK(!reader.read_matching(wrong,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    CHECK(diagnostic.code==ControllerMatchingRead::Code::not_published_yet);

    CHECK(reader.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    CHECK(diagnostic.mutex_attempted&&diagnostic.mutex_result==WAIT_OBJECT_0&&!diagnostic.mutex_contended);
    wchar_t lock_name[128]{};
    swprintf_s(lock_name,LR"(Local\GearsVR.ControllerInputLock.v4.%lu.%llu)",pid,
        static_cast<unsigned long long>(controller_process_start(GetCurrentProcess())));
    HANDLE test_lock=OpenMutexW(MUTEX_ALL_ACCESS,FALSE,lock_name);CHECK(test_lock);
    for(bool abandon:{false,true}){
        HANDLE entered=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        CHECK(entered&&release);
        std::thread holder([&](){CHECK(::WaitForSingleObject(test_lock,3000)==WAIT_OBJECT_0);
            SetEvent(entered);if(!abandon){CHECK(::WaitForSingleObject(release,3000)==WAIT_OBJECT_0);CHECK(ReleaseMutex(test_lock));}});
        CHECK(::WaitForSingleObject(entered,3000)==WAIT_OBJECT_0);
        if(abandon)holder.join();
        bool contended{};
        const auto before=copy;
        CHECK(!reader.read_matching(stereo,frame.received_ns+100,copy,false,&contended,&diagnostic));
        CHECK(diagnostic.code==ControllerMatchingRead::Code::mutex_unavailable&&diagnostic.mutex_attempted);
        CHECK(diagnostic.mutex_result==(abandon?WAIT_ABANDONED:WAIT_TIMEOUT));
        CHECK(contended==!abandon&&diagnostic.mutex_contended==!abandon);
        CHECK(!diagnostic.latest_sequence&&!diagnostic.selected_sequence&&std::memcmp(&before,&copy,sizeof copy)==0);
        if(!abandon){SetEvent(release);holder.join();}
        CHECK(reader.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
        CHECK(diagnostic.mutex_result==WAIT_OBJECT_0&&!diagnostic.mutex_contended);
        CloseHandle(entered);CloseHandle(release);
    }
    inject_wait_failure=true;injected_calls=0;
    CHECK(!reader.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    inject_wait_failure=false;
    CHECK(injected_calls==2&&diagnostic.mutex_attempted&&diagnostic.mutex_result==WAIT_FAILED&&!diagnostic.mutex_contended);
    CHECK(diagnostic.code==ControllerMatchingRead::Code::mutex_unavailable);
    CHECK(reader.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    ControllerInputBridge unopened;
    CHECK(!unopened.read_matching(stereo,frame.received_ns+100,copy,false,nullptr,&diagnostic));
    CHECK(diagnostic.code==ControllerMatchingRead::Code::not_open&&!diagnostic.mutex_attempted);
    CloseHandle(test_lock);
    std::puts("Original bridge: accepted, real competing-thread TIMEOUT, real ABANDONED, injected WAIT_FAILED and unopened distinguished; no retry, packet/output mutation or changed decision");
    auto unfocused=frame;unfocused.sequence=11;unfocused.focused=0;
    CHECK(publisher.publish(unfocused));
    wrong=stereo;wrong.sequence=11;CHECK(!reader.read_matching(wrong,frame.received_ns+100,copy));
    std::puts("controller frame identity, tracking, focus, expiry, payload and bridge ring checks passed");
}
