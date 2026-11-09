#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <type_traits>
#include "cover_physx_query_source.h"

// Exact Image pin/current C0 and typed full-identity/metadata guard required.
// This qualifies only the supplied DECLARED component/body fields. Owners,
// foreign rows and duplicates are retained, never interpreted as a self filter.
// Extra skeletal PAA bodies, attachments, lifetime, ABA/world fencing and
// complete shape coverage remain separate. No result is movement permission.
namespace gears_cover_physx_components {
namespace source=gears_cover_physx_query_source;
namespace raw=source::pose;
constexpr unsigned root_capacity=2,list_capacity=64,capacity=128;
enum class State:unsigned {not_sampled,nonprimitive,body_instance_field_null,body_data_field_null,body_data_nonnull};
struct Root {
    std::uintptr_t actor{};std::array<unsigned,2> offsets{};
    std::array<raw::ArrayHeader,2> lists{};
    std::array<std::array<std::uintptr_t,list_capacity>,2> members{};
};
struct Component {
    std::uintptr_t component{},owner{},body{},body_owner{},body_owner_actor{};
    unsigned owner_roots{},body_owner_roots{},body_instance_offset{};
    bool primitive{},skeletal{};State state{};std::int32_t body_index{};
    std::uintptr_t body_data{},bone_spring{};
};
struct Source {
    unsigned root_count{},component_count{};bool pawn_mesh_listed{};
    std::array<Root,root_capacity> roots{};std::array<Component,capacity> components{};
};
inline bool same(Root const& a,Root const& b)noexcept{
    if(a.actor!=b.actor||a.offsets!=b.offsets||a.members!=b.members)return false;
    for(unsigned n=0;n<2;++n)if(!raw::header_equal(a.lists[n],b.lists[n]))return false;return true;
}
inline bool same(Component const& a,Component const& b)noexcept{return a.component==b.component&&a.owner==b.owner&&a.body==b.body&&
    a.body_owner==b.body_owner&&a.body_owner_actor==b.body_owner_actor&&a.owner_roots==b.owner_roots&&a.body_owner_roots==b.body_owner_roots&&
    a.body_instance_offset==b.body_instance_offset&&a.primitive==b.primitive&&a.skeletal==b.skeletal&&a.state==b.state&&
    a.body_index==b.body_index&&a.body_data==b.body_data&&a.bone_spring==b.bone_spring;}
inline bool same(Source const& a,Source const& b)noexcept{
    if(a.root_count!=b.root_count||a.component_count!=b.component_count||a.pawn_mesh_listed!=b.pawn_mesh_listed)return false;
    for(unsigned n=0;n<root_capacity;++n)if(!same(a.roots[n],b.roots[n]))return false;
    for(unsigned n=0;n<capacity;++n)if(!same(a.components[n],b.components[n]))return false;return true;
}
enum class Presence:unsigned {not_sampled,nonprimitive,body_instance_field_null,body_data_field_null,actor_observed};
struct Actor {Presence presence{};std::uintptr_t table{},scene{},internal{};};
struct Snapshot {
    source::Snapshot query_source{};Source fields{};
    std::array<Actor,capacity> observations{};unsigned actor_count{};
    std::array<std::uintptr_t,capacity> actors{};
};
inline bool same(Snapshot const& a,Snapshot const& b)noexcept{
    if(!source::same(a.query_source,b.query_source)||!same(a.fields,b.fields)||a.actor_count!=b.actor_count||a.actors!=b.actors)return false;
    for(unsigned n=0;n<capacity;++n){auto const& x=a.observations[n];auto const& y=b.observations[n];
        if(x.presence!=y.presence||x.table!=y.table||x.scene!=y.scene||x.internal!=y.internal)return false;}return true;
}
struct Workspace {Source key{};Snapshot first{},last{};std::atomic<bool> active{};};
enum class Code {observed,request,guard,source_changed,read,unsupported,links,changed};
inline unsigned owner_mask(Source const& fields,std::uintptr_t address)noexcept{
    unsigned mask{};for(unsigned n=0;n<fields.root_count;++n)if(fields.roots[n].actor==address)mask|=1u<<n;return mask;
}
inline bool mesh_listed(Source const& fields,std::uintptr_t mesh)noexcept{
    for(unsigned l=0;l<2;++l)for(unsigned n=0;n<list_capacity;++n)if(fields.roots[0].members[l][n]==mesh)return true;return false;
}
inline bool valid(Source const& fields)noexcept{
    if(!fields.root_count||fields.root_count>root_capacity||fields.component_count>capacity)return false;
    unsigned unique{};
    for(unsigned r=0;r<root_capacity;++r){auto const& root=fields.roots[r];
        if(r>=fields.root_count){if(!same(root,Root{}))return false;continue;}
        if(!source::pointer(root.actor,88)||(r&&root.actor==fields.roots[0].actor))return false;
        const auto a=root.offsets[0],b=root.offsets[1];
        if(a<0x60||b<0x60||a>65520||b>65520||(a<=b?b-a<16:a-b<16))return false;
        for(unsigned l=0;l<2;++l){auto const& h=root.lists[l];
            if(h.count<0||h.count>int(list_capacity)||h.capacity<h.count||h.capacity>256||
               (!h.data?(h.count||h.capacity):!source::pointer(h.data,std::size_t(h.capacity?h.capacity:1)*8)))return false;
            for(unsigned k=0;k<list_capacity;++k){const auto at=root.members[l][k];
                if(k>=unsigned(h.count)){if(at)return false;continue;}
                if(!at)continue;if(!source::pointer(at,88))return false;
                bool duplicate=false;for(unsigned n=0;n<unique;++n)duplicate=duplicate||fields.components[n].component==at;
                if(!duplicate){if(unique>=fields.component_count||fields.components[unique].component!=at)return false;++unique;}
            }}
    }
    if(unique!=fields.component_count)return false;
    for(unsigned n=0;n<capacity;++n){auto const& row=fields.components[n];
        if(n>=unique){if(!same(row,Component{}))return false;continue;}
        if(!source::pointer(row.owner,88)||row.owner_roots!=owner_mask(fields,row.owner)||(row.skeletal&&!row.primitive))return false;
        if(row.state==State::nonprimitive){if(row.primitive||row.body_instance_offset||row.body||row.body_owner||row.body_owner_actor||
            row.body_owner_roots||row.body_index||row.body_data||row.bone_spring)return false;continue;}
        if(!row.primitive||row.body_instance_offset<0x60||row.body_instance_offset>65528)return false;
        if(row.state==State::body_instance_field_null){if(row.body||row.body_owner||row.body_owner_actor||row.body_owner_roots||row.body_index||row.body_data||row.bone_spring)return false;continue;}
        if(row.state!=State::body_data_field_null&&row.state!=State::body_data_nonnull)return false;
        if(!source::pointer(row.body,0x98)||!source::pointer(row.body_owner,88)||!source::pointer(row.body_owner_actor,88)||
           row.body_owner_roots!=owner_mask(fields,row.body_owner_actor)||
           (row.bone_spring&&!source::pointer(row.bone_spring,8)))return false;
        if(row.state==State::body_data_field_null){if(row.body_data)return false;}
        else if(!source::pointer(row.body_data,0x48))return false;
    }return true;
}
inline bool consistent(Snapshot const& packet)noexcept{
    if(!valid(packet.fields)||packet.actor_count>capacity||packet.fields.pawn_mesh_listed!=mesh_listed(packet.fields,packet.query_source.source.mesh))return false;
    unsigned count{};std::array<std::uintptr_t,capacity> actors{};
    for(unsigned n=0;n<capacity;++n){auto const& row=packet.fields.components[n];auto const& value=packet.observations[n];
        const auto expected=n>=packet.fields.component_count?Presence::not_sampled:row.state==State::nonprimitive?Presence::nonprimitive:
            row.state==State::body_instance_field_null?Presence::body_instance_field_null:row.state==State::body_data_field_null?Presence::body_data_field_null:Presence::actor_observed;
        if(value.presence!=expected)return false;
        if(expected!=Presence::actor_observed){if(value.table||value.scene||value.internal)return false;continue;}
        if(value.table!=packet.query_source.actor_table||value.scene!=packet.query_source.scene||!source::pointer(value.internal,0x18))return false;
        bool duplicate=false;for(unsigned k=0;k<count;++k)duplicate=duplicate||actors[k]==row.body_data;
        if(!duplicate)actors[count++]=row.body_data;
    }return count==packet.actor_count&&actors==packet.actors;
}
namespace detail {
template<class Read> Code sample(std::uintptr_t image_base,source::Snapshot const& scope,Source const& fields,Read& read,Snapshot& output){
    output.query_source=scope;output.fields=fields;output.actor_count=0;output.actors.fill(0);
    for(auto& value:output.observations)value={};
    for(unsigned r=0;r<fields.root_count;++r)for(unsigned l=0;l<2;++l){auto const& root=fields.roots[r];raw::ArrayHeader header{};
        if(!raw::field(read,header,root.actor,root.offsets[l]))return Code::read;
        if(!raw::header_equal(header,root.lists[l]))return Code::changed;
        std::array<std::uintptr_t,list_capacity> members{};
        if(header.count&&!raw::bytes(read,members.data(),header.data,std::size_t(header.count)*8))return Code::read;
        if(members!=root.members[l])return Code::changed;}
    for(unsigned n=0;n<fields.component_count;++n){auto const& row=fields.components[n];auto& value=output.observations[n];
        if(row.state==State::nonprimitive){value.presence=Presence::nonprimitive;continue;}
        std::uintptr_t body{},data{},spring{};
        if(!raw::field(read,body,row.component,row.body_instance_offset))return Code::read;
        if(body!=row.body)return Code::changed;
        if(!body){value.presence=Presence::body_instance_field_null;continue;}
        if(!raw::field(read,data,body,0x88)||!raw::field(read,spring,body,0x90))return Code::read;
        if(data!=row.body_data||spring!=row.bone_spring)return Code::changed;
        if(!data){value.presence=Presence::body_data_field_null;continue;}
        if(!raw::field(read,value.table,data,0)||!raw::field(read,value.scene,data,0x38)||!raw::field(read,value.internal,data,0x40))return Code::read;
        if(value.table!=image_base+0x2330a8)return Code::unsupported;
        if(value.scene!=scope.scene||!source::pointer(value.internal,0x18))return Code::links;
        std::uintptr_t reverse{};if(!raw::field(read,reverse,value.internal,0x10))return Code::read;
        if(reverse!=data)return Code::links;value.presence=Presence::actor_observed;
        bool duplicate=false;for(unsigned k=0;k<output.actor_count;++k)duplicate=duplicate||output.actors[k]==data;
        if(!duplicate)output.actors[output.actor_count++]=data;
    }return Code::observed;
}
}
template<class Read,class Guard> Code observe(std::uintptr_t image_base,std::uint32_t image_size,source::Snapshot const& scope,
    Source const& input,Read read,Guard guard,Workspace& work,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard&>&&std::is_invocable_r_v<bool,Guard&,Snapshot const&>);
    if(!image_base||(image_base&0xffff)||image_size<0x23cdd8||image_size>UINTPTR_MAX-image_base||scope.source.thread!=GetCurrentThreadId()||!valid(input)||input.pawn_mesh_listed!=mesh_listed(input,scope.source.mesh)||
       source::aliases(&work,sizeof work,&input,sizeof input)||source::aliases(&work,sizeof work,&scope,sizeof scope)||source::aliases(&work,sizeof work,&output,sizeof output)||
       source::aliases(&output,sizeof output,&scope,sizeof scope)||source::aliases(&output,sizeof output,&input,sizeof input))return Code::request;
    bool expected=false;if(!work.active.compare_exchange_strong(expected,true))return Code::request;
    struct Finish{std::atomic<bool>& active;~Finish(){active=false;}} finish{work.active};work.key=input;const auto key=scope;
    try{
        if(!guard())return Code::guard;auto owner=[&](){return guard();};source::Snapshot before{},after{};
        auto closed=source::capture(image_base,image_size,key.source,read,owner,before);
        if(closed==source::Code::guard)return Code::guard;if(closed!=source::Code::current||!source::same(key,before))return Code::source_changed;
        auto code=detail::sample(image_base,key,work.key,read,work.first);if(code!=Code::observed)return code;
        if(!guard(work.first))return Code::guard;
        closed=source::capture(image_base,image_size,key.source,read,owner,after);
        if(closed==source::Code::guard)return Code::guard;if(closed!=source::Code::current||!source::same(key,after))return Code::source_changed;
        code=detail::sample(image_base,key,work.key,read,work.last);if(code!=Code::observed)return code;
        if(!same(work.first,work.last))return Code::changed;
        if(!guard(work.last)||!guard())return Code::guard;
        if(!source::same(key,scope)||!same(work.key,input))return Code::source_changed;
        output=work.first;return Code::observed;
    }catch(...){return Code::read;}
}
}
