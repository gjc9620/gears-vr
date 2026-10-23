#include "material_overlay_plan.h"
#include "material_reticle_plan.h"
namespace owned_material {
// The CPU-side native upload certificate is tied to this TLS consumer and to
// the exact command list. Actual OM/ticket identity is checked independently.
bool draw(ID3D12GraphicsCommandList* list,uintptr_t caller,gears_ui_material::DrawArgs args){
 auto& c=*ui_material_context;if(!c.owned_ticket||c.owned_failed||gears_owned_scene_live::pipeline_failed)return false;
 const auto binding=owned_target_stats.binding(canonical_identity(list));
 const auto saved=owned_target_stats.draw_state(canonical_identity(list));
 const auto ref=owned_scene_gpu::list_ref(canonical_identity(list));
 const gears_frame_link::Target target{c.owned_resource,gears_native_render_size::width(),gears_native_render_size::height()};
 const auto pair=owned_scene_gpu::tracker.overlay_pair(ref,target);
 gears_ui_material::Quad quad{};gears_ui_material::Candidate candidate{};
 gears_ui_material::ViewPlane plane{};gears_ui_material::ViewPlaneReject reject{};
 uintptr_t wrapper{},native_list{};unsigned stride{},topology{},plan_stage{};
 gears_material_reticle::Reject reticle_reject{};
 auto reject_draw=[&](unsigned reason){
  {std::lock_guard<std::mutex> lock(failure_mutex);if(!first_draw_failure.reason){auto& d=first_draw_failure;
   d.reason=reason;d.ticket=c.owned_ticket;d.pair_ticket=pair.ticket;d.linked=pair.linked;
   d.targets=binding.targets;d.actual=binding.target.resource;d.expected=target.resource;d.width=unsigned(binding.target.width);d.height=binding.target.height;
   d.list=reinterpret_cast<uintptr_t>(list);d.native_list=native_list;d.stride=stride;d.topology=topology;
   d.kind=c.consumer.receipt.kind;d.plan_stage=plan_stage;d.reticle_reject=unsigned(reticle_reject);
   safe_copy(d.rect.data(),reinterpret_cast<void*>(c.consumer.view+0x48),sizeof(d.rect));
   safe_copy(d.projection.data(),reinterpret_cast<void*>(c.consumer.view+0xb0),sizeof(d.projection));
  }}fail(c,reason);return false;
 };
 const auto u=uploading;
 if(!u||u->material!=&c||!safe_copy(&wrapper,reinterpret_cast<void*>(u->native_context+0x127c),8)||
    !wrapper||!safe_copy(&native_list,reinterpret_cast<void*>(wrapper+0x14),8)||native_list!=reinterpret_cast<uintptr_t>(list)||
    !safe_copy(&stride,reinterpret_cast<void*>(u->native_context+0x428),4)||stride!=32||
    !safe_copy(&topology,reinterpret_cast<void*>(u->native_context+0x578),4)||topology!=5||
    !gears_ui_material::capture_quad({base},safe_copy,c.consumer,quad)||quad.source!=u->quad.source||quad.vertices!=u->quad.vertices||
    !gears_ui_material::classify_draw({base},c.consumer,quad,caller,args,candidate)||args.indexed)return reject_draw(25);
 if(owned_target_stats.descriptor_capacity_lost()||binding.targets!=1||binding.target.mip||
    binding.target.resource!=target.resource||binding.target.width!=target.width||binding.target.height!=target.height||
    !pair.linked||pair.ticket!=c.owned_ticket)return reject_draw(27);
 if(!gears_ui_material::material_view_plane(safe_copy,c.consumer,plane,&reject))return reject_draw(40+unsigned(reject));
 gears_vr::StereoSample sample{};gears_material_overlay::Result overlay{};
 gears_ui_layout::Matrix matrix{};std::memcpy(matrix.data(),c.consumer.receipt.transform.data(),sizeof(matrix));
 plan_stage=1;
 if(!owned_scene_gpu::image_tickets.lookup(c.owned_ticket,sample,simulator_operator))return reject_draw(28);
 bool reticle_suppressed=false;
 if(c.consumer.receipt.kind==1){
  gears_vr::StereoTicketGeometry geometry{};gears_material_reticle::Result reticle{};
  const bool directional=ui_reticle_direction_enabled&&ui_hud_plane_enabled;
  plan_stage=2;
  if(directional&&!owned_scene_gpu::image_tickets.lookup_geometry(c.owned_ticket,geometry,simulator_operator))return reject_draw(28);
  plan_stage=3;
  if(!gears_material_reticle::plan(c.consumer.receipt.kind,directional,saved,target.width,target.height,
      plane,quad,matrix,directional?&geometry:nullptr,reticle,&reticle_reject))return reject_draw(28);
  overlay.plan=reticle.plan;reticle_suppressed=reticle.suppressed;
 }else{
  plan_stage=4;
  if(!gears_material_overlay::plan(c.consumer.receipt.kind,ui_hud_plane_enabled,saved,target.width,target.height,
      sample,plane.aspect,ui_hud_parameters,ui_hud_layout_parameters,quad,matrix,overlay))return reject_draw(28);
 }
 if(overlay.layout_considered){++ui_layout_counts.considered;
  if(overlay.layout_applied)++ui_layout_counts.planned;else ++ui_layout_counts.fallback;
 }
 auto viewport=[&](gears_viewport::Rect r,float a,float b){D3D12_VIEWPORT v{r.x,r.y,r.width,r.height,a,b};original_viewports(list,1,&v);};
 auto scissor=[&](gears_viewport::Rect r){D3D12_RECT s{LONG(r.x),LONG(r.y),LONG(r.x+r.width),LONG(r.y+r.height)};original_scissors(list,1,&s);};
 unsigned mask{};
 try{mask=gears_viewport::replay_overlay(saved,overlay.plan,viewport,scissor,[&](unsigned){original_draw(list,args.count,args.instances,args.start,args.first_instance);});}
 catch(...){fail(c,29);throw;}
 if(!owned_scene_gpu::tracker.draw_overlay_pair(ref,pair.ticket,target,pair.eyes,mask)){fail(c,30);return true;}
 if(overlay.layout_applied&&mask==3)++ui_layout_counts.applied;
 if(c.consumer.receipt.kind==1){++reticle_draws;if(reticle_suppressed)++reticle_suppressed_draws;}
 ++c.owned_draws;++draws;owned_scene_gpu::material_candidate(c.owned_ticket);return true;
}
}
