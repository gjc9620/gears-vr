#include "cover_physx_query.h"
#include <cstdio>
#include <memory>
#include <thread>
namespace q=gears_cover_physx_query;
static unsigned checks{},failures{};
static void check(bool value,char const* what){++checks;if(!value){++failures;printf("FAIL %s\n",what);}}
static void const* expected_shape{};static q::Vector const* expected_delta{};
static bool __fastcall dispatch(q::detail::StreamReporter& callback,std::uint32_t n,q::Hit const* data){return callback.vtable[0](&callback,n,data);}
static std::uint32_t __fastcall synthetic(std::uintptr_t scene,void const* shape,q::Vector const* delta,std::uint32_t flags,
    void* cache,std::uint32_t capacity,q::Hit* batch,void* receiver,std::uint32_t groups,void const* mask){
    check(scene==0x180000&&shape==expected_shape&&delta==expected_delta&&flags==11&&!cache&&capacity==64&&batch&&receiver&&groups==UINT32_MAX&&!mask,"original10-argument all-hit ABI");
    auto& reporter=*static_cast<q::detail::StreamReporter*>(receiver);std::uint32_t raw{};
    batch[0].bytes.fill(0x11);raw+=1;if(!dispatch(reporter,1,batch))return raw;
    for(unsigned i=0;i<64;++i)batch[i].bytes.fill(static_cast<unsigned char>(i));raw+=64;
    if(!dispatch(reporter,64,batch))return raw;
    if(!dispatch(reporter,0,batch))return raw;
    batch[0].bytes.fill(0xee);raw+=1;dispatch(reporter,1,batch);return raw;
}
int main(){
    auto result=std::make_unique<q::AllSweep>();std::array<q::Hit,64> batch{};
    auto make=[&](q::StreamLimits limits=q::StreamLimits{}){return q::detail::StreamReporter{q::detail::stream_vtable.data(),result.get(),batch.data(),GetCurrentThreadId(),limits};};
    auto reporter=make();
    check(!dispatch(reporter,1,batch.data()),"unregistered callback refused");
    {q::detail::StreamScope scope(reporter);check(scope.admitted(),"one synchronous owner");
        q::detail::StreamScope nested(reporter);check(!nested.admitted(),"nested callback owner refused");
        check(dispatch(reporter,0,batch.data())&&result->reports==1&&!result->hit_count,"zero report is not end marker");
        batch[0].bytes.fill(0x75);check(dispatch(reporter,1,batch.data())&&result->hit_count==1&&result->hits[0].bytes.back()==0x75,"exact64-byte copy");
        bool other_thread=true;std::thread thread([&](){other_thread=dispatch(reporter,1,batch.data());});thread.join();
        check(!other_thread&&result->hit_count==1,"other thread cannot read/write reporter");}
    check(!dispatch(reporter,1,batch.data())&&!q::detail::active_stream_reporter,"late callback refused without state access");
    check(!q::detail::stream_report(reinterpret_cast<q::detail::StreamReporter*>(1),1,reinterpret_cast<q::Hit const*>(1)),"unrecognized pointer not dereferenced");
    for(unsigned kind=0;kind<4;++kind){*result={};reporter=make();q::detail::StreamScope scope(reporter);
        if(kind==0)check(!dispatch(reporter,65,batch.data()),"oversized batch refused");
        if(kind==1)check(!dispatch(reporter,1,batch.data()+1),"foreign record pointer refused");
        if(kind==2){reporter.thread++;check(!dispatch(reporter,1,batch.data()),"wrong reporter thread refused");}
        if(kind==3)check(!q::detail::stream_report(reinterpret_cast<q::detail::StreamReporter*>(1),1,batch.data()),"wrong callback object refused");
        check(reporter.malformed&&!result->hit_count&&!result->reports,"malformed packet never copied");}
    *result={};reporter=make({1,8});
    {q::detail::StreamScope scope(reporter);check(dispatch(reporter,1,batch.data()),"hit budget exact endpoint accepted");
        check(!dispatch(reporter,1,batch.data())&&reporter.truncated&&!reporter.malformed&&result->hit_count==1,"hit overflow explicit truncation");}
    *result={};reporter=make({1024,1});
    {q::detail::StreamScope scope(reporter);check(dispatch(reporter,0,batch.data()),"report budget includes empty packet");
        check(!dispatch(reporter,0,batch.data())&&reporter.truncated&&result->reports==1,"report overflow explicit truncation");}
    for(auto limits:std::array<q::StreamLimits,4>{{{0,1},{1025,1},{1,0},{1,257}}})check(!q::detail::stream_limits(limits),"invalid budgets rejected");
    q::Box box{{1,2,3},{1,2,3},{1,0,0,0,1,0,0,0,1}};q::Capsule capsule{{0,0,0},{0,0,1},.5f};q::Vector delta{.1f,.2f,.3f};expected_delta=&delta;
    auto invoke=[&](auto const& shape){*result={};reporter=make();q::detail::StreamScope scope(reporter);expected_shape=&shape;std::uint32_t raw{};
        check(q::detail::sweep_stream(reinterpret_cast<std::uintptr_t>(&synthetic),0x180000,shape,delta,batch.data(),reporter,raw),"typed stream dispatch returned");
        check(raw==66&&result->hit_count==66&&result->reports==4&&!reporter.truncated&&!reporter.malformed,"all batches consumed including final after empty");
        check(result->hits[0].bytes.front()==0x11&&result->hits[1].bytes.front()==0&&result->hits[64].bytes.back()==63&&result->hits[65].bytes.back()==0xee,"packet order and full record payload");};
    invoke(box);invoke(capsule);
    *result={};reporter=make({1,256});expected_shape=&box;std::uint32_t raw{};
    {q::detail::StreamScope scope(reporter);check(q::detail::sweep_stream(reinterpret_cast<std::uintptr_t>(&synthetic),0x180000,box,delta,batch.data(),reporter,raw),"truncated native-style return");
        check(reporter.truncated&&raw==65&&result->hit_count==1,"native count does not hide rejected batch");}
    q::Queries queries;q::source::Source source{};source.thread=GetCurrentThreadId();result->raw_count=99;
    auto read=[](void*,void const*,size_t){return false;};auto guard=[](){return true;};
    check(queries.sweep_all(source,box,delta,q::StreamLimits{},read,guard,*result)==q::Code::unavailable&&result->raw_count==99,"unprimed stream output unchanged");
    check(queries.sweep_all(source,capsule,delta,q::StreamLimits{},read,guard,*result)==q::Code::unavailable,"capsule stream instantiated");
    check(queries.sweep_all(source,box,delta,q::StreamLimits{0,1},read,guard,*result)==q::Code::request,"invalid query budget rejected");
    check(queries.sweep_all(source,box,q::Vector{},q::StreamLimits{},read,guard,*result)==q::Code::request,"zero stream not occupancy");
    check(!q::detail::active_stream_reporter&&!queries.pending()&&queries.close()&&!GetModuleHandleW(L"PhysXCore.dll"),"no callback/module/query debt");
    printf("CoverPhysXStream checks=%u failures=%u original-executed=false\n",checks,failures);return failures?1:0;
}
