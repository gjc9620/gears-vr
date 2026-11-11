#pragma once
#include "cover_physx_query_source.h"
#include <atomic>
#include <type_traits>
#include <cmath>
// Original BodySetup CollisionGeom(+cc)/CollisionGeomScale3D(+dc) pairing.
// Caller guard must certify reflected Pointer8/Vector12 array layout, exact
// original creation route, setup membership and C0 lifetime. Pointer values
// remain opaque: no descriptor dereference, active-shape join or world permit.
namespace gears_cover_physx_shape_cache {
namespace s=gears_cover_physx_query_source;namespace raw=s::pose;
constexpr unsigned capacity=64;
struct Source {std::uintptr_t setup{},pointers_header{},scales_header{};};
inline bool same(Source const& a,Source const& b)noexcept{return a.setup==b.setup&&a.pointers_header==b.pointers_header&&a.scales_header==b.scales_header;}
struct Row {std::uintptr_t descriptor{};std::array<float,3> scale{};};
struct Snapshot {Source source{};raw::ArrayHeader pointers{},scales{};unsigned count{};std::array<Row,capacity> rows{};};
inline bool same(Snapshot const& a,Snapshot const& b)noexcept{
    if(!same(a.source,b.source)||!raw::header_equal(a.pointers,b.pointers)||!raw::header_equal(a.scales,b.scales)||a.count!=b.count)return false;
    for(unsigned n=0;n<capacity;++n)if(a.rows[n].descriptor!=b.rows[n].descriptor||
       std::memcmp(a.rows[n].scale.data(),b.rows[n].scale.data(),sizeof a.rows[n].scale))return false;return true;
}
enum class Code {observed,request,alias,busy,guard,read,capacity,value,changed};
struct Workspace {Snapshot first{},last{};std::atomic<bool> active{};};
enum class Lookup {matched,miss,request,alias};
struct Selection {unsigned index{};Row row{};};
namespace detail {
inline bool header(raw::ArrayHeader const& h,unsigned stride,unsigned alignment)noexcept{
    if(h.count<0||h.count>int(capacity)||h.capacity<h.count||h.capacity>256)return false;
    if(!h.data)return !h.count&&!h.capacity;
    return !(h.data&(alignment-1))&&raw::span(h.data,std::size_t((std::max)(1,h.capacity))*stride);
}
template<class Read> Code pass(Source const& source,Read& read,Snapshot& output){
    Snapshot next{};next.source=source;
    if(!read(&next.pointers,reinterpret_cast<void const*>(source.pointers_header),sizeof next.pointers)||
       !read(&next.scales,reinterpret_cast<void const*>(source.scales_header),sizeof next.scales))return Code::read;
    if(!header(next.pointers,8,8)||!header(next.scales,12,4)||next.pointers.count!=next.scales.count)return Code::capacity;
    next.count=unsigned(next.pointers.count);
    for(unsigned n=0;n<next.count;++n){auto& row=next.rows[n];
      if(!read(&row.descriptor,reinterpret_cast<void const*>(next.pointers.data+8*std::size_t(n)),8)||
         !read(row.scale.data(),reinterpret_cast<void const*>(next.scales.data+12*std::size_t(n)),12))return Code::read;
      if(!s::pointer(row.descriptor,128))return Code::value;
      for(float v:row.scale)if(!std::isnormal(v)||v<=0)return Code::value;
    }
    raw::ArrayHeader pointers{},scales{};
    if(!read(&pointers,reinterpret_cast<void const*>(source.pointers_header),sizeof pointers)||
       !read(&scales,reinterpret_cast<void const*>(source.scales_header),sizeof scales))return Code::read;
    if(!raw::header_equal(pointers,next.pointers)||!raw::header_equal(scales,next.scales))return Code::changed;
    output=next;return Code::observed;
}
}
// Original finite-positive scale path: float32 subtraction, strict per-axis
// abs(delta)<float1e-4, and LAST matching entry (the loop does not break).
// Selecting an opaque cached descriptor does not identify an active shape.
inline Lookup select(Snapshot const& snapshot,std::array<float,3> const& requested,Selection& output)noexcept{
    if(s::aliases(&snapshot,sizeof snapshot,&output,sizeof output)||s::aliases(&requested,sizeof requested,&output,sizeof output))return Lookup::alias;
    if(snapshot.count>capacity||snapshot.pointers.count!=int(snapshot.count)||snapshot.scales.count!=int(snapshot.count)||
       !detail::header(snapshot.pointers,8,8)||!detail::header(snapshot.scales,12,4))return Lookup::request;
    for(float v:requested)if(!std::isnormal(v)||v<=0)return Lookup::request;
    bool found=false;Selection next{};
    for(unsigned n=0;n<snapshot.count;++n){auto const& row=snapshot.rows[n];
      if(!s::pointer(row.descriptor,128))return Lookup::request;bool matches=true;
      for(unsigned k=0;k<3;++k){if(!std::isnormal(row.scale[k])||row.scale[k]<=0)return Lookup::request;
        const float delta=row.scale[k]-requested[k];matches=matches&&(std::abs(delta)<1e-4f);}
      if(matches){found=true;next={n,row};}
    }
    if(!found)return Lookup::miss;output=next;return Lookup::matched;
}
template<class Read,class Guard> Code observe(Source const& source,Read read,Guard guard,Workspace& work,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard,Source const&>,"Full typed cache source/layout/current guard required");
    if(!s::pointer(source.setup,0xec)||source.pointers_header!=source.setup+0xcc||source.scales_header!=source.setup+0xdc)return Code::request;
    if(s::aliases(&source,sizeof source,&work,sizeof work)||s::aliases(&source,sizeof source,&output,sizeof output)||
       s::aliases(&work,sizeof work,&output,sizeof output))return Code::alias;
    bool inactive=false;if(!work.active.compare_exchange_strong(inactive,true))return Code::busy;
    struct Finish{std::atomic<bool>& active;~Finish(){active=false;}}finish{work.active};const auto key=source;
    try{
      auto current=[&](){return guard(key)&&same(key,source);};
      if(!current())return Code::guard;
      auto code=detail::pass(key,read,work.first);if(code!=Code::observed)return code;
      if(!current())return Code::guard;
      code=detail::pass(key,read,work.last);if(code!=Code::observed)return code;
      if(!same(work.first,work.last))return Code::changed;
      if(!current())return Code::guard;
      output=work.first;return Code::observed;
    }catch(...){return Code::read;}
}
}
