#pragma once
#include "cover_physx_matrix.h"
#include <array>
#include <cmath>
#include <cstring>
// Owned PhysX-unit matrices. Original getters do not normalize quaternions;
// validate rather than changing them. No memory access or movement authority.
namespace gears_cover_physx_pose_math {
using Matrix=gears_cover_physx_transform::Matrix34;
inline bool scalar(float x)noexcept{return std::isfinite(x)&&(x==0||std::isnormal(x));}
inline bool rigid(Matrix const& m)noexcept{
    for(float x:m.values)if(!scalar(x))return false;
    constexpr double tolerance=3e-5;
    for(unsigned r=0;r<3;++r)for(unsigned c=r;c<3;++c){double value{};
      for(unsigned k=0;k<3;++k)value+=double(m.values[3*r+k])*m.values[3*c+k];
      if(std::abs(value-(r==c?1.:0.))>tolerance)return false;}
    const auto* v=m.values;const double determinant=double(v[0])*(double(v[4])*v[8]-double(v[5])*v[7])-
      double(v[1])*(double(v[3])*v[8]-double(v[5])*v[6])+double(v[2])*(double(v[3])*v[7]-double(v[4])*v[6]);
    return std::abs(determinant-1.)<=tolerance;
}
inline bool quaternion(std::array<float,4> const& q,std::array<float,3> const& p,Matrix& output)noexcept{
    for(float x:q)if(!scalar(x))return false;for(float x:p)if(!scalar(x))return false;
    double norm{};for(float x:q)norm+=double(x)*x;if(std::abs(norm-1.)>3e-5)return false;
    Matrix next{};auto* v=next.values;
    const float w=q[3]+q[3],x=q[0]+q[0],y=q[1]+q[1],z=q[2]+q[2];
    const float yy=y*q[1],zz=z*q[2],remaining=1.f-x*q[0];
    v[0]=(1.f-yy)-zz;v[1]=x*q[1]-w*q[2];v[2]=x*q[2]+w*q[1];
    v[3]=x*q[1]+w*q[2];v[4]=remaining-zz;v[5]=y*q[2]-w*q[0];
    v[6]=x*q[2]-w*q[1];v[7]=y*q[2]+w*q[0];v[8]=remaining-yy;
    for(unsigned n=0;n<3;++n)v[9+n]=p[n];if(!rigid(next))return false;output=next;return true;
}
inline bool compose(Matrix const& actor_pose,Matrix const& shape,Matrix& output)noexcept{
    if(!rigid(actor_pose)||!rigid(shape))return false;Matrix next{};
    for(unsigned r=0;r<3;++r){
      for(unsigned c=0;c<3;++c){const float a=shape.values[3+c]*actor_pose.values[3*r+1];
        const float b=actor_pose.values[3*r]*shape.values[c];const float sum=a+b;
        next.values[3*r+c]=sum+shape.values[6+c]*actor_pose.values[3*r+2];}
      const float a=shape.values[11]*actor_pose.values[3*r+2],b=shape.values[9]*actor_pose.values[3*r];
      const float sum=a+b,with_y=sum+shape.values[10]*actor_pose.values[3*r+1];
      next.values[9+r]=with_y+actor_pose.values[9+r];
    }
    if(!rigid(next))return false;output=next;return true;
}
}
