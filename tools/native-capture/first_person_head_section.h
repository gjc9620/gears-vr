#pragma once
#include <array>
#include <cmath>
#include <cstdint>

// Original Marcus LOD0 material2/chunk2 only. The normal mesh palette must
// retain Spine3 for the torso; this separate shader palette can collapse it
// with the already-hidden neck without changing any arm or body draw.
namespace gears_first_person_head_section {
inline constexpr unsigned chunk=2,material=2,spine_slot=11,neck_slot=12;
inline constexpr std::array<std::uint16_t,21> bone_map{{80,86,55,73,81,88,71,72,77,76,59,5,54,65,67,69,64,62,70,60,57}};
struct Matrix {std::array<float,12> values{};};
using Palette=std::array<Matrix,bone_map.size()>;
static_assert(sizeof(Matrix)==48);
inline bool collapsed(Matrix const& m) noexcept {
    for(unsigned i=0;i<12;++i){
        if(!std::isfinite(m.values[i]))return false;
        if(i!=3&&i!=7&&i!=11&&m.values[i]!=0.f)return false;
    }
    return true;
}
inline bool plan(Palette const& before,Palette& after) noexcept {
    if(&before==&after||!collapsed(before[neck_slot]))return false;
    for(unsigned i=0;i<before.size();++i){
        for(float v:before[i].values)if(!std::isfinite(v))return false;
        if(i!=spine_slot&&before[i].values!=before[neck_slot].values)return false;
    }
    after=before;after[spine_slot]=before[neck_slot];return true;
}
}
