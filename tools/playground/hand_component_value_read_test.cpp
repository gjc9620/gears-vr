#include "../vr-common/weapon_mesh_component_lease.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <limits>

// Finite property-resolution/read seam. No engine, UObject ABI, original
// current/slot VM, elapsed-time or actual pose accuracy claim is made here.
namespace {
using uptr=std::uintptr_t;
using Value=gears_weapon::WeaponMeshComponentValue;
struct alignas(16) Node {std::array<unsigned char,256> bytes{};};
std::array<Node,8> nodes{};
std::array<uptr,5> bindings{};
std::array<bool,8> live{};
unsigned searches{},copies{},checks{};
int fail_copy{-1},fail_schema{-1},mutation{};
uptr address(unsigned index){return reinterpret_cast<uptr>(nodes[index].bytes.data());}
template<class T>T get(uptr p){T value{};std::memcpy(&value,reinterpret_cast<void const*>(p),sizeof(value));return value;}
template<class T>void put(uptr p,T value){std::memcpy(reinterpret_cast<void*>(p),&value,sizeof(value));}
bool registered(uptr p){
    for(unsigned i=0;i<nodes.size();++i)if(p==address(i))return live[i];
    return false;
}
void require(bool value,const char* why){
    ++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}
}
namespace playground {
template<std::size_t N>bool fields(uptr klass,const wchar_t* const (&labels)[N],std::array<uptr,N>& out){
    static_assert(N==5);++searches;
    constexpr const wchar_t* names[]={L"AbsoluteTranslation",L"AbsoluteRotation",L"AbsoluteScale",L"Translation",L"Rotation"};
    if(klass!=address(1))return false;
    for(unsigned i=0;i<N;++i){
        if(std::wcscmp(labels[i],names[i])||!bindings[i])return false;
        out[i]=bindings[i];
    }
    return true;
}
}
namespace lookup {
bool field_at(uptr mesh,uptr property,const wchar_t* kind,void* out,unsigned bytes){
    if(mesh!=address(0))return false;
    int index=-1;
    for(unsigned i=0;i<5;++i)if(property==address(i+2))index=static_cast<int>(i);
    if(index<0)return false;
    if(index==fail_copy)return false;
    const bool bit=index<3;
    if(bytes!=(bit?4u:12u)||std::wcscmp(kind,bit?L"BoolProperty":L"StructProperty"))return false;
    ++copies;
    std::memcpy(out,reinterpret_cast<void const*>(mesh+128+unsigned(index)*16),bytes);
    if(index==4){
        if(mutation==1)bindings[4]=address(2);
        if(mutation==2)put(mesh+0x50,address(7));
        if(mutation==3)live[0]=false;
    }
    return true;
}
}
bool triple_schema(uptr p,bool rotation){
    const int index=p==address(5)?3:(p==address(6)?4:-1);
    return index>=0&&index!=fail_schema&&(rotation==(index==4));
}
// This is the actual production helper, not a rewritten test implementation.
#include "hand_component_value_read.inl"

void reset(){
    nodes={};live.fill(true);searches=copies=0;fail_copy=fail_schema=-1;mutation=0;
    put(address(0)+0x50,address(1));
    for(unsigned i=0;i<5;++i)bindings[i]=address(i+2);
    for(unsigned i=0;i<3;++i){put(address(i+2)+0xd0,1u<<(i+1));put(address(0)+128+i*16,i==1?4u:0x80u);}
    const std::array<float,3> translation{1.25f,-2.5f,8.f};
    const std::array<int,3> rotation{11,-22,33};
    std::memcpy(reinterpret_cast<void*>(address(0)+176),translation.data(),12);
    std::memcpy(reinterpret_cast<void*>(address(0)+192),rotation.data(),12);
}
bool exact(Value const& a,Value const& b){return gears_weapon::mesh_component_exact(a,b);}
void expect_rejected(const char* why){
    Value before{};before.absolute.fill(true);before.translation.fill(0x5a);before.rotation.fill(0xa5);
    auto after=before;require(!read_mesh_values(address(0),after),why);
    require(exact(before,after),"failed read never publishes partial value");
}
}
int main(){
    reset();Value got{};require(read_mesh_values(address(0),got),"complete local read");
    require(searches==2&&copies==5,"two local hierarchy scans and all five typed reads");
    require(got.absolute==std::array<bool,3>{false,true,false},"packed bits use each property's own mask");
    require(std::memcmp(got.translation.data(),reinterpret_cast<void const*>(address(0)+176),12)==0&&
            std::memcmp(got.rotation.data(),reinterpret_cast<void const*>(address(0)+192),12)==0,"exact original triple bytes");
    for(unsigned i=0;i<5;++i){reset();bindings[i]=0;expect_rejected("missing field");}
    for(unsigned i=0;i<5;++i){reset();fail_copy=static_cast<int>(i);expect_rejected("failed typed read");}
    for(int i=3;i<5;++i){reset();fail_schema=i;expect_rejected("strict triple schema refusal");}
    for(unsigned i=0;i<3;++i)for(unsigned mask:{0u,3u}){
        reset();put(address(i+2)+0xd0,mask);expect_rejected("zero/multibit mask");
    }
    for(int i=1;i<=3;++i){reset();mutation=i;expect_rejected("late binding/class/lifetime change");}
    for(float x:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),1e8f}){
        reset();put(address(0)+176,x);expect_rejected("nonfinite/out-of-range translation");
    }
    reset();live[0]=false;expect_rejected("unregistered mesh");
    reset();live[1]=false;expect_rejected("unregistered class");
    std::printf("HandComponentValueReadTest checks=%u passed\n",checks);return 0;
}
