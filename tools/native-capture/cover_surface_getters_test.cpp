#include "cover_surface_getters.h"
#include <cstdio>
#include <fstream>
#include <vector>
#include <limits>
namespace s=gears_cover_surface;
namespace {
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
using Blobs=std::array<std::vector<unsigned char>,4>;
// The private test runner mechanically extracts the four saved JSON chunks
// into this bounded fixture. No game body/byte literal is stored in the repo.
bool load(const char* path,Blobs& blobs){
    std::ifstream input(path,std::ios::binary);std::uint32_t count{};
    if(!input.read(reinterpret_cast<char*>(&count),sizeof(count))||count!=s::chunks.size())return false;
    for(unsigned i=0;i<count;++i){std::uint64_t rva{};std::uint32_t size{};
        if(!input.read(reinterpret_cast<char*>(&rva),sizeof(rva))||!input.read(reinterpret_cast<char*>(&size),sizeof(size))||
           rva!=s::chunks[i].rva||size!=s::chunks[i].size||size>512)return false;
        blobs[i].resize(size);if(!input.read(reinterpret_cast<char*>(blobs[i].data()),size)||
            !gears_cover_bones::digest_matches(blobs[i].data(),size,s::chunks[i].sha256))return false;
    }
    return input.peek()==std::char_traits<char>::eof();
}
struct Fixture {
    static constexpr std::uintptr_t base=0x140000000,link=0x500000;
    Blobs const& blobs;s::Array header{0x600000,3,4};s::WorldLocation location{100,-3000,25};s::Rotation rotation{{1,-32768,65536}};
    unsigned reads{},code_reads{},header_reads{},guards{},locations{},rotations{},violations{};
    unsigned fail_read{},fail_guard{},mutate_code{},mutate_header{};bool location_fail{},rotation_fail{},change_after_location{},change_after_rotation{};
    bool read(void* out,void const* source,std::size_t bytes){
        if(++reads==fail_read)return false;const auto address=reinterpret_cast<std::uintptr_t>(source);
        if(address==link+s::slots_offset&&bytes==sizeof(header)){
            ++header_reads;auto value=header;if(header_reads==mutate_header)++value.capacity;std::memcpy(out,&value,bytes);return true;
        }
        for(unsigned i=0;i<s::chunks.size();++i)if(address==base+s::chunks[i].rva&&bytes==s::chunks[i].size){
            std::memcpy(out,blobs[i].data(),bytes);if(++code_reads==mutate_code)static_cast<unsigned char*>(out)[0]^=1;return true;
        }
        return false;
    }
    bool guard(){return ++guards!=fail_guard;}
    struct Invoke {
        Fixture* f;
        bool location(std::uintptr_t entry,std::uintptr_t selected_link,int slot,s::WorldLocation& out){
            ++f->locations;if(entry!=base+s::chunks[0].rva||selected_link!=Fixture::link||slot!=1)++f->violations;
            out=f->location;if(f->change_after_location)++f->header.capacity;return !f->location_fail;
        }
        bool rotation(std::uintptr_t entry,std::uintptr_t selected_link,int slot,s::Rotation& out){
            ++f->rotations;if(entry!=base+s::chunks[1].rva||selected_link!=Fixture::link||slot!=1)++f->violations;
            out=f->rotation;if(f->change_after_rotation)++f->header.data;return !f->rotation_fail;
        }
    };
    bool query(s::Result& out,int slot=1,unsigned thread=GetCurrentThreadId()){
        return s::query(base,link,slot,thread,[this](void* a,void const* b,std::size_t n){return read(a,b,n);},[this](){return guard();},out,Invoke{this});
    }
};
std::uintptr_t observed_link{};int observed_slot{};unsigned invoked{};
void* location(std::uintptr_t link,void* output,int slot){++invoked;observed_link=link;observed_slot=slot;*static_cast<s::WorldLocation*>(output)={1,2,3};return output;}
void* rotation(std::uintptr_t link,void* output,int slot){++invoked;observed_link=link;observed_slot=slot;*static_cast<s::Rotation*>(output)={{1,2,3}};return output;}
void* wrong(std::uintptr_t,void*,int){++invoked;return nullptr;}
void* fault(std::uintptr_t,void*,int){++invoked;RaiseException(0xe042c060,0,0,nullptr);return nullptr;}
void leaves(){
    s::WorldLocation p{};s::Rotation r{};
    C(s::call_location(reinterpret_cast<std::uintptr_t>(location),0x11223344,7,p));C(observed_link==0x11223344&&observed_slot==7&&p.x==1&&p.z==3);
    C(s::call_rotation(reinterpret_cast<std::uintptr_t>(rotation),0x88776655,-2,r));C(observed_link==0x88776655&&observed_slot==-2&&r.values[0]==1&&r.values[2]==3);
    C(!s::call_location(reinterpret_cast<std::uintptr_t>(wrong),0,0,p));C(!s::call_rotation(reinterpret_cast<std::uintptr_t>(wrong),0,0,r));
    C(!s::call_location(reinterpret_cast<std::uintptr_t>(fault),0,0,p));C(!s::call_rotation(reinterpret_cast<std::uintptr_t>(fault),0,0,r));C(invoked==6);
}
void unchanged(s::Result const& value){C(value.slot==99&&value.location.x==1234&&value.rotation.values[0]==5678);}
s::Result marker(){s::Result value{};value.slot=99;value.location.x=1234;value.rotation.values[0]=5678;return value;}
}
int main(int argc,char** argv){
    if(argc!=2){std::fprintf(stderr,"Explicit private historical-chunk fixture required; no embedded/proprietary bodies.\n");return 2;}
    Blobs blobs;if(!load(argv[1],blobs)){std::fprintf(stderr,"Four exact bounded hash-matching chunks required.\n");return 2;}
    leaves();Fixture good{blobs};s::Result out{};C(good.query(out));
    C(out.slot==1&&s::same(out.slots,good.header)&&out.location.x==100&&out.location.z==25&&out.rotation.values[1]==-32768);
    C(good.locations==1&&good.rotations==1&&good.violations==0&&good.code_reads==16&&good.header_reads==4&&good.guards==7);
    for(unsigned i=1;i<=good.reads;++i){Fixture f{blobs};f.fail_read=i;auto value=marker();C(!f.query(value));unchanged(value);}
    for(unsigned i=1;i<=good.guards;++i){Fixture f{blobs};f.fail_guard=i;auto value=marker();C(!f.query(value));unchanged(value);}
    for(unsigned i=1;i<=16;++i){Fixture f{blobs};f.mutate_code=i;auto value=marker();C(!f.query(value));unchanged(value);}
    for(unsigned i=1;i<=4;++i){Fixture f{blobs};f.mutate_header=i;auto value=marker();C(!f.query(value));unchanged(value);}
    for(unsigned field=0;field<3;++field)for(float invalid:{(std::numeric_limits<float>::quiet_NaN)(),(std::numeric_limits<float>::infinity)(),-(std::numeric_limits<float>::infinity)(),16777218.0f,-16777218.0f}){
        Fixture f{blobs};if(field==0)f.location.x=invalid;else if(field==1)f.location.y=invalid;else f.location.z=invalid;
        auto value=marker();C(!f.query(value));unchanged(value);C(f.rotations==0);
    }
    for(auto bad:{s::Array{0,3,4},s::Array{0x600001,3,4},s::Array{0x600000,-1,4},s::Array{0x600000,0,4},s::Array{0x600000,4,3},s::Array{0x600000,257,257},s::Array{0x600000,3,257},s::Array{UINTPTR_MAX-3,3,4}}){
        Fixture f{blobs};f.header=bad;auto value=marker();C(!f.query(value));unchanged(value);C(!f.locations&&!f.rotations);
    }
    for(int slot:{-1,3,256,(std::numeric_limits<int>::max)()}){Fixture f{blobs};auto value=marker();C(!f.query(value,slot));unchanged(value);C(!f.locations);}
    for(unsigned thread:{0u,unsigned(GetCurrentThreadId()+1)}){Fixture f{blobs};auto value=marker();C(!f.query(value,1,thread));unchanged(value);C(!f.reads&&!f.guards&&!f.locations);}
    for(unsigned mode=0;mode<4;++mode){Fixture f{blobs};f.location_fail=mode==0;f.rotation_fail=mode==1;f.change_after_location=mode==2;f.change_after_rotation=mode==3;auto value=marker();C(!f.query(value));unchanged(value);}
    Fixture limits{blobs};limits.location={16777216.0f,-16777216.0f,0};limits.rotation={{(std::numeric_limits<int>::min)(),(std::numeric_limits<int>::max)(),0}};
    C(limits.query(out)&&out.location.x==16777216.0f&&out.rotation.values[0]==(std::numeric_limits<int>::min)());
    Fixture capacity{blobs};capacity.header.count=capacity.header.capacity=256;C(capacity.query(out)&&out.slots.count==256);
    C(!s::certify(0,[](void*,void const*,std::size_t){return false;}));
    unsigned reads=0;C(!s::certify(UINTPTR_MAX-0x1000,[&](void*,void const*,std::size_t){++reads;return false;})&&reads==0);
    auto value=marker();C(!s::query(Fixture::base,UINTPTR_MAX-0xff,1,GetCurrentThreadId(),[&](void*,void const*,std::size_t){++reads;return false;},[](){return true;},value)&&reads==0);unchanged(value);
    C(s::chunks.size()==4&&s::slots_offset==0x340&&s::slot_stride==212);
    std::printf("cover_surface_getters: %u checks, %u failures; historical fixture only, no game/native body execution\n",checks,failures);return failures?1:0;
}
