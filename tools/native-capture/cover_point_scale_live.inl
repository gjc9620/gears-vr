#ifndef GEARS_COVER_POINT_SCALE_LIVE_INCLUDED
#define GEARS_COVER_POINT_SCALE_LIVE_INCLUDED
#include "cover_asset_setup_live.inl"
// Reflected producer for retained original point-query route3a6c34. Caller
// holds metadata/GT lock and certifies that code/route, roles and C0 lifetime.
// This does not infer active PhysX scale from authored component fields.
namespace cover_point_scale_live {
namespace b=weapon_hand_scene_binding;namespace roles=cover_physics_roles_live;
namespace asset=cover_asset_setup_live;namespace a=gears_cover_authored_array;
namespace m=gears_cover_primitive_metrics;
struct Snapshot {gears_anchor::Identity mesh{},owner{};m::PointScale input{};std::array<float,3> value{};};
inline bool same(Snapshot const& x,Snapshot const& y){return b::same(x.mesh,y.mesh)&&b::same(x.owner,y.owner)&&
    x.input.component_scalar==y.input.component_scalar&&x.input.component_xyz==y.input.component_xyz&&
    x.input.owner_present==y.input.owner_present&&x.input.owner_scalar==y.input.owner_scalar&&x.input.owner_xyz==y.input.owner_xyz&&x.value==y.value;}
inline bool vector(b::Proof& proof,gears_anchor::Identity const& object,wchar_t const* name,unsigned offset,std::array<float,3>& value){
    b::Property property{};std::uintptr_t target{};gears_anchor::Identity type{};
    if(!roles::role_property(proof,object.cls,name,L"StructProperty",12,nullptr,property)||property.offset!=offset||
       !b::source_field(proof,target,property.id.address,0xd0)||!b::certified(proof,target,type)||!b::named(type,L"Vector"))return false;
    const wchar_t* axes[]={L"X",L"Y",L"Z"};
    for(unsigned n=0;n<3;++n){b::Property axis{};
      if(!cover_struct_field_live::select(proof,type,12,axes[n],L"FloatProperty",4,n*4,nullptr,axis)||
         !b::source_field(proof,value[n],object.address,offset+n*4))return false;}
    return true;
}
inline bool scalar(b::Proof& proof,gears_anchor::Identity const& object,wchar_t const* name,unsigned offset,float& value){
    b::Property property{};return roles::role_property(proof,object.cls,name,L"FloatProperty",4,nullptr,property)&&
      property.offset==offset&&b::source_field(proof,value,object.address,offset);
}
inline bool prepare(b::Proof& proof,gears_anchor::Identity const& mesh,Snapshot& output){
    Snapshot next{};b::Property owner_field{};std::uintptr_t owner_address{};
    if(!b::metadata_current()||!b::certified(proof,mesh.address,next.mesh)||!b::same(mesh,next.mesh)||
       !std::wcsncmp(next.mesh.name.data(),L"Default__",9)||!roles::class_role(proof,mesh.cls,L"SkeletalMeshComponent")||
       !roles::role_property(proof,mesh.cls,L"Owner",L"ObjectProperty",8,L"Actor",owner_field)||owner_field.offset!=0x78||
       !b::source_field(proof,owner_address,mesh.address,owner_field.offset)||
       !scalar(proof,next.mesh,L"Scale",0x1e8,next.input.component_scalar)||!vector(proof,next.mesh,L"Scale3D",0x1ec,next.input.component_xyz))return false;
    next.input.owner_present=owner_address!=0;
    if(owner_address&&(!b::certified(proof,owner_address,next.owner)||!std::wcsncmp(next.owner.name.data(),L"Default__",9)||
       !roles::class_role(proof,next.owner.cls,L"Actor",owner_field.reference.address)||
       !scalar(proof,next.owner,L"DrawScale",0x198,next.input.owner_scalar)||!vector(proof,next.owner,L"DrawScale3D",0x19c,next.input.owner_xyz)))return false;
    if(m::point_scale(next.input,next.value)!=m::Code::computed||!b::current(proof)||!b::metadata_current())return false;
    output=next;return true;
}
template<class Read,class Guard> a::Code capture(Snapshot const& scale,b::Proof const& producer,
    asset::Snapshot const& inventory,b::Proof const& membership,cover_body_setup_live::Layout const& layout,
    b::Proof const& shape,a::Source const& source,Read read,Guard guard,a::Workspace& work,a::Snapshot& output){
    if(a::s::aliases(&scale,sizeof scale,&work,sizeof work)||a::s::aliases(&scale,sizeof scale,&output,sizeof output)||
       a::s::aliases(&producer,sizeof producer,&work,sizeof work)||a::s::aliases(&producer,sizeof producer,&output,sizeof output))return a::Code::alias;
    const auto frozen=scale;
    if(!b::same(scale.mesh,inventory.mesh)||source.receiver_scale!=scale.value)return a::Code::request;
    auto current=[&](a::Source const& candidate){return guard(candidate)&&same(scale,frozen)&&b::current(producer)&&candidate.receiver_scale==frozen.value;};
    return asset::capture(inventory,membership,layout,shape,source,read,current,work,output);
}
}
#endif
