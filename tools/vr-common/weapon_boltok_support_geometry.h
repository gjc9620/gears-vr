#pragma once
#include "weapon_boltok_owner_profiles.h"

// Pure object/mesh-to-right-hand math. Caller has authenticated the current
// root reference, model-in-right pose and scales. No input, lease or world pose.
namespace gears_boltok_support_geometry {
namespace vr=gears_vr;namespace owner=gears_boltok_owner_profiles;
struct Inputs {
    vr::Pose model_in_right{};vr::Vector root_reference_raw{};
    vr::Pose root_in_right_metres{};double model_scale{},units{};
};
struct Output {
    const char* capture_id{};
    vr::Pose support_wrist_in_right{};vr::Vector volume_center_in_right{};
    double volume_radius_metres{},snap_distance_metres{},snap_angle_radians{},release_distance_metres{};
};
inline bool unit(vr::Quaternion q) noexcept {
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&std::abs(n-1.)<=1e-4;
}
inline bool valid(Inputs const& in) noexcept {
    return vr::stereo_pose_valid(in.model_in_right)&&unit(in.model_in_right.orientation)&&unit(in.root_in_right_metres.orientation)&&vr::finite(in.root_reference_raw)&&
        vr::stereo_pose_valid(in.root_in_right_metres)&&std::isfinite(in.model_scale)&&in.model_scale>0&&
        std::isfinite(in.units)&&in.units>=10&&in.units<=500;
}
inline bool derive(owner::Profile const& profile,Inputs const& in,Output& out) noexcept {
    if(!valid(in)||!profile.capture_id||!profile.capture_id[0]||!unit(profile.wrist_raw_object.orientation)||
       !vr::finite(profile.wrist_raw_object.position)||!vr::finite(profile.volume_center_raw)||
       !std::isfinite(profile.volume_radius_metres)||profile.volume_radius_metres<=0||
       !std::isfinite(profile.snap_distance_metres)||profile.snap_distance_metres<=0||
       !std::isfinite(profile.snap_angle_radians)||profile.snap_angle_radians<0||
       !std::isfinite(profile.release_distance_metres)||profile.release_distance_metres<=0)return false;
    const auto rotation=vr::rotation_matrix(in.model_in_right.orientation);
    const auto root_delta=vr::transform(rotation,{in.root_reference_raw.x*in.model_scale,
        in.root_reference_raw.y*in.model_scale,in.root_reference_raw.z*in.model_scale});
    const vr::Vector root_expected{in.model_in_right.position.x+root_delta.x,
        in.model_in_right.position.y+root_delta.y,in.model_in_right.position.z+root_delta.z};
    const auto root_diff=vr::Vector{
        root_expected.x/in.units-in.root_in_right_metres.position.x,
        root_expected.y/in.units-in.root_in_right_metres.position.y,
        root_expected.z/in.units-in.root_in_right_metres.position.z};
    const double orientation_dot=in.model_in_right.orientation.x*in.root_in_right_metres.orientation.x+
        in.model_in_right.orientation.y*in.root_in_right_metres.orientation.y+
        in.model_in_right.orientation.z*in.root_in_right_metres.orientation.z+
        in.model_in_right.orientation.w*in.root_in_right_metres.orientation.w;
    if(!vr::finite(root_expected)||!vr::finite(root_diff)||
       std::hypot(std::hypot(root_diff.x,root_diff.y),root_diff.z)>.0001||
       2*std::acos(std::clamp(std::abs(orientation_dot),0.,1.))>.0002)return false;
    const auto wrist_delta=vr::transform(rotation,{profile.wrist_raw_object.position.x*in.model_scale,
        profile.wrist_raw_object.position.y*in.model_scale,profile.wrist_raw_object.position.z*in.model_scale});
    const auto volume_delta=vr::transform(rotation,{profile.volume_center_raw.x*in.model_scale,
        profile.volume_center_raw.y*in.model_scale,profile.volume_center_raw.z*in.model_scale});
    Output candidate{};
    candidate.capture_id=profile.capture_id;
    candidate.support_wrist_in_right={
        {(in.model_in_right.position.x+wrist_delta.x)/in.units,
         (in.model_in_right.position.y+wrist_delta.y)/in.units,
         (in.model_in_right.position.z+wrist_delta.z)/in.units},
        vr::normalized(vr::multiply(in.model_in_right.orientation,profile.wrist_raw_object.orientation))};
    candidate.volume_center_in_right=
        {(in.model_in_right.position.x+volume_delta.x)/in.units,
         (in.model_in_right.position.y+volume_delta.y)/in.units,
         (in.model_in_right.position.z+volume_delta.z)/in.units};
    const double physical=owner::authoring_raw_per_metre*in.model_scale/in.units;
    candidate.volume_radius_metres=profile.volume_radius_metres*physical;
    candidate.snap_distance_metres=profile.snap_distance_metres*physical;
    candidate.snap_angle_radians=profile.snap_angle_radians;
    candidate.release_distance_metres=profile.release_distance_metres*physical;
    if(!vr::stereo_pose_valid(candidate.support_wrist_in_right)||!vr::finite(candidate.volume_center_in_right)||
       !std::isfinite(candidate.volume_radius_metres)||!std::isfinite(candidate.snap_distance_metres)||
       !std::isfinite(candidate.release_distance_metres))return false;
    out=candidate;return true;
}
}
