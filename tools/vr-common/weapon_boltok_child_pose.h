#pragma once
#include "weapon_pose_solver.h"
namespace gears_boltok_child_pose {
// Both arguments use the same local metre basis. No live frame is retained.
inline bool relative(gears_vr::Pose const& parent,gears_vr::Pose const& child,gears_vr::Pose& out) noexcept {
    out={};
    if(!gears_vr::stereo_pose_valid(parent)||!gears_vr::stereo_pose_valid(child))return false;
    const auto inv=gears_vr::inverse(parent.orientation);
    gears_vr::Pose value{};
    value.position=gears_weapon::rotate(inv,gears_weapon::difference(child.position,parent.position));
    value.orientation=gears_vr::normalized(gears_vr::multiply(inv,child.orientation));
    if(!gears_vr::stereo_pose_valid(value))return false;
    out=value;return true;
}
inline bool current(gears_vr::Pose const& parent,gears_vr::Pose const& fixed_child,gears_vr::Pose& out) noexcept {
    out={};
    if(!gears_vr::stereo_pose_valid(parent)||!gears_vr::stereo_pose_valid(fixed_child))return false;
    gears_vr::Pose value{};
    value.position=gears_weapon::sum(parent.position,gears_weapon::rotate(parent.orientation,fixed_child.position));
    value.orientation=gears_vr::normalized(gears_vr::multiply(parent.orientation,fixed_child.orientation));
    if(!gears_vr::stereo_pose_valid(value))return false;
    out=value;return true;
}
}
