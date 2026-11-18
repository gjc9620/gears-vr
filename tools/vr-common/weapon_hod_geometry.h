#pragma once
#include "pose_math.h"
namespace gears_weapon_hod_geometry {
// Original COG_HOD.COG_HammerofDawn e8d29e9e, 8808 vertices/9524 triangles.
// The designator points along mesh +Y. Socket Muzzle=(0,40.1358185,12.75)
// has default zero relative rotation, so its +X is NOT the designator axis.
// Original model/side/top review is offline evidence; runtime fit is pending.
inline constexpr gears_vr::Vector axis{0,1,0};
inline gears_vr::Vector direction(gears_vr::Rotator socket) noexcept {
    return gears_vr::transform(gears_vr::ue_matrix(socket),axis);
}
inline gears_vr::Matrix mesh_in_hand(gears_vr::Matrix const& socket_mount) noexcept {
    // Same mounting relation consumed by mesh and query; apply once. This
    // maps model +Y onto hand +X while preserving up and a proper rotation.
    const gears_vr::Matrix local{{{0,1,0},{-1,0,0},{0,0,1}}};
    return gears_vr::multiply(local,socket_mount);
}
}
