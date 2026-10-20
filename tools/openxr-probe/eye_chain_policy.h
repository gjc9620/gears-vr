#pragma once
#include "../vr-common/stereo_image_ticket.h"
#include "../vr-common/render_size_plan.h"

namespace gears_eye_chain {
struct Limits {
    unsigned system_width{},system_height{};
    std::array<unsigned,2> view_width{},view_height{},view_samples{};
    bool independent_render_size{};gears_vr::RenderSizePlan render_size{};
};
struct Plan {
    unsigned source_width{},source_height{},eye_width{},eye_height{};
    std::array<gears_vr::StereoImageRect,2> crops{};
};
inline bool make_plan(unsigned width,unsigned height,Limits const& limits,Plan& out) {
    if(!width||width%2||width>8192||!height||height>8192)return false;
    if(limits.independent_render_size&&(!gears_vr::render_size_valid(limits.render_size)||width!=limits.render_size.source_width||height!=limits.render_size.source_height))return false;
    const unsigned half=width/2;
    if(!half||half>limits.system_width||height>limits.system_height)return false;
    for(unsigned eye=0;eye<2;++eye)
        if(half>limits.view_width[eye]||height>limits.view_height[eye]||!limits.view_samples[eye])return false;
    Plan candidate{width,height,half,height,{{{0,0,int(half),int(height)},{int(half),0,int(half),int(height)}}}};
    out=candidate;return true;
}
// Crop provenance comes from this acquired ticket, never from the newest pose.
inline bool ticket_crops_match(Plan const& plan,gears_vr::StereoImageTicket const& ticket) {
    if(!ticket.id||!plan.source_width||!plan.eye_width||!plan.eye_height)return false;
    for(unsigned eye=0;eye<2;++eye) {
        auto a=plan.crops[eye],b=ticket.rect[eye];
        if(a.x!=b.x||a.y!=b.y||a.width!=b.width||a.height!=b.height)return false;
    }
    return true;
}
} // namespace gears_eye_chain
