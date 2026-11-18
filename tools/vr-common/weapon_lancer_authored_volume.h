#pragma once
#include "authored_contact_volume.h"
#include <array>
namespace gears_lancer_authored_volume {
using namespace gears_authored_contact_volume;
// The original Lancer asset and workbench glTF have a reflected axis mapping.
// Use it for POINTS only; wrist bases use a different additional reflection.
// The gun-local point is already inverse-transformed by its actual mesh matrix,
// so do not apply world units/model scale a second time here.
inline vr::Vector asset_point_to_workbench(vr::Vector raw) noexcept {
    return {-raw.x*.01,raw.z*.01,-raw.y*.01};
}
inline bool contains_asset_point(Volume const& v,vr::Vector raw) noexcept {
    return contains(v,asset_point_to_workbench(raw));
}
inline bool wrist_snap(Volume const& v,vr::Pose const& physical_wrist,vr::Pose const& target_wrist,double units) noexcept {
    // Coordinate units may be actual units/metre divided by mesh scale.
    // Live calibration bounds belong to the caller, not this derived ratio.
    if(!valid(v)||!pose_valid(physical_wrist)||!pose_valid(target_wrist)||!std::isfinite(units)||units<=0)return false;
    const auto a=physical_wrist.position,b=target_wrist.position;
    const auto q=physical_wrist.orientation,r=target_wrist.orientation;
    const double distance=std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)/units;
    const double angle=2*std::acos(std::clamp(std::abs(q.x*r.x+q.y*r.y+q.z*r.z+q.w*r.w),0.,1.));
    return distance<=v.snap_distance&&angle<=v.snap_angle;
}
inline bool physical_wrist_in_asset(vr::Pose grip,vr::Pose const& mount_metres,double units,double mesh_scale,vr::Pose& out) noexcept {
    if(!pose_valid(grip)||!pose_valid(mount_metres)||!std::isfinite(units)||units<10||units>500||
        !std::isfinite(mesh_scale)||mesh_scale<.1||mesh_scale>10)return false;
    const auto d=vr::transform(vr::rotation_matrix(grip.orientation),mount_metres.position);
    grip.position={grip.position.x+d.x*units/mesh_scale,grip.position.y+d.y*units/mesh_scale,grip.position.z+d.z*units/mesh_scale};
    grip.orientation=vr::normalized(vr::multiply(grip.orientation,mount_metres.orientation));out=grip;return pose_valid(out);
}
// Current observed part delta in workbench object coordinates. This is a
// geometric inverse, not a tracking sample or permission to move the part.
inline bool contains_moving_asset_point(Volume const& v,vr::Vector raw,vr::Pose const& part_delta) noexcept {
    if(!pose_valid(part_delta)||!vr::finite(raw))return false;
    const auto p=asset_point_to_workbench(raw);
    const auto rest=vr::transform(vr::transpose(vr::rotation_matrix(part_delta.orientation)),
        {p.x-part_delta.position.x,p.y-part_delta.position.y,p.z-part_delta.position.z});
    return contains(v,rest);
}
// Original evaluated row-vector rigid delta, in raw asset coordinates.
// Using the observed matrix avoids inventing a second deployment pivot.
inline bool contains_raw_part_point(Volume const& v,vr::Vector raw,std::array<double,16> const& delta) noexcept {
    for(double n:delta)if(!std::isfinite(n))return false;
    if(!vr::finite(raw)||std::abs(delta[3])+std::abs(delta[7])+std::abs(delta[11])+std::abs(delta[15]-1)>1e-6)return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
        double dot{};for(unsigned k=0;k<3;++k)dot+=delta[i*4+k]*delta[j*4+k];
        if(std::abs(dot-(i==j?1.:0.))>1e-5)return false;
    }
    const double det=delta[0]*(delta[5]*delta[10]-delta[6]*delta[9])-
        delta[1]*(delta[4]*delta[10]-delta[6]*delta[8])+delta[2]*(delta[4]*delta[9]-delta[5]*delta[8]);
    if(std::abs(det-1)>1e-5)return false;
    const double x=raw.x-delta[12],y=raw.y-delta[13],z=raw.z-delta[14];
    return contains_asset_point(v,{x*delta[0]+y*delta[1]+z*delta[2],
        x*delta[4]+y*delta[5]+z*delta[6],x*delta[8]+y*delta[9]+z*delta[10]});
}
}
