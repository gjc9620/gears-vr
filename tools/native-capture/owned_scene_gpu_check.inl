#pragma once
// Fresh-process integration checks. The clear is real WARP work; paired draw
// metadata is synthetic, so this tests observation/readback, not scene rendering.
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureOwnedSceneGpuCheck(void* input){
#if !defined(GEARS_NATIVE_OWNED_PIXELS) || !GEARS_NATIVE_OWNED_PIXELS
 (void)input;return ERROR_NOT_SUPPORTED;
#else
 if(!input)return 2;const unsigned mode=*static_cast<unsigned*>(input);if(mode>12)return 2;
 const bool xr_mode=mode>=10;
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
 if(mode==8||mode==9){
  if(!continuous_probe)return ERROR_NOT_SUPPORTED;
  auto receipt=prepare(q.Get(),mode==8?gears_frame_link::Tracker::list_capacity+1:1,nullptr);
  commit(receipt);present(); // No invalid call is sent to the real GPU in this negative test.
  return error==15&&!watching()&&!birth_ok?0:23;
 }
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=4096;desc.Height=2048;
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
  retain_image();if(error||retained_images!=1)return 25;
  if(mode==11)++consuming_frozen_frame.sample.sequence;retain_image();
  if(mode==11)return error==17&&publication_failed()&&publication_count==0?0:24;
  if(error||retained_images!=1||duplicate_images!=1)return 25;
  {StereoPublishBypass bypass;if(!stereo_publisher.prepare())return 26;}
  try{receiver.open(device.Get(),GetCurrentProcessId());}catch(...){return 27;}
 }
 const FLOAT color[]{.25f,.5f,.75f,1};owned_hook_clear(list.Get(),handle,color,0,nullptr);
 const auto ref=tracker.current(canonical_identity(list.Get()));
 tracker.draw_final(ref,2,0,{resource,4096,2048},{0,0,2048,2048});
 tracker.draw_final(ref,2,1,{resource,4096,2048},{2048,0,2048,2048});
 if(mode==2)use(list.Get(),resource,true); // Copy-like state use must reject.
 if(mode==3)use(list.Get(),resource,false); // Later MRT write contaminates pair.
 if(mode==4)uncertain(); // Unknown descriptor/enhanced/bundle path.
 if(mode!=10)hook_resource_barrier(list.Get(),1,&b);
 if(FAILED(list->Close()))return 12;
 ID3D12CommandList* members[]{list.Get()};auto token=prepare(q.Get(),1,members);
 q->ExecuteCommandLists(1,members);commit(token);
 wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);swprintf_s(directory,L"%s",temp);
 copy(true,2);
 // Drain real test clear/transition even for negative cases; production never waits here.
 ComPtr<ID3D12Fence> done;if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&done)))||FAILED(q->Signal(done.Get(),1)))return 13;
 const auto deadline=GetTickCount64()+5000;while(done->GetCompletedValue()<1&&GetTickCount64()<deadline)Sleep(1);
 if(done->GetCompletedValue()!=1)return 14;
 if(mode&&mode<5)return !readback.result().queued&&error==(mode==3?9u:8u)?0:15;
 poll();auto const& r=readback.result();
 const bool good=proof.state_certified&&proof.final_state==(mode==10?4u:128u)&&r.saved&&r.completed&&
  r.eyes[0].nonblack==2048ull*2048&&r.eyes[1].nonblack==2048ull*2048&&r.differing_pixels==0;
 if(xr_mode){
  if(mode==12){receiver.close();stereo_publisher.mark_stopped();stereo_publisher.close();
   return error==18&&publication_count==0&&stereo_publisher.frames.load()==0?0:28;}
  bool received=false;try{received=receiver.acquire(q.Get());}catch(...){return 29;}
  if(!received||published_count!=1||image_tickets.size()||receiver.ticket().id!=2||
     !gears_vr::stereo_same(receiver.ticket().sample,image_sample)||receiver.width!=4096||receiver.height!=2048)return 30;
  owned_scene_readback::Probe shared_pixels;wchar_t shared_path[MAX_PATH]{};
  swprintf_s(shared_path,L"%s\\owned-shared-%lu.ppm",temp,GetCurrentProcessId());
  if(!shared_pixels.queue(receiver.texture.Get(),q.Get(),D3D12_RESOURCE_STATE_COMMON,true,shared_path))return 31;
  try{receiver.acknowledge(q.Get());}catch(...){return 32;}
  if(FAILED(q->Signal(done.Get(),2)))return 33;
  const auto until=GetTickCount64()+5000;while(done->GetCompletedValue()<2&&GetTickCount64()<until)Sleep(1);
  if(done->GetCompletedValue()!=2)return 34;shared_pixels.poll();const auto& pixels=shared_pixels.result();
  const bool same_pixels=pixels.saved&&pixels.completed&&pixels.eyes[0].hash==r.eyes[0].hash&&pixels.eyes[1].hash==r.eyes[1].hash;
  receiver.close();stereo_publisher.mark_stopped();const bool drained=stereo_publisher.drain(1000)&&stereo_publisher.close();
  DeleteFileW(shared_path);if(!same_pixels||!drained)return 35;
 }
 wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\owned-scene-%lu.ppm",directory,GetCurrentProcessId());DeleteFileW(path);
 return good?0:16;
#endif
}
