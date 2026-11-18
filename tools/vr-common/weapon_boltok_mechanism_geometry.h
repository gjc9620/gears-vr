#pragma once
#include "weapon_boltok_owner_profiles.h"
#include "hand_authored_mount_profile.h"

// Pure captured owner-mechanism conversion. The caller authenticates the live
// root, model-in-right transform, scale, units and hinge; this grants nothing.
namespace gears_boltok_mechanism_geometry {
namespace vr=gears_vr;
namespace owner=gears_boltok_owner_profiles;
struct Inputs {
    // model_in_right.position is RAW follower units; root is RIGHT metres.
    vr::Pose model_in_right{};
    vr::Pose root_in_right_metres{};
    vr::Vector hinge_pivot_in_root_metres{};
    vr::Vector hinge_axis_in_root{};
    double model_scale{};
    double units{};
    // Actual current root-space rotation of the existing hinge.
    double angle_radians{};
    // Existing LEFT authored calibration, not live left input.
    gears_hand_wrist::Calibration left_calibration{};
};
struct Volume {
    vr::Vector center_in_root_metres{};
    double radius_metres{},snap_distance_metres{},snap_angle_radians{},release_distance_metres{};
};
struct Output {
    const char* capture_id{};
    vr::Pose closed_grip_in_root_metres{};
    vr::Pose current_grip_in_root_metres{};
    Volume volume{};
    double angle_radians{};
};
inline bool unit(vr::Quaternion q) noexcept {
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&std::abs(n-1.)<=1e-4;
}
inline bool axis_unit(vr::Vector a) noexcept {
    const double n=a.x*a.x+a.y*a.y+a.z*a.z;
    return vr::finite(a)&&std::isfinite(n)&&std::abs(n-1.)<=1e-6;
}
inline bool valid(Inputs const& in) noexcept {
    return vr::stereo_pose_valid(in.model_in_right)&&vr::stereo_pose_valid(in.root_in_right_metres)&&
        unit(in.model_in_right.orientation)&&unit(in.root_in_right_metres.orientation)&&
        vr::finite(in.hinge_pivot_in_root_metres)&&axis_unit(in.hinge_axis_in_root)&&
        std::isfinite(in.model_scale)&&in.model_scale>0&&std::isfinite(in.units)&&
        in.units>=10&&in.units<=500&&std::isfinite(in.angle_radians)&&
        in.angle_radians>=-1e-8&&in.angle_radians<=1.48+1e-8&&
        in.left_calibration.known&&vr::finite(in.left_calibration.grip_to_wrist_metres)&&
        unit(in.left_calibration.grip_to_wrist_rotation);
}
// Rotate a ROOT-space point about the ACTUAL measured hinge pivot and axis.
inline vr::Vector rotate_root(vr::Vector point,vr::Vector pivot,vr::Vector axis,double angle) noexcept {
    const auto d=vr::Vector{point.x-pivot.x,point.y-pivot.y,point.z-pivot.z};
    const double c=std::cos(angle),s=std::sin(angle);
    const auto cross=vr::Vector{axis.y*d.z-axis.z*d.y,axis.z*d.x-axis.x*d.z,axis.x*d.y-axis.y*d.x};
    const double dot=axis.x*d.x+axis.y*d.y+axis.z*d.z;
    const auto rotated=vr::Vector{
        d.x*c+cross.x*s+axis.x*dot*(1-c),
        d.y*c+cross.y*s+axis.y*dot*(1-c),
        d.z*c+cross.z*s+axis.z*dot*(1-c)};
    return {pivot.x+rotated.x,pivot.y+rotated.y,pivot.z+rotated.z};
}
inline vr::Quaternion axis_angle(vr::Vector axis,double angle) noexcept {
    const double s=std::sin(angle/2);
    return {axis.x*s,axis.y*s,axis.z*s,std::cos(angle/2)};
}
// The captured profile's delta is verified identity for this source. Reject a
// nonidentity delta explicitly until its inverse is defined by accepted data.
inline bool derive(owner::Profile const& profile,Inputs const& in,Output& out) noexcept {
    if(!valid(in)||!profile.capture_id||!profile.capture_id[0]||
       !unit(profile.wrist_raw_object.orientation)||!vr::finite(profile.wrist_raw_object.position)||
       !vr::finite(profile.volume_center_raw)||!std::isfinite(profile.volume_radius_metres)||
       profile.volume_radius_metres<=0||!std::isfinite(profile.snap_distance_metres)||
       profile.snap_distance_metres<=0||!std::isfinite(profile.snap_angle_radians)||
       profile.snap_angle_radians<0||!std::isfinite(profile.release_distance_metres)||
       profile.release_distance_metres<=0)return false;
    if(!gears_hand_authored_grasp::identity(profile.captured_delta_xr_object))return false;
    const auto model_r=vr::rotation_matrix(in.model_in_right.orientation);
    const auto root_r=vr::rotation_matrix(in.root_in_right_metres.orientation);
    const auto model_to_root=vr::multiply(vr::transpose(root_r),model_r);
    // Captured wrist is authored raw object units. Remove the existing LEFT
    // palm marker and wrist bend to obtain the controller GRIP in object space.
    const auto wrist_world_q=profile.wrist_raw_object.orientation;
    const auto grip_world_q=vr::normalized(vr::multiply(wrist_world_q,
        vr::inverse(in.left_calibration.grip_to_wrist_rotation)));
    const auto palm_offset_raw=vr::transform(vr::rotation_matrix(grip_world_q),
        vr::Vector{in.left_calibration.grip_to_wrist_metres.x*in.units/in.model_scale,
            in.left_calibration.grip_to_wrist_metres.y*in.units/in.model_scale,
            in.left_calibration.grip_to_wrist_metres.z*in.units/in.model_scale});
    const auto grip_world_pos=vr::Vector{
        profile.wrist_raw_object.position.x-palm_offset_raw.x,
        profile.wrist_raw_object.position.y-palm_offset_raw.y,
        profile.wrist_raw_object.position.z-palm_offset_raw.z};
    const auto grip_in_model=vr::Vector{grip_world_pos.x*in.model_scale,
        grip_world_pos.y*in.model_scale,grip_world_pos.z*in.model_scale};
    const auto volume_in_model=vr::Vector{profile.volume_center_raw.x*in.model_scale,
        profile.volume_center_raw.y*in.model_scale,profile.volume_center_raw.z*in.model_scale};
    const auto grip_in_right_raw=vr::Vector{
        in.model_in_right.position.x+vr::transform(model_r,grip_in_model).x,
        in.model_in_right.position.y+vr::transform(model_r,grip_in_model).y,
        in.model_in_right.position.z+vr::transform(model_r,grip_in_model).z};
    const auto volume_in_right_raw=vr::Vector{
        in.model_in_right.position.x+vr::transform(model_r,volume_in_model).x,
        in.model_in_right.position.y+vr::transform(model_r,volume_in_model).y,
        in.model_in_right.position.z+vr::transform(model_r,volume_in_model).z};
    const auto closed_grip=vr::transform(vr::transpose(root_r),{
        grip_in_right_raw.x/in.units-in.root_in_right_metres.position.x,
        grip_in_right_raw.y/in.units-in.root_in_right_metres.position.y,
        grip_in_right_raw.z/in.units-in.root_in_right_metres.position.z});
    const auto closed_volume=vr::transform(vr::transpose(root_r),{
        volume_in_right_raw.x/in.units-in.root_in_right_metres.position.x,
        volume_in_right_raw.y/in.units-in.root_in_right_metres.position.y,
        volume_in_right_raw.z/in.units-in.root_in_right_metres.position.z});
    const auto model_q_in_root=vr::normalized(vr::multiply(
        vr::inverse(in.root_in_right_metres.orientation),in.model_in_right.orientation));
    const auto closed_q=vr::normalized(vr::multiply(model_q_in_root,grip_world_q));
    const auto pivot=in.hinge_pivot_in_root_metres;
    const auto delta=axis_angle(in.hinge_axis_in_root,in.angle_radians);
    const auto current_pos=rotate_root(closed_grip,pivot,in.hinge_axis_in_root,in.angle_radians);
    const auto current_q=vr::normalized(vr::multiply(delta,closed_q));
    const auto current_volume=rotate_root(closed_volume,pivot,in.hinge_axis_in_root,in.angle_radians);
    const double physical=owner::authoring_raw_per_metre*in.model_scale/in.units;
    Output candidate{};
    candidate.capture_id=profile.capture_id;
    candidate.closed_grip_in_root_metres={closed_grip,closed_q};
    candidate.current_grip_in_root_metres={current_pos,current_q};
    candidate.volume.center_in_root_metres=current_volume;
    candidate.volume.radius_metres=profile.volume_radius_metres*physical;
    candidate.volume.snap_distance_metres=profile.snap_distance_metres*physical;
    candidate.volume.snap_angle_radians=profile.snap_angle_radians;
    candidate.volume.release_distance_metres=profile.release_distance_metres*physical;
    candidate.angle_radians=in.angle_radians;
    if(!vr::stereo_pose_valid(candidate.closed_grip_in_root_metres)||
       !vr::stereo_pose_valid(candidate.current_grip_in_root_metres)||
       !vr::finite(candidate.volume.center_in_root_metres)||
       !std::isfinite(candidate.volume.radius_metres)||
       !std::isfinite(candidate.volume.snap_distance_metres)||
       !std::isfinite(candidate.volume.release_distance_metres))return false;
    out=candidate;return true;
}
} // namespace gears_boltok_mechanism_geometry
