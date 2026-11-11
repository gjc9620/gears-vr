#pragma once
#include "cover_physx_shape_owner.h"
#include "cover_physx_shape_parameter_code.h"
#include <cmath>
// Current stored shape parameters only. No getter dispatch/cache update, no
// actor-world composition or conversion to game units. Caller pins the exact
// original Image and closes full ownership/membership/C0 lifetime in Guard.
namespace gears_cover_physx_shape_parameters {
namespace o=gears_cover_physx_shape_owner;namespace s=gears_cover_physx_query_source;namespace raw=s::pose;
enum class Code {observed,request,alias,busy,guard,module,owner,read,unsupported,value,changed};
struct Parameters {o::Observation owner{};std::array<float,3> stored_position{};std::array<float,4> stored_quaternion{};
    // Original bd10 builds local corners from +/-this vector: half dimensions,
    // in PhysX units. Retained raw here; shape_geometry interprets owned values.
    std::array<float,3> box_dimensions{};float radius{},half_segment{};unsigned flags{};};
inline bool same(Parameters const& a,Parameters const& b)noexcept{return o::same(a.owner,b.owner)&&
    !std::memcmp(a.stored_position.data(),b.stored_position.data(),sizeof a.stored_position)&&
    !std::memcmp(a.stored_quaternion.data(),b.stored_quaternion.data(),sizeof a.stored_quaternion)&&
    !std::memcmp(a.box_dimensions.data(),b.box_dimensions.data(),sizeof a.box_dimensions)&&
    !std::memcmp(&a.radius,&b.radius,sizeof a.radius)&&!std::memcmp(&a.half_segment,&b.half_segment,sizeof a.half_segment)&&a.flags==b.flags;}
struct Workspace {Parameters first{},last{};std::atomic<bool> active{};};
namespace detail {
template<class Read> bool slot(Read& read,std::uintptr_t table,unsigned offset,std::uintptr_t expected){
    std::uintptr_t actual{};return raw::field(read,actual,table,offset)&&actual==expected;
}
template<class Read> Code sample(std::uintptr_t image_base,std::uint32_t size,o::Observation const& expected,Read& read,Parameters& out){
    Parameters next{};const auto owner_code=o::detail::sample(image_base,size,expected.shape,expected.scene,read,next.owner);
    if(owner_code!=o::Code::observed||!o::same(next.owner,expected))return Code::owner;
    const auto table=next.owner.internal_shape_table,shape=next.owner.internal_shape;
    std::uint32_t wanted{};if(expected.kind==o::Kind::box)wanted=0x226c80;
    else if(expected.kind==o::Kind::capsule)wanted=0x2272d0;else if(expected.kind==o::Kind::sphere)wanted=0x2303e8;else return Code::unsupported;
    if(table!=image_base+wanted||!s::pointer(shape,0x1c4)||
       !slot(read,table,0x68,image_base+0xb1350)||!slot(read,table,0x70,image_base+0xb1370)||!slot(read,table,0x98,image_base+0xc460)||
       !slot(read,expected.shape_table,0x30,image_base+0x13e7a0))return Code::module;
    if(expected.kind==o::Kind::box){
      if(!slot(read,table,0x140,image_base+0xc410)||!slot(read,expected.shape_table,0x130,image_base+0x13e780))return Code::module;
      for(unsigned n=0;n<3;++n)if(!raw::field(read,next.box_dimensions[n],shape,0x1b8+n*4))return Code::read;
    }else{
      const auto public_radius=expected.kind==o::Kind::capsule?0x138u:0x130u;
      if(!slot(read,table,0x158,image_base+0x12770)||!slot(read,expected.shape_table,public_radius,image_base+0x13fc90))return Code::module;
      if(!raw::field(read,next.radius,shape,0x1b0))return Code::read;
      if(expected.kind==o::Kind::capsule){
        if(!slot(read,table,0x160,image_base+0x12760)||!slot(read,expected.shape_table,0x148,image_base+0x13fc80))return Code::module;
        if(!raw::field(read,next.half_segment,shape,0x1b4))return Code::read;
      }
    }
    for(unsigned n=0;n<3;++n)if(!raw::field(read,next.stored_position[n],shape,0x88+n*4))return Code::read;
    for(unsigned n=0;n<4;++n)if(!raw::field(read,next.stored_quaternion[n],shape,0x78+n*4))return Code::read;
    if(!raw::field(read,next.flags,shape,0x180))return Code::read;
    for(float v:next.stored_position)if(!std::isfinite(v))return Code::value;
    for(float v:next.stored_quaternion)if(!std::isfinite(v))return Code::value;
    if(expected.kind==o::Kind::box){for(float v:next.box_dimensions)if(!std::isfinite(v)||v<=0)return Code::value;}
    else if(!std::isfinite(next.radius)||next.radius<=0||!std::isfinite(next.half_segment)||next.half_segment<0)return Code::value;
    out=next;return Code::observed;
}
}
template<class Read,class Guard> Code observe(std::uintptr_t image_base,std::uint32_t size,o::Observation const& owner,
    Read read,Guard guard,Workspace& work,Parameters& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard,o::Observation const&>,"Full current shape ownership/membership guard required");
    if(!image_base||(image_base&0xffff)||size<0x2401c0||size>UINTPTR_MAX-image_base||
       !s::pointer(owner.shape,0x40)||!s::pointer(owner.scene,8))return Code::request;
    if(s::aliases(&owner,sizeof owner,&work,sizeof work)||s::aliases(&owner,sizeof owner,&output,sizeof output)||
       s::aliases(&work,sizeof work,&output,sizeof output))return Code::alias;
    bool inactive=false;if(!work.active.compare_exchange_strong(inactive,true))return Code::busy;
    struct Finish{std::atomic<bool>& active;~Finish(){active=false;}} finish{work.active};const auto key=owner;
    try{
      auto current=[&](){return guard(key)&&o::same(key,owner);};
      if(!current())return Code::guard;
      if(!o::detail::code_current(image_base,read)||!gears_cover_physx_shape_parameter_code::current(image_base,read))return Code::module;
      auto code=detail::sample(image_base,size,key,read,work.first);if(code!=Code::observed)return code;
      if(!current())return Code::guard;
      code=detail::sample(image_base,size,key,read,work.last);if(code!=Code::observed)return code;
      if(!same(work.first,work.last))return Code::changed;
      o::Observation closing{};if(o::detail::sample(image_base,size,key.shape,key.scene,read,closing)!=o::Code::observed||!o::same(closing,key))return Code::owner;
      if(!o::detail::code_current(image_base,read)||!gears_cover_physx_shape_parameter_code::current(image_base,read))return Code::module;
      if(!current())return Code::guard;
      if(!o::same(key,owner))return Code::changed;
      output=work.first;return Code::observed;
    }catch(...){return Code::read;}
}
}
