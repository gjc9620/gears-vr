#pragma once
#include "cover_bone_getters.h"
#include <limits>
#include <type_traits>

// Read-only implementation calls, NOT a surface/collision/plane certificate.
// Hashes are from the saved historical surface-getters2/native-pose-math
// exports. A caller must match CURRENT bytes and its complete current typed
// Link/Pawn/World/slot binding; matching RVAs or historical blobs are insufficient.
namespace gears_cover_surface {
using Chunk=gears_cover_bones::Chunk;
inline constexpr std::array<Chunk,4> chunks{{
    {0x10f6cc,273,"fbe8d1e67c3837b5c9802e9f8e8ccd9d14ada3f328b54cc6b8e18636ae087f40"},
    {0x10eb80,84,"39c24bca471af82e5ffd72b287b2131e5bdd88b1b7d6c9c1439dccbb39ff1d09"},
    {0x8a43d2,23,"6a68f007d4b9c27a205bc33467d53425d01ec6e1f58a47e8f25d8cf957cc14cf"},
    {0x1330e0,374,"05965ddb6c030ab5ec90bc2eefa3178ca7313d9492a0965e2e4f65f4f407d461"}
}};
inline constexpr std::uintptr_t slots_offset=0x340,slot_stride=212;
struct WorldLocation {float x{},y{},z{};};
// Preserve native int32 component order/bits. No degrees/radians, facing,
// mirroring or modulo normalization is inferred by this transport.
struct Rotation {std::int32_t values[3]{};};
struct Array {std::uintptr_t data{};std::int32_t count{},capacity{};};
struct Result {WorldLocation location{};Rotation rotation{};Array slots{};int slot=-1;};
static_assert(sizeof(void*)==8&&sizeof(WorldLocation)==12&&sizeof(Rotation)==12&&sizeof(Array)==16);
static_assert(offsetof(Array,count)==8&&offsetof(Array,capacity)==12);
static_assert(std::is_trivially_copyable_v<Result>&&std::is_standard_layout_v<Result>);
inline bool same(Array const& a,Array const& b) noexcept {
    return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;
}
inline bool valid(WorldLocation const& value) noexcept {
    constexpr float limit=16777216.0f;
    return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z)&&
        std::fabs(value.x)<=limit&&std::fabs(value.y)<=limit&&std::fabs(value.z)<=limit;
}
inline bool valid(Array const& array,int slot) noexcept {
    return array.data&&array.data%alignof(std::int32_t)==0&&array.count>0&&array.count<=256&&
        array.capacity>=array.count&&array.capacity<=256&&slot>=0&&slot<array.count&&
        array.data<=UINTPTR_MAX-std::uintptr_t(array.capacity)*slot_stride;
}
template<class Read> bool certify(std::uintptr_t base,Read read){
    if(!base)return false;
    for(auto const& chunk:chunks){
        if(chunk.size>512||base>UINTPTR_MAX-chunk.rva||base+chunk.rva>UINTPTR_MAX-chunk.size)return false;
        std::array<unsigned char,512> first{},last{};
        auto* address=reinterpret_cast<void const*>(base+chunk.rva);
        if(!read(first.data(),address,chunk.size)||!gears_cover_bones::digest_matches(first.data(),chunk.size,chunk.sha256)||
           !read(last.data(),address,chunk.size)||std::memcmp(first.data(),last.data(),chunk.size))return false;
    }
    return true;
}
// Saved ASM establishes ONLY these Windows x64 register ABIs:
// RCX=Link, RDX=caller-owned 12-byte output, R8D=signed slot; RAX=output.
// These leaves contain no RAII and isolate only the two getter calls. They do
// not call wrappers/dispatchers, other engine functions or read a Result buffer.
inline bool call_location(std::uintptr_t entry,std::uintptr_t link,int slot,WorldLocation& output) noexcept {
    using Function=void*(*)(std::uintptr_t,void*,int);
    __try {return reinterpret_cast<Function>(entry)(link,&output,slot)==&output;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool call_rotation(std::uintptr_t entry,std::uintptr_t link,int slot,Rotation& output) noexcept {
    using Function=void*(*)(std::uintptr_t,void*,int);
    __try {return reinterpret_cast<Function>(entry)(link,&output,slot)==&output;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct NativeInvoker {
    bool location(std::uintptr_t entry,std::uintptr_t link,int slot,WorldLocation& out) const noexcept {return call_location(entry,link,slot,out);}
    bool rotation(std::uintptr_t entry,std::uintptr_t link,int slot,Rotation& out) const noexcept {return call_rotation(entry,link,slot,out);}
};

// Read(out,address,size) must be a bounded, fault-contained current-process RO
// reader; Guard() must be nonthrowing and authenticate the SAME typed Link,
// controlled Pawn, current World/generation, slot selection and reflected
// Slots layout at EVERY call. The caller supplies Scene/GT admission and any
// stronger transform/cohort checks; a stable header is not an atomic snapshot.
// Invoker is injectable for CPU tests. Production uses the certified defaults.
// Nothing here writes an Actor, Link, slot, mesh, result buffer or game memory.
template<class Read,class Guard,class Invoker>
bool query(std::uintptr_t base,std::uintptr_t link,int slot,unsigned thread,
           Read read,Guard guard,Result& out,Invoker invoke){
    if(!link||link%alignof(std::uintptr_t)||link>UINTPTR_MAX-slots_offset-sizeof(Array)||
       !thread||GetCurrentThreadId()!=thread||!guard()||!certify(base,read))return false;
    Array before{},after{};auto* header=reinterpret_cast<void const*>(link+slots_offset);
    if(!read(&before,header,sizeof(before))||!valid(before,slot)||!guard())return false;
    Result candidate{};candidate.slot=slot;candidate.slots=before;
    if(!invoke.location(base+chunks[0].rva,link,slot,candidate.location)||!valid(candidate.location)||!guard()||
       !read(&after,header,sizeof(after))||!same(before,after)||!guard())return false;
    if(!invoke.rotation(base+chunks[1].rva,link,slot,candidate.rotation)||!guard()||
       !read(&after,header,sizeof(after))||!same(before,after)||!certify(base,read)||!guard())return false;
    // Re-read after the closing source certificate as well. Unknown metadata
    // never publishes candidate output or authorizes a later call/write.
    if(!read(&after,header,sizeof(after))||!same(before,after)||!guard())return false;
    out=candidate;return true;
}
template<class Read,class Guard>
bool query(std::uintptr_t base,std::uintptr_t link,int slot,unsigned thread,Read read,Guard guard,Result& out){
    return query(base,link,slot,thread,read,guard,out,NativeInvoker{});
}
} // namespace gears_cover_surface
