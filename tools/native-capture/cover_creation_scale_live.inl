#ifndef GEARS_COVER_CREATION_SCALE_LIVE_INCLUDED
#define GEARS_COVER_CREATION_SCALE_LIVE_INCLUDED
#include "cover_point_scale_live.inl"
// Retained skeletal PhysicsAssetInstance creation route211900 ->2c3efc
// ->2cfa6c. Include/read under the existing binding lock. The external caller
// must attest that route and close roles/C0 lifetime. This is not proof that
// a cached descriptor or active shape was built from today's scale values.
namespace cover_creation_scale_live {
namespace point=cover_point_scale_live;namespace b=point::b;namespace roles=point::roles;
namespace a=point::a;namespace asset=point::asset;
struct Snapshot {point::Snapshot scale{};gears_anchor::Identity instance{};};
inline bool same(Snapshot const& x,Snapshot const& y){return point::same(x.scale,y.scale)&&b::same(x.instance,y.instance);}
inline bool prepare(b::Proof& proof,roles::Snapshot const& expected,Snapshot& output){
    if(a::s::aliases(&expected,sizeof expected,&output,sizeof output)||
       a::s::aliases(&proof,sizeof proof,&output,sizeof output))return false;
    Snapshot next{};b::Property instance_field{},owner_field{};std::uintptr_t instance{},owner{};
    if(!point::prepare(proof,expected.mesh,next.scale)||
       !b::certified(proof,expected.instance.address,next.instance)||!b::same(next.instance,expected.instance)||
       !std::wcsncmp(next.instance.name.data(),L"Default__",9)||
       !roles::class_role(proof,next.instance.cls,L"PhysicsAssetInstance")||
       !roles::role_property(proof,next.scale.mesh.cls,L"PhysicsAssetInstance",L"ObjectProperty",8,L"PhysicsAssetInstance",instance_field)||
       instance_field.offset!=0x250||!roles::class_role(proof,next.instance.cls,L"PhysicsAssetInstance",instance_field.reference.address)||
       !b::source_field(proof,instance,next.scale.mesh.address,instance_field.offset)||instance!=next.instance.address||
       !roles::role_property(proof,next.instance.cls,L"Owner",L"MeshPointer",8,nullptr,owner_field)||
       owner_field.offset!=0x60||!roles::reference_class(proof,owner_field)||
       !b::source_field(proof,owner,next.instance.address,owner_field.offset)||owner!=next.scale.owner.address||
       !b::same(next.scale.owner,expected.owner))return false;
    if(owner&&!roles::class_role(proof,next.scale.owner.cls,L"Actor",owner_field.reference.address))return false;
    if(!b::current(proof)||!b::metadata_current())return false;
    output=next;return true;
}
template<class Read,class Guard> a::Code capture(Snapshot const& scale,b::Proof const& producer,
    asset::Snapshot const& inventory,b::Proof const& membership,cover_body_setup_live::Layout const& layout,
    b::Proof const& shape,a::Source const& source,Read read,Guard guard,a::Workspace& work,a::Snapshot& output){
    if(a::s::aliases(&scale,sizeof scale,&work,sizeof work)||a::s::aliases(&scale,sizeof scale,&output,sizeof output)||
       a::s::aliases(&producer,sizeof producer,&work,sizeof work)||a::s::aliases(&producer,sizeof producer,&output,sizeof output))return a::Code::alias;
    const auto frozen=scale;
    if(!b::same(scale.scale.mesh,inventory.mesh)||!b::same(scale.instance,inventory.instance)||source.receiver_scale!=scale.scale.value)return a::Code::request;
    auto current=[&](a::Source const& candidate){return guard(candidate)&&same(scale,frozen)&&b::current(producer)&&candidate.receiver_scale==frozen.scale.value;};
    return asset::capture(inventory,membership,layout,shape,source,read,current,work,output);
}
}
#endif
