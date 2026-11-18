#pragma once
#include "weapon_support_volume.h"
#include "weapon_lancer_feed_anchor.h"

namespace gears_lancer_authored_support {
// Current certified mesh in the neutral right-controller UE frame. This is
// static authoring geometry, not another tracked world pose or input source.
inline bool from_asset(gears_lancer_authored_volume::Volume const& volume,
    std::array<float,16> const& gun,double units,gears_vr::Vector support_in_mount,
    gears_weapon::SupportRegion& out) noexcept {
    out={};gears_weapon::LancerFeedWorld check{};
    if(!gears_lancer_authored_volume::valid(volume)||!gears_vr::finite(support_in_mount)||
       !std::isfinite(units)||units<10||units>500||!gears_weapon::lancer_feed_world(gun,check))return false;
    gears_vr::Matrix rotation{};
    const double size=std::hypot(std::hypot(double(gun[0]),double(gun[1])),double(gun[2]));
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)rotation[j][i]=gun[i*4+j]/size;
    constexpr gears_vr::Matrix asset_to_object{{{-1,0,0},{0,0,1},{0,-1,0}}};
    constexpr gears_vr::Matrix ue_to_mount{{{0,1,0},{0,0,1},{-1,0,0}}};
    gears_weapon::SupportRegion region{};region.enabled=true;region.volume=volume;
    region.object_from_mount=gears_vr::multiply(gears_vr::multiply(asset_to_object,
        gears_vr::transpose(rotation)),gears_vr::transpose(ue_to_mount));
    region.scale=units/(100.*size);
    const gears_vr::Vector translation{gun[13]/units,gun[14]/units,-gun[12]/units};
    const auto delta=gears_vr::transform(region.object_from_mount,{support_in_mount.x-translation.x,
        support_in_mount.y-translation.y,support_in_mount.z-translation.z});
    region.origin_in_object={delta.x*region.scale,delta.y*region.scale,delta.z*region.scale};
    if(!region.valid())return false;out=region;return true;
}
}
