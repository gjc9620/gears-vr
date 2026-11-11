#ifndef GEARS_COVER_SHAPE_CACHE_LIVE_INCLUDED
#define GEARS_COVER_SHAPE_CACHE_LIVE_INCLUDED
#include "cover_point_scale_live.inl"
#include "cover_physx_shape_cache.h"
// Caller owns binding lock for prepare; closes current asset/body membership,
// original creation route and C0 lifetime in observe's external typed guard.
namespace cover_shape_cache_live {
namespace point=cover_point_scale_live;namespace b=point::b;namespace roles=point::roles;
namespace cache=gears_cover_physx_shape_cache;namespace names=cover_body_setup_live::names;
struct Layout {gears_anchor::Identity setup{};names::Name bone{};b::Property bone_field{};
    std::array<b::Property,2> arrays{};std::array<gears_anchor::Identity,2> inner{},element{};cache::Source source{};};
inline bool prepare(b::Proof& proof,gears_anchor::Identity const& expected,names::Name bone,Layout& output){
    if(cache::s::aliases(&proof,sizeof proof,&output,sizeof output)||cache::s::aliases(&expected,sizeof expected,&output,sizeof output))return false;
    Layout next{};next.bone=bone;
    if(bone.index<=0||!b::metadata_current()||!b::certified(proof,expected.address,next.setup)||!b::same(expected,next.setup)||
       !std::wcsncmp(next.setup.name.data(),L"Default__",9)||!roles::class_role(proof,next.setup.cls,L"RB_BodySetup")||
       !roles::role_property(proof,next.setup.cls,L"BoneName",L"NameProperty",8,nullptr,next.bone_field)||next.bone_field.offset!=0xb4)return false;
    names::Name actual{};if(!b::source_field(proof,actual,next.setup.address,0xb4)||!names::same(actual,bone))return false;
    const wchar_t* fields[]={L"CollisionGeom",L"CollisionGeomScale3D"};const wchar_t* types[]={L"Pointer",L"Vector"};
    for(unsigned n=0;n<2;++n){auto& field=next.arrays[n];std::uintptr_t inner_address{},target{};unsigned dim{},width{},offset{},extent{};
      if(!roles::role_property(proof,next.setup.cls,fields[n],L"ArrayProperty",16,nullptr,field)||field.offset!=0xcc+16*n||
         !b::source_field(proof,inner_address,field.id.address,0xd0)||!b::certified(proof,inner_address,next.inner[n])||next.inner[n].number||
         next.inner[n].outer!=field.id.address||!b::kind(proof,next.inner[n],L"StructProperty")||
         !b::source_field(proof,dim,inner_address,0x70)||dim!=1||!b::source_field(proof,width,inner_address,0x74)||width!=(n?12u:8u)||
         !b::source_field(proof,offset,inner_address,0x94)||offset||!b::source_field(proof,target,inner_address,0xd0)||
         !b::certified(proof,target,next.element[n])||next.element[n].number||!b::kind(proof,next.element[n],L"ScriptStruct")||
         !b::named(next.element[n],types[n])||!b::source_field(proof,extent,target,0x88)||extent!=width)return false;
      if(n){const wchar_t* axes[]={L"X",L"Y",L"Z"};for(unsigned k=0;k<3;++k){b::Property axis{};
        if(!cover_struct_field_live::select(proof,next.element[n],12,axes[k],L"FloatProperty",4,k*4,nullptr,axis))return false;}}
    }
    next.source={next.setup.address,next.setup.address+0xcc,next.setup.address+0xdc};
    if(!cache::s::pointer(next.setup.address,0xec)||!b::current(proof)||!b::metadata_current())return false;
    output=next;return true;
}
template<class Read,class Guard> cache::Code observe(Layout const& layout,b::Proof const& proof,Read read,Guard guard,
    cache::Workspace& work,cache::Snapshot& output){
    if(cache::s::aliases(&layout,sizeof layout,&work,sizeof work)||cache::s::aliases(&layout,sizeof layout,&output,sizeof output)||
       cache::s::aliases(&proof,sizeof proof,&work,sizeof work)||cache::s::aliases(&proof,sizeof proof,&output,sizeof output))return cache::Code::alias;
    const auto frozen=layout;
    auto current=[&](cache::Source const& source){return guard(source)&&b::same(layout.setup,frozen.setup)&&
      names::same(layout.bone,frozen.bone)&&cache::same(layout.source,frozen.source)&&cache::same(source,frozen.source)&&
      b::metadata_current()&&b::current(proof);};
    return cache::observe(layout.source,read,current,work,output);
}
}
#endif
