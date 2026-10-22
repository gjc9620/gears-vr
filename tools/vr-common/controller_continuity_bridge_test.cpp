#include "controller_input_bridge.h" // Exercise coexistence and Windows-first includes.
#include "controller_continuity_bridge.h"
#include <cstdio>
#include <cstdlib>
#include <thread>

#define CHECK(value) do { if(!(value)) { std::fprintf(stderr,"check failed: %s:%d: %s\n",__FILE__,__LINE__,#value);std::abort(); } } while(0)

namespace gears_hand_continuity {
struct SourceTrackerTestAccess {
    static void epochs(SourceTracker& tracker,std::uint64_t stream,std::uint64_t left,std::uint64_t right) {
        tracker.stream_epoch_=stream;tracker.tracking_epoch_[0]=left;tracker.tracking_epoch_[1]=right;
    }
};
}

namespace {
using namespace gears_hand_continuity;
using gears_vr::ControllerInputFrame;
constexpr auto maximum=(std::numeric_limits<std::uint64_t>::max)();

ControllerInputFrame frame(std::uint64_t sequence=1,std::uint64_t session=41) {
    ControllerInputFrame result{};
    result.session=session;result.sequence=sequence;result.space_epoch=2;
    result.predicted_time=1020;result.sync_ns=1000000000;result.received_ns=1000000100;result.focused=1;
    for(unsigned hand=0;hand<2;++hand){
        auto& item=result.hands[hand];
        item.grip_active=item.aim_active=1;item.grip_flags=item.aim_flags=15;
        item.grip.position={.1+hand,.2,.3};item.aim.position={.4,.5,.6};
        item.trigger_active=item.squeeze_active=item.stick_active=1;
        item.trigger=.2f;item.squeeze=.4f;item.stick_x=.1f;item.stick_y=-.2f;
        item.buttons_active=hand?gears_vr::controller_a:gears_vr::controller_x;
        item.buttons=item.buttons_active;
    }
    CHECK(gears_vr::controller_frame_shape_valid(result));return result;
}
gears_vr::StereoSample stereo_for(ControllerInputFrame const& input) {
    gears_vr::StereoSample stereo{};
    stereo.source=input.source;stereo.session=input.session;stereo.sequence=input.sequence;
    stereo.space_epoch=input.space_epoch;stereo.predicted_time=input.predicted_time;stereo.focused=input.focused;
    return stereo;
}
TrackedInput tracked(ControllerInputFrame const& input,std::uint64_t stream=1,
                     std::uint64_t left=1,std::uint64_t right=1) {
    TrackedInput result{};result.input=input;result.stream_epoch=stream;
    result.tracking_epoch[0]=left;result.tracking_epoch[1]=right;return result;
}
bool same_tracked(TrackedInput const& a,TrackedInput const& b) {
    return same_input(a.input,b.input)&&a.stream_epoch==b.stream_epoch&&
        a.tracking_epoch[0]==b.tracking_epoch[0]&&a.tracking_epoch[1]==b.tracking_epoch[1];
}
void empty(TrackedInput const& value) {CHECK(same_tracked(value,TrackedInput{}));}
void empty(Publisher const& value) {CHECK(!value.pid&&!value.start);}
void rejected_read(InputBridge& reader,gears_vr::StereoSample const& stereo,std::int64_t now,bool simulated=false) {
    auto out=tracked(frame());Publisher publisher{123,456};
    CHECK(!reader.read_matching(stereo,now,out,publisher,simulated));empty(out);empty(publisher);
}

void exact_value_comparison() {
    const auto original=frame(10);
    auto changed=[&](auto change){auto other=original;change(other);CHECK(!same_input(original,other));};
    CHECK(same_input(original,original));
    changed([](auto& f){f.source=gears_vr::StereoSource::MetaOperatorSimulated;});
    for(auto member:{&ControllerInputFrame::session,&ControllerInputFrame::sequence,&ControllerInputFrame::space_epoch,
                     &ControllerInputFrame::profile_generation,&ControllerInputFrame::input_epoch})
        changed([&](auto& f){++(f.*member);});
    for(auto member:{&ControllerInputFrame::predicted_time,&ControllerInputFrame::received_ns,&ControllerInputFrame::sync_ns})
        changed([&](auto& f){++(f.*member);});
    changed([](auto& f){f.focused=0;});
    using Hand=gears_vr::ControllerHand;
    for(unsigned hand=0;hand<2;++hand){
        for(auto pose:{&Hand::grip,&Hand::aim,&Hand::last_squeeze_press_grip}){
            for(auto component:{&gears_vr::Vector::x,&gears_vr::Vector::y,&gears_vr::Vector::z})
                changed([&](auto& f){(f.hands[hand].*pose).position.*component+=.01;});
            for(auto component:{&gears_vr::Quaternion::x,&gears_vr::Quaternion::y,&gears_vr::Quaternion::z,&gears_vr::Quaternion::w})
                changed([&](auto& f){(f.hands[hand].*pose).orientation.*component+=.01;});
        }
        for(auto member:{&Hand::grip_flags,&Hand::aim_flags,&Hand::last_squeeze_press_flags,&Hand::last_squeeze_press_sequence})
            changed([&](auto& f){++(f.hands[hand].*member);});
        for(auto member:{&Hand::grip_active,&Hand::aim_active,&Hand::buttons,&Hand::buttons_active,
                         &Hand::trigger_active,&Hand::squeeze_active,&Hand::stick_active})
            changed([&](auto& f){++(f.hands[hand].*member);});
        for(auto member:{&Hand::trigger,&Hand::squeeze,&Hand::stick_x,&Hand::stick_y})
            changed([&](auto& f){(f.hands[hand].*member)+=.01f;});
        for(unsigned control=0;control<gears_vr::control_count;++control){
            changed([&](auto& f){++f.hands[hand].pressed[control];});
            changed([&](auto& f){++f.hands[hand].released[control];});
        }
    }
    // Bytes between source and session are padding on the supported ABI.
    auto padded=original;
    static_assert(offsetof(ControllerInputFrame,session)>sizeof(padded.source));
    for(std::size_t i=sizeof(padded.source);i<offsetof(ControllerInputFrame,session);++i)
        reinterpret_cast<unsigned char*>(&padded)[i]^=0xff;
    CHECK(same_input(original,padded));
}

void source_loss_and_boundaries() {
    SourceTracker source;TrackedInput out{};
    auto input=frame();CHECK(source.update(input,out));
    CHECK(out.stream_epoch==1&&out.tracking_epoch[0]==1&&out.tracking_epoch[1]==1);
    const auto first=out;CHECK(source.update(input,out));CHECK(same_tracked(first,out));
    ++input.sequence;input.hands[0].grip_flags=3;CHECK(source.update(input,out));
    CHECK(out.stream_epoch==1&&out.tracking_epoch[0]==2&&out.tracking_epoch[1]==1);
    const auto lost=out;CHECK(source.update(input,out));CHECK(same_tracked(lost,out));
    ++input.sequence;input.hands[0].grip_flags=15;CHECK(source.update(input,out));
    CHECK(out.tracking_epoch[0]==2&&out.tracking_epoch[1]==1);
    ++input.sequence;input.hands[1].grip_active=0;CHECK(source.update(input,out));
    CHECK(out.tracking_epoch[0]==2&&out.tracking_epoch[1]==2);
    ++input.sequence;++input.space_epoch;input.hands[0].grip_active=0;
    CHECK(source.update(input,out));CHECK(out.stream_epoch==2&&out.tracking_epoch[0]==2);
    input.sequence+=3;input.hands[0].grip_active=1;input.hands[1].grip_active=1;
    CHECK(source.update(input,out));CHECK(out.stream_epoch==3&&out.tracking_epoch[0]==2&&out.tracking_epoch[1]==2);
    ++input.sequence;input.hands[0].grip_active=0;CHECK(source.update(input,out));CHECK(out.tracking_epoch[0]==3);
    input=frame(1,42);CHECK(source.update(input,out));
    CHECK(out.stream_epoch==4&&out.tracking_epoch[0]==3&&out.tracking_epoch[1]==2);
    // A publisher restart/reopen must retain this tracker; no tracker reset API exists.
    CHECK(source.update(out.input,out));CHECK(out.stream_epoch==4&&out.input.session==42);
}

void source_rejections_break_continuity() {
    {
        SourceTracker source;auto out=tracked(frame());
        CHECK(!source.update(ControllerInputFrame{},out));empty(out);
        CHECK(source.update(frame(),out));CHECK(out.stream_epoch==1);
    }
    auto exercise=[](auto corrupt){
        SourceTracker source;TrackedInput out{};auto initial=frame(10);
        initial.space_epoch=initial.input_epoch=initial.profile_generation=3;
        CHECK(source.update(initial,out));
        auto bad=initial;bad.sequence=11;corrupt(bad);
        CHECK(!source.update(bad,out));empty(out);
        CHECK(!source.update(initial,out));empty(out); // An old idempotent sample cannot clear pending.
        auto good=initial;good.sequence=12;CHECK(source.update(good,out));CHECK(out.stream_epoch==2);
        good.sequence=13;CHECK(source.update(good,out));CHECK(out.stream_epoch==2);
    };
    exercise([](auto& f){f.hands[0].grip_active=2;});
    exercise([](auto& f){f.sequence=9;});
    exercise([](auto& f){f.sequence=1;++f.space_epoch;});
    exercise([](auto& f){f.space_epoch=2;});
    exercise([](auto& f){f.input_epoch=2;});
    exercise([](auto& f){f.profile_generation=2;});
    exercise([](auto& f){f.sequence=10;f.hands[0].grip.position.x+=.01;});
    exercise([](auto& f){f.sequence=10;++f.received_ns;});
    SourceTracker source;TrackedInput out{};auto input=frame(20);
    CHECK(source.update(input,out));input=frame(1,42);input.space_epoch=1;
    CHECK(source.update(input,out));CHECK(out.stream_epoch==2);
    input.source=gears_vr::StereoSource::MetaOperatorSimulated;CHECK(source.update(input,out));CHECK(out.stream_epoch==3);
}

void source_overflow_is_permanent() {
    for(unsigned overflowing=0;overflowing<3;++overflowing){
        SourceTracker source;TrackedInput out{};auto input=frame();CHECK(source.update(input,out));
        SourceTrackerTestAccess::epochs(source,overflowing==0?maximum:1,overflowing==1?maximum:1,overflowing==2?maximum:1);
        if(overflowing==0)input.sequence=3;
        else {++input.sequence;input.hands[overflowing-1].grip_active=0;}
        CHECK(!source.update(input,out));empty(out);
        input=frame(1,99);CHECK(!source.update(input,out));empty(out);
    }
    SourceTracker source;TrackedInput out{};auto input=frame(maximum);
    CHECK(source.update(input,out));CHECK(out.input.sequence==maximum);
    input=frame(1,42);CHECK(source.update(input,out));CHECK(out.stream_epoch==2);
}

struct OwnedMapping {
    HANDLE mapping{},mutex{};ContinuityPacket* packet{};
    OwnedMapping() {
        const auto pid=GetCurrentProcessId();const auto start=gears_vr::controller_process_start(GetCurrentProcess());
        wchar_t map_name[128]{},lock_name[128]{};
        swprintf_s(map_name,L"Local\\GearsVR.ControllerContinuity.v1.%lu.%llu",pid,static_cast<unsigned long long>(start));
        swprintf_s(lock_name,L"Local\\GearsVR.ControllerContinuityLock.v1.%lu.%llu",pid,static_cast<unsigned long long>(start));
        mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,lock_name);
        mapping=OpenFileMappingW(FILE_MAP_READ|FILE_MAP_WRITE,FALSE,map_name);
        CHECK(mutex&&mapping);packet=static_cast<ContinuityPacket*>(MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,sizeof(ContinuityPacket)));
        CHECK(packet);
    }
    template<class Change> void change(Change change) {
        CHECK(WaitForSingleObject(mutex,0)==WAIT_OBJECT_0);change(*packet);CHECK(ReleaseMutex(mutex));
    }
    ~OwnedMapping(){if(packet)UnmapViewOfFile(packet);if(mapping)CloseHandle(mapping);if(mutex)CloseHandle(mutex);}
};

void coexistence_and_skipped_loss() {
    const auto pid=GetCurrentProcessId();
    gears_vr::ControllerInputBridge old_producer,old_reader;
    InputBridge producer,reader,duplicate;
    CHECK(old_producer.open(pid,true));CHECK(old_reader.open(pid,false));
    CHECK(producer.open(pid,true));CHECK(!duplicate.open(pid,true));CHECK(reader.open(pid,false));
    SourceTracker source;TrackedInput out{},received{};Publisher publisher{};ControllerInputFrame old_received{};
    auto input=frame();CHECK(source.update(input,out));CHECK(old_producer.publish(input));CHECK(producer.publish(out));
    CHECK(reader.read_matching(stereo_for(input),input.received_ns,received,publisher));
    CHECK(publisher.pid==pid&&publisher.start==gears_vr::controller_process_start(GetCurrentProcess()));
    CHECK(old_reader.read_matching(stereo_for(input),input.received_ns,old_received));CHECK(same_input(old_received,received.input));
    ++input.sequence;input.hands[0].grip_active=0;CHECK(source.update(input,out)); // Not published or read.
    ++input.sequence;input.hands[0].grip_active=1;CHECK(source.update(input,out));
    CHECK(producer.publish(out));CHECK(old_producer.publish(input));
    CHECK(reader.read_matching(stereo_for(input),input.received_ns,received,publisher));
    CHECK(received.stream_epoch==1&&received.tracking_epoch[0]==2&&received.tracking_epoch[1]==1);
    CHECK(old_reader.read_matching(stereo_for(input),input.received_ns,old_received));CHECK(same_input(old_received,received.input));
    CHECK(!producer.publish(out)); // Only the tracker supports idempotent rereads.
}

void matching_age_and_ring() {
    InputBridge producer,reader;const auto pid=GetCurrentProcessId();
    CHECK(producer.open(pid,true));CHECK(reader.open(pid,false));
    rejected_read(reader,stereo_for(frame()),1000000100);
    SourceTracker source;TrackedInput out{},received{};Publisher publisher{};
    for(std::uint64_t sequence=1;sequence<=10;++sequence){CHECK(source.update(frame(sequence),out));CHECK(producer.publish(out));}
    auto input=frame(3);CHECK(reader.read_matching(stereo_for(input),input.received_ns+100000000,received,publisher));
    CHECK(received.input.sequence==3);
    rejected_read(reader,stereo_for(frame(2)),input.received_ns);
    rejected_read(reader,stereo_for(frame(11)),input.received_ns);
    input=frame(10);auto stereo=stereo_for(input);
    rejected_read(reader,stereo,input.received_ns+100000001);
    rejected_read(reader,stereo,input.received_ns-1);
    auto wrong=stereo;++wrong.session;rejected_read(reader,wrong,input.received_ns);
    wrong=stereo;++wrong.space_epoch;rejected_read(reader,wrong,input.received_ns);
    wrong=stereo;++wrong.predicted_time;rejected_read(reader,wrong,input.received_ns);
    wrong=stereo;wrong.focused=0;rejected_read(reader,wrong,input.received_ns);
    wrong=stereo;wrong.source=gears_vr::StereoSource::MetaOperatorSimulated;rejected_read(reader,wrong,input.received_ns);
    rejected_read(reader,stereo,input.received_ns,true);
    rejected_read(producer,stereo,input.received_ns);
    input=frame(1,42);CHECK(source.update(input,out));CHECK(producer.publish(out));
    rejected_read(reader,stereo,1000000100); // Previous-stream slots are discarded.
    CHECK(reader.read_matching(stereo_for(input),input.received_ns,received,publisher));CHECK(received.stream_epoch==2);
    input.source=gears_vr::StereoSource::MetaOperatorSimulated;CHECK(source.update(input,out));CHECK(producer.publish(out));
    rejected_read(reader,stereo_for(input),input.received_ns);
    CHECK(reader.read_matching(stereo_for(input),input.received_ns,received,publisher,true));CHECK(received.stream_epoch==3);
    ++input.sequence;input.focused=0;CHECK(source.update(input,out));CHECK(producer.publish(out));
    rejected_read(reader,stereo_for(input),input.received_ns,true);
    reader.close();rejected_read(reader,stereo_for(input),input.received_ns,true);
}

void publication_rejection_does_not_consume() {
    InputBridge producer,reader;const auto pid=GetCurrentProcessId();
    CHECK(producer.open(pid,true));CHECK(reader.open(pid,false));
    auto current=tracked(frame(10),2,3,4);CHECK(producer.publish(current));
    auto reject=[&](auto change){auto bad=current;bad.input.sequence=11;change(bad);CHECK(!producer.publish(bad));};
    reject([](auto& f){f.stream_epoch=1;});
    reject([](auto& f){f.stream_epoch=0;});
    reject([](auto& f){f.tracking_epoch[0]=0;});
    reject([](auto& f){f.tracking_epoch[0]=2;});
    reject([](auto& f){f.tracking_epoch[1]=3;});
    reject([](auto& f){++f.input.space_epoch;}); // A changed raw key requires a newer stream.
    reject([](auto& f){f.input.hands[0].squeeze=2;});
    reject([](auto& f){f.input.source=static_cast<gears_vr::StereoSource>(99);});
    reject([](auto& f){f.stream_epoch=3;f.input.sequence=9;});
    reject([](auto& f){f.stream_epoch=3;f.input.space_epoch=1;});
    reject([](auto& f){f.stream_epoch=3;f.input.profile_generation=0;});
    ++current.input.sequence;CHECK(producer.publish(current)); // Invalid attempts did not consume sequence 11.
    current=tracked(frame(1,42),3,3,4);CHECK(producer.publish(current));
    auto bad=current;bad.input.session=43;bad.input.sequence=1;CHECK(!producer.publish(bad));
    ++current.input.sequence;CHECK(producer.publish(current));
    bad=current;bad.stream_epoch=4;bad.input.sequence=1;CHECK(!producer.publish(bad));
    auto next=tracked(frame(1,43),4,3,4);CHECK(producer.publish(next));
    TrackedInput out{};Publisher publisher{};
    CHECK(reader.read_matching(stereo_for(next.input),next.input.received_ns,out,publisher));CHECK(same_tracked(out,next));
}

void fixed_header_and_nonblocking_mutex() {
    InputBridge producer,reader;const auto pid=GetCurrentProcessId();
    CHECK(!reader.open(0,false));CHECK(!producer.open(0,true));
    CHECK(!reader.open(pid==MAXDWORD?pid-1:pid+1,false));
    CHECK(producer.open(pid,true));CHECK(reader.open(pid,false));
    auto current=tracked(frame());CHECK(producer.publish(current));auto next=tracked(frame(2));
    OwnedMapping mapping;
    auto corrupt=[&](auto change){
        ContinuityPacket saved{};mapping.change([&](auto& packet){saved=packet;change(packet);});
        rejected_read(reader,stereo_for(current.input),current.input.received_ns);CHECK(!producer.publish(next));
        mapping.change([&](auto& packet){packet=saved;});
    };
    corrupt([](auto& p){p.publisher_pid=p.publisher_pid==MAXDWORD?p.publisher_pid-1:p.publisher_pid+1;});
    corrupt([](auto& p){++p.publisher_start;});
    corrupt([](auto& p){++p.game_start;});
    corrupt([](auto& p){p.game_pid=0;});
    corrupt([](auto& p){++p.qpc_frequency;});
    corrupt([](auto& p){++p.magic;});
    corrupt([](auto& p){++p.version;});
    corrupt([](auto& p){--p.size;});
    corrupt([](auto& p){++p.latest_sequence;});
    mapping.change([](auto& p){p.frames[1].tracking_epoch[0]=0;});
    rejected_read(reader,stereo_for(current.input),current.input.received_ns);
    mapping.change([](auto& p){p.frames[1].tracking_epoch[0]=1;});
    HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);CHECK(ready&&release);
    std::thread holder([&]{CHECK(WaitForSingleObject(mapping.mutex,3000)==WAIT_OBJECT_0);SetEvent(ready);
        WaitForSingleObject(release,3000);CHECK(ReleaseMutex(mapping.mutex));});
    CHECK(WaitForSingleObject(ready,3000)==WAIT_OBJECT_0);
    rejected_read(reader,stereo_for(current.input),current.input.received_ns);CHECK(!producer.publish(next));
    CHECK(SetEvent(release));holder.join();CloseHandle(ready);CloseHandle(release);
    CHECK(producer.publish(next));
    // An abandoned mutex is rejected and released, rather than retained recursively.
    std::thread abandon([&]{CHECK(WaitForSingleObject(mapping.mutex,3000)==WAIT_OBJECT_0);});abandon.join();
    rejected_read(reader,stereo_for(next.input),next.input.received_ns);
    TrackedInput out{};Publisher publisher{};
    CHECK(reader.read_matching(stereo_for(next.input),next.input.received_ns,out,publisher));
}

void maximum_counters_do_not_wrap() {
    InputBridge producer,reader;const auto pid=GetCurrentProcessId();
    CHECK(producer.open(pid,true));CHECK(reader.open(pid,false));
    auto high=tracked(frame(maximum),maximum,maximum,maximum);CHECK(producer.publish(high));
    TrackedInput out{};Publisher publisher{};
    CHECK(reader.read_matching(stereo_for(high.input),high.input.received_ns,out,publisher));CHECK(same_tracked(high,out));
    auto wrapped=tracked(frame(1,42),0,maximum,maximum);CHECK(!producer.publish(wrapped));
    wrapped.stream_epoch=1;CHECK(!producer.publish(wrapped));CHECK(!producer.publish(high));
}

int publisher_child(int argc,char** argv) {
    if(argc!=5)return 2;
    const auto pid=static_cast<DWORD>(std::strtoul(argv[2],nullptr,10));
    HANDLE ready=OpenEventA(EVENT_MODIFY_STATE,FALSE,argv[3]),finish=OpenEventA(SYNCHRONIZE,FALSE,argv[4]);
    if(!ready||!finish)return 3;
    InputBridge producer;
    if(!producer.open(pid,true)||!producer.publish(tracked(frame())))return 4;
    if(!SetEvent(ready))return 5;
    const auto result=WaitForSingleObject(finish,10000);
    producer.close();CloseHandle(ready);CloseHandle(finish);return result==WAIT_OBJECT_0?0:6;
}
void publisher_process_lifetime() {
    const auto pid=GetCurrentProcessId();char ready_name[128]{},finish_name[128]{};
    sprintf_s(ready_name,"Local\\GearsVR.ContinuityTestReady.%lu",pid);
    sprintf_s(finish_name,"Local\\GearsVR.ContinuityTestFinish.%lu",pid);
    HANDLE ready=CreateEventA(nullptr,TRUE,FALSE,ready_name),finish=CreateEventA(nullptr,TRUE,FALSE,finish_name);CHECK(ready&&finish);
    wchar_t executable[32768]{},command[32768]{};
    CHECK(GetModuleFileNameW(nullptr,executable,32768)>0);
    CHECK(swprintf_s(command,L"\"%ls\" --publisher-child %lu %hs %hs",executable,pid,ready_name,finish_name)>0);
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    CHECK(CreateProcessW(executable,command,nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child));
    CHECK(WaitForSingleObject(ready,5000)==WAIT_OBJECT_0);
    InputBridge reader;CHECK(reader.open(pid,false));TrackedInput out{};Publisher publisher{};
    CHECK(reader.read_matching(stereo_for(frame()),frame().received_ns,out,publisher));
    CHECK(publisher.pid==child.dwProcessId&&publisher.start==gears_vr::controller_process_start(child.hProcess));
    CHECK(SetEvent(finish));CHECK(WaitForSingleObject(child.hProcess,5000)==WAIT_OBJECT_0);
    DWORD exit_code{};CHECK(GetExitCodeProcess(child.hProcess,&exit_code)&&exit_code==0);
    rejected_read(reader,stereo_for(frame()),frame().received_ns); // The mapping still exists, but its publisher is dead.
    reader.close();CloseHandle(child.hThread);CloseHandle(child.hProcess);CloseHandle(ready);CloseHandle(finish);
    InputBridge replacement;CHECK(replacement.open(pid,true));
}
}

int main(int argc,char** argv) {
    if(argc>1&&std::strcmp(argv[1],"--publisher-child")==0)return publisher_child(argc,argv);
    exact_value_comparison();source_loss_and_boundaries();source_rejections_break_continuity();source_overflow_is_permanent();
    coexistence_and_skipped_loss();matching_age_and_ring();publication_rejection_does_not_consume();
    fixed_header_and_nonblocking_mutex();maximum_counters_do_not_wrap();publisher_process_lifetime();
    std::puts("controller continuity source history, exact matching, GVC3 coexistence, ring, identity and lifetime checks passed");
}
