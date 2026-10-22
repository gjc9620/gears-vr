#pragma once
#include "pose_math.h"

namespace gears_weapon {
// Measured in one original retail Lancer reload's stable hand-held interval.
// This is a left-hand BONE mount, used as a private controller-grip candidate
// until the Quest grip-to-palm offset is measured in-headset.
inline bool lancer_magazine_from_left(gears_vr::Viewpoint const& left,
                                      gears_vr::Viewpoint& magazine) noexcept {
    if(!gears_vr::finite(left.position)||
       !std::isfinite(left.rotation.pitch)||!std::isfinite(left.rotation.yaw)||
       !std::isfinite(left.rotation.roll))return false;
    constexpr gears_vr::Vector origin{19.03603543,-5.56617749,.13964627};
    constexpr double axes[3][3]={
        {.03740158,.13477868,-.99016975},
        {-.03192186,-.99019607,-.13598739},
        {-.99879037,.03669508,-.03273385}};
    const auto hand=gears_vr::ue_matrix(left.rotation);
    const auto offset=gears_vr::transform(hand,origin);
    magazine.position={left.position.x+offset.x,left.position.y+offset.y,
                       left.position.z+offset.z};
    gears_vr::Matrix orientation{};
    // UE component matrices store local axes as rows. ue_matrix stores those
    // same axes as columns, so compose each measured magazine axis explicitly.
    for(unsigned world=0;world<3;++world)
        for(unsigned axis=0;axis<3;++axis)
            for(unsigned hand_axis=0;hand_axis<3;++hand_axis)
                orientation[world][axis]+=hand[world][hand_axis]*axes[axis][hand_axis];
    magazine.rotation=gears_vr::ue_angles(orientation);
    return gears_vr::finite(magazine.position)&&
        std::abs(magazine.position.x)<1e7&&std::abs(magazine.position.y)<1e7&&
        std::abs(magazine.position.z)<1e7&&
        std::isfinite(magazine.rotation.pitch)&&std::isfinite(magazine.rotation.yaw)&&
        std::isfinite(magazine.rotation.roll);
}
}
