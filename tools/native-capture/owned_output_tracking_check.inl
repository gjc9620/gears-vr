#pragma once
// Standalone-process production-route tests. No game, owner activation or XR.
#if defined(GEARS_OWNED_OUTPUT_TRACKING) && GEARS_OWNED_OUTPUT_TRACKING
namespace {
unsigned owned_test_execute_calls{},owned_test_barrier_calls{},owned_test_mode{};
bool owned_test_before_commit{};
ID3D12CommandAllocator* owned_test_allocator{};
void STDMETHODCALLTYPE owned_test_barrier(ID3D12GraphicsCommandList*,UINT,const D3D12_RESOURCE_BARRIER*){++owned_test_barrier_calls;}
void STDMETHODCALLTYPE owned_test_execute(ID3D12CommandQueue*,UINT count,ID3D12CommandList* const* members){
    ++owned_test_execute_calls;owned_test_before_commit=frame_link.owned_status().state==4;
    if(owned_test_mode==2&&count==1)
        hook_reset_list(static_cast<ID3D12GraphicsCommandList*>(members[0]),owned_test_allocator,nullptr);
    if(owned_test_mode==3&&count==1){
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
        hook_resource_barrier(static_cast<ID3D12GraphicsCommandList*>(members[0]),1,&b);
    }
}
}
#endif
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureOwnedOutputTrackingCheck(void* input){
#if !defined(GEARS_OWNED_OUTPUT_TRACKING) || !GEARS_OWNED_OUTPUT_TRACKING
    (void)input;return ERROR_NOT_SUPPORTED;
#else
    using Microsoft::WRL::ComPtr;
    if(!input)return 2;const unsigned mode=*static_cast<unsigned*>(input);if(mode>12)return 2;
    // Every test invocation is a fresh process/tracker lifetime, not recovery.
    if(frame_link.snapshot().reset_calls||frame_link.owned_status().enabled)return 10;
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12Resource> owned,other;ComPtr<ID3D12Fence> fence;
    if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))||FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)))||
       FAILED(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))))return 11;
    D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    if(FAILED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)))||
       FAILED(device->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&allocator)))||
       FAILED(device->CreateCommandList(0,q.Type,allocator.Get(),nullptr,IID_PPV_ARGS(&list))))return 12;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=64;desc.Height=32;
    desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if(FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&owned)))||
       FAILED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&other))))return 13;
    auto table=*reinterpret_cast<void***>(list.Get()),queue_table=*reinterpret_cast<void***>(queue.Get());
    original_reset_list=reinterpret_cast<ResetListFn>(table[10]);
    original_resource_barrier=mode==0?reinterpret_cast<BarrierFn>(table[24]):owned_test_barrier;
    original_execute=mode==0?reinterpret_cast<ExecuteFn>(queue_table[10]):owned_test_execute;
    self_testing=true;recording=true;frame_link_diagnostics=true;owned_test_mode=mode;owned_test_allocator=allocator.Get();
    if(FAILED(list->Close())||FAILED(hook_reset_list(list.Get(),allocator.Get(),nullptr)))return 14;
    const gears_frame_link::Target target{canonical_identity(owned.Get()),64,32};
    // Exercise the real-history first birth, preserving the actual Reset ref.
    const auto ref=frame_link.current(canonical_identity(list.Get()));
    if(!frame_link.birth_owned_output(target,canonical_identity(queue.Get()),1,4,true,true)||
       frame_link.current(ref.identity).generation!=ref.generation)return 15;
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition={other.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE};
    hook_resource_barrier(list.Get(),1,&b); // Unrelated transitions must not enter owned state.
    b.Transition.pResource=owned.Get();
    if(mode==1)b.Transition.StateBefore=D3D12_RESOURCE_STATE_COMMON;
    if(mode==4){b.Type=D3D12_RESOURCE_BARRIER_TYPE_ALIASING;b.Aliasing={other.Get(),other.Get()};}
    if(mode==5){b.Type=D3D12_RESOURCE_BARRIER_TYPE_ALIASING;b.Aliasing={nullptr,other.Get()};}
    if(mode==7)b.Flags=D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
    if(mode==8)b.Type=static_cast<D3D12_RESOURCE_BARRIER_TYPE>(99);
    if(mode==9)b.Transition.Subresource=1;
    if(mode==10){b.Type=D3D12_RESOURCE_BARRIER_TYPE_ALIASING;b.Aliasing={owned.Get(),other.Get()};}
    if(mode==11)b.Transition.pResource=reinterpret_cast<ID3D12Resource*>(1);
    hook_resource_barrier(list.Get(),mode==12?1025:1,mode==6?reinterpret_cast<const D3D12_RESOURCE_BARRIER*>(1):&b);
    if(mode==4){b={};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=other.Get();hook_resource_barrier(list.Get(),1,&b);}
    if(FAILED(list->Close()))return 16;
    ID3D12CommandList* members[]{list.Get()};hook_execute(queue.Get(),1,members);
    const auto status=frame_link.owned_status();
    if(mode==0){
        if(!status.certified||status.state!=128)return 17;
        // Actual test GPU fence only; production tracker makes no completion claim.
        if(FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)))||FAILED(queue->Signal(fence.Get(),1)))return 18;
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)return 19;
        const bool done=SUCCEEDED(fence->SetEventOnCompletion(1,event))&&WaitForSingleObject(event,5000)==WAIT_OBJECT_0;
        CloseHandle(event);if(!done)return 20;
    }else{
        if(owned_test_execute_calls!=1||!owned_test_before_commit||owned_test_barrier_calls<(mode==4?3u:2u))return 21;
        if(mode==4){if(!status.certified||status.state!=4)return 22;}
        else if(status.certified||status.state!=4)return 23;
    }
    recording=false;return active.load()?24:0;
#endif
}
