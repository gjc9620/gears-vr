#pragma once
#include "cover_physx_shape_parameters.h"
#include "cover_physx_pose_math.h"
#include "cover_physx_shape_world_code.h"
// Original static/dynamic global-shape getter route, read without dispatch.
// Values stay in PhysX units. Caller pins Image, all current shape membership,
// C0 owner/lifetime and query-visible phase. No world fence or movement permit.
namespace gears_cover_physx_shape_world {
namespace p=gears_cover_physx_shape_parameters;namespace o=p::o;namespace s=p::s;namespace raw=s::pose;
namespace math=gears_cover_physx_pose_math;
enum class Code {observed,request,alias,busy,guard,module,parameters,read,pose,changed,owner};
struct Snapshot {p::Parameters parameters{};bool body_pose_present{};std::uintptr_t body_table{};
    std::array<float,3> body_position{};std::array<float,4> body_quaternion{};math::Matrix actor_world{},shape_world{};};
inline bool same(Snapshot const& a,Snapshot const& b)noexcept{return p::same(a.parameters,b.parameters)&&a.body_pose_present==b.body_pose_present&&
    a.body_table==b.body_table&&!std::memcmp(a.body_position.data(),b.body_position.data(),sizeof a.body_position)&&
    !std::memcmp(a.body_quaternion.data(),b.body_quaternion.data(),sizeof a.body_quaternion)&&
    !std::memcmp(&a.actor_world,&b.actor_world,sizeof a.actor_world)&&!std::memcmp(&a.shape_world,&b.shape_world,sizeof a.shape_world);}
struct Workspace {Snapshot first{},last{};std::atomic<bool> active{};p::Code parameter_code{};};
namespace detail {
template<class Read> bool code_current(std::uintptr_t image_base,Read& read){return o::detail::code_current(image_base,read)&&
    gears_cover_physx_shape_parameter_code::current(image_base,read)&&gears_cover_physx_shape_world_code::current(image_base,read);}
template<class Read> Code sample(std::uintptr_t image_base,std::uint32_t size,o::Observation const& owner,Read& read,Workspace& work,Snapshot& output){
    Snapshot next{};work.parameter_code=p::detail::sample(image_base,size,owner,read,next.parameters);
    if(work.parameter_code!=p::Code::observed)return Code::parameters;
    if(!p::detail::slot(read,owner.shape_table,0x80,image_base+0x13e7e0))return Code::module;
    math::Matrix stored{};if(!math::quaternion(next.parameters.stored_quaternion,next.parameters.stored_position,stored))return Code::pose;
    const auto body=owner.internal_actor;next.body_pose_present=body!=0;
    if(!body){
      // Original178ff0 static branch uses stored shape pose directly. Never
      // read an unused Actor cache or invent a dynamic Body for that branch.
      next.shape_world=stored;
    }else{
      if(!s::pointer(body,0x218)||!raw::field(read,next.body_table,body,0))return Code::read;
      if(next.body_table!=image_base+0x226868||!p::detail::slot(read,next.body_table,8,image_base+0x4e50)||
         !p::detail::slot(read,next.body_table,0x10,image_base+0x4e70)||
         !p::detail::slot(read,owner.actor_table,0x48,image_base+0x133fe0))return Code::module;
      for(unsigned n=0;n<3;++n)if(!raw::field(read,next.body_position[n],body,0x20c+n*4))return Code::read;
      for(unsigned n=0;n<4;++n)if(!raw::field(read,next.body_quaternion[n],body,0x1fc+n*4))return Code::read;
      if(!math::quaternion(next.body_quaternion,next.body_position,next.actor_world)||
         !math::compose(next.actor_world,stored,next.shape_world))return Code::pose;
    }
    output=next;return Code::observed;
}
}
template<class Read,class Guard> Code observe(std::uintptr_t image_base,std::uint32_t size,o::Observation const& owner,
    Read read,Guard guard,Workspace& work,Snapshot& output)noexcept{
    static_assert(std::is_invocable_r_v<bool,Guard,o::Observation const&>,"Full current shape membership/C0 guard required");
    if(!image_base||(image_base&0xffff)||size<0x2401c0||size>UINTPTR_MAX-image_base||
       !s::pointer(owner.shape,0x40)||!s::pointer(owner.scene,8))return Code::request;
    if(s::aliases(&owner,sizeof owner,&work,sizeof work)||s::aliases(&owner,sizeof owner,&output,sizeof output)||
       s::aliases(&work,sizeof work,&output,sizeof output))return Code::alias;
    bool inactive=false;if(!work.active.compare_exchange_strong(inactive,true))return Code::busy;
    struct Finish{std::atomic<bool>& active;~Finish(){active=false;}}finish{work.active};const auto key=owner;
    try{
      auto current=[&](){return guard(key)&&o::same(key,owner);};
      if(!current())return Code::guard;if(!detail::code_current(image_base,read))return Code::module;
      auto code=detail::sample(image_base,size,key,read,work,work.first);if(code!=Code::observed)return code;
      if(!current())return Code::guard;
      code=detail::sample(image_base,size,key,read,work,work.last);if(code!=Code::observed)return code;
      if(!same(work.first,work.last))return Code::changed;
      o::Observation closing{};if(o::detail::sample(image_base,size,key.shape,key.scene,read,closing)!=o::Code::observed||!o::same(closing,key))return Code::owner;
      if(!detail::code_current(image_base,read))return Code::module;if(!current())return Code::guard;
      if(!o::same(key,owner))return Code::changed;output=work.first;return Code::observed;
    }catch(...){return Code::read;}
}
}
