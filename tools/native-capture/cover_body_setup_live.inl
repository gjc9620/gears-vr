#ifndef GEARS_COVER_BODY_SETUP_LIVE_INCLUDED
#define GEARS_COVER_BODY_SETUP_LIVE_INCLUDED
#include "cover_array_layout_live.inl"
// Caller holds the current metadata/GT lock and authenticates PhysicsAsset,
// body ordinal, C0 lifetime, receiver scale and native consumer correspondence.
namespace cover_body_setup_live {
namespace b=weapon_hand_scene_binding;namespace arrays=cover_array_layout_live;
namespace names=gears_cover_authored_array::g::names;
struct Layout {gears_anchor::Identity setup{};b::Property aggregate{},bone_field{};
    names::Name bone{};std::uintptr_t aggregate_base{};arrays::Layout array{};};
inline bool prepare(b::Proof& proof,gears_anchor::Identity const& setup,
                    names::Name expected_bone,unsigned kind,Layout& output){
    Layout next{};std::uintptr_t target{};
    if(!b::metadata_current()||!b::certified(proof,setup.address,next.setup)||!b::same(setup,next.setup)||
       !cover_physics_roles_live::class_role(proof,setup.cls,L"RB_BodySetup")||
       !cover_physics_roles_live::role_property(proof,setup.cls,L"AggGeom",L"StructProperty",72,nullptr,next.aggregate)||
       next.aggregate.owner.number||!b::named(next.aggregate.owner,L"KMeshProps")||
       !b::source_field(proof,target,next.aggregate.id.address,0xd0))return false;
    gears_anchor::Identity aggregate{};
    if(!b::certified(proof,target,aggregate)||
       !arrays::prepare(proof,aggregate,next.aggregate.owner,kind,next.array)||
       !cover_physics_roles_live::role_property(proof,setup.cls,L"BoneName",L"NameProperty",8,nullptr,next.bone_field)||
       next.bone_field.offset!=0xb4||!expected_bone.index||
       !b::source_field(proof,next.bone,setup.address,next.bone_field.offset)||!names::same(next.bone,expected_bone)||
       setup.address>UINTPTR_MAX-next.aggregate.offset)return false;
    // The two reflected values must be disjoint, even for a malformed class.
    if(next.aggregate.offset<next.bone_field.offset+8&&next.bone_field.offset<next.aggregate.offset+72)return false;
    next.aggregate_base=setup.address+next.aggregate.offset;
    if(!b::current(proof)||!b::metadata_current())return false;
    output=next;return true;
}
template<class Read,class Guard> gears_cover_authored_array::Code capture(Layout const& layout,b::Proof const& proof,
    gears_cover_authored_array::Source const& source,Read read,Guard guard,
    gears_cover_authored_array::Workspace& work,gears_cover_authored_array::Snapshot& output){
    namespace a=gears_cover_authored_array;
    if(a::s::aliases(&layout,sizeof layout,&work,sizeof work)||a::s::aliases(&layout,sizeof layout,&output,sizeof output))return a::Code::alias;
    const auto frozen=layout;
    if(!names::same(source.bone,frozen.bone))return a::Code::request;
    auto current=[&](a::Source const& candidate){return guard(candidate)&&
      b::same(layout.setup,frozen.setup)&&b::same(layout.aggregate.id,frozen.aggregate.id)&&
      b::same(layout.bone_field.id,frozen.bone_field.id)&&layout.aggregate_base==frozen.aggregate_base&&
      names::same(layout.bone,frozen.bone)&&names::same(candidate.bone,frozen.bone);};
    return arrays::capture(layout.array,proof,frozen.aggregate_base,source,read,current,work,output);
}
}
#endif
