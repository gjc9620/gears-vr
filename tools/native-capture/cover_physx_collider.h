#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <type_traits>
#include "cover_physx_query_source.h"
#include "cover_owner_link_observation.h"

// Caller owns the exact Image pin and current C0. Its typed guard certifies
// these named Pawn/component/body fields and their full reflected identities.
// Null means only this field is null; neither result is a complete self set,
// collider shape/lifetime/world fence, self filter or movement permission.
namespace gears_cover_physx_collider {
namespace source=gears_cover_physx_query_source;
namespace raw=source::pose;
namespace linkobs=gears_cover_owner_link_observation;
struct Source {
    std::uintptr_t pawn{},component{},body{};
    unsigned body_instance_offset{};
    std::int32_t body_index{};
    std::uintptr_t body_data{},bone_spring{};
};
inline bool same(Source const& a,Source const& b)noexcept{return a.pawn==b.pawn&&a.component==b.component&&a.body==b.body&&
    a.body_instance_offset==b.body_instance_offset&&a.body_index==b.body_index&&a.body_data==b.body_data&&a.bone_spring==b.bone_spring;}
enum class Presence:unsigned {not_sampled,body_instance_field_null,body_data_field_null,actor_observed};
struct Snapshot {
    source::Snapshot query_source{};
    Source fields{};Presence presence{};
    std::uintptr_t actor_table{},scene{},internal{};
};
inline bool same(Snapshot const& a,Snapshot const& b)noexcept{return source::same(a.query_source,b.query_source)&&same(a.fields,b.fields)&&
    a.presence==b.presence&&a.actor_table==b.actor_table&&a.scene==b.scene&&a.internal==b.internal;}
enum class Code {observed,request,guard,source_changed,read,unsupported,links,changed};
namespace detail {
inline bool valid(Source const& input)noexcept{return source::pointer(input.pawn,88)&&source::pointer(input.component,88)&&
    input.body_instance_offset>=0x60&&input.body_instance_offset<=65528&&
    (input.body?source::pointer(input.body,0x98):(!input.body_index&&!input.body_data&&!input.bone_spring))&&
    (!input.body_data||source::pointer(input.body_data,0x48))&&(!input.bone_spring||source::pointer(input.bone_spring,8));}
template<class Read> Code sample(std::uintptr_t image_base,source::Snapshot const& scope,Source const& input,Read& read,Snapshot& output){
    Snapshot next{};next.query_source=scope;next.fields=input;std::uintptr_t body{},data{},spring{};
    if(!raw::field(read,body,input.component,input.body_instance_offset))return Code::read;
    if(body!=input.body)return Code::changed;
    if(!body){next.presence=Presence::body_instance_field_null;output=next;return Code::observed;}
    if(!raw::field(read,data,body,0x88)||!raw::field(read,spring,body,0x90))return Code::read;
    if(data!=input.body_data||spring!=input.bone_spring)return Code::changed;
    if(!data){next.presence=Presence::body_data_field_null;output=next;return Code::observed;}
    if(!raw::field(read,next.actor_table,data,0)||!raw::field(read,next.scene,data,0x38)||!raw::field(read,next.internal,data,0x40))return Code::read;
    if(next.actor_table!=image_base+0x2330a8)return Code::unsupported;
    if(next.scene!=scope.scene||!source::pointer(next.internal,0x18)){
        linkobs::note({false,linkobs::Reader::collider,{},linkobs::Branch::scene_or_internal,scope.source.thread,
            data,next.actor_table,scope.scene,next.scene,next.internal,0,false});return Code::links;}
    std::uintptr_t reverse{};
    if(!raw::field(read,reverse,next.internal,0x10))return Code::read;
    if(reverse!=data){
        linkobs::note({false,linkobs::Reader::collider,{},linkobs::Branch::reverse,scope.source.thread,
            data,next.actor_table,scope.scene,next.scene,next.internal,reverse,true});return Code::links;}
    next.presence=Presence::actor_observed;output=next;return Code::observed;
}
}
template<class Read,class Guard> Code observe(std::uintptr_t image_base,std::uint32_t image_size,source::Snapshot const& scope,
    Source const& input,Read read,Guard guard,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard&>&&std::is_invocable_r_v<bool,Guard&,Snapshot const&>);
    if(!image_base||(image_base&0xffff)||image_size<0x23cdd8||image_size>UINTPTR_MAX-image_base||
       scope.source.thread!=GetCurrentThreadId()||!detail::valid(input)||
       source::aliases(&output,sizeof output,&scope,sizeof scope)||source::aliases(&output,sizeof output,&input,sizeof input))return Code::request;
    const auto key=scope;const auto fields=input;
    try{
        if(!guard())return Code::guard;
        source::Snapshot before{},after{};auto owner=[&](){return guard();};
        auto closed=source::capture(image_base,image_size,key.source,read,owner,before);
        if(closed==source::Code::guard)return Code::guard;
        if(closed!=source::Code::current||!source::same(before,key))return Code::source_changed;
        Snapshot first{},last{};auto code=detail::sample(image_base,key,fields,read,first);if(code!=Code::observed)return code;
        if(!guard(first))return Code::guard;
        closed=source::capture(image_base,image_size,key.source,read,owner,after);
        if(closed==source::Code::guard)return Code::guard;
        if(closed!=source::Code::current||!source::same(after,key))return Code::source_changed;
        code=detail::sample(image_base,key,fields,read,last);if(code!=Code::observed)return code;
        if(!same(first,last))return Code::changed;
        if(!guard(last)||!guard())return Code::guard;
        if(!source::same(key,scope)||!same(fields,input))return Code::source_changed;
        output=first;return Code::observed;
    }catch(...){return Code::read;}
}
}
