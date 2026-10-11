#pragma once
#include <array>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace gears_vr {
struct Vector {double x=0,y=0,z=0;};
struct Quaternion {double x=0,y=0,z=0,w=1;};
struct Pose {Vector position;Quaternion orientation;};
struct Rotator {double pitch=0,yaw=0,roll=0;}; // radians, UE conventions
using Matrix=std::array<std::array<double,3>,3>;
inline bool finite(Vector v) {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline Quaternion normalized(Quaternion q) {
    double length=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
    if(!std::isfinite(length)||length<1e-8)throw std::invalid_argument("Invalid pose quaternion");
    return {q.x/length,q.y/length,q.z/length,q.w/length};
}
inline Quaternion inverse(Quaternion q) {q=normalized(q);return {-q.x,-q.y,-q.z,q.w};}
inline Quaternion multiply(Quaternion a,Quaternion b) {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
            a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
            a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
inline Matrix rotation_matrix(Quaternion q) {
    q=normalized(q);
    return {{{1-2*(q.y*q.y+q.z*q.z),2*(q.x*q.y-q.z*q.w),2*(q.x*q.z+q.y*q.w)},
             {2*(q.x*q.y+q.z*q.w),1-2*(q.x*q.x+q.z*q.z),2*(q.y*q.z-q.x*q.w)},
             {2*(q.x*q.z-q.y*q.w),2*(q.y*q.z+q.x*q.w),1-2*(q.x*q.x+q.y*q.y)}}};
}
inline Vector transform(Matrix const& m,Vector v) {
    return {m[0][0]*v.x+m[0][1]*v.y+m[0][2]*v.z,
            m[1][0]*v.x+m[1][1]*v.y+m[1][2]*v.z,
            m[2][0]*v.x+m[2][1]*v.y+m[2][2]*v.z};
}
inline Matrix multiply(Matrix const& a,Matrix const& b) {
    Matrix result{};
    for(int row=0;row<3;++row)for(int col=0;col<3;++col)
        for(int k=0;k<3;++k)result[row][col]+=a[row][k]*b[k][col];
    return result;
}
inline Matrix transpose(Matrix const& m) {
    Matrix result{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)result[i][j]=m[j][i];return result;
}
// OpenXR: +X right, +Y up, -Z forward. UE: +X forward, +Y right, +Z up.
// Handedness changes; convert matrices by B R B^-1 instead of shuffling angles.
inline Vector xr_to_ue(Vector v) {return {-v.z,v.x,v.y};}
inline Matrix xr_to_ue(Matrix const& r) {
    const Matrix basis={{{0,0,-1},{1,0,0},{0,1,0}}};
    return multiply(multiply(basis,r),transpose(basis));
}
inline Matrix ue_matrix(Rotator r) {
    double cp=std::cos(r.pitch),sp=std::sin(r.pitch),cy=std::cos(r.yaw),sy=std::sin(r.yaw),cr=std::cos(r.roll),sr=std::sin(r.roll);
    // Columns are local forward/right/up directions in world coordinates.
    return {{{cp*cy,sr*sp*cy-cr*sy,-cr*sp*cy-sr*sy},
             {cp*sy,sr*sp*sy+cr*cy,sr*cy-cr*sp*sy},
             {sp,-sr*cp,cr*cp}}};
}
inline Rotator ue_angles(Matrix const& r) {
    double horizontal=std::hypot(r[0][0],r[1][0]);
    if(horizontal<1e-8)return {std::atan2(r[2][0],horizontal),std::atan2(-r[0][1],r[1][1]),0};
    return {std::atan2(r[2][0],horizontal),std::atan2(r[1][0],r[0][0]),std::atan2(-r[2][1],r[2][2])};
}
inline int to_rotator_units(double radians) {
    if(!std::isfinite(radians))throw std::invalid_argument("Invalid angle");
    return static_cast<int>(std::lround(std::remainder(radians,6.283185307179586)*65536/6.283185307179586));
}
struct Viewpoint {Vector position;Rotator rotation;};
inline Viewpoint relative_view(Viewpoint clean,Pose neutral,Pose current,double units_per_metre=100) {
    if(!finite(clean.position)||!finite(neutral.position)||!finite(current.position)||
       !std::isfinite(clean.rotation.pitch)||!std::isfinite(clean.rotation.yaw)||!std::isfinite(clean.rotation.roll)||
       !std::isfinite(units_per_metre)||units_per_metre<=0)
        throw std::invalid_argument("Invalid pose or unit scale");
    auto q_inverse=inverse(neutral.orientation);
    auto relative=normalized(multiply(q_inverse,normalized(current.orientation)));
    Matrix base=ue_matrix(clean.rotation);
    Vector metres{current.position.x-neutral.position.x,current.position.y-neutral.position.y,current.position.z-neutral.position.z};
    auto local_delta=xr_to_ue(transform(rotation_matrix(q_inverse),metres));
    auto world_delta=transform(base,{local_delta.x*units_per_metre,local_delta.y*units_per_metre,local_delta.z*units_per_metre});
    return {{clean.position.x+world_delta.x,clean.position.y+world_delta.y,clean.position.z+world_delta.z},
            ue_angles(multiply(base,xr_to_ue(rotation_matrix(relative))))};
}
} // namespace gears_vr
