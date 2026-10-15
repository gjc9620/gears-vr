#pragma once
#include <cmath>
#include <cstdint>

namespace gears_menu_panel {
struct Plan {
    std::int32_t x{},y{},width{},height{};
    float metres_width{},metres_height{};
};
inline bool make_plan(unsigned source_width,unsigned source_height,Plan& out) {
    if(!source_width||source_width%2||source_width>8192||!source_height||source_height>8192)return false;
    Plan candidate{};
    candidate.width=std::int32_t(source_width/2);candidate.height=std::int32_t(source_height);
    candidate.metres_width=1.6f;
    // Ordinary Canvas replay compresses the full UI into each half-width eye.
    // Restore source aspect on the shared panel, not the eye texture aspect.
    candidate.metres_height=float(double(candidate.metres_width)*source_height/source_width);
    if(candidate.width<=0||!std::isfinite(candidate.metres_width)||!std::isfinite(candidate.metres_height)||
       candidate.metres_width<=0||candidate.metres_height<=0)return false;
    out=candidate;return true;
}
}
