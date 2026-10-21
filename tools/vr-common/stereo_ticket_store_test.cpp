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
static StereoTicketGeometry geometry_fixture() {
    StereoTicketGeometry g{};
    g.source_view={1,0,0,0,0,1,0,0,0,0,1,0,11,22,33,1};
    g.source_projection={2,0,0,0,0,3,0,0,.2f,-.1f,1,1,0,0,-.5f,0};
    for(unsigned eye=0;eye<2;++eye) {
        g.eyes.view[eye]=g.source_view;g.eyes.view[eye][12]=eye?9.f:13.f;
        g.eyes.projection[eye]=g.source_projection;g.eyes.projection[eye][8]=eye?-.3f:.4f;
    }
    return g;
}
static bool same_geometry(StereoTicketGeometry const& a,StereoTicketGeometry const& b) {
    if(a.source_view!=b.source_view||a.source_projection!=b.source_projection||a.aiming!=b.aiming)return false;
    if(a.finite_aim.valid!=b.finite_aim.valid||a.finite_aim.point!=b.finite_aim.point||
       a.finite_aim.world!=b.finite_aim.world||a.finite_aim.tick!=b.finite_aim.tick)return false;
    for(unsigned eye=0;eye<2;++eye)
        if(a.eyes.view[eye]!=b.eyes.view[eye]||a.eyes.projection[eye]!=b.eyes.projection[eye])return false;
    return true;
}
static int finite_geometry_tests() {
    const auto original=geometry_fixture();auto sample=fixture();
    // Preserve the previous three-argument aggregate initialization contract.
    StereoTicketGeometry geometry{original.source_view,original.source_projection,original.eyes};
    CHECK(!geometry.finite_aim.valid);
    geometry.finite_aim={true,{1,2,3},4,5};const auto frozen=geometry;
    StereoTicketStore store;
    CHECK(store.retain(1,sample,false,&geometry)==TicketRetainResult::retained);
    CHECK(store.retain(1,sample,false,&geometry)==TicketRetainResult::duplicate);
    for(unsigned field=0;field<7;++field){auto changed=frozen;
        if(field==0)changed.finite_aim.valid=false;
        if(field>=1&&field<=3)changed.finite_aim.point[field-1]+=1;
        if(field==4)++changed.finite_aim.world;if(field==5)++changed.finite_aim.tick;
        if(field==6)changed.aiming=!changed.aiming;
        CHECK(store.retain(1,sample,false,&changed)==TicketRetainResult::conflict);
    }
    for(unsigned fault=0;fault<8;++fault){auto bad=frozen;
        if(fault<3)bad.finite_aim.point[fault]=std::numeric_limits<float>::quiet_NaN();
        if(fault>=3&&fault<6)bad.finite_aim.point[fault-3]=std::numeric_limits<float>::infinity();
        if(fault==6)bad.finite_aim.world=0;if(fault==7)bad.finite_aim.tick=0;
        CHECK(store.retain(2,sample,false,&bad)==TicketRetainResult::invalid);
        CHECK(store.size()==1&&store.highest_id()==1);
    }
    geometry.finite_aim.point={100,200,300};geometry.finite_aim.world=999;
    StereoTicketGeometry copy{};CHECK(store.lookup_geometry(1,copy)&&same_geometry(copy,frozen));
    copy.finite_aim.tick=99;CHECK(store.lookup_geometry(1,copy)&&same_geometry(copy,frozen));
    CHECK(store.retire_through(1));const auto sentinel=copy;
    CHECK(!store.lookup_geometry(1,copy)&&same_geometry(copy,sentinel));
    CHECK(store.retain(2,sample,false,&original)==TicketRetainResult::retained);
    CHECK(store.lookup_geometry(2,copy)&&!copy.finite_aim.valid); // Reused slot has no old point.
    return 0;
}
static int geometry_tests() {
    auto sample=fixture();auto original=geometry_fixture();auto geometry=original;
    StereoTicketStore store;StereoTicketGeometry copy=original;copy.source_view[12]=100;
    auto sentinel=copy;
    CHECK(!store.lookup_geometry(0,copy)&&same_geometry(copy,sentinel));
    CHECK(store.retain(1,sample)==TicketRetainResult::retained);
    CHECK(!store.lookup_geometry(1,copy)&&same_geometry(copy,sentinel));
    CHECK(store.retain(1,sample,false,&geometry)==TicketRetainResult::conflict); // No upgrade.
    CHECK(store.retain(2,sample,false,&geometry)==TicketRetainResult::retained);
    CHECK(store.retain(2,sample,false,&geometry)==TicketRetainResult::duplicate);
    CHECK(store.retain(2,sample)==TicketRetainResult::conflict); // No downgrade.
    auto changed_sample=sample;++changed_sample.sequence;
    CHECK(store.retain(2,changed_sample,false,&geometry)==TicketRetainResult::conflict);
    // Each independent matrix participates in immutable duplicate identity.
    for(unsigned matrix=0;matrix<6;++matrix) {
        auto changed=original;
        if(matrix==0)changed.source_view[12]+=1;
        if(matrix==1)changed.source_projection[0]+=1;
        if(matrix>=2&&matrix<4)changed.eyes.view[matrix-2][12]+=1;
        if(matrix>=4)changed.eyes.projection[matrix-4][8]+=.1f;
        CHECK(store.retain(2,sample,false,&changed)==TicketRetainResult::conflict);
    }
    // Caller memory and output copies cannot alter the retained geometry.
    geometry.source_view[12]=777;geometry.eyes.view[1][12]=888;
    CHECK(store.lookup_geometry(2,copy));
    CHECK(copy.source_view[12]==11&&copy.eyes.view[0][12]==13&&copy.eyes.view[1][12]==9);
    CHECK(copy.source_projection[8]==.2f&&copy.eyes.projection[0][8]==.4f&&copy.eyes.projection[1][8]==-.3f);
    copy.eyes.projection[0][8]=99;CHECK(store.lookup_geometry(2,copy)&&same_geometry(copy,original));
    for(unsigned invalid=0;invalid<6;++invalid) {
        auto bad=original;
        if(invalid==0)bad.source_view[12]=std::numeric_limits<float>::quiet_NaN();
        if(invalid==1)bad.source_view[0]=2;
        if(invalid==2)bad.source_projection[11]=0;
        if(invalid==3)bad.eyes.view[0][15]=0;
        if(invalid==4)bad.eyes.projection[1][14]=std::numeric_limits<float>::infinity();
        if(invalid==5)bad.eyes.projection[0][3]=1;
        CHECK(store.retain(3,sample,false,&bad)==TicketRetainResult::invalid);
        CHECK(store.size()==2&&store.highest_id()==2);
    }
    auto future=original;future.source_view[13]=55;
    CHECK(store.retain(4,sample,false,&original)==TicketRetainResult::retained);
    CHECK(store.retain(7,sample,false,&future)==TicketRetainResult::retained);
    CHECK(store.retire_through(4));copy=sentinel;
    CHECK(!store.lookup_geometry(2,copy)&&same_geometry(copy,sentinel));
    CHECK(store.lookup_geometry(7,copy)&&copy.source_view[13]==55);
    CHECK(store.retain(4,sample,false,&original)==TicketRetainResult::stale);
    CHECK(store.retain(6,sample,false,&original)==TicketRetainResult::stale);
    CHECK(!store.retire_through(6)&&store.lookup_geometry(7,copy));
    CHECK(store.retire_through(7));copy=sentinel;
    CHECK(!store.lookup_geometry(7,copy)&&same_geometry(copy,sentinel));
    // Capacity and subsequent reuse retire sample and geometry together.
    StereoTicketStore full;
    for(std::uint64_t id=1;id<=StereoTicketStore::capacity;++id)
        CHECK(full.retain(id,sample,false,&original)==TicketRetainResult::retained);
    CHECK(full.retain(129,sample,false,&future)==TicketRetainResult::overflow);
    CHECK(full.retain(1,sample,false,&original)==TicketRetainResult::duplicate);
    CHECK(full.lookup_geometry(128,copy)&&same_geometry(copy,original));
    CHECK(full.retire_through(64)&&full.retain(129,sample,false,&future)==TicketRetainResult::retained);
    CHECK(full.lookup_geometry(129,copy)&&same_geometry(copy,future));
    CHECK(full.retain(130,sample)==TicketRetainResult::retained);copy=sentinel;
    CHECK(!full.lookup_geometry(130,copy)&&same_geometry(copy,sentinel)); // Reused slot has no old geometry.
    // The same explicit simulator admission applies to geometry lookups.
    auto sim=sample;sim.source=StereoSource::MetaOperatorSimulated;sim.head_flags=3;
    CHECK(store.retain(8,sim,true,&original)==TicketRetainResult::retained);copy=sentinel;
    CHECK(!store.lookup_geometry(8,copy)&&same_geometry(copy,sentinel));
    CHECK(store.lookup_geometry(8,copy,true)&&same_geometry(copy,original));
    return 0;
}
static int visible_tests() {
    StereoTicketStore store;auto visible=fixture();visible.focused=0;visible.visible=1;
    const auto geometry=geometry_fixture();StereoSample copy{};StereoTicketGeometry geometry_copy{};
    CHECK(store.retain(1,visible,false,&geometry)==TicketRetainResult::retained);
    CHECK(store.retain(1,visible,false,&geometry)==TicketRetainResult::duplicate);
    CHECK(store.lookup(1,copy)&&stereo_same(copy,visible)&&copy.focused==0&&copy.visible==1);
    CHECK(store.lookup_geometry(1,geometry_copy)&&same_geometry(geometry_copy,geometry));
    auto focused=visible;focused.focused=1;
    CHECK(store.retain(1,focused,false,&geometry)==TicketRetainResult::conflict);
    focused.visible=0;
    CHECK(store.retain(1,focused,false,&geometry)==TicketRetainResult::conflict);
    CHECK(store.retain(2,focused)==TicketRetainResult::retained);
    // Invalid or hidden samples cannot replace an admitted visible ticket.
    for(unsigned invalid=0;invalid<4;++invalid){auto bad=visible;
        if(invalid==0)bad.visible=0;if(invalid==1)bad.visible=2;
        if(invalid==2)bad.focused=2;if(invalid==3)bad.should_render=0;
        CHECK(store.retain(3,bad)==TicketRetainResult::invalid);
        CHECK(store.lookup(1,copy)&&stereo_same(copy,visible));
        CHECK(store.size()==2&&store.highest_id()==2);
    }
    CHECK(store.retire_through(1));
    CHECK(!store.lookup(1,copy)&&store.lookup(2,copy)&&stereo_same(copy,focused));
    return 0;
}
int main() {
    if(auto result=finite_geometry_tests())return result;
    if(auto result=visible_tests())return result;
    if(auto result=geometry_tests())return result;
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
    image.sample.space_epoch=0;CHECK(!stereo_image_ticket_valid(image,640,360,true));
    image.sample=simulated;
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
    auto frozen=geometry_fixture();
    StereoTicketStore concurrent;CHECK(concurrent.retain(1,sample,false,&frozen)==TicketRetainResult::retained);
    std::atomic<bool> start{false};std::atomic<unsigned> failures{0};std::array<std::thread,8> readers;
    for(auto& reader:readers)reader=std::thread([&]{while(!start.load())std::this_thread::yield();
        for(unsigned iteration=0;iteration<3000;++iteration){StereoSample value;StereoTicketGeometry geometry_value;
            if(!concurrent.lookup(1,value)||!stereo_same(value,sample)||
               !concurrent.lookup_geometry(1,geometry_value)||!same_geometry(geometry_value,frozen)||
               concurrent.retain(1,sample,false,&frozen)!=TicketRetainResult::duplicate)++failures;
            // No retirement occurs here: once the sample is published, geometry
            // must already be published with it, even during future insertion.
            const auto future_id=std::uint64_t(iteration%StereoTicketStore::capacity+1);
            if(concurrent.lookup(future_id,value)&&
               (!concurrent.lookup_geometry(future_id,geometry_value)||!same_geometry(geometry_value,frozen)))++failures;}});
    start=true;
    for(std::uint64_t id=2;id<=128;++id)if(concurrent.retain(id,sample,false,&frozen)!=TicketRetainResult::retained)++failures;
    for(auto& reader:readers)reader.join();CHECK(failures==0&&concurrent.size()==128);
    CHECK(concurrent.retire_through(100)&&concurrent.size()==28&&concurrent.lookup(128,copied));
    CHECK(!concurrent.lookup(1,copied)&&concurrent.retain(1,sample)==TicketRetainResult::stale);
    std::printf("stereo ticket store tests passed: sample/geometry/conflict/invalid/overflow/boundary/future/replay/concurrency\n");
    return 0;
}
