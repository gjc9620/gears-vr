#pragma once
#include "pose_math.h"

// Same UE world-space rigid-pose mathematics only. No raw XR axis conversion,
// bone scale, skeleton assumptions, identity/age checks or write permission.
namespace gears_hand_wrist {
using gears_vr::Vector;
using gears_vr::Quaternion;
using gears_vr::Pose;

struct Calibration {
    // known means a mathematical offset exists, never physical calibration or
    // Native authorization. The caller binds asset/hand/Rig and invalidation.
    bool known{};
    Vector grip_to_wrist_metres{}; // Wrist origin expressed in grip-local axes.
    Quaternion grip_to_wrist_rotation{}; // qWrist = qGrip * this (Hamilton).
};
enum class Status {okay,unknown_calibration,invalid_units,nonfinite_input,invalid_quaternion,overflow};

namespace detail {
inline bool unit(Quaternion q,Quaternion& out) noexcept {
    if(!std::isfinite(q.x)||!std::isfinite(q.y)||!std::isfinite(q.z)||!std::isfinite(q.w))return false;
    const double scale=(std::max)({std::abs(q.x),std::abs(q.y),std::abs(q.z),std::abs(q.w)});
    if(!(scale>0))return false;
    // Only rescale the four components; reuse pose_math's normalization. This
    // avoids overflowing its norm for an otherwise legal non-unit quaternion.
    try {out=gears_vr::normalized({q.x/scale,q.y/scale,q.z/scale,q.w/scale});}
    catch(std::invalid_argument const&){return false;}
    return true;
}
inline bool units(double value) noexcept {return std::isfinite(value)&&value>0;}
} // namespace detail

// PRECONDITION: the caller has separately established that these two poses
// describe the intended physically/authored ALIGNED grip and wrist. This pure
// function cannot verify alignment. Arbitrary simultaneous grip tracking and
// an original animated wrist pose do not establish a measured calibration.
// Positions are in the SAME UE world space and game units; the stored offset
// is metres in grip-local axes. No automatic anchoring/re-anchoring occurs.
// Every failure leaves out byte-for-byte unchanged, including any prior known.
inline Status from_caller_aligned_world_pair(Pose const& world_grip,Pose const& world_wrist,
    double units_per_metre,Calibration& out) noexcept {
    if(!detail::units(units_per_metre))return Status::invalid_units;
    if(!gears_vr::finite(world_grip.position)||!gears_vr::finite(world_wrist.position))return Status::nonfinite_input;
    Quaternion grip{},wrist{};
    if(!detail::unit(world_grip.orientation,grip)||!detail::unit(world_wrist.orientation,wrist))return Status::invalid_quaternion;
    const Vector delta{world_wrist.position.x-world_grip.position.x,world_wrist.position.y-world_grip.position.y,
                       world_wrist.position.z-world_grip.position.z};
    if(!gears_vr::finite(delta))return Status::overflow;
    const Vector metres{delta.x/units_per_metre,delta.y/units_per_metre,delta.z/units_per_metre};
    if(!gears_vr::finite(metres))return Status::overflow;
    const auto offset=gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(grip)),metres);
    if(!gears_vr::finite(offset))return Status::overflow;
    Quaternion relative{};
    if(!detail::unit(gears_vr::multiply(gears_vr::inverse(grip),wrist),relative))return Status::invalid_quaternion;
    const Calibration candidate{true,offset,relative};out=candidate;return Status::okay;
}

// Apply an explicit known grip-local mount to a world grip already converted
// to the SAME UE world space as Native wrist targets. Full orientation/roll is
// retained. Translation is metres * units exactly once, then rotated by grip.
// No calibration mutation, snapping or re-anchoring; no Native calls/grants.
// Every failure leaves out byte-for-byte unchanged. out may alias world_grip.
inline Status native_wrist_target(Pose const& world_grip,Calibration const& calibration,
    double units_per_metre,Pose& out) noexcept {
    if(!calibration.known)return Status::unknown_calibration;
    if(!detail::units(units_per_metre))return Status::invalid_units;
    if(!gears_vr::finite(world_grip.position)||!gears_vr::finite(calibration.grip_to_wrist_metres))return Status::nonfinite_input;
    Quaternion grip{},relative{};
    if(!detail::unit(world_grip.orientation,grip)||!detail::unit(calibration.grip_to_wrist_rotation,relative))return Status::invalid_quaternion;
    const auto& metres=calibration.grip_to_wrist_metres;
    const Vector local{metres.x*units_per_metre,metres.y*units_per_metre,metres.z*units_per_metre};
    if(!gears_vr::finite(local))return Status::overflow;
    const auto offset=gears_vr::transform(gears_vr::rotation_matrix(grip),local);
    if(!gears_vr::finite(offset))return Status::overflow;
    const Vector position{world_grip.position.x+offset.x,world_grip.position.y+offset.y,world_grip.position.z+offset.z};
    if(!gears_vr::finite(position))return Status::overflow;
    Quaternion orientation{};
    if(!detail::unit(gears_vr::multiply(grip,relative),orientation))return Status::invalid_quaternion;
    const Pose candidate{position,orientation};out=candidate;return Status::okay;
}
} // namespace gears_hand_wrist
