#pragma once
#include "ngx_native_optimal_settings.h"
namespace gears_ngx_create {
// Actual completed native family Create fields. This is a CPU parameter source,
// not proof of allocation, production, safe GPU retirement or SR admission.
struct NativeCreateSource {
    uint64_t revision{},world{};uintptr_t handle{},parameters{};
    std::array<uint32_t,7> fields{}; // Width,Height,OutWidth,OutHeight,quality,flags,subrect byte.
    gears_ngx_stereo::NativeOptimalSettings eye_optimal{};
    bool matches(NativeCreateSource const& other)const{
        return revision&&world&&handle&&parameters&&revision==other.revision&&world==other.world&&
            handle==other.handle&&parameters==other.parameters&&fields==other.fields&&gears_ngx_stereo::same_optimal(eye_optimal,other.eye_optimal);
    }
    bool create_layout(gears_ngx_stereo::Layout& out)const{
        out={};if(!revision||!world||!parameters||fields[2]%2||fields[0]<128||fields[0]>fields[2]||fields[1]<64||fields[1]>fields[3])return false;
        out.full_output={fields[2],fields[3]};
        out.stride=fields[2]/2;out.eye_output={out.stride,fields[3]};out.input_base[1]={out.stride,0};
        if(eye_optimal.called){
            if(!gears_ngx_stereo::same(eye_optimal.output,out.eye_output)||eye_optimal.quality!=fields[4]||!eye_optimal.supports(eye_optimal.optimal)){out={};return false;}
            out.input=eye_optimal.optimal;
        }else{if(fields[0]%2){out={};return false;}out.input={fields[0]/2,fields[1]};}
        // Four physical extents intentionally remain unknown until first Eval.
        // A nonzero public handle is not required before native Create returns.
        if(!gears_ngx_stereo::valid_create_layout(out)){out={};return false;}return true;
    }
    bool split_input(uint32_t output_width,uint32_t output_height,uint32_t& width,uint32_t& height)const{
        if(!revision||!world||!handle||!parameters||fields[2]!=output_width||fields[3]!=output_height||
           output_width<128||output_width>8192||output_width%2||output_height<64||output_height>8192||
           fields[0]<128||fields[0]>output_width||fields[1]<64||fields[1]>output_height)return false;
        gears_ngx_stereo::Layout layout;if(!create_layout(layout))return false;
        width=layout.input.width;height=layout.input.height;return true;
    }
};
}
