#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <type_traits>
#include "cover_physx_query_source.h"

// A read bracket of the named PhysicsAssetInstance.Bodies -> BodyData fields.
// The typed guard must certify EVERY reflected body/owner/index/BodyData role.
// BoneSpring, the Pawn collider, component BodyInstance, attachments and other
// actor sources are not inferred from this one array. This is NOT a complete
// player self set, an actor lifetime/fence, a filter or a movement permission.
namespace gears_cover_physx_asset_actors {
namespace source=gears_cover_physx_query_source;
namespace raw=source::pose;
constexpr unsigned capacity=64;
enum class Presence:unsigned {not_sampled,body_data_field_null,actor_observed};
struct Body {
    Presence presence{};
    std::uintptr_t body{},actor{},table{},scene{},internal{};
};
struct Snapshot {
    source::Snapshot query_source{};
    unsigned body_count{},actor_count{};
    std::array<Body,capacity> bodies{};
    // First-occurrence order. Duplicate references keep separate Body rows.
    std::array<std::uintptr_t,capacity> actors{};
};
struct Workspace {Snapshot first{},last{};};
enum class Code {observed,request,guard,source_changed,read,unsupported,links,changed};
inline bool same(Snapshot const& a,Snapshot const& b)noexcept{
    if(!source::same(a.query_source,b.query_source)||a.body_count!=b.body_count||a.actor_count!=b.actor_count||a.actors!=b.actors)return false;
    for(unsigned i=0;i<capacity;++i){auto const& x=a.bodies[i];auto const& y=b.bodies[i];
        if(x.presence!=y.presence||x.body!=y.body||x.actor!=y.actor||x.table!=y.table||x.scene!=y.scene||x.internal!=y.internal)return false;}
    return true;
}
namespace detail {
template<class Read> Code sample(std::uintptr_t image_base,source::Snapshot const& scope,Read& read,Snapshot& out){
    out={};out.query_source=scope;
    if(scope.bodies.count<=0||scope.bodies.count>int(capacity))return Code::request;
    out.body_count=unsigned(scope.bodies.count);
    for(unsigned i=0;i<out.body_count;++i){auto& body=out.bodies[i];body.body=scope.members[i];
        if(!source::pointer(body.body,0x90)||!raw::field(read,body.actor,body.body,0x88))return Code::read;
        if(!body.actor){body.presence=Presence::body_data_field_null;continue;}
        if(!source::pointer(body.actor,0x48)||!raw::field(read,body.table,body.actor,0)||
           !raw::field(read,body.scene,body.actor,0x38)||!raw::field(read,body.internal,body.actor,0x40))return Code::read;
        if(body.table!=image_base+0x2330a8)return Code::unsupported;
        if(body.scene!=scope.scene||!source::pointer(body.internal,0x18))return Code::links;
        std::uintptr_t actor_back{};
        if(!raw::field(read,actor_back,body.internal,0x10))return Code::read;
        if(actor_back!=body.actor)return Code::links;
        body.presence=Presence::actor_observed;
        bool duplicate=false;for(unsigned n=0;n<out.actor_count;++n)duplicate=duplicate||out.actors[n]==body.actor;
        if(!duplicate)out.actors[out.actor_count++]=body.actor;
    }
    const auto root=scope.source.body_index;
    if(root>=out.body_count||out.bodies[root].presence!=Presence::actor_observed||out.bodies[root].actor!=scope.actor)return Code::source_changed;
    return Code::observed;
}
}
template<class Read,class Guard> Code observe(std::uintptr_t image_base,std::uint32_t image_size,
    source::Snapshot const& scope,Read read,Guard guard,Workspace& work,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard&>&&std::is_invocable_r_v<bool,Guard&,Snapshot const&>);
    if(!image_base||(image_base&0xffff)||image_size<0x23cdd8||image_size>UINTPTR_MAX-image_base||
       scope.source.thread!=GetCurrentThreadId()||
       source::aliases(&work,sizeof work,&output,sizeof output)||source::aliases(&work,sizeof work,&scope,sizeof scope)||
       source::aliases(&output,sizeof output,&scope,sizeof scope))return Code::request;
    const auto key=scope;
    try{
        if(!guard())return Code::guard;
        source::Snapshot before{},after{};
        auto owner=[&](){return guard();};
        auto closed=source::capture(image_base,image_size,key.source,read,owner,before);
        if(closed==source::Code::guard)return Code::guard;
        if(closed!=source::Code::current||!source::same(key,before))return Code::source_changed;
        auto code=detail::sample(image_base,key,read,work.first);if(code!=Code::observed)return code;
        if(!guard(work.first))return Code::guard;
        closed=source::capture(image_base,image_size,key.source,read,owner,after);
        if(closed==source::Code::guard)return Code::guard;
        if(closed!=source::Code::current||!source::same(key,after))return Code::source_changed;
        code=detail::sample(image_base,key,read,work.last);if(code!=Code::observed)return code;
        if(!same(work.first,work.last))return Code::changed;
        if(!guard(work.last)||!guard())return Code::guard;
        if(!source::same(key,scope))return Code::source_changed;
        output=work.first;return Code::observed;
    }catch(...){return Code::read;}
}
}
