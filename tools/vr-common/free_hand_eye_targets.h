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

enum class Status {okay,invalid_key,key_mismatch,invalid_basis,invalid_frame,invalid_scale,invalid_mask,invalid_side};
enum class HandStatus {not_requested,untracked,ready,invalid_pose,invalid_mount};
// Caller copies this key from ONE admitted C0 invocation and its actual input.
// Comparing it never authenticates a process, refreshes a frame or grants a
// writer. No key is synthesized by the target converter.
struct Key {
    std::uint32_t game_pid{},game_thread{};
    std::uint64_t game_start{},native_started_qpc{},world_generation{},mesh{},binding_generation{},tick{},compose{};
    gears_vr::StereoSource source{gears_vr::StereoSource::RuntimeTracked};
    std::uint64_t stereo_session{},stereo_sequence{},space_epoch{},reference_revision{};
    std::uint64_t controller_sequence{},input_epoch{},profile_generation{};
    std::int64_t predicted_time{};
};
struct EyeFrame {
    Key key{};
    Pose actual_eye_world{}; // SAME invocation's body Output.desired_eye.
};
struct TrackingFrame {
    Key key{};
    gears_vr::StereoSample stereo{};
    gears_vr::ControllerInputFrame controllers{};
};
struct HandTarget {
    Pose grip_world{},wrist_world{};
};
struct Targets {
    Key key{};
    std::uint32_t known_mask{};
    std::array<HandStatus,2> status{{HandStatus::not_requested,HandStatus::not_requested}};
    std::array<HandTarget,2> hands{};
};
struct Preparation {
    Status frame{Status::invalid_key};
    HandStatus hand{HandStatus::not_requested};
    explicit operator bool() const noexcept {return frame==Status::okay&&hand==HandStatus::ready;}
};

inline bool same_key(Key const& a,Key const& b) noexcept {
    return a.game_pid==b.game_pid&&a.game_thread==b.game_thread&&a.game_start==b.game_start&&
        a.native_started_qpc==b.native_started_qpc&&a.world_generation==b.world_generation&&a.mesh==b.mesh&&
        a.binding_generation==b.binding_generation&&a.tick==b.tick&&a.compose==b.compose&&a.source==b.source&&
        a.stereo_session==b.stereo_session&&a.stereo_sequence==b.stereo_sequence&&a.space_epoch==b.space_epoch&&
        a.reference_revision==b.reference_revision&&a.controller_sequence==b.controller_sequence&&
        a.input_epoch==b.input_epoch&&a.profile_generation==b.profile_generation&&a.predicted_time==b.predicted_time;
}

namespace detail {
inline bool unit(Quaternion q,Quaternion& out) noexcept {
    return gears_hand_wrist::detail::unit(q,out);
}
inline Quaternion conjugate(Quaternion q) noexcept {return {-q.x,-q.y,-q.z,q.w};}
inline Vector subtract(Vector a,Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline bool units(double value) noexcept {
    return std::isfinite(value)&&value>=10&&value<=500;
}
inline bool key_shape(Key const& k) noexcept {
    return k.game_pid&&k.game_thread&&k.game_start&&k.native_started_qpc&&k.world_generation&&k.mesh&&
        k.binding_generation&&k.tick&&k.compose&&gears_vr::stereo_source_known(k.source)&&k.stereo_session&&
        k.stereo_sequence&&k.space_epoch&&k.reference_revision&&k.controller_sequence&&k.input_epoch&&
        k.profile_generation&&k.predicted_time>0;
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

// Same-invocation association only, deliberately without a clock/age verdict.
inline Status frame_status(EyeFrame const& eye,TrackingFrame const& tracking,double units_per_metre) noexcept {
    if(!detail::key_shape(eye.key)||!detail::key_shape(tracking.key))return Status::invalid_key;
    if(!same_key(eye.key,tracking.key))return Status::key_mismatch;
    if(!detail::units(units_per_metre))return Status::invalid_scale;
    Quaternion eye_q{},head_q{};
    auto const& s=tracking.stereo;auto const& c=tracking.controllers;auto const& k=tracking.key;
    if(!gears_vr::finite(eye.actual_eye_world.position)||!gears_vr::finite(s.head.position)||
       !detail::unit(eye.actual_eye_world.orientation,eye_q)||!detail::unit(s.head.orientation,head_q))return Status::invalid_basis;
    if(!gears_vr::controller_frame_shape_valid(c)||s.source!=k.source||s.session!=k.stereo_session||
       s.sequence!=k.stereo_sequence||s.space_epoch!=k.space_epoch||s.predicted_time!=k.predicted_time||
       c.source!=s.source||c.session!=s.session||c.sequence!=s.sequence||c.sequence!=k.controller_sequence||
       c.space_epoch!=s.space_epoch||c.predicted_time!=s.predicted_time||
       c.profile_generation!=k.profile_generation||c.input_epoch!=k.input_epoch)return Status::invalid_frame;
    return Status::okay;
}

// Per-side adapter for compose_common's existing binding.authored_mount.
// Output is unchanged on failure. A hand without a tracked full grip never
// contributes a target; preparing that hand cannot mutate the other side.
inline Preparation prepare_one(EyeFrame const& eye,TrackingFrame const& tracking,unsigned side,
    gears_hand_wrist::Calibration const& mount,double units_per_metre,HandTarget& output) noexcept {
    if(side>1)return {Status::invalid_side,HandStatus::not_requested};
    const auto frame=frame_status(eye,tracking,units_per_metre);
    if(frame!=Status::okay)return {frame,HandStatus::not_requested};
    auto const& hand=tracking.controllers.hands[side];
    if(!gears_vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active))return {Status::okay,HandStatus::untracked};
    return {Status::okay,wrist_from_eye(eye.actual_eye_world,tracking.stereo.head,hand.grip,mount,units_per_metre,output)};
}

// Whole-frame adapter. Caller supplies calibrated mounts from its existing
// authenticated model bindings and an animation/interaction-owned free mask.
// A non-requested or untracked hand remains UNKNOWN, never a retained target.
// Matching here checks key/tuple/shape, not age, process identity or write rights.
// Caller retains the ORIGINAL admission instant; no receive time is refreshed.
inline Status prepare_both(EyeFrame const& eye,TrackingFrame const& tracking,
    std::array<gears_hand_wrist::Calibration,2> const& mounts,
    double units_per_metre,std::uint32_t requested_mask,Targets& output) noexcept {
    if(requested_mask&~3u)return Status::invalid_mask;
    const auto frame=frame_status(eye,tracking,units_per_metre);
    if(frame!=Status::okay)return frame;
    Targets candidate{};candidate.key=eye.key;
    for(unsigned side=0;side<2;++side){
        if(!(requested_mask&(1u<<side)))continue;
        auto const& hand=tracking.controllers.hands[side];
        if(!gears_vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)){
            candidate.status[side]=HandStatus::untracked;continue;
        }
        candidate.status[side]=wrist_from_eye(eye.actual_eye_world,tracking.stereo.head,hand.grip,mounts[side],units_per_metre,candidate.hands[side]);
        if(candidate.status[side]==HandStatus::ready)candidate.known_mask|=1u<<side;
    }
    output=candidate;return Status::okay;
}
} // namespace gears_free_hand_eye_targets
