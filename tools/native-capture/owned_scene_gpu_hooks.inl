#pragma once
#if (defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS) || (defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER)
using OwnedCreateListFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,UINT,D3D12_COMMAND_LIST_TYPE,ID3D12CommandAllocator*,ID3D12PipelineState*,REFIID,void**);
using OwnedCreateList1Fn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device4*,UINT,D3D12_COMMAND_LIST_TYPE,D3D12_COMMAND_LIST_FLAGS,REFIID,void**);
OwnedCreateListFn owned_original_create_list{};OwnedCreateList1Fn owned_original_create_list1{};
thread_local unsigned owned_list_create_depth{};
struct OwnedListCreateScope{bool outer;OwnedListCreateScope():outer(owned_list_create_depth++==0){}~OwnedListCreateScope(){--owned_list_create_depth;}};
HRESULT STDMETHODCALLTYPE owned_hook_create_list(ID3D12Device* d,UINT mask,D3D12_COMMAND_LIST_TYPE type,ID3D12CommandAllocator* allocator,ID3D12PipelineState* state,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_create_list(d,mask,type,allocator,state,iid,out);
 Scope scope;auto lock=frame_lock();OwnedListCreateScope creation;auto hr=owned_original_create_list(d,mask,type,allocator,state,iid,out);
 if(creation.outer){
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
  owned_scene_gpu::list_created(hr,out,true);
#endif
  ngx_capture_control::created_command(hr,out,true);}return hr;
}
HRESULT STDMETHODCALLTYPE owned_hook_create_list1(ID3D12Device4* d,UINT mask,D3D12_COMMAND_LIST_TYPE type,D3D12_COMMAND_LIST_FLAGS flags,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_create_list1(d,mask,type,flags,iid,out);
 Scope scope;auto lock=frame_lock();OwnedListCreateScope creation;auto hr=owned_original_create_list1(d,mask,type,flags,iid,out);
 if(creation.outer){
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
  owned_scene_gpu::list_created(hr,out,false);
#endif
  ngx_capture_control::created_command(hr,out,false);}return hr;
}
// One shared installer: owned pixels and NGX host never hook the same slot twice.
bool command_list_birth_install(ID3D12Device* device){
#if defined(GEARS_NGX_STEREO_HOST_OBSERVER) && GEARS_NGX_STEREO_HOST_OBSERVER
 constexpr bool enabled=true;
#else
 constexpr bool enabled=owned_scene_gpu::continuous_probe;
#endif
 if(!enabled)return true;
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
 constexpr unsigned first=27;
#else
 constexpr unsigned first=11;
#endif
 auto target=(*reinterpret_cast<void***>(device))[12];
 ID3D12Device4* device4{};void* target1{};
 if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&device4)))){target1=(*reinterpret_cast<void***>(device4))[51];device4->Release();}
 // Distinct public signatures cannot share a trampoline; reject an alias.
 if(!target||(target1&&target1==target))return false;
 viewport_targets[first]=target;
 bool ok=install(target,reinterpret_cast<void*>(owned_hook_create_list),reinterpret_cast<void**>(&owned_original_create_list));
 if(target1){viewport_targets[first+1]=target1;ok=install(target1,reinterpret_cast<void*>(owned_hook_create_list1),reinterpret_cast<void**>(&owned_original_create_list1))&&ok;}
 return ok;
}
#endif
#if defined(GEARS_NATIVE_OWNED_PIXELS) && GEARS_NATIVE_OWNED_PIXELS
using OwnedCommittedFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,const D3D12_RESOURCE_DESC*,D3D12_RESOURCE_STATES,const D3D12_CLEAR_VALUE*,REFIID,void**);
using OwnedCommitted1Fn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device4*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,const D3D12_RESOURCE_DESC*,D3D12_RESOURCE_STATES,const D3D12_CLEAR_VALUE*,ID3D12ProtectedResourceSession*,REFIID,void**);
using OwnedCommitted2Fn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device8*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,const D3D12_RESOURCE_DESC1*,D3D12_RESOURCE_STATES,const D3D12_CLEAR_VALUE*,ID3D12ProtectedResourceSession*,REFIID,void**);
using OwnedCommitted3Fn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device10*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,const D3D12_RESOURCE_DESC1*,D3D12_BARRIER_LAYOUT,const D3D12_CLEAR_VALUE*,ID3D12ProtectedResourceSession*,UINT32,const DXGI_FORMAT*,REFIID,void**);
using OwnedPlacedFn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*,ID3D12Heap*,UINT64,const D3D12_RESOURCE_DESC*,D3D12_RESOURCE_STATES,const D3D12_CLEAR_VALUE*,REFIID,void**);
using OwnedPlaced1Fn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device8*,ID3D12Heap*,UINT64,const D3D12_RESOURCE_DESC1*,D3D12_RESOURCE_STATES,const D3D12_CLEAR_VALUE*,REFIID,void**);
using OwnedPlaced2Fn=HRESULT(STDMETHODCALLTYPE*)(ID3D12Device10*,ID3D12Heap*,UINT64,const D3D12_RESOURCE_DESC1*,D3D12_BARRIER_LAYOUT,const D3D12_CLEAR_VALUE*,UINT32,const DXGI_FORMAT*,REFIID,void**);
using OwnedEnhancedFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList7*,UINT32,const D3D12_BARRIER_GROUP*);
using OwnedClearFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,D3D12_CPU_DESCRIPTOR_HANDLE,const FLOAT*,UINT,const D3D12_RECT*);
using OwnedCopyFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12Resource*,ID3D12Resource*);
using OwnedResolveFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12Resource*,UINT,ID3D12Resource*,UINT,DXGI_FORMAT);
using OwnedDiscardFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12Resource*,const D3D12_DISCARD_REGION*);
using OwnedBundleFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12GraphicsCommandList*);
using OwnedIndirectFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandSignature*,UINT,ID3D12Resource*,UINT64,ID3D12Resource*,UINT64);
using OwnedResolveRegionFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList1*,ID3D12Resource*,UINT,UINT,UINT,ID3D12Resource*,UINT,const D3D12_RECT*,DXGI_FORMAT,D3D12_RESOLVE_MODE);
using OwnedRenderPassFn=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,UINT,const D3D12_RENDER_PASS_RENDER_TARGET_DESC*,const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC*,D3D12_RENDER_PASS_FLAGS);
OwnedCommittedFn owned_original_committed{};OwnedPlacedFn owned_original_placed{};
OwnedCommitted1Fn owned_original_committed1{};OwnedCommitted2Fn owned_original_committed2{};OwnedCommitted3Fn owned_original_committed3{};
OwnedPlaced1Fn owned_original_placed1{};OwnedPlaced2Fn owned_original_placed2{};
OwnedEnhancedFn owned_original_enhanced{};OwnedClearFn owned_original_clear{};
OwnedCopyFn owned_original_copy{};OwnedResolveFn owned_original_resolve{};OwnedDiscardFn owned_original_discard{};
OwnedBundleFn owned_original_bundle{};OwnedIndirectFn owned_original_indirect{};
OwnedResolveRegionFn owned_original_resolve_region{};OwnedRenderPassFn owned_original_render_pass{};
HRESULT STDMETHODCALLTYPE owned_hook_committed(ID3D12Device* d,const D3D12_HEAP_PROPERTIES* h,D3D12_HEAP_FLAGS f,const D3D12_RESOURCE_DESC* desc,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_committed(d,h,f,desc,state,clear,iid,out);
 Scope scope;auto lock=frame_lock();const auto hr=owned_original_committed(d,h,f,desc,state,clear,iid,out);
 owned_scene_gpu::created(d,hr,out,state,1);return hr;
}
HRESULT STDMETHODCALLTYPE owned_hook_placed(ID3D12Device* d,ID3D12Heap* h,UINT64 offset,const D3D12_RESOURCE_DESC* desc,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_placed(d,h,offset,desc,state,clear,iid,out);
 Scope scope;auto lock=frame_lock();const auto hr=owned_original_placed(d,h,offset,desc,state,clear,iid,out);
 owned_scene_gpu::created(d,hr,out,state,2);return hr;
}
HRESULT STDMETHODCALLTYPE owned_hook_committed1(ID3D12Device4* d,const D3D12_HEAP_PROPERTIES* h,D3D12_HEAP_FLAGS f,const D3D12_RESOURCE_DESC* desc,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear,ID3D12ProtectedResourceSession* session,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_committed1(d,h,f,desc,state,clear,session,iid,out);
 Scope scope;auto lock=frame_lock();const auto hr=owned_original_committed1(d,h,f,desc,state,clear,session,iid,out);
 owned_scene_gpu::created(d,hr,out,state,5);return hr;
}
HRESULT STDMETHODCALLTYPE owned_hook_committed2(ID3D12Device8* d,const D3D12_HEAP_PROPERTIES* h,D3D12_HEAP_FLAGS f,const D3D12_RESOURCE_DESC1* desc,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear,ID3D12ProtectedResourceSession* session,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_committed2(d,h,f,desc,state,clear,session,iid,out);
 Scope scope;auto lock=frame_lock();const auto hr=owned_original_committed2(d,h,f,desc,state,clear,session,iid,out);
 owned_scene_gpu::created(d,hr,out,state,6);return hr;
}
HRESULT STDMETHODCALLTYPE owned_hook_committed3(ID3D12Device10* d,const D3D12_HEAP_PROPERTIES* h,D3D12_HEAP_FLAGS f,const D3D12_RESOURCE_DESC1* desc,D3D12_BARRIER_LAYOUT layout,const D3D12_CLEAR_VALUE* clear,ID3D12ProtectedResourceSession* session,UINT32 formats,const DXGI_FORMAT* castable,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_committed3(d,h,f,desc,layout,clear,session,formats,castable,iid,out);
 Scope scope;auto lock=frame_lock();const auto hr=owned_original_committed3(d,h,f,desc,layout,clear,session,formats,castable,iid,out);
 owned_scene_gpu::created(d,hr,out,D3D12_RESOURCE_STATE_COMMON,7,false,unsigned(layout));return hr;
}
void STDMETHODCALLTYPE owned_hook_enhanced(ID3D12GraphicsCommandList7* list,UINT32 count,const D3D12_BARRIER_GROUP* groups){
 if(stereo_publish_bypass){owned_original_enhanced(list,count,groups);return;}
 Scope scope;auto lock=frame_lock();if(count)owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::enhanced);owned_original_enhanced(list,count,groups);
}
HRESULT STDMETHODCALLTYPE owned_hook_placed1(ID3D12Device8* d,ID3D12Heap* h,UINT64 offset,const D3D12_RESOURCE_DESC1* desc,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_placed1(d,h,offset,desc,state,clear,iid,out);
 Scope scope;auto lock=frame_lock();const auto hr=owned_original_placed1(d,h,offset,desc,state,clear,iid,out);
 owned_scene_gpu::created(d,hr,out,state,3);return hr;
}
HRESULT STDMETHODCALLTYPE owned_hook_placed2(ID3D12Device10* d,ID3D12Heap* h,UINT64 offset,const D3D12_RESOURCE_DESC1* desc,D3D12_BARRIER_LAYOUT layout,const D3D12_CLEAR_VALUE* clear,UINT32 formats,const DXGI_FORMAT* castable,REFIID iid,void** out){
 if(stereo_publish_bypass)return owned_original_placed2(d,h,offset,desc,layout,clear,formats,castable,iid,out);
 Scope scope;auto lock=frame_lock();const auto hr=owned_original_placed2(d,h,offset,desc,layout,clear,formats,castable,iid,out);
 // An enhanced layout is explicitly NOT converted into a legacy resource state.
 owned_scene_gpu::created(d,hr,out,D3D12_RESOURCE_STATE_COMMON,4,false,unsigned(layout));return hr;
}
void STDMETHODCALLTYPE owned_hook_clear(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE handle,const FLOAT* rgba,UINT count,const D3D12_RECT* rect){
 if(stereo_publish_bypass){owned_original_clear(list,handle,rgba,count,rect);return;}
 Scope scope;auto lock=frame_lock();auto target=owned_target_stats.lookup(handle.ptr);
 if(!target.resource||owned_target_stats.descriptor_capacity_lost())owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::clear);else owned_scene_gpu::use(list,target.resource,false);
 owned_original_clear(list,handle,rgba,count,rect);
}
void STDMETHODCALLTYPE owned_hook_copy(ID3D12GraphicsCommandList* list,ID3D12Resource* dst,ID3D12Resource* src){
 if(stereo_publish_bypass){owned_original_copy(list,dst,src);return;}
 Scope scope;auto lock=frame_lock();if(owned_scene_gpu::watching()){owned_scene_gpu::use(list,canonical_identity(dst),true);owned_scene_gpu::use(list,canonical_identity(src),true);}
 owned_original_copy(list,dst,src);
}
void STDMETHODCALLTYPE owned_hook_resolve(ID3D12GraphicsCommandList* list,ID3D12Resource* dst,UINT di,ID3D12Resource* src,UINT si,DXGI_FORMAT fmt){
 if(stereo_publish_bypass){owned_original_resolve(list,dst,di,src,si,fmt);return;}
 Scope scope;auto lock=frame_lock();if(owned_scene_gpu::watching()){owned_scene_gpu::use(list,canonical_identity(dst),true);owned_scene_gpu::use(list,canonical_identity(src),true);}
 owned_original_resolve(list,dst,di,src,si,fmt);
}
void STDMETHODCALLTYPE owned_hook_discard(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,const D3D12_DISCARD_REGION* region){
 if(stereo_publish_bypass){owned_original_discard(list,resource,region);return;}
 Scope scope;auto lock=frame_lock();if(owned_scene_gpu::watching())owned_scene_gpu::use(list,canonical_identity(resource),true);owned_original_discard(list,resource,region);
}
void STDMETHODCALLTYPE owned_hook_bundle(ID3D12GraphicsCommandList* list,ID3D12GraphicsCommandList* bundle){
 if(stereo_publish_bypass){owned_original_bundle(list,bundle);return;}
 Scope scope;auto lock=frame_lock();owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::bundle);owned_original_bundle(list,bundle);
}
void STDMETHODCALLTYPE owned_hook_indirect(ID3D12GraphicsCommandList* list,ID3D12CommandSignature* sig,UINT count,ID3D12Resource* args,UINT64 offset,ID3D12Resource* counts,UINT64 count_offset){
 if(stereo_publish_bypass){owned_original_indirect(list,sig,count,args,offset,counts,count_offset);return;}
 Scope scope;auto lock=frame_lock();if(count)owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::indirect);owned_original_indirect(list,sig,count,args,offset,counts,count_offset);
}
void STDMETHODCALLTYPE owned_hook_resolve_region(ID3D12GraphicsCommandList1* list,ID3D12Resource* dst,UINT di,UINT x,UINT y,ID3D12Resource* src,UINT si,const D3D12_RECT* rect,DXGI_FORMAT fmt,D3D12_RESOLVE_MODE mode){
 if(stereo_publish_bypass){owned_original_resolve_region(list,dst,di,x,y,src,si,rect,fmt,mode);return;}
 Scope scope;auto lock=frame_lock();if(owned_scene_gpu::watching()){owned_scene_gpu::use(list,canonical_identity(dst),true);owned_scene_gpu::use(list,canonical_identity(src),true);}
 owned_original_resolve_region(list,dst,di,x,y,src,si,rect,fmt,mode);
}
void STDMETHODCALLTYPE owned_hook_render_pass(ID3D12GraphicsCommandList4* list,UINT count,const D3D12_RENDER_PASS_RENDER_TARGET_DESC* targets,const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* depth,D3D12_RENDER_PASS_FLAGS flags){
 if(stereo_publish_bypass){owned_original_render_pass(list,count,targets,depth,flags);return;}
 Scope scope;auto lock=frame_lock();owned_scene_gpu::uncertain(owned_scene_gpu::Unknown::render_pass);owned_original_render_pass(list,count,targets,depth,flags);
}
bool owned_scene_gpu_install(ID3D12Device* device,ID3D12GraphicsCommandList* list){
 auto dev=*reinterpret_cast<void***>(device);auto tab=*reinterpret_cast<void***>(list);bool ok=true;
 auto add=[&](unsigned index,void* target,void* hook,void** original){viewport_targets[index]=target;ok=install(target,hook,original)&&ok;};
 add(11,dev[27],reinterpret_cast<void*>(owned_hook_committed),reinterpret_cast<void**>(&owned_original_committed));
 add(12,dev[29],reinterpret_cast<void*>(owned_hook_placed),reinterpret_cast<void**>(&owned_original_placed));
 add(13,tab[48],reinterpret_cast<void*>(owned_hook_clear),reinterpret_cast<void**>(&owned_original_clear));
 add(14,tab[17],reinterpret_cast<void*>(owned_hook_copy),reinterpret_cast<void**>(&owned_original_copy));
 add(15,tab[19],reinterpret_cast<void*>(owned_hook_resolve),reinterpret_cast<void**>(&owned_original_resolve));
 add(16,tab[51],reinterpret_cast<void*>(owned_hook_discard),reinterpret_cast<void**>(&owned_original_discard));
 add(17,tab[27],reinterpret_cast<void*>(owned_hook_bundle),reinterpret_cast<void**>(&owned_original_bundle));
 add(18,tab[59],reinterpret_cast<void*>(owned_hook_indirect),reinterpret_cast<void**>(&owned_original_indirect));
 ID3D12GraphicsCommandList1* list1{};if(SUCCEEDED(list->QueryInterface(IID_PPV_ARGS(&list1)))){
  add(19,(*reinterpret_cast<void***>(list1))[64],reinterpret_cast<void*>(owned_hook_resolve_region),reinterpret_cast<void**>(&owned_original_resolve_region));list1->Release();}
 ID3D12GraphicsCommandList4* list4{};if(SUCCEEDED(list->QueryInterface(IID_PPV_ARGS(&list4)))){
  add(20,(*reinterpret_cast<void***>(list4))[68],reinterpret_cast<void*>(owned_hook_render_pass),reinterpret_cast<void**>(&owned_original_render_pass));list4->Release();}
 ID3D12GraphicsCommandList7* list7{};if(SUCCEEDED(list->QueryInterface(IID_PPV_ARGS(&list7)))){
  add(21,(*reinterpret_cast<void***>(list7))[80],reinterpret_cast<void*>(owned_hook_enhanced),reinterpret_cast<void**>(&owned_original_enhanced));list7->Release();}
 // CopyTextureRegion shares the existing diagnostic wrapper; it is forced on
 // for this build even when verbose color-copy logging is disabled.
 if(!color_copy_diagnostics){viewport_targets[9]=tab[16];ok=install(tab[16],reinterpret_cast<void*>(hook_color_copy),reinterpret_cast<void**>(&original_color_copy))&&ok;}
 // The saved native dispatcher 21c784 has separate +e8/+230/+268 slots.
 // Use public COM interfaces to observe whichever actual branch is used.
 ID3D12Device8* device8{};if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&device8)))){
  add(22,(*reinterpret_cast<void***>(device8))[70],reinterpret_cast<void*>(owned_hook_placed1),reinterpret_cast<void**>(&owned_original_placed1));
  add(25,(*reinterpret_cast<void***>(device8))[69],reinterpret_cast<void*>(owned_hook_committed2),reinterpret_cast<void**>(&owned_original_committed2));device8->Release();}
 ID3D12Device10* device10{};if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&device10)))){
  add(23,(*reinterpret_cast<void***>(device10))[77],reinterpret_cast<void*>(owned_hook_placed2),reinterpret_cast<void**>(&owned_original_placed2));
  add(26,(*reinterpret_cast<void***>(device10))[76],reinterpret_cast<void*>(owned_hook_committed3),reinterpret_cast<void**>(&owned_original_committed3));device10->Release();}
 ID3D12Device4* device4{};if(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&device4)))){
  add(24,(*reinterpret_cast<void***>(device4))[53],reinterpret_cast<void*>(owned_hook_committed1),reinterpret_cast<void**>(&owned_original_committed1));
  device4->Release();}
 return ok;
}
#endif
