// Actual QueryEngine/source/code checks; only module pin and original call
// are explicit Host seams. Original DLL is read as data, never executed.
#define main PriorSphereQueryFixtureMain
#include "cover_physx_query_test.cpp"
#undef main
#include <memory>
namespace sphere_test {
Memory* memory{};unsigned calls{},retains{};bool hit{},fault{},late_source{},late_code{};
struct Image {
    bool live{};
    std::uintptr_t base()const{return live?::base:0;}
    std::uint32_t size()const{return ::size;}
    bool prime(){live=true;return true;}
    void close(){live=false;}
    void retain_for_process(){++retains;}
};
struct Calls {
    static bool overlap(std::uintptr_t entry,std::uintptr_t receiver,q::Sphere const& shape,bool& output){
        ++calls;check(entry==base+0x1683d0&&receiver==0x180000,"dedicated sphere entry and exact current receiver");
        check(shape.centre.x==1&&shape.centre.y==2&&shape.centre.z==3&&shape.radius==.25f,"owned sphere payload preserved");
        if(late_source)memory->put<std::uintptr_t>(0x150088,0);
        if(late_code)memory->put<unsigned char>(base+0x16844c,0);
        if(fault)return false;
        output=hit;return true;
    }
};
using Engine=q::QueryEngine<Image,Calls>;
}
int main(int argc,char** argv){
    if(argc!=2)return 2;using namespace sphere_test;
    Memory original=fixture();check(load_code(argv[1],original),"exact original file as data");
    if(original.blocks.empty())return 1;
    auto m=original;memory=&m;
    auto read=[&](void* out,void const* address,std::size_t n){return m.read(out,address,n);};
    auto guard=[](){return true;};const auto input=source();q::Sphere sphere{{1,2,3},.25f};
    auto engine=std::make_unique<Engine>();check(engine->prime(),"Host image pin");q::Overlap output{true};
    check(engine->overlap(input,sphere,read,guard,output)==q::Code::queried&&!output.raw_hit&&calls==1,"valid raw miss, no occupancy authority");
    hit=true;check(engine->overlap(input,sphere,read,guard,output)==q::Code::queried&&output.raw_hit&&calls==2,"raw overlap hit");
    for(auto slot:{0x23cdb8u,0x22cf10u}){
        m=original;m.put<std::uintptr_t>(base+slot,base+0x1000);output.raw_hit=true;const auto before=calls;
        check(engine->overlap(input,sphere,read,guard,output)==q::Code::source&&output.raw_hit&&calls==before,"sphere dispatch slot substitution rejected before call");
    }
    for(auto rva:{0x16844cu,0xa6738u,0x12678bu,0x185a4fu,0x1ab306u}){
        m=original;m.blocks[base][rva]^=1;const auto before=calls;output.raw_hit=true;
        check(engine->overlap(input,sphere,read,guard,output)==q::Code::code&&output.raw_hit&&calls==before,"complete new range tail mutation rejected before call");
    }
    m=original;late_source=true;output.raw_hit=false;
    check(engine->overlap(input,sphere,read,guard,output)==q::Code::changed&&!output.raw_hit,"late source replacement cannot publish hit");late_source=false;
    m=original;late_code=true;output.raw_hit=false;
    check(engine->overlap(input,sphere,read,guard,output)==q::Code::code&&!output.raw_hit,"late original-code mutation cannot publish hit");late_code=false;
    m=original;auto invalid=sphere;invalid.radius=-1;const auto before=calls;output.raw_hit=true;
    check(engine->overlap(input,invalid,read,guard,output)==q::Code::request&&output.raw_hit&&calls==before,"invalid sphere never dispatches");
    invalid=sphere;invalid.centre.x=(std::numeric_limits<float>::max)();invalid.radius=invalid.centre.x;
    check(engine->overlap(input,invalid,read,guard,output)==q::Code::request&&calls==before,"sphere bounds overflow rejected");
    fault=true;output.raw_hit=false;
    check(engine->overlap(input,sphere,read,guard,output)==q::Code::fault_unknown&&!output.raw_hit&&engine->pending()&&retains==1,"fault stays uncertain and retains pin");
    check(!engine->close(),"uncertain original lock is never force-closed");
    check(!GetModuleHandleW(L"PhysXCore.dll"),"original module never loaded");
    std::printf("CoverSphereOverlap checks=%u failed=%u original-executed=false game-started=false\n",checks,failed);return failed?1:0;
}
