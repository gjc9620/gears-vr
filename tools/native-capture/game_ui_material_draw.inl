#include "game_ui_material_target.h"
namespace game_material {
bool draw(ID3D12GraphicsCommandList* list,uintptr_t caller,gears_ui_material::DrawArgs args){
    if(!current||!current->entered||current->bad||current->context!=ui_material_context||!recording.load())return false;
    auto& c=*ui_material_context;const auto* u=uploading;
    DrawFailure diagnostic{};diagnostic.kind=c.consumer.receipt.kind;
    const auto raw=reinterpret_cast<uintptr_t>(list);const auto binding=frame_target_stats.binding(raw);
    const auto saved=frame_target_stats.draw_state(raw);const auto ref=frame_link.current(canonical_identity(list));
    diagnostic.target=binding.target.resource;diagnostic.width=unsigned(binding.target.width);diagnostic.height=binding.target.height;
    auto reject=[&](unsigned reason){current->bad=true;++rejected;++reject_reasons[reason];
        if(!first_rejects[reason].reason){diagnostic.reason=reason;first_rejects[reason]=diagnostic;}return false;};
    const auto kind=gears_game_material::plan_kind(c.consumer.receipt.kind,ui_hud_plane_enabled);
    if(kind==gears_game_material::Plan::reject||!u||args.indexed)return reject(1);
    uintptr_t wrapper{},native_list{};unsigned stride{},topology{};
    gears_ui_material::Quad quad{};gears_ui_material::Candidate candidate{};
    if(!safe_copy(&wrapper,reinterpret_cast<void*>(u->native_context+0x127c),8)||!wrapper||
       !safe_copy(&native_list,reinterpret_cast<void*>(wrapper+0x14),8)||
       !safe_copy(&stride,reinterpret_cast<void*>(u->native_context+0x428),4)||
       !safe_copy(&topology,reinterpret_cast<void*>(u->native_context+0x578),4)||
       !gears_ui_material::capture_quad({base},safe_copy,c.consumer,quad)||
       !gears_game_material::upload_matches(*u,reinterpret_cast<uintptr_t>(&c),reinterpret_cast<uintptr_t>(list),native_list,stride,topology,quad)||
       !gears_ui_material::classify_draw({base},c.consumer,quad,caller,args,candidate))return reject(2);
    if(frame_target_stats.descriptor_capacity_lost()||binding.targets!=1||binding.target.mip||
       !binding.target.width||binding.target.width>8192||!binding.target.height||binding.target.height>8192)return reject(3);
    const gears_frame_link::Target target{binding.target.resource,unsigned(binding.target.width),binding.target.height};
    const auto pair=gears_game_material::material_target_pair(frame_link,ref,target,binding.target.format,
        swapchain_bindings.known_buffer(binding.target.resource),final_transfer_live::enabled());
    diagnostic.ticket=pair.ticket;diagnostic.pair_rejection=pair.rejection;
    if(!pair.linked||!pair.ticket)return reject(4);
    gears_ui_material::ViewPlane plane{};gears_ui_material::ViewPlaneReject plane_reject{};
    if(!gears_ui_material::material_view_plane(safe_copy,c.consumer,plane,&plane_reject)){
        diagnostic.plane_rejection=unsigned(plane_reject);return reject(5);}
    gears_viewport::OverlayPlan plan{};gears_vr::StereoSample sample{};
    if(!stereo_ticket_store.lookup(pair.ticket,sample,simulator_operator))return reject(6);
    if(kind==gears_game_material::Plan::ordinary)plan=gears_viewport::overlay_plan(saved,target.width,target.height);
    else {const auto parameters=gears_ui_hud::for_source(ui_hud_parameters,plane.aspect);
        if(!prepare_ui_panel(pair.ticket,saved,target.width,target.height,sample,plan,parameters))return reject(7);}
    if(!plan.valid)return reject(7);
    bool layout_applied=false;
    if(kind==gears_game_material::Plan::hud&&!gears_ui_layout::identity(ui_hud_layout_parameters)){
        ++ui_layout_counts.considered;
        gears_ui_layout::Matrix matrix{};std::memcpy(matrix.data(),c.consumer.receipt.transform.data(),sizeof(matrix));
        gears_ui_layout::Bounds bounds{};gears_viewport::OverlayPlan adjusted{};
        if(gears_ui_layout::quad_bounds(quad.vertices,matrix,bounds)&&
           gears_ui_layout::compose(saved,target.width,target.height,plan,bounds,ui_hud_layout_parameters,adjusted)){
            plan=adjusted;layout_applied=true;++ui_layout_counts.planned;
        }else ++ui_layout_counts.fallback;
    }
    auto viewport=[&](gears_viewport::Rect r,float a,float b){D3D12_VIEWPORT v{r.x,r.y,r.width,r.height,a,b};original_viewports(list,1,&v);};
    auto scissor=[&](gears_viewport::Rect r){D3D12_RECT s{LONG(r.x),LONG(r.y),LONG(r.x+r.width),LONG(r.y+r.height)};original_scissors(list,1,&s);};
    current->drew=true;unsigned mask{};
    try{mask=gears_viewport::replay_overlay(saved,plan,viewport,scissor,[&](unsigned){original_draw(list,args.count,args.instances,args.start,args.first_instance);});}
    catch(...){current->bad=true;frame_link.invalidate();throw;}
    if(!frame_link.draw_overlay_pair(ref,pair.ticket,target,pair.eyes,mask)){
        reject(8);frame_link.invalidate();return true; // Pixels already emitted; never forward a third draw.
    }
    ++draws;if(kind==gears_game_material::Plan::ordinary)++ordinary_draws;else ++hud_draws;
    if(layout_applied&&mask==3)++ui_layout_counts.applied;
    return true;
}
}
