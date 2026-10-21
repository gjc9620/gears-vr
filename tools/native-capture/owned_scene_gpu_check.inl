#pragma once
// Fresh-process integration checks. The clear is real WARP work; paired draw
// metadata is synthetic, so this tests observation/readback, not scene rendering.
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureOwnedSceneGpuCheck(void* input){
#if !defined(GEARS_NATIVE_OWNED_PIXELS) || !GEARS_NATIVE_OWNED_PIXELS
 (void)input;return ERROR_NOT_SUPPORTED;
#else
 if(!input)return 2;const unsigned mode=*static_cast<unsigned*>(input);if(mode>25)return 2;
 const bool xr_mode=(mode>=10&&mode<=12)||(mode>=19&&mode<=24);
 if(mode==19&&!owned_scene_gpu::sustained)return ERROR_NOT_SUPPORTED;
 gears_vr::RenderSizePlan fixture_size{2048,2048,2048,2048,4096,2048,1.};
 if(mode>=22){
#if !defined(GEARS_RUNTIME_RENDER_SIZE) || !GEARS_RUNTIME_RENDER_SIZE || !defined(GEARS_VR_DOF_COMPAT) || !GEARS_VR_DOF_COMPAT
  return ERROR_NOT_SUPPORTED;
#else
  const unsigned recommended_height=mode==24?1401u:1400u;
  const gears_vr::RenderSizeLimits limits{{1280,1280},{recommended_height,recommended_height},{4096,4096},{4096,4096},4096,4096};
  if(gears_vr::make_render_size_plan(limits,mode==23?.7:1.,fixture_size)!=gears_vr::RenderSizeError::none)return 73;
#endif
 }
 const unsigned source_width=fixture_size.source_width,source_height=fixture_size.source_height,eye_width=fixture_size.eye_width;
 const uint64_t eye_pixels=uint64_t(eye_width)*source_height;
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
 // Explicit synthetic fixture geometry; no runtime recommendation, engine
 // surface creation or actual-game rendering is claimed by this WARP test.
 gears_native_render_size::packet.plan=fixture_size;
 gears_native_render_size::packet.session=17;gears_native_render_size::loaded=true;
#endif
 using Microsoft::WRL::ComPtr;
 using namespace owned_scene_gpu;
 if(enabled||tracker.snapshot().reset_calls)return 3;
 ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;ComPtr<ID3D12Device> device;
 ComPtr<ID3D12CommandQueue> q;ComPtr<ID3D12CommandAllocator> allocator;
 ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12Resource> target;ComPtr<ID3D12DescriptorHeap> rtv;ComPtr<ID3D12Heap> placed_heap;
 if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))||FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)))||
  FAILED(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))return 4;
 D3D12_COMMAND_QUEUE_DESC qd{};
 if(FAILED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&q)))||
  FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)))||
  FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))))return 5;
 auto lt=*reinterpret_cast<void***>(list.Get());auto qt=*reinterpret_cast<void***>(q.Get());auto dt=*reinterpret_cast<void***>(device.Get());
 original_reset_list=reinterpret_cast<ResetListFn>(lt[10]);original_resource_barrier=reinterpret_cast<BarrierFn>(lt[26]);
 original_execute=reinterpret_cast<ExecuteFn>(qt[10]);owned_original_committed=reinterpret_cast<OwnedCommittedFn>(dt[27]);
 owned_original_clear=reinterpret_cast<OwnedClearFn>(lt[48]);original_create_rtv=reinterpret_cast<CreateRtvFn>(dt[20]);
 self_testing=true;recording=true;frame_link_diagnostics=true;enabled=true;
 if(FAILED(list->Close())||FAILED(hook_reset_list(list.Get(),allocator.Get(),nullptr)))return 6;
 if((mode>=13&&mode<=18)||mode==25){
#if defined(GEARS_NATIVE_OWNED_SCENE_PROBE) && GEARS_NATIVE_OWNED_SCENE_PROBE
  if(!continuous_probe)return ERROR_NOT_SUPPORTED;
  // Real WARP list lifetime, synthetic scene metadata. Exercise production
  // Draw/prepare/commit wiring; no scene resource or native Owner is created.
  ComPtr<ID3D12GraphicsCommandList> created_list;
  auto* observed=list.Get();
  if(mode==14||mode==15||mode==25){
   if(FAILED(list->Close()))return 53; // One recording list per allocator.
   if(mode!=15){
    if(FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&created_list))))return 40;
   }else{
    ComPtr<ID3D12Device4> d4;if(FAILED(device.As(&d4))||
     FAILED(d4->CreateCommandList1(0,D3D12_COMMAND_LIST_TYPE_DIRECT,D3D12_COMMAND_LIST_FLAG_NONE,IID_PPV_ARGS(&created_list))))return 41;
   }
   void* output=created_list.Get();list_created(S_OK,&output,mode!=15);observed=created_list.Get();
   if(error)return 42;
   const auto creates=tracker.snapshot().list_creates;
   list_created(S_OK,&output,mode!=15);
   if(error||list_creation_repeats!=1||tracker.snapshot().list_creates!=creates)return 74;
   if(mode==25){
    const gears_frame_link::Identity retired_id=canonical_identity(observed);
    release_retired_list_leases(&retired_id,1);
    return list_creation_active==0&&list_creation_retired==1&&
      tracker.current(retired_id).identity==retired_id?0:75;
   }
  }
  const auto identity=canonical_identity(observed);
  if(mode==13&&!frame_link.current(identity).identity)return 43;
  if(mode==16)reset(observed,E_FAIL); // Inject failed-Reset observation; never submit it.
  frame_link.retire_present_interval();present();
  if(frame_link.current(identity).identity)return 44;
  auto& t=gears_owned_scene_live::trial;
  if(!t.ready(100,200,1)||!t.select(300,2,200)||!t.construction(300,2,true,200)||!t.begin(300,2))return 45;
  if(!native_owned_scene_consuming(reinterpret_cast<void*>(300),2)||
     native_owned_scene_consuming(reinterpret_cast<void*>(301),2)||
     native_owned_scene_consuming(reinterpret_cast<void*>(300),3))return 56;
  const auto ref=list_ref(identity);
  const auto raw_list=reinterpret_cast<uintptr_t>(observed);
  frame_target_stats.reset(raw_list);
  frame_target_stats.set(raw_list,gears_viewport::other,false,{0,0,float(source_width),float(source_height)});
  frame_target_stats.set(raw_list,gears_viewport::other,true,{0,0,float(source_width),float(source_height)});
  gears_viewport::TargetDraw binding{};binding.targets=1;binding.target={100,source_width,source_height,28,0};
  consuming_frame_ticket=2;
  for(unsigned eye=0;eye<2;++eye){
   final_eye={true,eye,{eye*eye_width,0,eye_width,source_height},source_width,source_height};
   native_owned_scene_draw(observed,binding,base+0x1af92b,1,6,1);
  }
  final_eye={};
  if(mode==15||mode==16)return !ref.identity&&t.error==3&&!t.draw_mask?0:46;
  if(!ref.identity||!ref.generation||t.error||t.draw_mask!=3)return 47;
  ID3D12CommandList* members[]{observed};
  const auto receipt=native_owned_scene_prepare(q.Get(),1,members);
  if(receipt.count!=1)return 48;
  if(mode==18){
   for(unsigned state=0;state<3;++state){
    finished=state==0;enabled=state!=1;error=state==2?1:0;
    if(list_ref(identity).identity||native_owned_scene_prepare(q.Get(),1,members).count)return 54;
   }
   return !t.submitted_mask&&!t.submissions?0:55;
  }
  if(mode==17){
   if(FAILED(observed->Close())||FAILED(hook_reset_list(observed,allocator.Get(),nullptr)))return 49;
   native_owned_scene_commit(receipt);
   return !t.submitted_mask&&!t.submissions?0:50;
  }
  native_owned_scene_commit(receipt);
  // commit's subsequent copy guard correctly rejects the absent native Owner
  // (error 10). The CPU receipt must have committed before that separate guard.
  return t.submitted_mask==3&&t.submissions==1&&t.queue==canonical_identity(q.Get())&&
   t.error==10&&!readback.result().queued?0:51;
#else
  return ERROR_NOT_SUPPORTED;
#endif
 }
 if(mode==8||mode==9){
  if(!continuous_probe)return ERROR_NOT_SUPPORTED;
  auto receipt=prepare(q.Get(),mode==8?gears_frame_link::Tracker::list_capacity+1:1,nullptr);
  commit(receipt);present(); // No invalid call is sent to the real GPU in this negative test.
  return error==15&&!watching()&&!birth_ok?0:23;
 }
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=source_width;desc.Height=source_height;
 desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
 {
  CreateScope creation_scope;
  if(mode>=5&&mode<=7){
   ComPtr<ID3D12Device8> d8;if(FAILED(device.As(&d8)))return 17;
   auto info=device->GetResourceAllocationInfo(0,1,&desc);D3D12_HEAP_DESC hd{};hd.SizeInBytes=info.SizeInBytes;hd.Alignment=info.Alignment;
   hd.Properties=heap;hd.Flags=D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
   if(mode==5&&FAILED(device->CreateHeap(&hd,IID_PPV_ARGS(&placed_heap))))return 18;
   D3D12_RESOURCE_DESC1 d{};d.Dimension=desc.Dimension;d.Width=desc.Width;d.Height=desc.Height;d.DepthOrArraySize=d.MipLevels=1;
   d.Format=desc.Format;d.SampleDesc=desc.SampleDesc;d.Flags=desc.Flags;
   if(mode==5){
    owned_original_placed1=reinterpret_cast<OwnedPlaced1Fn>((*reinterpret_cast<void***>(d8.Get()))[70]);
    if(FAILED(owned_hook_placed1(d8.Get(),placed_heap.Get(),0,&d,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target))))return 19;
   }else if(mode==6){
    owned_original_committed2=reinterpret_cast<OwnedCommitted2Fn>((*reinterpret_cast<void***>(d8.Get()))[69]);
    if(FAILED(owned_hook_committed2(d8.Get(),&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,nullptr,IID_PPV_ARGS(&target))))return 20;
   }else{
    ComPtr<ID3D12Device4> d4;if(FAILED(device.As(&d4)))return 21;
    owned_original_committed1=reinterpret_cast<OwnedCommitted1Fn>((*reinterpret_cast<void***>(d4.Get()))[53]);
    if(FAILED(owned_hook_committed1(d4.Get(),&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,nullptr,IID_PPV_ARGS(&target))))return 22;
   }
  }else if(FAILED(owned_hook_committed(device.Get(),&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target))))return 7;
 }
 accept(target.Get(),1);if(error||creation_count!=1||creation.api!=(mode==5?3u:mode==6?6u:mode==7?5u:1u)||creation.state!=4)return 8;
 D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;
 if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv))))return 9;
 auto handle=rtv->GetCPUDescriptorHandleForHeapStart();hook_create_rtv(device.Get(),target.Get(),nullptr,handle);
 binding_owners[0].queue=q.Get();q->AddRef();
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
 b.Transition={target.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
 if(mode==1){hook_resource_barrier(list.Get(),1,&b);activate();return !birth_ok&&error==5&&creation.uses==1?0:10;}
 if(xr_mode){if(!owned_xr_probe)return ERROR_NOT_SUPPORTED;stereo_gpu_publish=true;}
 activate();if(error||!birth_ok)return 11;
 gears_vr::GpuReceiver receiver;gears_vr::StereoSample image_sample{};
 if(xr_mode){
  image_sample.session=17;image_sample.sequence=11;image_sample.received_ns=1000000001;image_sample.predicted_time=2000000001;
  image_sample.head_flags=image_sample.view_flags=15;image_sample.focused=image_sample.should_render=1;
  image_sample.head.orientation={};
  for(unsigned eye=0;eye<2;++eye){image_sample.eyes[eye].pose.orientation={};image_sample.eyes[eye].pose.position.x=eye?.032:-.032;image_sample.eyes[eye].fov={-.8,.7,.75,-.65};}
  consuming_frozen_frame.enabled=true;consuming_frozen_frame.id=consuming_frame_ticket=2;consuming_frozen_frame.sample=image_sample;
  consuming_frozen_frame.world_generation=committed_world_generation.load();
  consuming_frozen_frame.source_view={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
  consuming_frozen_frame.source_projection={2,0,0,0,0,3,0,0,0,0,1,1,0,0,-.5f,0};
  for(unsigned eye=0;eye<2;++eye){consuming_frozen_frame.expected.view[eye]=consuming_frozen_frame.source_view;
   consuming_frozen_frame.expected.projection[eye]=consuming_frozen_frame.source_projection;}
  selected(2);built(2);began(2);
  retain_image();if(error||retained_images!=1)return 25;
  if(mode==11)++consuming_frozen_frame.sample.sequence;retain_image();
  if(mode==11)return error==17&&publication_failed()&&publication_count==0?0:24;
  if(error||retained_images!=1||duplicate_images!=1)return 25;
  {StereoPublishBypass bypass;if(!stereo_publisher.prepare())return 26;}
  try{receiver.open(device.Get(),GetCurrentProcessId());}catch(...){return 27;}
 }
 const FLOAT color[]{.25f,.5f,.75f,1};
 if(mode>=22){
  const FLOAT right_color[]{.75f,.25f,.5f,1};
  const D3D12_RECT left{0,0,LONG(eye_width),LONG(source_height)},right{LONG(eye_width),0,LONG(source_width),LONG(source_height)};
  owned_hook_clear(list.Get(),handle,color,1,&left);owned_hook_clear(list.Get(),handle,right_color,1,&right);
 }else owned_hook_clear(list.Get(),handle,color,0,nullptr);
 const auto ref=tracker.current(canonical_identity(list.Get()));
 tracker.draw_final(ref,2,0,{resource,source_width,source_height},{0,0,eye_width,source_height});
 tracker.draw_final(ref,2,1,{resource,source_width,source_height},{eye_width,0,eye_width,source_height});
 if(mode==2)use(list.Get(),resource,true); // Copy-like state use must reject.
 if(mode==3)use(list.Get(),resource,false); // Later MRT write contaminates pair.
 if(mode==4)uncertain(); // Unknown descriptor/enhanced/bundle path.
 if(mode!=10&&mode<19)hook_resource_barrier(list.Get(),1,&b);
 if(FAILED(list->Close()))return 12;
 ID3D12CommandList* members[]{list.Get()};auto token=prepare(q.Get(),1,members);
 q->ExecuteCommandLists(1,members);commit(token);
 wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);swprintf_s(directory,L"%s",temp);
#if defined(GEARS_VR_DOF_COMPAT) && GEARS_VR_DOF_COMPAT
 if(xr_mode){const gears_vr_dof::Key key{publication_world,2};
  // Synthetic no-site metadata is separate from the real GPU clear/copy.
  // Incomplete proof and a changed world must prevent shared publication.
  if(!vr_dof_compat::store.begin(key))return 68;
  if(mode>=22){auto* entry=vr_dof_compat::store.find(key);if(!entry)return 74;
   for(unsigned eye=0;eye<2;++eye){if(!entry->reserve(eye,true))return 74;entry->proved(eye,true,true);
    if(!entry->reserve(eye,false))return 74;entry->proved(eye,false,true);}
  }
  if(mode!=20)vr_dof_compat::store.finish(key);
  vr_dof_compat::retire(); // Desktop boundary cannot retire deferred owned proof.
  if(vr_dof_compat::store.decision(key)!=(mode==20?gears_vr_dof::Decision::incomplete:mode>=22?gears_vr_dof::Decision::zero_proved:gears_vr_dof::Decision::no_site))return 69;
  if(mode==21)++publication_world;
 }
#endif
 copy(true,2);
 // Drain real test clear/transition even for negative cases; production never waits here.
 ComPtr<ID3D12Fence> done;if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&done)))||FAILED(q->Signal(done.Get(),1)))return 13;
 const auto deadline=GetTickCount64()+5000;while(done->GetCompletedValue()<1&&GetTickCount64()<deadline)Sleep(1);
 if(done->GetCompletedValue()!=1)return 14;
 if(mode&&mode<5)return !readback.result().queued&&error==(mode==3?9u:8u)?0:15;
 if(mode==20||mode==21){poll();return error==22&&!published_count&&stereo_publisher.frames.load()==0?0:70;}
 poll();auto const& r=readback.result();
 const bool good=proof.state_certified&&proof.final_state==((mode==10||mode>=19)?4u:128u)&&r.saved&&r.completed&&
  r.width==source_width&&r.height==source_height&&r.eyes[0].nonblack==eye_pixels&&r.eyes[1].nonblack==eye_pixels&&r.differing_pixels==(mode>=22?eye_pixels:0);
 if(xr_mode){
  if(mode==12){receiver.close();stereo_publisher.mark_stopped();stereo_publisher.close();
   return error==18&&publication_count==0&&stereo_publisher.frames.load()==0?0:28;}
  bool received=false;try{received=receiver.acquire(q.Get());}catch(...){return 29;}
  if(!received||published_count!=1||image_tickets.size()||receiver.ticket().id!=2||
     !gears_vr::stereo_same(receiver.ticket().sample,image_sample)||receiver.width!=source_width||receiver.height!=source_height)return 30;
  if(mode>=22){
   for(unsigned eye=0;eye<2;++eye){auto const& rect=receiver.ticket().rect[eye];
    if(rect.x!=int(eye*eye_width)||rect.y||rect.width!=int(eye_width)||rect.height!=int(source_height))return 75;}
#if defined(GEARS_VR_DOF_COMPAT) && GEARS_VR_DOF_COMPAT
   auto const& receipt=vr_dof_compat::last_allowed;
   if(!receipt.valid||!receipt.complete||receipt.ticket!=2||receipt.world!=publication_world||receipt.setters!=3||receipt.quads!=3||
      receipt.decision!=unsigned(gears_vr_dof::Decision::zero_proved)||vr_dof_compat::store.retired!=2||
      vr_dof_compat::store.decision({publication_world,2})!=gears_vr_dof::Decision::missing)return 76;
#endif
  }
  owned_scene_readback::Probe shared_pixels;wchar_t shared_path[MAX_PATH]{};
  swprintf_s(shared_path,L"%s\\owned-shared-%lu.ppm",temp,GetCurrentProcessId());
  if(!shared_pixels.queue(receiver.texture.Get(),q.Get(),D3D12_RESOURCE_STATE_COMMON,true,shared_path))return 31;
  try{receiver.acknowledge(q.Get());}catch(...){return 32;}
  if(FAILED(q->Signal(done.Get(),2)))return 33;
  const auto until=GetTickCount64()+5000;while(done->GetCompletedValue()<2&&GetTickCount64()<until)Sleep(1);
  if(done->GetCompletedValue()!=2)return 34;shared_pixels.poll();const auto& pixels=shared_pixels.result();
  const bool same_pixels=pixels.saved&&pixels.completed&&pixels.eyes[0].hash==r.eyes[0].hash&&pixels.eyes[1].hash==r.eyes[1].hash;
  if(mode>=22){
   if(!good||!same_pixels||pixels.width!=source_width||pixels.height!=source_height||pixels.differing_pixels!=eye_pixels||
      pixels.eyes[0].hash==pixels.eyes[1].hash)return 77;
   printf("{\"owned_rectangle_fixture\":true,\"mode\":%u,\"recommended\":[%u,%u],\"scale\":%.9g,\"source\":[%u,%u],\"eye\":[%u,%u],\"ticket\":2,\"world\":%llu,\"published\":%llu,\"same_ticket_dof_proved\":true,\"dof_retired_after_publication\":true,\"receiver_rectangles_match\":true,\"different_eye_pixels\":%llu,\"matching_source_receiver_eye_hashes\":true,\"actual_game_scene_rendered\":false,\"xr_session_opened\":false}\n",
    mode,fixture_size.recommended_width,fixture_size.recommended_height,fixture_size.scale,source_width,source_height,eye_width,source_height,
    publication_world,published_count,eye_pixels);
   fflush(stdout); // The fixture is also invoked from a Python-hosted DLL.
  }
  if(mode==19){
   // Cross both fixed history capacities using real WARP clears, ordered
   // submissions, shared image copies and receiver acknowledgements. Only
   // paired scene-draw metadata is synthetic; no game/headset claim follows.
   auto* first_cost=render_cost(2);UINT64 fence_value=2;
   auto wait_gpu=[&](){
    if(FAILED(q->Signal(done.Get(),++fence_value)))return false;
    const auto until=GetTickCount64()+5000;
    while(done->GetCompletedValue()<fence_value&&GetTickCount64()<until)Sleep(1);
    return done->GetCompletedValue()==fence_value;
   };
   for(uint64_t iteration=0;iteration<256;++iteration){
    const auto ticket=iteration+3;
    if(FAILED(allocator->Reset())||FAILED(hook_reset_list(list.Get(),allocator.Get(),nullptr)))return 60;
    consuming_frozen_frame.id=consuming_frame_ticket=ticket;
    consuming_frozen_frame.sample.sequence=12+iteration;
    selected(ticket);built(ticket);began(ticket);retain_image();
    owned_hook_clear(list.Get(),handle,color,0,nullptr);
    const auto ref=list_ref(canonical_identity(list.Get()));
    tracker.draw_final(ref,ticket,0,{resource,source_width,source_height},{0,0,eye_width,source_height});
    tracker.draw_final(ref,ticket,1,{resource,source_width,source_height},{eye_width,0,eye_width,source_height});
    if(FAILED(list->Close()))return 61;
    const auto token=prepare(q.Get(),1,members);q->ExecuteCommandLists(1,members);commit(token);
    submitted(ticket);returned(ticket);destroyed(ticket);
#if defined(GEARS_VR_DOF_COMPAT) && GEARS_VR_DOF_COMPAT
    const gears_vr_dof::Key key{publication_world,ticket};
    if(!vr_dof_compat::store.begin(key))return 71;vr_dof_compat::store.finish(key);
    vr_dof_compat::retire();if(vr_dof_compat::store.decision(key)!=gears_vr_dof::Decision::no_site)return 72;
#endif
    copy(true,ticket);
    if(error||finished||consumed_count!=iteration+2||last_consumed_ticket!=ticket||!can_advance(ticket))return 62;
    if(!wait_gpu())return 63;
    try{if(!receiver.acquire(q.Get())||receiver.ticket().id!=ticket||
        !gears_vr::stereo_same(receiver.ticket().sample,consuming_frozen_frame.sample))return 64;
        receiver.acknowledge(q.Get());}catch(...){return 65;}
    if(!wait_gpu())return 66;poll();
   }
   if(consumed_count!=257||publication_attempts!=257||published_count!=257||stored_frames!=frame_limit||
      publication_count!=frame_limit||timing_count!=frame_limit||latest_completed_timing.ticket!=258||
      !timing(258)||!timing(257)||timing(256)||latest_cost_profiled||render_cost(258)||render_cost(2)!=first_cost||
      !last_readback.result().saved||image_tickets.size()||!watching())return 67;
   wchar_t checkpoint[MAX_PATH]{};swprintf_s(checkpoint,L"%s\\owned-scene-%lu-last.ppm",directory,GetCurrentProcessId());DeleteFileW(checkpoint);
  }
  receiver.close();stereo_publisher.mark_stopped();const bool drained=stereo_publisher.drain(1000)&&stereo_publisher.close();
  DeleteFileW(shared_path);if(!same_pixels||!drained)return 35;
 }
 wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\owned-scene-%lu.ppm",directory,GetCurrentProcessId());DeleteFileW(path);
 return good?0:16;
#endif
}
