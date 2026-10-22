#pragma once
#include "weapon_pose_solver.h"
#include "gnasher_lever.h"
#include <algorithm>
#include <cmath>

namespace gears_weapon {
// Same held-instance SG_Chamber sample as original_gnasher_lever_travel. The
// pivot is in raw gun units; convert using the current stereo units/metre.
// The bone's downstroke rotation axis was (-.999997,.002315,-.000390), so -X
// is the stable reference direction. Neither value identifies the player's
// right-hand contact point on the lever.
inline constexpr Vector gnasher_lever_reference_pivot_raw{
    .014789434,20.622207420,-2.920573568};
inline constexpr Vector gnasher_lever_reference_axis{-1,0,0};
// A separate 24-frame purchased-game held pose measured these original pawn
// hand-bone origins in the same Gnasher main-bone frame. They are candidate
// palm references, not proven XR controller offsets or handle mesh contacts.
inline constexpr Vector gnasher_idle_left_hand_bone_raw{
    8.554029983,25.987054533,-9.762721862};
inline constexpr Vector gnasher_idle_right_hand_bone_raw{
    -2.316310263,-17.539562252,-9.911082264};
// The Gnasher main body is held by the left/front hand during a lever stroke.
// All offsets are in that verified mesh's reference pose, in metres. A model
// bone name alone is not enough to fill these values: the lever pivot, hinge
// axis and right-hand contact point must be checked against its animation.
struct LeverMount {
    Pose left_front_grip_in_main{};
    Vector pivot_in_main{};
    Vector right_handle_home_in_main{};
    Vector hinge_axis_in_main{gnasher_lever_reference_axis};
    double radial_tolerance_metres{.04};
    double axial_tolerance_metres{.04};
    bool valid() const noexcept {
        const auto arm=difference(right_handle_home_in_main,pivot_in_main);
        const auto axis_length=length(hinge_axis_in_main);
        if(!gears_vr::stereo_pose_valid(left_front_grip_in_main)||
           !gears_vr::finite(pivot_in_main)||!gears_vr::finite(right_handle_home_in_main)||
           !gears_vr::finite(hinge_axis_in_main)||!std::isfinite(axis_length)||
           axis_length<.99||axis_length>1.01||!std::isfinite(radial_tolerance_metres)||
           !std::isfinite(axial_tolerance_metres)||radial_tolerance_metres<0||
           radial_tolerance_metres>.15||axial_tolerance_metres<0||
           axial_tolerance_metres>.15)return false;
        const auto axial=dot(arm,hinge_axis_in_main);
        const auto radius=length(difference(arm,scaled(hinge_axis_in_main,axial)));
        return std::isfinite(radius)&&radius>.04&&radius<.7;
    }
};

// This pose intentionally follows the supporting left hand, not the right
// hand that is moving the lever. It is only used while both hands really hold
// their verified Gnasher contacts. Releasing either grip invalidates the step.
inline bool solve_lever_main_pose(gears_vr::ControllerInputFrame const& hands,
                                  gears_vr::StereoSample const& stereo,
                                  LeverMount const& mount,std::int64_t now_ns,
                                  Pose& out,bool allow_sim=false) noexcept {
    if(!mount.valid()||!gears_vr::controller_frame_matches(hands,stereo,now_ns,allow_sim)||
       !gears_vr::controller_pose_usable(hands.hands[0].grip,
           hands.hands[0].grip_flags,hands.hands[0].grip_active))return false;
    const auto& left=hands.hands[0].grip;
    Pose candidate{};
    candidate.orientation=gears_vr::normalized(gears_vr::multiply(
        left.orientation,gears_vr::inverse(mount.left_front_grip_in_main.orientation)));
    candidate.position=difference(left.position,rotate(candidate.orientation,
        mount.left_front_grip_in_main.position));
    if(!gears_vr::stereo_pose_valid(candidate))return false;
    out=candidate;return true;
}

// Convert a right-hand contact position into a signed hinge angle. Radius and
// axial checks prevent a distant hand swipe from being interpreted as a cycle.
// The downward direction must be established from the actual animation.
inline bool lever_angle_from_hands(Pose const& main,Vector right_hand,
                                    LeverMount const& mount,double& out) noexcept {
    if(!mount.valid()||!gears_vr::stereo_pose_valid(main)||!gears_vr::finite(right_hand))return false;
    const auto pivot=sum(main.position,rotate(main.orientation,mount.pivot_in_main));
    const auto axis=rotate(main.orientation,mount.hinge_axis_in_main);
    const auto home=rotate(main.orientation,difference(mount.right_handle_home_in_main,
                                                       mount.pivot_in_main));
    const auto observed=difference(right_hand,pivot);
    const auto home_axial=dot(home,axis),observed_axial=dot(observed,axis);
    const auto home_radial=difference(home,scaled(axis,home_axial));
    const auto observed_radial=difference(observed,scaled(axis,observed_axial));
    const auto home_radius=length(home_radial),observed_radius=length(observed_radial);
    if(!std::isfinite(observed_radius)||
       std::abs(observed_axial-home_axial)>mount.axial_tolerance_metres||
       std::abs(observed_radius-home_radius)>mount.radial_tolerance_metres||
       observed_radius<.04)return false;
    const auto a=scaled(home_radial,1/home_radius),b=scaled(observed_radial,1/observed_radius);
    const auto angle=std::atan2(dot(axis,cross(a,b)),(std::clamp)(dot(a,b),-1.,1.));
    if(!std::isfinite(angle)||angle<-.02||angle>3.14)return false;
    out=(std::max)(0.,angle);return true;
}

inline bool gnasher_lever_sample(gears_vr::ControllerInputFrame const& hands,
                                 gears_vr::StereoSample const& stereo,
                                 LeverMount const& mount,std::uint64_t weapon,
                                 std::uint64_t world,std::int64_t now_ns,
                                 bool left_front_held,bool right_handle_held,
                                 LeverPoseSample& out,bool allow_sim=false) noexcept {
    if(!weapon||!world||!left_front_held||!right_handle_held||
       !gears_vr::controller_pose_usable(hands.hands[1].grip,
           hands.hands[1].grip_flags,hands.hands[1].grip_active))return false;
    Pose main{};double angle{};
    if(!solve_lever_main_pose(hands,stereo,mount,now_ns,main,allow_sim)||
       !lever_angle_from_hands(main,hands.hands[1].grip.position,mount,angle))return false;
    out={weapon,world,hands.space_epoch,hands.sequence,now_ns,angle,true,true,true};
    return true;
}
}
