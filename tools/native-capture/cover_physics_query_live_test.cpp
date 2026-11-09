#define GEARS_COMPONENT_ROLES_TEST_MAIN CoverComponentsPriorTestMain
#include "cover_component_roles_live_test.cpp"
#undef GEARS_COMPONENT_ROLES_TEST_MAIN
#include "cover_physics_query_live.inl"
#include <memory>
#include <thread>
namespace service=cover_physics_query_live;namespace q=gears_cover_physx_query;
static unsigned prime_calls{},close_calls{},query_calls{};static bool prime_ok=true,fault_query{},wrong_receiver{},query_lock_free=true,stream_truncated{},wrong_asset{},late_component_change{},late_asset_change{};
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
    gears_cover_physx_components::Workspace parts_work{};gears_cover_physx_components::Snapshot parts_value{};
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
    template<class Shape,class Read,class Guard>q::Code sweep_all(q::source::Source const& source,Shape const&,q::Vector const&,q::StreamLimits const&,Read read,Guard current,q::AllSweep& out){
        const auto code=dispatch(source,read,current);if(code!=q::Code::queried)return code;
        if(stream_truncated)return q::Code::truncated;
        out.raw_count=out.hit_count=2;out.reports=1;out.hits[1].bytes.fill(0x78);return code;
    }
    template<class Shape,class Read,class Guard>q::Code sweep_all(q::source::Source const& source,Shape const& shape,q::Vector const& delta,q::StreamLimits const& limits,Read read,Guard current,q::AllSweepOwners& out,gears_cover_physx_collider::Source const* component_source,gears_cover_physx_components::Source const* component_catalog){
        const auto code=sweep_all(source,shape,delta,limits,read,current,out.raw);
        if(code==q::Code::queried){
            namespace asset=gears_cover_physx_asset_actors;
            q::source::Snapshot scope{};asset::Workspace work{};asset::Snapshot inventory{};
            if(q::source::capture(0x180000000,0x300000,source,read,current,scope)!=q::source::Code::current)return q::Code::changed;
            if(asset::observe(0x180000000,0x300000,scope,read,current,work,inventory)!=asset::Code::observed)return q::Code::changed;
            if(wrong_asset)inventory.bodies[0].actor+=8;
            if(!current(inventory))return q::Code::changed;
            gears_cover_physx_collider::Snapshot component{};
            if(!component_source||gears_cover_physx_collider::observe(0x180000000,0x300000,scope,*component_source,read,current,component)!=gears_cover_physx_collider::Code::observed)return q::Code::changed;
            if(!component_catalog||gears_cover_physx_components::observe(0x180000000,0x300000,scope,*component_catalog,read,current,parts_work,parts_value)!=gears_cover_physx_components::Code::observed)return q::Code::changed;
            out.component_sources=parts_value;
            out.collider_source=component;
            out.asset_bodies=inventory;out.owners[0].actor=source.mesh;out.owners[1].actor=source.instance;
            if(late_component_change)put(audio+0x170,fixture_pawn);
            if(late_asset_change)put(weapon_mesh+0x250,physics_instance);
        }
        return code;
    }
};
using HostSession=service::Session<HostEngine>;
static HostSession* active_session{};static q::Box test_box{{1,2,3},{1,2,3},{1,0,0,0,1,0,0,0,1}};
static q::Code nested_code{};static bool pending_during{},poll_during{};
static void stop_during(){active_session->request_drain();pending_during=active_session->pending_count()!=0;poll_during=active_session->poll_stop();}
static void nested_query(){q::Overlap out{};nested_code=active_session->overlap(actor,physics_expected,test_box,source_read,guard,out);}
static void replace_root(){put(physics_instance+0x7c,0);}
static void replace_collider_body(){put(cylinder+0x1a0,uintptr_t{});}
static void replace_component_member(){put(arena+0xd0000,uintptr_t{});}
static void replace_component_owner(){put(audio+0x170,fixture_pawn);}
static void replace_component_asset(){put(weapon_mesh+0x250,physics_instance);}
static void reset(){physics_setup();prime_calls=close_calls=query_calls=0;prime_ok=true;fault_query=wrong_receiver=stream_truncated=wrong_asset=late_component_change=late_asset_change=false;during_query=nullptr;query_lock_free=true;}
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
    reset();
    {auto owner=std::make_unique<HostSession>();auto all=std::make_unique<q::AllSweep>();C(owner->start(true,true));
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::queried&&all->hit_count==2&&all->reports==1&&all->hits[1].bytes.back()==0x78);
        q::Capsule cap{{0,0,0},{0,0,1},.5f};C(owner->sweep_all(actor,physics_expected,cap,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::queried);
        all->raw_count=99;stream_truncated=true;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::truncated&&all->raw_count==99&&owner->enabled());
        const auto calls=query_calls;C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{0,1},source_read,guard,*all)==q::Code::request&&query_calls==calls);
        owner->request_drain();C(owner->poll_stop()&&!owner->pending_count());}
    reset();
    {catalog_setup();auto owner=std::make_unique<HostSession>();auto all=std::make_unique<q::AllSweepOwners>();C(owner->start(true,true));
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::queried&&all->raw.hit_count==2&&all->owners[0].actor!=all->owners[1].actor);
        C(all->asset_bodies.body_count==2&&all->asset_bodies.actor_count==2&&all->asset_bodies.actors[0]==arena+0x88000);
        C(all->collider_source.presence==gears_cover_physx_collider::Presence::actor_observed&&all->collider_source.fields.body_data==arena+0xb0000);
        C(all->component_sources.fields.component_count==4&&all->component_sources.actor_count==3&&all->component_sources.fields.components[2].owner_roots==0);
        C(all->component_sources.fields.asset_body_count==2&&all->component_sources.asset_observations[0].presence==gears_cover_physx_components::Presence::actor_observed&&
            all->component_sources.fields.assets[3].sampled&&!all->component_sources.fields.assets[3].instance);
        q::Capsule cap{{0,0,0},{0,0,1},.5f};C(owner->sweep_all(actor,physics_expected,cap,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::queried);
        put(cylinder+0x1a0,uintptr_t{});
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::queried&&
            all->collider_source.presence==gears_cover_physx_collider::Presence::body_instance_field_null&&!all->collider_source.fields.body);
        put(cylinder+0x1a0,single_body);put(single_body+0x88,uintptr_t{});
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::queried&&
            all->collider_source.presence==gears_cover_physx_collider::Presence::body_data_field_null&&all->collider_source.fields.body==single_body);
        put(single_body+0x88,arena+0xb0000);
        all->raw.raw_count=99;all->owners[0].actor=0xdead;all->asset_bodies.actor_count=55;wrong_asset=true;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::changed&&all->asset_bodies.actor_count==55&&all->raw.raw_count==99);
        wrong_asset=false;stream_truncated=true;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::truncated&&all->raw.raw_count==99&&all->owners[0].actor==0xdead);
        stream_truncated=false;during_query=replace_collider_body;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::changed&&all->raw.raw_count==99&&all->owners[0].actor==0xdead);
        put(cylinder+0x1a0,single_body);during_query=replace_component_member;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::changed&&all->raw.raw_count==99);
        put(arena+0xd0000,cylinder);during_query=replace_component_owner;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::changed&&all->raw.raw_count==99);
        put(audio+0x170,fixture_pc);during_query=nullptr;late_component_change=true;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::changed&&all->raw.raw_count==99);
        put(audio+0x170,fixture_pc);late_component_change=false;during_query=replace_component_asset;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::changed&&all->raw.raw_count==99);
        put(weapon_mesh+0x250,uintptr_t{});during_query=nullptr;late_asset_change=true;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::changed&&all->raw.raw_count==99);
        put(weapon_mesh+0x250,uintptr_t{});late_asset_change=false;during_query=replace_root;
        C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::changed&&all->raw.raw_count==99&&all->owners[0].actor==0xdead);
        owner->request_drain();C(owner->sweep_all(actor,physics_expected,test_box,delta,q::StreamLimits{},source_read,guard,*all)==q::Code::unavailable);
        C(owner->poll_stop()&&!owner->pending_count());}
    C(native_calls==0&&query_lock_free);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("cover_physics_query_live: %u checks, %u failures; dispatch/module are Host seams, no original call\n",checks,failures);return failures?1:0;
}
