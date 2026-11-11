#ifndef GEARS_COVER_PHYSICS_ROLES_LIVE_INCLUDED
#define GEARS_COVER_PHYSICS_ROLES_LIVE_INCLUDED
#include "cover_physx_query_source.h"
#include "cover_physx_asset_actors.h"
#include "cover_member_witness_live.inl"

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
inline bool matches_asset_actors(Snapshot const& typed,gears_cover_physx_asset_actors::Snapshot const& raw){
    namespace actors=gears_cover_physx_asset_actors;
    if(!matches_query_source(typed,raw.query_source)||raw.body_count!=unsigned(typed.array.count)||raw.actor_count>raw.body_count)return false;
    std::array<std::uintptr_t,64> expected{};unsigned unique{};
    for(unsigned i=0;i<64;++i){auto const& body=raw.bodies[i];
        if(i>=raw.body_count){if(body.presence!=actors::Presence::not_sampled||body.body||body.actor||body.table||body.scene||body.internal)return false;continue;}
        if(body.body!=typed.bodies[i].identity.address||body.actor!=typed.bodies[i].main)return false;
        if(!body.actor){if(body.presence!=actors::Presence::body_data_field_null||body.table||body.scene||body.internal)return false;continue;}
        if(body.presence!=actors::Presence::actor_observed||body.table!=raw.query_source.actor_table||body.scene!=raw.query_source.scene||
           (body.internal&&!query_source::pointer(body.internal,0x18)))return false;
        bool duplicate=false;for(unsigned n=0;n<unique;++n)duplicate=duplicate||expected[n]==body.actor;
        if(!duplicate)expected[unique++]=body.actor;
    }
    return unique==raw.actor_count&&expected==raw.actors;
}
enum class Code {current,unavailable,request,guard,metadata,layout,read,changed,capacity,exception};
// Diagnostic only: last attempted predicate, never a query or movement permit.
enum class LayoutStep:unsigned {
    idle,
    mesh_class,
    asset_class,
    instance_class,
    mesh_asset_property,
    mesh_instance_property,
    mesh_asset_offset,
    mesh_instance_offset,
    asset_reference,
    instance_reference,
    owner_property,
    bodies_property,
    root_property,
    owner_reference,
    bodies_offset,
    inner_pointer,
    inner_identity,
    inner_number,
    inner_owner,
    inner_kind,
    inner_dimension,
    inner_width,
    inner_offset,
    body_class_pointer,
    body_class_identity,
    body_class_kind,
    body_class_name,
    owner_class,
    array_bounds,
    body_duplicate,
    body_identity,
    body_layout_capacity,
    body_layout_identity,
    body_layout_class,
    body_index_property,
    body_owner_property,
    body_owner_reference,
    body_data_property,
    body_spring_property,
    body_mesh_reference,
    complete,
};
struct LayoutObservation {
    LayoutStep step{};unsigned pass{},body_index{~0u};
    unsigned mesh_sources{},mesh_identities{},instance_sources{},instance_identities{};
    unsigned mesh_asset_offset{},mesh_instance_offset{},owner_offset{},bodies_offset{},root_offset{};
    unsigned member_identities{};bool member_capacity{};
};
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
struct BodyLayout {
    binding::Proof proof{};gears_anchor::Identity cls{};std::array<binding::Property,4> fields{};
    std::uintptr_t mesh_role_class{},mesh_role_reference{};unsigned mesh_role_pass{};
};
using MemberStore=cover_member_witness_live::Store;
struct Workspace {
    LayoutObservation witness{};
    MemberStore members{};
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
    binding::Binding sample_owner{};pose::Metadata sample_metadata{};bool recheck_ready{};
};
inline bool class_role(binding::Proof& proof,std::uintptr_t cls,wchar_t const* name,std::uintptr_t reference=0){
    binding::ClassPath path{};if(!binding::derives(cls,name,reference,path))return false;
    for(unsigned n=0;n<path.count;++n){auto wanted=path.ids[n];const auto parent=wanted.super;wanted.super=0;wanted.check_super=0;
        gears_anchor::Identity actual{};std::uintptr_t next{};
        if(!binding::certified(proof,wanted.address,actual)||!binding::same(wanted,actual)||!binding::kind(proof,actual,L"Class")||
           !binding::source_field(proof,next,wanted.address,binding::metadata.field_super)||next!=parent)return false;}
    return true;
}
inline bool body_mesh_role(BodyLayout& layout,std::uintptr_t mesh_class,unsigned pass){
    const auto reference=layout.fields[1].reference.address;
    // One successful certification per exact layout/key and values pass.
    // sample() clears ALL layouts before pass1; pass2 independently certifies
    // again. Every class path/source remains in proof for layouts_current().
    // This skips only repeated same-pass derivation for sibling body instances,
    // never their identity, owner, pointers, indices or the closing proof.
    if(pass&&layout.mesh_role_pass==pass&&layout.mesh_role_class==mesh_class&&
       layout.mesh_role_reference==reference)return true;
    if(!class_role(layout.proof,mesh_class,L"SkeletalMeshComponent",reference))return false;
    layout.mesh_role_class=mesh_class;layout.mesh_role_reference=reference;layout.mesh_role_pass=pass;
    return true;
}
// Close every sibling, including those AFTER the selected property. The common
// property helper only needs the selected descriptor; source consumers here
// additionally retain full declaring-owner membership/name witnesses.
inline bool membership(binding::Proof& p,binding::Property const& selected,MemberStore* members=nullptr){
    std::uintptr_t child{};if(!binding::source_field(p,child,selected.owner.address,0x80))return false;
    const auto head=child;const auto first=members?members->count:0;
    if(members){bool present{};const bool matched=members->match_chain(selected.owner.address,head,selected.id,present);
        if(present)return matched;}
    std::array<std::uintptr_t,4096> visited{};unsigned count{},matches{};
    while(child&&count<visited.size()){
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
        gears_anchor::Identity id{};
        if(!(members?binding::identity(child,id):binding::certified(p,child,id))||id.outer!=selected.owner.address)return false;
        if(!std::wcscmp(id.name.data(),selected.id.name.data())){
            if(++matches!=1||!binding::same(id,selected.id))return false;}
        if(members){if(!members->link(id,child))return false;}
        else if(!binding::source_field(p,child,child,0x68))return false;
    }
    return !child&&matches==1&&(!members||members->finish_chain(selected.owner.address,head,first,count));
}
inline bool role_property(binding::Proof& p,std::uintptr_t cls,wchar_t const* name,wchar_t const* kind,unsigned width,
    wchar_t const* reference,binding::Property& output,MemberStore* members=nullptr){
    return binding::property(p,cls,name,kind,width,reference,output)&&membership(p,output,members);
}
inline bool pointer_property(binding::Proof& p,std::uintptr_t cls,wchar_t const* name,unsigned offset,binding::Property& output,MemberStore* members=nullptr){
    std::uintptr_t at{};gears_anchor::Identity type{};unsigned extent{};
    return role_property(p,cls,name,L"StructProperty",8,nullptr,output,members)&&output.offset==offset&&
        binding::source_field(p,at,output.id.address,0xd0)&&binding::certified(p,at,type)&&binding::named(type,L"Pointer")&&
        binding::kind(p,type,L"ScriptStruct")&&binding::source_field(p,extent,at,0x88)&&extent==8;
}
inline bool reference_class(binding::Proof& p,binding::Property& field){
    std::uintptr_t at{};return binding::source_field(p,at,field.id.address,0xd0)&&
        binding::certified(p,at,field.reference)&&binding::kind(p,field.reference,L"Class");
}
inline bool step(Workspace& w,LayoutStep value){w.witness.step=value;return true;}
inline void step(LayoutObservation* witness,LayoutStep value){if(witness)witness->step=value;}
inline LayoutObservation observation(Workspace const& w)noexcept{
    auto out=w.witness;
    if(!out.pass)return out; // No layout read this sample; never expose prior proof counts/offsets.
    out.mesh_sources=w.mesh.source_count;out.mesh_identities=w.mesh.identity_count;
    out.instance_sources=w.instance.source_count;out.instance_identities=w.instance.identity_count;
    out.mesh_asset_offset=w.mesh_fields[0].offset;out.mesh_instance_offset=w.mesh_fields[1].offset;
    out.owner_offset=w.instance_fields[0].offset;out.bodies_offset=w.instance_fields[1].offset;out.root_offset=w.instance_fields[2].offset;
    out.member_identities=w.members.count;out.member_capacity=w.members.exhausted;
    return out;
}
inline bool schema(Workspace& w,Snapshot const& next){
    auto& m=w.mesh;auto& i=w.instance;
    step(w,LayoutStep::mesh_class);if(!(class_role(m,next.mesh.cls,L"SkeletalMeshComponent")))return false;
    step(w,LayoutStep::asset_class);if(!(class_role(m,next.asset.cls,L"PhysicsAsset")))return false;
    step(w,LayoutStep::instance_class);if(!(class_role(i,next.instance.cls,L"PhysicsAssetInstance")))return false;
    step(w,LayoutStep::mesh_asset_property);if(!(role_property(m,next.mesh.cls,L"PhysicsAsset",L"ObjectProperty",8,L"PhysicsAsset",w.mesh_fields[0],&w.members)))return false;
    step(w,LayoutStep::mesh_instance_property);if(!(role_property(m,next.mesh.cls,L"PhysicsAssetInstance",L"ObjectProperty",8,L"PhysicsAssetInstance",w.mesh_fields[1],&w.members)))return false;
    step(w,LayoutStep::mesh_asset_offset);if(!(w.mesh_fields[0].offset==0x248))return false;
    step(w,LayoutStep::mesh_instance_offset);if(!(w.mesh_fields[1].offset==0x250))return false;
    step(w,LayoutStep::asset_reference);if(!(class_role(m,next.asset.cls,L"PhysicsAsset",w.mesh_fields[0].reference.address)))return false;
    step(w,LayoutStep::instance_reference);if(!(class_role(i,next.instance.cls,L"PhysicsAssetInstance",w.mesh_fields[1].reference.address)))return false;
    step(w,LayoutStep::owner_property);if(!(role_property(i,next.instance.cls,L"Owner",L"MeshPointer",8,nullptr,w.instance_fields[0],&w.members)))return false;
    step(w,LayoutStep::bodies_property);if(!(role_property(i,next.instance.cls,L"Bodies",L"ArrayProperty",16,nullptr,w.instance_fields[1],&w.members)))return false;
    step(w,LayoutStep::root_property);if(!(role_property(i,next.instance.cls,L"RootBodyIndex",L"IntProperty",4,nullptr,w.instance_fields[2],&w.members)))return false;
    step(w,LayoutStep::owner_reference);if(!(reference_class(i,w.instance_fields[0])))return false;
    step(w,LayoutStep::bodies_offset);if(!(w.instance_fields[1].offset==0x6c))return false;
    auto const& array=w.instance_fields[1];std::uintptr_t inner{},target{};gears_anchor::Identity id{};unsigned dim{},width{},offset{};
    step(w,LayoutStep::inner_pointer);if(!(binding::source_field(i,inner,array.id.address,0xd0)))return false;
    step(w,LayoutStep::inner_identity);if(!(binding::certified(i,inner,id)))return false;
    step(w,LayoutStep::inner_number);if(!(!id.number))return false;
    step(w,LayoutStep::inner_owner);if(!(id.outer==array.id.address))return false;
    step(w,LayoutStep::inner_kind);if(!(binding::kind(i,id,L"ObjectProperty")))return false;
    step(w,LayoutStep::inner_dimension);if(!(binding::source_field(i,dim,inner,0x70)&&dim==1))return false;
    step(w,LayoutStep::inner_width);if(!(binding::source_field(i,width,inner,0x74)&&width==8))return false;
    step(w,LayoutStep::inner_offset);if(!(binding::source_field(i,offset,inner,0x94)&&!offset))return false;
    step(w,LayoutStep::body_class_pointer);if(!(binding::source_field(i,target,inner,0xd0)))return false;
    step(w,LayoutStep::body_class_identity);if(!(binding::certified(i,target,w.body_class)))return false;
    step(w,LayoutStep::body_class_kind);if(!(binding::kind(i,w.body_class,L"Class")))return false;
    step(w,LayoutStep::body_class_name);if(!(binding::named(w.body_class,L"RB_BodyInstance")))return false;
    return true;
}

template<class Work> inline BodyLayout* body_layout(Work& w,gears_anchor::Identity const& body,LayoutObservation* witness=nullptr,MemberStore* members=nullptr){
    for(unsigned n=0;n<w.layout_count;++n)if(w.layouts[n].cls.address==body.cls)return &w.layouts[n];
    step(witness,LayoutStep::body_layout_capacity);if(w.layout_count==w.layouts.size())return nullptr;
    auto& l=w.layouts[w.layout_count];auto& p=l.proof;
    step(witness,LayoutStep::body_layout_identity);if(!(binding::certified(p,body.cls,l.cls)))return nullptr;
    step(witness,LayoutStep::body_layout_class);if(!(class_role(p,body.cls,L"RB_BodyInstance",w.body_class.address)))return nullptr;
    step(witness,LayoutStep::body_index_property);if(!(role_property(p,body.cls,L"BodyIndex",L"IntProperty",4,nullptr,l.fields[0],members)))return nullptr;
    step(witness,LayoutStep::body_owner_property);if(!(role_property(p,body.cls,L"OwnerComponent",L"ComponentProperty",8,nullptr,l.fields[1],members)))return nullptr;
    step(witness,LayoutStep::body_owner_reference);if(!(reference_class(p,l.fields[1])))return nullptr;
    step(witness,LayoutStep::body_data_property);if(!(pointer_property(p,body.cls,L"BodyData",0x88,l.fields[2],members)))return nullptr;
    step(witness,LayoutStep::body_spring_property);if(!(pointer_property(p,body.cls,L"BoneSpring",0x90,l.fields[3],members)))return nullptr;
    ++w.layout_count;return &l;
}
inline bool layouts_current(Workspace const& w){
    if(!binding::metadata_current()||w.generation!=binding::metadata_generation||w.metadata!=binding::metadata_bytes||
       !binding::current(w.mesh)||!binding::current(w.instance)||!w.members.current())return false;
    for(unsigned n=0;n<w.layout_count;++n)if(!binding::current(w.layouts[n].proof))return false;
    return true;
}
inline Code values(Workspace& w,binding::Binding const& actor,pose::Metadata const& expected,Snapshot& out,bool repeat=false){
    Snapshot next{};
    if(!binding::identity(actor.mesh.address,next.mesh,true)||!binding::same(next.mesh,actor.mesh)||
       !binding::identity(actor.pawn.address,next.pawn,true)||!binding::same(next.pawn,actor.pawn)||
       !binding::identity(expected.physics_asset,next.asset,true)||!binding::identity(expected.physics_instance,next.instance,true))return Code::read;
    if(repeat){
        // Only sample's second values pass reuses the already selected schema.
        // Revalidate its complete identities/source fields BEFORE consuming
        // offsets, then retain the original complete closing proof below.
        if(!binding::same(next.mesh,w.first.mesh)||!binding::same(next.asset,w.first.asset)||
           !binding::same(next.instance,w.first.instance)||!binding::metadata_current()||
           w.generation!=binding::metadata_generation||w.metadata!=binding::metadata_bytes||
           !binding::current(w.mesh)||!binding::current(w.instance))return Code::changed;
        for(unsigned n=0;n<w.layout_count;++n)if(!binding::current(w.layouts[n].proof))return Code::changed;
    }else if(!schema(w,next))return Code::layout;
    std::uintptr_t asset{},instance{},owner{};
    if(!binding::source_field(w.mesh,asset,next.mesh.address,w.mesh_fields[0].offset)||asset!=next.asset.address||
       !binding::source_field(w.mesh,instance,next.mesh.address,w.mesh_fields[1].offset)||instance!=next.instance.address||
       !binding::source_field(w.instance,owner,instance,w.instance_fields[0].offset)||!binding::identity(owner,next.owner,true)||
       (!binding::same(next.owner,next.mesh)&&!binding::same(next.owner,next.pawn)))return Code::changed;
    step(w,LayoutStep::owner_class);
    // A repeat already re-read the complete instance/class-path proof above.
    // The fresh owner identity must be exactly the initial owner; no class
    // derivation is cached across samples or used for a changed owner/class.
    if(repeat){if(!binding::same(next.owner,w.first.owner))return Code::changed;}
    else if(!class_role(w.instance,next.owner.cls,binding::same(next.owner,next.mesh)?L"SkeletalMeshComponent":L"Pawn",
        w.instance_fields[0].reference.address))return Code::layout;
    const auto array_offset=w.instance_fields[1].offset;
    if(!binding::source_field(w.instance,next.array.data,instance,array_offset)||
       !binding::source_field(w.instance,next.array.count,instance,array_offset+8)||
       !binding::source_field(w.instance,next.array.capacity,instance,array_offset+12)||
       !binding::source_field(w.instance,next.root,instance,w.instance_fields[2].offset))return Code::read;
    const auto& h=next.array;step(w,LayoutStep::array_bounds);
    if(h.count<=0||h.count>64||h.capacity<h.count||h.capacity>256||next.root<0||next.root>=h.count||
       !query_source::pointer(h.data,std::size_t(h.capacity)*8))return Code::layout;
    std::array<std::uintptr_t,64> pointers{};
    if(!safe_copy(pointers.data(),reinterpret_cast<void const*>(h.data),std::size_t(h.count)*8))return Code::read;
    for(int n=0;n<h.count;++n){auto& body=next.bodies[unsigned(n)];
        std::uintptr_t member{};
        if(!binding::source_field(w.instance,member,h.data,unsigned(n)*8)||member!=pointers[unsigned(n)])return Code::changed;
        w.witness.body_index=unsigned(n);step(w,LayoutStep::body_duplicate);
        for(int k=0;k<n;++k)if(pointers[unsigned(k)]==pointers[unsigned(n)])return Code::layout;
        step(w,LayoutStep::body_identity);
        if(!binding::identity(pointers[unsigned(n)],body.identity,true))return Code::read;
        auto* l=body_layout(w,body.identity,&w.witness,&w.members);if(!l)return w.layout_count==w.layouts.size()?Code::capacity:Code::layout;
        step(w,LayoutStep::body_mesh_reference);
        if(repeat){
            if(l->mesh_role_class!=next.mesh.cls||l->mesh_role_reference!=l->fields[1].reference.address||!l->mesh_role_pass)return Code::changed;
        }else if(!body_mesh_role(*l,next.mesh.cls,w.witness.pass))return Code::layout;
        if(!binding::source_field(l->proof,body.index,body.identity.address,l->fields[0].offset)||
           !binding::source_field(l->proof,body.owner,body.identity.address,l->fields[1].offset)||
           !binding::source_field(l->proof,body.main,body.identity.address,l->fields[2].offset)||
           !binding::source_field(l->proof,body.auxiliary,body.identity.address,l->fields[3].offset))return Code::read;
        if(body.index!=n||body.owner!=next.mesh.address||(body.main&&!query_source::pointer(body.main,8))||
           (body.auxiliary&&!query_source::pointer(body.auxiliary,8)))return Code::changed;
    }
    if(!next.bodies[unsigned(next.root)].main)return Code::changed;
    next.source={next.mesh.address,next.asset.address,next.instance.address,unsigned(next.root),GetCurrentThreadId()};
    step(w,LayoutStep::complete);out=next;return Code::current;
}
template<class Guard> Code sample(binding::Binding const& actor,pose::Metadata const& metadata,Guard guard,Workspace& work,Snapshot& output) noexcept {
    if(!actor.context_valid||!actor.world_generation||!metadata.physics_asset||!metadata.physics_instance||
       query_source::aliases(&work,sizeof work,&output,sizeof output)||query_source::aliases(&actor,sizeof actor,&output,sizeof output)||
       query_source::aliases(&metadata,sizeof metadata,&output,sizeof output)||query_source::aliases(&work,sizeof work,&actor,sizeof actor)||
       query_source::aliases(&work,sizeof work,&metadata,sizeof metadata))return Code::request;
    work.witness={};
    work.recheck_ready=false;
    const auto owner=actor;const auto expected=metadata;
    try{
        if(!guard())return Code::guard;
        {
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return Code::metadata;
            if(!binding::context_ready()||!binding::metadata_current()||actor.world_generation!=binding::metadata_generation)return Code::metadata;
            work.members.clear();work.mesh={};work.instance={};work.mesh_fields={};work.instance_fields={};work.body_class={};work.layout_count=0;for(auto& l:work.layouts)l={};
            work.metadata=binding::metadata_bytes;work.generation=binding::metadata_generation;
            work.witness.pass=1;auto code=values(work,owner,expected,work.first);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            work.witness.pass=2;work.witness.body_index=~0u;code=values(work,owner,expected,work.last,true);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            if(!same(work.first,work.last)||!layouts_current(work))return Code::changed;
        }
        if(!guard())return Code::guard;
        if(!binding::same_key(actor,owner)||actor.scene_invocation!=owner.scene_invocation||actor.special_move!=owner.special_move||
           !pose::metadata_equal(metadata,expected))return Code::changed;
        output=work.first;work.sample_owner=owner;work.sample_metadata=expected;work.recheck_ready=true;return Code::current;
    }catch(...){return Code::exception;}
}
// Only the serialized Session SourceGuard may reuse the initial successful
// sample. The next execute always calls sample() and rebuilds all layouts.
// Rechecks retain two actual dynamic observations, complete prior Proof checks
// before selected offsets, comparison with the initial result and final close.
template<class Guard> Code recheck(binding::Binding const& actor,pose::Metadata const& metadata,Guard guard,Workspace& work,Snapshot& output)noexcept{
    if(!actor.context_valid||!actor.world_generation||!metadata.physics_asset||!metadata.physics_instance||
       query_source::aliases(&work,sizeof work,&output,sizeof output)||query_source::aliases(&actor,sizeof actor,&output,sizeof output)||
       query_source::aliases(&metadata,sizeof metadata,&output,sizeof output)||query_source::aliases(&work,sizeof work,&actor,sizeof actor)||
       query_source::aliases(&work,sizeof work,&metadata,sizeof metadata))return Code::request;
    const auto owner=actor;const auto expected=metadata;const bool prepared=work.recheck_ready;work.recheck_ready=false;
    try{
        if(!guard())return Code::guard;
        if(!prepared||!binding::same_key(owner,work.sample_owner)||owner.scene_invocation!=work.sample_owner.scene_invocation||
           owner.special_move!=work.sample_owner.special_move||!pose::metadata_equal(expected,work.sample_metadata))return Code::changed;
        {
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return Code::metadata;
            if(!binding::context_ready()||!binding::metadata_current()||actor.world_generation!=binding::metadata_generation)return Code::metadata;
            work.witness.pass=1;work.witness.body_index=~0u;
            auto code=values(work,owner,expected,work.last,true);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            if(!same(work.first,work.last))return Code::changed;
            work.witness.pass=2;work.witness.body_index=~0u;
            code=values(work,owner,expected,work.last,true);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            if(!same(work.first,work.last)||!layouts_current(work))return Code::changed;
        }
        if(!guard())return Code::guard;
        if(!binding::same_key(actor,owner)||actor.scene_invocation!=owner.scene_invocation||actor.special_move!=owner.special_move||
           !pose::metadata_equal(metadata,expected))return Code::changed;
        output=work.last;work.recheck_ready=true;return Code::current;
    }catch(...){return Code::exception;}
}
#else
struct Workspace {};
inline LayoutObservation observation(Workspace const&)noexcept{return {};}
template<class Guard> Code sample(binding::Binding const&,pose::Metadata const&,Guard,Workspace&,Snapshot&) noexcept{return Code::unavailable;}
template<class Guard> Code recheck(binding::Binding const&,pose::Metadata const&,Guard,Workspace&,Snapshot&) noexcept{return Code::unavailable;}
#endif
} // namespace cover_physics_roles_live
#endif
