#pragma once
#include "weapon_torque_bow_arrow_tracking.h"
#include "../vr-common/weapon_torque_bow_feed.h"
// Cosmetic only. One solved grip places BOTH the original arrow and authored
// palm. raw remains the authenticated, untouched mechanical observation.
namespace gears_torque_bow_arrow_presentation {
namespace vr=gears_vr;namespace tracking=gears_torque_bow_arrow_tracking;
namespace am=gears_torque_bow_arrow_mount;namespace w=gears_weapon;
struct Config {w::TorqueBowFeedCalibration const& feed;w::WeaponMeshFollow const& fixed;std::uint64_t arrow_serial{};};
struct Result {
    tracking::Sample raw{};vr::Pose source_world{},tip_world{},grip_world{};
    bool feed_assisted{};double raw_depth{},raw_lateral{},raw_angle{},grip_correction_metres{};
};
inline bool same_pose(vr::Pose const& a,vr::Pose const& b,double epsilon) noexcept {
    if(!tracking::close_vector(a.position,b.position,epsilon))return false;
    const auto q=a.orientation,p=b.orientation;const double sign=q.x*p.x+q.y*p.y+q.z*p.z+q.w*p.w<0?-1.:1.;
    return std::abs(q.x-sign*p.x)<=epsilon&&std::abs(q.y-sign*p.y)<=epsilon&&
        std::abs(q.z-sign*p.z)<=epsilon&&std::abs(q.w-sign*p.w)<=epsilon;
}
inline bool calibrated(w::TorqueBowFeedCalibration const& current,double bow_scale,am::Mount const& arrow) noexcept {
    w::TorqueBowFeedCalibration expected{};
    if(!w::author_torque_bow_feed(current.instance_serial,bow_scale,arrow,expected)||
       current.bow_asset!=expected.bow_asset||current.arrow_asset!=expected.arrow_asset||!current.valid())return false;
    const auto& a=current.mount;const auto& b=expected.mount;
    // Root scale reconstructed from a float engine matrix may vary by a few
    // ulps. This is a geometry comparison, never a wider action tolerance.
    return same_pose(a.port_in_weapon,b.port_in_weapon,1e-6)&&same_pose(a.item_in_left_grip,b.item_in_left_grip,1e-8)&&
        tracking::close_vector(a.outward_axis_in_weapon,b.outward_axis_in_weapon,1e-10)&&
        std::abs(a.approach_depth_metres-b.approach_depth_metres)<=1e-6&&a.seated_depth_metres==b.seated_depth_metres&&
        a.lateral_tolerance_metres==b.lateral_tolerance_metres&&a.maximum_orientation_error_radians==b.maximum_orientation_error_radians&&
        a.maximum_relative_speed_metres_per_second==b.maximum_relative_speed_metres_per_second&&a.maximum_sample_gap_ns==b.maximum_sample_gap_ns;
}
inline bool make(gears_weapon_hand::Frame const& f,gears_rig_tracking::Frame const& rig,
    am::Mount const& arrow,Config const& config,std::int64_t now,bool keeping,Result& out) noexcept {
    out={};Result value{};tracking::RootSample root{};
    if(!config.arrow_serial||!tracking::sample(f,rig,arrow,now,value.raw)||!tracking::root_sample(f,rig,config.fixed,now,root)||
       !calibrated(config.feed,root.scale,arrow))return false;
    value.source_world=value.raw.source_world;value.tip_world=value.raw.tip_world;value.grip_world=rig.world_grip[0];
    const double units=rig.units_per_metre;
    auto gun=root.world,left=rig.world_grip[0];gun.position=am::mul(gun.position,1/units);left.position=am::mul(left.position,1/units);
    const w::FeedSample raw{f.weapon.address,config.arrow_serial,f.world.address,f.controllers.space_epoch,f.controllers.sequence,
        f.controllers.sync_ns,gun,left,true,true};
    w::TorqueBowFeedSnap snap{};
    if(w::torque_bow_feed_snap(raw,config.feed,keeping,false,snap)){
        const vr::Pose tip{am::mul(snap.item.position,units),snap.item.orientation};
        const auto rotation=vr::normalized(vr::multiply(tip.orientation,vr::inverse(arrow.tip_in_grip_units.orientation)));
        const vr::Pose grip{am::sub(tip.position,am::rotate(rotation,arrow.tip_in_grip_units.position)),rotation};
        vr::Pose source{};
        if(!am::source_from_tip(tip,arrow.component_scale,source)||!am::pose_valid(grip))return false;
        const auto delta=am::sub(grip.position,value.grip_world.position);
        const double correction=std::hypot(std::hypot(delta.x,delta.y),delta.z)/units;
        // A small head correction must not pivot a long arrow into a large
        // palm pull. Outside this bound, use the untouched current raw pose.
        if(std::isfinite(correction)&&correction<=(keeping?.05:.04)){
            value.source_world=source;value.tip_world=tip;value.grip_world=grip;value.feed_assisted=true;
            value.raw_depth=snap.raw_depth;value.raw_lateral=snap.raw_lateral;value.raw_angle=snap.raw_angle;
            value.grip_correction_metres=correction;
        }
    }
    out=value;return true;
}
}
