#pragma once
#include "cover_primitive_world.h"

// Complete conversion of a caller-authenticated authored-shape inventory at
// ONE current C0 pose. No native reads, calls, scene synchronization or sweep
// permission. The caller must close body/setup/shape arrays and their current
// native associations; these counts do not prove the live inventory by itself.
namespace gears_cover_body_geometry {
namespace world=gears_cover_primitive_world;
namespace body=world::body;namespace names=world::bones;namespace metrics=world::metrics;
using Code=world::Code;
constexpr unsigned max_primitives=256;
struct Inventory {std::array<std::uint32_t,4> count{};}; // Sphere,Box,Sphyl,Convex.
struct Shape {world::Authored authored{};std::uint32_t array_index{};};
struct Row {world::Primitive primitive{};std::uint32_t array_index{};};
struct Geometry {std::uint32_t body_count{},count{};std::array<Row,max_primitives> rows{};};
// Caller-owned heap/TLS scratch; candidate may change on failure. Public output
// stays byte-identical, including a failure on the last shape in the inventory.
struct Workspace {world::Workspace skeleton{};Geometry candidate{};};
inline Code convert(body::Model const& model,body::Frame const& frame,names::Mapping const& mapping,
                    Inventory const* inventory,unsigned body_count,Shape const* shapes,unsigned count,
                    Workspace& work,Geometry& output) noexcept {
    if(!inventory||!shapes||!body_count||body_count>names::max_bodies||
       !count||count>max_primitives)return Code::request;
    const body::d::Range ranges[]={body::d::range(&model,sizeof model),body::d::range(&frame,sizeof frame),
        body::d::range(&mapping,sizeof mapping),body::d::range(inventory,std::size_t(body_count)*sizeof(Inventory)),
        body::d::range(shapes,std::size_t(count)*sizeof(Shape)),body::d::range(&work,sizeof work),body::d::range(&output,sizeof output)};
    if(!body::d::disjoint(ranges,7))return Code::alias;
    if(body_count!=mapping.count)return Code::request;
    unsigned total=0;
    for(unsigned b=0;b<body_count;++b){
        // An unsupported part of the declared body stops the complete result,
        // even when the caller omits that part from its supplied shape list.
        if(inventory[b].count[3])return Code::unsupported;
        for(unsigned k=0;k<3;++k){const auto n=inventory[b].count[k];
            if(n>max_primitives-total)return Code::inventory;total+=n;}
    }
    if(total!=count)return Code::inventory;
    // Require canonical body/kind/original-array order. Count and ordinal
    // checks reject truncation, duplicate records, missing and swapped shapes.
    unsigned at=0;
    for(unsigned b=0;b<body_count;++b)for(unsigned k=0;k<3;++k)for(unsigned n=0;n<inventory[b].count[k];++n){
        const auto& s=shapes[at++];
        if(s.authored.body_index!=b||unsigned(s.authored.kind)!=k||s.array_index!=n)return Code::inventory;
    }
    const auto prepared=world::detail::prepare_skeleton(model,frame,mapping,work.skeleton);
    if(prepared!=Code::converted)return prepared;
    for(auto& row:work.candidate.rows)row={};
    work.candidate.body_count=body_count;work.candidate.count=count;
    for(unsigned i=0;i<count;++i){world::Input decoded{};
        const auto raw=world::prepare_authored(shapes[i].authored,decoded);
        if(raw!=Code::converted)return raw;
        const auto placed=world::detail::place(decoded,work.skeleton,work.candidate.rows[i].primitive);
        if(placed!=Code::converted)return placed;
        work.candidate.rows[i].array_index=shapes[i].array_index;
    }
    output=work.candidate;return Code::converted;
}
} // namespace gears_cover_body_geometry
