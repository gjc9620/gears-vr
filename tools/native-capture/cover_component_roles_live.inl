#ifndef GEARS_COVER_COMPONENT_ROLES_LIVE_INCLUDED
#define GEARS_COVER_COMPONENT_ROLES_LIVE_INCLUDED
#include "cover_collider_roles_live.inl"
#include <algorithm>

// Current Pawn and known current Weapon DECLARED component arrays. The C0
// caller supplies its actual owner guard. Neither membership nor Owner alone
// declares a component self or all active physics covered. Attachments, other
// actors, extra skeletal PAA bodies, lifetime and world synchronization remain
// separate obligations. This reader calls no engine method and writes no body.
namespace cover_component_roles_live {
namespace binding=weapon_hand_scene_binding;
namespace roles=cover_physics_roles_live;
namespace collider=cover_collider_roles_live;
namespace source=gears_cover_physx_query_source;
using Code=roles::Code;
constexpr unsigned root_capacity=2,list_capacity=64,component_capacity=128;
enum class BodyState:unsigned {not_sampled,nonprimitive,body_instance_field_null,body_data_field_null,body_data_nonnull};
struct Root {
    gears_anchor::Identity actor{};
    std::array<source::pose::ArrayHeader,2> lists{};
    std::array<std::array<std::uintptr_t,list_capacity>,2> members{};
};
struct Component {
    gears_anchor::Identity identity{},owner{};
    gears_anchor::Identity body_owner{},body_owner_actor{};
    // Bit0 is Pawn, bit1 is current Weapon. Zero preserves an external Owner.
    unsigned owner_roots{},body_owner_roots{};bool primitive{},skeletal{};
    BodyState body_state{};unsigned body_instance_offset{};roles::Body body{};
};
struct Snapshot {
    unsigned root_count{},component_count{};bool pawn_mesh_listed{};
    std::array<Root,root_capacity> roots{};
    // First occurrence order; original array rows (nulls/duplicates included)
    // remain in Root::members instead of being silently dropped.
    std::array<Component,component_capacity> components{};
};
inline bool same_body(roles::Body const& a,roles::Body const& b){return gears_anchor::equal_identity(a.identity,b.identity)&&
    a.owner==b.owner&&a.main==b.main&&a.auxiliary==b.auxiliary&&a.index==b.index;}
inline bool same(Snapshot const& a,Snapshot const& b){
    if(a.root_count!=b.root_count||a.component_count!=b.component_count||a.pawn_mesh_listed!=b.pawn_mesh_listed)return false;
    for(unsigned i=0;i<root_capacity;++i){auto const& x=a.roots[i];auto const& y=b.roots[i];
        if(!gears_anchor::equal_identity(x.actor,y.actor)||x.members!=y.members)return false;
        for(unsigned n=0;n<2;++n)if(!source::pose::header_equal(x.lists[n],y.lists[n]))return false;}
    for(unsigned i=0;i<component_capacity;++i){auto const& x=a.components[i];auto const& y=b.components[i];
        if(!gears_anchor::equal_identity(x.identity,y.identity)||!gears_anchor::equal_identity(x.owner,y.owner)||
           !gears_anchor::equal_identity(x.body_owner,y.body_owner)||!gears_anchor::equal_identity(x.body_owner_actor,y.body_owner_actor)||
           x.owner_roots!=y.owner_roots||x.body_owner_roots!=y.body_owner_roots||x.primitive!=y.primitive||x.skeletal!=y.skeletal||x.body_state!=y.body_state||
           x.body_instance_offset!=y.body_instance_offset||!same_body(x.body,y.body))return false;}
    return true;
}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
struct RootLayout {binding::Proof proof{};std::array<binding::Property,3> fields{};};
struct ComponentLayout {binding::Proof proof{};gears_anchor::Identity cls{};std::array<binding::Property,2> fields{};bool primitive{},skeletal{};};
struct BodyLayouts {gears_anchor::Identity body_class{};std::array<roles::BodyLayout,8> layouts{};unsigned layout_count{};};
struct Workspace {
    std::array<RootLayout,root_capacity> roots{};
    std::array<ComponentLayout,8> layouts{};unsigned layout_count{};
    gears_anchor::Identity component_base{};BodyLayouts bodies{};
    Snapshot first{},last{};
    std::array<unsigned char,sizeof(gears_anchor::Config)> metadata{};std::uint64_t generation{};
};
inline bool separate(unsigned a,unsigned aw,unsigned b,unsigned bw){return a<=b?b-a>=aw:a-b>=bw;}
inline bool array_schema(binding::Proof& p,binding::Property const& field,gears_anchor::Identity& component_base){
    std::uintptr_t inner_descriptor{},target{};gears_anchor::Identity id{},type{};unsigned dim{},width{},offset{};
    if(!binding::source_field(p,inner_descriptor,field.id.address,0xd0)||!binding::certified(p,inner_descriptor,id)||id.number||id.outer!=field.id.address||
       !binding::kind(p,id,L"ComponentProperty")||!binding::source_field(p,dim,inner_descriptor,0x70)||dim!=1||
       !binding::source_field(p,width,inner_descriptor,0x74)||width!=8||!binding::source_field(p,offset,inner_descriptor,0x94)||offset||
       !binding::source_field(p,target,inner_descriptor,0xd0)||!binding::certified(p,target,type)||!binding::kind(p,type,L"Class")||
       !binding::named(type,L"ActorComponent"))return false;
    if(component_base.address&&!binding::same(component_base,type))return false;component_base=type;return true;
}
inline Code root(Workspace& w,unsigned index,binding::Binding const& context,Root& output){
    Root next{};auto& l=w.roots[index];auto& p=l.proof;auto const& wanted=index?context.weapon:context.pawn;
    if(!collider::instance(p,wanted.address,next.actor)||!binding::same(next.actor,wanted))return Code::read;
    if(!roles::class_role(p,next.actor.cls,index?L"Weapon":L"Pawn")||
       !roles::role_property(p,next.actor.cls,L"WorldInfo",L"ObjectProperty",8,L"WorldInfo",l.fields[0])||
       !roles::role_property(p,next.actor.cls,L"Components",L"ArrayProperty",16,nullptr,l.fields[1])||
       !roles::role_property(p,next.actor.cls,L"AllComponents",L"ArrayProperty",16,nullptr,l.fields[2]))return Code::layout;
    for(unsigned a=0;a<3;++a)for(unsigned b=a+1;b<3;++b)
        if(!separate(l.fields[a].offset,a?16u:8u,l.fields[b].offset,b?16u:8u))return Code::layout;
    std::uintptr_t world{};
    if(!binding::source_field(p,world,next.actor.address,l.fields[0].offset)||world!=context.world.address)return Code::changed;
    for(unsigned n=0;n<2;++n){auto const& field=l.fields[n+1];auto& list=next.lists[n];
        if(!array_schema(p,field,w.component_base))return Code::layout;
        if(!binding::source_field(p,list.data,next.actor.address,field.offset)||
           !binding::source_field(p,list.count,next.actor.address,field.offset+8)||
           !binding::source_field(p,list.capacity,next.actor.address,field.offset+12))return Code::read;
        if(list.count<0||list.count>int(list_capacity)||list.capacity<list.count||list.capacity>256||
           (!list.data?(list.count||list.capacity):!source::pointer(list.data,std::size_t((std::max)(1,list.capacity))*8)))return Code::capacity;
        if(list.count&&!safe_copy(next.members[n].data(),reinterpret_cast<void const*>(list.data),std::size_t(list.count)*8))return Code::read;
    }
    output=next;return Code::current;
}
inline ComponentLayout* component_layout(Workspace& w,gears_anchor::Identity const& id){
    for(unsigned n=0;n<w.layout_count;++n)if(w.layouts[n].cls.address==id.cls)return &w.layouts[n];
    if(w.layout_count==w.layouts.size())return nullptr;
    auto& l=w.layouts[w.layout_count];auto& p=l.proof;binding::ClassPath path{};
    if(!binding::certified(p,id.cls,l.cls)||!roles::class_role(p,id.cls,L"ActorComponent",w.component_base.address)||
       !binding::derives(id.cls,L"ActorComponent",w.component_base.address,path)||
       !roles::role_property(p,id.cls,L"Owner",L"ObjectProperty",8,L"Actor",l.fields[0]))return nullptr;
    for(unsigned i=0;i<path.count;++i){l.primitive=l.primitive||binding::named(path.ids[i],L"PrimitiveComponent");
        l.skeletal=l.skeletal||binding::named(path.ids[i],L"SkeletalMeshComponent");}
    if(l.primitive){
        if(!roles::role_property(p,id.cls,L"BodyInstance",L"ObjectProperty",8,L"RB_BodyInstance",l.fields[1]))return nullptr;
        if(!separate(l.fields[0].offset,8,l.fields[1].offset,8))return nullptr;
        if(w.bodies.body_class.address&&!binding::same(w.bodies.body_class,l.fields[1].reference))return nullptr;
        w.bodies.body_class=l.fields[1].reference;
    }
    ++w.layout_count;return &l;
}
inline Code component(Workspace& w,Snapshot const& roots,std::uintptr_t at,Component& output){
    Component next{};
    if(!binding::identity(at,next.identity,true))return Code::read;
    auto* layout=component_layout(w,next.identity);if(!layout)return w.layout_count==w.layouts.size()?Code::capacity:Code::layout;
    std::uintptr_t owner{},body{};
    if(!binding::field(owner,at,layout->fields[0].offset)||!binding::identity(owner,next.owner,true))return Code::read;
    if(!roles::class_role(layout->proof,next.owner.cls,L"Actor",layout->fields[0].reference.address))return Code::layout;
    for(unsigned i=0;i<roots.root_count;++i)if(binding::same(next.owner,roots.roots[i].actor))next.owner_roots|=1u<<i;
    next.primitive=layout->primitive;next.skeletal=layout->skeletal;
    if(!next.primitive){next.body_state=BodyState::nonprimitive;output=next;return Code::current;}
    next.body_instance_offset=layout->fields[1].offset;
    if(!binding::field(body,at,next.body_instance_offset))return Code::read;
    if(!body){next.body_state=BodyState::body_instance_field_null;output=next;return Code::current;}
    if(!binding::identity(body,next.body.identity,true))return Code::read;
    auto* l=roles::body_layout(w.bodies,next.body.identity);if(!l)return w.bodies.layout_count==w.bodies.layouts.size()?Code::capacity:Code::layout;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b)
        if(!separate(l->fields[a].offset,a?8u:4u,l->fields[b].offset,b?8u:4u))return Code::layout;
    if(!roles::class_role(l->proof,next.identity.cls,L"PrimitiveComponent",l->fields[1].reference.address))return Code::layout;
    if(!binding::field(next.body.owner,body,l->fields[1].offset)||!binding::identity(next.body.owner,next.body_owner,true)||
       !binding::field(next.body.index,body,l->fields[0].offset)||!binding::field(next.body.main,body,l->fields[2].offset)||
       !binding::field(next.body.auxiliary,body,l->fields[3].offset)||
       (next.body.main&&!source::pointer(next.body.main,8))||(next.body.auxiliary&&!source::pointer(next.body.auxiliary,8)))return Code::changed;
    if(!roles::class_role(l->proof,next.body_owner.cls,L"PrimitiveComponent",l->fields[1].reference.address))return Code::layout;
    auto* backing=component_layout(w,next.body_owner);if(!backing)return w.layout_count==w.layouts.size()?Code::capacity:Code::layout;
    std::uintptr_t backing_actor{};
    if(!binding::field(backing_actor,next.body.owner,backing->fields[0].offset)||!binding::identity(backing_actor,next.body_owner_actor,true))return Code::read;
    if(!roles::class_role(backing->proof,next.body_owner_actor.cls,L"Actor",backing->fields[0].reference.address))return Code::layout;
    for(unsigned i=0;i<roots.root_count;++i)if(binding::same(next.body_owner_actor,roots.roots[i].actor))next.body_owner_roots|=1u<<i;
    next.body_state=next.body.main?BodyState::body_data_nonnull:BodyState::body_data_field_null;output=next;return Code::current;
}
inline Code values(Workspace& w,binding::Binding const& context,Snapshot& output){
    // Work candidates are already member-owned; do not put the large catalog
    // on the game stack or publish a partial candidate to the caller.
    auto& next=output;next.root_count=context.weapon.address?2u:1u;next.component_count=0;next.pawn_mesh_listed=false;
    for(auto& item:next.roots)item={};for(auto& item:next.components)item={};
    for(unsigned i=0;i<next.root_count;++i){const auto code=root(w,i,context,next.roots[i]);if(code!=Code::current)return code;}
    for(unsigned r=0;r<next.root_count;++r)for(unsigned l=0;l<2;++l){auto const& list=next.roots[r];
        for(int i=0;i<list.lists[l].count;++i){const auto at=list.members[l][unsigned(i)];if(!at)continue;
            if(!r&&at==context.mesh.address)next.pawn_mesh_listed=true;
            bool duplicate=false;for(unsigned n=0;n<next.component_count;++n)duplicate=duplicate||next.components[n].identity.address==at;
            if(duplicate)continue;
            if(next.component_count==component_capacity)return Code::capacity;
            const auto code=component(w,next,at,next.components[next.component_count]);if(code!=Code::current)return code;++next.component_count;
        }}
    return Code::current;
}
inline bool current(Workspace const& w,unsigned roots){
    if(!binding::metadata_current()||w.metadata!=binding::metadata_bytes||w.generation!=binding::metadata_generation)return false;
    for(unsigned i=0;i<roots;++i)if(!binding::current(w.roots[i].proof))return false;
    for(unsigned i=0;i<w.layout_count;++i)if(!binding::current(w.layouts[i].proof))return false;
    for(unsigned i=0;i<w.bodies.layout_count;++i)if(!binding::current(w.bodies.layouts[i].proof))return false;
    return true;
}
inline void clear_proof(binding::Proof& proof){
    // Do not create an aggregate containing all eight large Proof objects on
    // the game stack. Zero active lengths/keys: every selected source and
    // identity is re-read this invocation; inactive scratch is never consumed.
    proof.source_count=proof.identity_count=proof.attempts=0;proof.last_attempt=0;
    proof.attempted=proof.ready=false;proof.fields={};proof.keys={};
}
inline void clear(Workspace& work){
    for(auto& r:work.roots){clear_proof(r.proof);r.fields={};}
    for(auto& l:work.layouts){clear_proof(l.proof);l.cls={};l.fields={};l.primitive=l.skeletal=false;}
    work.layout_count=0;work.component_base={};work.bodies.body_class={};work.bodies.layout_count=0;
    for(auto& l:work.bodies.layouts){clear_proof(l.proof);l.cls={};l.fields={};}
}
template<class Guard> Code sample(binding::Binding const& context,Guard guard,Workspace& work,Snapshot& output)noexcept{
    if(!context.context_valid||!context.world_generation||!context.weapon_known||!context.pawn.address||!context.world.address||
       (!context.weapon.address&&!gears_anchor::equal_identity(context.weapon,gears_anchor::Identity{}))||
       source::aliases(&work,sizeof work,&output,sizeof output)||source::aliases(&work,sizeof work,&context,sizeof context)||
       source::aliases(&output,sizeof output,&context,sizeof context))return Code::request;
    const auto owner=context;
    try{
        if(!guard())return Code::guard;
        {
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return Code::metadata;
            if(!binding::context_ready()||!binding::metadata_current()||context.world_generation!=binding::metadata_generation)return Code::metadata;
            clear(work);
            work.metadata=binding::metadata_bytes;work.generation=binding::metadata_generation;
            auto code=values(work,owner,work.first);if(code!=Code::current)return code;
            code=values(work,owner,work.last);if(code!=Code::current)return code;
            if(!same(work.first,work.last)||!current(work,work.first.root_count))return Code::changed;
        }
        if(!guard())return Code::guard;
        if(!binding::same_key(owner,context)||owner.scene_invocation!=context.scene_invocation||owner.special_move!=context.special_move)return Code::changed;
        output=work.first;return Code::current;
    }catch(...){return Code::exception;}
}
#else
struct Workspace {};
template<class Guard> Code sample(binding::Binding const&,Guard,Workspace&,Snapshot&)noexcept{return Code::unavailable;}
#endif
}
#endif
