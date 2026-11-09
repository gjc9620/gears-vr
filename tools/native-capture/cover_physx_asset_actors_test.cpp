#include "cover_physx_asset_actors.h"
#define main PriorPhysXAssetSourceFixtureMain
#include "cover_physx_query_test.cpp"
#undef main
#include <thread>
namespace asset=gears_cover_physx_asset_actors;
constexpr std::uintptr_t extra_actor=0x190000,root_internal=0x1a0000,extra_internal=0x1b0000;
static Memory asset_fixture(){
    auto memory=fixture();memory.block(0x160000,0x48);memory.block(extra_actor,0x48);
    memory.put<std::uintptr_t>(0x160000,base+0x2330a8);memory.put<std::uintptr_t>(0x160038,0x170000);
    memory.block(root_internal,0x18);memory.block(extra_internal,0x18);
    memory.put<std::uintptr_t>(0x160040,root_internal);memory.put<std::uintptr_t>(root_internal+0x10,0x160000);
    memory.put<std::uintptr_t>(extra_actor,base+0x2330a8);memory.put<std::uintptr_t>(extra_actor+0x38,0x170000);
    memory.put<std::uintptr_t>(extra_actor+0x40,extra_internal);memory.put<std::uintptr_t>(extra_internal+0x10,extra_actor);
    return memory;
}
struct AssetGuard {
    unsigned* calls{};unsigned fail_at{};
    bool operator()(){return !calls||++*calls!=fail_at;}
    bool operator()(asset::Snapshot const& value){return operator()()&&value.query_source.actor==0x160000;}
};
int main(){
    auto memory=asset_fixture();
    auto read=[&](void* p,void const* address,std::size_t bytes){return memory.read(p,address,bytes);};
    s::Snapshot scope{};check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"fixture source capture");
    asset::Workspace work{};asset::Snapshot output{};output.actor_count=55;
    const auto saved=output;unsigned guards{};memory.calls=0;
    check(asset::observe(base,size,scope,read,AssetGuard{&guards},work,output)==asset::Code::observed,"whole declared Bodies inventory");
    const auto reads=memory.calls,guard_count=guards;
    check(output.body_count==2&&output.actor_count==1&&output.actors[0]==scope.actor&&
        output.bodies[0].presence==asset::Presence::body_data_field_null&&output.bodies[1].presence==asset::Presence::actor_observed,"null field retained separately from root actor");
    check(output.bodies[2].presence==asset::Presence::not_sampled&&!output.bodies[2].body&&!output.actors[1],"unsampled tails clear");
    for(unsigned n=1;n<=reads;++n){output=saved;memory.calls=0;memory.fail_at=n;
        check(asset::observe(base,size,scope,read,AssetGuard{},work,output)!=asset::Code::observed&&asset::same(output,saved),"every read failure preserves whole output");}
    memory.fail_at=0;
    for(unsigned n=1;n<=guard_count;++n){output=saved;guards=0;
        check(asset::observe(base,size,scope,read,AssetGuard{&guards,n},work,output)==asset::Code::guard&&asset::same(output,saved),"every owner boundary rejects without publish");}
    // A null non-root field may reference the root actor or a distinct actor.
    memory.put<std::uintptr_t>(0x140088,scope.actor);
    check(asset::observe(base,size,scope,read,AssetGuard{},work,output)==asset::Code::observed&&output.actor_count==1&&
        output.bodies[0].actor==output.bodies[1].actor,"shared actor deduplicates only actor list, not Body rows");
    memory.put<std::uintptr_t>(0x140088,extra_actor);
    check(asset::observe(base,size,scope,read,AssetGuard{},work,output)==asset::Code::observed&&output.actor_count==2&&
        output.actors[0]==extra_actor&&output.actors[1]==scope.actor,"distinct actor preserves first occurrence order");
    const auto two=memory;
    for(unsigned bad=0;bad<5;++bad){memory=two;output=saved;
        if(bad==0)memory.put<std::uintptr_t>(extra_actor,base+0x2330b0);
        if(bad==1)memory.put<std::uintptr_t>(extra_actor+0x38,0x180000);
        if(bad==2)memory.put<std::uintptr_t>(extra_actor+0x40,0);
        if(bad==3)memory.put<std::uintptr_t>(extra_internal+0x10,scope.actor);
        if(bad==4)memory.put<std::uintptr_t>(0x140088,extra_actor+1);
        const auto code=asset::observe(base,size,scope,read,AssetGuard{},work,output);
        check(code!=asset::Code::observed&&asset::same(output,saved),"non-root unsupported scene/internal/actor mismatch rejects");}
    memory=two;output=saved;unsigned typed_calls{};
    struct ChangingGuard {
        Memory& memory;unsigned& calls;
        bool operator()(){return true;}
        bool operator()(asset::Snapshot const&){if(++calls==1)memory.put<std::uintptr_t>(0x140088,0x160000);return true;}
    } changing{memory,typed_calls};
    check(asset::observe(base,size,scope,read,changing,work,output)==asset::Code::changed&&asset::same(output,saved),"two individually valid inventories cannot close across non-root change");
    memory=two;auto stale=scope;stale.members[0]+=8;
    check(asset::observe(base,size,stale,read,AssetGuard{},work,output)==asset::Code::source_changed&&asset::same(output,saved),"stale member list rejects");
    auto changed=scope;
    struct ScopeGuard {
        s::Snapshot& scope;unsigned typed{};
        bool operator()(){return true;}
        bool operator()(asset::Snapshot const&){if(++typed==2)scope.actor+=8;return true;}
    } mutate{changed};
    check(asset::observe(base,size,changed,read,mutate,work,output)==asset::Code::source_changed&&asset::same(output,saved),"caller input closes after last typed boundary");
    check(asset::observe(base,size,scope,read,AssetGuard{},work,work.first)==asset::Code::request,"workspace output alias rejected");
    check(asset::observe(base,size,output.query_source,read,AssetGuard{},work,output)==asset::Code::request,"input output alias rejected");
    check(asset::observe(base+1,size,scope,read,AssetGuard{},work,output)==asset::Code::request,"unaligned module rejected");
    check(asset::observe(base,1,scope,read,AssetGuard{},work,output)==asset::Code::request,"module span rejected");
    asset::Code wrong_thread{};std::thread thread([&](){wrong_thread=asset::observe(base,size,scope,read,AssetGuard{},work,output);});thread.join();
    check(wrong_thread==asset::Code::request&&asset::same(output,saved),"wrong thread rejects before reads");
    check(asset::observe(base,size,scope,[](void*,void const*,std::size_t)->bool{throw 7;},AssetGuard{},work,output)==asset::Code::read&&asset::same(output,saved),"capture exception rejects without publish");
    // Full64 membership with the root last and shared actor references. Only
    // BodyData is readable: no BoneSpring bytes have been allocated.
    memory=asset_fixture();memory.block(0x130000,64*8);
    for(unsigned n=0;n<64;++n){const auto body=0x200000+std::uintptr_t(n)*0x100;
        memory.block(body,0x90);memory.put<std::uintptr_t>(0x130000+n*8,body);
        memory.put<std::uintptr_t>(body+0x88,n%3?scope.actor:extra_actor);}
    auto full=source();full.body_index=63;memory.put<std::uintptr_t>(0x200000+63*0x100+0x88,scope.actor);
    memory.put(0x12006c,s::pose::ArrayHeader{0x130000,64,256});
    check(s::capture(base,size,full,read,[]{return true;},scope)==s::Code::current,"full64 source membership");
    check(asset::observe(base,size,scope,read,AssetGuard{},work,output)==asset::Code::observed&&output.body_count==64&&output.actor_count==2&&
        output.bodies[63].actor==scope.actor,"all64 rows and final root are observed with aliases");
    memory.put(0x12006c,s::pose::ArrayHeader{0x130000,65,256});
    const auto last=output;
    check(asset::observe(base,size,scope,read,AssetGuard{},work,output)==asset::Code::source_changed&&asset::same(output,last),"growth beyond bound is not truncated success");
    check(!GetModuleHandleW(L"PhysXCore.dll"),"original DLL not loaded");
    printf("CoverPhysXAssetActors checks=%u failed=%u synthetic Bodies/links only; no complete self set or original execution\n",checks,failed);
    return failed?1:0;
}
