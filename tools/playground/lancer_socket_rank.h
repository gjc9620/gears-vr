#pragma once
#include "../vr-common/weapon_lancer_authored_volume.h"
// Cosmetic candidate ranking only. Callers separately prove current contact,
// Snap limits, identity and ownership; this score never authorizes a grab.
namespace gears_lancer_socket_rank {
inline bool distance(gears_lancer_authored_volume::Volume const& region,
    std::array<float,16> const& mesh,gears_vr::Vector physical,double units,
    double& out,std::array<double,16> const* delta=nullptr) noexcept {
    if(!gears_lancer_authored_volume::valid(region)||!gears_vr::finite(physical)||
       !std::isfinite(units)||units<=0)return false;
    const auto c=region.local.position;
    gears_vr::Vector p{-100*c.x,-100*c.z,100*c.y};
    if(delta){auto const& d=*delta;for(double n:d)if(!std::isfinite(n))return false;
        p={p.x*d[0]+p.y*d[4]+p.z*d[8]+d[12],
           p.x*d[1]+p.y*d[5]+p.z*d[9]+d[13],
           p.x*d[2]+p.y*d[6]+p.z*d[10]+d[14]};}
    for(float n:mesh)if(!std::isfinite(n))return false;
    const gears_vr::Vector world{p.x*mesh[0]+p.y*mesh[4]+p.z*mesh[8]+mesh[12],
        p.x*mesh[1]+p.y*mesh[5]+p.z*mesh[9]+mesh[13],
        p.x*mesh[2]+p.y*mesh[6]+p.z*mesh[10]+mesh[14]};
    const double result=std::hypot(std::hypot(world.x-physical.x,world.y-physical.y),world.z-physical.z)/units;
    if(!std::isfinite(result))return false;out=result;return true;
}
}
