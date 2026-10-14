#include "stereo_ticket_store.h"
#include "stereo_image_ticket.h"
#include <atomic>
#include <cstdio>
#include <limits>
#include <thread>

using namespace gears_vr;
static StereoSample fixture() {
    StereoSample s{};s.session=9;s.sequence=18;s.received_ns=1000000000;s.predicted_time=1001234567;
    s.head_flags=s.view_flags=15;s.focused=s.should_render=1;s.head.orientation={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.orientation={};s.eyes[i].pose.position.x=i?.032:-.032;
        s.eyes[i].fov={-.8,.7,.75,-.65};}
    return s;
}
#define CHECK(condition) do{if(!(condition)){std::printf("Failed line %d: %s\n",__LINE__,#condition);return __LINE__;}}while(0)
int main() {
    auto simulated=fixture();simulated.source=StereoSource::MetaOperatorSimulated;simulated.head_flags=3;
    StereoTicketStore simulation_store;StereoSample raw_copy{};
    CHECK(simulation_store.retain(1,simulated)==TicketRetainResult::invalid);
    CHECK(simulation_store.retain(1,simulated,true)==TicketRetainResult::retained);
    CHECK(!simulation_store.lookup(1,raw_copy));
    CHECK(simulation_store.lookup(1,raw_copy,true)&&stereo_same(raw_copy,simulated)&&raw_copy.head_flags==3);
    auto unknown=simulated;unknown.source=static_cast<StereoSource>(99);
    CHECK(simulation_store.retain(2,unknown,true)==TicketRetainResult::invalid);
    auto source_conflict=simulated;source_conflict.source=StereoSource::RuntimeTracked;source_conflict.head_flags=15;
    CHECK(simulation_store.retain(1,source_conflict,true)==TicketRetainResult::invalid);
    CHECK(simulation_store.retain(1,source_conflict)==TicketRetainResult::conflict);
    CHECK(!stereo_same(source_conflict,simulated));
    source_conflict.head_flags=3;CHECK(simulation_store.retain(2,source_conflict,true)==TicketRetainResult::invalid);
    auto missing_view_tracking=simulated;missing_view_tracking.view_flags=3;
    CHECK(simulation_store.retain(2,missing_view_tracking,true)==TicketRetainResult::invalid);
    StereoImageTicket image{};image.id=1;image.sample=simulated;
    image.rect[0]={0,0,320,360};image.rect[1]={320,0,320,360};
    CHECK(!stereo_image_ticket_valid(image,640,360));CHECK(stereo_image_ticket_valid(image,640,360,true));
    CHECK(image.sample.head_flags==3);
    image.sample.source=static_cast<StereoSource>(99);CHECK(!stereo_image_ticket_valid(image,640,360,true));
    image.sample=simulated;image.sample.head_flags=1;CHECK(!stereo_image_ticket_valid(image,640,360,true));
    image.sample=missing_view_tracking;CHECK(!stereo_image_ticket_valid(image,640,360,true));
    image.sample=simulated;image.rect[1].x=0;CHECK(!stereo_image_ticket_valid(image,640,360,true));
    auto sample=fixture();StereoTicketStore store;StereoSample copied{};
    CHECK(store.retain(1,sample)==TicketRetainResult::retained);
    CHECK(store.retain(1,sample)==TicketRetainResult::duplicate);
    auto changed=sample;++changed.predicted_time;
    CHECK(store.retain(1,changed)==TicketRetainResult::conflict);
    CHECK(store.lookup(1,copied)&&stereo_same(copied,sample));
    // Caller mutations and renderer retirement cannot change retained values.
    changed.head.position.x=50;CHECK(store.lookup(1,copied)&&copied.head.position.x==0);
    for(unsigned invalid=0;invalid<8;++invalid){auto bad=sample;
        if(invalid==0)bad.session=0;if(invalid==1)bad.sequence=0;if(invalid==2)bad.received_ns=0;
        if(invalid==3)bad.focused=0;if(invalid==4)bad.should_render=0;if(invalid==5)bad.view_flags=7;
        if(invalid==6)bad.head.position.x=std::numeric_limits<double>::quiet_NaN();
        if(invalid==7)bad.eyes[1].pose.position.x=bad.eyes[0].pose.position.x;
        CHECK(store.retain(2,bad)==TicketRetainResult::invalid);
    }
    CHECK(store.retain(0,sample)==TicketRetainResult::invalid);
    // Future window ticket survives present retiring the observed older window.
    CHECK(store.retain(4,sample)==TicketRetainResult::retained);
    CHECK(store.retain(7,sample)==TicketRetainResult::retained);
    CHECK(store.retire_through(4));CHECK(store.size()==1&&store.lookup(7,copied));
    CHECK(!store.lookup(1,copied)&&stereo_same(copied,sample));
    CHECK(store.retain(1,sample)==TicketRetainResult::stale);
    CHECK(store.retain(5,sample)==TicketRetainResult::stale); // Never-used old id.
    CHECK(!store.retire_through(6)&&!store.retire_through(8));
    CHECK(store.retire_through(4)&&store.retire_through(0));
    CHECK(store.retire_through(7)&&store.size()==0&&store.highest_id()==7);
    CHECK(store.retain(7,sample)==TicketRetainResult::stale);
    CHECK(store.retain(8,sample)==TicketRetainResult::retained);
    StereoTicketStore full;
    for(std::uint64_t id=1;id<=128;++id)CHECK(full.retain(id,sample)==TicketRetainResult::retained);
    CHECK(full.size()==128&&full.retain(129,sample)==TicketRetainResult::overflow);
    CHECK(full.retain(1,sample)==TicketRetainResult::duplicate);
    CHECK(full.lookup(128,copied)&&stereo_same(copied,sample));
    CHECK(full.retire_through(64)&&full.size()==64);
    CHECK(full.lookup(128,copied)&&full.retain(129,sample)==TicketRetainResult::retained);
    // Synchronized value copies while one ordered producer inserts future ids.
    StereoTicketStore concurrent;CHECK(concurrent.retain(1,sample)==TicketRetainResult::retained);
    std::atomic<bool> start{false};std::atomic<unsigned> failures{0};std::array<std::thread,8> readers;
    for(auto& reader:readers)reader=std::thread([&]{while(!start.load())std::this_thread::yield();
        for(unsigned iteration=0;iteration<3000;++iteration){StereoSample value;
            if(!concurrent.lookup(1,value)||!stereo_same(value,sample)||concurrent.retain(1,sample)!=TicketRetainResult::duplicate)++failures;}});
    start=true;
    for(std::uint64_t id=2;id<=128;++id)if(concurrent.retain(id,sample)!=TicketRetainResult::retained)++failures;
    for(auto& reader:readers)reader.join();CHECK(failures==0&&concurrent.size()==128);
    CHECK(concurrent.retire_through(100)&&concurrent.size()==28&&concurrent.lookup(128,copied));
    CHECK(!concurrent.lookup(1,copied)&&concurrent.retain(1,sample)==TicketRetainResult::stale);
    std::printf("stereo ticket store tests passed: retain/conflict/overflow/boundary/future/replay/concurrency\n");
    return 0;
}
