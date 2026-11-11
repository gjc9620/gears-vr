// Reuse the existing actual-PE/data and current-source fixture, without
// executing its earlier test main or loading/calling the original DLL.
#include "cover_physx_shape_owner.h"
#define main PriorPhysXQueryFixtureMain
#include "cover_physx_query_test.cpp"
#undef main
namespace owner=gears_cover_physx_shape_owner;
constexpr std::uintptr_t shape_at=0x190000,internal_shape_at=0x1a0000,internal_actor_at=0x1b0000,
    other_actor_at=0x1c0000,other_internal_actor_at=0x1d0000;
static void links(Memory& m,std::uintptr_t actor,std::uintptr_t internal){
    m.put<std::uintptr_t>(shape_at+0x38,actor);m.put<std::uintptr_t>(internal_shape_at+0xd0,internal);
}
static Memory ownership_fixture(char const* path){
    auto m=fixture();if(!load_code(path,m))throw 3;
    m.block(shape_at,0x40);m.block(internal_shape_at,0x170);m.block(internal_actor_at,0x18);
    m.block(0x160000,0x50);m.block(other_actor_at,0x50);m.block(other_internal_actor_at,0x18);
    m.put<std::uintptr_t>(shape_at,base+owner::shape_tables[0]);
    m.put<std::uintptr_t>(shape_at+0x30,internal_shape_at);
    m.put<std::uintptr_t>(internal_shape_at,base+0x226c80);m.put<std::uintptr_t>(internal_shape_at+0xa0,shape_at);
    m.put<std::uintptr_t>(0x160040,internal_actor_at);m.put<std::uintptr_t>(internal_actor_at+0x10,0x160000);
    m.put<std::uintptr_t>(other_actor_at,base+0x2330a8);m.actor_owner(other_actor_at,base);m.put<std::uintptr_t>(other_actor_at+0x38,0x170000);
    m.put<std::uintptr_t>(other_actor_at+0x40,other_internal_actor_at);m.put<std::uintptr_t>(other_internal_actor_at+0x10,other_actor_at);
    m.put<std::uintptr_t>(0x160048,internal_shape_at);m.put<std::uintptr_t>(other_actor_at+0x48,internal_shape_at);links(m,0x160000,internal_actor_at);return m;
}
#ifndef GEARS_SHAPE_OWNER_TEST_MAIN
#define GEARS_SHAPE_OWNER_TEST_MAIN main
#endif
int GEARS_SHAPE_OWNER_TEST_MAIN(int argc,char** argv){
    if(argc!=2)return 2;
    try{
        auto initial=ownership_fixture(argv[1]);auto m=initial;
        auto read=[&](void* p,void const* a,std::size_t n){return m.read(p,a,n);};
        s::Snapshot scope{};auto input=source();check(s::capture(base,size,input,read,[]{return true;},scope)==s::Code::current,"current fixture source");
        q::Hit hit{};memcpy(hit.bytes.data()+8,&shape_at,sizeof shape_at);
        owner::Observation output{},sentinel{};sentinel.actor=0xdeadbeef;unsigned guards{};
        auto guard=[&](s::Snapshot const& key){++guards;return s::same(key,scope);};
        m.calls=0;check(owner::observe(base,size,scope,hit,read,guard,output)==owner::Code::observed,"PE-backed ownership bracket");
        const auto reads=m.calls,guard_calls=guards;
        check(output.shape==shape_at&&output.actor==scope.actor&&output.scene==scope.scene&&output.kind==owner::Kind::box,"observed identities");
        check(output.internal_shape==internal_shape_at&&output.internal_actor==internal_actor_at,"bidirectional internal links");
        // The eight PUBLIC class tables are real original bytes. Internal box
        // links here are synthetic relation data, not eight geometry proofs.
        for(unsigned k=0;k<owner::shape_tables.size();++k){
            m=initial;m.put<std::uintptr_t>(shape_at,base+owner::shape_tables[k]);output=sentinel;
            check(owner::observe(base,size,scope,hit,read,guard,output)==owner::Code::observed&&unsigned(output.kind)==k,"all eight actual primary getter tables");
        }
        m=initial;links(m,other_actor_at,other_internal_actor_at);
        check(owner::observe(base,size,scope,hit,read,guard,output)==owner::Code::observed&&output.actor==other_actor_at,"other same-scene actor is observed, not classified away");
        m=initial;links(m,other_actor_at,0);m.put<std::uintptr_t>(other_actor_at+0x40,0);
        check(owner::observe(base,size,scope,hit,read,guard,output)==owner::Code::observed&&!output.internal_actor&&output.actor==other_actor_at,"static foreign Actor qualified by actual single Shape membership");
        for(unsigned bad=0;bad<15;++bad){
            m=initial;output=sentinel;
            if(bad==0)m.put<std::uintptr_t>(shape_at,base+0x241358);
            if(bad==1)m.put<std::uintptr_t>(base+owner::shape_tables[0]+8,base+0x143030);
            if(bad==2)m.put<unsigned char>(base+0x13e734,0x90);
            if(bad==3)m.put<unsigned char>(base+0xc4e7,0x90);
            if(bad==4)m.put<std::uintptr_t>(shape_at+0x38,0);
            if(bad==5)m.put<std::uintptr_t>(0x160000,base+0x2330b0);
            if(bad==6)m.put<std::uintptr_t>(0x160038,0x170008);
            if(bad==7)m.put<std::uintptr_t>(shape_at+0x30,0);
            if(bad==8)m.put<std::uintptr_t>(internal_shape_at,base+size-8);
            if(bad==9)m.put<std::uintptr_t>(base+0x226cc0,base+0xc4e8);
            if(bad==10)m.put<std::uintptr_t>(internal_shape_at+0xa0,shape_at+8);
            if(bad==11)m.put<std::uintptr_t>(internal_shape_at+0xd0,other_internal_actor_at);
            if(bad==12)m.put<std::uintptr_t>(0x160030,other_actor_at);
            if(bad==13)m.put<std::uintptr_t>(0x160040,0);
            if(bad==14)m.put<std::uintptr_t>(0x150088,other_actor_at);
            check(owner::observe(base,size,scope,hit,read,guard,output)!=owner::Code::observed,"reject unsupported/code/links/source changes");
            check(owner::same(output,sentinel),"failure leaves output untouched");
        }
        for(unsigned failure=1;failure<=reads;++failure){m=initial;m.calls=0;m.fail_at=failure;output=sentinel;
            check(owner::observe(base,size,scope,hit,read,guard,output)!=owner::Code::observed,"every memory-read failure rejected");check(owner::same(output,sentinel),"read failure preserves output");}
        for(unsigned failure=1;failure<=guard_calls;++failure){m=initial;output=sentinel;unsigned calls{};
            check(owner::observe(base,size,scope,hit,read,[&](s::Snapshot const&){return ++calls!=failure;},output)!=owner::Code::observed,"every owner guard failure rejected");check(owner::same(output,sentinel),"guard failure preserves output");}
        m=initial;output=sentinel;bool switched=false;
        auto swapping=[&](void* p,void const* a,std::size_t n){const bool result=m.read(p,a,n);
            if(result&&!switched&&reinterpret_cast<std::uintptr_t>(a)==0x160030){switched=true;links(m,other_actor_at,other_internal_actor_at);}return result;};
        check(owner::observe(base,size,scope,hit,swapping,guard,output)!=owner::Code::observed&&owner::same(output,sentinel),"two individually consistent owners cannot replace the first");
        m=initial;output=sentinel;unsigned code_reads{};
        auto packet_change=[&](void* p,void const* a,std::size_t n){const bool result=m.read(p,a,n);
            if(reinterpret_cast<std::uintptr_t>(a)==base+0x13e730&&++code_reads==2)hit.bytes[0]^=1;return result;};
        check(owner::observe(base,size,scope,hit,packet_change,guard,output)==owner::Code::source_changed&&owner::same(output,sentinel),"late raw hit mutation rejected");
        hit.bytes[0]^=1;m=initial;output=sentinel;unsigned last_guard{};const auto saved=scope;
        check(owner::observe(base,size,scope,hit,read,[&](s::Snapshot const&){if(++last_guard==guard_calls)scope.actor^=8;return true;},output)==owner::Code::source_changed&&owner::same(output,sentinel),"final guard cannot mutate caller source after closing");scope=saved;
        for(auto pointer:std::array<std::uintptr_t,3>{{0,shape_at+1,UINTPTR_MAX-7}}){m=initial;m.calls=0;output=sentinel;auto bad=hit;memcpy(bad.bytes.data()+8,&pointer,sizeof pointer);
            check(owner::observe(base,size,scope,bad,read,guard,output)==owner::Code::request&&!m.calls&&owner::same(output,sentinel),"invalid hit shape rejected before reads");}
        check(!GetModuleHandleW(L"PhysXCore.dll"),"original DLL remained unloaded");
        printf("read_budget=%u guard_calls=%u\n",reads,guard_calls);
    }catch(...){check(false,"ownership fixture exception");}
    printf("CoverPhysXShapeOwner checks=%u failed=%u original-executed=false game-started=false\n",checks,failed);return failed?1:0;
}
