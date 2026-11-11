#pragma once
#include "hand_object_grasp.h"
#include "weapon_two_hand_hold.h"
#include "weapon_lancer_contact_dimensions.h"

// Query-local cosmetic geometry only. Never a Hand Frame, lease, tracking
// packet or cached world pose. The caller authenticates the current query.
namespace gears_fresh_support_socket {
namespace vr=gears_vr;
struct Description {
    unsigned kind{},selected{}; // 0 unsupported, 1 exact Hammerburst.
    vr::Pose relative{{},{0,0,0,0}};
};
inline bool empty_geometry(vr::Pose const& p) noexcept {
    return p.position.x==0&&p.position.y==0&&p.position.z==0&&p.orientation.x==0&&
        p.orientation.y==0&&p.orientation.z==0&&p.orientation.w==0;
}
inline bool shape(Description const& d) noexcept {
    return d.kind<=1&&d.selected<=1&&(!d.selected||d.kind==1)&&
        (d.kind==0?(!d.selected&&empty_geometry(d.relative)):
         (gears_weapon_two_hand::geometry(d.relative)||(!d.selected&&empty_geometry(d.relative))));
}
inline bool neutral(vr::ControllerInputFrame const& input) noexcept {
    auto const& l=input.hands[0];
    return l.squeeze_active==1&&l.trigger_active==1&&
        !gears_weapon_hand::control_held(l,vr::control_squeeze)&&!gears_weapon_hand::control_held(l,vr::control_trigger)&&
        vr::controller_pose_usable(l.grip,l.grip_flags,l.grip_active)&&
        vr::controller_pose_usable(input.hands[1].grip,input.hands[1].grip_flags,input.hands[1].grip_active);
}
inline bool contact(vr::ControllerInputFrame const& input,vr::Pose const& geometry) noexcept {
    if(!neutral(input)||!gears_weapon_two_hand::geometry(geometry))return false;
    const auto& right=input.hands[1].grip;const auto& left=input.hands[0].grip;
    // Same UE-to-tracking conversion and volume as fill_support_mount.
    const vr::Vector local{geometry.position.y,geometry.position.z,-geometry.position.x};
    const auto expected=vr::transform(vr::rotation_matrix(right.orientation),local);
    const auto actual=gears_weapon_two_hand::sub(left.position,right.position);
    const auto distance=gears_weapon_two_hand::length(gears_weapon_two_hand::sub(expected,actual));
    const auto a=gears_weapon_two_hand::length(actual),e=gears_weapon_two_hand::length(expected);
    const double dot=expected.x*actual.x+expected.y*actual.y+expected.z*actual.z;
    return std::isfinite(a)&&a>.10&&std::isfinite(distance)&&
        distance<=gears_weapon::lancer_contact::support_radius&&std::abs(a-e)<=.22&&dot/(a*e)>=std::cos(.8);
}
inline bool world(vr::Pose const& right,vr::Pose const& relative,double units,vr::Pose& out) noexcept {
    if(!vr::stereo_pose_valid(right)||!gears_weapon_two_hand::geometry(relative)||
       !std::isfinite(units)||units<10||units>500)return false;
    const auto offset=vr::transform(vr::rotation_matrix(right.orientation),relative.position);
    out={{right.position.x+offset.x*units,right.position.y+offset.y*units,right.position.z+offset.z*units},
        vr::normalized(vr::multiply(right.orientation,relative.orientation))};
    return vr::stereo_pose_valid(out);
}
}
