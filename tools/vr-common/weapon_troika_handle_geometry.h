#pragma once
#include "pose_math.h"
#include <array>
#include <limits>
namespace gears_weapon::troika_handle_geometry {
using V=gears_vr::Vector;
// Authored sparse shaft envelope from authenticated TroikaTurret LOD0,
// export 7baac05a...adc037. Raw source units, not a physical unit assumption.
// The current handleroot bone9 follows the actual original gun parent chain.
// Its reference origin is (0,47.27169418,111.43261719), rotation identity.
// No handle_L/R bone origin is used as a surface marker.
struct Section {double z,x_low,x_high,y_low,y_high;};
inline constexpr std::array<Section,6> shaft{{
    {99,10.11,15.55373,25.95624,33.75870},
    {102,10.50975,15.96452,24.65951,33.97700},
    {106,10.77597,16.14344,23.44412,32.51886},
    {110,10.78891,16.13383,23.51315,32.07880},
    {114,10.67282,15.99663,24.62190,32.72869},
    {118,10.65734,15.36797,27.21556,33.87503}}};
struct Contact {bool touched{};double distance_metres{};V local_surface{};};
inline double dot(V a,V b) noexcept{return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V sub(V a,V b) noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
// GetBoneMatrix is row-vector UE space. Reject shear, reflection, nonuniform
// scale and nonfinite matrices; never silently normalize bad source geometry.
inline bool local_point(std::array<float,16> const& m,V world,V& local,double& scale) noexcept {
    for(float x:m)if(!std::isfinite(x))return false;
    if(!gears_vr::finite(world)||std::abs(m[3])+std::abs(m[7])+std::abs(m[11])>1e-5||std::abs(m[15]-1)>1e-5)return false;
    const V x{m[0],m[1],m[2]},y{m[4],m[5],m[6]},z{m[8],m[9],m[10]};
    const double xx=dot(x,x),yy=dot(y,y),zz=dot(z,z);
    if(xx<1e-8||xx>1e6||std::abs(xx-yy)>xx*1e-4||std::abs(xx-zz)>xx*1e-4||
       std::abs(dot(x,y))+std::abs(dot(x,z))+std::abs(dot(y,z))>xx*1e-4)return false;
    const V cross{x.y*y.z-x.z*y.y,x.z*y.x-x.x*y.z,x.x*y.y-x.y*y.x};
    if(dot(cross,z)<=0)return false;
    const auto d=sub(world,{m[12],m[13],m[14]});
    local={dot(d,x)/xx,dot(d,y)/xx,dot(d,z)/xx};scale=std::sqrt(xx);return gears_vr::finite(local);
}
inline bool contact(std::array<float,16> const& handleroot,V raw_world_palm,unsigned handle,
    double units_per_metre,Contact& out) noexcept {
    if(handle>1||!std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500)return false;
    V local{};double scale{};if(!local_point(handleroot,raw_world_palm,local,scale))return false;
    // Mesh reference coordinates; bilateral geometry is measured separately
    // from the differently rotated/asymmetric handle reference bones.
    const V model{handle? -local.x:local.x,local.y+47.27169418334961,local.z+111.4326171875};
    Contact value{};value.distance_metres=std::numeric_limits<double>::infinity();
    if(model.z<shaft.front().z||model.z>shaft.back().z){out=value;return true;}
    unsigned i=0;while(i+2<shaft.size()&&model.z>shaft[i+1].z)++i;
    const auto& a=shaft[i];const auto& b=shaft[i+1];const double t=(model.z-a.z)/(b.z-a.z);
    const auto lerp=[&](double lo,double hi){return lo+(hi-lo)*t;};
    const double xl=lerp(a.x_low,b.x_low),xh=lerp(a.x_high,b.x_high),yl=lerp(a.y_low,b.y_low),yh=lerp(a.y_high,b.y_high);
    const double cx=(xl+xh)*.5,cy=(yl+yh)*.5,rx=(xh-xl)*.5,ry=(yh-yl)*.5;
    // LOD0 shaft corners are broad/chamfered, not an ellipse. Use the
    // measured rounded rectangular envelope; an ellipse misses real corners.
    constexpr double corner=.6;
    const double dx=model.x-cx,dy=model.y-cy;
    const double qx=std::abs(dx)-(rx-corner),qy=std::abs(dy)-(ry-corner);
    const double ox=(std::max)(qx,0.),oy=(std::max)(qy,0.);
    const double sdf=std::hypot(ox,oy)+(std::min)((std::max)(qx,qy),0.)-corner;
    value.distance_metres=std::abs(sdf)*scale/units_per_metre;
    value.touched=value.distance_metres<=.012; // surface band, no long-range pull
    double sx=(std::max)(-(rx-corner),(std::min)(dx,rx-corner));
    double sy=(std::max)(-(ry-corner),(std::min)(dy,ry-corner));
    double nx=dx-sx,ny=dy-sy;const double len=std::hypot(nx,ny);
    if(len>1e-9){nx/=len;ny/=len;}
    else if(qx>qy){nx=dx<0?-1.:1.;ny=0.;}
    else{nx=0.;ny=dy<0?-1.:1.;}
    value.local_surface={cx+sx+nx*corner,cy+sy+ny*corner-47.27169418334961,local.z};
    if(handle)value.local_surface.x=-value.local_surface.x;
    out=value;return true;
}
} // namespace gears_weapon::troika_handle_geometry
