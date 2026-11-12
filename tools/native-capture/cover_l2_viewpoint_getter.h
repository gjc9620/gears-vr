#pragma once
#include "cover_surface_getters.h"

// Historical code evidence: original-l2-points-r257/viewpoint-asm.txt.
// The script wrapper decodes int SlotIdx, byte Type, byte Action and calls
// this vector implementation. Production must match CURRENT code and the
// caller's current typed CoverLink/slot/Scene before and after every call.
namespace gears_cover_l2_points {
namespace surface=gears_cover_surface;
using Vector=surface::WorldLocation;
inline constexpr std::array<surface::Chunk,2> chunks{{
    {0x3b61b0,795,"00303c3db289aa4815270c6ed1bc7cdfb0e10f1a0eecbf02c58e7d60faf61006"},
    {0x921a86,26,"aa99201625e163472ab8f0c3c5d5fddeb0aade5cab4b493fe0153fb586a3dc1e"}
}};
struct Permissions {bool enabled{},left{},right{},top{};};
struct Result {
    Vector neutral{};
    std::array<Vector,3> points{}; // original actions 3/4/7
    std::array<bool,3> available{};
    unsigned char type{};
};
template<class Read> bool certify(std::uintptr_t base,Read read){
    if(!surface::certify(base,read))return false;
    for(auto const& chunk:chunks){
        if(base>UINTPTR_MAX-chunk.rva||base+chunk.rva>UINTPTR_MAX-chunk.size)return false;
        std::array<unsigned char,1024> first{},second{};
        const auto address=reinterpret_cast<void const*>(base+chunk.rva);
        if(chunk.size>first.size()||!read(first.data(),address,chunk.size)||
           !gears_cover_bones::digest_matches(first.data(),chunk.size,chunk.sha256)||
           !read(second.data(),address,chunk.size)||std::memcmp(first.data(),second.data(),chunk.size))return false;
    }
    return true;
}
// ASM ABI, independent of the decompiler's inferred prototype:
// RCX=Link, RDX=12-byte caller output, R8D=slot, R9B=Type,
// [entry RSP+0x28]=Action, RAX=output. No script dispatcher is called.
inline bool call(std::uintptr_t entry,std::uintptr_t link,int slot,unsigned char type,
    unsigned char action,Vector& output)noexcept{
    using Function=void*(*)(std::uintptr_t,void*,int,unsigned char,unsigned char);
    __try{return reinterpret_cast<Function>(entry)(link,&output,slot,type,action)==&output;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct Invoker {
    bool operator()(std::uintptr_t entry,std::uintptr_t link,int slot,unsigned char type,
        unsigned char action,Vector& output)const noexcept{return call(entry,link,slot,type,action,output);}
};
// Injection points permit offline checks of permission, binding and publication
// behavior without executing the game. Certificate defaults to CURRENT hashes.
struct Certificate {
    template<class Read>bool operator()(std::uintptr_t base,Read read)const{return certify(base,read);}
};
template<class Read,class Guard,class Invoke=Invoker,class Code=Certificate>
bool query(std::uintptr_t base,std::uintptr_t link,int slot,unsigned thread,unsigned char type,
    Permissions permissions,Read read,Guard guard,Result& output,Invoke invoke={},Code code={}){
    if(!base||!link||link%8||link>UINTPTR_MAX-0x3a8||!thread||GetCurrentThreadId()!=thread||
       (type!=1&&type!=2)||!permissions.enabled||!guard()||!code(base,read))return false;
    surface::Array header{};
    std::array<unsigned char,36> offsets{}; // Standing/Crouch/Popup offsets.
    if(!read(&header,reinterpret_cast<void const*>(link+0x340),sizeof header)||!surface::valid(header,slot)||
       !read(offsets.data(),reinterpret_cast<void const*>(link+0x384),offsets.size()))return false;
    auto current=[&](){
        surface::Array h{};std::array<unsigned char,36> values{};
        return guard()&&read(&h,reinterpret_cast<void const*>(link+0x340),sizeof h)&&surface::same(h,header)&&
            read(values.data(),reinterpret_cast<void const*>(link+0x384),values.size())&&values==offsets;
    };
    Result next{};next.type=type;
    if(!current()||!invoke(base+chunks[0].rva,link,slot,type,0,next.neutral)||
       !surface::valid(next.neutral)||!current())return false;
    const bool allowed[]={permissions.left,permissions.right,type==2&&permissions.top};
    constexpr unsigned char actions[]={3,4,7};
    for(unsigned i=0;i<3;++i)if(allowed[i]){
        if(!current()||!invoke(base+chunks[0].rva,link,slot,type,actions[i],next.points[i])||
           !surface::valid(next.points[i])||!current())return false;
        next.available[i]=true;
    }
    if(!code(base,read)||!current())return false;
    output=next;return true;
}
} // namespace gears_cover_l2_points
