#pragma once
#include "owned_scene_trial.h"
#if defined(GEARS_NATIVE_OWNED_SCENE_PROBE) && GEARS_NATIVE_OWNED_SCENE_PROBE
#include "native_owned_surface.inl"
namespace gears_owned_scene_live {
gears_owned_scene::Trial trial;
gears_native_owned::Owner<gears_native_owned::NativeOps> owner;
std::unique_ptr<gears_native_owned::NativeOps> ops;
bool image_verified{},attempted{};
gears_native_owned::Snapshot created{};
bool healthy(){const auto live=owner.snapshot();
 if(trial.resource&&(live.adapter_fault||live.phase!=gears_native_owned::Phase::live))trial.error=10;
 return !trial.error;
}
}
void initialize_native_owned_scene(bool image_verified){gears_owned_scene_live::image_verified=image_verified;
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
 owned_scene_gpu::enabled=image_verified;
#endif
}
bool native_owned_scene_tick(uint64_t ticket){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 if(attempted)return !trial.error;attempted=true;
 ops=std::make_unique<gears_native_owned::NativeOps>(base,safe_copy,image_verified);
 owned_scene_gpu::CreateScope scope;
 const bool ok=owner.create_on_rhi(*ops,gears_native_owned::Quality::low,ticket);created=owner.snapshot();
 if(!ok){trial.error=8;return false;}
 owned_scene_gpu::accept(owner.borrow_resource(),ticket);
 if(!trial.ready(canonical_identity(owner.borrow_resource()),reinterpret_cast<uintptr_t>(owner.borrow_adapter()),ticket)){
  trial.error=9;return false;}return true;
}
uintptr_t native_owned_scene_candidate(){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
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
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
 const auto consumed_before=owned_scene_gpu::consumed_count;
#endif
 owned_scene_gpu::copy(healthy()&&trial.returned&&trial.destroyed&&trial.submitted_mask==3&&!trial.unknown_draws,trial.ticket);
#if defined(GEARS_OWNED_UI_ROUTE_PROBE) && GEARS_OWNED_UI_ROUTE_PROBE
 if(owned_scene_gpu::consumed_count>consumed_before)owned_ui_published(trial.ticket);
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
 const bool exact=final_eye.valid&&binding.targets==1&&!binding.target.mip&&final_eye.width==4096&&final_eye.height==2048&&
  binding.target.width==4096&&binding.target.height==2048&&verified_stereo_draw(
   frame_target_stats.draw_state(reinterpret_cast<uintptr_t>(list)),final_eye,caller,kind,elements,instances);
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
void print_native_owned_scene(FILE* file){using namespace gears_owned_scene_live;
 std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
 owned_scene_gpu::poll();owned_scene_gpu::print(file);
 healthy();const auto live=owner.snapshot();
 fprintf(file,"native_owned_scene attempted=%u resource=%llx adapter=%llx born=%llu ticket=%llu renderer=%llx width=%u height=%u built=%u entered=%u returned=%u destroyed=%u draw_mask=%u submitted_mask=%u draws=%llu unknown_draws=%llu submissions=%llu queue=%llx error=%u create_error=%u live_phase=%u live_error=%u adapter_fault=%u retained=%u retention_until_process_exit=1 canvas_redirected=0 gpu_state_certified=%u pixels_read_back=%u pixels_verified=0\n",
  attempted?1:0,trial.resource,trial.adapter,trial.born,trial.ticket,trial.renderer,created.width,created.height,
  trial.built?1:0,trial.entered?1:0,trial.returned?1:0,trial.destroyed?1:0,trial.draw_mask,trial.submitted_mask,
  trial.draws,trial.unknown_draws,trial.submissions,trial.queue,trial.error,unsigned(created.error),unsigned(live.phase),unsigned(live.error),live.adapter_fault?1:0,owner.retained()?1:0,
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
void print_native_owned_scene(FILE*){}
#endif
