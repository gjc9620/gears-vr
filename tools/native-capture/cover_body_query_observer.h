#pragma once
#include "cover_body_query_catalog.h"
// Complete RAW overlap observations of a supplied catalog. A hit may be self;
// even all misses are not a world fence, sweep, occupancy or move permission.
namespace gears_cover_body_query_observer {
namespace c=gears_cover_body_query_catalog;namespace q=gears_cover_physx_query;
enum class Code {observed,request,alias,catalog,source,query};
struct Result {unsigned count{};std::array<bool,c::g::max_primitives> raw_hit{};};
struct Workspace {Result candidate{};c::Catalog initial{};c::g::names::Mapping mapping{};
    std::array<c::g::Inventory,c::g::names::max_bodies> inventory{};};
template<class Session,class Actor,class Metadata,class Read,class Guard>
Code observe(Session& session,Actor const& actor,Metadata const& metadata,c::Catalog const& catalog,
             c::g::names::Mapping const& mapping,c::g::Inventory const* inventory,
             Read read,Guard guard,Workspace& work,Result& output) noexcept {
    const auto bodies=catalog.body_count;
    if(!inventory||!bodies||bodies>c::g::names::max_bodies||!catalog.count||catalog.count>c::g::max_primitives)return Code::request;
    const c::g::body::d::Range ranges[]={c::g::body::d::range(&session,sizeof session),c::g::body::d::range(&actor,sizeof actor),
      c::g::body::d::range(&metadata,sizeof metadata),c::g::body::d::range(&catalog,sizeof catalog),c::g::body::d::range(&mapping,sizeof mapping),
      c::g::body::d::range(inventory,std::size_t(bodies)*sizeof(*inventory)),c::g::body::d::range(&work,sizeof work),c::g::body::d::range(&output,sizeof output)};
    if(!c::g::body::d::disjoint(ranges,8))return Code::alias;
    work.initial=catalog;work.mapping=mapping;
    std::memcpy(work.inventory.data(),inventory,std::size_t(bodies)*sizeof(*inventory));
    auto unchanged=[&](){return !std::memcmp(&catalog,&work.initial,sizeof catalog)&&
        !std::memcmp(&mapping,&work.mapping,sizeof mapping)&&
        !std::memcmp(inventory,work.inventory.data(),std::size_t(bodies)*sizeof(*inventory));};
    if(catalog.units!=c::p::Units::physx||!gears_cover_body_overlap::detail::mapping(mapping,bodies))return Code::catalog;
    unsigned total=0;
    for(unsigned b=0;b<bodies;++b){if(inventory[b].count[3])return Code::catalog;
      for(unsigned k=0;k<3;++k){if(inventory[b].count[k]>c::g::max_primitives-total)return Code::catalog;total+=inventory[b].count[k];}}
    if(total!=catalog.count)return Code::catalog;
    unsigned at=0;
    // Preflight ALL shapes before the first query, including the final row.
    for(unsigned b=0;b<bodies;++b)for(unsigned k=0;k<3;++k)for(unsigned n=0;n<inventory[b].count[k];++n){
      const auto& row=catalog.rows[at++];const auto& s=row.shape;
      if(s.units!=c::p::Units::physx||s.body_index!=b||s.bone_index!=mapping.rows[b].bone_index||!c::g::names::same(s.bone,mapping.rows[b].name)||
         unsigned(s.kind)!=k||row.array_index!=n)return Code::catalog;
      const bool valid=k==0?q::detail::valid(s.sphere):k==1?q::detail::valid(s.box):q::detail::valid(s.capsule);
      if(!valid)return Code::catalog;
    }
    try{
      if(!guard()||!unchanged())return Code::source;work.candidate={};work.candidate.count=total;
      for(unsigned i=0;i<total;++i){const auto& s=catalog.rows[i].shape;q::Overlap raw{};q::Code result{};
        if(!guard()||!unchanged())return Code::source;
        if(s.kind==c::g::metrics::Kind::sphere)result=session.overlap(actor,metadata,s.sphere,read,guard,raw);
        else if(s.kind==c::g::metrics::Kind::box)result=session.overlap(actor,metadata,s.box,read,guard,raw);
        else result=session.overlap(actor,metadata,s.capsule,read,guard,raw);
        if(result!=q::Code::queried)return Code::query;
        if(!guard()||!unchanged())return Code::source;work.candidate.raw_hit[i]=raw.raw_hit;
      }
      if(!guard()||!unchanged())return Code::source;output=work.candidate;return Code::observed;
    }catch(...){return Code::query;}
}
}
