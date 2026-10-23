#pragma once
#include "ui_material_batch.h"
#include "ui_hud_layout.h"
#include "ui_reticle_direction_plan.h"

namespace gears_material_reticle {
enum class Reject : unsigned {none,kind,source_quad,draw_state,geometry};
struct Result {gears_viewport::OverlayPlan plan{};bool suppressed{};};
// A kind-1 Material is not an ordinary HUD panel. The sniper scope is a
// full-Canvas quad in the authenticated crosshair invocation. Certify those
// four corners before using the same frozen aim map as its separate icon.
// The caller still owns upload, target, command-list and ticket admission.
inline bool plan(unsigned kind,bool directional,gears_viewport::DrawState const& saved,
    unsigned width,unsigned height,gears_ui_material::ViewPlane const& source,
    gears_ui_material::Quad const& quad,gears_ui_layout::Matrix const& matrix,
    gears_vr::StereoTicketGeometry const* geometry,Result& out,Reject* rejection=nullptr){
    auto fail=[&](Reject reason){if(rejection)*rejection=reason;return false;};
    if(kind!=1)return fail(Reject::kind);
    if(!std::isfinite(source.source_width)||!std::isfinite(source.source_height)||
       source.source_width<=0||source.source_height<=0||source.source_width>16384||source.source_height>16384||
       !std::isfinite(source.aspect)||source.aspect<=0||
       std::abs(double(source.aspect)-double(source.source_width)/source.source_height)>1e-6)
        return fail(Reject::source_quad);
    unsigned corners=0;
    for(auto const& vertex:quad.vertices){
        float xyz[3]{};std::memcpy(xyz,vertex.data(),sizeof(xyz));
        gears_ui_layout::Bounds point{};
        if(xyz[2]!=0||!gears_ui_layout::project(point,matrix,xyz[0],xyz[1]))return fail(Reject::source_quad);
        const unsigned x=point.left>.5?1u:0u,y=point.top>.5?1u:0u;
        // Native Canvas projection may include a half-source-pixel bias.
        // This tolerance never becomes a placement offset or recenters bytes.
        if(std::abs(point.left-x)*source.source_width>1.001||
           std::abs(point.top-y)*source.source_height>1.001)return fail(Reject::source_quad);
        const unsigned bit=1u<<(x+2*y);
        if(corners&bit)return fail(Reject::source_quad);
        corners|=bit;
    }
    if(corners!=15)return fail(Reject::source_quad);
    Result result{};result.plan=gears_viewport::overlay_plan(saved,width,height);
    if(!result.plan.valid)return fail(Reject::draw_state);
    if(directional){
        if(!geometry||!gears_vr::stereo_rigid(geometry->source_view)||
           !gears_vr::stereo_projection_supported(geometry->source_projection))return fail(Reject::geometry);
        for(unsigned eye=0;eye<2;++eye)
            if(!gears_vr::stereo_rigid(geometry->eyes.view[eye])||
               !gears_vr::stereo_projection_supported(geometry->eyes.projection[eye]))return fail(Reject::geometry);
        if(geometry->finite_aim.valid){
            if(!geometry->finite_aim.world||!geometry->finite_aim.tick)return fail(Reject::geometry);
            for(float value:geometry->finite_aim.point)if(!std::isfinite(value))return fail(Reject::geometry);
        }
        if(!gears_ui_reticle::display_plan(saved,width,height,*geometry,result.plan,source.aspect,&result.suppressed)){
            // Match the icon's ordinary clipping when its aim is behind an eye.
            // Never replace the frozen target with head-forward or another ray.
            result.suppressed=true;
            for(unsigned eye=0;eye<2;++eye)result.plan.scissors[eye]={float(eye*width/2),0,0,0};
        }
    }
    out=result;if(rejection)*rejection=Reject::none;return true;
}
}
