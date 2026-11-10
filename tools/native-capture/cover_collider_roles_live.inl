#ifndef GEARS_COVER_COLLIDER_ROLES_LIVE_INCLUDED
#define GEARS_COVER_COLLIDER_ROLES_LIVE_INCLUDED
#include "cover_physics_roles_live.inl"
#include "cover_physx_collider.h"

// Include in the existing Native binding environment. The caller must own the
// current admitted C0 and keep its actual owner guard current. These reflected
// fields supplement the PAA Bodies source; they are NOT a complete self set,
// certified NxActor, collider geometry, world fence or movement permission.
namespace cover_collider_roles_live {
namespace binding=weapon_hand_scene_binding;
namespace roles=cover_physics_roles_live;
namespace source=gears_cover_physx_query_source;
using Code=roles::Code;
enum class Presence:unsigned {not_sampled,body_instance_field_null,body_data_field_null,body_data_nonnull};
struct Snapshot {
    gears_anchor::Identity pawn{},cylinder{},body{};
    Presence presence{};
    std::int32_t body_index{};
    unsigned body_instance_offset{};
    std::uintptr_t body_data{},bone_spring{};
};
inline bool same(Snapshot const& a,Snapshot const& b){return gears_anchor::equal_identity(a.pawn,b.pawn)&&gears_anchor::equal_identity(a.cylinder,b.cylinder)&&
    gears_anchor::equal_identity(a.body,b.body)&&a.presence==b.presence&&a.body_index==b.body_index&&a.body_instance_offset==b.body_instance_offset&&a.body_data==b.body_data&&a.bone_spring==b.bone_spring;}
inline gears_cover_physx_collider::Source input(Snapshot const& value){return {value.pawn.address,value.cylinder.address,value.body.address,
    value.body_instance_offset,value.body_index,value.body_data,value.bone_spring};}
inline bool matches(Snapshot const& typed,gears_cover_physx_collider::Snapshot const& value){
    namespace cx=gears_cover_physx_collider;
    if(!cx::same(input(typed),value.fields))return false;
    if(typed.presence==Presence::body_instance_field_null)return value.presence==cx::Presence::body_instance_field_null&&!value.actor_table&&!value.scene&&!value.internal;
    if(typed.presence==Presence::body_data_field_null)return value.presence==cx::Presence::body_data_field_null&&!value.actor_table&&!value.scene&&!value.internal;
    return typed.presence==Presence::body_data_nonnull&&value.presence==cx::Presence::actor_observed&&
        value.actor_table==value.query_source.actor_table&&value.scene==value.query_source.scene&&value.internal;
}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
struct Workspace {
    roles::MemberStore members{};
    binding::Proof pawn{},component{},body{};
    std::array<binding::Property,2> pawn_fields{},component_fields{};
    std::array<binding::Property,4> body_fields{};
    Snapshot first{},last{};
    std::array<unsigned char,sizeof(gears_anchor::Config)> metadata{};
    std::uint64_t generation{};
};
inline bool instance(binding::Proof& proof,std::uintptr_t address,gears_anchor::Identity& result){
    return binding::identity(address,result,true)&&binding::remember(proof,result);
}
inline Code values(Workspace& w,binding::Binding const& owner,Snapshot& output,bool repeat=false){
    Snapshot next{};
    if(!instance(w.pawn,owner.pawn.address,next.pawn)||!binding::same(next.pawn,owner.pawn))return Code::read;
    if(repeat){
        if(!binding::same(next.pawn,w.first.pawn)||!binding::metadata_current()||
           w.generation!=binding::metadata_generation||w.metadata!=binding::metadata_bytes||
           !binding::current(w.pawn))return Code::changed;
    }else if(!roles::class_role(w.pawn,next.pawn.cls,L"Pawn")||
       !roles::role_property(w.pawn,next.pawn.cls,L"CylinderComponent",L"ComponentProperty",8,L"CylinderComponent",w.pawn_fields[0],&w.members)||
       !roles::role_property(w.pawn,next.pawn.cls,L"CollisionComponent",L"ComponentProperty",8,L"PrimitiveComponent",w.pawn_fields[1],&w.members))return Code::layout;
    std::uintptr_t cylinder{},selected{},component_owner{},body{};
    if(!binding::field(cylinder,next.pawn.address,w.pawn_fields[0].offset)||
       !binding::field(selected,next.pawn.address,w.pawn_fields[1].offset)||!cylinder||selected!=cylinder||
       !instance(w.component,cylinder,next.cylinder)||next.cylinder.outer!=next.pawn.address)return Code::read;
    if(repeat){
        if(!binding::same(next.cylinder,w.first.cylinder)||!binding::current(w.component))return Code::changed;
    }else if(!roles::class_role(w.component,next.cylinder.cls,L"CylinderComponent",w.pawn_fields[0].reference.address)||
       !roles::class_role(w.component,next.cylinder.cls,L"PrimitiveComponent",w.pawn_fields[1].reference.address)||
       !roles::role_property(w.component,next.cylinder.cls,L"Owner",L"ObjectProperty",8,L"Actor",w.component_fields[0],&w.members)||
       !roles::role_property(w.component,next.cylinder.cls,L"BodyInstance",L"ObjectProperty",8,L"RB_BodyInstance",w.component_fields[1],&w.members))return Code::layout;
    if(!binding::field(component_owner,cylinder,w.component_fields[0].offset)||component_owner!=next.pawn.address||
       !binding::field(body,cylinder,w.component_fields[1].offset))return Code::read;
    next.body_instance_offset=w.component_fields[1].offset;
    if(!body){next.presence=Presence::body_instance_field_null;output=next;return Code::current;}
    if(!instance(w.body,body,next.body))return Code::read;
    if(repeat){
        // A formerly null/replaced BodyInstance has no reusable body proof.
        // Reject the changed snapshot before consuming any cached body offset.
        if(!binding::same(next.body,w.first.body)||!binding::current(w.body))return Code::changed;
    }else if(!roles::class_role(w.body,next.body.cls,L"RB_BodyInstance",w.component_fields[1].reference.address)||
       !roles::role_property(w.body,next.body.cls,L"BodyIndex",L"IntProperty",4,nullptr,w.body_fields[0],&w.members)||
       !roles::role_property(w.body,next.body.cls,L"OwnerComponent",L"ComponentProperty",8,L"PrimitiveComponent",w.body_fields[1],&w.members)||
       !roles::class_role(w.body,next.cylinder.cls,L"PrimitiveComponent",w.body_fields[1].reference.address)||
       !roles::pointer_property(w.body,next.body.cls,L"BodyData",0x88,w.body_fields[2],&w.members)||
       !roles::pointer_property(w.body,next.body.cls,L"BoneSpring",0x90,w.body_fields[3],&w.members))return Code::layout;
    std::uintptr_t body_owner{};
    if(!binding::field(next.body_index,body,w.body_fields[0].offset)||
       !binding::field(body_owner,body,w.body_fields[1].offset)||body_owner!=cylinder||
       !binding::field(next.body_data,body,w.body_fields[2].offset)||
       !binding::field(next.bone_spring,body,w.body_fields[3].offset)||
       (next.body_data&&!source::pointer(next.body_data,8))||(next.bone_spring&&!source::pointer(next.bone_spring,8)))return Code::read;
    // BodyIndex is observed, not constrained to a PAA-array row. BoneSpring is
    // opaque; nonzero BodyData is still only a pointer, not a qualified actor.
    next.presence=next.body_data?Presence::body_data_nonnull:Presence::body_data_field_null;
    output=next;return Code::current;
}
inline bool current(Workspace const& w){return binding::metadata_current()&&w.generation==binding::metadata_generation&&
    w.metadata==binding::metadata_bytes&&w.members.current()&&binding::current(w.pawn)&&binding::current(w.component)&&
    (w.first.presence==Presence::body_instance_field_null||binding::current(w.body));}
template<class Guard> Code sample(binding::Binding const& actor,Guard guard,Workspace& work,Snapshot& output)noexcept{
    if(!actor.context_valid||!actor.world_generation||source::aliases(&work,sizeof work,&output,sizeof output)||
       source::aliases(&work,sizeof work,&actor,sizeof actor)||source::aliases(&output,sizeof output,&actor,sizeof actor))return Code::request;
    const auto owner=actor;
    try{
        if(!guard())return Code::guard;
        {
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return Code::metadata;
            if(!binding::context_ready()||!binding::metadata_current()||actor.world_generation!=binding::metadata_generation)return Code::metadata;
            work.members.clear();work.pawn={};work.component={};work.body={};work.metadata=binding::metadata_bytes;work.generation=binding::metadata_generation;
            auto code=values(work,owner,work.first);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            code=values(work,owner,work.last,true);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            if(!same(work.first,work.last)||!current(work))return Code::changed;
        }
        if(!guard())return Code::guard;
        if(!binding::same_key(owner,actor)||owner.scene_invocation!=actor.scene_invocation||owner.special_move!=actor.special_move)return Code::changed;
        output=work.first;return Code::current;
    }catch(...){return Code::exception;}
}
#else
struct Workspace {};
template<class Guard> Code sample(binding::Binding const&,Guard,Workspace&,Snapshot&)noexcept{return Code::unavailable;}
#endif
} // namespace cover_collider_roles_live
#endif
