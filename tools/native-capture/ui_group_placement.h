#pragma once
#include "gpu_viewport_stats.h"
#include <algorithm>

namespace gears_ui_group {
// One source-normalized affine recipe for an entire admitted group. Positive
// translation_y lowers every member equally; no primitive center is involved.
struct Recipe {double scale=1,translation_x=0,translation_y=0;};
inline bool valid(Recipe p){return std::isfinite(p.scale)&&p.scale>0&&
    std::isfinite(p.translation_x)&&std::isfinite(p.translation_y);}
inline bool identity(Recipe p){return p.scale==1&&p.translation_x==0&&p.translation_y==0;}
// Map content and its saved source clip together, then intersect each eye.
// Empty intersections reject rather than inventing visibility. Publication is
// all-or-nothing; the caller's output survives every failed preflight.
inline bool compose(gears_viewport::DrawState const& saved,unsigned width,unsigned height,
    gears_viewport::OverlayPlan const& panel,Recipe p,gears_viewport::OverlayPlan& out,bool recompute_identity_clip=false){
    if(!panel.valid||!valid(p)||!gears_viewport::overlay_plan(saved,width,height).valid)return false;
    // HUD non-identity policies historically recompute the source clip even
    // when their resulting affine happens to be identity.
    if(identity(p)&&!recompute_identity_clip){out=panel;return true;}
    const double l=(std::max)(0.,double(saved.scissor.x))/width;
    const double r=(std::min)(double(width),double(saved.scissor.x)+saved.scissor.width)/width;
    const double t=(std::max)(0.,double(saved.scissor.y))/height;
    const double b=(std::min)(double(height),double(saved.scissor.y)+saved.scissor.height)/height;
    gears_viewport::OverlayPlan result{};
    for(unsigned eye=0;eye<2;++eye){auto const base=panel.viewports[eye];
        auto& v=result.viewports[eye];v={float(base.x+base.width*p.translation_x),float(base.y+base.height*p.translation_y),
            float(base.width*p.scale),float(base.height*p.scale)};
        for(double value:{double(v.x),double(v.y),double(v.width),double(v.height),double(v.x)+v.width,double(v.y)+v.height})
            if(!std::isfinite(value)||value<-32768||value>32767)return false;
        if(v.width<=0||v.height<=0)return false;
        const double origin=double(eye)*width/2;
        const double cl=(std::max)(origin,double(v.x)+v.width*l),cr=(std::min)(origin+width/2.,double(v.x)+v.width*r);
        const double ct=(std::max)(0.,double(v.y)+v.height*t),cb=(std::min)(double(height),double(v.y)+v.height*b);
        if(!(cr>cl&&cb>ct))return false;
        const double sl=std::floor(cl),st=std::floor(ct),sr=std::ceil(cr),sb=std::ceil(cb);
        result.scissors[eye]={float(sl),float(st),float(sr-sl),float(sb-st)};
    }
    result.valid=true;out=result;return true;
}
}
