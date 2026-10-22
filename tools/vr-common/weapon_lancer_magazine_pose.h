#pragma once
#include "pose_math.h"

namespace gears_weapon {
// Measured in one original retail Lancer reload's stable hand-held interval.
// This is a left-hand BONE mount, used as a private controller-grip candidate
// until the Quest grip-to-palm offset is measured in-headset.
inline constexpr gears_vr::Vector lancer_magazine_in_left_origin_raw{
    19.03603543,-5.56617749,.13964627};
inline constexpr double lancer_magazine_in_left_axes[3][3]={
    {.03740158,.13477868,-.99016975},
    {-.03192186,-.99019607,-.13598739},
    {-.99879037,.03669508,-.03273385}};
inline bool lancer_magazine_from_left(gears_vr::Viewpoint const& left,
                                      gears_vr::Viewpoint& magazine) noexcept {
    if(!gears_vr::finite(left.position)||
       !std::isfinite(left.rotation.pitch)||!std::isfinite(left.rotation.yaw)||
       !std::isfinite(left.rotation.roll))return false;
    const auto hand=gears_vr::ue_matrix(left.rotation);
    const auto offset=gears_vr::transform(hand,lancer_magazine_in_left_origin_raw);
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

// Inverse mount is used only to synthesize an aligned private retail test
// path. It is not an automatic hand snap or a calibration for Quest grips.
inline bool lancer_left_for_magazine(gears_vr::Viewpoint const& magazine,
                                     gears_vr::Viewpoint& left) noexcept {
    if(!gears_vr::finite(magazine.position)||
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
    const auto offset=gears_vr::transform(hand,lancer_magazine_in_left_origin_raw);
    left.position={magazine.position.x-offset.x,magazine.position.y-offset.y,
                   magazine.position.z-offset.z};
    left.rotation=gears_vr::ue_angles(hand);
    return gears_vr::finite(left.position)&&
        std::isfinite(left.rotation.pitch)&&std::isfinite(left.rotation.yaw)&&
        std::isfinite(left.rotation.roll);
}
}
