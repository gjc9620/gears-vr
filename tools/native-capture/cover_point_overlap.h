#pragma once
#include "cover_primitive_world.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#if defined(_MSC_VER)
#pragma float_control(precise, on, push)
#endif
// Owned narrow-phase math for the Sphere/Sphyl omission in the retained
// aggregate PointCheck. This does NOT discover scene candidates, certify their
// participation/current pose, interpret a native miss, or authorize movement.
// Box is the supplied WORLD axis-aligned query volume, not a scene certificate.
namespace gears_cover_point_overlap {
namespace world=gears_cover_primitive_world;
using Vec=world::Vec;
static_assert(sizeof(double)==8&&sizeof(float)==4&&std::numeric_limits<double>::is_iec559&&
    std::numeric_limits<float>::is_iec559,"Overlap intervals require IEEE binary64/binary32");
struct Box {Vec minimum{},maximum{};};
enum class Status {evaluated,unsupported,invalid,alias,numeric};
enum class Relation {separated,overlapping,indeterminate};
struct Result {
    // Bounds enclose the minimum squared distance from the sphere center or
    // sphyl segment to the box. They are NOT surface clearance or a sweep.
    // Near tangency can be indeterminate; it must never be treated as clear.
    Relation relation{Relation::indeterminate};double lower_squared{},upper_squared{};
    unsigned candidates{};
};
namespace detail {
struct Interval {double lo{},hi{};};
inline bool finite(double x) noexcept {std::uint64_t b{};std::memcpy(&b,&x,8);return ((b>>52)&0x7ff)!=0x7ff;}
inline bool normal_or_zero(double x) noexcept {
    std::uint64_t b{};std::memcpy(&b,&x,8);const auto e=(b>>52)&0x7ff;
    return e!=0x7ff&&(e!=0||(b&0xfffffffffffffull)==0);
}
inline bool normal_or_zero(float x) noexcept {
    std::uint32_t b{};std::memcpy(&b,&x,4);const auto e=(b>>23)&255;
    return e!=255&&(e!=0||(b&0x7fffffu)==0);
}
inline bool zero(Vec p) noexcept {return p.x==0&&p.y==0&&p.z==0;}
struct Math {
    bool okay{true};
    // Reject subnormal/underflow arithmetic instead of relying on the game's
    // FTZ/DAZ setting. Exact zero is kept exact. Each remaining basic operation
    // is enlarged by one adjacent double, enclosing its real result.
    Interval rounded(double x,bool exact_zero) noexcept {
        if(!normal_or_zero(x)||(x==0&&!exact_zero)){okay=false;return {};}
        if(x==0)return {};
        const double lo=std::nextafter(x,-(std::numeric_limits<double>::infinity)());
        const double hi=std::nextafter(x,(std::numeric_limits<double>::infinity)());
        if(!normal_or_zero(lo)||!normal_or_zero(hi)){okay=false;return {};}
        return {lo,hi};
    }
    Interval scalar_add(double a,double b) noexcept {return rounded(a+b,a==-b);}
    Interval scalar_sub(double a,double b) noexcept {return rounded(a-b,a==b);}
    Interval scalar_mul(double a,double b) noexcept {return rounded(a*b,a==0||b==0);}
    Interval scalar_div(double a,double b) noexcept {
        if(b<=0){okay=false;return {};}return rounded(a/b,a==0);
    }
    static bool zero(Interval a) noexcept {return a.lo==0&&a.hi==0;}
    static Interval neg(Interval a) noexcept {return {-a.hi,-a.lo};}
    Interval add(Interval a,Interval b) noexcept {
        if(zero(a))return b;if(zero(b))return a;
        return {scalar_add(a.lo,b.lo).lo,scalar_add(a.hi,b.hi).hi};
    }
    Interval sub(Interval a,Interval b) noexcept {
        if(zero(b))return a;if(zero(a))return neg(b);
        return {scalar_sub(a.lo,b.hi).lo,scalar_sub(a.hi,b.lo).hi};
    }
    Interval mul(Interval a,Interval b) noexcept {
        if(zero(a)||zero(b))return {};
        const Interval values[]={scalar_mul(a.lo,b.lo),scalar_mul(a.lo,b.hi),scalar_mul(a.hi,b.lo),scalar_mul(a.hi,b.hi)};
        Interval out=values[0];for(auto v:values){out.lo=(std::min)(out.lo,v.lo);out.hi=(std::max)(out.hi,v.hi);}return out;
    }
    Interval square(Interval a) noexcept {
        if(zero(a))return {};
        const auto x=scalar_mul(a.lo,a.lo),y=scalar_mul(a.hi,a.hi);
        return {a.lo<=0&&a.hi>=0?0:(std::max)(0.,(std::min)(x.lo,y.lo)),(std::max)(x.hi,y.hi)};
    }
    Interval divide_positive(Interval a,Interval b) noexcept {
        const Interval values[]={scalar_div(a.lo,b.lo),scalar_div(a.lo,b.hi),scalar_div(a.hi,b.lo),scalar_div(a.hi,b.hi)};
        Interval out=values[0];for(auto v:values){out.lo=(std::min)(out.lo,v.lo);out.hi=(std::max)(out.hi,v.hi);}return out;
    }
};
inline bool contains(Interval a,double x) noexcept {return x>=a.lo&&x<=a.hi;}
inline Interval point(double x) noexcept {return {x,x};}
inline Interval clamp_parameter(Interval t) noexcept {
    return {(std::clamp)(t.lo,0.,1.),(std::clamp)(t.hi,0.,1.)};
}
inline Interval box_distance(Math& m,std::array<Interval,3> const& p,Box const& box) noexcept {
    const double low[]={box.minimum.x,box.minimum.y,box.minimum.z},high[]={box.maximum.x,box.maximum.y,box.maximum.z};
    Interval total{};
    for(unsigned i=0;i<3;++i){double lower=0;
        if(p[i].hi<low[i])lower=(std::max)(0.,m.scalar_sub(low[i],p[i].hi).lo);
        else if(p[i].lo>high[i])lower=(std::max)(0.,m.scalar_sub(p[i].lo,high[i]).lo);
        const double upper=(std::max)({0.,m.scalar_sub(low[i],p[i].lo).hi,m.scalar_sub(p[i].hi,high[i]).hi});
        total=m.add(total,m.square({lower,upper}));
    }
    total.lo=(std::max)(0.,total.lo);return total;
}
inline Status validate(world::Primitive const& p,Box const& box) noexcept {
    if(p.dimensions.kind!=world::metrics::Kind::sphere&&p.dimensions.kind!=world::metrics::Kind::sphyl)return Status::unsupported;
    const auto& q=p.shape_world.orientation;const auto& c=p.shape_world.position;
    const double values[]={c.x,c.y,c.z,q.x,q.y,q.z,q.w,box.minimum.x,box.minimum.y,box.minimum.z,
        box.maximum.x,box.maximum.y,box.maximum.z,p.segment_axis_world.x,p.segment_axis_world.y,p.segment_axis_world.z,
        p.segment_start.x,p.segment_start.y,p.segment_start.z,p.segment_end.x,p.segment_end.y,p.segment_end.z};
    for(double x:values){if(!finite(x))return Status::invalid;if(!normal_or_zero(x))return Status::numeric;}
    const double norm=std::hypot(std::hypot(q.x,q.y),std::hypot(q.z,q.w));
    if(!finite(norm)||std::abs(norm-1)>1e-3||box.minimum.x>box.maximum.x||box.minimum.y>box.maximum.y||box.minimum.z>box.maximum.z)return Status::invalid;
    const auto& d=p.dimensions;
    if(!std::isfinite(d.radius)||d.radius<=0||!std::isfinite(d.half_segment)||d.half_segment<0)return Status::invalid;
    if(!normal_or_zero(d.radius)||!normal_or_zero(d.half_segment))return Status::numeric;
    for(float x:d.box_half_extents)if(x!=0)return Status::invalid;
    if(d.kind==world::metrics::Kind::sphere){
        if(d.half_segment!=0||!zero(p.segment_axis_world)||!zero(p.segment_start)||!zero(p.segment_end))return Status::invalid;
    }else if(std::abs(std::hypot(p.segment_axis_world.x,p.segment_axis_world.y,p.segment_axis_world.z)-1)>1e-6)return Status::invalid;
    return Status::evaluated;
}
}
// All non-evaluated statuses preserve output byte-for-byte. Even evaluated
// describes only these supplied shapes; the caller still owns complete scene
// coverage, source/pose/world closure and any later movement transaction.
inline Status classify(world::Primitive const& shape,Box const& box,Result& output) noexcept {
    const world::body::d::Range ranges[]={world::body::d::range(&shape,sizeof shape),
        world::body::d::range(&box,sizeof box),world::body::d::range(&output,sizeof output)};
    if(!world::body::d::disjoint(ranges,3))return Status::alias;
    const auto valid=detail::validate(shape,box);if(valid!=Status::evaluated)return valid;
    using detail::Interval;using detail::point;
    detail::Math m;std::array<Interval,3> a{},b{},delta{};
    const double center[]={shape.shape_world.position.x,shape.shape_world.position.y,shape.shape_world.position.z};
    const double axis[]={shape.segment_axis_world.x,shape.segment_axis_world.y,shape.segment_axis_world.z};
    const double first[]={shape.segment_start.x,shape.segment_start.y,shape.segment_start.z};
    const double last[]={shape.segment_end.x,shape.segment_end.y,shape.segment_end.z};
    const bool capsule=shape.dimensions.kind==world::metrics::Kind::sphyl;
    for(unsigned i=0;i<3;++i){a[i]=b[i]=point(center[i]);
        if(capsule){const auto half=m.mul(point(axis[i]),point(shape.dimensions.half_segment));
            a[i]=m.sub(point(center[i]),half);b[i]=m.add(point(center[i]),half);
            delta[i]=m.mul(point(axis[i]),point(2.*shape.dimensions.half_segment));
            if(!m.okay)return Status::numeric;
            if(!detail::contains(a[i],first[i])||!detail::contains(b[i],last[i]))return Status::invalid;
        }
    }
    const auto radius=m.square(point(shape.dimensions.radius));
    Result next{};next.lower_squared=(std::numeric_limits<double>::max)();next.upper_squared=next.lower_squared;
    const auto visit=[&](std::array<Interval,3> const& p){const auto d=detail::box_distance(m,p,box);
        next.lower_squared=(std::min)(next.lower_squared,d.lo);next.upper_squared=(std::min)(next.upper_squared,d.hi);++next.candidates;};
    visit(a);if(capsule)visit(b);
    if(capsule){const double low[]={box.minimum.x,box.minimum.y,box.minimum.z},high[]={box.maximum.x,box.maximum.y,box.maximum.z};
        // Squared segment-to-box distance is piecewise quadratic. Enumerate
        // all 3^3 below/inside/above regions, without fragile breakpoint sorting.
        // A nonflat interior minimum is -B/A in its actual region. A flat
        // minimum is covered by an endpoint or a plane crossing (one-axis
        // region). Extra roots are harmless real segment points.
        for(unsigned region=1;region<27;++region){unsigned remaining=region;Interval A{},B{};bool varying=false;
            for(unsigned i=0;i<3;++i){const auto side=remaining%3;remaining/=3;if(!side)continue;
                if(!detail::Math::zero(delta[i]))varying=true;
                A=m.add(A,m.square(delta[i]));
                B=m.add(B,m.mul(delta[i],m.sub(a[i],point(side==1?low[i]:high[i]))));
            }
            if(!varying)continue;
            const auto t=A.lo>0?detail::clamp_parameter(m.divide_positive(detail::Math::neg(B),A)):Interval{0,1};
            std::array<Interval,3> p{};for(unsigned i=0;i<3;++i)p[i]=m.add(a[i],m.mul(delta[i],t));
            visit(p);
        }
    }
    if(!m.okay||!detail::normal_or_zero(next.lower_squared)||!detail::normal_or_zero(next.upper_squared))return Status::numeric;
    if(next.lower_squared>radius.hi)next.relation=Relation::separated;
    else if(next.upper_squared<=radius.lo)next.relation=Relation::overlapping;
    else next.relation=Relation::indeterminate;
    output=next;return Status::evaluated;
}
} // namespace gears_cover_point_overlap
#if defined(_MSC_VER)
#pragma float_control(pop)
#endif
