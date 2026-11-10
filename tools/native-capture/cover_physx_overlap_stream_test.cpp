#include "cover_physx_query.h"
#include <cstdio>
#include <memory>
#include <thread>
namespace q=gears_cover_physx_query;
namespace d=q::detail;
static unsigned checks{},failed{};
static void check(bool value,char const* label){++checks;if(!value){++failed;printf("FAIL %s\n",label);}}
static q::Capsule expected_geometry{{1,2,3},{1,2,5},.5f};
static std::uint32_t __fastcall synthetic(std::uintptr_t supplied_scene,q::Capsule const* geometry,std::uint32_t types,std::uint32_t capacity,
    std::uintptr_t* batch,void* report,std::uint32_t groups,void const* mask,bool accurate){
    check(supplied_scene==0x180000&&geometry==&expected_geometry&&types==3&&capacity==64&&batch&&report&&groups==UINT32_MAX&&!mask&&accurate,"nine original enumeration arguments incl exact predicate");
    auto* receiver=static_cast<d::OverlapReporter*>(report);
    batch[0]=0x100000;batch[1]=0x200000;
    if(!receiver->vtable[0](receiver,2,batch))return 2;
    batch[2]=0x100000;
    if(!receiver->vtable[0](receiver,1,batch+2))return 3;
    receiver->vtable[0](receiver,0,batch+64);
    return 3;
}
static std::uint32_t __fastcall synthetic_fault(std::uintptr_t,q::Capsule const*,std::uint32_t,std::uint32_t,
    std::uintptr_t*,void*,std::uint32_t,void const*,bool){RaiseException(0xe0000123,0,0,nullptr);return 0;}
int main(){
    auto packet=std::make_unique<q::AllOverlap>();std::array<std::uintptr_t,64> batch{};
    auto make=[&](q::StreamLimits limits=q::StreamLimits{}){return d::OverlapReporter{d::overlap_vtable.data(),packet.get(),batch.data(),GetCurrentThreadId(),limits};};
    auto receiver=make();
    check(!d::overlap_report(&receiver,1,batch.data()),"unregistered callback refused");
    check(!d::overlap_report(reinterpret_cast<d::OverlapReporter*>(1),1,reinterpret_cast<std::uintptr_t const*>(1)),"unknown receiver never dereferenced");
    {d::OverlapScope scope(receiver);check(scope.admitted(),"owner admitted");
        d::OverlapScope nested(receiver);check(!nested.admitted(),"overlap nested owner refused");
        q::AllSweep sweep{};std::array<q::Hit,64> sweep_batch{};
        d::StreamReporter other{d::stream_vtable.data(),&sweep,sweep_batch.data(),GetCurrentThreadId(),{}};
        d::StreamScope sweep_scope(other);check(!sweep_scope.admitted(),"sweep owner cannot enter active overlap");
        std::uint32_t raw=99;
        check(d::overlap_shapes(reinterpret_cast<std::uintptr_t>(&synthetic),0x180000,expected_geometry,batch.data(),receiver,raw),"synthetic function returns through same ABI adapter");
        check(raw==3&&packet->shape_count==3&&packet->reports==3,"subspan and zero report counted exactly");
        check(packet->shapes[0]==0x100000&&packet->shapes[1]==0x200000&&packet->shapes[2]==0x100000&&!packet->shapes[3],"exact pointer order and duplicate retained, eight byte records");
        bool wrong_thread=true;std::thread thread([&](){wrong_thread=d::overlap_report(&receiver,1,batch.data());});thread.join();
        check(!wrong_thread&&packet->shape_count==3&&!receiver.malformed,"other thread callback cannot access owner");
        scope.close();scope.close();check(!d::active_overlap_reporter&&!d::active_query_reporter,"idempotent close clears both owner slots");}
    check(!d::overlap_report(&receiver,1,batch.data()),"expired callback refused");
    for(unsigned kind=0;kind<8;++kind){*packet={};receiver=make();d::OverlapScope scope(receiver);
        bool accepted=true;
        if(kind==0)accepted=d::overlap_report(&receiver,65,batch.data());
        if(kind==1)accepted=d::overlap_report(&receiver,2,batch.data()+63);
        if(kind==2)accepted=d::overlap_report(&receiver,1,reinterpret_cast<std::uintptr_t const*>(reinterpret_cast<std::uintptr_t>(batch.data())+1));
        if(kind==3)accepted=d::overlap_report(&receiver,1,reinterpret_cast<std::uintptr_t const*>(1));
        if(kind==4)accepted=d::overlap_report(&receiver,0,nullptr);
        if(kind==5){std::array<std::uintptr_t,64> fallback{};accepted=d::overlap_report(&receiver,1,fallback.data());}
        if(kind==6){receiver.thread+=1;accepted=d::overlap_report(&receiver,1,batch.data());}
        if(kind==7){receiver.result=reinterpret_cast<q::AllOverlap*>(batch.data());accepted=d::overlap_report(&receiver,1,batch.data());}
        check(!accepted&&!packet->shape_count,"malformed span/thread or untrusted original fallback buffer refused without copy");
    }
    { *packet={};receiver=make({3,10});d::OverlapScope scope(receiver);
        check(d::overlap_report(&receiver,3,batch.data()),"exact hit budget accepted");
        check(!d::overlap_report(&receiver,1,batch.data())&&receiver.truncated&&packet->shape_count==3,"hit budget exhausted explicitly without partial copy");
        check(!d::overlap_report(&receiver,0,batch.data()),"truncation cannot be reset by later empty callback");}
    { *packet={};receiver=make({2,10});d::OverlapScope scope(receiver);std::uint32_t raw=99;
        check(d::overlap_shapes(reinterpret_cast<std::uintptr_t>(&synthetic),0x180000,expected_geometry,batch.data(),receiver,raw),"normal original return can accompany callback stop");
        check(raw==3&&packet->shape_count==2&&receiver.truncated,"return count is not completeness; excess callback not copied");}
    { *packet={};receiver=make();d::OverlapScope scope(receiver);std::uint32_t raw=99;
        check(!d::overlap_shapes(reinterpret_cast<std::uintptr_t>(&synthetic_fault),0x180000,expected_geometry,batch.data(),receiver,raw)&&raw==99,"original call exception remains failure with unknown lifetime; count unchanged");}
    { *packet={};receiver=make({1024,2});d::OverlapScope scope(receiver);
        check(d::overlap_report(&receiver,0,batch.data())&&d::overlap_report(&receiver,0,batch.data()),"empty callbacks consume report budget");
        check(!d::overlap_report(&receiver,0,batch.data())&&receiver.truncated&&packet->reports==2,"report budget exhausted explicitly");}
    { *packet={};receiver=make();d::OverlapScope scope(receiver);
        for(unsigned n=0;n<16;++n)check(d::overlap_report(&receiver,64,batch.data()),"all1024 shape records retained");
        check(packet->shape_count==1024&&!d::overlap_report(&receiver,1,batch.data())&&receiver.truncated,"fixed output capacity never overflowed");}
    { *packet={};receiver=make();d::OverlapScope scope(receiver);batch[63]=UINT64_C(0x7777777766666666);
        check(d::overlap_report(&receiver,1,batch.data()+63)&&packet->shapes[0]==batch[63],"last eight-byte caller slot is valid");
        batch[63]=0;check(packet->shapes[0]==UINT64_C(0x7777777766666666),"copied pointer independent of later caller buffer reuse");
        check(!d::overlap_report(&receiver,1,batch.data()+64)&&receiver.untrusted_batch,"positive count at caller buffer end rejected");}
    { *packet={};receiver=make();d::OverlapScope scope(receiver);packet->shape_count=1025;
        check(!d::overlap_report(&receiver,1,batch.data())&&receiver.malformed,"corrupted count cannot become output index");}
    { *packet={};receiver=make();d::OverlapScope scope(receiver);
        check(!d::overlap_report(reinterpret_cast<d::OverlapReporter*>(1),1,batch.data())&&receiver.malformed,"wrong receiver invalidates current callback scope without reading stranger");
        check(!d::overlap_report(&receiver,0,batch.data()),"malformed callback state is sticky");}
    { q::AllSweep sweep{};std::array<q::Hit,64> sweep_batch{};
        d::StreamReporter other{d::stream_vtable.data(),&sweep,sweep_batch.data(),GetCurrentThreadId(),{}};
        d::StreamScope scope(other);receiver=make();d::OverlapScope nested(receiver);
        check(scope.admitted()&&!nested.admitted(),"overlap owner cannot enter active sweep");}
    check(!d::active_query_reporter&&!d::active_overlap_reporter&&!d::active_stream_reporter,"all callback scopes closed");
    printf("CoverOverlapStream checks=%u failed=%u original ABI with synthetic dispatch; raw pointers only; no game\n",checks,failed);
    return failed?1:0;
}
