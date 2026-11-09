#pragma once
#include "cover_physx_query_code.h"
#include "cover_pose_phase.h"

// The caller's guard must certify its current C0 owner, original F26D4 field
// correspondence and typed mesh/PhysicsAssetInstance association. This closes
// the original main BodyData -> NpActor -> Scene -> NpScene path inside that
// scope. It issues NO callable token, geometry/world lease or body permission.
namespace gears_cover_physx_query_source {
namespace pose=gears_cover_pose_phase;
using Digest=std::array<unsigned char,32>;
struct Source {
    std::uintptr_t mesh{},asset{},instance{};
    std::uint32_t body_index{},thread{};
};
struct Snapshot {
    Source source{};pose::ArrayHeader bodies{};
    std::array<std::uintptr_t,64> members{};
    std::uintptr_t body{},actor{},actor_table{},scene{},scene_table{},public_scene{},public_table{};
};
enum class Code {current,request,guard,read,changed,layout,module};
inline bool same(Source const& a,Source const& b) noexcept {
    return a.mesh==b.mesh&&a.asset==b.asset&&a.instance==b.instance&&
        a.body_index==b.body_index&&a.thread==b.thread;
}
inline bool aliases(void const* a,std::size_t an,void const* b,std::size_t bn) noexcept {
    const auto av=reinterpret_cast<std::uintptr_t>(a),bv=reinterpret_cast<std::uintptr_t>(b);
    if(!pose::span(av,an)||!pose::span(bv,bn))return true;
    return av<=bv?bv-av<an:av-bv<bn;
}
inline bool same(Snapshot const& a,Snapshot const& b) noexcept {
    return a.source.mesh==b.source.mesh&&a.source.asset==b.source.asset&&a.source.instance==b.source.instance&&
        a.source.body_index==b.source.body_index&&a.source.thread==b.source.thread&&pose::header_equal(a.bodies,b.bodies)&&
        a.members==b.members&&a.body==b.body&&a.actor==b.actor&&a.actor_table==b.actor_table&&a.scene==b.scene&&
        a.scene_table==b.scene_table&&a.public_scene==b.public_scene&&a.public_table==b.public_table;
}
inline bool pointer(std::uintptr_t p,std::size_t bytes) noexcept{return !(p&7)&&pose::span(p,bytes);}
template<class Read,class Hash> bool code_current(std::uintptr_t base,std::uint32_t image_size,Read read,Hash hash) {
    if(!base||base&0xffff||!image_size||image_size>UINTPTR_MAX-base)return false;
    std::array<unsigned char,1024> scratch{};
    for(auto const& c:gears_cover_physx_query_code::chunks){
        if(!c.size||c.size>scratch.size()||c.rva>=image_size||c.size>image_size-c.rva)return false;
        Digest actual{};
        if(!read(scratch.data(),reinterpret_cast<void const*>(base+c.rva),c.size)||
           !hash(scratch.data(),c.size,actual)||actual!=c.sha256)return false;
    }
    return true;
}
template<class Read,class Guard> Code capture(std::uintptr_t base,std::uint32_t image_size,
    Source const& source,Read read,Guard guard,Snapshot& output) {
    if(aliases(&source,sizeof source,&output,sizeof output))return Code::request;
    const Source input=source;
    if(!input.thread||!pointer(input.mesh,0x258)||!pointer(input.asset,8)||!pointer(input.instance,0x7c)||
       !base||(base&0xffff)||image_size<0x23cdd8||image_size>UINTPTR_MAX-base)return Code::request;
    if(!guard())return Code::guard;
    Snapshot next{};next.source=input;std::uintptr_t asset{},instance{};
    if(!pose::field(read,asset,input.mesh,0x248)||!pose::field(read,instance,input.mesh,0x250))return Code::read;
    if(asset!=input.asset||instance!=input.instance)return Code::changed;
    if(!pose::field(read,next.bodies,instance,0x6c))return Code::read;
    auto const& h=next.bodies;
    if(h.count<=0||h.count>64||h.capacity<h.count||h.capacity>256||input.body_index>=unsigned(h.count)||
       !pointer(h.data,std::size_t(h.capacity)*8))return Code::layout;
    if(!pose::bytes(read,next.members.data(),h.data,std::size_t(h.count)*8))return Code::read;
    for(int i=0;i<h.count;++i){if(!pointer(next.members[unsigned(i)],0x90))return Code::layout;
        for(int j=0;j<i;++j)if(next.members[unsigned(i)]==next.members[unsigned(j)])return Code::layout;}
    next.body=next.members[input.body_index];
    if(!pose::field(read,next.actor,next.body,0x88)||!pointer(next.actor,0x40)||
       !pose::field(read,next.actor_table,next.actor,0)||!pose::field(read,next.scene,next.actor,0x38))return Code::read;
    if(next.actor_table!=base+0x2330a8||!pointer(next.scene,0x278))return Code::module;
    if(!pose::field(read,next.scene_table,next.scene,0)||!pose::field(read,next.public_scene,next.scene,0x270))return Code::read;
    if(next.scene_table!=base+0x22ccb8||!pointer(next.public_scene,0x6b8))return Code::module;
    std::uintptr_t reverse{};
    if(!pose::field(read,next.public_table,next.public_scene,0)||!pose::field(read,reverse,next.public_scene,0x6b0))return Code::read;
    if(next.public_table!=base+0x23ca98||reverse!=next.scene)return Code::module;
    constexpr std::array<std::array<std::uint32_t,3>,10> slots{{
        {{0x2330a8,8,0x135b00}},{{0x22ccb8,8,0xa0300}},
        {{0x23ca98,0x308,0x171770}},{{0x23ca98,0x310,0x1716f0}},
        {{0x23ca98,0x330,0x168350}},{{0x23ca98,0x338,0x1682d0}},
        {{0x22ccb8,0x288,0xa6e30}},{{0x22ccb8,0x290,0xa6aa0}},
        {{0x22ccb8,0x268,0xa6290}},{{0x22ccb8,0x270,0xa60b0}}}};
    for(auto const& s:slots){std::uintptr_t value{};if(s[0]>=image_size||s[1]+8>image_size-s[0]||s[2]>=image_size)return Code::module;
        if(!pose::field(read,value,base+s[0],s[1]))return Code::read;
        if(value!=base+s[2])return Code::module;}
    if(!guard())return Code::guard;
    if(!same(input,source))return Code::changed;
    output=next;return Code::current;
}
} // namespace gears_cover_physx_query_source
