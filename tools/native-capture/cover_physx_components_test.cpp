#include "cover_physx_query.h"
#define main PriorComponentsQueryFixtureMain
#include "cover_physx_query_test.cpp"
#undef main
#include <memory>
#include <thread>
namespace p=gears_cover_physx_components;
static Memory parts_fixture(p::Source& fields){
    auto m=fixture();m.block(0x160000,0x48);m.put<std::uintptr_t>(0x160000,base+0x2330a8);m.actor_owner(0x160000,base);m.put<std::uintptr_t>(0x160038,0x170000);
    m.block(0x190000,0x188);m.block(0x1a0000,0x98);m.block(0x1b0000,0x320);m.block(0x1c0000,0x18);
    m.block(0x1d0000,88);m.block(0x1e0000,88);m.block(0x280000,32);m.block(0x290000,32);
    m.put<std::uintptr_t>(0x190180,0x1a0000);m.put<std::uintptr_t>(0x1a0088,0x160000);m.put<std::uintptr_t>(0x1a0090,0x220000);
    m.put<std::uintptr_t>(0x160040,0x1c0000);m.put<std::uintptr_t>(0x1c0010,0x160000);
    m.block(0x1f0000,0x48);m.block(0x210000,0x18);m.put<std::uintptr_t>(0x1f0000,base+0x2330a8);m.actor_owner(0x1f0000,base);
    m.put<std::uintptr_t>(0x1f0038,0x170000);m.put<std::uintptr_t>(0x1f0040,0x210000);m.put<std::uintptr_t>(0x210010,0x1f0000);
    fields={};fields.seed_count=fields.root_count=1;fields.component_count=2;auto& root=fields.roots[0];root.actor=0x1b0000;root.offsets={0x300,0x310};
    root.base_offset=0x2e0;root.attached_offset=0x2f0;
    root.lists={s::pose::ArrayHeader{0x280000,4,4},s::pose::ArrayHeader{0x290000,2,4}};
    root.members[0]={0x190000,0,0x1d0000,0x190000};root.members[1]={0x1d0000,0x190000};
    for(unsigned n=0;n<2;++n){m.put(root.actor+root.offsets[n],root.lists[n]);for(unsigned k=0;k<4;++k)m.put(root.lists[n].data+k*8,root.members[n][k]);}
    fields.components[0]={0x190000,root.actor,0x1a0000,0x190000,root.actor,1,1,0x180,true,false,p::State::body_data_nonnull,-1,0x160000,0x220000};
    fields.components[1]={0x1d0000,0x1e0000,0,0,0,0,0,0,false,false,p::State::nonprimitive};return m;
}
struct PartsGuard {
    p::Source const& fields;unsigned* count{};unsigned fail_at{};
    bool operator()(){return !count||++*count!=fail_at;}
    bool operator()(p::Snapshot const& value){return (*this)()&&p::same(fields,value.fields)&&p::consistent(value);}
};
#ifndef GEARS_COMPONENTS_PHYSX_TEST_MAIN
#define GEARS_COMPONENTS_PHYSX_TEST_MAIN main
#endif
int GEARS_COMPONENTS_PHYSX_TEST_MAIN(){
    auto fields=std::make_unique<p::Source>();auto memory=parts_fixture(*fields);
    auto read=[&](void* dst,void const* at,std::size_t bytes){return memory.read(dst,at,bytes);};s::Snapshot scope{};
    check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"query source");
    auto work=std::make_unique<p::Workspace>();auto output=std::make_unique<p::Snapshot>();output->actor_count=99;
    auto saved=std::make_unique<p::Snapshot>(*output);unsigned guards{};memory.calls=0;
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields,&guards},*work,*output)==p::Code::observed,"declared component sources qualified together");
    const auto reads=memory.calls,guard_count=guards;
    check(output->actor_count==1&&output->actors[0]==0x160000&&output->observations[1].presence==p::Presence::nonprimitive&&
        !output->fields.components[1].owner_roots&&output->fields.roots[0].members[0][1]==0,"preserves duplicate/null membership and external Owner without self filtering");
    for(unsigned n=1;n<=reads;++n){*output=*saved;memory.calls=0;memory.fail_at=n;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)!=p::Code::observed&&p::same(*output,*saved),"each read failure preserves whole catalog");}
    memory.fail_at=0;
    for(unsigned n=1;n<=guard_count;++n){*output=*saved;guards=0;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields,&guards,n},*work,*output)==p::Code::guard&&p::same(*output,*saved),"each owner boundary rejects without publish");}
    auto original=std::make_unique<p::Source>(*fields);const auto initial=memory;
    fields->components[0].body_data=0x1f0000;memory.put<std::uintptr_t>(0x1a0088,0x1f0000);
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&output->actors[0]==0x1f0000,"distinct same-Scene actor, no root-actor assumption");
    for(unsigned bad=0;bad<5;++bad){memory=initial;*fields=*original;*output=*saved;
        fields->components[0].body_data=0x1f0000;memory.put<std::uintptr_t>(0x1a0088,0x1f0000);
        if(bad==0)memory.put<std::uintptr_t>(0x1f0000,base+0x2330b0);
        if(bad==1)memory.put<std::uintptr_t>(0x1f0038,0x180000);
        if(bad==2)memory.put<std::uintptr_t>(0x1f0030,0x160000);
        if(bad==3)memory.put<std::uintptr_t>(0x280000,0);
        if(bad==4)memory.put<std::uintptr_t>(0x1a0090,0);
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)!=p::Code::observed&&p::same(*output,*saved),"unsupported/Scene/reverse/list/body change rejects");}
    memory=initial;*fields=*original;
    struct Change {Memory& m;p::Source const& fields;unsigned typed{};bool operator()(){return true;}
        bool operator()(p::Snapshot const& value){if(++typed==1)m.put<std::uintptr_t>(0x290000,0);return p::same(fields,value.fields);}} change{memory,*fields};
    check(p::observe(base,size,scope,*fields,read,change,*work,*output)==p::Code::changed&&p::same(*output,*saved),"array mutation between qualified passes");
    memory=initial;
    struct Mutate {p::Source& fields;unsigned typed{};bool operator()(){return true;}
        bool operator()(p::Snapshot const&){if(++typed==2)fields.components[0].body_index++;return true;}} mutate{*fields};
    check(p::observe(base,size,scope,*fields,read,mutate,*work,*output)==p::Code::source_changed&&p::same(*output,*saved),"caller input closes after last guard");*fields=*original;
    struct Reenter {Memory& m;p::Source const& fields;s::Snapshot const& scope;p::Workspace& work;p::Code nested{};unsigned typed{};
        bool operator()(){return true;}bool operator()(p::Snapshot const&){if(++typed==1){auto out=std::make_unique<p::Snapshot>();
            nested=p::observe(base,size,scope,fields,[&](void* d,void const* a,std::size_t n){return m.read(d,a,n);},PartsGuard{fields},work,*out);}return true;}};
    Reenter reentry{memory,*fields,scope,*work}; // Use a shared result holder because the guard itself is copied.
    p::Code nested{};
    struct SharedReenter:Reenter {p::Code& result;SharedReenter(Reenter const& value,p::Code& out):Reenter(value),result(out){}
        using Reenter::operator();bool operator()(p::Snapshot const& value){const bool okay=Reenter::operator()(value);result=nested;return okay;}} shared{reentry,nested};
    check(p::observe(base,size,scope,*fields,read,shared,*work,*output)==p::Code::observed&&nested==p::Code::request,"same workspace reentry cannot overwrite key/candidates");
    for(unsigned bad=0;bad<9;++bad){*fields=*original;*output=*saved;
        if(bad==0)fields->components[0].owner_roots=0;
        if(bad==1)fields->components[1].body_data=0x160000;
        if(bad==2)fields->roots[0].members[0][4]=0x190000;
        if(bad==3)fields->component_count=1;
        if(bad==4)fields->roots[0].offsets[1]=0x300;
        if(bad==5)fields->components[2].component=0x190000;
        if(bad==6)fields->roots[1].actor=0x1b0000;
        if(bad==7)fields->components[0].body_data++;
        if(bad==8)fields->pawn_mesh_listed=true;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::request&&p::same(*output,*saved),"malformed or hidden partial source rejects");}
    *fields=*original;memory=initial;
    // Three sampled states, followed by an empty catalog clearing all old rows.
    auto& row=fields->components[0];row.state=p::State::body_instance_field_null;row.body=row.body_owner=row.body_owner_actor=row.body_data=row.bone_spring=0;row.body_owner_roots=0;row.body_index=0;
    memory.put<std::uintptr_t>(0x190180,0);
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&output->observations[0].presence==p::Presence::body_instance_field_null&&!output->actor_count,"null BodyInstance remains explicit");
    *fields=*original;memory=initial;fields->components[0].state=p::State::body_data_field_null;fields->components[0].body_data=0;memory.put<std::uintptr_t>(0x1a0088,0);
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&output->observations[0].presence==p::Presence::body_data_field_null&&!output->actor_count,"null BodyData remains distinct and opaque spring retained");
    *fields=*original;memory=initial;fields->component_count=0;fields->components={};
    for(unsigned l=0;l<2;++l){fields->roots[0].lists[l]={};fields->roots[0].members[l]={};memory.put(fields->roots[0].actor+fields->roots[0].offsets[l],fields->roots[0].lists[l]);}
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&!output->actor_count&&!output->actors[0]&&output->observations[0].presence==p::Presence::not_sampled,"empty declaration clears prior actor rows, not occupancy proof");
    *fields=*original;memory=initial;fields->component_count=128;
    for(unsigned l=0;l<2;++l){fields->roots[0].lists[l]={0x500000+std::uintptr_t(l)*0x10000,64,256};memory.block(fields->roots[0].lists[l].data,512);memory.put(fields->roots[0].actor+fields->roots[0].offsets[l],fields->roots[0].lists[l]);}
    for(unsigned n=0;n<128;++n){const auto comp=0x300000+std::uintptr_t(n)*0x200,body=0x400000+std::uintptr_t(n)*0x100;
        memory.block(comp,0x188);memory.block(body,0x98);memory.put(comp+0x180,body);memory.put<std::uintptr_t>(body+0x88,0x160000);
        fields->roots[0].members[n/64][n%64]=comp;memory.put(fields->roots[0].lists[n/64].data+(n%64)*8,comp);
        fields->components[n]={comp,0x1b0000,body,comp,0x1b0000,1,1,0x180,true,false,p::State::body_data_nonnull,int(n),0x160000,0};}
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&output->actor_count==1&&output->observations[127].presence==p::Presence::actor_observed,"all128 component bodies and final row, shared actors dedup only in side list");
    const auto full=std::make_unique<p::Snapshot>(*output);p::Code wrong{};std::thread t([&](){wrong=p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output);});t.join();
    check(wrong==p::Code::request&&p::same(*output,*full),"wrong thread");
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,work->first)==p::Code::request,"workspace output alias");
    check(!GetModuleHandleW(L"PhysXCore.dll"),"original DLL not loaded");
    printf("CoverPhysXComponents checks=%u failed=%u synthetic fields/Scene only; no complete self set, filter or original execution\n",checks,failed);return failed?1:0;
}
