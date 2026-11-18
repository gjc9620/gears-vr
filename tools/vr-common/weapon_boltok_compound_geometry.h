#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "weapon_part_geometry.h"
#include "weapon_boltok_visual_geometry.h"

namespace gears_boltok_compound_geometry {
namespace geom=gears_weapon::part_geometry;
using Result=geom::Result;

template<std::size_t N>
Result build_typed(geom::Prepared const& cylinder,
    geom::Prepared const& round,gears_weapon::BoltokVisualGeometry const& g,
    unsigned mask,geom::Prepared& out) {
    std::array<geom::Prepared,N> parts{};
    parts[0]=cylinder;
    std::size_t part_count=1;
    for(unsigned slot=0;slot<6;++slot){
        if(!(mask&(1u<<slot)))continue;
        if(part_count>=N)return Result::invalid_input;
        gears_vr::Pose offset{};
        if(!g.round(gears_vr::Pose{},slot,offset))
            return Result::invalid_input;
        const auto& q=offset.orientation;
        if(q.x!=0.0||q.y!=0.0||q.z!=0.0||q.w!=1.0)
            return Result::invalid_conversion;
        if(!std::isfinite(offset.position.x)||
           !std::isfinite(offset.position.y)||
           !std::isfinite(offset.position.z))
            return Result::invalid_vertex;
        auto& part=parts[part_count++];
        part=round;
        for(auto& vertex:part.vertices){
            vertex.position[0]=static_cast<float>(
                double(vertex.position[0])+offset.position.x);
            vertex.position[1]=static_cast<float>(
                double(vertex.position[1])+offset.position.y);
            vertex.position[2]=static_cast<float>(
                double(vertex.position[2])+offset.position.z);
        }
    }
    if(part_count!=N)return Result::invalid_input;
    geom::Prepared merged{};
    const auto result=geom::merge_rigid(parts,merged);
    if(result!=Result::ready)return result;
    out=std::move(merged);
    return Result::ready;
}

inline Result build(geom::Prepared const& cylinder,
    geom::Prepared const& round,gears_weapon::BoltokVisualGeometry const& g,
    unsigned mask,geom::Prepared& out) noexcept try {
    if(mask>63||!g.valid()||&out==&cylinder||&out==&round)
        return Result::invalid_input;
    if(cylinder.vertices.empty()||cylinder.indices.empty()||
       cylinder.sections.empty()||round.vertices.empty()||
       round.indices.empty()||round.sections.empty())
        return Result::invalid_input;
    if(cylinder.calibration_generation!=g.calibration||
       round.calibration_generation!=g.calibration)
        return Result::invalid_conversion;
    if(cylinder.winding_reversed!=round.winding_reversed)
        return Result::invalid_input;

    unsigned count=1;for(unsigned bits=mask;bits;bits>>=1)count+=bits&1u;
    switch(count){
    case 1u:return build_typed<1>(cylinder,round,g,mask,out);
    case 2u:return build_typed<2>(cylinder,round,g,mask,out);
    case 3u:return build_typed<3>(cylinder,round,g,mask,out);
    case 4u:return build_typed<4>(cylinder,round,g,mask,out);
    case 5u:return build_typed<5>(cylinder,round,g,mask,out);
    case 6u:return build_typed<6>(cylinder,round,g,mask,out);
    case 7u:return build_typed<7>(cylinder,round,g,mask,out);
    default:return Result::invalid_input;
    }
} catch(...) {
    return Result::invalid_input;
}

} // namespace gears_boltok_compound_geometry
