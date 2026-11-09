#pragma once
#include "cover_physx_query_source.h"
#include "cover_physx_stream.h"

// Current read-only ownership observation for the selected original DLL.
// The caller must retain its exact primed/pinned Image and admitted C0 owner.
// This reads original getter fields; it NEVER calls a shape/actor method.
// It is not a hit lifetime/world fence, complete self set, filter or movement
// permission. Unknown shapes fail explicitly rather than become non-self.
namespace gears_cover_physx_shape_owner {
namespace source=gears_cover_physx_query_source;
namespace raw=source::pose;
using Hit=gears_cover_physx_query::Hit;
enum class Kind:unsigned {box,capsule,convex,heightfield,plane,sphere,triangle_mesh,wheel,unknown};
inline constexpr std::array<std::uint32_t,8> shape_tables{{
    0x2353b0,0x2358f0,0x2361d0,0x238ac0,0x23ab60,0x23ea88,0x23fae0,0x2401b0}};
enum class Code {observed,request,guard,source_changed,read,unsupported,module,links,changed};
struct Observation {
    Kind kind{Kind::unknown};
    std::uintptr_t shape{},shape_table{},actor{},actor_table{},scene{},
        internal_shape{},internal_shape_table{},internal_actor{};
};
inline bool same(Observation const& a,Observation const& b)noexcept{
    return a.kind==b.kind&&a.shape==b.shape&&a.shape_table==b.shape_table&&a.actor==b.actor&&
        a.actor_table==b.actor_table&&a.scene==b.scene&&a.internal_shape==b.internal_shape&&
        a.internal_shape_table==b.internal_shape_table&&a.internal_actor==b.internal_actor;
}
namespace detail {
inline bool in_image(std::uintptr_t image_base,std::uint32_t image_bytes,std::uintptr_t address,std::size_t bytes)noexcept{
    return image_base&&image_bytes<=UINTPTR_MAX-image_base&&address>=image_base&&address-image_base<image_bytes&&bytes<=image_bytes-(address-image_base);
}
template<class Read> Code sample(std::uintptr_t image_base,std::uint32_t image_size,
    std::uintptr_t shape,std::uintptr_t scene,Read& read,Observation& output){
    Observation next{};next.shape=shape;
    if(!source::pointer(shape,0x40))return Code::request;
    if(!raw::field(read,next.shape_table,shape,0))return Code::read;
    for(unsigned i=0;i<shape_tables.size();++i)
        if(next.shape_table==image_base+shape_tables[i]){next.kind=Kind(i);break;}
    if(next.kind==Kind::unknown)return Code::unsupported;
    std::uintptr_t method{};
    if(!in_image(image_base,image_size,next.shape_table,16)||!raw::field(read,method,next.shape_table,8))return Code::read;
    if(method!=image_base+0x13e730)return Code::module;
    // All eight primary shape tables share MOV RAX,[RCX+38]; RET.
    if(!raw::field(read,next.actor,shape,0x38)||!source::pointer(next.actor,0x48)||
       !raw::field(read,next.actor_table,next.actor,0)||!raw::field(read,next.scene,next.actor,0x38))return Code::read;
    if(next.actor_table!=image_base+0x2330a8||next.scene!=scene)return Code::links;
    if(!raw::field(read,next.internal_shape,shape,0x30)||!source::pointer(next.internal_shape,0xd8)||
       !raw::field(read,next.internal_shape_table,next.internal_shape,0))return Code::read;
    if(!in_image(image_base,image_size,next.internal_shape_table,0x48)||
       !raw::field(read,method,next.internal_shape_table,0x40))return Code::read;
    if(method!=image_base+0xc4e0)return Code::module;
    std::uintptr_t public_shape{},shape_actor{},public_actor{};
    if(!raw::field(read,public_shape,next.internal_shape,0xa0)||
       !raw::field(read,shape_actor,next.internal_shape,0xd0)||
       !raw::field(read,next.internal_actor,next.actor,0x40)||!source::pointer(next.internal_actor,0x18)||
       !raw::field(read,public_actor,next.internal_actor,0x10))return Code::read;
    if(public_shape!=shape||shape_actor!=next.internal_actor||public_actor!=next.actor)return Code::links;
    output=next;return Code::observed;
}
template<class Read> bool code_current(std::uintptr_t image_base,Read& read){
    constexpr std::array<unsigned char,5> actor_getter{{0x48,0x8b,0x41,0x38,0xc3}};
    constexpr std::array<unsigned char,8> shape_getter{{0x48,0x8b,0x81,0xa0,0,0,0,0xc3}};
    std::array<unsigned char,5> a{};std::array<unsigned char,8> b{};
    return raw::bytes(read,a.data(),image_base+0x13e730,a.size())&&a==actor_getter&&
        raw::bytes(read,b.data(),image_base+0xc4e0,b.size())&&b==shape_getter;
}
}
template<class Read,class Guard> Code observe(std::uintptr_t image_base,std::uint32_t image_size,
    source::Snapshot const& scope,Hit const& hit,Read read,Guard guard,Observation& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard&,source::Snapshot const&>);
    if(!image_base||(image_base&0xffff)||image_size<0x2401c0||image_size>UINTPTR_MAX-image_base||
       scope.source.thread!=GetCurrentThreadId()||
       source::aliases(&output,sizeof output,&scope,sizeof scope)||source::aliases(&output,sizeof output,&hit,sizeof hit))return Code::request;
    const auto key=scope;const auto packet=hit;std::uintptr_t shape{};
    // Original A71F0 stores the public shape at Hit+8 after virtual+40.
    std::memcpy(&shape,packet.bytes.data()+8,sizeof shape);
    if(!source::pointer(shape,0x40))return Code::request;
    try{
        auto current_owner=[&](){return guard(key);};
        if(!current_owner())return Code::guard;
        source::Snapshot before{},after{};
        if(source::capture(image_base,image_size,key.source,read,current_owner,before)!=source::Code::current||
           !source::same(key,before))return Code::source_changed;
        if(!detail::code_current(image_base,read))return Code::module;
        Observation first{},second{};
        const auto result=detail::sample(image_base,image_size,shape,key.scene,read,first);if(result!=Code::observed)return result;
        if(source::capture(image_base,image_size,key.source,read,current_owner,after)!=source::Code::current||
           !source::same(key,after)||!source::same(key,scope)||std::memcmp(&packet,&hit,sizeof packet))return Code::source_changed;
        const auto closing=detail::sample(image_base,image_size,shape,key.scene,read,second);if(closing!=Code::observed)return closing;
        if(!same(first,second))return Code::changed;
        if(!detail::code_current(image_base,read))return Code::module;
        if(!current_owner())return Code::guard;
        if(!source::same(key,scope)||std::memcmp(&packet,&hit,sizeof packet))return Code::source_changed;
        output=first;return Code::observed;
    }catch(...){return Code::read;}
}
}
