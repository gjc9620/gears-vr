#pragma once
#include "cover_physx_shape_world.h"
// Interpretation of an owned observation, in native PhysX units throughout.
// Original BoxShape bd10 builds corners from +/-1b8/1bc/1c0; 12780
// builds capsule endpoints from column Y times half_segment. This does not
// match authored bodies, certify freshness, select query flags or permit motion.
namespace gears_cover_physx_shape_geometry {
namespace w=gears_cover_physx_shape_world;
using Kind=w::o::Kind;
enum class Code {converted,alias,unsupported,pose,value};
struct Geometry {
    Kind kind{Kind::unknown};
    std::array<double,3> centre{},half{},endpoint_a{},endpoint_b{};
    std::array<double,9> orientation{};double radius{};
};
inline Code convert(w::Snapshot const& source,Geometry& output)noexcept{
    if(w::s::aliases(&source,sizeof source,&output,sizeof output))return Code::alias;
    const auto kind=source.parameters.owner.kind;
    if(kind!=Kind::box&&kind!=Kind::sphere&&kind!=Kind::capsule)return Code::unsupported;
    // This strict adapter requires the original raw quaternion contract too;
    // an observed proper matrix (for example q.w=2 with xyz=0) cannot bypass
    // unit-pose qualification merely because its getter matrix is identity.
    w::math::Matrix stored{},expected{},actor_pose{};
    if(source.body_pose_present!=(source.parameters.owner.internal_actor!=0)||
       !w::math::quaternion(source.parameters.stored_quaternion,source.parameters.stored_position,stored))return Code::pose;
    if(source.body_pose_present){
      if(!w::math::quaternion(source.body_quaternion,source.body_position,actor_pose)||
         !w::math::compose(actor_pose,stored,expected))return Code::pose;
    }else expected=stored;
    if(std::memcmp(&expected,&source.shape_world,sizeof expected)||!w::math::rigid(source.shape_world))return Code::pose;
    Geometry next{};next.kind=kind;const auto* m=source.shape_world.values;
    for(unsigned n=0;n<3;++n)next.centre[n]=m[9+n];
    if(kind==Kind::box){
      for(unsigned n=0;n<3;++n){const auto x=source.parameters.box_dimensions[n];
        if(!w::math::scalar(x)||x<=0)return Code::value;next.half[n]=x;}
      for(unsigned n=0;n<9;++n)next.orientation[n]=m[n];
    }else{
      const auto radius=source.parameters.radius;
      if(!w::math::scalar(radius)||radius<=0)return Code::value;next.radius=radius;
      if(kind==Kind::capsule){const auto half=source.parameters.half_segment;
        if(!w::math::scalar(half)||half<0)return Code::value;
        for(unsigned n=0;n<3;++n){const double delta=double(m[3*n+1])*half;
          next.endpoint_a[n]=next.centre[n]-delta;next.endpoint_b[n]=next.centre[n]+delta;}
      }
    }
    output=next;return Code::converted;
}
}
