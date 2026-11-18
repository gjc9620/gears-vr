#pragma once
#include "weapon_pose_solver.h"

namespace gears_weapon {
// Unscaled gun-local engine units, matching Snub's baked contact geometry.
// Both arguments must come from the same current observation; this function
// grants no input, lease or freshness. Never unscale the original part twice.
inline bool snub_part_contact_local(gears_vr::Pose const& gun,
    gears_vr::Vector world_contact,gears_vr::Vector& out) noexcept {
    out={};
    if(!gears_vr::stereo_pose_valid(gun)||!gears_vr::finite(world_contact))return false;
    const auto local=rotate(gears_vr::inverse(gun.orientation),difference(world_contact,gun.position));
    if(!gears_vr::finite(local))return false;
    out=local;return true;
}
}
