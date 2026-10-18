#pragma once
#include "game_ui_material.h"
#include "ui_hud_layout.h"

namespace gears_material_overlay {
// Match the existing game Material policy while leaving target/ticket/upload
// certification to the caller. Native source geometry is never rewritten.
struct Result {
    gears_game_material::Plan kind=gears_game_material::Plan::reject;
    gears_viewport::OverlayPlan plan{};
    bool layout_considered{},layout_applied{};
};
inline bool plan(unsigned kind,bool hud_enabled,gears_viewport::DrawState const& saved,
    unsigned width,unsigned height,gears_vr::StereoSample const& sample,double source_aspect,
    gears_ui_hud::Parameters hud,gears_ui_layout::Parameters layout,
    gears_ui_material::Quad const& quad,gears_ui_layout::Matrix const& matrix,Result& out){
    Result result{};result.kind=gears_game_material::plan_kind(kind,hud_enabled);
    if(result.kind==gears_game_material::Plan::reject)return false;
    if(result.kind==gears_game_material::Plan::ordinary)
        result.plan=gears_viewport::overlay_plan(saved,width,height);
    else if(!gears_ui_hud::plan(saved,width,height,sample,result.plan,
        gears_ui_hud::for_source(hud,source_aspect)))return false;
    if(!result.plan.valid)return false;
    if(result.kind==gears_game_material::Plan::hud&&!gears_ui_layout::identity(layout)){
        result.layout_considered=true;
        gears_ui_layout::Bounds bounds{};gears_viewport::OverlayPlan adjusted{};
        if(gears_ui_layout::quad_bounds(quad.vertices,matrix,bounds)&&
           gears_ui_layout::compose(saved,width,height,result.plan,bounds,layout,adjusted)){
            result.plan=adjusted;result.layout_applied=true;
        }
    }
    out=result;return true;
}
}
