#pragma once
#include "cover_physx_pose_math.h"
#include <float.h>
#include <algorithm>
#include <limits>
// Original getter arithmetic, WITHOUT normalization. This is an observed
// near-rotation with an explicit Gram error, never a strict rigid query pose.
namespace gears_cover_physx_observed_pose {
namespace m=gears_cover_physx_pose_math;
struct Pose {m::Matrix matrix{};double gram_error{};};
constexpr double norm_tolerance=3e-5;
inline bool nearest()noexcept{unsigned c{};return !_controlfp_s(&c,0,0)&&(c&_MCW_RC)==_RC_NEAR;}
inline double determinant(m::Matrix const& a)noexcept{const auto* v=a.values;return double(v[0])*(double(v[4])*v[8]-double(v[5])*v[7])-double(v[1])*(double(v[3])*v[8]-double(v[5])*v[6])+double(v[2])*(double(v[3])*v[7]-double(v[4])*v[6]);}
// Frobenius upper bound on R^T R-I, hence also the spectral norm. Float
// products are exact in double; allow double reduction/sqrt error explicitly.
inline double gram_bound(m::Matrix const& a)noexcept{
    double squares{},maximum{};for(unsigned n=0;n<9;++n)maximum=std::max(maximum,std::abs(double(a.values[n])));
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){double x{};
      for(unsigned k=0;k<3;++k)x+=double(a.values[3*k+i])*a.values[3*k+j];x-=i==j?1.:0.;squares+=x*x;}
    return std::nextafter(std::sqrt(squares)+64*std::numeric_limits<double>::epsilon()*(1+maximum*maximum),std::numeric_limits<double>::infinity());
}
// For exact quaternion formula, Gram eigenvalues are 1 and
// 1+4*(|q|^2-1)*|q.xyz|^2. Each original float matrix entry has fewer
// than 8 eps absolute rounding error; 64 eps bounds its operator error.
inline double quaternion_limit()noexcept{
    const double e=64*std::numeric_limits<float>::epsilon(),g=4*norm_tolerance*(1+norm_tolerance);
    return std::sqrt(3.)*(g+2*std::sqrt(1+g)*e+e*e)+128*std::numeric_limits<double>::epsilon();
}
inline bool finite(m::Matrix const& a)noexcept{for(float x:a.values)if(!m::scalar(x))return false;return true;}
inline bool qualified(Pose const& a)noexcept{return finite(a.matrix)&&std::isfinite(a.gram_error)&&a.gram_error>=gram_bound(a.matrix)&&a.gram_error<=quaternion_limit()&&determinant(a.matrix)>0;}
inline bool quaternion(std::array<float,4> const& q,std::array<float,3> const& p,Pose& output)noexcept{
    if(!nearest())return false;for(float x:q)if(!m::scalar(x))return false;for(float x:p)if(!m::scalar(x))return false;
    double norm{};for(float x:q)norm+=double(x)*x;if(std::abs(norm-1.)>norm_tolerance)return false;
    Pose next{};auto* v=next.matrix.values;
    const float w=q[3]+q[3],x=q[0]+q[0],y=q[1]+q[1],z=q[2]+q[2];
    const float yy=y*q[1],zz=z*q[2],remaining=1.f-x*q[0];
    v[0]=(1.f-yy)-zz;v[1]=x*q[1]-w*q[2];v[2]=x*q[2]+w*q[1];
    v[3]=x*q[1]+w*q[2];v[4]=remaining-zz;v[5]=y*q[2]-w*q[0];
    v[6]=x*q[2]-w*q[1];v[7]=y*q[2]+w*q[0];v[8]=remaining-yy;
    for(unsigned n=0;n<3;++n)v[9+n]=p[n];next.gram_error=gram_bound(next.matrix);
    if(!qualified(next))return false;output=next;return true;
}
inline bool compose(Pose const& actor_pose,Pose const& shape,Pose& output)noexcept{
    if(!nearest()||!qualified(actor_pose)||!qualified(shape))return false;Pose next{};
    const auto& a=actor_pose.matrix;const auto& s=shape.matrix;auto* v=next.matrix.values;
    for(unsigned r=0;r<3;++r){
      for(unsigned c=0;c<3;++c){const float x=s.values[3+c]*a.values[3*r+1],y=a.values[3*r]*s.values[c];const float sum=x+y;v[3*r+c]=sum+s.values[6+c]*a.values[3*r+2];}
      const float x=s.values[11]*a.values[3*r+2],y=s.values[9]*a.values[3*r];const float sum=x+y,with_y=sum+s.values[10]*a.values[3*r+1];v[9+r]=with_y+a.values[9+r];
    }
    next.gram_error=gram_bound(next.matrix);
    const double e=64*std::numeric_limits<float>::epsilon(),product=(1+actor_pose.gram_error)*(1+shape.gram_error);
    const double limit=std::sqrt(3.)*(product-1+2*std::sqrt(product)*e+e*e)+128*std::numeric_limits<double>::epsilon();
    if(!finite(next.matrix)||determinant(next.matrix)<=0||next.gram_error>limit)return false;output=next;return true;
}
}
