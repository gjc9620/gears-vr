#pragma once
#include "weapon_boltok_observation.h"
#include "weapon_boltok_child_pose.h"
namespace gears_boltok_observed_geometry {
namespace api=gears_boltok_original;
struct Geometry {gears_vr::Pose support{},cylinder{};};
inline bool pose(api::Matrix const& m,double units,gears_vr::Pose& out) noexcept {
    if(!api::math::proper(m)||!std::isfinite(units)||units<10||units>500)return false;
    api::Atom atom{};
    if(!api::math::atom(m,false,api::Atom{},atom))return false;
    out.position={m[12]/units,m[13]/units,m[14]/units};
    out.orientation=gears_vr::normalized({atom.rotation[0],atom.rotation[1],atom.rotation[2],atom.rotation[3]});
    return gears_vr::stereo_pose_valid(out);
}
inline bool convert(api::Observation const& o,double physical_scale,double units,
    gears_vr::Pose const* physical_cylinder_relative,Geometry& out) noexcept {
    out={};
    if(o.size!=sizeof o||o.version!=1||!o.verified||!o.published||o.restoring||
       !std::isfinite(physical_scale)||physical_scale<=0||physical_scale>10||
       !gears_boltok_observation::matrices_valid(o))return false;
    gears_vr::Pose root{},support{},cylinder{};
    if(!pose(o.root,units,root)||!pose(o.support,units,support)||
       !gears_boltok_child_pose::relative(root,support,out.support))return false;
    const double scale=std::hypot(std::hypot(o.root[0],o.root[1]),o.root[2]);
    if(!std::isfinite(scale)||scale<=0)return false;
    const auto factor=physical_scale/scale;
    out.support.position=gears_weapon::scaled(out.support.position,factor);
    if(o.hidden){
        // Neutral child offset already includes this exact instance's physical scale.
        if(!physical_cylinder_relative||!gears_boltok_child_pose::current(out.support,*physical_cylinder_relative,out.cylinder))return false;
    }else{
        if(!pose(o.cylinder,units,cylinder)||!gears_boltok_child_pose::relative(root,cylinder,out.cylinder))return false;
        out.cylinder.position=gears_weapon::scaled(out.cylinder.position,factor);
    }
    return gears_vr::stereo_pose_valid(out.support)&&gears_vr::stereo_pose_valid(out.cylinder);
}
}
