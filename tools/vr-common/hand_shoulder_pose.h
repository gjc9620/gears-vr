#pragma once
#include "hand_two_bone_pose.h"

// Cosmetic joint placement only, in one already-authenticated coordinate
// space. No tracking, old pose, clock, source/lease or engine permission.
// A fixed clavicle and upper/lower arm are rotated; no translation/scale is
// edited. The caller retains the actual desired wrist position/orientation.
namespace gears_hand_shoulder {
namespace two=gears_hand_two_bone;
using two::Pose;using two::Vector;using two::Quaternion;
struct Input {two::Input arm{};Pose clavicle{};Vector up{},outward{};double units_per_metre{100};
    // Cosmetic attachment endpoint only. Neutral/free and release keep the
    // original arm.authored pole; no history or interaction permission.
    two::Pole attached_body_pole{};
};
struct Solution {
    Pose clavicle{};two::Solution arm{};
    double wrist_bend{},clavicle_turn{},shoulder_displacement{};
    bool adjusted{};
};
namespace detail {
constexpr double pi=3.14159265358979323846;
constexpr double maximum_turn=pi/6; // Bounded scapular motion, not extra reach/stretch.
struct Candidate {Solution result{};double score{},elevation{},protraction{};};
inline double bend(two::Solution const& arm,Vector forward) noexcept {
    Vector axis{};if(!two::detail::unit(two::detail::sub(arm.wrist.position,arm.elbow.position),axis))return pi;
    return std::acos((std::clamp)(two::detail::dot(axis,forward),-1.,1.));
}
// Failure leaves output unchanged. A large fixed wrist bend is reduced by
// moving a bounded shoulder before folding the hand. The endpoint stays exact;
// a candidate outside the original fixed arm lengths is simply not usable.
inline bool evaluate(Input const& input,Vector base,Vector tangent_up,Vector tangent_side,
    Vector up,Vector outward,Vector forward,Quaternion clavicle_q,double radius,
    double elevation,double protraction,Candidate& output) noexcept {
    using namespace two::detail;
    if(elevation < -pi/12||elevation>maximum_turn||std::abs(protraction)>maximum_turn)return false;
    const auto direction=add(add(scaled(base,std::cos(elevation)*std::cos(protraction)),
        scaled(tangent_side,std::cos(elevation)*std::sin(protraction))),scaled(tangent_up,std::sin(elevation)));
    const double cosine=(std::clamp)(dot(base,direction),-1.,1.);
    if(cosine<std::cos(maximum_turn)-1e-12)return false;
    Quaternion delta{};
    if(!swing(base,direction,up,clavicle_q,delta))return false;
    auto arm=input.arm;
    const auto move=[&](Pose p){return Pose{add(input.clavicle.position,rotated(delta,sub(p.position,input.clavicle.position))),
        gears_vr::multiply(delta,p.orientation)};};
    arm.shoulder=move(arm.shoulder);arm.elbow=move(arm.elbow);arm.wrist=move(arm.wrist);
    arm.authored={true,add(arm.shoulder.position,sub(scaled(outward,.35),up))};arm.previous={};
    Candidate c{};if(two::solve(arm,c.result.arm)!=two::Status::solved)return false;
    // Do not trade a straight wrist for an elbow over the hand/shoulder or
    // substantially inward through the torso. The base pose remains fallback.
    const auto elbow=c.result.arm.elbow.position;
    if(dot(elbow,up)>(std::max)(dot(arm.shoulder.position,up),dot(arm.target.position,up))+.02*input.units_per_metre||
       dot(sub(elbow,input.arm.shoulder.position),outward)<-.10*input.units_per_metre)return false;
    c.result.clavicle={input.clavicle.position,gears_vr::multiply(delta,clavicle_q)};
    c.result.wrist_bend=bend(c.result.arm,forward);c.result.clavicle_turn=std::acos(cosine);
    c.result.shoulder_displacement=length(sub(arm.shoulder.position,input.arm.shoulder.position));
    if(!same_length(length(sub(arm.shoulder.position,input.clavicle.position)),radius,radius))return false;
    const double excess=(std::max)(0.,c.result.wrist_bend-input.arm.preferred_wrist_bend)*180/pi;
    const double centimetres=c.result.shoulder_displacement*100/input.units_per_metre;
    c.score=excess*excess+centimetres*centimetres;c.elevation=elevation;c.protraction=protraction;
    if(!std::isfinite(c.score))return false;output=c;return true;
}
}
inline two::Status solve(Input const& input,Solution& output) noexcept {
    using namespace two::detail;
    if(input.arm.lengths.mode!=two::LengthMode::preserve||input.arm.lengths.upper_scale!=1||
       input.arm.lengths.lower_scale!=1)return two::Status::invalid_policy;
    if(!gears_vr::finite(input.clavicle.position)||!std::isfinite(input.units_per_metre)||
       input.units_per_metre<10||input.units_per_metre>500)return two::Status::invalid_input;
    Quaternion clavicle_q{};Vector up{},outward{},base{},tangent_up{},tangent_side{},forward{};
    if(!quaternion(input.clavicle.orientation,clavicle_q))return two::Status::invalid_quaternion;
    const auto offset=sub(input.arm.shoulder.position,input.clavicle.position);const double radius=length(offset);
    if(!unit(input.up,up)||!unit(input.outward,outward)||!unit(offset,base)||
       !perpendicular(up,base,tangent_up)||!unit(cross(tangent_up,base),tangent_side))return two::Status::invalid_input;
    detail::Candidate best{};const auto initial=two::solve(input.arm,best.result.arm);
    if(initial!=two::Status::solved)return initial;
    Quaternion wrist_q{};
    if(!quaternion(best.result.arm.wrist.orientation,wrist_q)||
       !unit(rotated(wrist_q,input.arm.wrist_forward_local),forward))return two::Status::invalid_input;
    best.result.clavicle=input.clavicle;best.result.wrist_bend=detail::bend(best.result.arm,forward);
    const double excess=(std::max)(0.,best.result.wrist_bend-input.arm.preferred_wrist_bend)*180/detail::pi;
    best.score=excess*excess;
    if(input.arm.preferred_wrist_bend<=0||excess<1e-7){output=best.result;return two::Status::solved;}
    const auto consider=[&](double elevation,double protraction){detail::Candidate c{};
        if(detail::evaluate(input,base,tangent_up,tangent_side,up,outward,forward,clavicle_q,radius,elevation,protraction,c)&&
           c.score<best.score-1e-9)best=c;};
    // Bounded deterministic search/refinement. No history-dependent wrist fit,
    // per-frame enumeration of the game, or new source publication is involved.
    for(double e:{-detail::pi/12,0.,detail::pi/12,detail::pi/6})
        for(double p:{-detail::pi/6,-detail::pi/12,0.,detail::pi/12,detail::pi/6})consider(e,p);
    for(double step=detail::pi/24;step>=detail::pi/384;step*=.5){
        const auto e=best.elevation,p=best.protraction;
        for(int de=-1;de<=1;++de)for(int dp=-1;dp<=1;++dp)if(de||dp)consider(e+de*step,p+dp*step);
    }
    best.result.adjusted=best.result.shoulder_displacement>1e-7;output=best.result;return two::Status::solved;
}
// Blend complete, valid geometric endpoints instead of changing the preferred
// bend or a search bound: the shoulder search can switch candidates at either
// threshold. Re-solving on the interpolated fixed-length circle keeps the wrist
// exact and avoids shortening the arm by linearly mixing elbow positions.
inline two::Status solve_body_pole_blend(Input const& input,double weight,Solution& output) noexcept {
    using namespace two::detail;
    if(!std::isfinite(weight)||weight<0||weight>1||!std::isfinite(input.arm.preferred_wrist_bend)||
       input.arm.preferred_wrist_bend<0||input.arm.preferred_wrist_bend>detail::pi)return two::Status::invalid_input;
    if(weight==0)return solve(input,output);
    auto body=input;body.arm.preferred_wrist_bend=detail::pi;
    if(input.attached_body_pole.known)body.arm.authored=input.attached_body_pole;
    Solution from{},to{};
    auto status=two::Status::solved;
    if(weight<1){status=solve(input,from);if(status!=two::Status::solved)return status;}
    status=solve(body,to);if(status!=two::Status::solved)return status;
    // Correct the BODY endpoint before interpolation. Re-applying the wrist
    // preference after interpolating poles can cross the signed pole-turn
    // branch and jump the elbow even though both endpoints are valid.
    const auto endpoint_delta=gears_vr::multiply(to.clavicle.orientation,
        gears_vr::inverse(input.clavicle.orientation));
    const auto endpoint_move=[&](Pose p){return Pose{
        add(input.clavicle.position,rotated(endpoint_delta,sub(p.position,input.clavicle.position))),
        gears_vr::multiply(endpoint_delta,p.orientation)};};
    auto endpoint=input.arm;
    endpoint.shoulder=endpoint_move(endpoint.shoulder);endpoint.elbow=endpoint_move(endpoint.elbow);
    endpoint.wrist=endpoint_move(endpoint.wrist);
    endpoint.authored={true,to.arm.elbow.position};endpoint.previous={};
    status=two::solve(endpoint,to.arm);if(status!=two::Status::solved)return status;
    Quaternion endpoint_target{};Vector endpoint_forward{};
    if(!quaternion(endpoint.target.orientation,endpoint_target)||
       !unit(rotated(endpoint_target,endpoint.wrist_forward_local),endpoint_forward))return two::Status::invalid_input;
    to.wrist_bend=detail::bend(to.arm,endpoint_forward);
    if(weight==1){output=to;return two::Status::solved;}
    Quaternion q=to.clavicle.orientation;
    if(weight<1){
        const auto a=from.clavicle.orientation;auto b=to.clavicle.orientation;
        if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0)b={-b.x,-b.y,-b.z,-b.w};
        if(!quaternion({a.x+(b.x-a.x)*weight,a.y+(b.y-a.y)*weight,
                       a.z+(b.z-a.z)*weight,a.w+(b.w-a.w)*weight},q))return two::Status::numerical_failure;
    }
    const auto delta=gears_vr::multiply(q,gears_vr::inverse(input.clavicle.orientation));
    const auto move=[&](Pose p){return Pose{add(input.clavicle.position,rotated(delta,sub(p.position,input.clavicle.position))),
        gears_vr::multiply(delta,p.orientation)};};
    auto arm=input.arm;arm.shoulder=move(arm.shoulder);arm.elbow=move(arm.elbow);arm.wrist=move(arm.wrist);
    arm.authored={true,weight==1?to.arm.elbow.position:
        add(scaled(from.arm.elbow.position,1-weight),scaled(to.arm.elbow.position,weight))};
    // Both endpoint elbows already include wrist comfort. This last solve
    // only projects their interpolated pole onto the fixed-length circle;
    // it must not perform a second pole-turn selection along that blend.
    arm.previous={};arm.preferred_wrist_bend=detail::pi;
    Solution candidate{};status=two::solve(arm,candidate.arm);if(status!=two::Status::solved)return status;
    candidate.clavicle={input.clavicle.position,q};
    Quaternion target{};Vector forward{};
    if(!quaternion(arm.target.orientation,target)||!unit(rotated(target,arm.wrist_forward_local),forward))return two::Status::invalid_input;
    candidate.wrist_bend=detail::bend(candidate.arm,forward);
    const auto base=input.clavicle.orientation;
    candidate.clavicle_turn=2*std::acos((std::clamp)(std::abs(q.x*base.x+q.y*base.y+q.z*base.z+q.w*base.w),0.,1.));
    candidate.shoulder_displacement=length(sub(candidate.arm.shoulder.position,input.arm.shoulder.position));
    candidate.adjusted=candidate.clavicle_turn>1e-7;
    output=candidate;return two::Status::solved;
}
}
