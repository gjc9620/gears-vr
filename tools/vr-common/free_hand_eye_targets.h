#pragma once
#include "controller_input_frame.h"
#include "hand_wrist_calibration.h"
#include <array>

// Pure controller-to-wrist conversion for an already-admitted first-person
// HMD eye. Native owns the common eye basis, tracking admission, animation
// hand-back, fixed-length IK and the single skeletal writer. This file owns
// none of those lifecycles and reads no clock, mapping, game or model memory.
namespace gears_free_hand_eye_targets {
using gears_vr::Pose;
using gears_vr::Quaternion;
using gears_vr::Vector;

enum class Status {okay,invalid_basis,invalid_frame,invalid_scale,invalid_mask};
enum class HandStatus {not_requested,untracked,ready,invalid_pose,invalid_mount};
struct HandTarget {
    Pose grip_world{},wrist_world{};
};
struct Targets {
    std::uint32_t known_mask{};
    std::array<HandStatus,2> status{{HandStatus::not_requested,HandStatus::not_requested}};
    std::array<HandTarget,2> hands{};
};

namespace detail {
inline bool unit(Quaternion q,Quaternion& out) noexcept {
    return gears_hand_wrist::detail::unit(q,out);
}
inline Quaternion conjugate(Quaternion q) noexcept {return {-q.x,-q.y,-q.z,q.w};}
inline Vector subtract(Vector a,Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline bool units(double value) noexcept {
    return std::isfinite(value)&&value>=10&&value<=500;
}
}

// eye_world is the final HMD eye pose in UE world units and F/R/U axes,
// paired with this raw tracking-head pose. It is NOT a head-bone transform,
// pre-HMD render base, CTW, unmodified third-person camera or reference pose.
// The caller must establish that pair from the SAME first-person basis used
// to render this input. Do not derive it from an arbitrary animated wrist.
//
// Raw XR axes are right/up/back. Subtract the raw head position, undo its FULL
// orientation, convert to UE axes, then compose once with the world eye. This
// cancels head motion already present in eye_world; looking alone therefore
// does not retarget a stationary world-space controller. No extra HMD/head,
// body, shoulder, camera-closer or recenter offset is added here.
//
// Failure preserves output; the function does not grant a bone write lease.
inline HandStatus wrist_from_eye(Pose const& eye_world,Pose const& tracking_head,
    Pose const& tracking_grip,gears_hand_wrist::Calibration const& mount,
    double units_per_metre,HandTarget& output) noexcept {
    if(!detail::units(units_per_metre)||!gears_vr::finite(eye_world.position)||
       !gears_vr::finite(tracking_head.position)||!gears_vr::finite(tracking_grip.position))return HandStatus::invalid_pose;
    Quaternion eye_q{},head_q{},grip_q{};
    if(!detail::unit(eye_world.orientation,eye_q)||!detail::unit(tracking_head.orientation,head_q)||
       !detail::unit(tracking_grip.orientation,grip_q))return HandStatus::invalid_pose;
    const auto head_inverse=detail::conjugate(head_q);
    const auto delta=detail::subtract(tracking_grip.position,tracking_head.position);
    const auto head_local=gears_vr::transform(gears_vr::rotation_matrix(head_inverse),delta);
    const auto ue_metres=gears_vr::xr_to_ue(head_local);
    const auto displacement=gears_vr::transform(gears_vr::rotation_matrix(eye_q),
        {ue_metres.x*units_per_metre,ue_metres.y*units_per_metre,ue_metres.z*units_per_metre});
    if(!gears_vr::finite(displacement))return HandStatus::invalid_pose;
    Quaternion relative{};
    if(!detail::unit(gears_vr::multiply(head_inverse,grip_q),relative))return HandStatus::invalid_pose;
    // B R B^-1 with B: XR xyz -> UE (-z,x,y). B is a reflection, so the
    // quaternion vector transforms as det(B)*B*v, not as a set of Euler angles.
    const Quaternion ue_relative{relative.z,-relative.x,-relative.y,relative.w};
    Quaternion world_q{};
    if(!detail::unit(gears_vr::multiply(eye_q,ue_relative),world_q))return HandStatus::invalid_pose;
    HandTarget candidate{};
    candidate.grip_world={{eye_world.position.x+displacement.x,eye_world.position.y+displacement.y,
                           eye_world.position.z+displacement.z},world_q};
    if(!gears_vr::finite(candidate.grip_world.position))return HandStatus::invalid_pose;
    if(gears_hand_wrist::native_wrist_target(candidate.grip_world,mount,units_per_metre,
        candidate.wrist_world)!=gears_hand_wrist::Status::okay)return HandStatus::invalid_mount;
    output=candidate;return HandStatus::ready;
}

// Whole-frame adapter. Caller supplies calibrated mounts from its existing
// authenticated model bindings and an animation/interaction-owned free mask.
// A non-requested or untracked hand remains UNKNOWN, never a retained target.
// Matching here checks tuple/shape, not age, process identity or write rights.
// Caller retains the ORIGINAL admission instant; no receive time is refreshed.
inline Status from_first_person_eye(Pose const& eye_world,gears_vr::StereoSample const& tracking,
    gears_vr::ControllerInputFrame const& controllers,
    std::array<gears_hand_wrist::Calibration,2> const& mounts,
    double units_per_metre,std::uint32_t requested_mask,Targets& output) noexcept {
    if(requested_mask&~3u)return Status::invalid_mask;
    if(!detail::units(units_per_metre))return Status::invalid_scale;
    Quaternion eye_q{},head_q{};
    if(!gears_vr::finite(eye_world.position)||!gears_vr::finite(tracking.head.position)||
       !detail::unit(eye_world.orientation,eye_q)||!detail::unit(tracking.head.orientation,head_q))return Status::invalid_basis;
    if(!gears_vr::controller_frame_shape_valid(controllers)||!tracking.session||!tracking.sequence||!tracking.space_epoch||
       controllers.source!=tracking.source||controllers.session!=tracking.session||controllers.sequence!=tracking.sequence||
       controllers.space_epoch!=tracking.space_epoch||controllers.predicted_time!=tracking.predicted_time)return Status::invalid_frame;
    Targets candidate{};
    for(unsigned side=0;side<2;++side){
        if(!(requested_mask&(1u<<side)))continue;
        auto const& hand=controllers.hands[side];
        if(!gears_vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)){
            candidate.status[side]=HandStatus::untracked;continue;
        }
        candidate.status[side]=wrist_from_eye(eye_world,tracking.head,hand.grip,mounts[side],units_per_metre,candidate.hands[side]);
        if(candidate.status[side]==HandStatus::ready)candidate.known_mask|=1u<<side;
    }
    output=candidate;return Status::okay;
}
} // namespace gears_free_hand_eye_targets
