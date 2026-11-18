#pragma once
#include "weapon_part_geometry.h"
namespace gears_weapon::boomshot_carrier_geometry {
// Original Weap_Boomer_base.Notify_AmmoGrab:1 for Pawn_GeistBoomer,
//0.75 for the player. Apply that explicit model scale AFTER real-unit
// conversion. Never falsify the live units-per-metre calibration to scale it.
inline constexpr float original_scale(bool geist_boomer) noexcept{return geist_boomer?1.f:.75f;}
inline bool apply_original_scale(part_geometry::Prepared& data,bool geist_boomer) noexcept {
    if(!data.calibration_generation||data.vertices.size()!=108||data.indices.size()!=372||data.sections.size()!=1)return false;
    for(auto const& v:data.vertices)for(float x:v.position)if(!std::isfinite(x))return false;
    for(unsigned i=0;i<3;++i)if(!std::isfinite(data.bounds.centre[i])||!std::isfinite(data.bounds.extent[i])||data.bounds.extent[i]<0)return false;
    if(!std::isfinite(data.bounds.radius)||data.bounds.radius<=0)return false;
    const float scale=original_scale(geist_boomer);
    for(auto& v:data.vertices)for(auto& x:v.position)x*=scale;
    for(unsigned i=0;i<3;++i){data.bounds.centre[i]*=scale;data.bounds.extent[i]*=scale;}
    data.bounds.radius*=scale;return true;
}
}
