#pragma once
#include "cover_body_geometry.h"
#include "cover_physx_query_source.h"
#include <type_traits>
// Live memory transport for ONE externally certified reflected primitive array.
// Guard(Source) must authenticate current setup/bone/field schema, original
// receiver scale and C0 identity/time. This reader cannot grant that authority.
namespace gears_cover_authored_array {
namespace g=gears_cover_body_geometry;namespace s=gears_cover_physx_query_source;
constexpr unsigned capacity=64,stride=80;
struct Source {std::uintptr_t header{};unsigned body_index{};g::names::Name bone{};g::metrics::Kind kind{};
    unsigned reflected_extent{},collision_mask{};std::array<float,3> receiver_scale{};};
struct Row {g::Shape shape{};bool collision_disabled{};unsigned collision_word{};std::array<unsigned char,stride> raw{};};
struct Snapshot {s::pose::ArrayHeader header{};unsigned count{};std::array<Row,capacity> rows{};};
struct Workspace {Snapshot first{},last{};};
enum class Code {captured,request,alias,guard,read,capacity,changed,value};
inline bool same(Source const&a,Source const&b) noexcept {return a.header==b.header&&a.body_index==b.body_index&&
    g::names::same(a.bone,b.bone)&&a.kind==b.kind&&a.reflected_extent==b.reflected_extent&&a.collision_mask==b.collision_mask&&a.receiver_scale==b.receiver_scale;}
template<class Read> Code pass(Source const& source,Read& read,Snapshot& out){
    out={};if(!read(&out.header,reinterpret_cast<void const*>(source.header),sizeof out.header))return Code::read;
    const auto h=out.header;
    if(h.count<0||h.count>int(capacity)||h.capacity<h.count||h.capacity>256||
       (!h.data?(h.count||h.capacity):!s::pointer(h.data,std::size_t((std::max)(1,h.capacity))*stride)))return Code::capacity;
    out.count=unsigned(h.count);
    for(unsigned i=0;i<out.count;++i){auto& row=out.rows[i];
      if(!read(row.raw.data(),reinterpret_cast<void const*>(h.data+std::size_t(i)*stride),stride))return Code::read;
      auto& shape=row.shape;shape.array_index=i;auto& a=shape.authored;
      a.body_index=source.body_index;a.bone=source.bone;a.kind=source.kind;a.receiver_scale=source.receiver_scale;
      std::memcpy(&a.tm,row.raw.data(),64);
      const unsigned dimensions=source.kind==g::metrics::Kind::sphere?1:source.kind==g::metrics::Kind::box?3:2;
      std::memcpy(a.raw_dimensions.data(),row.raw.data()+64,dimensions*4);
      std::memcpy(&row.collision_word,row.raw.data()+64+dimensions*4,4);row.collision_disabled=(row.collision_word&source.collision_mask)!=0;
      g::world::Input checked{};if(g::world::prepare_authored(a,checked)!=g::Code::converted)return Code::value;
    }
    s::pose::ArrayHeader closing{};if(!read(&closing,reinterpret_cast<void const*>(source.header),sizeof closing))return Code::read;
    return s::pose::header_equal(h,closing)?Code::captured:Code::changed;
}
template<class Read,class Guard> Code capture(Source const& source,Read read,Guard guard,Workspace& work,Snapshot& output) noexcept {
    static_assert(std::is_invocable_r_v<bool,Guard,Source const&>,"Full typed Source guard required");
    const unsigned expected=source.kind==g::metrics::Kind::sphere?72:source.kind==g::metrics::Kind::box?80:76;
    // Authored headers are packed at BodySetup+0x6c (+16 per kind).
    // Their qword data member is read by copy, not an aligned dereference.
    // Keep the separate eight-byte data/PhysX pointer rules unchanged.
    if((source.header&3)||!s::pose::span(source.header,16)||source.body_index>=g::names::max_bodies||source.bone.index<=0||
       unsigned(source.kind)>unsigned(g::metrics::Kind::sphyl)||source.reflected_extent!=expected||
       !source.collision_mask||(source.collision_mask&(source.collision_mask-1)))return Code::request;
    if(!std::isnormal(source.receiver_scale[0])||source.receiver_scale[0]<=0||
       source.receiver_scale[1]!=source.receiver_scale[0]||source.receiver_scale[2]!=source.receiver_scale[0])return Code::request;
    const g::body::d::Range ranges[]={g::body::d::range(&source,sizeof source),g::body::d::range(&work,sizeof work),g::body::d::range(&output,sizeof output)};
    if(!g::body::d::disjoint(ranges,3))return Code::alias;
    const auto original=source;
    try{
      if(!guard(original))return Code::guard;
      auto code=pass(original,read,work.first);if(code!=Code::captured)return code;
      if(!guard(original))return Code::guard;
      code=pass(original,read,work.last);if(code!=Code::captured)return code;
      if(!s::pose::header_equal(work.first.header,work.last.header)||work.first.count!=work.last.count)return Code::changed;
      for(unsigned i=0;i<work.first.count;++i)if(work.first.rows[i].raw!=work.last.rows[i].raw)return Code::changed;
      if(!guard(original))return Code::guard;
      if(!same(original,source))return Code::changed;
      output=work.last;return Code::captured;
    }catch(...){return Code::read;}
}
}
