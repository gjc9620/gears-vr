#pragma once
#include "cover_physx_shape_world.h"
// Broad-phase enclosure of the ORIGINAL observed primitive, in PhysX units.
// Box corners use the observed matrix; capsules use its Y column for their
// segment and retain the original unscaled radius; spheres retain that radius.
// No normalization, rigid-query pose, freshness, world fence or motion permit.
namespace gears_cover_physx_observed_bounds {
namespace w=gears_cover_physx_shape_world;
namespace p=gears_cover_physx_observed_pose;
using Kind=w::o::Kind;
enum class Code {bounded,alias,rounding,unsupported,pose,value};
struct Bounds {std::array<double,3> lower{},upper{};};
inline double down(double x)noexcept{return std::nextafter(x,-std::numeric_limits<double>::infinity());}
inline double up(double x)noexcept{return std::nextafter(x,std::numeric_limits<double>::infinity());}
// Pure geometry entry point also supports historical-packet verification.
// It qualifies a supplied matrix, not the origin or lifetime of that matrix.
inline Code primitive(p::Pose const& pose,Kind kind,std::array<float,3> const& half,
                      float radius,float half_segment,Bounds& output)noexcept{
    if(w::s::aliases(&pose,sizeof pose,&output,sizeof output)||
       w::s::aliases(&half,sizeof half,&output,sizeof output))return Code::alias;
    if(!p::nearest())return Code::rounding;
    if(kind!=Kind::box&&kind!=Kind::sphere&&kind!=Kind::capsule)return Code::unsupported;
    if(!p::qualified(pose))return Code::pose;
    if(kind==Kind::box){for(float x:half)if(!w::math::scalar(x)||x<=0)return Code::value;}
    else if(!w::math::scalar(radius)||radius<=0||
            (kind==Kind::capsule&&(!w::math::scalar(half_segment)||half_segment<0)))return Code::value;
    Bounds next{};const auto* m=pose.matrix.values;
    for(unsigned r=0;r<3;++r){
      // float*float fits and is exact in double. Each reduction and final
      // endpoint is rounded outwards; no relative epsilon loses cancellation.
      double extent{};
      if(kind==Kind::box){for(unsigned c=0;c<3;++c)extent=up(extent+std::abs(double(m[3*r+c])*half[c]));}
      else{extent=radius;if(kind==Kind::capsule)extent=up(extent+std::abs(double(m[3*r+1])*half_segment));}
      next.lower[r]=down(double(m[9+r])-extent);next.upper[r]=up(double(m[9+r])+extent);
      if(!std::isfinite(next.lower[r])||!std::isfinite(next.upper[r]))return Code::value;
    }
    output=next;return Code::bounded;
}
// Owned-snapshot adapter replays the original stored/body getter arithmetic
// and composition. A forged world matrix or reported error cannot bypass it.
inline Code observe(w::Snapshot const& source,Bounds& output)noexcept{
    if(w::s::aliases(&source,sizeof source,&output,sizeof output))return Code::alias;
    if(!p::nearest())return Code::rounding;
    if(source.body_pose_present!=(source.parameters.owner.internal_actor!=0))return Code::pose;
    p::Pose stored{},actor{},world{};
    if(!p::quaternion(source.parameters.stored_quaternion,source.parameters.stored_position,stored))return Code::pose;
    if(source.body_pose_present){
      if(!p::quaternion(source.body_quaternion,source.body_position,actor)||
         !p::compose(actor,stored,world)||std::memcmp(&actor.matrix,&source.actor_world,sizeof actor.matrix)||
         actor.gram_error!=source.actor_gram_error)return Code::pose;
    }else world=stored;
    if(std::memcmp(&world.matrix,&source.shape_world,sizeof world.matrix)||world.gram_error!=source.shape_gram_error)return Code::pose;
    return primitive(world,source.parameters.owner.kind,source.parameters.box_dimensions,
                     source.parameters.radius,source.parameters.half_segment,output);
}
}
