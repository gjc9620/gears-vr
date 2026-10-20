#pragma once
#include <algorithm>
#include <cstdint>
namespace gears_vr {
struct MirrorRectangle {unsigned x{},y{},width{},height{};};
inline MirrorRectangle mirror_rectangle(unsigned destination_width,unsigned destination_height,unsigned eye_width,unsigned eye_height) {
    if(!destination_width||!destination_height||!eye_width||!eye_height)return {};
    unsigned width=destination_width,height=destination_height;
    if(std::uint64_t(width)*eye_height>std::uint64_t(height)*eye_width)
        width=unsigned(std::uint64_t(height)*eye_width/eye_height);
    else height=unsigned(std::uint64_t(width)*eye_height/eye_width);
    if(!width||!height)return {};
    return {(destination_width-width)/2,(destination_height-height)/2,width,height};
}
}
