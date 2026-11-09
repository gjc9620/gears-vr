#pragma once
#include "full_body_c0_planner.h"

// Pure join of caller-owned values. The RefBone68 codec is the existing
// supported Native reference contract, not reflected IntProperty storage.
// Callers separately certify current body/setup/asset identity, code/layout,
// GNames and the SAME C0 source bracket. No memory read, getter, physics call,
// state publication, active-shape or movement permission is supplied here.
namespace gears_cover_physics_bones {
namespace body=gears_full_body_c0;
constexpr unsigned max_bodies=64;
struct Name {std::int32_t index{};std::uint32_t number{};};
inline bool same(Name a,Name b) noexcept{return a.index==b.index&&a.number==b.number;}
struct Input {std::uint32_t body_index{};Name bone{};};
struct Row {
    std::uint32_t body_index{},bone_index{};Name name{};
    std::int32_t skeletal_parent{},nearest_named_body_ancestor{-1};
};
struct Mapping {
    std::uint32_t count{},root_body_index{},root_bone_index{};
    body::rig::Signature geometry{};
    std::array<Name,body::bones> reference_names{};
    std::array<Row,max_bodies> rows{};
};
enum class Status {okay,request,alias,model,reference,name,duplicate_reference,body_index,duplicate_body_bone,missing_bone};
namespace detail {
inline bool geometry(body::Model const& m,body::Frame const& f) noexcept {
    body::rig::CanonicalGeometry canonical{};
    if(f.count!=body::bones||m.signature!=body::rig::marcus115_signature||
       !body::rig::canonical_geometry(f.reference.data(),f.count,canonical)||canonical!=m.canonical)return false;
    for(unsigned i=0;i<body::bones;++i){
        std::int32_t parent{};std::memcpy(&parent,f.reference[i].bytes+56,4);
        if(m.parents[i]!=parent||std::memcmp(m.reference[i].q,f.reference[i].bytes+12,16)||
           std::memcmp(m.reference[i].translation,f.reference[i].bytes+28,12)||m.reference[i].scale!=1)return false;
    }
    return true;
}
inline Name name(body::rig::ReferenceRecord68 const& record) noexcept {
    Name out{};std::memcpy(&out.index,record.bytes,4);std::memcpy(&out.number,record.bytes+4,4);return out;
}
}

inline Status bind(body::Model const& model,body::Frame const& frame,Input const* inputs,
                   unsigned count,unsigned root_body_index,Mapping& output) noexcept {
    if(!inputs||!count||count>max_bodies||root_body_index>=count)return Status::request;
    const body::d::Range ranges[]={body::d::range(&model,sizeof model),body::d::range(&frame,sizeof frame),
        body::d::range(inputs,std::size_t(count)*sizeof(Input)),body::d::range(&output,sizeof output)};
    if(!body::d::disjoint(ranges,4))return Status::alias;
    if(!detail::geometry(model,frame))return Status::model;
    Mapping candidate{};candidate.count=count;candidate.root_body_index=root_body_index;candidate.geometry=model.signature;
    for(unsigned i=0;i<body::bones;++i){
        const auto name=detail::name(frame.reference[i]);if(name.index<=0)return Status::name;
        for(unsigned j=0;j<i;++j)if(same(name,candidate.reference_names[j]))return Status::duplicate_reference;
        candidate.reference_names[i]=name;
    }
    std::array<int,body::bones> body_for_bone{};body_for_bone.fill(-1);
    for(unsigned i=0;i<count;++i){
        const auto& input=inputs[i];
        if(input.body_index!=i)return Status::body_index;
        if(input.bone.index<=0)return Status::name;
        unsigned bone=body::bones;
        for(unsigned j=0;j<body::bones;++j)if(same(input.bone,candidate.reference_names[j])){bone=j;break;}
        if(bone==body::bones)return Status::missing_bone;
        if(body_for_bone[bone]!=-1)return Status::duplicate_body_bone;
        body_for_bone[bone]=int(i);
        candidate.rows[i]={i,bone,input.bone,model.parents[bone],-1};
    }
    for(unsigned i=0;i<count;++i){
        auto& row=candidate.rows[i];unsigned bone=row.bone_index;
        // This is ancestry in the named skeleton, never a PhysX constraint
        // parent, simulation island or proof that the asset is articulated.
        while(bone){
            bone=unsigned(model.parents[bone]);
            if(body_for_bone[bone]>=0){row.nearest_named_body_ancestor=body_for_bone[bone];break;}
        }
    }
    candidate.root_bone_index=candidate.rows[root_body_index].bone_index;
    output=candidate;return Status::okay;
}
} // namespace gears_cover_physics_bones
