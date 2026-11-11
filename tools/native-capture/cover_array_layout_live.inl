#ifndef GEARS_COVER_ARRAY_LAYOUT_LIVE_INCLUDED
#define GEARS_COVER_ARRAY_LAYOUT_LIVE_INCLUDED
#include "cover_primitive_layout_live.inl"
// Reflected association only. Caller authenticates aggregate instance/base,
// native stride/code, current BodySetup/bone and C0 lifetime separately.
namespace cover_array_layout_live {
namespace b=weapon_hand_scene_binding;namespace primitive=cover_primitive_layout_live;
struct Layout {gears_anchor::Identity aggregate{},inner{};b::Property field{};primitive::Layout element{};unsigned stride{};};
inline bool prepare(b::Proof& proof,gears_anchor::Identity const& aggregate,
                    gears_anchor::Identity const& declaring_class,unsigned kind,Layout& output){
    if(kind>2)return false;gears_anchor::Identity actual{},owner{};
    if(!b::metadata_current()||!b::certified(proof,declaring_class.address,owner)||!b::same(owner,declaring_class)||owner.number||
       !b::named(owner,L"KMeshProps")||!b::kind(proof,owner,L"Class")||
       !b::certified(proof,aggregate.address,actual)||!b::same(actual,aggregate)||actual.number||
       actual.outer!=owner.address||!b::named(actual,L"KAggregateGeom"))return false;
    const wchar_t* names[]={L"SphereElems",L"BoxElems",L"SphylElems"};Layout next{};next.aggregate=actual;
    if(!cover_struct_field_live::select(proof,actual,72,names[kind],L"ArrayProperty",16,16*kind,nullptr,next.field))return false;
    std::uintptr_t inner_address{},target{};unsigned dimension{},offset{};gears_anchor::Identity element{};
    if(!b::source_field(proof,inner_address,next.field.id.address,0xd0)||!b::certified(proof,inner_address,next.inner)||
       next.inner.number||next.inner.outer!=next.field.id.address||!b::kind(proof,next.inner,L"StructProperty")||
       !b::source_field(proof,dimension,inner_address,0x70)||dimension!=1||
       !b::source_field(proof,next.stride,inner_address,0x74)||next.stride!=80||
       !b::source_field(proof,offset,inner_address,0x94)||offset||
       !b::source_field(proof,target,inner_address,0xd0)||!b::certified(proof,target,element)||
       !primitive::prepare(proof,element,owner,kind,next.element)||!b::current(proof)||!b::metadata_current())return false;
    output=next;return true;
}
// Requires cover_authored_array.h included globally, as in the Native prefix.
// aggregate_base must be authenticated by the caller's typed guard as the
// current BodySetup.AggGeom instance, not merely a readable address.
template<class Read,class Guard> gears_cover_authored_array::Code capture(
    Layout const& layout,b::Proof const& proof,std::uintptr_t aggregate_base,
    gears_cover_authored_array::Source const& source,Read read,Guard guard,
    gears_cover_authored_array::Workspace& work,gears_cover_authored_array::Snapshot& output){
    namespace a=gears_cover_authored_array;
    if(a::s::aliases(&layout,sizeof layout,&work,sizeof work)||a::s::aliases(&layout,sizeof layout,&output,sizeof output)||
       a::s::aliases(&proof,sizeof proof,&work,sizeof work)||a::s::aliases(&proof,sizeof proof,&output,sizeof output))return a::Code::alias;
    const auto frozen=layout;
    if(!aggregate_base||aggregate_base>UINTPTR_MAX-frozen.field.offset||
       source.header!=aggregate_base+frozen.field.offset||unsigned(source.kind)!=frozen.element.kind||
       source.reflected_extent!=frozen.element.extent||source.collision_mask!=frozen.element.collision_mask||frozen.stride!=a::stride)return a::Code::request;
    auto current=[&](a::Source const& candidate){return guard(candidate)&&b::metadata_current()&&b::current(proof)&&
      b::same(layout.aggregate,frozen.aggregate)&&b::same(layout.inner,frozen.inner)&&b::same(layout.field.id,frozen.field.id)&&
      layout.field.offset==frozen.field.offset&&layout.stride==frozen.stride&&
      b::same(layout.element.element,frozen.element.element)&&layout.element.kind==frozen.element.kind&&
      layout.element.extent==frozen.element.extent&&layout.element.collision_mask==frozen.element.collision_mask;};
    return a::capture(source,read,current,work,output);
}
}
#endif
