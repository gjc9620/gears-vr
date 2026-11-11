#pragma once
#include "cover_physx_shape_creation.h"
#include <cfenv>
// Owned representation bridge for original Actor124660 -> ShapeB0790 and
// Capsule104b0. No constructor executes. Dynamic stores descriptor-local pose;
// static stores ActorWorld*descriptor-local pose. Caller certifies that actor
// pose and the original code/lifetime; this is not active-shape identity proof.
namespace gears_cover_physx_shape_construction {
namespace creation=gears_cover_physx_shape_creation;namespace math=creation::g::w::math;
using Matrix=math::Matrix;using Kind=creation::Kind;
static_assert(sizeof(Matrix)==48,"Original stored pose matrix is48 bytes");
enum class Placement {dynamic_local,static_world};
enum class Code {converted,request,alias,rounding,pose,value};
struct Stored {Kind kind{Kind::unknown};Placement placement{};std::array<float,3> position{};
    std::array<float,4> quaternion{};std::array<float,3> half{};float radius{},half_segment{};};
namespace detail {
inline bool compose_static(Matrix const& actor_pose,Matrix const& local,Matrix& output)noexcept{
    if(!math::rigid(actor_pose)||!math::rigid(local))return false;Matrix next{};
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c){
      const float y=actor_pose.values[3*r+1]*local.values[3+c],x=actor_pose.values[3*r]*local.values[c];
      const float sum=y+x;next.values[3*r+c]=sum+actor_pose.values[3*r+2]*local.values[6+c];}
    for(unsigned r=0;r<3;++r){const float x=local.values[9]*actor_pose.values[3*r],y=local.values[10]*actor_pose.values[3*r+1],z=local.values[11]*actor_pose.values[3*r+2];
      // Original124660 row0 uses Y+X+Z; rows1/2 use X+Z+Y.
      const float first=r==0?y+x:x+z,second=r==0?first+z:first+y;
      next.values[9+r]=second+actor_pose.values[9+r];}
    if(!math::rigid(next))return false;output=next;return true;
}
inline bool quaternion(Matrix const& m,Placement placement,std::array<float,4>& output)noexcept{
    if(!math::rigid(m))return false;const auto* v=m.values;std::array<float,4> q{};
    const float first=placement==Placement::static_world?v[8]+v[4]:v[4]+v[0];
    const float trace=first+(placement==Placement::static_world?v[0]:v[8]);
    if(trace<0){const unsigned selected=v[0]<v[4]?1:0;
      if(v[4*selected]<v[8]){const float value=std::sqrt((v[8]-(v[4]+v[0]))+1.f),factor=.5f/value;
        q[3]=(v[3]-v[1])*factor;q[0]=(v[6]+v[2])*factor;q[2]=value*.5f;q[1]=(v[7]+v[5])*factor;
      }else if(selected){const float value=std::sqrt((v[4]-(v[8]+v[0]))+1.f),factor=.5f/value;
        q[3]=(v[2]-v[6])*factor;q[0]=(v[1]+v[3])*factor;q[2]=(v[7]+v[5])*factor;q[1]=value*.5f;
      }else{const float value=std::sqrt((v[0]-(v[8]+v[4]))+1.f),factor=.5f/value;
        q[3]=(v[7]-v[5])*factor;q[0]=value*.5f;q[2]=(v[6]+v[2])*factor;q[1]=(v[3]+v[1])*factor;}
    }else{const float value=std::sqrt(trace+1.f),factor=.5f/value;
      q[3]=value*.5f;q[0]=(v[7]-v[5])*factor;q[2]=(v[3]-v[1])*factor;q[1]=(v[2]-v[6])*factor;}
    Matrix checked{};if(!math::quaternion(q,{m.values[9],m.values[10],m.values[11]},checked))return false;
    output=q;return true;
}
}
inline Code convert(creation::Descriptor const& descriptor,Placement placement,Matrix const& actor_pose,Stored& output)noexcept{
    namespace s=creation::g::w::s;
    if(s::aliases(&descriptor,sizeof descriptor,&output,sizeof output)||s::aliases(&actor_pose,sizeof actor_pose,&output,sizeof output))return Code::alias;
    if(placement!=Placement::dynamic_local&&placement!=Placement::static_world)return Code::request;
    if(std::fegetround()!=FE_TONEAREST)return Code::rounding;
    const auto kind=descriptor.kind;if(kind!=Kind::box&&kind!=Kind::sphere&&kind!=Kind::capsule)return Code::request;
    Matrix pose=descriptor.local;
    if(placement==Placement::static_world&&!detail::compose_static(actor_pose,descriptor.local,pose))return Code::pose;
    Stored next{};next.kind=kind;next.placement=placement;
    if(!detail::quaternion(pose,placement,next.quaternion))return Code::pose;
    for(unsigned k=0;k<3;++k)next.position[k]=pose.values[9+k];
    if(kind==Kind::box){for(unsigned k=0;k<3;++k){const float v=descriptor.half[k];if(!std::isnormal(v)||v<=0)return Code::value;next.half[k]=v;}}
    else{if(!std::isnormal(descriptor.radius)||descriptor.radius<=0)return Code::value;next.radius=descriptor.radius;
      if(kind==Kind::capsule){const float length=descriptor.full_segment_length;
        if(!math::scalar(length)||length<0)return Code::value;next.half_segment=length*.5f;
        if(!math::scalar(next.half_segment)||(length!=0&&next.half_segment==0))return Code::value;}}
    output=next;return Code::converted;
}
}
