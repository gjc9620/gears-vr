#ifndef GEARS_COVER_PHYSICS_ROLES_LIVE_INCLUDED
#define GEARS_COVER_PHYSICS_ROLES_LIVE_INCLUDED
#include "cover_physx_query_source.h"

// Include after weapon_hand_scene_binding. The caller brackets this synchronous
// read with its admitted ActualContext::current and current backend metadata.
// Never hold binding::mutex when invoking that guard or an original query.
// This typed source observation is NOT a scene/world/physics/movement permit.
namespace cover_physics_roles_live {
namespace binding=weapon_hand_scene_binding;
namespace query_source=gears_cover_physx_query_source;
namespace pose=gears_cover_pose_phase;
struct Body {
    gears_anchor::Identity identity{};
    std::uintptr_t owner{},main{},auxiliary{};std::int32_t index{};
};
struct Snapshot {
    gears_anchor::Identity mesh{},pawn{},asset{},instance{},owner{};
    pose::ArrayHeader array{};std::int32_t root{-1};
    std::array<Body,64> bodies{};query_source::Source source{};
};
inline bool same(Snapshot const& a,Snapshot const& b){
    if(!gears_anchor::equal_identity(a.mesh,b.mesh)||!gears_anchor::equal_identity(a.pawn,b.pawn)||!gears_anchor::equal_identity(a.asset,b.asset)||
       !gears_anchor::equal_identity(a.instance,b.instance)||!gears_anchor::equal_identity(a.owner,b.owner)||!pose::header_equal(a.array,b.array)||
       a.root!=b.root||!query_source::same(a.source,b.source))return false;
    for(unsigned i=0;i<a.bodies.size();++i){auto const& x=a.bodies[i];auto const& y=b.bodies[i];
        if(!gears_anchor::equal_identity(x.identity,y.identity)||x.owner!=y.owner||x.main!=y.main||x.auxiliary!=y.auxiliary||x.index!=y.index)return false;}
    return true;
}
inline bool matches_query_source(Snapshot const& typed,query_source::Snapshot const& raw){
    if(typed.array.count<=0||typed.array.count>64||typed.root<0||typed.root>=typed.array.count||
       !query_source::same(typed.source,raw.source)||!pose::header_equal(typed.array,raw.bodies)||
       raw.body!=typed.bodies[unsigned(typed.root)].identity.address||raw.actor!=typed.bodies[unsigned(typed.root)].main)return false;
    for(unsigned i=0;i<typed.bodies.size();++i)if(typed.bodies[i].identity.address!=raw.members[i])return false;
    return true;
}
enum class Code {current,unavailable,request,guard,metadata,layout,read,changed,capacity,exception};
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
struct BodyLayout {binding::Proof proof{};gears_anchor::Identity cls{};std::array<binding::Property,4> fields{};};
struct Workspace {
    binding::Proof mesh{},instance{};
    std::array<binding::Property,2> mesh_fields{};
    std::array<binding::Property,3> instance_fields{};
    gears_anchor::Identity body_class{};
    // Original arrays are bounded at64; distinct derived layouts are bounded
    // independently. Exhaustion is a rejection, never a truncated success.
    std::array<BodyLayout,8> layouts{};unsigned layout_count{};
    Snapshot first{},last{};
    std::array<unsigned char,sizeof(gears_anchor::Config)> metadata{};
    std::uint64_t generation{};
};
inline bool class_role(binding::Proof& proof,std::uintptr_t cls,wchar_t const* name,std::uintptr_t reference=0){
    binding::ClassPath path{};if(!binding::derives(cls,name,reference,path))return false;
    for(unsigned n=0;n<path.count;++n){auto wanted=path.ids[n];const auto parent=wanted.super;wanted.super=0;wanted.check_super=0;
        gears_anchor::Identity actual{};std::uintptr_t next{};
        if(!binding::certified(proof,wanted.address,actual)||!binding::same(wanted,actual)||!binding::kind(proof,actual,L"Class")||
           !binding::source_field(proof,next,wanted.address,binding::metadata.field_super)||next!=parent)return false;}
    return true;
}
// Close every sibling, including those AFTER the selected property. The common
// property helper only needs the selected descriptor; source consumers here
// additionally retain full declaring-owner membership/name witnesses.
inline bool membership(binding::Proof& p,binding::Property const& selected){
    std::uintptr_t child{};if(!binding::source_field(p,child,selected.owner.address,0x80))return false;
    std::array<std::uintptr_t,4096> visited{};unsigned count{},matches{};
    while(child&&count<visited.size()){
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
        gears_anchor::Identity id{};
        if(!binding::certified(p,child,id)||id.outer!=selected.owner.address)return false;
        if(!std::wcscmp(id.name.data(),selected.id.name.data())){
            if(++matches!=1||!binding::same(id,selected.id))return false;}
        if(!binding::source_field(p,child,child,0x68))return false;
    }
    return !child&&matches==1;
}
inline bool role_property(binding::Proof& p,std::uintptr_t cls,wchar_t const* name,wchar_t const* kind,unsigned width,
    wchar_t const* reference,binding::Property& output){
    return binding::property(p,cls,name,kind,width,reference,output)&&membership(p,output);
}
inline bool pointer_property(binding::Proof& p,std::uintptr_t cls,wchar_t const* name,unsigned offset,binding::Property& output){
    std::uintptr_t at{};gears_anchor::Identity type{};unsigned extent{};
    return role_property(p,cls,name,L"StructProperty",8,nullptr,output)&&output.offset==offset&&
        binding::source_field(p,at,output.id.address,0xd0)&&binding::certified(p,at,type)&&binding::named(type,L"Pointer")&&
        binding::kind(p,type,L"ScriptStruct")&&binding::source_field(p,extent,at,0x88)&&extent==8;
}
inline bool reference_class(binding::Proof& p,binding::Property& field){
    std::uintptr_t at{};return binding::source_field(p,at,field.id.address,0xd0)&&
        binding::certified(p,at,field.reference)&&binding::kind(p,field.reference,L"Class");
}
inline bool schema(Workspace& w,Snapshot const& next){
    auto& m=w.mesh;auto& i=w.instance;
    if(!class_role(m,next.mesh.cls,L"SkeletalMeshComponent")||!class_role(m,next.asset.cls,L"PhysicsAsset")||
       !class_role(i,next.instance.cls,L"PhysicsAssetInstance")||
       !role_property(m,next.mesh.cls,L"PhysicsAsset",L"ObjectProperty",8,L"PhysicsAsset",w.mesh_fields[0])||
       !role_property(m,next.mesh.cls,L"PhysicsAssetInstance",L"ObjectProperty",8,L"PhysicsAssetInstance",w.mesh_fields[1])||
       w.mesh_fields[0].offset!=0x248||w.mesh_fields[1].offset!=0x250||
       !class_role(m,next.asset.cls,L"PhysicsAsset",w.mesh_fields[0].reference.address)||
       !class_role(i,next.instance.cls,L"PhysicsAssetInstance",w.mesh_fields[1].reference.address)||
       !role_property(i,next.instance.cls,L"Owner",L"MeshPointer",8,nullptr,w.instance_fields[0])||
       !role_property(i,next.instance.cls,L"Bodies",L"ArrayProperty",16,nullptr,w.instance_fields[1])||
       !role_property(i,next.instance.cls,L"RootBodyIndex",L"IntProperty",4,nullptr,w.instance_fields[2])||
       !reference_class(i,w.instance_fields[0])||w.instance_fields[1].offset!=0x6c)return false;
    auto const& array=w.instance_fields[1];std::uintptr_t inner{},target{};gears_anchor::Identity id{};unsigned dim{},width{},offset{};
    return binding::source_field(i,inner,array.id.address,0xd0)&&binding::certified(i,inner,id)&&!id.number&&id.outer==array.id.address&&
        binding::kind(i,id,L"ObjectProperty")&&binding::source_field(i,dim,inner,0x70)&&dim==1&&
        binding::source_field(i,width,inner,0x74)&&width==8&&binding::source_field(i,offset,inner,0x94)&&!offset&&
        binding::source_field(i,target,inner,0xd0)&&binding::certified(i,target,w.body_class)&&
        binding::kind(i,w.body_class,L"Class")&&binding::named(w.body_class,L"RB_BodyInstance");
}
inline BodyLayout* body_layout(Workspace& w,gears_anchor::Identity const& body){
    for(unsigned n=0;n<w.layout_count;++n)if(w.layouts[n].cls.address==body.cls)return &w.layouts[n];
    if(w.layout_count==w.layouts.size())return nullptr;
    auto& l=w.layouts[w.layout_count];auto& p=l.proof;
    if(!binding::certified(p,body.cls,l.cls)||!class_role(p,body.cls,L"RB_BodyInstance",w.body_class.address)||
       !role_property(p,body.cls,L"BodyIndex",L"IntProperty",4,nullptr,l.fields[0])||
       !role_property(p,body.cls,L"OwnerComponent",L"ComponentProperty",8,nullptr,l.fields[1])||!reference_class(p,l.fields[1])||
       !pointer_property(p,body.cls,L"BodyData",0x88,l.fields[2])||!pointer_property(p,body.cls,L"BoneSpring",0x90,l.fields[3]))return nullptr;
    ++w.layout_count;return &l;
}
inline bool layouts_current(Workspace const& w){
    if(!binding::metadata_current()||w.generation!=binding::metadata_generation||w.metadata!=binding::metadata_bytes||
       !binding::current(w.mesh)||!binding::current(w.instance))return false;
    for(unsigned n=0;n<w.layout_count;++n)if(!binding::current(w.layouts[n].proof))return false;
    return true;
}
inline Code values(Workspace& w,binding::Binding const& actor,pose::Metadata const& expected,Snapshot& out){
    Snapshot next{};
    if(!binding::identity(actor.mesh.address,next.mesh,true)||!binding::same(next.mesh,actor.mesh)||
       !binding::identity(actor.pawn.address,next.pawn,true)||!binding::same(next.pawn,actor.pawn)||
       !binding::identity(expected.physics_asset,next.asset,true)||!binding::identity(expected.physics_instance,next.instance,true))return Code::read;
    if(!schema(w,next))return Code::layout;
    std::uintptr_t asset{},instance{},owner{};
    if(!binding::field(asset,next.mesh.address,w.mesh_fields[0].offset)||asset!=next.asset.address||
       !binding::field(instance,next.mesh.address,w.mesh_fields[1].offset)||instance!=next.instance.address||
       !binding::field(owner,instance,w.instance_fields[0].offset)||!binding::identity(owner,next.owner,true)||
       (!binding::same(next.owner,next.mesh)&&!binding::same(next.owner,next.pawn)))return Code::changed;
    if(!class_role(w.instance,next.owner.cls,binding::same(next.owner,next.mesh)?L"SkeletalMeshComponent":L"Pawn",
        w.instance_fields[0].reference.address))return Code::layout;
    if(!binding::field(next.array,instance,w.instance_fields[1].offset)||!binding::field(next.root,instance,w.instance_fields[2].offset))return Code::read;
    const auto& h=next.array;
    if(h.count<=0||h.count>64||h.capacity<h.count||h.capacity>256||next.root<0||next.root>=h.count||
       !query_source::pointer(h.data,std::size_t(h.capacity)*8))return Code::layout;
    std::array<std::uintptr_t,64> pointers{};
    if(!safe_copy(pointers.data(),reinterpret_cast<void const*>(h.data),std::size_t(h.count)*8))return Code::read;
    for(int n=0;n<h.count;++n){auto& body=next.bodies[unsigned(n)];
        for(int k=0;k<n;++k)if(pointers[unsigned(k)]==pointers[unsigned(n)])return Code::layout;
        if(!binding::identity(pointers[unsigned(n)],body.identity,true))return Code::read;
        auto* l=body_layout(w,body.identity);if(!l)return w.layout_count==w.layouts.size()?Code::capacity:Code::layout;
        if(!class_role(l->proof,next.mesh.cls,L"SkeletalMeshComponent",l->fields[1].reference.address))return Code::layout;
        if(!binding::field(body.index,body.identity.address,l->fields[0].offset)||
           !binding::field(body.owner,body.identity.address,l->fields[1].offset)||
           !binding::field(body.main,body.identity.address,l->fields[2].offset)||
           !binding::field(body.auxiliary,body.identity.address,l->fields[3].offset))return Code::read;
        if(body.index!=n||body.owner!=next.mesh.address||(body.main&&!query_source::pointer(body.main,8))||
           (body.auxiliary&&!query_source::pointer(body.auxiliary,8)))return Code::changed;
    }
    if(!next.bodies[unsigned(next.root)].main)return Code::changed;
    next.source={next.mesh.address,next.asset.address,next.instance.address,unsigned(next.root),GetCurrentThreadId()};
    out=next;return Code::current;
}
template<class Guard> Code sample(binding::Binding const& actor,pose::Metadata const& metadata,Guard guard,Workspace& work,Snapshot& output) noexcept {
    if(!actor.context_valid||!actor.world_generation||!metadata.physics_asset||!metadata.physics_instance||
       query_source::aliases(&work,sizeof work,&output,sizeof output)||query_source::aliases(&actor,sizeof actor,&output,sizeof output)||
       query_source::aliases(&metadata,sizeof metadata,&output,sizeof output)||query_source::aliases(&work,sizeof work,&actor,sizeof actor)||
       query_source::aliases(&work,sizeof work,&metadata,sizeof metadata))return Code::request;
    const auto owner=actor;const auto expected=metadata;
    try{
        if(!guard())return Code::guard;
        {
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return Code::metadata;
            if(!binding::context_ready()||!binding::metadata_current()||actor.world_generation!=binding::metadata_generation)return Code::metadata;
            work.mesh={};work.instance={};work.layout_count=0;for(auto& l:work.layouts)l={};
            work.metadata=binding::metadata_bytes;work.generation=binding::metadata_generation;
            auto code=values(work,owner,expected,work.first);if(code!=Code::current)return code;
            code=values(work,owner,expected,work.last);if(code!=Code::current)return code;
            if(!same(work.first,work.last)||!layouts_current(work))return Code::changed;
        }
        if(!guard())return Code::guard;
        if(!binding::same_key(actor,owner)||actor.scene_invocation!=owner.scene_invocation||actor.special_move!=owner.special_move||
           !pose::metadata_equal(metadata,expected))return Code::changed;
        output=work.first;return Code::current;
    }catch(...){return Code::exception;}
}
#else
struct Workspace {};
template<class Guard> Code sample(binding::Binding const&,pose::Metadata const&,Guard,Workspace&,Snapshot&) noexcept{return Code::unavailable;}
#endif
} // namespace cover_physics_roles_live
#endif
