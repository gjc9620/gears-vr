#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

#include "weapon_mesh_follow.h"
#include "hand_wrist_calibration.h"

// Boltok authored_default main-grip PRIVATE visual preview task301.
// Position retargeted by original fixed-wrist surface comparison. The Boltok
// baseline uses neutral Aim axes; its current Aim/Grip relation is applied by
// the existing follower. Native retains the same wrist-local palm marker.
// Original capture file is preserved; this preview is not fit-accepted.
// Source artifact SHA256:
// a68a9b053bfde2f7133cca16e0dd2413f44ab90e779ec1a5016a90bc704fb4f2
// This is authored data, not a physical capture and not evidence of visible
// or headset-accepted fit.
namespace gears_boltok_main_grip {

inline bool bind(gears_weapon::WeaponMeshFollow& out,
    gears_vr::Viewpoint const& right,std::array<float,16> const& matrix,
    double units,gears_hand_wrist::Calibration const& native) noexcept {
    if(!native.known||!std::isfinite(units)||units<10||units>500||out.bound())
        return false;

    const auto& t=native.grip_to_wrist_metres;
    const auto& q=native.grip_to_wrist_rotation;
    const double qn2=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(!std::isfinite(t.x)||!std::isfinite(t.y)||!std::isfinite(t.z)||
       !std::isfinite(q.x)||!std::isfinite(q.y)||!std::isfinite(q.z)||
       !std::isfinite(q.w)||!std::isfinite(qn2)||std::abs(qn2-1.0)>1e-4)
        return false;

    double scale=0.0;
    for(unsigned column=0;column<3;++column){
        const double x=matrix[column*4+0];
        const double y=matrix[column*4+1];
        const double z=matrix[column*4+2];
        const double length=std::hypot(std::hypot(x,y),z);
        if(!std::isfinite(length)||length<=0.0)return false;
        scale+=length;
    }
    scale/=3.0;
    if(!std::isfinite(scale)||scale<=0.0)return false;

    const auto rotation=gears_vr::rotation_matrix(q);
    const auto local=gears_vr::transform(gears_vr::transpose(rotation),t);
    const gears_vr::Vector marker_wrist{
        -local.x*units/scale,-local.y*units/scale,-local.z*units/scale};
    if(!std::isfinite(marker_wrist.x)||!std::isfinite(marker_wrist.y)||
       !std::isfinite(marker_wrist.z))return false;

    const gears_vr::Vector source_wrist_raw{
        -0.54326629638671875,4.7744400024414055,-1.2979869842529301};
    const gears_vr::Matrix wrist_in_mesh{
        {{0,-1,0},{1,0,0},{0,0,1}}};
    const auto offset=gears_vr::transform(wrist_in_mesh,marker_wrist);
    const gears_vr::Vector palm{
        source_wrist_raw.x+offset.x,
        source_wrist_raw.y+offset.y,
        source_wrist_raw.z+offset.z};
    if(!std::isfinite(palm.x)||!std::isfinite(palm.y)||!std::isfinite(palm.z))
        return false;

    gears_weapon::WeaponMeshFollow candidate{};
    const gears_vr::Matrix neutral{{{1,0,0},{0,1,0},{0,0,1}}};
    if(!candidate.bind_mount(right,matrix,palm,wrist_in_mesh,neutral))
        return false;
    out=candidate;
    return true;
}

} // namespace gears_boltok_main_grip
