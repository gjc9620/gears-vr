#pragma once
#include "pose_math.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace gears_weapon {

// Provisional location next to the original Lancer's chainsaw housing. This
// must be aligned to the visible VR starter and calibrated in a headset before
// it can become a playable grip. The private synthetic trial uses it only to
// verify the gesture-to-original-input boundary.
inline constexpr gears_vr::Vector lancer_starter_local_raw{6,26,10.65}; // fixed socket origin
// The authored StarterPullGrip crossbar is at X=.045m; .010/.022m
// are the cord's fixed/moving ends. Use the same geometry for visible parts
// and grasp contact instead of asking the player to grip the socket itself.
inline constexpr double lancer_starter_grip_raw=4.5;
inline constexpr double lancer_starter_cord_start_raw=1.;
inline constexpr double lancer_starter_cord_end_raw=2.2;
inline constexpr gears_vr::Vector lancer_starter_axis_local{1,0,0};
struct LancerStarterWorld {
    gears_vr::Vector anchor_raw{},pull_axis{},socket_raw{};
};

inline bool lancer_starter_world(std::array<float,16> const& gun,
                                LancerStarterWorld& out) noexcept {
    for(float value:gun)if(!std::isfinite(value))return false;
    if(std::abs(gun[3])>1e-4||std::abs(gun[7])>1e-4||
       std::abs(gun[11])>1e-4||std::abs(gun[15]-1)>1e-4)return false;
    double scale[3]{};
    for(int i=0;i<3;++i){
        scale[i]=std::hypot(std::hypot(double(gun[i*4]),double(gun[i*4+1])),
                             double(gun[i*4+2]));
        if(scale[i]<.1||scale[i]>10)return false;
    }
    for(int i=0;i<3;++i)for(int j=i+1;j<3;++j){
        double cosine=0;
        for(int k=0;k<3;++k)cosine+=double(gun[i*4+k])*gun[j*4+k];
        if(std::abs(cosine/(scale[i]*scale[j]))>1e-3||
           std::abs(scale[i]-scale[j])>(std::max)(scale[i],scale[j])*.02)
            return false;
    }
    const double det=double(gun[0])*(double(gun[5])*gun[10]-double(gun[6])*gun[9])-
        double(gun[1])*(double(gun[4])*gun[10]-double(gun[6])*gun[8])+
        double(gun[2])*(double(gun[4])*gun[9]-double(gun[5])*gun[8]);
    if(det<=0||!std::isfinite(det))return false;
    LancerStarterWorld next{};
    next.socket_raw={
        gun[12]+lancer_starter_local_raw.x*gun[0]+
            lancer_starter_local_raw.y*gun[4]+lancer_starter_local_raw.z*gun[8],
        gun[13]+lancer_starter_local_raw.x*gun[1]+
            lancer_starter_local_raw.y*gun[5]+lancer_starter_local_raw.z*gun[9],
        gun[14]+lancer_starter_local_raw.x*gun[2]+
            lancer_starter_local_raw.y*gun[6]+lancer_starter_local_raw.z*gun[10]};
    next.anchor_raw={next.socket_raw.x+lancer_starter_grip_raw*gun[0],
        next.socket_raw.y+lancer_starter_grip_raw*gun[1],
        next.socket_raw.z+lancer_starter_grip_raw*gun[2]};
    next.pull_axis={gun[0]/scale[0],gun[1]/scale[0],gun[2]/scale[0]};
    if(!gears_vr::finite(next.anchor_raw)||!gears_vr::finite(next.pull_axis)||
       !gears_vr::finite(next.socket_raw))return false;
    out=next;return true;
}

struct LancerStarterParts {
    std::array<float,16> socket{},grip{},cord{};
    gears_vr::Vector grasp_raw{};
};
inline bool lancer_starter_draw_pose(std::array<float,16> const& matrix,
    gears_vr::Viewpoint& pose,gears_vr::Vector& scaling) noexcept {
    for(float v:matrix)if(!std::isfinite(v))return false;
    if(std::abs(matrix[3])>1e-4||std::abs(matrix[7])>1e-4||std::abs(matrix[11])>1e-4||
       std::abs(matrix[15]-1)>1e-4)return false;
    gears_vr::Matrix basis{};double size[3]{};
    for(unsigned i=0;i<3;++i){
        size[i]=std::hypot(std::hypot(double(matrix[i*4]),double(matrix[i*4+1])),double(matrix[i*4+2]));
        if(size[i]<.01||size[i]>200)return false;
        for(unsigned j=0;j<3;++j)basis[j][i]=matrix[i*4+j]/size[i];
    }
    for(unsigned i=0;i<3;++i)for(unsigned j=i;j<3;++j){
        double dot=0;for(unsigned k=0;k<3;++k)dot+=basis[k][i]*basis[k][j];
        if(std::abs(dot-(i==j?1.:0.))>1e-4)return false;
    }
    const double det=basis[0][0]*(basis[1][1]*basis[2][2]-basis[1][2]*basis[2][1])-
        basis[0][1]*(basis[1][0]*basis[2][2]-basis[1][2]*basis[2][0])+
        basis[0][2]*(basis[1][0]*basis[2][1]-basis[1][1]*basis[2][0]);
    if(std::abs(det-1)>1e-4)return false;
    pose={{matrix[12],matrix[13],matrix[14]},gears_vr::ue_angles(basis)};
    scaling={size[0],size[1],size[2]};return true;
}
inline bool lancer_starter_rotation_basis_valid(gears_vr::Matrix const& basis) noexcept {
    for(auto const& row:basis)for(double v:row)if(!std::isfinite(v))return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=i;j<3;++j){
        double dot=0;for(unsigned k=0;k<3;++k)dot+=basis[k][i]*basis[k][j];
        if(std::abs(dot-(i==j?1.:0.))>1e-4)return false;
    }
    const double det=basis[0][0]*(basis[1][1]*basis[2][2]-basis[1][2]*basis[2][1])-
        basis[0][1]*(basis[1][0]*basis[2][2]-basis[1][2]*basis[2][0])+
        basis[0][2]*(basis[1][0]*basis[2][1]-basis[1][1]*basis[2][0]);
    return std::abs(det-1)<=1e-4;
}
inline bool lancer_starter_matches_draw(std::array<float,16> const& actual,
    gears_vr::Viewpoint const& pose,gears_vr::Vector const& scale,
    gears_vr::Matrix const* original_engine_basis=nullptr) noexcept {
    for(float v:actual)if(!std::isfinite(v))return false;
    if(!gears_vr::finite(pose.position)||!gears_vr::finite(scale)||
       !std::isfinite(pose.rotation.pitch)||!std::isfinite(pose.rotation.yaw)||!std::isfinite(pose.rotation.roll)||
       scale.x<=0||scale.y<=0||scale.z<=0||std::abs(actual[3])>1e-4||std::abs(actual[7])>1e-4||
       std::abs(actual[11])>1e-4||std::abs(actual[15]-1)>1e-4)return false;
    const double position[]={pose.position.x,pose.position.y,pose.position.z},sizes[]={scale.x,scale.y,scale.z};
    // The original Rotator stores65536units/turn. Allow its quantisation plus
    // float roundoff, not a visible angle/scale difference or a previous pose.
    // Production may supply original Core.GetAxes for the EXACT submitted
    // Rotator. Continuous analytical Euler math is not the original engine's
    // discrete trigonometric result. The ACK tolerance is unchanged.
    const auto basis=original_engine_basis?*original_engine_basis:gears_vr::ue_matrix(pose.rotation);
    if(!lancer_starter_rotation_basis_valid(basis))return false;
    for(unsigned axis=0;axis<3;++axis){
        if(std::abs(actual[12+axis]-position[axis])>.02)return false;
        for(unsigned row=0;row<3;++row)
            if(std::abs(actual[axis*4+row]-basis[row][axis]*sizes[axis])>.00011*sizes[axis]+.0001)return false;
    }
    return true;
}
// All three authored parts share one gun transform. Their geometry is in
// original engine units (the GLB's metres converted exactly once by100).
// Only cord X stretches; its gun-side end stays fixed. These are visual
// transforms, not input/hand/action permits or proof of runtime visibility.
inline bool lancer_starter_parts(std::array<float,16> const& gun,
    double pull_metres,double units_per_metre,LancerStarterParts& out) noexcept {
    LancerStarterWorld world{};
    if(!lancer_starter_world(gun,world)||!std::isfinite(pull_metres)||pull_metres<0||pull_metres>.3||
       !std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500)return false;
    const double size=std::hypot(std::hypot(double(gun[0]),double(gun[1])),double(gun[2]));
    const double travel=pull_metres*units_per_metre;
    const double stretch=1+travel/((lancer_starter_cord_end_raw-lancer_starter_cord_start_raw)*size);
    LancerStarterParts next{};next.socket=next.grip=next.cord=gun;
    for(unsigned axis=0;axis<3;++axis){
        const double origin=axis==0?world.socket_raw.x:axis==1?world.socket_raw.y:world.socket_raw.z;
        const double direction=axis==0?world.pull_axis.x:axis==1?world.pull_axis.y:world.pull_axis.z;
        next.socket[12+axis]=float(origin);
        next.grip[12+axis]=float(origin+direction*travel);
        next.cord[12+axis]=float(origin+double(gun[axis])*lancer_starter_cord_start_raw*(1-stretch));
        next.cord[axis]=float(double(gun[axis])*stretch);
    }
    next.grasp_raw={world.anchor_raw.x+world.pull_axis.x*travel,
        world.anchor_raw.y+world.pull_axis.y*travel,world.anchor_raw.z+world.pull_axis.z*travel};
    for(auto const* matrix:{&next.socket,&next.grip,&next.cord})
        for(float value:*matrix)if(!std::isfinite(value))return false;
    if(!gears_vr::finite(next.grasp_raw))return false;
    out=next;return true;
}

} // namespace gears_weapon
