#pragma once
#include "../vr-common/pose_math.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

// Offline/private-buffer mathematics only: no engine calls, native field
// access, allocation or write permission. Pointers must name caller-owned live
// arrays. Matrix is a private double-precision row-vector matrix, NOT native
// Matrix64 storage. Space = Local * ParentSpace; World = Space * component.
namespace gears_cover_skeleton_ik {
constexpr std::size_t max_bones=512;
struct Atom {float q[4],translation[3],scale;}; // xyzw, translation, uniform scale
using Vec=gears_vr::Vector;
using Quat=gears_vr::Quaternion;
using Pose=gears_vr::Pose;
struct Matrix {double m[16]{};};
struct Chain {std::size_t upper{},lower{},end{};};
enum class LengthPolicy {fixed,adapt_translations};
struct AdaptResult {double max_position_error{};bool translations_allowed{};};
enum class Status {okay,count,alias,atom,parent,component,nonfinite,chain,target,inconsistent_pose};
// Caller supplies this large scratch object; do not place it on a game stack.
// Scratch may change on failure. Every public output remains byte-identical
// on failure. All input/output/scratch ranges must be disjoint.
struct Workspace {Atom local[max_bones];Matrix space[max_bones],world[max_bones];};
static_assert(sizeof(Atom)==32&&std::is_trivially_copyable<Atom>::value);
namespace detail {
inline bool finite_vector(Vec a) noexcept {return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline Vec sub(Vec a,Vec b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec mul(Vec a,double k) noexcept {return {a.x*k,a.y*k,a.z*k};}
inline double dot(Vec a,Vec b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec cross(Vec a,Vec b) noexcept {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(Vec a) noexcept {return std::hypot(a.x,a.y,a.z);}
inline bool normalized(Quat q,Quat& out) noexcept {
    const double n=std::hypot(std::hypot(q.x,q.y),std::hypot(q.z,q.w));
    if(!std::isfinite(n)||std::abs(n-1)>1e-3)return false;
    out={q.x/n,q.y/n,q.z/n,q.w/n};return true;
}
inline Quat product(Quat a,Quat b) noexcept {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
inline Matrix atom_matrix(Atom const& a) noexcept {
    const double x=a.q[0],y=a.q[1],z=a.q[2],w=a.q[3],s=a.scale;
    return {{s*(1-2*y*y-2*z*z),s*(2*x*y+2*w*z),s*(2*x*z-2*w*y),0,
        s*(2*x*y-2*w*z),s*(1-2*x*x-2*z*z),s*(2*y*z+2*w*x),0,
        s*(2*x*z+2*w*y),s*(2*y*z-2*w*x),s*(1-2*x*x-2*y*y),0,
        a.translation[0],a.translation[1],a.translation[2],1}};
}
inline Matrix multiply(Matrix const& a,Matrix const& b) noexcept {
    Matrix r{};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)
        for(unsigned k=0;k<4;++k)r.m[4*i+j]+=a.m[4*i+k]*b.m[4*k+j];return r;
}
inline bool finite(Matrix const& a) noexcept {for(double x:a.m)if(!std::isfinite(x))return false;return true;}
inline Vec row(Matrix const& m,unsigned r) noexcept {return {m.m[4*r],m.m[4*r+1],m.m[4*r+2]};}
inline Vec position(Matrix const& m) noexcept {return row(m,3);}
inline bool similarity(Matrix const& m) noexcept {
    if(!finite(m)||m.m[3]!=0||m.m[7]!=0||m.m[11]!=0||m.m[15]!=1)return false;
    const Vec a=row(m,0),b=row(m,1),c=row(m,2);const double s=length(a);
    if(!std::isfinite(s)||s<1e-12||std::abs(length(b)/s-1)>1e-4||std::abs(length(c)/s-1)>1e-4)return false;
    const Vec u=mul(a,1/s),v=mul(b,1/s),w=mul(c,1/s);
    return std::abs(dot(u,v))<1e-4&&std::abs(dot(u,w))<1e-4&&std::abs(dot(v,w))<1e-4&&dot(cross(u,v),w)>.9999;
}
inline Vec inverse_direction(Matrix const& parent,Vec v) noexcept {
    const double s=length(row(parent,0));return {dot(v,row(parent,0))/s,dot(v,row(parent,1))/s,dot(v,row(parent,2))/s};
}
inline Quat rotation(Matrix const& m) noexcept {
    const double s=length(row(m,0)),a=m.m[0]/s,b=m.m[5]/s,c=m.m[10]/s;Quat q{};
    // Standard column extraction on the transpose of our row matrix.
    if(a+b+c>0){const double t=2*std::sqrt(1+a+b+c);q={(m.m[6]-m.m[9])/s/t,(m.m[8]-m.m[2])/s/t,(m.m[1]-m.m[4])/s/t,t/4};}
    else if(a>b&&a>c){const double t=2*std::sqrt(1+a-b-c);q={t/4,(m.m[4]+m.m[1])/s/t,(m.m[8]+m.m[2])/s/t,(m.m[6]-m.m[9])/s/t};}
    else if(b>c){const double t=2*std::sqrt(1+b-a-c);q={(m.m[4]+m.m[1])/s/t,t/4,(m.m[9]+m.m[6])/s/t,(m.m[8]-m.m[2])/s/t};}
    else {const double t=2*std::sqrt(1+c-a-b);q={(m.m[8]+m.m[2])/s/t,(m.m[9]+m.m[6])/s/t,t/4,(m.m[1]-m.m[4])/s/t};}
    Quat n{};normalized(q,n);return n;
}
struct Range {std::uintptr_t at;std::size_t size;};
inline bool disjoint(Range const* ranges,std::size_t n) noexcept {
    constexpr auto limit=(std::numeric_limits<std::uintptr_t>::max)();
    for(std::size_t i=0;i<n;++i){const auto a=ranges[i];if(!a.at||!a.size||a.size-1>limit-a.at)return false;
        for(std::size_t j=0;j<i;++j){const auto b=ranges[j];if(a.at<=b.at+b.size-1&&b.at<=a.at+a.size-1)return false;}}
    return true;
}
inline Range range(void const* p,std::size_t n) noexcept {return {reinterpret_cast<std::uintptr_t>(p),n};}
inline Status validate(Atom const* local,std::int32_t const* parents,std::size_t n,Matrix const& component) noexcept {
    if(!similarity(component))return Status::component;
    for(std::size_t i=0;i<n;++i){
        if((!i&&parents[i]!=0)||(i&&(parents[i]<0||static_cast<std::size_t>(parents[i])>=i)))return Status::parent;
        const auto& a=local[i];Quat q{};
        if(!normalized({a.q[0],a.q[1],a.q[2],a.q[3]},q)||!finite_vector(Vec{a.translation[0],a.translation[1],a.translation[2]})||
            !std::isfinite(a.scale)||a.scale<=0)return Status::atom;
    }return Status::okay;
}
inline bool build(Workspace& w,std::int32_t const* parents,std::size_t n,Matrix const& component) noexcept {
    for(std::size_t i=0;i<n;++i){const auto m=atom_matrix(w.local[i]);w.space[i]=i?multiply(m,w.space[parents[i]]):m;
        w.world[i]=multiply(w.space[i],component);if(!finite(w.space[i])||!finite(w.world[i]))return false;}return true;
}
inline bool set_quat(Atom& a,Quat q) noexcept {
    Quat n{};if(!normalized(q,n))return false;
    a.q[0]=static_cast<float>(n.x);a.q[1]=static_cast<float>(n.y);a.q[2]=static_cast<float>(n.z);a.q[3]=static_cast<float>(n.w);return true;
}
} // namespace detail

// Extract a same-space rigid pose, discarding only the positive uniform scale.
// Hamilton orientation matches the shared solver: qWorld=qParent*qLocal.
inline Status world_pose(Matrix const& matrix,Pose& out) noexcept {
    const detail::Range ranges[]={detail::range(&matrix,sizeof(matrix)),detail::range(&out,sizeof(out))};
    if(!detail::disjoint(ranges,2))return Status::alias;
    if(!detail::similarity(matrix))return Status::component;
    const Pose candidate{detail::position(matrix),detail::rotation(matrix)};out=candidate;return Status::okay;
}

inline Status forward(Atom const* local,std::int32_t const* parents,std::size_t n,Matrix const& component,
    Workspace& work,Matrix* space,Matrix* world) noexcept {
    if(!n||n>max_bones)return Status::count;
    const detail::Range ranges[]={detail::range(local,n*sizeof(Atom)),detail::range(parents,n*sizeof(*parents)),
        detail::range(&component,sizeof(component)),detail::range(&work,sizeof(work)),detail::range(space,n*sizeof(Matrix)),detail::range(world,n*sizeof(Matrix))};
    if(!detail::disjoint(ranges,6))return Status::alias;
    const auto valid=detail::validate(local,parents,n,component);if(valid!=Status::okay)return valid;
    std::memcpy(work.local,local,n*sizeof(Atom));if(!detail::build(work,parents,n,component))return Status::nonfinite;
    std::memcpy(space,work.space,n*sizeof(Matrix));std::memcpy(world,work.world,n*sizeof(Matrix));return Status::okay;
}

// World-pose adapter for an independently owned solver. Upper translation is
// always preserved. Fixed mode preserves ALL translation bytes. Adaptive mode
// changes only lower/end translation, explicitly accepting solver lengths.
// No target projection/clamping occurs here; inconsistent poses are rejected.
inline Status world_poses_to_local(Atom const* local,std::int32_t const* parents,std::size_t n,Matrix const& component,
    Chain const& chain,Pose const* poses,LengthPolicy policy,Workspace& work,Atom* solved,Matrix* space,Matrix* world,
    AdaptResult& result,double tolerance=1e-4) noexcept {
    using namespace detail;
    if(!n||n>max_bones)return Status::count;
    const Range ranges[]={range(local,n*sizeof(Atom)),range(parents,n*sizeof(*parents)),range(&component,sizeof(component)),
        range(&chain,sizeof(chain)),range(poses,3*sizeof(Pose)),range(&work,sizeof(work)),range(solved,n*sizeof(Atom)),
        range(space,n*sizeof(Matrix)),range(world,n*sizeof(Matrix)),range(&result,sizeof(result))};
    if(!disjoint(ranges,10))return Status::alias;
    const auto valid=validate(local,parents,n,component);if(valid!=Status::okay)return valid;
    const std::size_t indices[]={chain.upper,chain.lower,chain.end};
    if(!indices[0]||indices[0]>=n||indices[1]>=n||indices[2]>=n||parents[indices[1]]!=static_cast<std::int32_t>(indices[0])||
        parents[indices[2]]!=static_cast<std::int32_t>(indices[1]))return Status::chain;
    if((policy!=LengthPolicy::fixed&&policy!=LengthPolicy::adapt_translations)||!std::isfinite(tolerance)||tolerance<=0||tolerance>.1)return Status::target;
    for(unsigned j=0;j<3;++j){Quat q{};if(!finite_vector(poses[j].position)||!normalized(poses[j].orientation,q))return Status::target;}
    std::memcpy(work.local,local,n*sizeof(Atom));if(!build(work,parents,n,component))return Status::nonfinite;
    for(unsigned j=0;j<3;++j){const auto i=indices[j];const auto& p=work.world[parents[i]];
        if(!similarity(p))return Status::component;
        if(j&&policy==LengthPolicy::adapt_translations){const auto v=inverse_direction(p,sub(poses[j].position,position(p)));
            const double s=length(row(p,0));auto& a=work.local[i];
            a.translation[0]=static_cast<float>(v.x/s);a.translation[1]=static_cast<float>(v.y/s);a.translation[2]=static_cast<float>(v.z/s);
            if(!finite_vector(Vec{a.translation[0],a.translation[1],a.translation[2]}))return Status::nonfinite;}
        const Quat q=rotation(p);if(!set_quat(work.local[i],product({-q.x,-q.y,-q.z,q.w},poses[j].orientation))||
            !build(work,parents,n,component))return Status::nonfinite;
        if(length(sub(position(work.world[i]),poses[j].position))>tolerance)return Status::inconsistent_pose;
    }
    AdaptResult candidate{};candidate.translations_allowed=policy==LengthPolicy::adapt_translations;
    for(unsigned j=0;j<3;++j)candidate.max_position_error=(std::max)(candidate.max_position_error,length(sub(position(work.world[indices[j]]),poses[j].position)));
    std::memcpy(solved,work.local,n*sizeof(Atom));std::memcpy(space,work.space,n*sizeof(Matrix));
    std::memcpy(world,work.world,n*sizeof(Matrix));result=candidate;return Status::okay;
}

} // namespace gears_cover_skeleton_ik
