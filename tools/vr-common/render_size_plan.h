#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace gears_vr {
// One immutable engine/XR geometry per session. Multipliers apply to each
// dimension, not pixel area. The SBS contract currently caps its width at 8192.
struct RenderSizeLimits {
    std::array<unsigned,2> recommended_width{},recommended_height{},maximum_width{},maximum_height{};
    unsigned system_width{},system_height{};
};
struct RenderSizePlan {
    unsigned recommended_width{},recommended_height{},eye_width{},eye_height{},source_width{},source_height{};
    double scale{};
};
enum class RenderSizeError {none,scale,recommendation,runtime_limit,engine_limit};
inline bool render_size_valid(RenderSizePlan const& p) {
    return std::isfinite(p.scale)&&p.scale>=.25&&p.scale<=2.&&p.recommended_width&&p.recommended_height&&
        p.eye_width>=64&&p.eye_width<=4096&&p.eye_height>=64&&p.eye_height<=8192&&
        p.source_width==p.eye_width*2&&p.source_height==p.eye_height&&
        double(p.eye_width)==std::ceil(double(p.recommended_width)*p.scale-1e-9)&&
        double(p.eye_height)==std::ceil(double(p.recommended_height)*p.scale-1e-9);
}
inline RenderSizeError make_render_size_plan(RenderSizeLimits const& limits,double scale,RenderSizePlan& out) {
    if(!std::isfinite(scale)||scale<.25||scale>2.)return RenderSizeError::scale;
    for(unsigned eye=0;eye<2;++eye)
        if(!limits.recommended_width[eye]||!limits.recommended_height[eye]||
           limits.recommended_width[eye]>limits.maximum_width[eye]||
           limits.recommended_height[eye]>limits.maximum_height[eye])return RenderSizeError::recommendation;
    RenderSizePlan p{};p.scale=scale;
    p.recommended_width=(std::max)(limits.recommended_width[0],limits.recommended_width[1]);
    p.recommended_height=(std::max)(limits.recommended_height[0],limits.recommended_height[1]);
    const double w=std::ceil(double(p.recommended_width)*scale-1e-9),h=std::ceil(double(p.recommended_height)*scale-1e-9);
    if(w<64||w>4096||h<64||h>8192)return RenderSizeError::engine_limit;
    p.eye_width=unsigned(w);p.eye_height=unsigned(h);p.source_width=p.eye_width*2;p.source_height=p.eye_height;
    if(p.eye_width>limits.system_width||p.eye_height>limits.system_height)return RenderSizeError::runtime_limit;
    for(unsigned eye=0;eye<2;++eye)
        if(p.eye_width>limits.maximum_width[eye]||p.eye_height>limits.maximum_height[eye])return RenderSizeError::runtime_limit;
    out=p;return RenderSizeError::none;
}
inline char const* render_size_error(RenderSizeError error) {
    switch(error){case RenderSizeError::none:return "none";case RenderSizeError::scale:return "render_scale must be between 0.25 and 2.0";
    case RenderSizeError::recommendation:return "invalid runtime recommended eye size";
    case RenderSizeError::runtime_limit:return "requested eye size exceeds runtime limits";
    default:return "requested engine SBS size exceeds 8192 pixels or is smaller than 64 pixels per eye; reduce render_scale";}
}
} // namespace gears_vr
