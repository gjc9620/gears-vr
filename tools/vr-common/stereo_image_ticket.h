#pragma once
#include "stereo_frame.h"
#include <type_traits>

namespace gears_vr {
struct StereoImageRect {std::int32_t x{},y{},width{},height{};};
// Supplied by the native family owner, never synthesized from a latest pose.
// id==0 means the legacy quad image has no stereo provenance.
struct StereoImageTicket {
    std::uint64_t id{};
    StereoSample sample{};
    StereoImageRect rect[2]{};
};
inline bool stereo_image_ticket_valid(StereoImageTicket const& t,unsigned width,unsigned height,bool allow_sim=false) {
    auto const& s=t.sample;
    if(!t.id||!s.session||!s.sequence||!s.space_epoch||s.received_ns<=0||
       !stereo_render_active(s)||!stereo_tracking_valid(s,allow_sim)||
       !stereo_geometry_valid(s)||!width||!height||width>8192||height>8192)return false;
    auto a=t.rect[0],b=t.rect[1];
    return a.x==0&&a.y==0&&b.y==0&&a.width>0&&b.width>0&&
        a.height==static_cast<std::int32_t>(height)&&b.height==a.height&&b.x==a.width&&
        std::int64_t(a.width)+b.width==width;
}
inline bool stereo_image_ticket_empty(StereoImageTicket const& t) {
    if(t.id||!stereo_same(t.sample,StereoSample{}))return false;
    for(auto r:t.rect)if(r.x||r.y||r.width||r.height)return false;
    return true;
}
static_assert(std::is_trivially_copyable<StereoImageTicket>::value,"Shared ticket must remain a plain value");
}
