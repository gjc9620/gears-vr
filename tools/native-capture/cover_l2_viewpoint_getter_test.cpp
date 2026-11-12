#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "cover_l2_viewpoint_getter.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
namespace l2=gears_cover_l2_points;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
struct Fixture {
    static constexpr uintptr_t base=0x140000000,link=0x20000000;
    l2::surface::Array header{0x30000000,3,3};std::array<unsigned char,36> offsets{};
    bool stable=true,certificate=true,fail_call=false,mutate=false,nonfinite=false;
    std::vector<unsigned> calls;
    bool read(void* out,void const* source,size_t size){
        const auto at=reinterpret_cast<uintptr_t>(source);
        if(at==link+0x340&&size==sizeof header){std::memcpy(out,&header,size);return true;}
        if(at==link+0x384&&size==offsets.size()){std::memcpy(out,offsets.data(),size);return true;}
        return false;
    }
    bool run(unsigned char type,l2::Permissions p,l2::Result& out){
        return l2::query(base,link,1,GetCurrentThreadId(),type,p,
            [&](void* a,void const* b,size_t n){return read(a,b,n);},[&](){return stable;},out,
            [&](uintptr_t entry,uintptr_t actor,int slot,unsigned char ct,unsigned char action,l2::Vector& v){
                C(entry==base+l2::chunks[0].rva&&actor==link&&slot==1&&ct==type);
                calls.push_back(action);v={float(action),float(ct),nonfinite?NAN:19.f};
                if(mutate)++offsets[0];return !fail_call;
            },[&](uintptr_t,auto){return certificate;});
    }
};
int main(int argc,char** argv){
    l2::Permissions all{true,true,true,true};l2::Result out{};out.type=99;
    Fixture f;C(f.run(2,all,out));C((f.calls==std::vector<unsigned>{0,3,4,7}));
    C(out.type==2&&out.available[0]&&out.available[1]&&out.available[2]&&out.points[2].x==7);
    f={};C(f.run(1,all,out));C((f.calls==std::vector<unsigned>{0,3,4}));C(!out.available[2]);
    f={};C(f.run(2,{true,false,true,false},out));C((f.calls==std::vector<unsigned>{0,4}));
    for(unsigned failure=0;failure<7;++failure){
        f={};out.type=99;
        if(failure==0)f.stable=false;
        if(failure==1)f.certificate=false;
        if(failure==2)f.fail_call=true;
        if(failure==3)f.mutate=true;
        if(failure==4)f.nonfinite=true;
        if(failure==5)f.header.count=1;
        C(!f.run(failure==6?0:2,all,out));C(out.type==99);
    }
    f={};C(!f.run(2,{false,true,true,true},out));C(f.calls.empty());
    // Hash check saved original code without executing any game instruction.
    if(argc==2){
        std::ifstream stream(argv[1],std::ios::binary);std::vector<char> code{std::istreambuf_iterator<char>(stream),{}};
        auto read=[&](void* dest,void const* source,size_t bytes){
            auto at=reinterpret_cast<uintptr_t>(source);
            if(at<Fixture::base+0x1000)return false;
            at-=Fixture::base+0x1000;
            if(at>code.size()||bytes>code.size()-at)return false;
            std::memcpy(dest,code.data()+at,bytes);return true;
        };
        C(!code.empty());C(l2::certify(Fixture::base,read));
        if(code.size()>l2::chunks[0].rva-0x1000){code[l2::chunks[0].rva-0x1000]^=1;C(!l2::certify(Fixture::base,read));}
    }
    std::printf("CoverL2ViewpointGetterTest checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
