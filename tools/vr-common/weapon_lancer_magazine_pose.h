#pragma once
#include "pose_math.h"

namespace gears_weapon {
// Capture the installed item's relative pose once at a genuine pinch. The
// item starts where it was visible and follows the unassisted physical hand.
struct LancerMagazineGraspMount {
    gears_vr::Vector position{};gears_vr::Matrix orientation{};bool known{};
    bool capture(gears_vr::Pose const& grip,gears_vr::Viewpoint const& item) noexcept try {
        if(!gears_vr::finite(grip.position)||!gears_vr::finite(item.position)||
           !std::isfinite(item.rotation.pitch)||!std::isfinite(item.rotation.yaw)||!std::isfinite(item.rotation.roll))return false;
        const auto inverse=gears_vr::transpose(gears_vr::rotation_matrix(grip.orientation));
        const auto p=gears_vr::transform(inverse,{item.position.x-grip.position.x,item.position.y-grip.position.y,item.position.z-grip.position.z});
        const auto q=gears_vr::multiply(inverse,gears_vr::ue_matrix(item.rotation));
        if(!gears_vr::finite(p))return false;position=p;orientation=q;known=true;return true;
    }catch(...){return false;}
    bool apply(gears_vr::Pose const& grip,gears_vr::Viewpoint& out) const noexcept try {
        if(!known||!gears_vr::finite(grip.position))return false;
        const auto q=gears_vr::rotation_matrix(grip.orientation);
        const auto offset=gears_vr::transform(q,position);
        out={{grip.position.x+offset.x,grip.position.y+offset.y,grip.position.z+offset.z},
             gears_vr::ue_angles(gears_vr::multiply(q,orientation))};return true;
    }catch(...){return false;}
};
// Measured in one original retail Lancer reload's stable hand-held interval.
// This is a left-hand WRIST BONE mount, never a controller-grip mount.
// Live consumers pass the actual committed wrist, after grip calibration and
// fixed-length reach projection. Offset units are original mesh units.
inline constexpr gears_vr::Vector lancer_magazine_in_left_origin_raw{
    19.03603543,-5.56617749,.13964627};
inline constexpr double lancer_magazine_in_left_axes[3][3]={
    {.03740158,.13477868,-.99016975},
    {-.03192186,-.99019607,-.13598739},
    {-.99879037,.03669508,-.03273385}};
inline bool lancer_magazine_from_left(gears_vr::Viewpoint const& left,
                                      gears_vr::Viewpoint& magazine,double model_scale=1.) noexcept {
    if(!gears_vr::finite(left.position)||
       !std::isfinite(model_scale)||model_scale<=0||
       !std::isfinite(left.rotation.pitch)||!std::isfinite(left.rotation.yaw)||
       !std::isfinite(left.rotation.roll))return false;
    const auto hand=gears_vr::ue_matrix(left.rotation);
    const auto raw=lancer_magazine_in_left_origin_raw;
    const auto offset=gears_vr::transform(hand,{raw.x*model_scale,raw.y*model_scale,raw.z*model_scale});
    magazine.position={left.position.x+offset.x,left.position.y+offset.y,
                       left.position.z+offset.z};
    gears_vr::Matrix orientation{};
    // UE component matrices store local axes as rows. ue_matrix stores those
    // same axes as columns, so compose each measured magazine axis explicitly.
    for(unsigned world=0;world<3;++world)
        for(unsigned axis=0;axis<3;++axis)
            for(unsigned hand_axis=0;hand_axis<3;++hand_axis)
                orientation[world][axis]+=
                    hand[world][hand_axis]*lancer_magazine_in_left_axes[axis][hand_axis];
    magazine.rotation=gears_vr::ue_angles(orientation);
    return gears_vr::finite(magazine.position)&&
        std::abs(magazine.position.x)<1e7&&std::abs(magazine.position.y)<1e7&&
        std::abs(magazine.position.z)<1e7&&
        std::isfinite(magazine.rotation.pitch)&&std::isfinite(magazine.rotation.yaw)&&
        std::isfinite(magazine.rotation.roll);
}

// Inverse authored WRIST mount. Also used by cosmetic hand fitting after an
// actual magazine lease; this does not grant input/contact or calibrate grips.
inline bool lancer_left_for_magazine(gears_vr::Viewpoint const& magazine,
                                     gears_vr::Viewpoint& left,double model_scale=1.) noexcept {
    if(!gears_vr::finite(magazine.position)||
       !std::isfinite(model_scale)||model_scale<=0||
       !std::isfinite(magazine.rotation.pitch)||
       !std::isfinite(magazine.rotation.yaw)||
       !std::isfinite(magazine.rotation.roll))return false;
    const auto target=gears_vr::ue_matrix(magazine.rotation);
    gears_vr::Matrix hand{};
    for(unsigned world=0;world<3;++world)
        for(unsigned hand_axis=0;hand_axis<3;++hand_axis)
            for(unsigned magazine_axis=0;magazine_axis<3;++magazine_axis)
                hand[world][hand_axis]+=
                    target[world][magazine_axis]*
                    lancer_magazine_in_left_axes[magazine_axis][hand_axis];
    const auto raw=lancer_magazine_in_left_origin_raw;
    const auto offset=gears_vr::transform(hand,{raw.x*model_scale,raw.y*model_scale,raw.z*model_scale});
    left.position={magazine.position.x-offset.x,magazine.position.y-offset.y,
                   magazine.position.z-offset.z};
    left.rotation=gears_vr::ue_angles(hand);
    return gears_vr::finite(left.position)&&
        std::isfinite(left.rotation.pitch)&&std::isfinite(left.rotation.yaw)&&
        std::isfinite(left.rotation.roll);
}
}
