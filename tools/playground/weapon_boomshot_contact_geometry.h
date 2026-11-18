#pragma once
#include "weapon_rig_binding.h"
#include "../vr-common/weapon_support_grip.h"
#include "../vr-common/committed_hand_endpoint.h"
#include "../vr-common/weapon_boomshot_barrel_path.h"

// Measured geometry, not a hand grant. A new C0 serial needs a new original
// bone measurement; this object never extrapolates a gun from raw right input.
namespace gears_boomshot_contact {
namespace hand=gears_weapon_hand;namespace rig=gears_rig_tracking;
namespace endpoint=gears_committed_hand;namespace vr=gears_vr;
using P=vr::Pose;using V=vr::Vector;
// Centroid of actual rigid Barrel triangle1454 (zero based), source OBJ
// c6524094...0fe41, vertices188/337/189. Root bind rotation is identity.
// This initial left-side mechanism contact is NOT a fitted hand/wrist pose.
inline constexpr V closed_surface_raw{-8.47420279,63.9282252,5.558382036666667};
inline V minus(V a,V b) noexcept{return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V local(P const& root,V world,double scale) noexcept {
    const auto v=gears_weapon::rotate(vr::inverse(root.orientation),minus(world,root.position));
    return {v.x/scale,v.y/scale,v.z/scale};
}
inline V point(P const& root,V raw,double scale) noexcept {
    const auto d=gears_weapon::rotate(root.orientation,{raw.x*scale,raw.y*scale,raw.z*scale});
    return {root.position.x+d.x,root.position.y+d.y,root.position.z+d.z};
}
inline bool same_endpoint(endpoint::Frame const& a,endpoint::Frame const& b) noexcept {
    return a.serial==b.serial&&a.tick==b.tick&&a.compose==b.compose&&hand::same(a.parent,b.parent)&&
        a.acquisition_begin_ns==b.acquisition_begin_ns&&a.acquisition_end_ns==b.acquisition_end_ns&&
        a.committed_ns==b.committed_ns&&a.returned_ns==b.returned_ns&&
        a.geometry==b.geometry&&vr::stereo_same_pose(a.right_grip,b.right_grip)&&
        vr::stereo_same(a.stereo,b.stereo)&&gears_hand_continuity::same_input(a.controllers,b.controllers);
}
struct Measurement {
    hand::Frame source{};endpoint::Frame committed{};
    hand::ObjectIdentity component{},asset{};
    P root{},barrel_relative{};double scale{};
    std::int64_t measured_ns{};bool known{};
};
struct Geometry {
    V raw_left_root_m{},surface_world{};
    double progress{},distance_m{};
    std::int64_t input_sync_ns{},geometry_acquired_ns{};
    std::uint64_t endpoint_serial{};
};
// Pure spatial calculation shared by current typed array reads and the GT
// getter comparison. The caller authenticates its source and observation.
inline bool spatial(P const& root,P const& barrel_relative,double scale,rig::Frame const& tracking,Geometry& out) noexcept {
    out={};
    if(!vr::stereo_pose_valid(root)||!vr::stereo_pose_valid(barrel_relative)||!std::isfinite(scale)||scale<.01||scale>10||
       !std::isfinite(tracking.units_per_metre)||tracking.units_per_metre<10||tracking.units_per_metre>500||
       !(tracking.world_pose_mask&rig::left_grip_pose)||!vr::stereo_pose_valid(tracking.world_grip[0]))return false;
    double q{};if(!gears_weapon::boomshot_barrel::progress(barrel_relative.position,q))return false;
    const auto& rotation=barrel_relative.orientation;
    const double length=std::sqrt(rotation.x*rotation.x+rotation.y*rotation.y+rotation.z*rotation.z+rotation.w*rotation.w);
    if(std::abs(rotation.w)/length<std::cos(.017453292519943295))return false;
    auto raw=closed_surface_raw;raw.y-=q*gears_weapon::boomshot_barrel::travel_raw;
    Geometry g{};g.surface_world=point(root,raw,scale);
    g.raw_left_root_m=local(root,tracking.world_grip[0].position,scale*tracking.units_per_metre);
    const auto delta=minus(tracking.world_grip[0].position,g.surface_world);
    g.distance_m=std::hypot(delta.x,delta.y,delta.z)/tracking.units_per_metre;g.progress=q;
    if(!vr::finite(g.surface_world)||!vr::finite(g.raw_left_root_m)||!std::isfinite(g.distance_m))return false;
    out=g;return true;
}
inline bool evaluate(Measurement const& m,hand::Frame const& f,rig::Frame const& tracking,
    endpoint::Frame const& actual,std::uint64_t held,std::int64_t now,Geometry& out) noexcept {
    out={};const auto source=unsigned(f.stereo.source);
    if(!m.known||!hand::identity_valid(m.component)||!hand::identity_valid(m.asset)||
       !vr::stereo_pose_valid(m.root)||!vr::stereo_pose_valid(m.barrel_relative)||
       !std::isfinite(m.scale)||m.scale<.01||m.scale>10||m.measured_ns<=0||
       now<m.measured_ns||now-m.measured_ns>100000000||
       !hand::same_frame(m.source,f)||
       gears_equipment_rig::match(f,tracking,now,source)!=gears_equipment_rig::Status::matched||
       !(tracking.world_pose_mask&rig::left_grip_pose)||
       !endpoint::matches(m.committed,f,tracking,held,now,source==1)||
       !endpoint::matches(actual,f,tracking,held,now,source==1)||!same_endpoint(m.committed,actual))return false;
    Geometry g{};if(!spatial(m.root,m.barrel_relative,m.scale,tracking,g))return false;
    g.input_sync_ns=f.controllers.sync_ns;g.geometry_acquired_ns=m.committed.acquisition_begin_ns;
    g.endpoint_serial=m.committed.serial;
    if(!vr::finite(g.surface_world)||!vr::finite(g.raw_left_root_m)||!std::isfinite(g.distance_m))return false;
    out=g;return true;
}
}
