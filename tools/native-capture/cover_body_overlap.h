#pragma once
#include "cover_body_geometry.h"
#include "cover_box_overlap.h"

// Complete consumption of the SUPPLIED inventory/mapping/geometry. Their
// current Native/source/pose/world association remains the caller's obligation.
// This tests one AABB against all declared body shapes, NOT the body against
// the whole scene, and NOT a motion/pose sweep or a movement/occupancy permit.
namespace gears_cover_body_overlap {
namespace geometry=gears_cover_body_geometry;
namespace point=gears_cover_point_overlap;namespace box=gears_cover_box_overlap;
using Relation=point::Relation;using Query=point::Box;
enum class Code {evaluated,request,alias,mapping,inventory,unsupported,invalid,numeric};
constexpr unsigned none=(std::numeric_limits<unsigned>::max)();
struct Result {
    Relation relation{Relation::indeterminate};
    unsigned evaluated{},separated{},overlapping{},indeterminate{};
    std::array<unsigned,3> per_kind{}; // Sphere, Box, Sphyl.
    unsigned first_overlap{none},first_indeterminate{none};
};
namespace detail {
inline Code status(point::Status s) noexcept {
    switch(s){
    case point::Status::evaluated:return Code::evaluated;
    case point::Status::unsupported:return Code::unsupported;
    case point::Status::alias:return Code::alias;
    case point::Status::numeric:return Code::numeric;
    default:return Code::invalid;
    }
}
// Structural association checks, not a replacement for bind(model,currentC0)
// or for current source closure. No pointers are retained for future use.
inline bool mapping(geometry::names::Mapping const& m,unsigned count) noexcept {
    if(m.count!=count||m.geometry!=geometry::body::rig::marcus115_signature||m.root_body_index>=count||
       m.root_bone_index!=m.rows[m.root_body_index].bone_index)return false;
    for(unsigned i=0;i<count;++i){const auto& row=m.rows[i];
        if(row.body_index!=i||row.bone_index>=geometry::body::bones||row.name.index<=0||
           !geometry::names::same(row.name,m.reference_names[row.bone_index]))return false;
        for(unsigned j=0;j<i;++j)if(row.bone_index==m.rows[j].bone_index)return false;
    }
    return true;
}
}
// No early-out on overlap or indeterminate: a malformed/unsupported last row
// invalidates the complete operation and preserves all bytes of the old result.
inline Code classify(geometry::names::Mapping const& mapping,geometry::Inventory const* inventory,unsigned body_count,
                     geometry::Geometry const& shapes,Query const& query,Result& output) noexcept {
    if(!inventory||!body_count||body_count>geometry::names::max_bodies)return Code::request;
    const geometry::body::d::Range ranges[]={geometry::body::d::range(&mapping,sizeof mapping),
        geometry::body::d::range(inventory,std::size_t(body_count)*sizeof(geometry::Inventory)),
        geometry::body::d::range(&shapes,sizeof shapes),geometry::body::d::range(&query,sizeof query),
        geometry::body::d::range(&output,sizeof output)};
    if(!geometry::body::d::disjoint(ranges,5))return Code::alias;
    if(!detail::mapping(mapping,body_count))return Code::mapping;
    if(shapes.body_count!=body_count||!shapes.count||shapes.count>geometry::max_primitives)return Code::inventory;
    unsigned total=0;
    for(unsigned b=0;b<body_count;++b){
        if(inventory[b].count[3])return Code::unsupported; // even if the caller omitted its row
        for(unsigned k=0;k<3;++k){const auto count=inventory[b].count[k];
            if(count>geometry::max_primitives-total)return Code::inventory;total+=count;}
    }
    if(total!=shapes.count)return Code::inventory;
    unsigned at=0;Result next{};
    for(unsigned b=0;b<body_count;++b)for(unsigned k=0;k<3;++k)for(unsigned ordinal=0;ordinal<inventory[b].count[k];++ordinal){
        const auto& row=shapes.rows[at];const auto& shape=row.primitive;const auto& bone=mapping.rows[b];
        if(shape.body_index!=b||unsigned(shape.dimensions.kind)!=k||row.array_index!=ordinal)return Code::inventory;
        if(shape.bone_index!=bone.bone_index||!geometry::names::same(shape.bone,bone.name))return Code::mapping;
        Relation relation{};point::Status status{};
        if(k==unsigned(geometry::metrics::Kind::box)){
            box::Result result{};status=box::classify(shape,query,result);relation=result.relation;
        }else{
            point::Result result{};status=point::classify(shape,query,result);relation=result.relation;
        }
        if(status!=point::Status::evaluated)return detail::status(status);
        ++next.evaluated;++next.per_kind[k];
        switch(relation){
        case Relation::separated:++next.separated;break;
        case Relation::overlapping:++next.overlapping;if(next.first_overlap==none)next.first_overlap=at;break;
        case Relation::indeterminate:++next.indeterminate;if(next.first_indeterminate==none)next.first_indeterminate=at;break;
        default:return Code::invalid;
        }
        ++at;
    }
    if(at!=shapes.count)return Code::inventory;
    next.relation=next.overlapping?Relation::overlapping:next.indeterminate?Relation::indeterminate:Relation::separated;
    output=next;return Code::evaluated;
}
} // namespace gears_cover_body_overlap
