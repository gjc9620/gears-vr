#pragma once
#include "cover_physx_query.h"
#include <float.h>

// Pure geometry for a TRANSLATING capsule. Not an articulated-pose path,
// query result, world fence, self filter, body commit or occupancy permission.
namespace gears_cover_capsule_path {
namespace q=gears_cover_physx_query;
constexpr unsigned max_segments=64;
enum class Code {planned,request,alias,capacity,numeric};
struct Policy {unsigned segment_limit{max_segments};double maximum_radius_expansion{};};
struct Segment {q::Capsule shape{};q::Vector movement{};};
struct Plan {
    q::Capsule original{},endpoint{};q::Vector requested{};unsigned count{};
    double maximum_endpoint_error{},maximum_radius_expansion{};
    std::array<Segment,max_segments> segments{};
};
struct Workspace {Plan candidate{};};
namespace detail {
inline bool nearest_rounding()noexcept {unsigned mode{};return !_controlfp_s(&mode,0,0)&&(mode&_MCW_RC)==_RC_NEAR;}
inline double upward(double value)noexcept{return value==0?0:std::nextafter(value,(std::numeric_limits<double>::infinity)());}
inline double sum_upper(double a,double b)noexcept {
    const double sum=a+b,bv=sum-a,low=(a-(sum-bv))+(b-bv);
    return low>0?upward(sum):sum;
}
inline bool bounded_step(q::Capsule const& shape,q::Vector step)noexcept {
    if(!q::detail::sweep_motion(shape,step))return false;
    // Products of two float24 values are exact in double53. Outward sums
    // retain a tiny second axis even beside an exact 100-unit first axis.
    const double squared=sum_upper(sum_upper(double(step.x)*step.x,double(step.y)*step.y),double(step.z)*step.z);
    constexpr double limit=gears_cover_physx_query_code::capsule_vertex_limit;
    return squared<=limit*limit;
}
// step*i has at most 31 significant bits (float24 * index<=64). TwoSum keeps
// the exact remainder of adding it to the original float endpoint. No large
// world-coordinate subtraction is used to manufacture the next movement.
inline bool coordinate(float original,float step,unsigned index,float& value,double& error)noexcept {
    const double a=original,b=double(step)*index,sum=a+b;
    const double bv=sum-a,low=(a-(sum-bv))+(b-bv);
    if(!std::isfinite(sum)||std::abs(sum)>(std::numeric_limits<float>::max)()||
       (sum!=0&&std::abs(sum)<(std::numeric_limits<float>::min)()))return false;
    value=float(sum);
    if(!q::detail::scalar(value))return false;
    error=upward(std::abs(double(value)-sum)+std::abs(low));return std::isfinite(error);
}
inline bool point(q::Vector original,q::Vector step,unsigned index,q::Vector& value,double& error)noexcept {
    double x{},y{},z{};
    if(!coordinate(original.x,step.x,index,value.x,x)||!coordinate(original.y,step.y,index,value.y,y)||
       !coordinate(original.z,step.z,index,value.z,z))return false;
    error=upward(upward(x+y)+z);return std::isfinite(error);
}
inline bool place(q::Capsule const& original,q::Vector step,unsigned index,Policy const& policy,
                  q::Capsule& value,double& error,double& expansion)noexcept {
    double first{},second{};
    if(!point(original.first,step,index,value.first,first)||!point(original.second,step,index,value.second,second))return false;
    error=(std::max)(first,second);
    const double radius=error==0?double(original.radius):upward(double(original.radius)+error);
    if(!std::isfinite(radius)||radius>(std::numeric_limits<float>::max)())return false;
    float expanded=float(radius);
    if(double(expanded)<radius)expanded=std::nextafter(expanded,(std::numeric_limits<float>::infinity)());
    expansion=double(expanded)-original.radius;
    if(!q::detail::scalar(expanded)||expansion<error||expansion>policy.maximum_radius_expansion)return false;
    value.radius=expanded;return q::detail::valid(value);
}
}
inline Code prepare(q::Capsule const& shape,q::Vector const& movement,Policy const& policy,
                    Workspace& path_workspace,Plan& output)noexcept {
    struct Range {void const* data;std::size_t bytes;};
    const Range ranges[]={{&shape,sizeof shape},{&movement,sizeof movement},{&policy,sizeof policy},
                          {&path_workspace,sizeof path_workspace},{&output,sizeof output}};
    for(unsigned i=0;i<5;++i)for(unsigned j=i+1;j<5;++j)
        if(q::source::aliases(ranges[i].data,ranges[i].bytes,ranges[j].data,ranges[j].bytes))return Code::alias;
    if(!detail::nearest_rounding()||!q::detail::valid(shape)||!q::detail::finite(movement)||
       (movement.x==0&&movement.y==0&&movement.z==0)||!policy.segment_limit||policy.segment_limit>max_segments||
       !std::isfinite(policy.maximum_radius_expansion)||policy.maximum_radius_expansion<0)return Code::request;
    unsigned count=1;q::Vector step=movement;
    while(!detail::bounded_step(shape,step)){
        if(count>policy.segment_limit/2)return Code::capacity;
        count*=2;
        for(float value:{movement.x,movement.y,movement.z})
            if(value!=0&&std::abs(double(value)/count)<(std::numeric_limits<float>::min)())return Code::numeric;
        step={movement.x/float(count),movement.y/float(count),movement.z/float(count)};
        // No lost small axis and no DAZ-dependent subdivision is admitted.
        if(!q::detail::finite(step)||double(step.x)*count!=double(movement.x)||
           double(step.y)*count!=double(movement.y)||double(step.z)*count!=double(movement.z))return Code::numeric;
    }
    auto& next=path_workspace.candidate;next={};next.original=shape;next.requested=movement;next.count=count;
    for(unsigned i=0;i<count;++i){auto& segment=next.segments[i];segment.movement=step;
        double error{},expansion{};
        if(!detail::place(shape,step,i,policy,segment.shape,error,expansion)||
           !detail::bounded_step(segment.shape,step))return Code::numeric;
        next.maximum_endpoint_error=(std::max)(next.maximum_endpoint_error,error);
        next.maximum_radius_expansion=(std::max)(next.maximum_radius_expansion,expansion);
    }
    double endpoint_error{},endpoint_expansion{};
    if(!detail::place(shape,step,count,policy,next.endpoint,endpoint_error,endpoint_expansion))return Code::numeric;
    next.maximum_endpoint_error=(std::max)(next.maximum_endpoint_error,endpoint_error);
    next.maximum_radius_expansion=(std::max)(next.maximum_radius_expansion,endpoint_expansion);
    // Each segment's two endpoint errors are constant under its exact dyadic
    // translation. Their L1 upper bound also bounds every point of the shaft
    // by convexity. The outward radius covers the original entire interval,
    // including joints between independently rounded starts. This statement
    // concerns supplied geometry, not floating-point accuracy of original
    // PhysX intersection algorithms or current-world collision coverage.
    output=next;return Code::planned;
}
} // namespace gears_cover_capsule_path
