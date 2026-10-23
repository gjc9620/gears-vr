#pragma once
#include "pose_math.h"
#include <array>
#include <cstdint>
#include <cmath>

namespace gears_weapon {
// Pure rotation inverse for an authenticated asset-local proper rotation O.
// UE row axes compose L=O*C; pose_math columns therefore compose L=C*O.
// desired_model is unquantized radians. RotOrigin is Pitch,Yaw,Roll in UE
// integer units (65536/turn), wrapped modulo65536 to [-32768,32767].
// Callers quantize only the final returned component Rotator, once per axis.
// This supplies neither translation/Origin handling nor write authorization.
inline bool model_to_component_rotation(gears_vr::Rotator const& desired_model,
        std::array<std::int32_t,3> const& mesh_rot_origin,gears_vr::Rotator& out) noexcept {
    if(!std::isfinite(desired_model.pitch)||!std::isfinite(desired_model.yaw)||
       !std::isfinite(desired_model.roll))return false;
    const auto radians=[](std::int32_t raw) noexcept {
        // Conversion to unsigned is defined modulo2^32, including INT32_MIN;
        // its low16 bits are the UE phase, with no signed overflow or cast back.
        const auto phase=static_cast<std::uint32_t>(raw)&0xffffu;
        const double units=phase<32768u?double(phase):double(phase)-65536.;
        return units*(6.283185307179586/65536.);
    };
    const gears_vr::Rotator origin{radians(mesh_rot_origin[0]),radians(mesh_rot_origin[1]),radians(mesh_rot_origin[2])};
    const auto control=gears_vr::multiply(gears_vr::ue_matrix(desired_model),
        gears_vr::transpose(gears_vr::ue_matrix(origin)));
    for(auto const& row:control)for(double value:row)if(!std::isfinite(value))return false;
    const auto candidate=gears_vr::ue_angles(control);
    if(!std::isfinite(candidate.pitch)||!std::isfinite(candidate.yaw)||!std::isfinite(candidate.roll))return false;
    out=candidate;return true;
}
} // namespace gears_weapon
