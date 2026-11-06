#pragma once
#include <array>
#include <cmath>
#include <cstdint>

// Recover only the observed zero-sized final presentation, using a separately
// verified current output and input of identical extent. Valid framing and
// intentional crop/aspect-ratio choices are passed through unchanged.
namespace gears_startup_menu_geometry {
struct Input {
    std::uintptr_t caller{};
    std::array<float,8> args{};
    std::array<unsigned,4> dimensions{};
    unsigned width{},height{};
    bool code{},pending{},source{},current_buffer{},rectangles{};
};
struct Plan {bool valid{};std::array<float,8> args{};std::array<unsigned,4> dimensions{};};
inline Plan plan(Input const& in) {
    Plan out{};out.args=in.args;out.dimensions=in.dimensions;
    if(in.caller!=0x9a7377||!in.code||!in.pending||!in.source||!in.current_buffer||!in.rectangles||
       in.width<640||in.height<360||in.width>8192||in.height>8192||
       in.dimensions!=std::array<unsigned,4>{0,0,in.width,in.height})return out;
    for(unsigned i=0;i<4;++i)if(in.args[i]!=0)return out;
    for(unsigned i=4;i<8;++i)if(!std::isnan(in.args[i]))return out;
    out.valid=true;out.args={0,0,float(in.width),float(in.height),0,0,float(in.width),float(in.height)};
    out.dimensions={in.width,in.height,in.width,in.height};return out;
}
}
