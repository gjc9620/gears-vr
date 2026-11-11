#pragma once
#include "cover_physx_query_source.h"
#include "cover_physx_stream.h"
#include "cover_physx_actor_shapes.h"
#include "cover_physx_actor_shape_code.h"

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
inline constexpr auto shape_tables=gears_cover_physx_shape_types::tables;
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
       !raw::field(read,next.internal_actor,next.actor,0x40))return Code::read;
    if(next.internal_actor&&!source::pointer(next.internal_actor,0x18))return Code::links;
    namespace back=gears_cover_physx_actor_backlink;
    const auto relation=back::sample(image_base,next.actor,read,public_actor);
    if(relation==back::Code::read)return Code::read;
    if(relation==back::Code::unsupported)return Code::module;
    if(relation!=back::Code::current)return Code::links;
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
        namespace inventory=gears_cover_physx_actor_shapes;
        inventory::Workspace inventory_work{};inventory::Snapshot members_before{},members_after{};
        auto certified=[&](){return current_owner()&&detail::code_current(image_base,read)&&gears_cover_physx_actor_shape_code::current(image_base,read);};
        auto inventory_result=inventory::observe(image_base,image_size,first.actor,key.scene,read,certified,inventory_work,members_before);
        if(inventory_result!=inventory::Code::observed)return inventory_result==inventory::Code::guard?Code::guard:inventory_result==inventory::Code::module?Code::module:Code::links;
        auto member=[&](inventory::Snapshot const& value){unsigned matches{};for(unsigned n=0;n<value.count;++n)
            if(value.rows[n].public_shape==shape&&value.rows[n].internal==first.internal_shape)++matches;
            return matches==1&&value.body==first.internal_actor;};
        if(!member(members_before))return Code::links;
        if(source::capture(image_base,image_size,key.source,read,current_owner,after)!=source::Code::current||
           !source::same(key,after)||!source::same(key,scope)||std::memcmp(&packet,&hit,sizeof packet))return Code::source_changed;
        const auto closing=detail::sample(image_base,image_size,shape,key.scene,read,second);if(closing!=Code::observed)return closing;
        if(!same(first,second))return Code::changed;
        inventory_result=inventory::observe(image_base,image_size,second.actor,key.scene,read,certified,inventory_work,members_after);
        if(inventory_result!=inventory::Code::observed||!inventory::same(members_before,members_after)||!member(members_after))return Code::changed;
        if(!detail::code_current(image_base,read))return Code::module;
        if(!current_owner())return Code::guard;
        if(!source::same(key,scope)||std::memcmp(&packet,&hit,sizeof packet))return Code::source_changed;
        output=first;return Code::observed;
    }catch(...){return Code::read;}
}
// The overlap enumerator returns a public shape address, not a sweep Hit.
// Reuse the exact membership/backlink observer with a private address carrier;
// no synthetic sweep packet is exposed as an original collision observation.
template<class Read,class Guard> Code observe_address(std::uintptr_t image_base,std::uint32_t image_size,
    source::Snapshot const& scope,std::uintptr_t const& shape_address,Read read,Guard guard,Observation& output)noexcept{
    if(source::aliases(&output,sizeof output,&shape_address,sizeof shape_address))return Code::request;
    const auto captured_address=shape_address;Hit address_carrier{};
    std::memcpy(address_carrier.bytes.data()+8,&captured_address,sizeof captured_address);
    auto stable=[&](source::Snapshot const& current_scope){
        if(shape_address!=captured_address)return false;
        const bool owner_is_current=guard(current_scope);return owner_is_current&&shape_address==captured_address;
    };
    return observe(image_base,image_size,scope,address_carrier,read,stable,output);
}
}
