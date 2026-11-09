#pragma once
#include "weapon_hammerburst_refire.h"
namespace gears_hammerburst_first {
using Name=gears_hammerburst_refire::Name;
struct Bindings {std::array<std::uint64_t,13> objects{};std::array<Name,3> names{};};
// Original export37976:191 serialized bytes,271 runtime bytes. These are the
// twenty object operand positions after32->64 relocation, including repeated
// references. The three virtual FNames retain their original eight-byte width.
inline constexpr std::array<unsigned,20> runtime_objects{{3,14,23,38,47,60,71,91,111,121,133,148,160,187,197,209,221,230,250,259}};
inline constexpr std::array<unsigned,20> object_slots{{9,6,1,6,1,5,10,12,8,1,0,1,4,8,1,0,3,7,2,11}};
inline constexpr std::array<unsigned,3> runtime_names{{82,175,240}};
inline bool entry(unsigned char const* code,std::size_t bytes,Bindings const& b) noexcept {
    if(!code||bytes!=271)return false;
    for(unsigned i=0;i<b.objects.size();++i){
        if(!b.objects[i])return false;
        for(unsigned j=0;j<i;++j)if(b.objects[i]==b.objects[j])return false;
    }
    for(auto const& n:b.names)if(!n.index||n.number)return false;
    // All non-reference tokens, branch targets, constants, original counter
    // reset, sound/feedback branches and original super call remain exact.
    constexpr char raw[]="142d013294000082772e1e0e000001c5f6ffff2a16182100192e1e0e000001c5f6ffff0a00042d01f7040000160f013594000024001b7a7f00000000000020115a01001e0000803f1607f90082772e596900001901c5f6ffff0900080184f6ffff2a161818001901c5f6ffff0a00041c81f9ffff161607ef00811b505b0000000000001616192e596900001901c5f6ffff0900080184f6ffff1300001c2ff9ffff01c9590000161b026a000000000000161c86f7ffff005794000016040b53";
    static_assert(sizeof raw==383);
    constexpr std::array<unsigned,20> offsets{{3,10,15,26,31,40,47,63,79,85,93,104,112,135,141,149,157,162,178,183}};
    constexpr std::array<unsigned,3> names{{54,123,168}};
    std::array<unsigned char,271> expected{};unsigned in{},out{},object{},name{};
    const auto digit=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
    while(in<191){
        if(object<offsets.size()&&in==offsets[object]){
            std::memcpy(expected.data()+out,&b.objects[object_slots[object]],8);out+=8;in+=4;++object;
        }else if(name<names.size()&&in==names[name]){
            std::memcpy(expected.data()+out,&b.names[name],8);out+=8;in+=8;++name;
        }else{expected[out++]=static_cast<unsigned char>(16*digit(raw[2*in])+digit(raw[2*in+1]));++in;}
    }
    return out==271&&object==20&&name==3&&!std::memcmp(expected.data(),code,271);
}
}
