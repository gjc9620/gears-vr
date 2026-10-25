#include "cover_bone_getters.h"
#include <cstdio>
#include <limits>
namespace b=gears_cover_bones;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
std::uintptr_t seen_mesh{};std::uint64_t seen_name{};int seen_index{};
int match(std::uintptr_t mesh,std::uint64_t name){seen_mesh=mesh;seen_name=name;return 55;}
void* matrix(std::uintptr_t mesh,void* output,int index){
    seen_mesh=mesh;seen_index=index;
    auto& value=*static_cast<b::Matrix*>(output);
    value.values[0]=value.values[5]=value.values[10]=value.values[15]=1;
    value.values[12]=100;value.values[13]=-3000;value.values[14]=-39;
    return output;
}
int fault_match(std::uintptr_t,std::uint64_t){RaiseException(0xe042c014,0,0,nullptr);return 0;}
void* fault_matrix(std::uintptr_t,void*,int){RaiseException(0xe042c015,0,0,nullptr);return nullptr;}
int main(){
    static constexpr char abc_sha[]="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    C(b::digest_matches("abc",3,abc_sha));C(!b::digest_matches("abd",3,abc_sha));
    C(!b::digest_matches(nullptr,3,abc_sha));C(!b::digest_matches("abc",0,abc_sha));C(!b::digest_matches("abc",3,"abc"));
    int index=-1;const std::uint64_t name=0x1234567887654321;
    C(b::call_match(reinterpret_cast<std::uintptr_t>(match),0x99887766,name,index));
    C(index==55&&seen_mesh==0x99887766&&seen_name==name);
    b::Matrix value{};C(!b::valid(value));
    C(b::call_matrix(reinterpret_cast<std::uintptr_t>(matrix),0x88776655,index,value));
    C(seen_mesh==0x88776655&&seen_index==55&&b::valid(value));
    C(value.values[12]==100&&value.values[14]==-39);
    C(!b::call_match(reinterpret_cast<std::uintptr_t>(fault_match),0,name,index));
    C(!b::call_matrix(reinterpret_cast<std::uintptr_t>(fault_matrix),0,55,value));
    for(unsigned n=0;n<16;++n){auto invalid=value;invalid.values[n]=(std::numeric_limits<float>::infinity)();C(!b::valid(invalid));}
    for(unsigned n:{3u,7u,11u}){auto invalid=value;invalid.values[n]=1;C(!b::valid(invalid));}
    auto zero=value;zero.values[0]=zero.values[1]=zero.values[2]=0;C(!b::valid(zero));
    b::Result output{};output.index=7;unsigned reads=0,guards=0;
    auto read=[&](void*,void const*,std::size_t){++reads;return false;};
    auto guard=[&](){++guards;return true;};
    C(!b::query(0,1,1,GetCurrentThreadId(),read,guard,output)&&output.index==7&&guards==1&&reads==0);
    C(!b::query(0,1,1,0,read,guard,output)&&output.index==7&&guards==1&&reads==0);
    C(!b::certify(UINTPTR_MAX-0x1000,read)&&reads==0);
    C(!b::certify(0x140000000,read)&&reads==1);
    C(b::chunks.size()==8);
    std::printf("cover_bone_getters: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
