#pragma once
#include "pose_math.h"

// Authored Marcus115 surface landmarks for the existing precision/cylinder
// finger profiles. Geometry only: no contact, tracking, action or bone writer.
namespace gears_weapon_longshot_grasp {
namespace vr=gears_vr;
inline constexpr vr::Vector precision_contact_in_wrist_m{.0975,-.046,-.021};
inline constexpr vr::Vector cylinder_contact_in_wrist_m{.10,0,0};
inline constexpr double cartridge_contact_from_primer_m=.0525;
inline vr::Vector add(vr::Vector a,vr::Vector b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline vr::Vector sub(vr::Vector a,vr::Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline vr::Vector scale(vr::Vector a,double x) noexcept {return {a.x*x,a.y*x,a.z*x};}
inline double dot(vr::Vector a,vr::Vector b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool normalize(vr::Vector& a) noexcept {
    const double n=std::hypot(std::hypot(a.x,a.y),a.z);
    if(!vr::finite(a)||!std::isfinite(n)||n<1e-8)return false;a=scale(a,1/n);return true;
}
inline bool rotation_valid(vr::Quaternion q) noexcept {
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return std::isfinite(n)&&std::abs(n-1)<1e-5;
}
inline bool cartridge_mount(vr::Vector grip_to_wrist_m,vr::Quaternion grip_to_wrist,double units,vr::Pose& out) noexcept {
    if(!vr::finite(grip_to_wrist_m)||!rotation_valid(grip_to_wrist)||!std::isfinite(units)||units<10||units>500)return false;
    // The single live round's +Y is its primer-to-tip axis. Put the measured
    // precision-pad centre on its case, with the same mount used for drawing
    // AND the unsnapped physical tip. No visual-only cartridge displacement.
    const auto primer=sub(precision_contact_in_wrist_m,{cartridge_contact_from_primer_m,0,0});
    const auto p=add(grip_to_wrist_m,vr::transform(vr::rotation_matrix(grip_to_wrist),primer));
    const vr::Quaternion turn{0,0,-.7071067811865475,.7071067811865475};
    out={scale(p,units),vr::normalized(vr::multiply(grip_to_wrist,turn))};return true;
}
inline bool cylinder_grip(vr::Vector contact,vr::Vector axis,vr::Pose const& physical_grip,
    vr::Vector grip_to_wrist_m,vr::Quaternion grip_to_wrist,double units,vr::Pose& out) noexcept {
    if(!vr::finite(contact)||!normalize(axis)||!vr::finite(physical_grip.position)||
       !rotation_valid(physical_grip.orientation)||!vr::finite(grip_to_wrist_m)||
       !rotation_valid(grip_to_wrist)||!std::isfinite(units)||units<10||units>500)return false;
    const auto physical_wrist=vr::normalized(vr::multiply(physical_grip.orientation,grip_to_wrist));
    const auto row=vr::transform(vr::rotation_matrix(physical_wrist),{0,0,1});
    if(dot(row,axis)<0)axis=scale(axis,-1);
    const double cosine=std::clamp(dot(row,axis),-1.,1.);
    const auto cross=vr::Vector{row.y*axis.z-row.z*axis.y,row.z*axis.x-row.x*axis.z,row.x*axis.y-row.y*axis.x};
    const auto arc=vr::normalized(vr::Quaternion{cross.x,cross.y,cross.z,1+cosine});
    const auto wrist=vr::normalized(vr::multiply(arc,physical_wrist));
    const auto grip=vr::normalized(vr::multiply(wrist,vr::inverse(grip_to_wrist)));
    const auto palm=vr::transform(vr::rotation_matrix(wrist),scale(cylinder_contact_in_wrist_m,units));
    const auto offset=vr::transform(vr::rotation_matrix(grip),scale(grip_to_wrist_m,units));
    out={sub(sub(contact,palm),offset),grip};return true;
}
}
