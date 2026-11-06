#pragma once
#include "pose_math.h"
#include "weapon_lancer_contact_dimensions.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace gears_weapon {

// COGAssaultRifle LOD0 b_AR_Magazine reference pivot, checked against a
// same-tick retail bone/socket capture. This is an upper feed-port candidate,
// not a calibrated controller-held item mount or a visible receiver surface.
inline constexpr gears_vr::Vector lancer_magazine_upper_anchor_raw{
    0.00796632598,28.14602802187,4.39606437790};
inline constexpr gears_vr::Vector lancer_magazine_outward_axis_raw{0,0,-1};

struct LancerFeedWorld {
    gears_vr::Vector point_raw{},outward_axis{};
};

// Project the fixed gun-frame reference through the ORIGINAL mesh component.
// Never read the animated Magazine socket as the active VR insertion target.
inline bool lancer_feed_world(std::array<float,16> const& mesh,
                              LancerFeedWorld& out) noexcept {
    for(float value:mesh)if(!std::isfinite(value))return false;
    if(std::abs(mesh[3])>1e-4||std::abs(mesh[7])>1e-4||
       std::abs(mesh[11])>1e-4||std::abs(mesh[15]-1)>1e-4)return false;
    double scale[3]{};
    for(int axis=0;axis<3;++axis){
        scale[axis]=std::hypot(std::hypot(double(mesh[axis*4]),double(mesh[axis*4+1])),
                               double(mesh[axis*4+2]));
        if(scale[axis]<.1||scale[axis]>10)return false;
    }
    for(int i=0;i<3;++i)for(int j=i+1;j<3;++j){
        double dot=0;
        for(int world=0;world<3;++world)
            dot+=double(mesh[i*4+world])*mesh[j*4+world]/(scale[i]*scale[j]);
        if(std::abs(dot)>1e-3||std::abs(scale[i]-scale[j])>
           (std::max)(scale[i],scale[j])*.02)return false;
    }
    const double det=double(mesh[0])*(double(mesh[5])*mesh[10]-double(mesh[6])*mesh[9])-
        double(mesh[1])*(double(mesh[4])*mesh[10]-double(mesh[6])*mesh[8])+
        double(mesh[2])*(double(mesh[4])*mesh[9]-double(mesh[5])*mesh[8]);
    if(det<=0||!std::isfinite(det))return false;
    auto point=gears_vr::Vector{
        mesh[12]+lancer_magazine_upper_anchor_raw.x*mesh[0]+
            lancer_magazine_upper_anchor_raw.y*mesh[4]+
            lancer_magazine_upper_anchor_raw.z*mesh[8],
        mesh[13]+lancer_magazine_upper_anchor_raw.x*mesh[1]+
            lancer_magazine_upper_anchor_raw.y*mesh[5]+
            lancer_magazine_upper_anchor_raw.z*mesh[9],
        mesh[14]+lancer_magazine_upper_anchor_raw.x*mesh[2]+
            lancer_magazine_upper_anchor_raw.y*mesh[6]+
            lancer_magazine_upper_anchor_raw.z*mesh[10]};
    const auto axis=gears_vr::Vector{-mesh[8]/scale[2],-mesh[9]/scale[2],-mesh[10]/scale[2]};
    if(!gears_vr::finite(point)||!gears_vr::finite(axis))return false;
    out={point,axis};return true;
}

// Removal starts by pinching the visible installed body, not by satisfying
// the insertion orientation. This capsule is contact only, never a stroke.
inline bool lancer_magazine_grab(std::array<float,16> const& mesh,gears_vr::Vector palm,double units,
    double* distance_metres=nullptr) noexcept {
    LancerFeedWorld feed{};
    if(!std::isfinite(units)||units<10||units>500||!gears_vr::finite(palm)||!lancer_feed_world(mesh,feed))return false;
    const gears_vr::Vector d{palm.x-feed.point_raw.x,palm.y-feed.point_raw.y,palm.z-feed.point_raw.z};
    const double depth=d.x*feed.outward_axis.x+d.y*feed.outward_axis.y+d.z*feed.outward_axis.z;
    const double along=std::clamp(depth,0.,lancer_contact::magazine_axis_length*units);
    const gears_vr::Vector e{d.x-along*feed.outward_axis.x,d.y-along*feed.outward_axis.y,d.z-along*feed.outward_axis.z};
    const double distance=std::hypot(std::hypot(e.x,e.y),e.z)/units;
    if(distance_metres)*distance_metres=distance;
    return distance<=lancer_contact::magazine_radius;
}
inline bool lancer_seated_magazine(std::array<float,16> const& mesh,gears_vr::Viewpoint& out) noexcept {
    LancerFeedWorld feed{};if(!lancer_feed_world(mesh,feed))return false;
    gears_vr::Matrix axes{};
    for(unsigned i=0;i<3;++i){
        const unsigned source=i==0?1:i==1?0:2;
        const double scale=std::hypot(std::hypot(double(mesh[source*4]),double(mesh[source*4+1])),double(mesh[source*4+2]));
        for(unsigned j=0;j<3;++j)axes[j][i]=(i==2?-1.:1.)*mesh[source*4+j]/scale;
    }
    out={feed.point_raw,gears_vr::ue_angles(axes)};return true;
}
} // namespace gears_weapon
