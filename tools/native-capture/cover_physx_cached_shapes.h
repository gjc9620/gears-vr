#pragma once
#include "cover_physx_shape_cache.h"
#include "cover_physx_pose_math.h"
// Selected original cached actor descriptor -> complete shape descriptor list.
// Values remain native PhysX units. Guard pins the original game image/route,
// typed cache layout/body membership and C0 lifetime. No native calls or proof
// that an active actor was constructed from this cache entry.
namespace gears_cover_physx_cached_shapes {
namespace cache=gears_cover_physx_shape_cache;namespace s=cache::s;namespace raw=cache::raw;
namespace math=gears_cover_physx_pose_math;
constexpr unsigned capacity=256;
struct Source {std::uintptr_t game_base{};std::uint32_t game_size{};};
inline bool same(Source const& a,Source const& b)noexcept{return a.game_base==b.game_base&&a.game_size==b.game_size;}
struct Vector {std::uintptr_t begin{},end{},limit{};};
inline bool same(Vector const& a,Vector const& b)noexcept{return a.begin==b.begin&&a.end==b.end&&a.limit==b.limit;}
struct Row {std::uintptr_t address{},table{};unsigned tag{},flags{};math::Matrix local{};
    std::array<float,3> half{};float radius{},full_segment_length{};std::array<unsigned char,144> bytes{};};
struct Snapshot {cache::Selection selected{};Vector vector{};unsigned count{};std::array<Row,capacity> rows{};};
inline bool same(Snapshot const& a,Snapshot const& b)noexcept{
    if(a.selected.index!=b.selected.index||a.selected.row.descriptor!=b.selected.row.descriptor||a.selected.row.scale!=b.selected.row.scale||
       !same(a.vector,b.vector)||a.count!=b.count)return false;
    for(unsigned n=0;n<capacity;++n)if(a.rows[n].address!=b.rows[n].address||a.rows[n].bytes!=b.rows[n].bytes)return false;
    return true;
}
enum class Code {observed,request,alias,busy,guard,cache_error,miss,read,vector,type,pose,value,changed};
struct Workspace {Snapshot first{},last{};cache::Workspace cache_work{};cache::Snapshot cache_observed{};
    std::atomic<bool> active{};cache::Code cache_code{};};
namespace detail {
template<class T>void unpack(std::array<unsigned char,144> const& bytes,unsigned offset,T& out){std::memcpy(&out,bytes.data()+offset,sizeof out);}
inline bool vector(Vector const& v,unsigned& count)noexcept{
    if(!v.begin){count=0;return !v.end&&!v.limit;}
    if((v.begin&7)||v.end<v.begin||v.limit<v.end||(v.end-v.begin)%8||(v.limit-v.begin)%8||
       (v.limit-v.begin)/8>capacity||!raw::span(v.begin,(std::max)(std::size_t(8),std::size_t(v.limit-v.begin))))return false;
    count=unsigned((v.end-v.begin)/8);return true;
}
template<class Read>Code pass(Source const& source,cache::Selection const& selected,Read& read,Snapshot& output){
    output={};output.selected=selected;
    if(!read(&output.vector,reinterpret_cast<void const*>(selected.row.descriptor+104),sizeof output.vector))return Code::read;
    if(!vector(output.vector,output.count))return Code::vector;
    for(unsigned n=0;n<output.count;++n){auto& row=output.rows[n];
      if(!read(&row.address,reinterpret_cast<void const*>(output.vector.begin+8*std::size_t(n)),8))return Code::read;
      if(!s::pointer(row.address,136)||!read(row.bytes.data(),reinterpret_cast<void const*>(row.address),12))return Code::read;
      unpack(row.bytes,0,row.table);unpack(row.bytes,8,row.tag);
      const unsigned rva=row.tag==1?0x1347dc0:row.tag==2?0x13486f0:row.tag==3?0x1347dd8:0;
      if(!rva||row.table!=source.game_base+rva)return Code::type;
      const unsigned size=row.tag==1?136:144;
      if(!s::pointer(row.address,size))return Code::read;
      if(!read(row.bytes.data(),reinterpret_cast<void const*>(row.address),size))return Code::read;
      std::uintptr_t closing_table{};unsigned closing_tag{};unpack(row.bytes,0,closing_table);unpack(row.bytes,8,closing_tag);
      if(closing_table!=row.table||closing_tag!=row.tag)return Code::changed;
      std::memcpy(&row.local,row.bytes.data()+12,sizeof row.local);unpack(row.bytes,60,row.flags);
      if(!math::rigid(row.local))return Code::pose;
      if(row.tag==2){for(unsigned k=0;k<3;++k){unpack(row.bytes,128+4*k,row.half[k]);if(!std::isnormal(row.half[k])||row.half[k]<=0)return Code::value;}}
      else{unpack(row.bytes,128,row.radius);if(!std::isnormal(row.radius)||row.radius<=0)return Code::value;
        if(row.tag==3){unpack(row.bytes,132,row.full_segment_length);if(!math::scalar(row.full_segment_length)||row.full_segment_length<0)return Code::value;}}
    }
    // Closing only the vector's three pointers misses in-place member swaps.
    // Close every observed member as well, preserving original order/duplicates.
    for(unsigned n=0;n<output.count;++n){std::uintptr_t closing_member{};
      if(!read(&closing_member,reinterpret_cast<void const*>(output.vector.begin+8*std::size_t(n)),8))return Code::read;
      if(closing_member!=output.rows[n].address)return Code::changed;}
    Vector closing{};if(!read(&closing,reinterpret_cast<void const*>(selected.row.descriptor+104),sizeof closing))return Code::read;
    return same(output.vector,closing)?Code::observed:Code::changed;
}
}
template<class Read,class Guard>Code observe(Source const& source,cache::Snapshot const& inventory,std::array<float,3> const& requested,
    Read read,Guard guard,Workspace& work,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard,Source const&,cache::Snapshot const&>,"Pinned original game image and full typed cache scope required");
    if(!source.game_base||(source.game_base&0xffff)||source.game_size<0x1349000||source.game_size>UINTPTR_MAX-source.game_base)return Code::request;
    if(s::aliases(&source,sizeof source,&work,sizeof work)||s::aliases(&source,sizeof source,&output,sizeof output)||
       s::aliases(&inventory,sizeof inventory,&work,sizeof work)||s::aliases(&inventory,sizeof inventory,&output,sizeof output)||
       s::aliases(&requested,sizeof requested,&work,sizeof work)||s::aliases(&requested,sizeof requested,&output,sizeof output)||
       s::aliases(&work,sizeof work,&output,sizeof output))return Code::alias;
    bool inactive=false;if(!work.active.compare_exchange_strong(inactive,true))return Code::busy;
    struct Finish{std::atomic<bool>& active;~Finish(){active=false;}}finish{work.active};
    const auto key=source;const auto frozen=inventory;const auto scale=requested;cache::Selection selected{};
    try{
      auto current=[&](){return guard(key,frozen)&&same(key,source)&&cache::same(frozen,inventory)&&scale==requested;};
      if(!current())return Code::guard;
      const auto lookup=cache::select(frozen,scale,selected);if(lookup==cache::Lookup::miss)return Code::miss;
      if(lookup!=cache::Lookup::matched)return Code::request;
      auto close_cache=[&](){auto cache_guard=[&](cache::Source const& src){return current()&&cache::same(src,frozen.source);};
        work.cache_code=cache::observe(frozen.source,read,cache_guard,work.cache_work,work.cache_observed);
        return work.cache_code==cache::Code::observed&&cache::same(work.cache_observed,frozen);};
      if(!close_cache())return Code::cache_error;
      auto code=detail::pass(key,selected,read,work.first);if(code!=Code::observed)return code;
      if(!current())return Code::guard;
      code=detail::pass(key,selected,read,work.last);if(code!=Code::observed)return code;
      if(!same(work.first,work.last))return Code::changed;
      if(!close_cache())return Code::cache_error;
      if(!current())return Code::guard;
      output=work.first;return Code::observed;
    }catch(...){return Code::read;}
}
}
