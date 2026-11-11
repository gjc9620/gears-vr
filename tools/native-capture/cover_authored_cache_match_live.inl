#ifndef GEARS_COVER_AUTHORED_CACHE_MATCH_LIVE_INCLUDED
#define GEARS_COVER_AUTHORED_CACHE_MATCH_LIVE_INCLUDED
#include "cover_authored_geometry_live.inl"
#include "cover_physx_cached_shapes.h"
#include "cover_physx_shape_creation.h"
// Owned semantic correspondence for ONE supplied BodySetup cache template.
// Replays descriptor creation with the SELECTED cached scale, not today's
// point scale. Caller closes current identity/lifetime/code separately. No
// active actor identity, flag eligibility, world fence or movement permission.
namespace cover_authored_cache_match_live {
namespace authored=cover_authored_geometry_live;namespace inv=authored::live;
namespace a=inv::a;namespace b=inv::b;namespace cached=gears_cover_physx_cached_shapes;
namespace creation=gears_cover_physx_shape_creation;
enum class Code {matched,request,alias,source,inventory,raw,unsupported,creation,mismatch};
struct Row {unsigned authored_index{},cache_index{};};
struct Result {gears_anchor::Identity setup{};a::g::names::Name bone{};unsigned body_index{},authored_count{},count{};
    std::array<Row,a::g::max_primitives> rows{};std::array<bool,a::g::max_primitives> disabled{};};
inline bool descriptor(cached::Source const& source,cached::Row const& row,creation::Descriptor const& expected,unsigned tag)noexcept{
    std::uintptr_t table{};unsigned actual_tag{},flags{};cached::math::Matrix local{};
    cached::detail::unpack(row.bytes,0,table);cached::detail::unpack(row.bytes,8,actual_tag);cached::detail::unpack(row.bytes,60,flags);
    const auto rva=tag==1?0x1347dc0u:tag==2?0x13486f0u:0x1347dd8u;
    if(!a::s::pointer(row.address,tag==1?136:144)||actual_tag!=tag||row.tag!=tag||row.table!=table||table!=source.game_base+rva||row.flags!=flags)return false;
    std::memcpy(&local,row.bytes.data()+12,sizeof local);
    if(std::memcmp(&row.local,&local,sizeof local)||std::memcmp(&local,&expected.local,sizeof local))return false;
    std::array<float,3> half{};float radius{},length{};
    if(tag==2){for(unsigned k=0;k<3;++k)cached::detail::unpack(row.bytes,128+4*k,half[k]);}
    else{cached::detail::unpack(row.bytes,128,radius);if(tag==3)cached::detail::unpack(row.bytes,132,length);}
    return half==row.half&&half==expected.half&&radius==row.radius&&radius==expected.radius&&length==row.full_segment_length&&length==expected.full_segment_length;
}
inline Code match(inv::Snapshot const& inventory,unsigned body_index,cached::Snapshot const& cache,Result& output)noexcept{
    if(a::s::aliases(&inventory,sizeof inventory,&output,sizeof output)||a::s::aliases(&cache,sizeof cache,&output,sizeof output))return Code::alias;
    const auto bodies=inventory.membership.count;
    if(!bodies||bodies>64||body_index>=bodies||inventory.count>a::g::max_primitives||cache.count>cached::capacity)return Code::request;
    unsigned vector_count{};
    if(!cache.source.game_base||(cache.source.game_base&0xffff)||cache.source.game_size<0x1349000||cache.source.game_size>UINTPTR_MAX-cache.source.game_base||
       !cached::detail::vector(cache.vector,vector_count)||vector_count!=cache.count||cache.selected.index>=cached::cache::capacity)return Code::source;
    const auto& member=inventory.membership.rows[body_index];const auto address=member.setup.address;
    if(!a::s::pointer(address,0xec)||cache.cache_source.setup!=address||cache.cache_source.pointers_header!=address+0xcc||
       cache.cache_source.scales_header!=address+0xdc||member.bone.index<=0||
       !a::s::pointer(cache.selected.row.descriptor,128))return Code::source;
    std::array<float,3> scale{};
    if(a::g::metrics::point_scale(inventory.scaling.input,scale)!=a::g::metrics::Code::computed||scale!=inventory.scaling.value||
       cache.requested_scale!=scale)return Code::source;
    for(unsigned k=0;k<3;++k){const float value=cache.selected.row.scale[k];const float delta=value-scale[k];
      if(!std::isnormal(value)||value<=0||!(std::abs(delta)<1e-4f))return Code::source;}
    unsigned total{},begin{},end{};
    for(unsigned n=0;n<64;++n){if(n<bodies&&n==body_index)begin=total;
      for(unsigned k=0;k<4;++k){const auto count=inventory.inventory[n].count[k];
        if((n>=bodies&&count)||(k==3&&count))return Code::unsupported;
        if(count>64||count>inventory.count-total)return Code::inventory;total+=count;
      }
      if(n==body_index)end=total;
    }
    if(total!=inventory.count)return Code::inventory;
    Result next{};next.setup=member.setup;next.bone=member.bone;next.body_index=body_index;next.authored_count=end-begin;
    unsigned at=begin,cache_at{};
    for(unsigned k=0;k<3;++k){const auto count=inventory.inventory[body_index].count[k];const auto& header=inventory.headers[body_index][k];
      if(header.count!=int(count)||header.capacity<header.count||header.capacity>256||
         (!header.data?(header.count||header.capacity):!a::s::pointer(header.data,std::size_t((std::max)(1,header.capacity))*a::stride))||
         inventory.collision_masks[body_index][k]!=1)return Code::inventory;
      a::Source source{};source.header=address+0x6c+16*k;source.body_index=body_index;source.bone=member.bone;
      source.kind=static_cast<a::g::metrics::Kind>(k);source.reflected_extent=k==0?72:k==1?80:76;source.collision_mask=1;source.receiver_scale=inventory.scaling.value;
      if(!a::valid_source(source))return Code::source;
      for(unsigned ordinal=0;ordinal<count;++ordinal,++at){a::Row decoded{};const auto& row=inventory.rows[at];
        if(a::decode(source,row.raw,ordinal,decoded)!=a::Code::captured||!authored::decoded_equal(decoded,row))return Code::raw;
        next.disabled[at-begin]=decoded.collision_disabled;
        creation::Input input{};input.kind=k==0?creation::Kind::sphere:k==1?creation::Kind::box:creation::Kind::capsule;
        std::memcpy(input.tm.data(),decoded.shape.authored.tm.values,sizeof decoded.shape.authored.tm);input.dimensions=decoded.shape.authored.raw_dimensions;
        input.scale=cache.selected.row.scale;input.no_rb_collision=decoded.collision_word;
        creation::Descriptor expected{};const auto code=creation::convert(input,expected);
        if(decoded.collision_disabled){if(code!=creation::Code::disabled)return Code::creation;continue;}
        if(code!=creation::Code::converted)return Code::creation;
        if(cache_at>=cache.count)return Code::mismatch;const auto& actual=cache.rows[cache_at];
        if(!descriptor(cache.source,actual,expected,k+1))return Code::mismatch;
        next.rows[cache_at]={at,cache_at};++cache_at;
      }
    }
    if(at!=end||cache_at!=cache.count)return Code::mismatch;next.count=cache_at;output=next;return Code::matched;
}
}
#endif
