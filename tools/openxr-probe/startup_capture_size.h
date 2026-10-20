#pragma once
#include <algorithm>
#include <cstdint>
namespace startup_capture_size {
inline bool fit(unsigned width,unsigned height,unsigned& output_width,unsigned& output_height) {
    if(!width||!height)return false;
    output_width=std::min(1920u,width);
    output_height=std::max(1u,unsigned(std::uint64_t(height)*output_width/width));
    return output_height<=2160;
}
}
