#pragma once
#include "controller_input_frame.h"
#include "rig_tracking_math.h"
#include "hand_wrist_calibration.h"

// Pure coordinate composition only. The caller must authenticate the current
// controller frame, profile, rig lifetime and both rigid local bases. This code
// grants no action, freshness, identity or engine-write authority.
namespace gears_boltok_aim_grip {
namespace vr=gears_vr;

struct Input {
    vr::ControllerHand const* hand{}; // Same selected frame; Grip is position, Aim is rotation.
    vr::StereoMatrix const* base_view{};
    vr::Pose const* reference{};
    double units_per_metre{};
    vr::Pose const* wrist_in_mesh{}; // Orientation plus metre position in mesh space.
    vr::Quaternion const* muzzle_in_mesh{}; // Rigid local muzzle orientation only.
    vr::Vector const* palm_marker_in_wrist_metres{}; // Authenticated authored grip marker.
};

struct Result {vr::Pose wrist_world{};vr::Pose mesh_world{};};

// Preserve the same wrist-local palm marker while removing the rifle-only
// wrist pitch. The caller still validates its original admitted mount.
inline bool neutral_calibration(gears_hand_wrist::Calibration const& source,
    gears_hand_wrist::Calibration& out) noexcept {
    const auto q=source.grip_to_wrist_rotation;
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(!source.known||!vr::finite(source.grip_to_wrist_metres)||
       !std::isfinite(n)||std::abs(n-1.)>1e-4)return false;
    const auto local=vr::transform(vr::transpose(vr::rotation_matrix(q)),source.grip_to_wrist_metres);
    if(!vr::finite(local))return false;
    out={true,local,{0,0,0,1}};return true;
}

namespace detail {
inline bool rigid(vr::Quaternion const& q) noexcept {
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&std::abs(n-1.)<=1e-4;
}
inline vr::Vector mul(vr::Vector v,double s) noexcept {return {v.x*s,v.y*s,v.z*s};}
inline vr::Vector sub(vr::Vector a,vr::Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
}

// Let Rw=Aim world rotation and M=muzzle_in_mesh, so the mesh is solved from
// Rw=Mmesh*M as Rmesh=Rw*transpose(M). Wrist and palm are then placed in that
// mesh. Outputs change only after every conversion succeeds.
inline bool derive(Input const& input,Result& out) noexcept {
    if(!input.hand||!input.base_view||!input.reference||!input.wrist_in_mesh||
       !input.muzzle_in_mesh||!input.palm_marker_in_wrist_metres)return false;
    const auto& h=*input.hand;
    if(!vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active)||
       !vr::controller_pose_usable(h.aim,h.aim_flags,h.aim_active)||
       !std::isfinite(input.units_per_metre)||input.units_per_metre<10||input.units_per_metre>500||
       !detail::rigid(input.wrist_in_mesh->orientation)||!detail::rigid(*input.muzzle_in_mesh)||
       !vr::finite(input.wrist_in_mesh->position)||!vr::finite(*input.palm_marker_in_wrist_metres))return false;

    // One basis is used for both poses. Aim.position and Grip.orientation are
    // intentionally irrelevant: only Grip.position and Aim.orientation remain.
    vr::Pose grip{},aim{};
    if(!vr::rig_tracking::pose(*input.base_view,*input.reference,h.grip,input.units_per_metre,grip)||
       !vr::rig_tracking::pose(*input.base_view,*input.reference,h.aim,input.units_per_metre,aim))return false;
    const auto aim_r=vr::rotation_matrix(aim.orientation);
    const auto wrist_local_r=vr::rotation_matrix(input.wrist_in_mesh->orientation);
    const auto mesh_r=vr::multiply(aim_r,vr::transpose(vr::rotation_matrix(*input.muzzle_in_mesh)));
    const auto wrist_r=vr::multiply(mesh_r,wrist_local_r);
    vr::Pose wrist{},mesh{};
    vr::Quaternion wrist_q{},mesh_q{};
    if(!vr::rig_tracking::detail::quaternion(wrist_r,wrist_q)||
       !vr::rig_tracking::detail::quaternion(mesh_r,mesh_q))return false;

    // Grip.position is the anchor. The marker is expressed in wrist metres;
    // wrist_in_mesh.position is separately converted from metres using units.
    // Thus the same mesh transform reconstructs marker world == Grip.position.
    const auto marker_world_offset=vr::transform(wrist_r,
        detail::mul(*input.palm_marker_in_wrist_metres,input.units_per_metre));
    wrist.position=detail::sub(grip.position,marker_world_offset);
    wrist.orientation=wrist_q;
    mesh.orientation=mesh_q;
    const auto wrist_offset=vr::transform(mesh_r,
        detail::mul(input.wrist_in_mesh->position,input.units_per_metre));
    mesh.position=detail::sub(wrist.position,wrist_offset);
    if(!vr::stereo_pose_valid(wrist)||!vr::stereo_pose_valid(mesh))return false;
    out={wrist,mesh};return true;
}
}
