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
// Optional skeletal PAA rows are qualified on this same Scene as well.
// Actor.Attached edges and Base fields close the supplied recursive actor
// catalog. Skeletal attachment structure rows retain their raw source bytes.
// Lifetime, ABA/world fencing and complete shape coverage remain
// separate. No result is movement permission or a complete self filter.
namespace gears_cover_physx_components {
namespace source=gears_cover_physx_query_source;
namespace raw=source::pose;
constexpr unsigned root_capacity=16,list_capacity=64,capacity=128,asset_body_capacity=256,actor_capacity=capacity+asset_body_capacity;
constexpr unsigned attachment_capacity=256,attachment_stride_capacity=128;
enum class State:unsigned {not_sampled,nonprimitive,body_instance_field_null,body_data_field_null,body_data_nonnull};
struct Root {
    std::uintptr_t actor{};std::array<unsigned,2> offsets{};
    std::array<raw::ArrayHeader,2> lists{};
    std::array<std::array<std::uintptr_t,list_capacity>,2> members{};
    std::uintptr_t base{};unsigned base_offset{},attached_offset{};raw::ArrayHeader attached{};
    std::array<std::uintptr_t,list_capacity> attached_members{};
};
struct Component {
    std::uintptr_t component{},owner{},body{},body_owner{},body_owner_actor{};
    unsigned owner_roots{},body_owner_roots{},body_instance_offset{};
    bool primitive{},skeletal{};State state{};std::int32_t body_index{};
    std::uintptr_t body_data{},bone_spring{};
};
struct AssetScope {
    bool sampled{};std::uintptr_t asset{},instance{},owner{};
    std::array<unsigned,3> offsets{};raw::ArrayHeader array{};std::int32_t root{};unsigned first_body{};
};
struct AssetBody {
    std::uintptr_t body{},owner{},owner_actor{};unsigned owner_roots{};std::int32_t index{};
    std::uintptr_t data{},spring{};
};
struct AttachmentScope {
    bool sampled{};std::uintptr_t structure{};unsigned offset{},stride{},component_offset{},first_row{};raw::ArrayHeader array{};
};
struct AttachmentRow {std::uintptr_t component{};std::array<unsigned char,attachment_stride_capacity> bytes{};};
struct Source {
    unsigned seed_count{},root_count{},component_count{},asset_body_count{},attachment_count{};bool pawn_mesh_listed{};std::uintptr_t mesh_seed{};
    std::array<Root,root_capacity> roots{};std::array<Component,capacity> components{};
    std::array<AssetScope,capacity> assets{};std::array<AssetBody,asset_body_capacity> asset_bodies{};
    std::array<AttachmentScope,capacity> attachments{};std::array<AttachmentRow,attachment_capacity> attachment_rows{};
};
inline bool same(Root const& a,Root const& b)noexcept{
    if(a.actor!=b.actor||a.offsets!=b.offsets||a.members!=b.members||a.base!=b.base||a.base_offset!=b.base_offset||
       a.attached_offset!=b.attached_offset||a.attached_members!=b.attached_members||!raw::header_equal(a.attached,b.attached))return false;
    for(unsigned n=0;n<2;++n)if(!raw::header_equal(a.lists[n],b.lists[n]))return false;return true;
}
inline bool same(Component const& a,Component const& b)noexcept{return a.component==b.component&&a.owner==b.owner&&a.body==b.body&&
    a.body_owner==b.body_owner&&a.body_owner_actor==b.body_owner_actor&&a.owner_roots==b.owner_roots&&a.body_owner_roots==b.body_owner_roots&&
    a.body_instance_offset==b.body_instance_offset&&a.primitive==b.primitive&&a.skeletal==b.skeletal&&a.state==b.state&&
    a.body_index==b.body_index&&a.body_data==b.body_data&&a.bone_spring==b.bone_spring;}
inline bool same(AssetScope const& a,AssetScope const& b)noexcept{return a.sampled==b.sampled&&a.asset==b.asset&&a.instance==b.instance&&
    a.owner==b.owner&&a.offsets==b.offsets&&raw::header_equal(a.array,b.array)&&a.root==b.root&&a.first_body==b.first_body;}
inline bool same(AssetBody const& a,AssetBody const& b)noexcept{return a.body==b.body&&a.owner==b.owner&&a.owner_actor==b.owner_actor&&
    a.owner_roots==b.owner_roots&&a.index==b.index&&a.data==b.data&&a.spring==b.spring;}
inline bool same(AttachmentScope const& a,AttachmentScope const& b)noexcept{return a.sampled==b.sampled&&a.structure==b.structure&&
    a.offset==b.offset&&a.stride==b.stride&&a.component_offset==b.component_offset&&a.first_row==b.first_row&&raw::header_equal(a.array,b.array);}
inline bool same(AttachmentRow const& a,AttachmentRow const& b)noexcept{return a.component==b.component&&a.bytes==b.bytes;}
inline bool same(Source const& a,Source const& b)noexcept{
    if(a.seed_count!=b.seed_count||a.root_count!=b.root_count||a.component_count!=b.component_count||a.asset_body_count!=b.asset_body_count||
       a.attachment_count!=b.attachment_count||a.pawn_mesh_listed!=b.pawn_mesh_listed||a.mesh_seed!=b.mesh_seed)return false;
    for(unsigned n=0;n<root_capacity;++n)if(!same(a.roots[n],b.roots[n]))return false;
    for(unsigned n=0;n<capacity;++n)if(!same(a.components[n],b.components[n])||!same(a.assets[n],b.assets[n])||!same(a.attachments[n],b.attachments[n]))return false;
    for(unsigned n=0;n<asset_body_capacity;++n)if(!same(a.asset_bodies[n],b.asset_bodies[n]))return false;
    for(unsigned n=0;n<attachment_capacity;++n)if(!same(a.attachment_rows[n],b.attachment_rows[n]))return false;return true;
}
enum class Presence:unsigned {not_sampled,nonprimitive,body_instance_field_null,body_data_field_null,actor_observed,body_field_null};
struct Actor {Presence presence{};std::uintptr_t table{},scene{},internal{};};
struct Snapshot {
    source::Snapshot query_source{};Source fields{};
    std::array<Actor,capacity> observations{};unsigned actor_count{};
    std::array<Actor,asset_body_capacity> asset_observations{};
    std::array<std::uintptr_t,actor_capacity> actors{};
};
inline bool same(Snapshot const& a,Snapshot const& b)noexcept{
    if(!source::same(a.query_source,b.query_source)||!same(a.fields,b.fields)||a.actor_count!=b.actor_count||a.actors!=b.actors)return false;
    for(unsigned n=0;n<capacity;++n){auto const& x=a.observations[n];auto const& y=b.observations[n];
        if(x.presence!=y.presence||x.table!=y.table||x.scene!=y.scene||x.internal!=y.internal)return false;}
    for(unsigned n=0;n<asset_body_capacity;++n){auto const& x=a.asset_observations[n];auto const& y=b.asset_observations[n];
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
    if(!fields.seed_count||fields.seed_count>2||fields.root_count<fields.seed_count||fields.root_count>root_capacity||
       fields.component_count>capacity||fields.asset_body_count>asset_body_capacity||fields.attachment_count>attachment_capacity)return false;
    unsigned unique{},discovered=fields.seed_count;
    for(unsigned r=0;r<root_capacity;++r){auto const& root=fields.roots[r];
        if(r>=fields.root_count){if(!same(root,Root{}))return false;continue;}
        if(r>=discovered||!source::pointer(root.actor,88)||(root.base&&!source::pointer(root.base,88)))return false;
        for(unsigned n=0;n<r;++n)if(root.actor==fields.roots[n].actor)return false;
        const std::array<unsigned,4> offsets{root.offsets[0],root.offsets[1],root.base_offset,root.attached_offset};
        for(unsigned a=0;a<4;++a){const unsigned width=a==2?8u:16u;if(offsets[a]<0x60||offsets[a]>65536-width)return false;
            for(unsigned b=0;b<a;++b){const unsigned other=b==2?8u:16u;
                if(offsets[a]<=offsets[b]?offsets[b]-offsets[a]<width:offsets[a]-offsets[b]<other)return false;}}
        auto const& attached=root.attached;
        if(attached.count<0||attached.count>int(list_capacity)||attached.capacity<attached.count||attached.capacity>256||
           (!attached.data?(attached.count||attached.capacity):!source::pointer(attached.data,std::size_t(attached.capacity?attached.capacity:1)*8)))return false;
        for(unsigned k=0;k<list_capacity;++k){const auto at=root.attached_members[k];
            if(k>=unsigned(attached.count)){if(at)return false;continue;}if(!at)continue;if(!source::pointer(at,88))return false;
            bool duplicate=false;for(unsigned n=0;n<discovered;++n)duplicate=duplicate||fields.roots[n].actor==at;
            if(!duplicate){if(discovered>=fields.root_count||fields.roots[discovered].actor!=at)return false;++discovered;}}
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
    if(discovered!=fields.root_count)return false;
    if(fields.mesh_seed){
        if(!source::pointer(fields.mesh_seed,88))return false;unsigned n{};
        for(;n<unique;++n)if(fields.components[n].component==fields.mesh_seed)break;
        if(n==unique){if(unique>=fields.component_count||fields.components[unique].component!=fields.mesh_seed)return false;++unique;}
        if(!fields.components[n].skeletal)return false;
    }
    unsigned attachment_count{};
    for(unsigned n=0;n<capacity;++n){auto const& scope=fields.attachments[n];
        if(n>=fields.component_count){if(!same(scope,AttachmentScope{}))return false;continue;}
        if(n>=unique)return false;auto const& component=fields.components[n];
        if(!component.skeletal){if(!same(scope,AttachmentScope{}))return false;continue;}
        if(!scope.sampled||!source::pointer(scope.structure,88)||scope.offset<0x60||scope.offset>65520||
           scope.stride<8||scope.stride>attachment_stride_capacity||scope.component_offset>scope.stride-8||scope.first_row!=attachment_count)return false;
        for(auto off:{component.body_instance_offset,0x248u,0x250u})if(scope.offset<=off?off-scope.offset<16:scope.offset-off<8)return false;
        auto const& h=scope.array;
        if(h.count<0||h.count>int(list_capacity)||h.capacity<h.count||h.capacity>256||
           (!h.data?(h.count||h.capacity):!source::pointer(h.data,std::size_t(h.capacity?h.capacity:1)*scope.stride))||
           unsigned(h.count)>attachment_capacity-attachment_count)return false;
        for(int k=0;k<h.count;++k){auto const& row=fields.attachment_rows[attachment_count++];std::uintptr_t at{};
            std::memcpy(&at,row.bytes.data()+scope.component_offset,sizeof at);if(at!=row.component)return false;
            for(unsigned tail=scope.stride;tail<attachment_stride_capacity;++tail)if(row.bytes[tail])return false;
            if(!at)continue;if(!source::pointer(at,88))return false;
            bool duplicate=false;for(unsigned i=0;i<unique;++i)duplicate=duplicate||fields.components[i].component==at;
            if(!duplicate){if(unique>=fields.component_count||fields.components[unique].component!=at)return false;++unique;}
        }
    }
    if(unique!=fields.component_count||attachment_count!=fields.attachment_count)return false;
    for(unsigned n=attachment_count;n<attachment_capacity;++n)if(!same(fields.attachment_rows[n],AttachmentRow{}))return false;
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
    }
    unsigned body_count{};
    for(unsigned n=0;n<capacity;++n){auto const& asset=fields.assets[n];
        if(n>=unique||!fields.components[n].skeletal){if(!same(asset,AssetScope{}))return false;continue;}
        if(!asset.sampled||(asset.asset&&!source::pointer(asset.asset,88)))return false;
        if(!asset.instance){auto absent=asset;absent.asset=0;absent.sampled=false;if(!same(absent,AssetScope{}))return false;continue;}
        if(!source::pointer(asset.instance,88)||(asset.owner&&!source::pointer(asset.owner,88))||asset.first_body!=body_count)return false;
        for(unsigned a=0;a<3;++a){const auto off=asset.offsets[a];const unsigned width=a==1?16u:a==2?4u:8u;
            if(off<0x60||off>65536-width||(a==1&&off!=0x6c))return false;
            for(unsigned b=0;b<a;++b){const auto other=asset.offsets[b];const unsigned ow=b==1?16u:b==2?4u:8u;
                if(off<=other?other-off<width:off-other<ow)return false;}}
        auto const& h=asset.array;
        if(h.count<0||h.count>int(list_capacity)||h.capacity<h.count||h.capacity>256||
           (!h.data?(h.count||h.capacity):!source::pointer(h.data,std::size_t(h.capacity?h.capacity:1)*8))||
           asset.root < -1||(asset.root>=0&&asset.root>=h.count)||unsigned(h.count)>asset_body_capacity-body_count)return false;
        body_count+=unsigned(h.count);
    }
    if(body_count!=fields.asset_body_count)return false;
    for(unsigned n=0;n<asset_body_capacity;++n){auto const& body=fields.asset_bodies[n];
        if(n>=body_count||!body.body){if(!same(body,AssetBody{}))return false;continue;}
        if(!source::pointer(body.body,0x98)||!source::pointer(body.owner,88)||!source::pointer(body.owner_actor,88)||
           body.owner_roots!=owner_mask(fields,body.owner_actor)||(body.data&&!source::pointer(body.data,0x48))||
           (body.spring&&!source::pointer(body.spring,8)))return false;
    }return true;
}
inline bool consistent(Snapshot const& packet)noexcept{
    if(!valid(packet.fields)||packet.actor_count>actor_capacity||packet.fields.pawn_mesh_listed!=mesh_listed(packet.fields,packet.query_source.source.mesh)||
       (packet.fields.mesh_seed&&packet.fields.mesh_seed!=packet.query_source.source.mesh))return false;
    unsigned count{};std::array<std::uintptr_t,actor_capacity> actors{};
    auto accept=[&](Actor const& value,Presence expected,std::uintptr_t at){
        if(value.presence!=expected)return false;
        if(expected!=Presence::actor_observed)return !value.table&&!value.scene&&!value.internal;
        if(value.table!=packet.query_source.actor_table||value.scene!=packet.query_source.scene||!source::pointer(value.internal,0x18))return false;
        bool duplicate=false;for(unsigned k=0;k<count;++k)duplicate=duplicate||actors[k]==at;
        if(!duplicate)actors[count++]=at;return true;};
    for(unsigned n=0;n<capacity;++n){auto const& row=packet.fields.components[n];auto const& value=packet.observations[n];
        const auto expected=n>=packet.fields.component_count?Presence::not_sampled:row.state==State::nonprimitive?Presence::nonprimitive:
            row.state==State::body_instance_field_null?Presence::body_instance_field_null:row.state==State::body_data_field_null?Presence::body_data_field_null:Presence::actor_observed;
        if(!accept(value,expected,row.body_data))return false;
    }
    for(unsigned n=0;n<asset_body_capacity;++n){auto const& body=packet.fields.asset_bodies[n];
        const auto expected=n>=packet.fields.asset_body_count?Presence::not_sampled:!body.body?Presence::body_field_null:
            !body.data?Presence::body_data_field_null:Presence::actor_observed;
        if(!accept(packet.asset_observations[n],expected,body.data))return false;
    }return count==packet.actor_count&&actors==packet.actors;
}
namespace detail {
template<class Read> Code actor(std::uintptr_t image_base,source::Snapshot const& scope,std::uintptr_t data,Read& read,Actor& value,Snapshot& output){
    if(!data){value.presence=Presence::body_data_field_null;return Code::observed;}
    if(!raw::field(read,value.table,data,0)||!raw::field(read,value.scene,data,0x38)||!raw::field(read,value.internal,data,0x40))return Code::read;
    if(value.table!=image_base+0x2330a8)return Code::unsupported;
    if(value.scene!=scope.scene||!source::pointer(value.internal,0x18))return Code::links;
    std::uintptr_t reverse{};if(!raw::field(read,reverse,value.internal,0x10))return Code::read;
    if(reverse!=data)return Code::links;value.presence=Presence::actor_observed;
    bool duplicate=false;for(unsigned k=0;k<output.actor_count;++k)duplicate=duplicate||output.actors[k]==data;
    if(!duplicate)output.actors[output.actor_count++]=data;return Code::observed;
}
template<class Read> Code sample(std::uintptr_t image_base,source::Snapshot const& scope,Source const& fields,Read& read,Snapshot& output){
    output.query_source=scope;output.fields=fields;output.actor_count=0;output.actors.fill(0);
    for(auto& value:output.observations)value={};
    for(auto& value:output.asset_observations)value={};
    for(unsigned r=0;r<fields.root_count;++r){auto const& root=fields.roots[r];raw::ArrayHeader header{};
        for(unsigned l=0;l<2;++l){
            if(!raw::field(read,header,root.actor,root.offsets[l]))return Code::read;
            if(!raw::header_equal(header,root.lists[l]))return Code::changed;
            std::array<std::uintptr_t,list_capacity> members{};
            if(header.count&&!raw::bytes(read,members.data(),header.data,std::size_t(header.count)*8))return Code::read;
            if(members!=root.members[l])return Code::changed;}
        std::uintptr_t base_actor{};if(!raw::field(read,base_actor,root.actor,root.base_offset)||
           !raw::field(read,header,root.actor,root.attached_offset))return Code::read;
        if(base_actor!=root.base||!raw::header_equal(header,root.attached))return Code::changed;
        std::array<std::uintptr_t,list_capacity> members{};
        if(header.count&&!raw::bytes(read,members.data(),header.data,std::size_t(header.count)*8))return Code::read;
        if(members!=root.attached_members)return Code::changed;
    }
    for(unsigned n=0;n<fields.component_count;++n){auto const& row=fields.components[n];auto& value=output.observations[n];
        if(row.state==State::nonprimitive){value.presence=Presence::nonprimitive;continue;}
        std::uintptr_t body{},data{},spring{};
        if(!raw::field(read,body,row.component,row.body_instance_offset))return Code::read;
        if(body!=row.body)return Code::changed;
        if(!body){value.presence=Presence::body_instance_field_null;continue;}
        if(!raw::field(read,data,body,0x88)||!raw::field(read,spring,body,0x90))return Code::read;
        if(data!=row.body_data||spring!=row.bone_spring)return Code::changed;
        const auto code=actor(image_base,scope,data,read,value,output);if(code!=Code::observed)return code;
    }
    for(unsigned n=0;n<fields.component_count;++n){auto const& component=fields.components[n];if(!component.skeletal)continue;
        auto const& asset=fields.assets[n];std::uintptr_t asset_at{},instance_at{},owner{};raw::ArrayHeader header{};std::int32_t root{};
        if(!raw::field(read,asset_at,component.component,0x248)||!raw::field(read,instance_at,component.component,0x250))return Code::read;
        if(asset_at!=asset.asset||instance_at!=asset.instance)return Code::changed;
        if(!instance_at)continue;
        if(!raw::field(read,owner,instance_at,asset.offsets[0])||!raw::field(read,header,instance_at,asset.offsets[1])||
           !raw::field(read,root,instance_at,asset.offsets[2]))return Code::read;
        if(owner!=asset.owner||!raw::header_equal(header,asset.array)||root!=asset.root)return Code::changed;
        std::array<std::uintptr_t,list_capacity> members{};
        if(header.count&&!raw::bytes(read,members.data(),header.data,std::size_t(header.count)*8))return Code::read;
        for(unsigned k=0;k<unsigned(header.count);++k){const auto index=asset.first_body+k;auto const& body=fields.asset_bodies[index];
            if(members[k]!=body.body)return Code::changed;auto& value=output.asset_observations[index];
            if(!body.body){value.presence=Presence::body_field_null;continue;}
            std::uintptr_t data{},spring{};
            if(!raw::field(read,data,body.body,0x88)||!raw::field(read,spring,body.body,0x90))return Code::read;
            if(data!=body.data||spring!=body.spring)return Code::changed;
            const auto code=actor(image_base,scope,data,read,value,output);if(code!=Code::observed)return code;
        }
    }
    for(unsigned n=0;n<fields.component_count;++n){auto const& component=fields.components[n];if(!component.skeletal)continue;
        auto const& attachment=fields.attachments[n];raw::ArrayHeader header{};
        if(!raw::field(read,header,component.component,attachment.offset))return Code::read;
        if(!raw::header_equal(header,attachment.array))return Code::changed;
        for(int k=0;k<header.count;++k){AttachmentRow row{};
            if(!raw::bytes(read,row.bytes.data(),header.data+std::size_t(k)*attachment.stride,attachment.stride))return Code::read;
            std::memcpy(&row.component,row.bytes.data()+attachment.component_offset,sizeof row.component);
            if(!same(row,fields.attachment_rows[attachment.first_row+unsigned(k)]))return Code::changed;
        }
    }return Code::observed;
}
}
template<class Read,class Guard> Code observe(std::uintptr_t image_base,std::uint32_t image_size,source::Snapshot const& scope,
    Source const& input,Read read,Guard guard,Workspace& work,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard&>&&std::is_invocable_r_v<bool,Guard&,Snapshot const&>);
    if(!image_base||(image_base&0xffff)||image_size<0x23cdd8||image_size>UINTPTR_MAX-image_base||scope.source.thread!=GetCurrentThreadId()||!valid(input)||input.pawn_mesh_listed!=mesh_listed(input,scope.source.mesh)||
       (input.mesh_seed&&input.mesh_seed!=scope.source.mesh)||
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
