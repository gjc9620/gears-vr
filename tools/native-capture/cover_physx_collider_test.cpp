#include "cover_physx_query.h"
#define main PriorColliderQueryFixtureMain
#include "cover_physx_query_test.cpp"
#undef main
#include <thread>
namespace cx=gears_cover_physx_collider;
static Memory component_fixture(){
    auto memory=fixture();memory.block(0x160000,0x48);memory.put<std::uintptr_t>(0x160000,base+0x2330a8);memory.put<std::uintptr_t>(0x160038,0x170000);
    memory.block(0x190000,0x188);memory.block(0x1a0000,0x98);memory.block(0x1c0000,0x18);
    memory.put<std::uintptr_t>(0x190180,0x1a0000);memory.put<std::uintptr_t>(0x1a0088,0x160000);memory.put<std::uintptr_t>(0x1a0090,0x1f0000);
    memory.put<std::uintptr_t>(0x160040,0x1c0000);memory.put<std::uintptr_t>(0x1c0010,0x160000);
    memory.block(0x1d0000,0x48);memory.block(0x1e0000,0x18);
    memory.put<std::uintptr_t>(0x1d0000,base+0x2330a8);memory.put<std::uintptr_t>(0x1d0038,0x170000);
    memory.put<std::uintptr_t>(0x1d0040,0x1e0000);memory.put<std::uintptr_t>(0x1e0010,0x1d0000);
    return memory;
}
struct ComponentGuard {
    cx::Source const& fields;unsigned* calls{};unsigned fail_at{};
    bool operator()(){return !calls||++*calls!=fail_at;}
    bool operator()(cx::Snapshot const& value){return (*this)()&&cx::same(fields,value.fields);}
};
int main(){
    auto memory=component_fixture();auto read=[&](void* p,void const* a,std::size_t n){return memory.read(p,a,n);};
    s::Snapshot scope{};check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"current query scope");
    cx::Source fields{0x1b0000,0x190000,0x1a0000,0x180,-1,0x160000,0x1f0000};
    cx::Snapshot output{};output.fields.body_data=99;const auto saved=output;unsigned guards{};memory.calls=0;
    check(cx::observe(base,size,scope,fields,read,ComponentGuard{fields,&guards},output)==cx::Code::observed,"component BodyData qualified on same original Scene");
    const auto reads=memory.calls,guard_count=guards;
    check(output.presence==cx::Presence::actor_observed&&output.fields.body_index==-1&&output.actor_table==base+0x2330a8&&
        output.scene==scope.scene&&output.internal==0x1c0000&&output.fields.bone_spring==0x1f0000,"keeps signed index and opaque BoneSpring; shared PAA actor allowed");
    for(unsigned n=1;n<=reads;++n){output=saved;memory.calls=0;memory.fail_at=n;
        check(cx::observe(base,size,scope,fields,read,ComponentGuard{fields},output)!=cx::Code::observed&&cx::same(output,saved),"every read failure preserves packet");}
    memory.fail_at=0;
    for(unsigned n=1;n<=guard_count;++n){output=saved;guards=0;
        check(cx::observe(base,size,scope,fields,read,ComponentGuard{fields,&guards,n},output)==cx::Code::guard&&cx::same(output,saved),"each owner boundary can reject");}
    auto empty=fields;empty.body=empty.body_data=empty.bone_spring=0;empty.body_index=0;memory.put<std::uintptr_t>(0x190180,0);
    check(cx::observe(base,size,scope,empty,read,ComponentGuard{empty},output)==cx::Code::observed&&
        output.presence==cx::Presence::body_instance_field_null&&!output.actor_table&&!output.internal,"null BodyInstance not a no-physics certificate");
    memory=component_fixture();empty=fields;empty.body_data=0;memory.put<std::uintptr_t>(0x1a0088,0);
    check(cx::observe(base,size,scope,empty,read,ComponentGuard{empty},output)==cx::Code::observed&&
        output.presence==cx::Presence::body_data_field_null&&output.fields.bone_spring==0x1f0000&&!output.actor_table,"null BodyData differs from absent BodyInstance; BoneSpring not interpreted");
    auto foreign=fields;foreign.body_data=0x1d0000;memory=component_fixture();memory.put<std::uintptr_t>(0x1a0088,foreign.body_data);
    check(cx::observe(base,size,scope,foreign,read,ComponentGuard{foreign},output)==cx::Code::observed&&output.internal==0x1e0000,"distinct same-Scene actor accepted without assuming root identity");
    for(unsigned bad=0;bad<7;++bad){memory=component_fixture();output=saved;
        if(bad==0)memory.put<std::uintptr_t>(0x190180,0);
        if(bad==1)memory.put<std::uintptr_t>(0x1a0088,0x1d0000);
        if(bad==2)memory.put<std::uintptr_t>(0x160000,base+0x2330b0);
        if(bad==3)memory.put<std::uintptr_t>(0x160038,0x180000);
        if(bad==4)memory.put<std::uintptr_t>(0x160040,0);
        if(bad==5)memory.put<std::uintptr_t>(0x1c0010,0x1d0000);
        if(bad==6)memory.put<std::uintptr_t>(0x1a0090,0);
        check(cx::observe(base,size,scope,fields,read,ComponentGuard{fields},output)!=cx::Code::observed&&cx::same(output,saved),"changed component/body/table/Scene/internal relation refuses whole output");}
    memory=component_fixture();memory.put<std::uintptr_t>(0x1e0010,0x160000);
    struct Change {Memory& m;cx::Source const& fields;unsigned typed{};bool operator()(){return true;}
        bool operator()(cx::Snapshot const& v){if(++typed==1)m.put<std::uintptr_t>(0x160040,0x1e0000);return cx::same(fields,v.fields);}} change{memory,fields};
    check(cx::observe(base,size,scope,fields,read,change,output)==cx::Code::changed&&cx::same(output,saved),"two valid internal actors cannot span one observation");
    memory=component_fixture();auto mutable_fields=fields;
    struct Mutate {cx::Source& fields;unsigned typed{};bool operator()(){return true;}
        bool operator()(cx::Snapshot const&){if(++typed==2)fields.body_instance_offset+=8;return true;}} mutate{mutable_fields};
    check(cx::observe(base,size,scope,mutable_fields,read,mutate,output)==cx::Code::source_changed&&cx::same(output,saved),"input closes after final typed guard");
    check(cx::observe(base,size,scope,output.fields,read,ComponentGuard{output.fields},output)==cx::Code::request,"output/source alias");
    for(unsigned bad=0;bad<5;++bad){auto invalid=fields;if(bad==0)invalid.component+=1;if(bad==1)invalid.body_instance_offset=0;
        if(bad==2)invalid.body_instance_offset=65536;if(bad==3)invalid.body=0;if(bad==4)invalid.body_data+=1;
        check(cx::observe(base,size,scope,invalid,read,ComponentGuard{invalid},output)==cx::Code::request&&cx::same(output,saved),"bad input rejected before pointer reads");}
    cx::Code other{};std::thread thread([&](){other=cx::observe(base,size,scope,fields,read,ComponentGuard{fields},output);});thread.join();
    check(other==cx::Code::request&&cx::same(output,saved),"wrong thread rejected");
    check(cx::observe(base,size,scope,fields,[](void*,void const*,std::size_t)->bool{throw 7;},ComponentGuard{fields},output)==cx::Code::read&&cx::same(output,saved),"read exception preserves packet");
    check(!GetModuleHandleW(L"PhysXCore.dll"),"original DLL not loaded");
    printf("CoverPhysXCollider checks=%u failed=%u synthetic source/actor links only; no complete self set or original execution\n",checks,failed);return failed?1:0;
}
