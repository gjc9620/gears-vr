#pragma once
#include "pose_math.h"
#include <limits>

// Pure same-space arm geometry. No skeleton indices, local-parent composition,
// engine calls, memory writes, ownership or Native permission are supplied here.
namespace gears_hand_two_bone {
using gears_vr::Vector;
using gears_vr::Quaternion;
using gears_vr::Pose;

enum class LengthMode {preserve,adaptive};
struct LengthPolicy {
    LengthMode mode{LengthMode::preserve};
    // Explicit calibration multipliers over the two measured input lengths.
    double upper_scale{1},lower_scale{1};
};
struct Pole {bool known{};Vector point{};}; // Point in the same space as all poses.
struct Input {
    Pose shoulder{},elbow{},wrist{},target{};
    Pole authored{},previous{};
    LengthPolicy lengths{};
};
enum class Status {solved,invalid_input,invalid_quaternion,invalid_policy,degenerate_bone,unreachable,numerical_failure};
enum class PoleSource {authored,previous,original_elbow,shoulder_frame};
enum class LengthResolution {preserved,calibrated,extended,rebalanced};
struct Solution {
    Pose shoulder{},elbow{},wrist{};
    double upper_length{},lower_length{};
    bool length_adjusted{};
    LengthResolution lengths{LengthResolution::preserved};
    PoleSource pole{PoleSource::shoulder_frame};
};

namespace detail {
constexpr double epsilon=64*std::numeric_limits<double>::epsilon();
inline Vector add(Vector a,Vector b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vector sub(Vector a,Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vector scaled(Vector a,double k) noexcept {return {a.x*k,a.y*k,a.z*k};}
inline double dot(Vector a,Vector b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vector cross(Vector a,Vector b) noexcept {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(Vector v) noexcept {return std::hypot(v.x,v.y,v.z);}
inline bool unit(Vector value,Vector& out) noexcept {
    if(!gears_vr::finite(value))return false;
    const double scale=(std::max)({std::abs(value.x),std::abs(value.y),std::abs(value.z)});
    if(!(scale>0))return false;
    // Division rather than 1/scale also supports representable subnormals.
    const Vector safe{value.x/scale,value.y/scale,value.z/scale};
    const double size=length(safe);
    if(!(size>0)||!std::isfinite(size))return false;
    out={safe.x/size,safe.y/size,safe.z/size};return gears_vr::finite(out);
}
inline bool quaternion(Quaternion value,Quaternion& out) noexcept {
    if(!std::isfinite(value.x)||!std::isfinite(value.y)||!std::isfinite(value.z)||!std::isfinite(value.w))return false;
    const double scale=(std::max)({std::abs(value.x),std::abs(value.y),std::abs(value.z),std::abs(value.w)});
    if(!(scale>0))return false;
    const Quaternion small{value.x/scale,value.y/scale,value.z/scale,value.w/scale};
    const double size=std::sqrt(small.x*small.x+small.y*small.y+small.z*small.z+small.w*small.w);
    if(!(size>0)||!std::isfinite(size))return false;
    out={small.x/size,small.y/size,small.z/size,small.w/size};return true;
}
inline Vector rotated(Quaternion q,Vector v) noexcept {
    const Vector u{q.x,q.y,q.z};
    return add(v,scaled(add(scaled(cross(u,v),q.w),cross(u,cross(u,v))),2));
}
inline bool perpendicular(Vector candidate,Vector axis,Vector& out) noexcept {
    Vector direction{};
    if(!unit(candidate,direction))return false;
    const auto projected=sub(direction,scaled(axis,dot(direction,axis)));
    if(length(projected)<=epsilon)return false;
    return unit(projected,out);
}
inline bool frame_perpendicular(Quaternion frame,Vector axis,Vector& out) noexcept {
    // These are the original orientation's basis directions, not assumptions
    // about the actual bone's local axis. A tolerance-stable tie keeps rigid
    // transforms from swapping equally suitable basis directions.
    const Vector local[]={{1,0,0},{0,1,0},{0,0,1}};
    double best=-1;Vector selected{};
    for(auto basis:local){
        const auto world=rotated(frame,basis);
        const auto projected=sub(world,scaled(axis,dot(world,axis)));
        const double score=dot(projected,projected);
        if(score>best+epsilon){best=score;selected=projected;}
    }
    return best>epsilon&&unit(selected,out);
}
inline bool swing(Vector from,Vector to,Vector normal,Quaternion frame,Quaternion& out) noexcept {
    Vector a{},b{};
    if(!unit(from,a)||!unit(to,b))return false;
    const double cosine=(std::clamp)(dot(a,b),-1.,1.);
    const auto product=cross(a,b);const double sine=length(product);
    if(sine>epsilon){
        const double half=std::atan2(sine,cosine)/2;
        const auto axis=scaled(product,1/sine);const double amount=std::sin(half);
        return quaternion({axis.x*amount,axis.y*amount,axis.z*amount,std::cos(half)},out);
    }
    if(cosine>=0){out={};return true;}
    Vector axis{};
    if(!perpendicular(normal,a,axis)&&!frame_perpendicular(frame,a,axis))return false;
    out={axis.x,axis.y,axis.z,0};return true;
}
inline bool same_length(double actual,double wanted,double scale) noexcept {
    const double tolerance=(std::max)(epsilon*8*scale,8*std::numeric_limits<double>::denorm_min());
    return std::isfinite(actual)&&actual>0&&std::abs(actual-wanted)<=tolerance;
}
} // namespace detail

// Failure leaves output byte-for-byte untouched. preserve rejects an unreachable
// target, including the unequal-length shoulder target. adaptive extends both
// lengths proportionally outside reach. Inside minimum reach it preserves the
// total length and smoothly reduces the signed difference to d^2/old_difference;
// at zero distance both become half the total. This may change their ratio.
// No target projection, shoulder-radius clamp or model-specific length exists.
inline Status solve(Input const& input,Solution& output) noexcept {
    using namespace detail;
    if(!gears_vr::finite(input.shoulder.position)||!gears_vr::finite(input.elbow.position)||
       !gears_vr::finite(input.wrist.position)||!gears_vr::finite(input.target.position)||
       (input.authored.known&&!gears_vr::finite(input.authored.point))||
       (input.previous.known&&!gears_vr::finite(input.previous.point)))return Status::invalid_input;
    Quaternion shoulder{},elbow{},wrist{},target{};
    if(!quaternion(input.shoulder.orientation,shoulder)||!quaternion(input.elbow.orientation,elbow)||
       !quaternion(input.wrist.orientation,wrist)||!quaternion(input.target.orientation,target))return Status::invalid_quaternion;
    (void)wrist;
    auto const& policy=input.lengths;
    if((policy.mode!=LengthMode::preserve&&policy.mode!=LengthMode::adaptive)||
       !std::isfinite(policy.upper_scale)||!std::isfinite(policy.lower_scale)||
       policy.upper_scale<=0||policy.lower_scale<=0)return Status::invalid_policy;
    const auto old_upper=sub(input.elbow.position,input.shoulder.position);
    const auto old_lower=sub(input.wrist.position,input.elbow.position);
    const auto displacement=sub(input.target.position,input.shoulder.position);
    if(!gears_vr::finite(old_upper)||!gears_vr::finite(old_lower)||!gears_vr::finite(displacement))return Status::numerical_failure;
    const double original_upper=length(old_upper),original_lower=length(old_lower),distance=length(displacement);
    if(original_upper==0||original_lower==0)return Status::degenerate_bone;
    const double upper=original_upper*policy.upper_scale,lower=original_lower*policy.lower_scale;
    if(!std::isfinite(upper)||!std::isfinite(lower)||!std::isfinite(distance)||upper<=0||lower<=0)return Status::numerical_failure;
    const double scale=(std::max)({upper,lower,distance});
    double a=upper/scale,b=lower/scale;const double d=distance/scale;
    if(a<=0||b<=0)return Status::numerical_failure;
    const double total=a+b,old_difference=a-b;
    // Classify against the measured lengths before normalization. Dividing a
    // valid closed boundary first can turn it into a one-ULP false rejection.
    // Subtraction avoids overflow of upper+lower; no target tolerance/clamp.
    const bool outside=distance>upper&&distance-upper>lower;
    const bool inside=distance<std::abs(upper-lower);
    if(policy.mode==LengthMode::preserve&&(outside||inside))return Status::unreachable;
    Solution candidate{};
    candidate.length_adjusted=policy.upper_scale!=1||policy.lower_scale!=1||outside||inside;
    candidate.lengths=candidate.length_adjusted?LengthResolution::calibrated:LengthResolution::preserved;
    double x{};bool straight=false;
    if(outside){
        const double fraction=a/total;
        a=d*fraction;b=d*(1-fraction);x=a;straight=true;
        candidate.lengths=LengthResolution::extended;
    }else if(inside){
        const double fraction=d/std::abs(old_difference);
        const double difference=std::copysign(d*fraction,old_difference);
        a=(total+difference)/2;b=(total-difference)/2;
        x=d==0?0:(d+std::copysign(fraction,old_difference)*total)/2;
        candidate.lengths=LengthResolution::rebalanced;
    }else if(d!=0){
        x=(d+(a-b)*(a+b)/d)/2;
    }
    if(a<=0||b<=0||!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(x))return Status::numerical_failure;
    Vector axis{};
    if(distance>0){if(!unit(displacement,axis))return Status::numerical_failure;}
    else {
        // At the shoulder there is no target direction. Pick a perpendicular
        // axis so the highest-priority usable pole (or original elbow) itself
        // remains the bend direction rather than being projected away.
        Vector hint{};bool found=false;
        for(auto const& pole:{input.authored,input.previous})if(pole.known&&!found){
            const auto value=sub(pole.point,input.shoulder.position);
            if(!gears_vr::finite(value))return Status::numerical_failure;
            found=unit(value,hint);
        }
        if(!found&&!unit(old_upper,hint))return Status::numerical_failure;
        if(!frame_perpendicular(shoulder,hint,axis))return Status::numerical_failure;
    }
    Vector radial{};bool selected=false;
    if(input.authored.known){
        const auto pole=sub(input.authored.point,input.shoulder.position);
        if(!gears_vr::finite(pole))return Status::numerical_failure;
        selected=perpendicular(pole,axis,radial);if(selected)candidate.pole=PoleSource::authored;
    }
    if(!selected&&input.previous.known){
        const auto pole=sub(input.previous.point,input.shoulder.position);
        if(!gears_vr::finite(pole))return Status::numerical_failure;
        selected=perpendicular(pole,axis,radial);if(selected)candidate.pole=PoleSource::previous;
    }
    if(!selected){selected=perpendicular(old_upper,axis,radial);if(selected)candidate.pole=PoleSource::original_elbow;}
    if(!selected&&!frame_perpendicular(shoulder,axis,radial))return Status::numerical_failure;
    double ratio=x/a;
    if(!std::isfinite(ratio)||std::abs(ratio)>1+epsilon*8)return Status::numerical_failure;
    ratio=(std::clamp)(ratio,-1.,1.);
    const double height=straight?0:a*std::sqrt((std::max)(0.,(1-ratio)*(1+ratio)));
    const auto offset=add(scaled(axis,x*scale),scaled(radial,height*scale));
    candidate.shoulder.position=input.shoulder.position;
    candidate.elbow.position=add(input.shoulder.position,offset);
    candidate.wrist.position=input.target.position;candidate.wrist.orientation=target;
    if(!gears_vr::finite(candidate.elbow.position))return Status::numerical_failure;
    const auto new_upper=sub(candidate.elbow.position,candidate.shoulder.position);
    const auto new_lower=sub(candidate.wrist.position,candidate.elbow.position);
    candidate.upper_length=length(new_upper);candidate.lower_length=length(new_lower);
    if(!same_length(candidate.upper_length,a*scale,scale)||!same_length(candidate.lower_length,b*scale,scale))return Status::numerical_failure;
    const auto normal=cross(axis,radial);Quaternion upper_delta{},lower_delta{};
    if(!swing(old_upper,new_upper,normal,shoulder,upper_delta)||!swing(old_lower,new_lower,normal,elbow,lower_delta)||
       !quaternion(gears_vr::multiply(upper_delta,shoulder),candidate.shoulder.orientation)||
       !quaternion(gears_vr::multiply(lower_delta,elbow),candidate.elbow.orientation))return Status::numerical_failure;
    output=candidate;return Status::solved;
}
} // namespace gears_hand_two_bone
