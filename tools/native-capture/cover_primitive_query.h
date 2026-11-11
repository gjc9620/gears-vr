#pragma once
#include "cover_primitive_world.h"
#include "cover_physx_query.h"
// Value conversion only. Exact representation is required until an outward
// error-bound converter exists; never round a body smaller to obtain a miss.
// Does not certify inventory, current C0, active geometry or movement safety.
namespace gears_cover_primitive_query {
namespace w=gears_cover_primitive_world;namespace q=gears_cover_physx_query;
enum class Code {converted,alias,unsupported,invalid,precision};
struct Shape {unsigned body_index{},bone_index{};w::bones::Name bone{};w::metrics::Kind kind{};q::Sphere sphere{};q::Box box{};q::Capsule capsule{};};
inline bool exact(double value,float& out) noexcept {
    if(!std::isfinite(value)||std::abs(value)>(std::numeric_limits<float>::max)())return false;
    const float f=float(value);if(!q::detail::scalar(f)||double(f)!=value)return false;out=f;return true;
}
inline bool exact(w::Vec value,q::Vector& out) noexcept {
    return exact(value.x,out.x)&&exact(value.y,out.y)&&exact(value.z,out.z);
}
inline Code convert(w::Primitive const& in,Shape& out) noexcept {
    if(q::source::aliases(&in,sizeof in,&out,sizeof out))return Code::alias;
    if(unsigned(in.dimensions.kind)>unsigned(w::metrics::Kind::sphyl))return Code::unsupported;
    if(in.body_index>=w::bones::max_bodies||in.bone_index>=w::body::bones||in.bone.index<=0)return Code::invalid;
    Shape next{};next.body_index=in.body_index;next.bone_index=in.bone_index;next.bone=in.bone;next.kind=in.dimensions.kind;
    if(next.kind==w::metrics::Kind::sphere){
        if(!exact(in.shape_world.position,next.sphere.centre))return Code::precision;
        next.sphere.radius=in.dimensions.radius;
        if(!q::detail::valid(next.sphere))return Code::invalid;
    }else if(next.kind==w::metrics::Kind::box){
        if(!exact(in.shape_world.position,next.box.centre))return Code::precision;
        next.box.half={in.dimensions.box_half_extents[0],in.dimensions.box_half_extents[1],in.dimensions.box_half_extents[2]};
        w::Quat checked{};
        if(!w::body::d::normalized(in.shape_world.orientation,checked)||
           checked.x!=in.shape_world.orientation.x||checked.y!=in.shape_world.orientation.y||
           checked.z!=in.shape_world.orientation.z||checked.w!=in.shape_world.orientation.w)return Code::invalid;
        const auto x=w::body::d::rotate(checked,{1,0,0}),y=w::body::d::rotate(checked,{0,1,0}),z=w::body::d::rotate(checked,{0,0,1});
        const double values[]{x.x,y.x,z.x,x.y,y.y,z.y,x.z,y.z,z.z};
        for(unsigned i=0;i<9;++i)if(!exact(values[i],next.box.rotation[i]))return Code::precision;
        if(!q::detail::valid(next.box))return Code::invalid;
    }else{
        if(!exact(in.segment_start,next.capsule.first)||!exact(in.segment_end,next.capsule.second))return Code::precision;
        next.capsule.radius=in.dimensions.radius;
        if(!q::detail::valid(next.capsule))return Code::invalid;
    }
    out=next;return Code::converted;
}
}
