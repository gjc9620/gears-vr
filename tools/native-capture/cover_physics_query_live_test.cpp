#define GEARS_PHYSICS_ROLES_TEST_MAIN CoverPhysicsRolesPriorTestMain
#include "cover_physics_roles_live_test.cpp"
#undef GEARS_PHYSICS_ROLES_TEST_MAIN
#include "cover_physics_query_live.inl"
#include <memory>
#include <thread>
namespace service=cover_physics_query_live;namespace q=gears_cover_physx_query;
static unsigned prime_calls{},close_calls{},query_calls{};static bool prime_ok=true,fault_query{},wrong_receiver{},query_lock_free=true;
static void(*during_query)(){};
static bool source_read(void* dst,void const* address,size_t n){
    constexpr uintptr_t physx=0x180000000;
    constexpr std::array<std::pair<unsigned,unsigned>,10> slots{{
        {0x2330b0,0x135b00},{0x22ccc0,0xa0300},{0x23cda0,0x171770},{0x23cda8,0x1716f0},{0x23cdc8,0x168350},
        {0x23cdd0,0x1682d0},{0x22cf40,0xa6e30},{0x22cf48,0xa6aa0},{0x22cf20,0xa6290},{0x22cf28,0xa60b0}}};
    for(auto const& slot:slots)if(reinterpret_cast<uintptr_t>(address)==physx+slot.first&&n==8){const auto target=physx+slot.second;memcpy(dst,&target,8);return true;}
    return safe_copy(dst,address,n);
}
// Replaces ONLY original module loading/dispatch. Real typed roles, registry,
// source path and Session code execute above; no original callee is executed.
struct HostEngine {
    bool active{},unknown{};
    bool prime(){++prime_calls;return prime_ok;}
    bool close(){++close_calls;return !pending();}
    bool pending()const{return active||unknown;}
    bool uncertain()const{return unknown;}
    template<class Guard,class Read>q::Code dispatch(q::source::Source const& source,Read read,Guard current){
        ++query_calls;active=true;
        if(!h::mutex.try_lock()){query_lock_free=false;active=false;return q::Code::source;}h::mutex.unlock();
        q::source::Snapshot raw{};
        const auto code=q::source::capture(0x180000000,0x300000,source,read,current,raw);
        if(code!=q::source::Code::current){active=false;return q::Code::source;}
        if(during_query)during_query();
        if(fault_query){unknown=true;active=false;return q::Code::fault_unknown;}
        if(wrong_receiver)raw.actor+=8;
        const bool accepted=q::detail::source_guard(current,raw);active=false;
        return accepted?q::Code::queried:q::Code::changed;
    }
    template<class Shape,class Read,class Guard>q::Code overlap(q::source::Source const& source,Shape const&,Read read,Guard current,q::Overlap& out){
        const auto code=dispatch(source,read,current);if(code==q::Code::queried)out.raw_hit=true;return code;
    }
    template<class Shape,class Read,class Guard>q::Code sweep(q::source::Source const& source,Shape const&,q::Vector const&,Read read,Guard current,q::Sweep& out){
        const auto code=dispatch(source,read,current);if(code==q::Code::queried){out.raw_count=1;out.raw_hit.bytes.fill(0x75);}return code;
    }
};
using HostSession=service::Session<HostEngine>;
static HostSession* active_session{};static q::Box test_box{{1,2,3},{1,2,3},{1,0,0,0,1,0,0,0,1}};
static q::Code nested_code{};static bool pending_during{},poll_during{};
static void stop_during(){active_session->request_drain();pending_during=active_session->pending_count()!=0;poll_during=active_session->poll_stop();}
static void nested_query(){q::Overlap out{};nested_code=active_session->overlap(actor,physics_expected,test_box,source_read,guard,out);}
static void replace_root(){put(physics_instance+0x7c,0);}
static void reset(){physics_setup();prime_calls=close_calls=query_calls=0;prime_ok=true;fault_query=wrong_receiver=false;during_query=nullptr;query_lock_free=true;}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    reset();q::Overlap overlap_result{};q::Sweep swept{};q::Vector delta{.1f,.2f,.3f};
    {auto owner=std::make_unique<HostSession>();C(owner->start(false,true));C(!owner->enabled()&&!owner->pending_count()&&!prime_calls);
        const auto old_reads=reads;C(owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::unavailable&&reads==old_reads);
        C(!owner->start(true,true));owner->request_drain();C(owner->poll_stop()&&!close_calls);}
    {auto owner=std::make_unique<HostSession>();C(!owner->start(true,false)&&!prime_calls&&!owner->pending_count());}
    reset();prime_ok=false;
    {auto owner=std::make_unique<HostSession>();C(!owner->start(true,true)&&prime_calls==1&&close_calls==1&&!owner->pending_count());}
    reset();
    {auto owner=std::make_unique<HostSession>();active_session=owner.get();C(owner->start(true,true)&&owner->enabled()&&owner->pending_count());
        memcpy(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size());
        C(owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::queried&&overlap_result.raw_hit);
        C(owner->sweep(actor,physics_expected,test_box,delta,source_read,guard,swept)==q::Code::queried&&swept.raw_count==1&&swept.raw_hit.bytes.back()==0x75);
        C(!memcmp(saved_arena.data(),reinterpret_cast<void*>(arena),saved_arena.size())&&query_lock_free&&query_calls==2);
        during_query=nested_query;C(owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::queried&&nested_code==q::Code::reentrant);
        during_query=nullptr;owner->request_drain();bool other_worker=true;std::thread thread([&](){other_worker=owner->poll_stop();});thread.join();
        C(!other_worker&&close_calls==0&&owner->pending_count());C(owner->poll_stop()&&close_calls==1&&!owner->pending_count());C(owner->poll_stop()&&close_calls==1);}
    reset();
    {auto owner=std::make_unique<HostSession>();C(owner->start(true,true));wrong_receiver=true;overlap_result.raw_hit=false;
        C(owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::changed&&!overlap_result.raw_hit);
        wrong_receiver=false;during_query=replace_root;
        C(owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::changed&&!overlap_result.raw_hit);
        during_query=nullptr;owner->request_drain();C(owner->poll_stop());}
    reset();
    {auto owner=std::make_unique<HostSession>();active_session=owner.get();C(owner->start(true,true));during_query=stop_during;overlap_result.raw_hit=false;
        C(owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::changed&&!overlap_result.raw_hit);
        C(pending_during&&!poll_during&&close_calls==0);C(owner->poll_stop()&&close_calls==1&&!owner->pending_count());}
    reset();
    {auto owner=std::make_unique<HostSession>();C(owner->start(true,true));fault_query=true;overlap_result.raw_hit=false;
        C(owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::fault_unknown&&!overlap_result.raw_hit);
        const auto fault_calls=query_calls;
        C(!owner->enabled()&&owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::unavailable&&query_calls==fault_calls);
        owner->request_drain();C(owner->uncertain()&&owner->pending_count()&&!owner->poll_stop()&&close_calls==0);
        C(!owner->poll_stop()&&owner->pending_count());const auto calls=query_calls;
        C(owner->overlap(actor,physics_expected,test_box,source_read,guard,overlap_result)==q::Code::unavailable&&query_calls==calls);}
    reset();
    {auto owner=std::make_unique<HostSession>();C(owner->start(true,true));swept.raw_count=99;
        C(owner->sweep(actor,physics_expected,test_box,q::Vector{},source_read,guard,swept)==q::Code::request&&swept.raw_count==99&&!query_calls);
        auto bad=test_box;bad.half.x=0;C(owner->overlap(actor,physics_expected,bad,source_read,guard,overlap_result)==q::Code::request&&!query_calls);
        C(owner->overlap(actor,physics_expected,test_box,source_read,[](){return false;},overlap_result)==q::Code::source&&!query_calls);
        owner->request_drain();C(owner->poll_stop());}
    C(native_calls==0&&query_lock_free);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("cover_physics_query_live: %u checks, %u failures; dispatch/module are Host seams, no original call\n",checks,failures);return failures?1:0;
}
