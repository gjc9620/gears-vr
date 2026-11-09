#pragma once
#include "cover_point_overlap.h"

#if defined(_MSC_VER)
#pragma float_control(precise, on, push)
#endif
// One supplied world box versus one supplied WORLD AABB. This does not find
// scene candidates, certify participating shapes/current pose, or sweep a body.
// Uses all 15 separating axes (Eberly, DynamicCollisionDetection, section 2).
// Source: https://www.geometrictools.com/Documentation/DynamicCollisionDetection.pdf
namespace gears_cover_box_overlap {
namespace point=gears_cover_point_overlap;
namespace world=point::world;
using Status=point::Status;using Relation=point::Relation;using Box=point::Box;
struct Result {
    Relation relation{Relation::indeterminate};
    unsigned tested_axes{},uncertain_axes{},separating_axis{}; // axis ID 1..15, 0 when none
};
namespace detail {
using I=point::detail::Interval;using M=point::detail::Math;using V=std::array<I,3>;
using point::detail::point;
inline I absolute(I a) noexcept {
    return {a.lo<=0&&a.hi>=0?0:(std::min)(std::abs(a.lo),std::abs(a.hi)),
        (std::max)(std::abs(a.lo),std::abs(a.hi))};
}
inline I dot(M& m,V const& a,V const& b) noexcept {
    I out{};for(unsigned i=0;i<3;++i)out=m.add(out,m.mul(a[i],b[i]));return out;
}
inline V cross(M& m,V const& a,V const& b) noexcept {
    return {m.sub(m.mul(a[1],b[2]),m.mul(a[2],b[1])),
        m.sub(m.mul(a[2],b[0]),m.mul(a[0],b[2])),
        m.sub(m.mul(a[0],b[1]),m.mul(a[1],b[0]))};
}
inline bool zero(V const& a) noexcept {return M::zero(a[0])&&M::zero(a[1])&&M::zero(a[2]);}
inline Status validate(world::Primitive const& p,Box const& box) noexcept {
    if(p.dimensions.kind!=world::metrics::Kind::box)return Status::unsupported;
    const auto& q=p.shape_world.orientation;const auto& c=p.shape_world.position;
    const double values[]={c.x,c.y,c.z,q.x,q.y,q.z,q.w,box.minimum.x,box.minimum.y,box.minimum.z,
        box.maximum.x,box.maximum.y,box.maximum.z,p.segment_axis_world.x,p.segment_axis_world.y,p.segment_axis_world.z,
        p.segment_start.x,p.segment_start.y,p.segment_start.z,p.segment_end.x,p.segment_end.y,p.segment_end.z};
    for(double x:values){if(!point::detail::finite(x))return Status::invalid;
        if(!point::detail::normal_or_zero(x))return Status::numeric;}
    if(box.minimum.x>box.maximum.x||box.minimum.y>box.maximum.y||box.minimum.z>box.maximum.z)return Status::invalid;
    const double norm=std::hypot(std::hypot(q.x,q.y),std::hypot(q.z,q.w));
    if(!point::detail::finite(norm)||std::abs(norm-1)>1e-3)return Status::invalid;
    if(p.dimensions.radius!=0||p.dimensions.half_segment!=0||!point::detail::zero(p.segment_axis_world)||
       !point::detail::zero(p.segment_start)||!point::detail::zero(p.segment_end))return Status::invalid;
    for(float x:p.dimensions.box_half_extents){
        if(!point::detail::normal_or_zero(x))return std::isfinite(x)?Status::numeric:Status::invalid;
        if(x<=0)return Status::invalid;
    }
    return Status::evaluated;
}
// The geometric orientation is the EXACT normalization of the supplied near-
// unit quaternion. Its matrix entries are rational expressions in q / |q|^2;
// no rounded axis is silently assumed to be exactly unit/perpendicular.
inline std::array<V,3> axes(M& m,world::Quat const& q) noexcept {
    const I x=point(q.x),y=point(q.y),z=point(q.z),w=point(q.w),two=point(2),one=point(1);
    const I xx=m.square(x),yy=m.square(y),zz=m.square(z),ww=m.square(w);
    const I norm=m.add(m.add(xx,yy),m.add(zz,ww));
    if(norm.lo<=0){m.okay=false;return {};}
    const auto twice_over=[&](I value){return m.divide_positive(m.mul(two,value),norm);};
    const I xy=m.mul(x,y),xz=m.mul(x,z),yz=m.mul(y,z),xw=m.mul(x,w),yw=m.mul(y,w),zw=m.mul(z,w);
    // Columns, i.e. the three world-space box axes.
    return {V{m.sub(one,twice_over(m.add(yy,zz))),twice_over(m.add(xy,zw)),twice_over(m.sub(xz,yw))},
        V{twice_over(m.sub(xy,zw)),m.sub(one,twice_over(m.add(xx,zz))),twice_over(m.add(yz,xw))},
        V{twice_over(m.add(xz,yw)),twice_over(m.sub(yz,xw)),m.sub(one,twice_over(m.add(xx,yy)))}};
}
}
// Non-evaluated statuses preserve the old output. Tangency/numerical uncertainty
// is NOT a clear result. Result carries no distance, occupancy or movement right.
inline Status classify(world::Primitive const& shape,Box const& box,Result& output) noexcept {
    const world::body::d::Range ranges[]={world::body::d::range(&shape,sizeof shape),
        world::body::d::range(&box,sizeof box),world::body::d::range(&output,sizeof output)};
    if(!world::body::d::disjoint(ranges,3))return Status::alias;
    const auto valid=detail::validate(shape,box);if(valid!=Status::evaluated)return valid;
    using detail::I;using detail::V;using detail::point;
    detail::M m;const auto oriented=detail::axes(m,shape.shape_world.orientation);
    const std::array<V,3> aligned={V{point(1),point(0),point(0)},V{point(0),point(1),point(0)},V{point(0),point(0),point(1)}};
    const double low[]={box.minimum.x,box.minimum.y,box.minimum.z},high[]={box.maximum.x,box.maximum.y,box.maximum.z};
    const double center[]={shape.shape_world.position.x,shape.shape_world.position.y,shape.shape_world.position.z};
    V delta{},extent{};
    for(unsigned i=0;i<3;++i){
        const I c=m.mul(m.add(point(low[i]),point(high[i])),point(.5));
        delta[i]=m.sub(point(center[i]),c);
        extent[i]=m.mul(m.sub(point(high[i]),point(low[i])),point(.5));
    }
    Result next{};
    const auto visit=[&](V const& axis,unsigned id){
        if(detail::zero(axis))return;
        ++next.tested_axes;
        I support{};
        for(unsigned i=0;i<3;++i){
            support=m.add(support,m.mul(point(shape.dimensions.box_half_extents[i]),detail::absolute(detail::dot(m,oriented[i],axis))));
            support=m.add(support,m.mul(extent[i],detail::absolute(axis[i])));
        }
        const I gap=m.sub(detail::absolute(detail::dot(m,delta,axis)),support);
        if(gap.lo>0&&!next.separating_axis)next.separating_axis=id;
        if(gap.lo<=0&&gap.hi>0)++next.uncertain_axes;
    };
    // Finish the bounded check even after finding a separating axis so numeric
    // failures cannot be hidden by an earlier apparently successful sub-step.
    for(unsigned i=0;i<3;++i)visit(aligned[i],i+1);
    for(unsigned i=0;i<3;++i)visit(oriented[i],i+4);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)visit(detail::cross(m,oriented[i],aligned[j]),7+i*3+j);
    if(!m.okay)return Status::numeric;
    next.relation=next.separating_axis?Relation::separated:
        next.uncertain_axes?Relation::indeterminate:Relation::overlapping;
    output=next;return Status::evaluated;
}
} // namespace gears_cover_box_overlap
#if defined(_MSC_VER)
#pragma float_control(pop)
#endif
