#pragma once
#include "pose_math.h"
namespace gears_boomshot_direction {
// Original Boomshot model: the barrel extends along model +Y, model +Z is
// up. Map those fixed geometry axes to controller/Aim +X forward, +Z up.
// Do not capture a transient animated socket basis during weapon switching.
inline constexpr gears_vr::Matrix mesh_in_hand{{{0,1,0},{-1,0,0},{0,0,1}}};
}
