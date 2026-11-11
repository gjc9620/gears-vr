#ifndef GEARS_COVER_PRIMITIVE_LAYOUT_LIVE_INCLUDED
#define GEARS_COVER_PRIMITIVE_LAYOUT_LIVE_INCLUDED
#include "cover_struct_field_live.inl"
// Current reflected schema only. Caller binds array inner, BodySetup, original
// native stride/code and C0 source separately, then rechecks this full Proof.
namespace cover_primitive_layout_live {
namespace b=weapon_hand_scene_binding;namespace f=cover_struct_field_live;
struct Layout {gears_anchor::Identity element{},matrix{},plane{};unsigned kind{},extent{},collision_mask{};};
inline bool prepare(b::Proof& proof,gears_anchor::Identity const& element,
                    gears_anchor::Identity const& declaring_class,unsigned kind,Layout& output){
    if(kind>2)return false;
    gears_anchor::Identity owner{},actual{};
    if(!b::metadata_current()||!b::certified(proof,declaring_class.address,owner)||!b::same(owner,declaring_class)||
       owner.number||!b::named(owner,L"KMeshProps")||!b::kind(proof,owner,L"Class")||
       !b::certified(proof,element.address,actual)||!b::same(actual,element)||actual.number||actual.outer!=owner.address)return false;
    const wchar_t* names[]={L"KSphereElem",L"KBoxElem",L"KSphylElem"};
    const unsigned extents[]={72,80,76};const unsigned dims[]={1,3,2};
    if(!b::named(actual,names[kind]))return false;
    Layout next{};next.kind=kind;next.extent=extents[kind];next.element=actual;
    b::Property field{};
    if(!f::select(proof,actual,next.extent,L"TM",L"StructProperty",64,0,L"Matrix",field))return false;
    next.matrix=field.reference;
    const wchar_t* planes[]={L"XPlane",L"YPlane",L"ZPlane",L"WPlane"};
    for(unsigned i=0;i<4;++i){
      if(!f::select(proof,next.matrix,64,planes[i],L"StructProperty",16,i*16,L"Plane",field))return false;
      if(i&&!b::same(next.plane,field.reference))return false;next.plane=field.reference;
    }
    const wchar_t* components[]={L"X",L"Y",L"Z",L"W"};
    for(unsigned i=0;i<4;++i)if(!f::select(proof,next.plane,16,components[i],L"FloatProperty",4,i*4,nullptr,field))return false;
    const wchar_t* dimension_names[3][3]={{L"Radius",nullptr,nullptr},{L"X",L"Y",L"Z"},{L"Radius",L"Length",nullptr}};
    for(unsigned i=0;i<dims[kind];++i)if(!f::select(proof,actual,next.extent,dimension_names[kind][i],L"FloatProperty",4,64+i*4,nullptr,field))return false;
    if(!f::select(proof,actual,next.extent,L"bNoRBCollision",L"BoolProperty",4,64+dims[kind]*4,nullptr,field)||
       !b::source_field(proof,next.collision_mask,field.id.address,0xd0)||!next.collision_mask||
       (next.collision_mask&(next.collision_mask-1))||!b::current(proof)||!b::metadata_current())return false;
    output=next;return true;
}
}
#endif
