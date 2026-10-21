#pragma once
#include "controller_input_frame.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gears_weapon {
using gears_vr::Vector;
using gears_vr::Pose;
using gears_vr::Quaternion;

inline Vector difference(Vector a,Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vector sum(Vector a,Vector b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vector scaled(Vector a,double k) noexcept {return {a.x*k,a.y*k,a.z*k};}
inline double dot(Vector a,Vector b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vector cross(Vector a,Vector b) noexcept {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
inline double length(Vector a) noexcept {return std::sqrt(dot(a,a));}
inline Vector rotate(Quaternion q,Vector v) {return gears_vr::transform(gears_vr::rotation_matrix(q),v);}

// Offsets are metres in the verified weapon-mesh frame, not a single generic
// offset shared by all fourteen weapons. The live adapter must determine them
// from each actual mesh/animation before enabling weapon fire.
struct WeaponMount {
    Pose right_grip_in_weapon{};
    Vector support_grip_in_weapon{};
    Vector muzzle_in_weapon{};
    Vector bore_forward_in_weapon{0,0,-1};
    double support_length_tolerance_metres{.12};
    double maximum_support_correction_radians{.8};
    bool valid() const noexcept {
        const auto bore=length(bore_forward_in_weapon);
        const auto support=length(difference(support_grip_in_weapon,right_grip_in_weapon.position));
        return gears_vr::stereo_pose_valid(right_grip_in_weapon)&&
            gears_vr::finite(support_grip_in_weapon)&&gears_vr::finite(muzzle_in_weapon)&&
            gears_vr::finite(bore_forward_in_weapon)&&std::isfinite(bore)&&bore>.9&&bore<1.1&&
            std::isfinite(support)&&support>.15&&support<1.2&&
            std::isfinite(support_length_tolerance_metres)&&support_length_tolerance_metres>=0&&
            support_length_tolerance_metres<=.3&&
            std::isfinite(maximum_support_correction_radians)&&
            maximum_support_correction_radians>0&&maximum_support_correction_radians<=1.2;
    }
};
struct WeaponPoseSolution {
    Pose gun{};
    Vector muzzle{},direction{}; // XR LOCAL, metres and unit direction.
    std::uint64_t session{},sequence{},space_epoch{};
    std::int64_t predicted_time{};
    bool support_used{};
};

inline bool solve_weapon_pose(gears_vr::ControllerInputFrame const& hands,
                              gears_vr::StereoSample const& stereo,
                              WeaponMount const& mount,bool support_gripped,
                              std::int64_t now_ns,WeaponPoseSolution& out,
                              bool allow_sim=false) {
    if(!mount.valid()||!gears_vr::controller_frame_matches(hands,stereo,now_ns,allow_sim)||
       !gears_vr::controller_pose_usable(hands.hands[1].grip,
           hands.hands[1].grip_flags,hands.hands[1].grip_active))return false;
    const auto& right=hands.hands[1].grip;
    WeaponPoseSolution result{};
    result.session=hands.session;result.sequence=hands.sequence;
    result.space_epoch=hands.space_epoch;result.predicted_time=hands.predicted_time;
    auto orientation=gears_vr::normalized(gears_vr::multiply(
        right.orientation,gears_vr::inverse(mount.right_grip_in_weapon.orientation)));
    if(support_gripped&&gears_vr::controller_pose_usable(hands.hands[0].grip,
            hands.hands[0].grip_flags,hands.hands[0].grip_active)){
        const auto expected=rotate(orientation,difference(mount.support_grip_in_weapon,
            mount.right_grip_in_weapon.position));
        const auto actual=difference(hands.hands[0].grip.position,right.position);
        const double expected_length=length(expected),actual_length=length(actual);
        if(std::isfinite(actual_length)&&actual_length>.1&&
           std::abs(actual_length-expected_length)<=mount.support_length_tolerance_metres){
            const auto from=scaled(expected,1/expected_length),to=scaled(actual,1/actual_length);
            const double cosine=(std::clamp)(dot(from,to),-1.,1.);
            if(cosine>=std::cos(mount.maximum_support_correction_radians)){
                const auto axis=cross(from,to);
                const Quaternion correction=gears_vr::normalized(
                    {axis.x,axis.y,axis.z,1+cosine});
                orientation=gears_vr::normalized(gears_vr::multiply(correction,orientation));
                result.support_used=true;
            }
        }
    }
    result.gun.orientation=orientation;
    result.gun.position=difference(right.position,rotate(orientation,mount.right_grip_in_weapon.position));
    result.muzzle=sum(result.gun.position,rotate(orientation,mount.muzzle_in_weapon));
    result.direction=rotate(orientation,mount.bore_forward_in_weapon);
    const double direction_length=length(result.direction);
    if(!gears_vr::stereo_pose_valid(result.gun)||!gears_vr::finite(result.muzzle)||
       !std::isfinite(direction_length)||direction_length<.9||direction_length>1.1)return false;
    result.direction=scaled(result.direction,1/direction_length);
    out=result;return true;
}

struct WeaponAimLine {Vector start{},direction{};}; // game world units
// The game adapter supplies the same frozen recenter reference and original
// world camera basis used by stereo rendering. The current HMD is absent here.
inline bool weapon_world_aim(WeaponPoseSolution const& weapon,Pose const& reference,
                             gears_vr::Viewpoint const& game_base,double units_per_metre,
                             WeaponAimLine& out) {
    if(!weapon.session||!weapon.sequence||!weapon.space_epoch||
       !gears_vr::stereo_pose_valid(reference)||!gears_vr::finite(weapon.muzzle)||
       !gears_vr::finite(weapon.direction)||
       !gears_vr::finite(game_base.position)||
       !std::isfinite(game_base.rotation.pitch)||!std::isfinite(game_base.rotation.yaw)||
       !std::isfinite(game_base.rotation.roll)||
       !std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500)return false;
    const auto ref_inverse=gears_vr::transpose(gears_vr::rotation_matrix(reference.orientation));
    const auto base=gears_vr::ue_matrix(game_base.rotation);
    const auto to_game=[&](Vector xr){return gears_vr::transform(base,
        gears_vr::xr_to_ue(gears_vr::transform(ref_inverse,xr)));};
    WeaponAimLine result{};
    result.start=sum(game_base.position,scaled(to_game(difference(weapon.muzzle,reference.position)),units_per_metre));
    result.direction=to_game(weapon.direction);
    const double unit=length(result.direction);
    if(!gears_vr::finite(result.start)||!std::isfinite(unit)||unit<.9||unit>1.1)return false;
    result.direction=scaled(result.direction,1/unit);
    out=result;return true;
}
}
