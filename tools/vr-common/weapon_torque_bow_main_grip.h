#pragma once
#include "weapon_mesh_follow.h"
#include "weapon_torque_bow_arrow_mount.h"
// Original Bow AR_Reload key25 + Marcus115, raw mesh/reference convention.
// Original export 1db9c32b..., decoded JSON 52b2c9d5... . This fixes the
// mesh-to-wrist relation only; no natural skin or physical calibration claim.
namespace gears_torque_bow_main_grip {
namespace vr=gears_vr;namespace am=gears_torque_bow_arrow_mount;
inline constexpr vr::Vector wrist_in_mesh_raw{-2.323001146315469,4.921195001899023e-7,-2.8218982919270275e-6};
inline constexpr vr::Matrix wrist_basis_in_mesh{{
    {1.3887688273709871e-7,-.9999999999995354,-9.536743316778233e-7},
    {.9999999999998914,1.3887645854947538e-7,4.447945194448763e-7},
    {-4.447943869640565e-7,-9.536743934613376e-7,.9999999999994462}}};
inline bool uniform_scale(std::array<float,16> const& m,double& out) noexcept {
    out=0;for(float value:m)if(!std::isfinite(value))return false;
    if(std::abs(m[3])>1e-5||std::abs(m[7])>1e-5||std::abs(m[11])>1e-5||std::abs(m[15]-1)>1e-5)return false;
    double scales[3]{};vr::Vector axes[3]{};
    for(unsigned i=0;i<3;++i){scales[i]=std::hypot(std::hypot(double(m[i*4]),double(m[i*4+1])),double(m[i*4+2]));
        if(scales[i]<=.1||scales[i]>=10)return false;axes[i]={m[i*4]/scales[i],m[i*4+1]/scales[i],m[i*4+2]/scales[i]};}
    const auto dot=[](vr::Vector a,vr::Vector b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j)
        if(std::abs(scales[i]-scales[j])>1e-5*scales[0]||std::abs(dot(axes[i],axes[j]))>1e-5)return false;
    const auto& a=axes[0];const auto& b=axes[1];
    const vr::Vector cross{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
    if(std::abs(dot(cross,axes[2])-1)>1e-5)return false;
    out=(scales[0]+scales[1]+scales[2])/3;return true;
}
// The caller supplies the CURRENT admitted Native held-wrist calibration.
// The palm marker is reconstructed from that same calibration, with no
// Lancer private contact point, first-pickup offset or animated wrist read.
inline bool bind(gears_weapon::WeaponMeshFollow& out,vr::Viewpoint const& right,
    std::array<float,16> const& observed,double avatar_scale,double units,
    gears_hand_wrist::Calibration const& native_wrist) noexcept {
    double scale{};
    if(out.bound()||!uniform_scale(observed,scale)||!std::isfinite(avatar_scale)||avatar_scale<=.1||avatar_scale>=10||
       std::abs(scale-avatar_scale)>1e-5*scale||!std::isfinite(units)||units<10||units>500||
       !native_wrist.known||!vr::finite(native_wrist.grip_to_wrist_metres)||
       !am::pose_valid({{},native_wrist.grip_to_wrist_rotation}))return false;
    const auto marker=am::rotate(vr::inverse(native_wrist.grip_to_wrist_rotation),
        am::mul(native_wrist.grip_to_wrist_metres,-units/scale));
    const auto palm=am::add(wrist_in_mesh_raw,vr::transform(wrist_basis_in_mesh,marker));
    gears_weapon::WeaponMeshFollow candidate;
    if(!candidate.bind_mount(right,observed,palm,wrist_basis_in_mesh,vr::rotation_matrix(native_wrist.grip_to_wrist_rotation)))return false;
    out=candidate;return true;
}
}
