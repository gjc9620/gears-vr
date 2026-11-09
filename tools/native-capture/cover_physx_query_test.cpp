#include "cover_physx_query.h"
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>

// Offline source/ABI test. The original DLL is read as data, never loaded or
// executed. Synthetic functions below test only this adapter's x64 arguments.
namespace q=gears_cover_physx_query;
namespace s=gears_cover_physx_query_source;
namespace c=gears_cover_physx_query_code;
static unsigned checks{},failed{};
static void check(bool value,char const* name){++checks;if(!value){++failed;std::printf("FAIL %s\n",name);}}
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
    unsigned calls{},fail_at{};
    void block(std::uintptr_t at,std::size_t n){blocks[at].resize(n);}
    template<class T> void put(std::uintptr_t at,T const& value){
        auto i=blocks.upper_bound(at);if(i==blocks.begin())throw 1;--i;
        const auto offset=at-i->first;
        if(offset>i->second.size()||sizeof value>i->second.size()-offset)throw 2;
        std::memcpy(i->second.data()+offset,&value,sizeof value);
    }
    bool read(void* out,void const* address,std::size_t n){
        if(++calls==fail_at)return false;
        const auto at=reinterpret_cast<std::uintptr_t>(address);
        auto i=blocks.upper_bound(at);if(i==blocks.begin())return false;--i;
        const auto offset=at-i->first;
        if(offset>i->second.size()||n>i->second.size()-offset)return false;
        std::memcpy(out,i->second.data()+offset,n);return true;
    }
};
constexpr std::uintptr_t base=0x180000000;
constexpr std::uint32_t size=0x300000;
static s::Source source(){return {0x100000,0x110000,0x120000,1,GetCurrentThreadId()};}
static Memory fixture(){
    Memory m;
    for(auto const& item:std::array<std::pair<std::uintptr_t,std::size_t>,10>{{
        {0x100000,0x258},{0x110000,8},{0x120000,0x80},{0x130000,16},
        {0x140000,0x90},{0x150000,0x90},{0x160000,0x40},{0x170000,0x278},
        {0x180000,0x6b8},{base, size}}})m.block(item.first,item.second);
    m.put<std::uintptr_t>(0x100248,0x110000);m.put<std::uintptr_t>(0x100250,0x120000);
    m.put(0x12006c,s::pose::ArrayHeader{0x130000,2,2});
    m.put<std::uintptr_t>(0x130000,0x140000);m.put<std::uintptr_t>(0x130008,0x150000);
    m.put<std::uintptr_t>(0x150088,0x160000);m.put<std::uintptr_t>(0x160000,base+0x2330a8);
    m.put<std::uintptr_t>(0x160038,0x170000);m.put<std::uintptr_t>(0x170000,base+0x22ccb8);
    m.put<std::uintptr_t>(0x170270,0x180000);m.put<std::uintptr_t>(0x180000,base+0x23ca98);
    m.put<std::uintptr_t>(0x1806b0,0x170000);
    constexpr std::array<std::array<unsigned,3>,10> slots{{
        {{0x2330a8,8,0x135b00}},{{0x22ccb8,8,0xa0300}},
        {{0x23ca98,0x308,0x171770}},{{0x23ca98,0x310,0x1716f0}},
        {{0x23ca98,0x330,0x168350}},{{0x23ca98,0x338,0x1682d0}},
        {{0x22ccb8,0x288,0xa6e30}},{{0x22ccb8,0x290,0xa6aa0}},
        {{0x22ccb8,0x268,0xa6290}},{{0x22ccb8,0x270,0xa60b0}}}};
    for(auto const& slot:slots)m.put<std::uintptr_t>(base+slot[0]+slot[1],base+slot[2]);
    return m;
}
static void source_tests(){
    auto m=fixture();auto input=source();s::Snapshot out{};
    auto read=[&](void* p,void const* a,std::size_t n){return m.read(p,a,n);};
    auto guard=[](){return true;};
    check(s::capture(base,size,input,read,guard,out)==s::Code::current,"source route");
    check(out.body==0x150000&&out.actor==0x160000&&out.scene==0x170000&&out.public_scene==0x180000,"selected body receiver");
    const auto count=m.calls;const s::Snapshot good=out;
    for(unsigned i=1;i<=count;++i){m.calls=0;m.fail_at=i;out=good;
        check(s::capture(base,size,input,read,guard,out)==s::Code::read&&s::same(out,good),"each failed read preserves output");}
    m.fail_at=0;m.calls=0;out=good;
    check(s::capture(base,size,input,read,[](){return false;},out)==s::Code::guard&&s::same(out,good),"entry guard");
    unsigned guards{};
    check(s::capture(base,size,input,read,[&](){return ++guards<2;},out)==s::Code::guard&&s::same(out,good),"closing guard");
    auto reject=[&](Memory changed){auto r=[&](void* p,void const* a,std::size_t n){return changed.read(p,a,n);};out=good;
        check(s::capture(base,size,input,r,guard,out)!=s::Code::current&&s::same(out,good),"changed association/layout rejected");};
    for(auto at:{0x100248ull,0x100250ull,0x150088ull,0x160000ull,0x160038ull,0x170000ull,0x170270ull,0x180000ull,0x1806b0ull}){
        auto changed=fixture();changed.put<std::uintptr_t>(at,0);reject(changed);}
    {auto changed=fixture();changed.put<std::uintptr_t>(0x130008,0x140000);reject(changed);}
    {auto changed=fixture();changed.put<std::uintptr_t>(0x130000,0x140001);reject(changed);}
    for(auto h:std::array<s::pose::ArrayHeader,6>{{{0x130000,0,2},{0x130000,-1,2},{0x130000,65,65},
        {0x130000,2,1},{0x130000,2,257},{UINTPTR_MAX-7,2,2}}}){auto changed=fixture();changed.put(0x12006c,h);reject(changed);}
    for(auto offset:{0x2330b0u,0x22ccc0u,0x23cda0u,0x23cda8u,0x23cdc8u,0x23cdd0u,0x22cf40u,0x22cf48u,0x22cf20u,0x22cf28u}){
        auto changed=fixture();changed.put<std::uintptr_t>(base+offset,base+0x1000);reject(changed);}
    m=fixture();bool altered=false;
    auto mutate=[&](void* p,void const* a,std::size_t n){const bool ok=m.read(p,a,n);if(!altered){input.body_index=0;altered=true;}return ok;};
    check(s::capture(base,size,input,mutate,guard,out)==s::Code::changed&&s::same(out,good),"callback cannot replace request");
    input=source();out.source=input;
    check(s::capture(base,size,out.source,read,guard,out)==s::Code::request,"request/output alias");
    check(s::capture(base+1,size,input,read,guard,out)==s::Code::request,"unaligned module");
    check(s::capture(base,0x23cdd0,input,read,guard,out)==s::Code::request,"module excludes last slot");
    input.body_index=2;check(s::capture(base,size,input,read,guard,out)==s::Code::layout,"selected index range");
}
static bool load_code(char const* path,Memory& memory){
    std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)return false;
    const auto n=file.tellg();if(n!=c::file_bytes)return false;
    std::vector<unsigned char> bytes(static_cast<std::size_t>(n));file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))return false;
    s::Digest hash{};if(!q::detail::digest(bytes.data(),bytes.size(),hash)||hash!=c::image_sha256)return false;
    auto u16=[&](std::size_t at){std::uint16_t v{};if(at+2>bytes.size())throw 1;std::memcpy(&v,bytes.data()+at,2);return v;};
    auto u32=[&](std::size_t at){std::uint32_t v{};if(at+4>bytes.size())throw 1;std::memcpy(&v,bytes.data()+at,4);return v;};
    const auto pe=u32(0x3c);if(u32(pe)!=0x4550||u16(pe+4)!=0x8664)return false;
    const auto table=pe+24+u16(pe+20);memory.block(base,size);
    for(unsigned i=0;i<u16(pe+6);++i){const auto at=table+i*40,rv=u32(at+12),raw=u32(at+16),off=u32(at+20);
        if(rv>size||raw>size-rv||off>bytes.size()||raw>bytes.size()-off)return false;
        std::memcpy(memory.blocks[base].data()+rv,bytes.data()+off,raw);}
    return true;
}
static void code_tests(char const* path){
    Memory m;check(load_code(path,m),"original disk identity and section bounds");if(m.blocks.empty())return;
    auto read=[&](void* p,void const* a,std::size_t n){return m.read(p,a,n);};
    check(s::code_current(base,size,read,q::detail::digest),"all complete original chunks hash");
    unsigned bytes{};
    for(auto const& chunk:c::chunks){bytes+=chunk.size;auto& last=m.blocks[base][chunk.rva+chunk.size-1];last^=1;
        check(!s::code_current(base,size,read,q::detail::digest),"last byte mutation including cold tail");last^=1;
        check(!s::code_current(base,chunk.rva+chunk.size-1,read,q::detail::digest),"truncated code image");}
    check(bytes==9522,"full ranges include complete all-hit worker, not short unwind fragments");
    for(unsigned i=1;i<=c::chunks.size();++i){m.calls=0;m.fail_at=i;check(!s::code_current(base,size,read,q::detail::digest),"each code read failure");}
    m.fail_at=0;
    check(!s::code_current(base,size,read,[](void const*,std::size_t,s::Digest&){return false;}),"hash failure");
}
static unsigned abi_calls{};static void const* expected_shape{};static q::Vector const* expected_delta{};
static bool __fastcall fake_overlap(std::uintptr_t scene,void const* shape,std::uint32_t flags,std::uint32_t groups,void const* mask){
    ++abi_calls;check(scene==0x180000&&shape==expected_shape&&flags==3&&groups==UINT32_MAX&&!mask,"overlap five args");return true;
}
static std::uint32_t __fastcall fake_sweep(std::uintptr_t scene,void const* shape,q::Vector const* delta,std::uint32_t flags,
    void* cache,std::uint32_t capacity,q::Hit* hit,void* callback,std::uint32_t groups,void const* mask){
    ++abi_calls;check(scene==0x180000&&shape==expected_shape&&delta==expected_delta&&flags==3&&!cache&&capacity==1&&hit&&!callback&&groups==UINT32_MAX&&!mask,"sweep ten args including stack");
    hit->bytes.fill(0xa5);return 1;
}
static bool __fastcall fake_fault(std::uintptr_t,void const*,std::uint32_t,std::uint32_t,void const*){
    RaiseException(0xe0000075,0,0,nullptr);return false;
}
static void query_tests(){
    s::Snapshot observed{};observed.public_scene=0x180000;
    auto legacy=[](){return true;};check(q::detail::source_guard(legacy,observed),"legacy no-argument guard");
    struct TypedGuard {unsigned plain{},typed{};bool operator()(){++plain;return true;}
        bool operator()(s::Snapshot const& value){++typed;return value.public_scene==0x180000;}} typed;
    check(q::detail::source_guard(typed,observed)&&typed.typed==1&&!typed.plain,"typed source guard sees actual receiver");
    observed.public_scene=0;check(!q::detail::source_guard(typed,observed)&&typed.typed==2&&!typed.plain,"typed source mismatch rejected");
    q::Box box{{1,2,3},{4,5,6},{1,0,0,0,1,0,0,0,1}};q::Capsule capsule{{1,2,3},{1,2,4},.25f};q::Vector delta{.1f,.2f,.3f};
    check(q::detail::valid(box)&&q::detail::valid(capsule)&&q::detail::motion(delta),"valid geometry");
    auto bad=box;bad.rotation[0]=-1;check(!q::detail::valid(bad),"reflected box rejected");
    bad=box;bad.rotation[1]=.1f;check(!q::detail::valid(bad),"sheared box rejected");
    bad=box;bad.half.x=0;check(!q::detail::valid(bad),"zero extent rejected");
    auto cap=capsule;cap.second=cap.first;check(!q::detail::valid(cap),"sphere not guessed capsule");
    check(!q::detail::motion({})&&!q::detail::motion({std::numeric_limits<float>::infinity(),0,0}),"zero or nonfinite sweep rejected");
    const auto maximum=(std::numeric_limits<float>::max)(),tiny=(std::numeric_limits<float>::min)();
    check(!q::detail::motion({maximum,0,0})&&!q::detail::motion({tiny,0,0}),"float normalization overflow/underflow rejected");
    check(!q::detail::motion({std::numeric_limits<float>::denorm_min(),0,0}),"DAZ dependent motion rejected");
    cap=capsule;cap.first.x=-maximum;cap.second.x=maximum;check(!q::detail::valid(cap),"capsule segment subtraction overflow rejected");
    bad=box;bad.centre.x=maximum;bad.half.x=maximum;check(!q::detail::valid(bad),"box bound overflow rejected");
    bool hit=false;q::Sweep result{};expected_shape=&box;expected_delta=&delta;
    check(q::detail::overlap(reinterpret_cast<std::uintptr_t>(&fake_overlap),0x180000,box,hit)&&hit,"box overlap ABI");
    check(q::detail::sweep(reinterpret_cast<std::uintptr_t>(&fake_sweep),0x180000,box,delta,result)&&result.raw_count==1&&result.raw_hit.bytes.front()==0xa5&&result.raw_hit.bytes.back()==0xa5,"box sweep ABI/64-byte result");
    expected_shape=&capsule;
    check(q::detail::overlap(reinterpret_cast<std::uintptr_t>(&fake_overlap),0x180000,capsule,hit)&&hit,"capsule overlap ABI");
    check(q::detail::sweep(reinterpret_cast<std::uintptr_t>(&fake_sweep),0x180000,capsule,delta,result)&&result.raw_count==1,"capsule sweep ABI");
    check(abi_calls==4,"synthetic dispatch count");
    check(q::detail::OriginalCalls::overlap(reinterpret_cast<std::uintptr_t>(&fake_overlap),0x180000,capsule,hit)&&hit,"production overlap forwarding ABI");
    check(q::detail::OriginalCalls::sweep(reinterpret_cast<std::uintptr_t>(&fake_sweep),0x180000,capsule,delta,result)&&result.raw_count==1,"production sweep forwarding ABI");
    check(abi_calls==6,"production forwarding calls exactly once");
    hit=false;check(!q::detail::overlap(reinterpret_cast<std::uintptr_t>(&fake_fault),0x180000,box,hit)&&!hit,"SEH does not return miss");
    q::Queries queries;s::Source input=source();q::Overlap output{true};unsigned reads{},guards{};
    auto read=[&](void*,void const*,std::size_t){++reads;return false;};auto guard=[&](){++guards;return true;};
    check(queries.overlap(input,box,read,guard,output)==q::Code::unavailable&&output.raw_hit,"unprimed box keeps output");
    check(queries.overlap(input,capsule,read,guard,output)==q::Code::unavailable,"unprimed capsule");
    result.raw_count=77;
    check(queries.sweep(input,box,delta,read,guard,result)==q::Code::unavailable&&result.raw_count==77,"unprimed sweep keeps output");
    check(queries.sweep(input,capsule,delta,read,guard,result)==q::Code::unavailable,"unprimed capsule sweep");
    check(queries.sweep(input,box,q::Vector{},read,guard,result)==q::Code::request,"zero sweep not occupancy");
    check(!reads&&!guards&&!queries.pending()&&queries.close(),"unavailable never reads target or acquires debt");
    check(!GetModuleHandleW(L"PhysXCore.dll"),"original DLL not loaded before prime");
    check(!queries.prime()&&!GetModuleHandleW(L"PhysXCore.dll"),"prime never loads missing module");
}
int main(int argc,char** argv){
    if(argc!=2){std::puts("expected exact original PhysXCore.dll path as data");return 2;}
    try{source_tests();code_tests(argv[1]);query_tests();}catch(...){check(false,"fixture exception");}
    std::printf("CoverPhysXQuery checks=%u failed=%u original-executed=false game-started=false\n",checks,failed);return failed?1:0;
}
