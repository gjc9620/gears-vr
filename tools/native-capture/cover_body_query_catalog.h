#pragma once
#include "cover_body_overlap.h"
#include "cover_primitive_query.h"
#include "cover_query_units.h"
// All-or-nothing conversion of a SUPPLIED complete body inventory at one pose.
// Native identity, current active geometry and full world/path safety still
// belong to the caller. A catalog is query input, never movement permission.
namespace gears_cover_body_query_catalog {
namespace g=gears_cover_body_geometry;namespace p=gears_cover_primitive_query;
enum class Code {prepared,request,alias,mapping,inventory,unsupported,shape,precision};
struct Row {unsigned array_index{};p::Shape shape{};};
struct Catalog {p::Units units{};unsigned body_count{},count{};std::array<Row,g::max_primitives> rows{};};
struct Workspace {Catalog candidate{};};
inline Code prepare(g::names::Mapping const& mapping,g::Inventory const* inventory,unsigned body_count,
                    g::Geometry const& geometry,Workspace& work,Catalog& output,double maximum_game_expansion=0) noexcept {
    if(!inventory||!body_count||body_count>g::names::max_bodies||!geometry.count||geometry.count>g::max_primitives)return Code::request;
    const g::body::d::Range ranges[]={g::body::d::range(&mapping,sizeof mapping),g::body::d::range(inventory,std::size_t(body_count)*sizeof(g::Inventory)),
        g::body::d::range(&geometry,sizeof geometry),g::body::d::range(&work,sizeof work),g::body::d::range(&output,sizeof output)};
    if(!g::body::d::disjoint(ranges,5))return Code::alias;
    if(!gears_cover_body_overlap::detail::mapping(mapping,body_count))return Code::mapping;
    if(geometry.body_count!=body_count)return Code::inventory;
    unsigned total=0;
    for(unsigned b=0;b<body_count;++b){if(inventory[b].count[3])return Code::unsupported;
        for(unsigned k=0;k<3;++k){if(inventory[b].count[k]>g::max_primitives-total)return Code::inventory;total+=inventory[b].count[k];}}
    if(total!=geometry.count)return Code::inventory;
    auto& next=work.candidate;next.units=p::Units::physx;next.body_count=body_count;next.count=total;
    for(auto& row:next.rows)row={};
    unsigned at=0;
    for(unsigned b=0;b<body_count;++b)for(unsigned k=0;k<3;++k)for(unsigned n=0;n<inventory[b].count[k];++n){
        const auto& row=geometry.rows[at];const auto& s=row.primitive;const auto& bone=mapping.rows[b];
        if(s.body_index!=b||unsigned(s.dimensions.kind)!=k||row.array_index!=n)return Code::inventory;
        if(s.bone_index!=bone.bone_index||!g::names::same(s.bone,bone.name))return Code::mapping;
        const auto converted=gears_cover_query_units::convert(s,maximum_game_expansion,next.rows[at].shape);
        if(converted==p::Code::precision)return Code::precision;
        if(converted!=p::Code::converted)return Code::shape;
        next.rows[at].array_index=n;++at;
    }
    output=next;return Code::prepared;
}
}
