#pragma once
#include "ngx_stereo_layout.h"
namespace gears_ngx_create {
// Actual completed native family Create fields. This is a CPU parameter source,
// not proof of allocation, production, safe GPU retirement or SR admission.
struct NativeCreateSource {
    uint64_t revision{},world{};uintptr_t handle{},parameters{};
    std::array<uint32_t,7> fields{}; // Width,Height,OutWidth,OutHeight,quality,flags,subrect byte.
    bool matches(NativeCreateSource const& other)const{
        return revision&&world&&handle&&parameters&&revision==other.revision&&world==other.world&&
            handle==other.handle&&parameters==other.parameters&&fields==other.fields;
    }
    bool create_layout(gears_ngx_stereo::Layout& out)const{
        out={};if(!revision||!world||!parameters||fields[0]%2||fields[2]%2)return false;
        out.full_output={fields[2],fields[3]};out.input={fields[0]/2,fields[1]};
        out.stride=fields[2]/2;out.eye_output={out.stride,fields[3]};out.input_base[1]={out.stride,0};
        // Four physical extents intentionally remain unknown until first Eval.
        // A nonzero public handle is not required before native Create returns.
        if(!gears_ngx_stereo::valid_create_layout(out)){out={};return false;}return true;
    }
    bool split_input(uint32_t output_width,uint32_t output_height,uint32_t& width,uint32_t& height)const{
        if(!revision||!world||!handle||!parameters||fields[2]!=output_width||fields[3]!=output_height||
           output_width<128||output_width>8192||output_width%2||output_height<64||output_height>8192||
           fields[0]<128||fields[0]%2||fields[0]>output_width||fields[1]<64||fields[1]>output_height)return false;
        width=fields[0]/2;height=fields[1];return true;
    }
};
}
