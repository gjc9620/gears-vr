#pragma once
#include "ui_hud_plan.h"
#include "ui_reticle_direction_plan.h"

namespace gears_ui_aim {
// Keep the readable HUD size, but anchor its center to the SAME frozen weapon
// target/ray as the reticle. No live head query or screen-center recentering.
// The HUD remains a readable billboard; this does not claim a rigid 3D plane.
inline bool plan(gears_viewport::DrawState const& saved,unsigned width,unsigned height,
    gears_vr::StereoSample const& sample,gears_vr::StereoTicketGeometry const& geometry,
    gears_viewport::OverlayPlan& out,gears_ui_hud::Parameters parameters={},
    gears_ui_hud::Diagnostic* diagnostic=nullptr){
    if(!geometry.aiming)return gears_ui_hud::plan(saved,width,height,sample,out,parameters,diagnostic);
    gears_viewport::OverlayPlan panel{},anchor{};bool suppressed=false;
    if(!gears_viewport::overlay_plan(saved,width,height).valid)return false;
    auto shape=saved;shape.scissor={0,0,float(width),float(height)};
    if(!gears_ui_hud::plan(shape,width,height,sample,panel,parameters,diagnostic))return false;
    if(!gears_ui_reticle::display_plan(saved,width,height,geometry,anchor,parameters.source_aspect,&suppressed)){
        for(unsigned eye=0;eye<2;++eye)panel.scissors[eye]={float(eye*width/2),0,0,0};
        out=panel;return true;
    }
    const double l=(std::max)(0.,double(saved.scissor.x)/width);
    const double r=(std::min)(1.,(double(saved.scissor.x)+saved.scissor.width)/width);
    const double t=(std::max)(0.,double(saved.scissor.y)/height);
    const double b=(std::min)(1.,(double(saved.scissor.y)+saved.scissor.height)/height);
    for(unsigned eye=0;eye<2;++eye){
        auto& v=panel.viewports[eye];auto const& a=anchor.viewports[eye];
        const double origin=double(eye)*width/2;
        v.x=float(double(a.x)+a.width/2.-v.width/2.);
        v.y=float(double(a.y)+a.height/2.-v.height/2.);
        bool legal=true;
        for(double n:{double(v.x),double(v.y),double(v.x)+v.width,double(v.y)+v.height})
            legal&=std::isfinite(n)&&n>=-32768&&n<=32767;
        if(suppressed||!legal){
            v=gears_viewport::overlay_plan(saved,width,height).viewports[eye];
            panel.scissors[eye]={float(origin),0,0,0};continue;
        }
        const double cl=(std::max)(origin,double(v.x)+v.width*l);
        const double cr=(std::min)(origin+width/2.,double(v.x)+v.width*r);
        const double ct=(std::max)(0.,double(v.y)+v.height*t);
        const double cb=(std::min)(double(height),double(v.y)+v.height*b);
        if(!(cr>cl&&cb>ct)){panel.scissors[eye]={float(origin),0,0,0};continue;}
        panel.scissors[eye]={float(std::floor(cl)),float(std::floor(ct)),
            float(std::ceil(cr)-std::floor(cl)),float(std::ceil(cb)-std::floor(ct))};
    }
    out=panel;return true;
}
}
