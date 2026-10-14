#pragma once
#include "stereo_frame.h"

namespace gears_vr {
struct UiEyeProjection {
    double x{},y{},w{};
    bool in_viewport_xy{};
};
struct UiStereoProjection {std::array<UiEyeProjection,2> eyes{};};

// Project the SAME caller-certified world point through already frozen eyes.
// Row vectors, native +Z forward; continuous top-left pixel coordinates with
// inclusive viewport edges [0,width] x [0,height]. The XY flag is not depth
// clipping, occlusion or visibility. No pose read, world-point provenance,
// physical scale, HUD distance or render-ticket association is supplied here.
// Outside-XY coordinates are a successful projection with a false flag.
// Any invalid matrix/point/dimension, nonfinite intermediate, or nonpositive
// eye depth/W rejects BOTH eyes and leaves out unchanged.
inline bool project_ui_anchor(StereoMatrices const& frozen,Vector world,
    unsigned image_width,unsigned image_height,UiStereoProjection& out) {
    if(!image_width||!image_height||!finite(world))return false;
    UiStereoProjection candidate{};
    const double point[]{world.x,world.y,world.z,1};
    for(unsigned eye=0;eye<2;++eye){
        auto const& view=frozen.view[eye];auto const& projection=frozen.projection[eye];
        if(!stereo_rigid(view)||!stereo_projection_supported(projection))return false;
        double camera[4]{},clip[4]{};
        for(unsigned column=0;column<4;++column){
            for(unsigned row=0;row<4;++row)camera[column]+=point[row]*view[4*row+column];
            if(!std::isfinite(camera[column]))return false;
        }
        if(camera[2]<=0)return false;
        for(unsigned column=0;column<4;++column){
            for(unsigned row=0;row<4;++row)clip[column]+=camera[row]*projection[4*row+column];
            if(!std::isfinite(clip[column]))return false;
        }
        if(clip[3]<=0)return false;
        const double nx=clip[0]/clip[3],ny=clip[1]/clip[3];
        auto& result=candidate.eyes[eye];result.x=(nx+1)*.5*image_width;
        result.y=(1-ny)*.5*image_height;result.w=clip[3];
        if(!std::isfinite(nx)||!std::isfinite(ny)||!std::isfinite(result.x)||!std::isfinite(result.y))return false;
        result.in_viewport_xy=nx>=-1&&nx<=1&&ny>=-1&&ny<=1;
    }
    out=candidate;return true;
}
}
