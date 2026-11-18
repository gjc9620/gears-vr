#pragma once
#include "weapon_pose_solver.h"
// Numeric conversion only. The live adapter must authenticate and bracket
// the CURRENT component/bone matrices; package quaternions are not that proof.
namespace gears_weapon::gnasher_live_basis {
inline bool relation(Pose const& parent_world,double parent_scale,Pose const& child_world,
    double child_scale,double units,Pose& child_in_parent_metres) noexcept {
    if(!gears_vr::stereo_pose_valid(parent_world)||!gears_vr::stereo_pose_valid(child_world)||
       !std::isfinite(parent_scale)||parent_scale<=.1||parent_scale>=10||
       !std::isfinite(child_scale)||std::abs(parent_scale-child_scale)>1e-5||
       !std::isfinite(units)||units<10||units>500)return false;
    Pose value{};const auto inverse=gears_vr::inverse(parent_world.orientation);
    // World offsets already contain component scale. Convert engine units
    // to metres once; dividing by scale here would later apply it twice.
    value.position=scaled(rotate(inverse,difference(child_world.position,parent_world.position)),1/units);
    value.orientation=gears_vr::normalized(gears_vr::multiply(inverse,child_world.orientation));
    if(!gears_vr::stereo_pose_valid(value))return false;
    child_in_parent_metres=value;return true;
}
inline bool component_for_main(Pose const& desired_main_metres,Pose const& main_in_component_metres,
    Pose& component_metres) noexcept {
    if(!gears_vr::stereo_pose_valid(desired_main_metres)||!gears_vr::stereo_pose_valid(main_in_component_metres))return false;
    Pose value{};value.orientation=gears_vr::normalized(gears_vr::multiply(desired_main_metres.orientation,
        gears_vr::inverse(main_in_component_metres.orientation)));
    value.position=difference(desired_main_metres.position,rotate(value.orientation,main_in_component_metres.position));
    if(!gears_vr::stereo_pose_valid(value))return false;component_metres=value;return true;
}
}
