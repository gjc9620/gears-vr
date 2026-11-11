#ifndef GEARS_COVER_ASSET_SETUP_LIVE_INCLUDED
#define GEARS_COVER_ASSET_SETUP_LIVE_INCLUDED
#include "cover_body_setup_live.inl"
// Read under the binding lock. The caller's typed guard closes the current
// roles snapshot/C0 lifetime; this module authenticates authored membership,
// not active PhysX shapes or movement permission.
namespace cover_asset_setup_live {
namespace b=weapon_hand_scene_binding;namespace roles=cover_physics_roles_live;
namespace body=cover_body_setup_live;namespace a=gears_cover_authored_array;
struct Row {gears_anchor::Identity setup{};body::names::Name bone{};};
struct Snapshot {gears_anchor::Identity mesh{},asset{},instance{};roles::pose::ArrayHeader array{};
    unsigned count{};std::array<Row,64> rows{};};
inline bool same(Snapshot const& x,Snapshot const& y){
    if(!b::same(x.mesh,y.mesh)||!b::same(x.asset,y.asset)||!b::same(x.instance,y.instance)||
       !roles::pose::header_equal(x.array,y.array)||x.count!=y.count)return false;
    for(unsigned n=0;n<64;++n)if(!b::same(x.rows[n].setup,y.rows[n].setup)||!body::names::same(x.rows[n].bone,y.rows[n].bone))return false;
    return true;
}
inline bool prepare(b::Proof& proof,roles::Snapshot const& expected,Snapshot& output){
    Snapshot next{};b::Property mesh_asset{},mesh_instance{},setups{},bodies{};
    if(!b::metadata_current()||expected.array.count<=0||expected.array.count>64||
       expected.root<0||expected.root>=expected.array.count||
       !b::certified(proof,expected.mesh.address,next.mesh)||!b::same(next.mesh,expected.mesh)||
       !b::certified(proof,expected.asset.address,next.asset)||!b::same(next.asset,expected.asset)||
       !b::certified(proof,expected.instance.address,next.instance)||!b::same(next.instance,expected.instance)||
       !std::wcsncmp(next.mesh.name.data(),L"Default__",9)||!std::wcsncmp(next.asset.name.data(),L"Default__",9)||
       !std::wcsncmp(next.instance.name.data(),L"Default__",9)||
       !roles::class_role(proof,next.mesh.cls,L"SkeletalMeshComponent")||
       !roles::class_role(proof,next.asset.cls,L"PhysicsAsset")||
       !roles::class_role(proof,next.instance.cls,L"PhysicsAssetInstance")||
       !roles::role_property(proof,next.mesh.cls,L"PhysicsAsset",L"ObjectProperty",8,L"PhysicsAsset",mesh_asset)||mesh_asset.offset!=0x248||
       !roles::role_property(proof,next.mesh.cls,L"PhysicsAssetInstance",L"ObjectProperty",8,L"PhysicsAssetInstance",mesh_instance)||mesh_instance.offset!=0x250||
       !roles::class_role(proof,next.asset.cls,L"PhysicsAsset",mesh_asset.reference.address)||
       !roles::class_role(proof,next.instance.cls,L"PhysicsAssetInstance",mesh_instance.reference.address))return false;
    std::uintptr_t asset{},instance{};
    if(!b::source_field(proof,asset,next.mesh.address,mesh_asset.offset)||asset!=next.asset.address||
       !b::source_field(proof,instance,next.mesh.address,mesh_instance.offset)||instance!=next.instance.address||
       !roles::role_property(proof,next.asset.cls,L"BodySetup",L"ArrayProperty",16,nullptr,setups)||setups.offset!=0x68||
       !roles::role_property(proof,next.instance.cls,L"Bodies",L"ArrayProperty",16,nullptr,bodies)||bodies.offset!=0x6c)return false;
    std::uintptr_t membership_inner{},target{};gears_anchor::Identity inner_id{},setup_class{};unsigned dimension{},width{},offset{};
    if(!b::source_field(proof,membership_inner,setups.id.address,0xd0)||!b::certified(proof,membership_inner,inner_id)||inner_id.number||inner_id.outer!=setups.id.address||
       !b::kind(proof,inner_id,L"ObjectProperty")||!b::source_field(proof,dimension,membership_inner,0x70)||dimension!=1||
       !b::source_field(proof,width,membership_inner,0x74)||width!=8||!b::source_field(proof,offset,membership_inner,0x94)||offset||
       !b::source_field(proof,target,membership_inner,0xd0)||!b::certified(proof,target,setup_class)||setup_class.number||
       !b::named(setup_class,L"RB_BodySetup")||!b::kind(proof,setup_class,L"Class"))return false;
    roles::pose::ArrayHeader live_bodies{};
    auto header=[&](std::uintptr_t header_base,unsigned at,roles::pose::ArrayHeader& h){return
      b::source_field(proof,h.data,header_base,at)&&b::source_field(proof,h.count,header_base,at+8)&&b::source_field(proof,h.capacity,header_base,at+12);};
    if(!header(next.asset.address,setups.offset,next.array)||!header(next.instance.address,bodies.offset,live_bodies)||
       !roles::pose::header_equal(live_bodies,expected.array)||next.array.count!=expected.array.count||
       next.array.capacity<next.array.count||next.array.capacity>256||
       !roles::query_source::pointer(next.array.data,std::size_t(next.array.capacity)*8)||
       live_bodies.capacity<live_bodies.count||live_bodies.capacity>256||
       !roles::query_source::pointer(live_bodies.data,std::size_t(live_bodies.capacity)*8))return false;
    next.count=unsigned(next.array.count);
    for(unsigned n=0;n<next.count;++n){std::uintptr_t member{},instance_member{};auto& row=next.rows[n];b::Property bone{};
      if(!b::source_field(proof,member,next.array.data,n*8)||!b::source_field(proof,instance_member,live_bodies.data,n*8)||
         instance_member!=expected.bodies[n].identity.address||expected.bodies[n].index!=int(n)||expected.bodies[n].owner!=next.mesh.address||
         !b::certified(proof,member,row.setup)||!std::wcsncmp(row.setup.name.data(),L"Default__",9)||
         !roles::class_role(proof,row.setup.cls,L"RB_BodySetup",setup_class.address)||
         !roles::role_property(proof,row.setup.cls,L"BoneName",L"NameProperty",8,nullptr,bone)||bone.offset!=0xb4||
         !b::source_field(proof,row.bone,member,bone.offset)||row.bone.index<=0)return false;
      for(unsigned k=0;k<n;++k)if(next.rows[k].setup.address==member)return false;
    }
    if(!b::current(proof)||!b::metadata_current())return false;
    output=next;return true;
}
template<class Read,class Guard> a::Code capture(Snapshot const& asset,b::Proof const& membership,
    body::Layout const& layout,b::Proof const& shape,a::Source const& source,Read read,Guard guard,
    a::Workspace& work,a::Snapshot& output){
    if(a::s::aliases(&asset,sizeof asset,&work,sizeof work)||a::s::aliases(&asset,sizeof asset,&output,sizeof output)||
       a::s::aliases(&membership,sizeof membership,&work,sizeof work)||a::s::aliases(&membership,sizeof membership,&output,sizeof output))return a::Code::alias;
    const auto frozen=asset;
    if(!asset.count||asset.count>64||source.body_index>=asset.count||
       !b::same(asset.rows[source.body_index].setup,layout.setup)||
       !body::names::same(asset.rows[source.body_index].bone,source.bone))return a::Code::request;
    auto current=[&](a::Source const& candidate){return guard(candidate)&&same(asset,frozen)&&b::current(membership);};
    return body::capture(layout,shape,source,read,current,work,output);
}
}
#endif
