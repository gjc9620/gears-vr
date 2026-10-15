#include "hook_entry_attestation.h"
#include <cstdio>
#include <cstdlib>
using namespace gears_hook_entry;
static unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d\n",__LINE__);std::exit(1);}}while(false)
struct Memory {
    std::uintptr_t target=0x1000,relay=0x2000,detour=0x3000;
    std::array<unsigned char,32> original{},patch{};
    std::array<unsigned char,14> jump{};
    unsigned calls{},mutate_at{};bool fail{};
    Memory(){for(unsigned i=0;i<32;++i)original[i]=static_cast<unsigned char>(i+0x40);build();}
    void build(){patch=original;patch[0]=0xe9;const auto displacement=std::int32_t(std::int64_t(relay)-std::int64_t(target+5));
        std::memcpy(patch.data()+1,&displacement,4);jump={0xff,0x25};std::memcpy(jump.data()+6,&detour,8);}
    bool read(void* out,void const* from,std::size_t size){
        ++calls;if(fail)return false;if(mutate_at==calls)patch[8]^=1;
        auto address=reinterpret_cast<std::uintptr_t>(from);
        if(address==target&&size==32){std::memcpy(out,patch.data(),size);return true;}
        if(address==relay&&size==14){std::memcpy(out,jump.data(),size);return true;}return false;
    }
};
int main(){
    Memory m;auto read=[&](void* out,void const* from,std::size_t size){return m.read(out,from,size);};
    Proof proof;CHECK(seal(m.target,m.detour,m.original,read,proof));CHECK(verify(proof,read));
    const auto saved=proof;CHECK(!seal(m.target,m.detour,m.original,read,proof));CHECK(proof.patched32==saved.patched32&&proof.relay14==saved.relay14);
    std::array<unsigned char,32> output{};CHECK(read_original(proof,m.target,output.data(),32,read));CHECK(output==m.original);
    CHECK(read_original(proof,m.target+31,output.data(),1,read));CHECK(output[0]==m.original[31]);
    CHECK(!read_original(proof,m.target+31,output.data(),2,read));CHECK(!read_original(proof,m.target-1,output.data(),2,read));
    CHECK(!read_original(proof,m.target+32,output.data(),1,read));CHECK(!read_original(proof,m.target,output.data(),0,read));
    CHECK(!read_original(proof,UINTPTR_MAX,output.data(),2,read));CHECK(!read_original(proof,m.target,nullptr,1,read));
    for(unsigned index:{0u,4u,5u,31u}){m.patch[index]^=1;CHECK(!verify(proof,read));CHECK(!read_original(proof,m.target,output.data(),1,read));m.patch[index]^=1;}
    for(unsigned index:{0u,1u,2u,5u,6u,13u}){m.jump[index]^=1;CHECK(!verify(proof,read));m.jump[index]^=1;}
    m.detour+=0x100;m.build();CHECK(!seal(m.target,m.detour,m.original,read,proof));
    CHECK(proof.detour==saved.detour&&proof.patched32==saved.patched32&&proof.relay14==saved.relay14);
    output.fill(0xcc);CHECK(!read_original(proof,m.target,output.data(),32,read));
    for(auto value:output)CHECK(value==0xcc);
    m.detour-=0x100;m.build();
    Proof bad=proof;bad.detour++;CHECK(!verify(bad,read));bad=proof;bad.relay++;CHECK(!verify(bad,read));
    m.patch[0]=0xeb;Proof rejected;CHECK(!seal(m.target,m.detour,m.original,read,rejected));CHECK(!rejected.target);m.build();
    m.patch[20]^=1;CHECK(!seal(m.target,m.detour,m.original,read,rejected));m.build();
    m.fail=true;CHECK(!verify(proof,read));CHECK(!seal(m.target,m.detour,m.original,read,rejected));m.fail=false;
    m.mutate_at=m.calls+4;CHECK(!verify(proof,read));m.mutate_at=0;m.build();
    m.mutate_at=m.calls+6;CHECK(!seal(m.target,m.detour,m.original,read,rejected));CHECK(!rejected.target);m.mutate_at=0;m.build();
    // Valid negative displacement, and both arithmetic extremes.
    m.relay=0x800;m.build();CHECK(seal(m.target,m.detour,m.original,read,rejected));CHECK(verify(rejected,read));
    std::uintptr_t relay{};auto patch=m.patch;std::int32_t displacement=INT32_MIN;std::memcpy(patch.data()+1,&displacement,4);
    CHECK(!relay_target(1,patch,relay));displacement=INT32_MAX;std::memcpy(patch.data()+1,&displacement,4);
    CHECK(!relay_target(UINTPTR_MAX-31,patch,relay));CHECK(!relay_target(UINTPTR_MAX-30,patch,relay));
    CHECK(!range(0,1));CHECK(!range(UINTPTR_MAX,2));
    std::printf("Hook entry attestation: %u checks passed\n",checks);
}
