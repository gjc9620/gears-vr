#pragma once
#include "pose_math.h"

// Authored VR zero for the original COGAssaultRifle's existing sight rings.
// Offline geometry: df356ff3dda45e060f87b5ed94c806d96bbfe36b74b21a0f1d2032596f9dcf4d.
// Rear inner-arc fit has <=0.03 raw-unit radial approximation error. This is
// not an original factory-zero claim or permission to use another live asset.
// The caller still certifies its current Lancer/frame/mount/socket and writer.
namespace gears_weapon_lancer_sight {
inline constexpr char profile_id[]="lancer-original-rings-vr-zero-25m-v1";
inline constexpr double zero_metres=25.;
inline constexpr gears_vr::Vector rear_raw{0.,24.2729,18.565985797372093};
inline constexpr gears_vr::Vector front_raw{0.,60.057634353637695,17.508113861083984};
struct Result {gears_vr::Vector rear{},front{},zero{},direction{};double correction_radians{},units{},model_scale{};};
inline double length(gears_vr::Vector p) noexcept {return std::hypot(std::hypot(p.x,p.y),p.z);}
inline gears_vr::Vector minus(gears_vr::Vector a,gears_vr::Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline bool solve(gears_vr::Matrix const& basis,gears_vr::Vector mesh_origin,double model_scale,
    double units,gears_vr::Vector actual_muzzle,gears_vr::Vector original_forward,
    Result& output,double range_metres=zero_metres) noexcept {
    using namespace gears_vr;
    if(!finite(mesh_origin)||!finite(actual_muzzle)||!finite(original_forward)||
       !std::isfinite(model_scale)||model_scale<.1||model_scale>10||
       !std::isfinite(units)||units<10||units>500||
       !std::isfinite(range_metres)||range_metres<2||range_metres>100)return false;
    for(int i=0;i<3;++i)for(int j=i;j<3;++j){double product{};
        for(int k=0;k<3;++k){if(!std::isfinite(basis[k][i])||!std::isfinite(basis[k][j]))return false;
            product+=basis[k][i]*basis[k][j];}
        if(std::abs(product-(i==j?1.:0.))>.001)return false;
    }
    const Vector cross{basis[1][0]*basis[2][1]-basis[2][0]*basis[1][1],
        basis[2][0]*basis[0][1]-basis[0][0]*basis[2][1],basis[0][0]*basis[1][1]-basis[1][0]*basis[0][1]};
    if(cross.x*basis[0][2]+cross.y*basis[1][2]+cross.z*basis[2][2]<.999)return false;
    auto local=transform(transpose(basis),minus(actual_muzzle,mesh_origin));
    local={local.x/model_scale,local.y/model_scale,local.z/model_scale};
    // Sanity-check the actual current socket against this original model;
    // these bounds never substitute a hard-coded muzzle location.
    if(std::abs(local.x)>2||local.y<65||local.y>85||local.z<5||local.z>16)return false;
    const double forward_length=length(original_forward);if(forward_length<.99||forward_length>1.01)return false;
    const auto point=[&](Vector p){auto q=transform(basis,{p.x*model_scale,p.y*model_scale,p.z*model_scale});
        return Vector{mesh_origin.x+q.x,mesh_origin.y+q.y,mesh_origin.z+q.z};};
    Result result{};result.units=units;result.model_scale=model_scale;
    result.rear=point(rear_raw);result.front=point(front_raw);
    const auto line=minus(result.front,result.rear);const double sight_length=length(line);
    if(!std::isfinite(sight_length)||sight_length<.01)return false;
    const double distance=range_metres*units;
    result.zero={result.rear.x+line.x/sight_length*distance,
        result.rear.y+line.y/sight_length*distance,result.rear.z+line.z/sight_length*distance};
    const auto aimed=minus(result.zero,actual_muzzle);const double aimed_length=length(aimed);
    if(!std::isfinite(aimed_length)||aimed_length<units)return false;
    result.direction={aimed.x/aimed_length,aimed.y/aimed_length,aimed.z/aimed_length};
    const double dot=(result.direction.x*original_forward.x+result.direction.y*original_forward.y+
        result.direction.z*original_forward.z)/forward_length;
    result.correction_radians=std::acos(std::clamp(dot,-1.,1.));
    if(!finite(result.zero)||!finite(result.direction)||result.correction_radians>.105)return false;
    output=result;return true;
}
}
