#pragma once
#include "owned_scene_trial.h"
#include "native_render_size.h"
#if defined(GEARS_NATIVE_OWNED_SCENE_PROBE) && GEARS_NATIVE_OWNED_SCENE_PROBE
#include "native_owned_surface.inl"
#include "owned_scene_retirement.h"
void owned_ui_discard_completed(uint64_t ticket);
namespace gears_owned_scene_live {
gears_owned_scene::Trial trial;
gears_native_owned::Owner<gears_native_owned::NativeOps> owner;
std::unique_ptr<gears_native_owned::NativeOps> ops;
bool image_verified{},attempted{};
gears_owned_retirement::Gate shutdown;
bool shutdown_conflict{};
std::atomic<bool> shutdown_rhi_ready{};
gears_native_owned::Snapshot created{};
bool healthy(){const auto live=owner.snapshot();
 const bool released=shutdown.phase()==gears_owned_retirement::Phase::released&&live.phase==gears_native_owned::Phase::released;
 if(trial.resource&&(live.adapter_fault||(!released&&live.phase!=gears_native_owned::Phase::live)))trial.error=10;
 return !trial.error;
}
}
void initialize_native_owned_scene(bool image_verified){gears_owned_scene_live::image_verified=image_verified&&gears_native_render_size::initialize();
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
 owned_scene_gpu::enabled=gears_owned_scene_live::image_verified;
#endif
}
bool native_owned_scene_tick(uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 if(attempted)return !trial.error;attempted=true;
 if(!image_verified){trial.error=13;return false;}
 ops=std::make_unique<gears_native_owned::NativeOps>(base,safe_copy,image_verified);
 owned_scene_gpu::CreateScope scope;
 const bool ok=owner.create_on_rhi(*ops,gears_native_render_size::width(),gears_native_render_size::height(),ticket);created=owner.snapshot();
 if(!ok){trial.error=8;return false;}
 owned_scene_gpu::accept(owner.borrow_resource(),ticket);
 if(!trial.ready(canonical_identity(owner.borrow_resource()),reinterpret_cast<uintptr_t>(owner.borrow_adapter()),ticket)){
  trial.error=9;return false;}return true;
}
uintptr_t native_owned_scene_candidate(){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 if(shutdown.stopping())return 0;
 if(trial.complete()&&healthy()&&owned_scene_gpu::can_advance(trial.ticket)){
  if(!trial.advance(trial.ticket,true)){trial.error=12;return 0;}
 }
 return healthy()&&trial.candidate()?trial.adapter:0;}
bool native_owned_scene_select(void* renderer,uint64_t ticket,uintptr_t adapter){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 const bool ok=trial.select(reinterpret_cast<uintptr_t>(renderer),ticket,adapter);if(ok)owned_scene_gpu::selected(ticket);return ok;}
bool native_owned_scene_built(void* renderer,uint64_t ticket,bool verified){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);uintptr_t target{};
 const bool read=safe_copy(&target,static_cast<unsigned char*>(renderer)+0x18,8);
 owned_scene_gpu::built(ticket);
 return trial.construction(reinterpret_cast<uintptr_t>(renderer),ticket,verified&&read&&healthy(),target);}
bool native_owned_scene_begin(void* renderer,uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 const bool ok=healthy()&&trial.begin(reinterpret_cast<uintptr_t>(renderer),ticket);
 if(ok&&trial.renderer==reinterpret_cast<uintptr_t>(renderer)&&trial.ticket==ticket){owned_scene_gpu::began(ticket);owned_scene_gpu::activate();}return ok;}
void native_owned_scene_copy(){using namespace gears_owned_scene_live;
 if(shutdown.stopping())return;
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
 const auto consumed_before=owned_scene_gpu::consumed_count,published_before=owned_scene_gpu::published_count;
#endif
 owned_scene_gpu::copy(healthy()&&trial.returned&&trial.destroyed&&trial.submitted_mask==3&&!trial.unknown_draws&&owned_ui_ready(trial.ticket),trial.ticket);
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
 if(owned_scene_gpu::consumed_count>consumed_before){
  if(owned_scene_gpu::published_count>published_before&&owned_scene_gpu::latest_publication.ticket==trial.ticket)
   owned_ui_published(trial.ticket);
  else owned_ui_discard_completed(trial.ticket);
 }
#endif
}
bool native_owned_scene_consuming(void* renderer,uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 return trial.resource&&!trial.error&&trial.entered&&!trial.returned&&ticket&&
  trial.ticket==ticket&&trial.renderer==reinterpret_cast<uintptr_t>(renderer);}
void native_owned_scene_end(void* renderer,uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);trial.end(reinterpret_cast<uintptr_t>(renderer),ticket);
 if(trial.renderer==reinterpret_cast<uintptr_t>(renderer)&&trial.ticket==ticket)owned_scene_gpu::returned(ticket);native_owned_scene_copy();}
void native_owned_scene_destroyed(void* renderer){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);trial.retire(reinterpret_cast<uintptr_t>(renderer));
 if(trial.renderer==reinterpret_cast<uintptr_t>(renderer))owned_scene_gpu::destroyed(trial.ticket);native_owned_scene_copy();}
void native_owned_scene_draw(ID3D12GraphicsCommandList* list,gears_viewport::TargetDraw const& binding,
 uintptr_t caller,unsigned kind,unsigned elements,unsigned instances){using namespace gears_owned_scene_live;
 if(!trial.resource||binding.target.resource!=trial.resource)return;
 if(shutdown.stopping()){shutdown_conflict=true;return;}
 const auto state=owned_bindings_enabled?owned_target_stats.draw_state(canonical_identity(list)):
  frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list));
 const bool exact=final_eye.valid&&binding.targets==1&&!binding.target.mip&&final_eye.width==gears_native_render_size::width()&&final_eye.height==gears_native_render_size::height()&&
  binding.target.width==gears_native_render_size::width()&&binding.target.height==gears_native_render_size::height()&&verified_stereo_draw(
   state,final_eye,caller,kind,elements,instances);
 trial.draw(binding.target.resource,owned_scene_gpu::list_ref(canonical_identity(list)),consuming_frame_ticket,final_eye.eye,exact);
 if(exact){owned_scene_gpu::retain_image();owned_scene_gpu::drew(consuming_frame_ticket,final_eye.eye);}
 owned_scene_gpu::draw(list,binding.target.resource,exact,final_eye.eye,final_eye.rect,consuming_frame_ticket);
}
gears_owned_scene::Receipt native_owned_scene_prepare(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList* const* members){
 using namespace gears_owned_scene_live;if(!trial.ticket||trial.error)return {};
 std::array<ID3D12CommandList*,gears_frame_link::Tracker::list_capacity> lists{};
 std::array<gears_frame_link::ListRef,gears_frame_link::Tracker::list_capacity> refs{};
 if(!count||count>lists.size()||!safe_copy(lists.data(),members,count*sizeof(lists[0])))return {};
 for(unsigned i=0;i<count;++i)refs[i]=owned_scene_gpu::list_ref(canonical_identity(lists[i]));
 return trial.prepare(canonical_identity(queue),queue->GetDesc().Type==D3D12_COMMAND_LIST_TYPE_DIRECT,refs.data(),count);
}
void native_owned_scene_commit(gears_owned_scene::Receipt const& receipt){using namespace gears_owned_scene_live;
 trial.commit(receipt,[](uintptr_t identity){return owned_scene_gpu::list_ref(identity);});
 if(trial.submitted_mask==3)owned_scene_gpu::submitted(trial.ticket);native_owned_scene_copy();}
bool native_owned_scene_retained(){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);return attempted&&owner.retained();}
// Caller holds session_native_mutex: native_cpu_drained includes closed entry
// admission, no pending submitted renderers/state, and no producer build scope.
// Canvas is independently derived from every active owned UI ledger receipt.
bool native_owned_scene_shutdown_worker(bool native_cpu_drained,bool canvas_drained,bool bridges_closed){
 using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 if(!attempted||!owner.retained())return true;
 if(shutdown_conflict)return false;
 if(!shutdown.stopping()){
  if(!native_cpu_drained||!canvas_drained||(trial.ticket&&(!trial.returned||!trial.destroyed)))return false;
  const gears_native_owned::DrainProof proof{created.generation,true,true,true,true,false};
  if(!shutdown.request(proof))return false;
 }
 if(shutdown.phase()==gears_owned_retirement::Phase::cpu_drained&&owned_scene_gpu::begin_shutdown()&&
    owned_scene_gpu::drain_shutdown(bridges_closed))
  if(shutdown.gpu_drained(created.generation,true,bridges_closed,true))shutdown_rhi_ready.store(true);
 return shutdown.phase()==gears_owned_retirement::Phase::released&&!owner.retained();
}
void native_owned_scene_shutdown_rhi(){using namespace gears_owned_scene_live;
 thread_local bool servicing{};
 if(servicing||!shutdown_rhi_ready.load())return;
 std::lock_guard<std::recursive_mutex> native_guard(session_native_mutex);
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 if(shutdown.phase()!=gears_owned_retirement::Phase::gpu_drained||shutdown_conflict||GetCurrentThreadId()!=created.creation_thread)return;
 servicing=true;if(shutdown.retire_on_rhi(owner))shutdown_rhi_ready.store(false);servicing=false;
}
void print_native_owned_scene(FILE* file){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 owned_scene_gpu::poll();owned_scene_gpu::print(file);
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
 const auto& size=gears_native_render_size::packet;
 fprintf(file,"runtime_render_size loaded=%u session=%llu scale=%.6g recommended=%u,%u eye=%u,%u source=%u,%u game_resolution_independent=1 dimensions_are_plan_not_gpu_proof=1\n",
  unsigned(gears_native_render_size::loaded),size.session,size.plan.scale,size.plan.recommended_width,size.plan.recommended_height,
  size.plan.eye_width,size.plan.eye_height,size.plan.source_width,size.plan.source_height);
#endif
 const auto bindings=owned_target_stats.snapshot();
 fprintf(file,"owned_bindings strict=1 resets=%llu overflow=%llu descriptor_evictions=%llu descriptor_lost=%u\n",
  bindings.resets,bindings.overflow,bindings.descriptor_evictions,owned_target_stats.descriptor_capacity_lost()?1:0);
 healthy();const auto live=owner.snapshot();
 fprintf(file,"native_owned_scene attempted=%u resource=%llx adapter=%llx born=%llu ticket=%llu renderer=%llx width=%u height=%u built=%u entered=%u returned=%u destroyed=%u draw_mask=%u submitted_mask=%u draws=%llu unknown_draws=%llu submissions=%llu queue=%llx error=%u create_error=%u live_phase=%u live_error=%u adapter_fault=%u retained=%u retention_until_process_exit=%u shutdown_phase=%u shutdown_conflict=%u canvas_redirected=0 gpu_state_certified=%u pixels_read_back=%u pixels_verified=0\n",
  attempted?1:0,trial.resource,trial.adapter,trial.born,trial.ticket,trial.renderer,created.width,created.height,
  trial.built?1:0,trial.entered?1:0,trial.returned?1:0,trial.destroyed?1:0,trial.draw_mask,trial.submitted_mask,
  trial.draws,trial.unknown_draws,trial.submissions,trial.queue,trial.error,unsigned(created.error),unsigned(live.phase),unsigned(live.error),live.adapter_fault?1:0,owner.retained()?1:0,
  owner.retained()?1:0,unsigned(shutdown.phase()),shutdown_conflict?1:0,
  owned_scene_gpu::state_certified()?1:0,owned_scene_gpu::pixels_read_back()?1:0);
}
#else
void initialize_native_owned_scene(bool){}
bool native_owned_scene_tick(uint64_t){return true;}
uintptr_t native_owned_scene_candidate(){return 0;}
bool native_owned_scene_select(void*,uint64_t,uintptr_t){return false;}
bool native_owned_scene_built(void*,uint64_t,bool){return true;}
bool native_owned_scene_begin(void*,uint64_t){return true;}
bool native_owned_scene_consuming(void*,uint64_t){return false;}
void native_owned_scene_end(void*,uint64_t){}
void native_owned_scene_destroyed(void*){}
void native_owned_scene_draw(ID3D12GraphicsCommandList*,gears_viewport::TargetDraw const&,uintptr_t,unsigned,unsigned,unsigned){}
gears_owned_scene::Receipt native_owned_scene_prepare(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){return {};}
void native_owned_scene_commit(gears_owned_scene::Receipt const&){}
bool native_owned_scene_retained(){return false;}
bool native_owned_scene_shutdown_worker(bool,bool,bool){return true;}
void native_owned_scene_shutdown_rhi(){}
void print_native_owned_scene(FILE*){}
#endif
